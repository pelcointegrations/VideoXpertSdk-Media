#include "stdafx.h"
#include "GstWrapper.h"

#include <gst/gst.h>
#include <gst/rtp/gstrtcpbuffer.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>
#include <gst/rtsp/gstrtsp.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "libsoup/soup-logger.h"
#include <chrono>
#ifdef WIN32
#include "Winsock2.h"
#else
#include <arpa/inet.h>
#endif

using namespace std;
using namespace MediaController;

void UpdateTextOverlay(GstVars* vars, unsigned int unixTime = 0) {
    if (unixTime == 0)
        unixTime = vars->currentTimestamp;

    if (!vars->pipeline || unixTime != 0 && !vars->includeDateTimeInOverlay || unixTime != 0 && vars->stringToOverlay.length() == 0)
        return;

    GstElement* textOverlay = gst_bin_get_by_name(GST_BIN(vars->pipeline), "textOverlay");
    if (!textOverlay)
        return;

    string overlayText = vars->stringToOverlay;
    if (vars->includeDateTimeInOverlay == true) {
        // The time is universal time, convert to local time
        tm timeStruct = { 0 };
        time_t timeIn = unixTime;
        localtime_s(&timeStruct, &timeIn);

        stringstream overlayStream;
        overlayStream << put_time(&timeStruct, overlayText.c_str());
        overlayText = overlayStream.str();
    }

    boost::trim(overlayText);
    g_object_set(textOverlay, "text", overlayText.c_str(), NULL);
    g_object_set(textOverlay, "valignment", vars->overlayPositionV, NULL);
    g_object_set(textOverlay, "halignment", vars->overlayPositionH, NULL);
    g_object_set(textOverlay, "line-alignment", vars->overlayLineAlignment, NULL);
    g_object_set(textOverlay, "shaded-background", TRUE, NULL);
    g_object_set(textOverlay, "shading-value", 30, NULL);
}

gboolean RTPLossCallback(GstVars* vars) {
    // This means we are done storing the file so stop the pipeline 
    vars->timerId = 0;
    if (gst_element_send_event(vars->videoDecoder, gst_event_new_eos()) == 0)
        g_printerr("Cannot send EOS after storing video \n");

    gst_element_set_state(vars->pipeline, GST_STATE_NULL);
    gst_object_unref(vars->pipeline);
    vars->pipeline = nullptr;
    vars->lastTimestamp = 0;
    g_main_loop_unref(vars->loop);

    for (size_t i = 0; i < vars->streamEventObserverList.size(); i++) {
        StreamEvent* newEvent = new StreamEvent();
        newEvent->eventType = StreamEvent::Type::kFileStoredComplete;
        vars->streamEventObserverList[i](newEvent);
        delete newEvent;
    }

    return FALSE;
}

static gboolean ResetConnectionLostTimer(GstVars* vars) {
    // Timer for stream lost
    if (vars->timerId != 0) {
        g_source_remove(vars->timerId);
        vars->timerId = 0;
    }

    //   Timer is in seconds -------------vv
    vars->timerId = g_timeout_add_seconds(1, (GSourceFunc)RTPLossCallback, vars);
    return TRUE;
}

GstPadProbeReturn OnRtpPacketReceived(GstPad* localPad, GstPadProbeInfo* info, GstVars* vars) {
    if (GST_PAD_PROBE_INFO_TYPE(info) | GST_PAD_PROBE_TYPE_BUFFER) {
        if (vars->isStoringVideo) {
            // Set a faster timeout for the storing video fast case since this is the 
            // most reliable method to know you are done storing data
            ResetConnectionLostTimer(vars);
        }

        GstBuffer* buff = gst_pad_probe_info_get_buffer(info);
        GstRTPBuffer rtp = { nullptr };
        gpointer data;

        if (gst_rtp_buffer_map(buff, GST_MAP_READ, &rtp)) {
            // If the nalValue is 6, test for a Pelco camera and pass back that data
            // The test for Pelco data is PLCO in the data
            gpointer testPtr = gst_rtp_buffer_get_payload(&rtp);
            guint payloadSize = gst_rtp_buffer_get_payload_len(&rtp);
            int nalValue = ((UINT8 *)testPtr)[0];

            if (nalValue == 6) {
                // Pass this back with its own callback
                for (size_t i = 0; i < vars->pelcoDataObserverList.size(); i++) {
                    PelcoDataEvent* newEvent = new PelcoDataEvent();
                    newEvent->eventType = newEvent->kUnknown;
                    newEvent->eventData = vars->eventData;
                    memcpy(newEvent->pelcoData, testPtr, (payloadSize > sizeof(newEvent->pelcoData)) ? sizeof(newEvent->pelcoData) : payloadSize);
                    vars->pelcoDataObserverList[i](newEvent);
                    delete newEvent;
                }
            }

            // Parse the buffer based on the current mode.
            if (vars->mode == MediaController::Controller::kPlayback) {
                // Playback packets contain extension data, which gives us the stream time.
                guint wordLen;
                if (gst_rtp_buffer_get_extension_data(&rtp, nullptr, &data, &wordLen)) {
                    const guint64 unixTime = GST_READ_UINT64_BE(data) - (Constants::kNtpToEpochDiffSec << 32);
                    const unsigned long timeStamp = (unsigned long)gst_util_uint64_scale(unixTime, GST_NSECOND, (G_GINT64_CONSTANT(1) << 32));
                    const unsigned timestampMicroSeconds = (GST_READ_UINT32_BE((guint8*)data + 4) * Constants::kMsInNs) >> 32;
                    vars->currentTimestamp = timeStamp;

                    UpdateTextOverlay(vars);
                    // Send the timestamp event to all observers.
                    for (size_t i = 0; i < vars->observerList.size(); i++) {
                        TimestampEvent* newEvent = new TimestampEvent();
                        newEvent->unixTime = vars->currentTimestamp;
                        newEvent->unixTimeMicroSeconds = timestampMicroSeconds;
                        newEvent->eventData = vars->eventData;
                        vars->observerList[i](newEvent);
                        delete newEvent;
                    }
                }
            }
            else {
                // Check if the packet is a marker packet.
                if (gst_rtp_buffer_get_marker(&rtp)) {
                    // Get the timestamp of the packet.  Note that this is not the actual time of the stream.  It is a
                    // time generated for the session to keep the internal clock in sync.
                    guint32 currentTs = gst_rtp_buffer_get_timestamp(&rtp);
                    guint64 rtcpTsMs = vars->rtcpTimestamp;

                    // If we have not received an RTCP timestamp yet, we set the current time to the time of the local PC.  Otherwise
                    // we calculate the stream time based on the RTCP timestamp and the elapsed time of the buffer clock.
                    if (rtcpTsMs != 0) {
                        if (vars->lastTimestamp != currentTs) {
                            // Get the payload type of the buffer.
                            guint8 payloadType = gst_rtp_buffer_get_payload_type(&rtp);
                            // Set the clock rate to the default value.
                            guint32 clockRate = Constants::kClockRate;
                            // If the payload type is not the standard type then use the default clock rate provided by GStreamer.
                            if (payloadType < Constants::kPayloadType)
                                clockRate = gst_rtp_buffer_default_clock_rate(payloadType);

                            // Calculate the frames per second of the stream based on the elapsed time between timestamps and the clock rate.
                            double fps = clockRate / ((currentTs - vars->lastTimestamp) * 1.0);
                            // Calculate the millisecond duration and add it to the last timestamp.
                            int ms = static_cast<int>((Constants::kMillisecondsFloat / (fps < 1 ? Constants::kMillisecondsInt : fps)) + 0.5);
                            rtcpTsMs += ms;

                            if (rtcpTsMs - ms == vars->rtcpTimestamp)
                                vars->rtcpTimestamp = rtcpTsMs;

                            vars->currentTimestamp = static_cast<unsigned int>(rtcpTsMs / Constants::kMillisecondsInt);
                            UpdateTextOverlay(vars);
                            // Send the timestamp event to all observers.
                            for (size_t i = 0; i < vars->observerList.size(); i++) {
                                TimestampEvent* newEvent = new TimestampEvent();
                                newEvent->unixTime = vars->currentTimestamp;
                                newEvent->unixTimeMicroSeconds = 0;
                                newEvent->eventData = vars->eventData;
                                vars->observerList[i](newEvent);
                                delete newEvent;
                            }
                        }
                    }
                    else {
                        vars->rtcpTimestamp = Utilities::CurrentUnixTime();
                        vars->rtcpTimestamp *= Constants::kMillisecondsInt;
                    }
                    // Set the lastTimestamp value to the newly generated value.
                    vars->lastTimestamp = currentTs;
                }
            }
            gst_rtp_buffer_unmap(&rtp);
        }
    }
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn OnJpegPacketReceived(GstPad* localPad, GstPadProbeInfo* info, GstVars* vars) {
    // Get the event info if available.
    GstEvent* event = GST_PAD_PROBE_INFO_EVENT(info);
    const GstStructure* baseEvent = gst_event_get_structure(event);
    if (baseEvent == nullptr)
        return GST_PAD_PROBE_OK;

    // Check if the event contains an http-headers element.
    if (g_ascii_strncasecmp(gst_structure_get_name(baseEvent), Constants::kHttpHeaders, sizeof Constants::kHttpHeaders) == 0) {
        // Get the response-headers element and verify it has the Content-Disposition header.
        const GstStructure* responseHeaders = gst_value_get_structure(gst_structure_get_value(baseEvent, Constants::kResponseHeaders));
        if (gst_structure_has_field(responseHeaders, Constants::kHeaderResourceTimestamp)) {
            string timestamp(gst_structure_get_string(responseHeaders, Constants::kHeaderResourceTimestamp));
            if (timestamp.length() > 23)
                timestamp = timestamp.substr(0, 23);

            const boost::posix_time::ptime parsedTime = boost::posix_time::from_iso_extended_string(timestamp);
            static boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
            unsigned long secs = static_cast<unsigned long>((parsedTime - epoch).total_seconds());
            if (vars->mode == Controller::kLive)
                secs -= Utilities::TzOffset();

            const unsigned int currentTimestampMicroSeconds = static_cast<unsigned>(parsedTime.time_of_day().fractional_seconds());
            vars->currentTimestamp = static_cast<unsigned long>(secs);

            // Send the timestamp event to all observers.
            for (size_t i = 0; i < vars->observerList.size(); i++) {
                if (vars->currentTimestamp == 0)
                    break;

                for (size_t ii = 0; ii < vars->observerList.size(); ii++) {
                    TimestampEvent* newEvent = new TimestampEvent();
                    newEvent->unixTime = vars->currentTimestamp;
                    newEvent->unixTimeMicroSeconds = currentTimestampMicroSeconds;
                    newEvent->eventData = vars->eventData;
                    vars->observerList[ii](newEvent);
                    delete newEvent;
                }
            }
        }

        UpdateTextOverlay(vars);
    }

    return GST_PAD_PROBE_OK;
}

gboolean OnBusMessage(GstBus* bus, GstMessage* msg, GstVars* vars) {
    // GStreamer does not support MJPEG pull sources.  After receiving a JPEG from the server we receive an EOS (End of Stream) message
    // since no further images will be pushed out.  To work around this we set the pipeline to state to NULL when we get an EOS event
    // and then set it back to PLAYING.  This reinitializes the pipeline and it fetches a new image and the process repeats.  We also do
    // this when an error is received since it has the same effect of stopping the pipeline.
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
        case GST_MESSAGE_ERROR:
            // Set the pipeline to ready then back to playing.
            gst_element_set_state(vars->pipeline, GST_STATE_READY);
            gst_element_set_state(vars->pipeline, GST_STATE_PLAYING);
            break;
        case GST_MESSAGE_ELEMENT:
            if (vars->isMjpeg)
                break;

            const GstStructure* s = gst_message_get_structure(msg);
            if (gst_structure_has_name(s, "GstBinForwarded")) {
                GstMessage* forward_msg = NULL;

                gst_structure_get(s, "message", GST_TYPE_MESSAGE, &forward_msg, NULL);
                if (GST_MESSAGE_TYPE(forward_msg) == GST_MESSAGE_EOS) {
                    gchar* elementName = GST_OBJECT_NAME(GST_MESSAGE_SRC(forward_msg));
                    g_print("EOS from element %s\n", elementName);
                    if (strcmp(elementName, "snapshotFilesink") == 0 || strcmp(elementName, "recordFilesink") == 0) {
                        g_signal_handler_disconnect(G_OBJECT(bus), vars->busWatchId);
                        if (vars->teeQueue) {
                            gst_element_set_state(vars->teeQueue, GST_STATE_NULL);
                            gst_bin_remove(GST_BIN(vars->pipeline), vars->teeQueue);
                            gst_object_unref(vars->teeQueue);
                            vars->teeQueue = nullptr;
                        }

                        if (vars->encoder) {
                            gst_element_set_state(vars->encoder, GST_STATE_NULL);
                            gst_bin_remove(GST_BIN(vars->pipeline), vars->encoder);
                            gst_object_unref(vars->encoder);
                            vars->encoder = nullptr;
                        }

                        if (vars->muxer) {
                            gst_element_set_state(vars->muxer, GST_STATE_NULL);
                            gst_bin_remove(GST_BIN(vars->pipeline), vars->muxer);
                            gst_object_unref(vars->muxer);
                            vars->muxer = nullptr;
                        }

                        if (vars->fileSink) {
                            gst_element_set_state(vars->fileSink, GST_STATE_NULL);
                            gst_bin_remove(GST_BIN(vars->pipeline), vars->fileSink);
                            gst_object_unref(vars->fileSink);
                            vars->fileSink = nullptr;
                        }

                        if (vars->videoTeePad) {
                            gst_element_release_request_pad(vars->videoTee, vars->videoTeePad);
                            gst_object_unref(vars->videoTeePad);
                            vars->videoTeePad = nullptr;
                        }

                        vars->isRecording = false;
                    }

                    g_main_loop_unref(vars->loop);
                }

                gst_message_unref(forward_msg);
            }
            break;
    }

    return true;
}

static void OnTimeOut(GObject* session, GObject* source, GstVars* vars) {
    GST_DEBUG_OBJECT(vars->pipeline, "Session Timed Out");
    // Send the event to all observers.
    for (size_t i = 0; i < vars->streamEventObserverList.size(); i++) {
        StreamEvent* newEvent = new StreamEvent();
        newEvent->eventType = StreamEvent::kConnectionLost;
        vars->streamEventObserverList[i](newEvent);
        delete newEvent;
    }
}

static void OnNewManager(GstElement* rtspsrc, GstElement* mgr, GstVars* vars) {
    if (g_signal_lookup("get-internal-session", G_OBJECT_TYPE(mgr)) != 0) {
        GObject* rtpSession;
        // In our case, will be zero
        g_signal_emit_by_name(mgr, "get-internal-session", 0, &rtpSession);
        if (rtpSession != NULL) {
            GST_DEBUG_OBJECT(vars->pipeline, "Set up Time Out Callback");
            g_signal_connect(rtpSession, "on-bye-timeout", (GCallback)OnTimeOut, vars);
            g_signal_connect(rtpSession, "on-timeout", (GCallback)OnTimeOut, vars);
        }
        else {
            GST_ERROR_OBJECT(vars->pipeline, "No Timeout callback - will never get session lost callback");
        }
    }
}

static void OnPadAdded(GstElement* element, GstPad* new_pad, GstVars* vars) {
    g_print("New payload on pad: %s\n", GST_PAD_NAME(new_pad));

    GstPad* sinkpad;
    gchar* name = gst_element_get_name(element);
    if (g_str_equal(name, "videoSource")) {
        // Setup a probe on the Pad for RTP packets (will give time and metadata)
        gst_pad_add_probe(new_pad, GST_PAD_PROBE_TYPE_BUFFER, GstPadProbeCallback(OnRtpPacketReceived), vars, nullptr);
        sinkpad = gst_element_get_static_pad(vars->videoDecoder, Constants::kSink);
    }
    else {
        sinkpad = gst_element_get_static_pad(vars->videoTee, Constants::kSink);
    }

    g_assert(sinkpad);
    gst_pad_link(new_pad, sinkpad);
    gst_object_unref(sinkpad);

    // Get session and connect to stream lost signals
    if (vars->rtpBinManager) {
        if (g_signal_lookup("get-internal-session", G_OBJECT_TYPE(vars->rtpBinManager)) != 0) {
            GObject* rtpSession = nullptr;
            // In our case, will be zero
            g_signal_emit_by_name(vars->rtpBinManager, "get-internal-session", 0, &rtpSession);
            if (rtpSession) {
                GST_DEBUG_OBJECT(vars->pipeline, "Set up Time Out Callback");
                g_signal_connect(rtpSession, "on-bye-timeout", (GCallback)OnTimeOut, vars);
                g_signal_connect(rtpSession, "on-timeout", (GCallback)OnTimeOut, vars);
            }
            else {
                GST_ERROR_OBJECT(vars->pipeline, "No Timeout callback - will never get session lost callback");
            }
        }
    }
}

static gboolean OnBeforeSend(GstElement* rtspsrc, GstRTSPMessage* message, GstVars* vars) {
    if (message->type == GST_RTSP_MESSAGE_REQUEST && message->type_data.request.method == GST_RTSP_PLAY && !vars->isPaused) {
        g_print("Speed: %f\n", vars->speed);
        g_print("SeekTime: %i\n", vars->seekTime);
        g_print("StoreVideoFast: %s\n", vars->isStoringVideo ? "True" : "False");

        // If playback
        if (vars->seekTime != 0) {
            // Set Range header
            string timeStr = Utilities::UnixTimeToRfc3339(vars->seekTime);
            string range = "clock=";
            range += timeStr.c_str();
            if (vars->isStoringVideo) {
                timeStr = Utilities::UnixTimeToRfc3339(vars->endTime);
                // Has a dash at the end
                timeStr = timeStr.substr(0, timeStr.length() - 1);
                range += timeStr.c_str();
            }

            gst_rtsp_message_remove_header(message, GST_RTSP_HDR_RANGE, -1);
            gst_rtsp_message_add_header(message, GST_RTSP_HDR_RANGE, range.c_str());

            // Set Rate-Control
            if (vars->isStoringVideo) {
                gst_rtsp_message_remove_header_by_name(message, Constants::kHeaderRateControl, -1);
                gst_rtsp_message_add_header_by_name(message, Constants::kHeaderRateControl, Constants::kRateControlValue);
            }

            // Set Scale
            string scale = vars->speed < 0 ? "-1.0" : "1.0";
            if (!vars->isStoringVideo) {
                stringstream ss;
                ss << setprecision(1) << fixed << vars->speed;
                scale = ss.str();
                g_print("Scale: %s\n", scale.c_str());
            }

            gst_rtsp_message_remove_header(message, GST_RTSP_HDR_SCALE, -1);
            gst_rtsp_message_add_header(message, GST_RTSP_HDR_SCALE, scale.c_str());

            // Set Frames
            stringstream frames;
            if (vars->speed < 0)
                frames << Constants::kIntraPrefix << Constants::kForwardSlash << abs(static_cast<int>(vars->speed * Constants::kIntraFrameDiv));
            else
                frames << Constants::kFramesAllValue;

            gst_rtsp_message_remove_header_by_name(message, Constants::kHeaderFrames, -1);
            gst_rtsp_message_add_header_by_name(message, Constants::kHeaderFrames, frames.str().c_str());

            g_print("Frames: %s\n", frames.str().c_str());
        }
    }

    vars->isPaused = false;
    return true;
}

static GstPadProbeReturn OnUnlink(GstPad* pad, GstPadProbeInfo* info, GstVars* vars) {
    GstPad* sinkpad = gst_element_get_static_pad(vars->teeQueue, Constants::kSink);
    gst_pad_unlink(pad, sinkpad);
    gst_object_unref(sinkpad);

    gst_element_send_event(vars->encoder, gst_event_new_eos());

    return GST_PAD_PROBE_REMOVE;
}

static GstBusSyncReply OnPrepareWindow(GstBus* bus, GstMessage* message, GstVars* vars) {
    // ignore anything but 'prepare-window-handle' element messages
    if (!gst_is_video_overlay_prepare_window_handle_message(message))
        return GST_BUS_PASS;

    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(message)), vars->windowHandle);

    vars->videoSink = (GstElement*)GST_MESSAGE_SRC(message);
    g_object_set(vars->videoSink, "force-aspect-ratio", !vars->stretchToFit, NULL);

    gst_message_unref(message);

    return GST_BUS_DROP;
}

static void OnSourceSetup(GstElement* element, GstElement* source, GstVars* vars) {
    g_signal_connect(source, "before-send", G_CALLBACK(OnBeforeSend), vars);
    if ((vars->speed != 1.0) || (vars->seekTime == 0)) {
        // Will get smoother operation if latency is smaller when the playback is not 1.0
        // Also, want a small latency for live
        g_object_set(source, "latency", 100, NULL);
    }
}

#ifdef WIN32
extern "C" gboolean plugin_init_d3d(GstPlugin * plugin);
#endif

GstWrapper::GstWrapper() {
    SetMode(Controller::kStopped);
    if (!gst_is_initialized()) {
        gst_init(nullptr, nullptr);
#ifdef WIN32
        gboolean d3dresult = gst_plugin_register_static(1, 2, "d3dvideosinkpelco", "Pelco d3dvideosink", plugin_init_d3d, "3", "LGPL", "Source", "package", "origin");
        if (d3dresult == FALSE) {
            g_printerr("Cannot register plugin Pelco d3dvideosink");
        }
#endif
    }
}

GstWrapper::~GstWrapper() = default;

void GstWrapper::CreateRtspPipeline(float speed, unsigned int seekTime, MediaRequest request, IStream::RTSPNetworkTransport transport) {
    // Create the pipeline.
    _gstVars.pipeline = gst_pipeline_new(nullptr);
    _gstVars.videoSource = gst_element_factory_make(Constants::kRtspSrc, "videoSource");
    _gstVars.videoDecoder = gst_element_factory_make(Constants::kDecodeBin, "videoDecoder");
    _gstVars.videoTee = gst_element_factory_make(Constants::kTee, "videoTee");
    _gstVars.videoQueue = gst_element_factory_make(Constants::kQueue, "videoQueue");
    GstElement* textOverlay = gst_element_factory_make(Constants::kTextOverlay, "textOverlay");
    GstElement* aspectRatioCrop = gst_element_factory_make(Constants::kAspectRatioCrop, "aspectRatioCrop");
    GstElement* videoConvert = gst_element_factory_make(Constants::kVideoConvert, "videoConvert");
    GstElement* videoSink = gst_element_factory_make(Constants::kVideoSink, "videoSink");
    if (!_gstVars.pipeline || !_gstVars.videoSource || !_gstVars.videoDecoder || !_gstVars.videoTee || !_gstVars.videoQueue || !textOverlay || !aspectRatioCrop || !videoConvert || !videoSink) {
        g_printerr("An element could not be created\n");
        return;
    }

    _gstVars.speed = speed;
    _gstVars.seekTime = seekTime;
    _gstVars.currentTimestamp = 0;
    _gstVars.isMjpeg = false;
    g_object_set(GST_BIN(_gstVars.pipeline), "message-forward", TRUE, NULL);
    g_object_set(videoSink, "sync", FALSE, NULL);
    g_object_set(_gstVars.videoSource, "location", request.dataInterface.dataEndpoint, NULL);
    if (transport == IStream::RTSPNetworkTransport::kUDP)
        g_object_set(_gstVars.videoSource, "protocols", (request.dataInterface.supportsMulticast ? GST_RTSP_LOWER_TRANS_UDP_MCAST : GST_RTSP_LOWER_TRANS_UDP), NULL);
    else
        g_object_set(_gstVars.videoSource, "protocols", GST_RTSP_LOWER_TRANS_TCP, NULL);
   
    if ((_gstVars.speed != 1.0) || (_gstVars.seekTime == 0)) {
        // Will get smoother operation if latency is smaller when the playback is not 1.0
        // Also, want a small latency for live
        g_object_set(_gstVars.videoSource, "latency", 100, NULL);
    }

    gst_bin_add_many(GST_BIN(_gstVars.pipeline), _gstVars.videoSource, _gstVars.videoDecoder, _gstVars.videoTee, _gstVars.videoQueue, textOverlay, aspectRatioCrop, videoConvert, videoSink, NULL);
    gst_element_link_many(_gstVars.videoTee, _gstVars.videoQueue, textOverlay, aspectRatioCrop, videoConvert, videoSink, NULL);

    UpdateTextOverlay(&_gstVars);
    SetAspectRatio(_gstVars.aspectRatio);

    g_signal_connect(_gstVars.videoSource, "new-manager", G_CALLBACK(OnNewManager), &_gstVars);
    g_signal_connect(_gstVars.videoSource, "before-send", G_CALLBACK(OnBeforeSend), &_gstVars);
    g_signal_connect(_gstVars.videoSource, "pad-added", G_CALLBACK(OnPadAdded), &_gstVars);
    g_signal_connect(_gstVars.videoDecoder, "pad-added", G_CALLBACK(OnPadAdded), &_gstVars);

    if (request.audioDataSource != nullptr && !std::string(request.audioDataInterface.dataEndpoint).empty())
    {
        _gstVars.audioSource = gst_element_factory_make(Constants::kPlaybin, "audioSource");
        if (_gstVars.audioSource) {
            gst_bin_add_many(GST_BIN(_gstVars.pipeline), _gstVars.audioSource, NULL);
            g_object_set(_gstVars.audioSource, "uri", request.audioDataInterface.dataEndpoint, NULL);
            g_signal_connect(_gstVars.audioSource, "source-setup", G_CALLBACK(OnSourceSetup), &_gstVars);
        }
    }

    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_gstVars.pipeline));
    gst_bus_set_sync_handler(bus, (GstBusSyncHandler)OnPrepareWindow, &_gstVars, NULL);
    gst_object_unref(bus);

    g_print("Created RTSP pipeline.\n");
}

void GstWrapper::CreateMjpegPipeline(float speed, char* jpegUri) {
    // Create the pipeline.
    _gstVars.pipeline = gst_pipeline_new(nullptr);
    _gstVars.videoSource = gst_element_factory_make(Constants::kHttpSrc, "videoSource");
    _gstVars.videoDecoder = gst_element_factory_make(Constants::kJpegDec, "jpegDecoder");
    GstElement* textOverlay = gst_element_factory_make(Constants::kTextOverlay, "textOverlay");
    GstElement* videoConvert = gst_element_factory_make(Constants::kVideoConvert, "videoConvert");
    GstElement* videoSink = gst_element_factory_make(Constants::kVideoSink, "videoSink");
    if (!_gstVars.pipeline || !_gstVars.videoSource || !_gstVars.videoDecoder || !textOverlay || !videoConvert || !videoSink) {
        g_printerr("An element could not be created\n");
        return;
    }

    _gstVars.speed = speed;
    _gstVars.isMjpeg = true;
    static const char* cookie[] = { _gstVars.cookie.c_str(), NULL };
    g_object_set(GST_BIN(_gstVars.pipeline), "message-forward", TRUE, NULL);
    g_object_set(_gstVars.videoSource, Constants::kCookies, cookie, NULL);
    g_object_set(_gstVars.videoSource, Constants::kRetries, 5, NULL);
    g_object_set(_gstVars.videoSource, Constants::kKeepAlive, TRUE, NULL);
    g_object_set(_gstVars.videoSource, Constants::kLocation, jpegUri, NULL);
    g_object_set(_gstVars.videoSource, Constants::kHttpLogLevel, SOUP_LOGGER_LOG_HEADERS, NULL);
    g_object_set(_gstVars.videoSource, Constants::kSslStrict, FALSE, NULL);

    // Add elements to the pipeline and link.
    gst_bin_add_many(GST_BIN(_gstVars.pipeline), _gstVars.videoSource, _gstVars.videoDecoder, textOverlay, videoConvert, videoSink, NULL);
    gst_element_link_many(_gstVars.videoSource, _gstVars.videoDecoder, textOverlay, videoConvert, videoSink, NULL);

    UpdateTextOverlay(&_gstVars);

    // Add a probe to souphttpsrc.
    GstPad* httpsrcpad = gst_element_get_static_pad(_gstVars.videoSource, Constants::kSrc);
    gst_pad_add_probe(httpsrcpad, GST_PAD_PROBE_TYPE_EVENT_BOTH, GstPadProbeCallback(OnJpegPacketReceived), &_gstVars, nullptr);
    gst_object_unref(httpsrcpad);

    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_gstVars.pipeline));
    gst_bus_add_signal_watch(bus);
    _gstVars.busWatchId = g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(OnBusMessage), &_gstVars);
    gst_bus_set_sync_handler(bus, (GstBusSyncHandler)OnPrepareWindow, &_gstVars, NULL);
    gst_object_unref(bus);

    // Start the loop to receive bus messages.
    _gstVars.loop = g_main_loop_new(nullptr, FALSE);
    boost::thread _workerThread(g_main_loop_run, _gstVars.loop);
    g_print("Created MJPEG pipeline.\n");
}

bool GstWrapper::StartLocalRecord(char* filePath, char* fileName) {
    if (_gstVars.isRecording)
        return false;

    boost::filesystem::path recordingPath = boost::filesystem::path(filePath);
    if (!exists(recordingPath))
        if (!create_directories(recordingPath))
            return false;

    GstPadTemplate* padTemplate = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(_gstVars.videoTee), "src_%u");
    _gstVars.videoTeePad = gst_element_request_pad(_gstVars.videoTee, padTemplate, NULL, NULL);
    _gstVars.teeQueue = gst_element_factory_make(Constants::kQueue, "recordQueue");
    _gstVars.encoder = gst_element_factory_make(Constants::kX264Enc, "recordEncoder");
    _gstVars.muxer = gst_element_factory_make(Constants::kMp4Mux, "recordMuxer");
    _gstVars.fileSink = gst_element_factory_make(Constants::kFilesink, "recordFilesink");

    g_object_set(_gstVars.fileSink, "location", recordingPath.append(std::string(fileName) + ".mp4").generic_string().c_str(), NULL);
    g_object_set(_gstVars.encoder, "tune", 4, NULL);

    gst_bin_add_many(GST_BIN(_gstVars.pipeline), GST_ELEMENT(gst_object_ref(_gstVars.teeQueue)), gst_object_ref(_gstVars.encoder), gst_object_ref(_gstVars.muxer), gst_object_ref(_gstVars.fileSink), NULL);
    gst_element_link_many(_gstVars.teeQueue, _gstVars.encoder, _gstVars.muxer, _gstVars.fileSink, NULL);

    gst_element_sync_state_with_parent(_gstVars.teeQueue);
    gst_element_sync_state_with_parent(_gstVars.encoder);
    gst_element_sync_state_with_parent(_gstVars.muxer);
    gst_element_sync_state_with_parent(_gstVars.fileSink);

    GstPad* sinkpad = gst_element_get_static_pad(_gstVars.teeQueue, Constants::kSink);
    GstPadLinkReturn linkReturn = gst_pad_link(_gstVars.videoTeePad, sinkpad);
    gst_object_unref(sinkpad);

    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_gstVars.pipeline));
    gst_bus_add_signal_watch(bus);
    _gstVars.busWatchId = g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(OnBusMessage), &_gstVars);
    gst_object_unref(bus);

    // Start the loop to receive bus messages.
    _gstVars.loop = g_main_loop_new(nullptr, FALSE);
    boost::thread _workerThread(g_main_loop_run, _gstVars.loop);

    _gstVars.isRecording = true;
    return true;
}

void GstWrapper::StopLocalRecord() {
    if (_gstVars.isRecording)
        gst_pad_add_probe(_gstVars.videoTeePad, GST_PAD_PROBE_TYPE_IDLE, GstPadProbeCallback(OnUnlink), &_gstVars, nullptr);
}

bool GstWrapper::SnapShot(char* filePath, char* fileName) {
    // Bunch of tries with g-streamer - cannot get file to work though equivalent works on the command line
    if (!_gstVars.pipeline || _gstVars.isRecording)
        return false;

    boost::filesystem::path snapshotPath = boost::filesystem::path(filePath);
    if (!exists(snapshotPath))
        if (!create_directories(snapshotPath))
            return false;

    //   This g-streamer seems to work.  It is very important to set snapshot to true.
    //   Also, you must have set the window to the media controller (which sets the window handler for gstreamr)
    //   to store JPEGs or video.
    GstPadTemplate* padTemplate = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(_gstVars.videoTee), "src_%u");
    _gstVars.videoTeePad = gst_element_request_pad(_gstVars.videoTee, padTemplate, NULL, NULL);
    _gstVars.teeQueue = gst_element_factory_make(Constants::kQueue, "snapshotQueue");
    _gstVars.encoder = gst_element_factory_make(Constants::kJpegEnc, "snapshotEncoder");
    _gstVars.fileSink = gst_element_factory_make(Constants::kFilesink, "snapshotFilesink");

    g_object_set(_gstVars.encoder, "snapshot", 1, NULL);
    g_object_set(_gstVars.fileSink, "location", snapshotPath.append(std::string(fileName) + ".jpg").generic_string().c_str(), NULL);

    gst_bin_add_many(GST_BIN(_gstVars.pipeline), GST_ELEMENT(gst_object_ref(_gstVars.teeQueue)), gst_object_ref(_gstVars.encoder), gst_object_ref(_gstVars.fileSink), NULL);
    gst_element_link_many(_gstVars.teeQueue, _gstVars.encoder, _gstVars.fileSink, NULL);

    gst_element_sync_state_with_parent(_gstVars.teeQueue);
    gst_element_sync_state_with_parent(_gstVars.encoder);
    gst_element_sync_state_with_parent(_gstVars.fileSink);

    GstPad* sinkPad = gst_element_get_static_pad(_gstVars.teeQueue, Constants::kSink);
    if (gst_pad_link(_gstVars.videoTeePad, sinkPad) != GST_PAD_LINK_OK) {
        g_printerr("\nLink To Pad FAILED in JPEG snapshot function\n");
        return false;
    }
    gst_object_unref(sinkPad);

    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_gstVars.pipeline));
    gst_bus_add_signal_watch(bus);
    _gstVars.busWatchId = g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(OnBusMessage), &_gstVars);
    gst_object_unref(bus);
    
    // Start the loop to receive bus messages.
    _gstVars.loop = g_main_loop_new(nullptr, FALSE);
    boost::thread _workerThread(g_main_loop_run, _gstVars.loop);

    // So how do you know if the file is done?  We will actually look for the file, or wait for a timeout
    // Even if you timeout, we still want to send an EOS to tear down correctly
    for (int i = 0; i < Constants::kSnapshotTimeoutMs / Constants::kSnapshotSleepTimeMs; i++) {
        if (exists(snapshotPath) == true && file_size(snapshotPath) > 10) {
            break;
        }
        Sleep(Constants::kSnapshotSleepTimeMs);
    }

    // Disconnect from the tee when it is idle, so probe for an idle condition in the tee
    gst_pad_add_probe(_gstVars.videoTeePad, GST_PAD_PROBE_TYPE_IDLE, GstPadProbeCallback(OnUnlink), &_gstVars, nullptr);

    return true;
}

bool GstWrapper::StoreVideo(char* filePath, char* fileName, unsigned int startTime, unsigned int endTime, MediaRequest request) {
    boost::filesystem::path recordingPath = boost::filesystem::path(filePath);
    if (!exists(recordingPath)) {
        if (!create_directories(recordingPath)) {
            g_printerr("Cannot make directory\n");
            return false;
        }
    }

    // Create the pipeline.
    _gstVars.pipeline = gst_pipeline_new(nullptr);
    _gstVars.videoSource = gst_element_factory_make(Constants::kRtspSrc, "videoSource");
    _gstVars.videoDecoder = gst_element_factory_make(Constants::kH264Depay, "videoDepay");
    _gstVars.videoParse = gst_element_factory_make(Constants::kH264Parse, "videoParse");
    _gstVars.muxer = gst_element_factory_make(Constants::kMkvMux, "storageMuxer");
    _gstVars.fileSink = gst_element_factory_make(Constants::kFilesink, "storageFilesink");
    if (!_gstVars.pipeline || !_gstVars.videoSource || !_gstVars.videoDecoder || !_gstVars.videoParse || !_gstVars.muxer || !_gstVars.fileSink) {
        g_printerr("An element could not be created\n");
        return false;
    }

    _gstVars.seekTime = startTime;
    _gstVars.endTime = endTime;
    _gstVars.isStoringVideo = true;
    g_object_set(GST_BIN(_gstVars.pipeline), "message-forward", TRUE, NULL);
    g_object_set(_gstVars.videoSource, "location", request.dataInterface.dataEndpoint, NULL);
    g_object_set(_gstVars.videoSource, "protocols", 0x4, NULL);
    g_object_set(_gstVars.videoSource, "latency", 100, NULL);
    g_object_set(_gstVars.videoParse, "config-interval", 1, NULL);
    g_object_set(_gstVars.videoParse, "disable-passthrough", 1, NULL);
    g_object_set(_gstVars.fileSink, "location", recordingPath.append(std::string(fileName) + ".mkv").generic_string().c_str(), NULL);
    g_object_set(_gstVars.fileSink, "sync", 0, NULL);
    g_object_set(_gstVars.fileSink, "qos", 0, NULL);

    gst_bin_add_many(GST_BIN(_gstVars.pipeline), _gstVars.videoSource, _gstVars.videoDecoder, _gstVars.videoParse, _gstVars.muxer, _gstVars.fileSink, NULL);
    gst_element_link_many(_gstVars.videoDecoder, _gstVars.videoParse, _gstVars.muxer, _gstVars.fileSink, NULL);

    g_signal_connect(_gstVars.videoSource, "pad-added", G_CALLBACK(OnPadAdded), &_gstVars);
    g_signal_connect(_gstVars.videoSource, "before-send", G_CALLBACK(OnBeforeSend), &_gstVars);
    g_signal_connect(_gstVars.videoSource, "new-manager", G_CALLBACK(OnNewManager), &_gstVars);

    // Start the loop to receive bus messages.
    _gstVars.loop = g_main_loop_new(nullptr, FALSE);
    boost::thread _workerThread(g_main_loop_run, _gstVars.loop);

    g_print("Created store video pipeline\n");
    return true;
}

void GstWrapper::ClearPipeline() {
    if (_gstVars.timerId != 0) {
        g_source_remove(_gstVars.timerId);
        _gstVars.timerId = 0;
    }

    if (_gstVars.pipeline) {
        g_print("Stopping the pipeline.\n");
        if (_gstVars.busWatchId > 0) {
            GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_gstVars.pipeline));
            gst_bus_remove_signal_watch(bus);
            gst_object_unref(bus);
        }

        if (_gstVars.isMjpeg)
            Sleep(Constants::kMjpegShutdownSleepTimeMs);

        if (_gstVars.isStoringVideo) {
            // This means we are done storing the file; stop the pipeline.
            if (gst_element_send_event(_gstVars.videoDecoder, gst_event_new_eos()) == 0)
                g_printerr("Cannot send EOS after storing video \n");

            return;
        }

        StopLocalRecord();
        gst_element_set_state(_gstVars.pipeline, GST_STATE_NULL);
        gst_object_unref(_gstVars.pipeline);
        _gstVars.pipeline = nullptr;
        if (_gstVars.loop)
            g_main_loop_unref(_gstVars.loop);      
    }
}

void GstWrapper::Play() {
    if (_gstVars.pipeline) {
        gst_element_set_state(_gstVars.pipeline, GST_STATE_PLAYING);
    }
}
void GstWrapper::Pause() {
    // Do not allow pause when storing to a file
    if (!_gstVars.isStoringVideo) {
        gst_element_set_state(_gstVars.pipeline, GST_STATE_PAUSED);
        _gstVars.isPaused = true;
    }
}

void GstWrapper::AddObserver(TimestampEventCallback observer) {
    _gstVars.observerList.push_back(observer);
}

void GstWrapper::RemoveObserver(TimestampEventCallback observer) {
    _gstVars.observerList.erase(remove(_gstVars.observerList.begin(), _gstVars.observerList.end(), observer), _gstVars.observerList.end());
}

void GstWrapper::AddPelcoDataObserver(PelcoDataEventCallback observer) {
    _gstVars.pelcoDataObserverList.push_back(observer);
}

void GstWrapper::RemovePelcoDataObserver(PelcoDataEventCallback observer) {
    _gstVars.pelcoDataObserverList.erase(remove(_gstVars.pelcoDataObserverList.begin(), _gstVars.pelcoDataObserverList.end(), observer), _gstVars.pelcoDataObserverList.end());
}

void GstWrapper::AddStreamObserver(StreamEventCallback observer) {
    _gstVars.streamEventObserverList.push_back(observer);
}

void GstWrapper::RemoveStreamObserver(StreamEventCallback observer) {
    _gstVars.streamEventObserverList.erase(remove(_gstVars.streamEventObserverList.begin(), _gstVars.streamEventObserverList.end(), observer), _gstVars.streamEventObserverList.end());
}

void GstWrapper::ClearObservers() {
    _gstVars.observerList.clear();
    _gstVars.pelcoDataObserverList.clear();
}

void GstWrapper::AddEventData(void* customData) {
    _gstVars.eventData = customData;
}

bool GstWrapper::SetOverlayString(string stringToOverlay, IController::VideoOverlayDataPosition position, bool includeDateTime) {
    switch (position) {
        case MediaController::IController::kTopLeft:
            _gstVars.overlayPositionV = 2;
            _gstVars.overlayPositionH = 0;
            _gstVars.overlayLineAlignment = 0;
            break;
        case MediaController::IController::kTopCenter:
            _gstVars.overlayPositionV = 2;
            _gstVars.overlayPositionH = 1;
            _gstVars.overlayLineAlignment = 1;
            break;
        case MediaController::IController::kTopRight:
            _gstVars.overlayPositionV = 2;
            _gstVars.overlayPositionH = 2;
            _gstVars.overlayLineAlignment = 2;
            break;
        case MediaController::IController::kMiddleLeft:
            _gstVars.overlayPositionV = 4;
            _gstVars.overlayPositionH = 0;
            _gstVars.overlayLineAlignment = 0;
            break;
        case MediaController::IController::kMiddleCenter:
            _gstVars.overlayPositionV = 4;
            _gstVars.overlayPositionH = 1;
            _gstVars.overlayLineAlignment = 1;
            break;
        case MediaController::IController::kMiddleRight:
            _gstVars.overlayPositionV = 4;
            _gstVars.overlayPositionH = 2;
            _gstVars.overlayLineAlignment = 2;
            break;
        case MediaController::IController::kBottomLeft:
            _gstVars.overlayPositionV = 1;
            _gstVars.overlayPositionH = 0;
            _gstVars.overlayLineAlignment = 0;
            break;
        case MediaController::IController::kBottomCenter:
            _gstVars.overlayPositionV = 1;
            _gstVars.overlayPositionH = 1;
            _gstVars.overlayLineAlignment = 1;
            break;
        case MediaController::IController::kBottomRight:
            _gstVars.overlayPositionV = 1;
            _gstVars.overlayPositionH = 2;
            _gstVars.overlayLineAlignment = 2;
            break;
    }

    _gstVars.stringToOverlay = stringToOverlay;
    _gstVars.includeDateTimeInOverlay = includeDateTime;
    UpdateTextOverlay(&_gstVars);

    return true;
}

void GstWrapper::SetAspectRatio(Controller::AspectRatios aspectRatio) {
    _gstVars.aspectRatio = aspectRatio;
    if (_gstVars.pipeline) {
        GstElement* aspectRatioCrop = gst_bin_get_by_name(GST_BIN(_gstVars.pipeline), "aspectRatioCrop");
        switch (aspectRatio) {
            case IController::k4x3:
                g_object_set(aspectRatioCrop, "aspect-ratio", 4, 3, NULL);
                break;
            case IController::k1x1:
                g_object_set(aspectRatioCrop, "aspect-ratio", 1, 1, NULL);
                break;
            case IController::k3x2:
                g_object_set(aspectRatioCrop, "aspect-ratio", 3, 2, NULL);
                break;
            case IController::k5x4:
                g_object_set(aspectRatioCrop, "aspect-ratio", 5, 4, NULL);
                break;
            case IController::k16x9:
            default:
                g_object_set(aspectRatioCrop, "aspect-ratio", 16, 9, NULL);
                break;
        }
    }
}

void GstWrapper::SetStretchToFit(bool stretchToFit) {
    _gstVars.stretchToFit = stretchToFit;
    if (_gstVars.pipeline && _gstVars.videoSink)
        g_object_set(_gstVars.videoSink, "force-aspect-ratio", !stretchToFit, NULL);
}

void GstWrapper::SetWindowHandle(guintptr winhandle) {
    _gstVars.windowHandle = winhandle;
}

void GstWrapper::SetCookie(std::string cookie) {
    _gstVars.cookie = cookie;
}

void GstWrapper::SetTimestamp(unsigned int seekTime) {
    _gstVars.currentTimestamp = seekTime;
}

void GstWrapper::SetMode(Controller::Mode mode) {
    _gstVars.mode = mode;
}

void GstWrapper::SetSpeed(float speed) {
    if (_gstVars.pipeline && speed != 0) {
        _gstVars.speed = speed;
        if (_gstVars.isMjpeg)
            return;

        _gstVars.seekTime = _gstVars.currentTimestamp;
        gst_element_set_state(_gstVars.pipeline, GST_STATE_PAUSED);
        gst_element_get_state(_gstVars.pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
        gst_element_set_state(_gstVars.pipeline, GST_STATE_PLAYING);
    }
}