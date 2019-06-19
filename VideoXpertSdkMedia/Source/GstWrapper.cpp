#include "stdafx.h"
#include "GstWrapper.h"

#include <gst/gst.h>
#include <gst/rtp/gstrtcpbuffer.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include "libsoup/soup-logger.h"
#include <chrono>
#ifdef WIN32
#include "Winsock2.h"
#else
#include <arpa/inet.h>
#endif

using namespace std;
using namespace MediaController;

GstPadProbeReturn OnRtpPacketReceived(GstPad *localPad, GstPadProbeInfo *info, GstVars *vars) {
    if (GST_PAD_PROBE_INFO_TYPE(info) | GST_PAD_PROBE_TYPE_BUFFER) {
        GstBuffer *buff = gst_pad_probe_info_get_buffer(info);
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
                    // Convert the time contained in the extension data from NTP format to Unix format.
                    vars->currentTimestamp = ntohl(*reinterpret_cast<unsigned long*>(data)) - Constants::kNtpToEpochDiffSec;
                    unsigned int curTime = vars->currentTimestamp;
                    // Send the timestamp event to all observers.
                    for (size_t i = 0; i < vars->observerList.size(); i++) {
                        TimestampEvent* newEvent = new TimestampEvent();
                        newEvent->unixTime = curTime;
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

                            // Send the timestamp event to all observers.
                            for (size_t i = 0; i < vars->observerList.size(); i++) {
                                TimestampEvent* newEvent = new TimestampEvent();
                                newEvent->unixTime = static_cast<unsigned int>(rtcpTsMs / Constants::kMillisecondsInt);
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

long TzOffset() {
    tm utcTm{ 0 }, localTm{ 0 };
    time_t local = time(nullptr);
    gmtime_s(&utcTm, &local);
    localtime_s(&localTm, &local);
    localTm.tm_isdst = 0;
    local = mktime(&localTm);
    time_t utc = mktime(&utcTm);
    return static_cast<long>(difftime(local, utc));
}

GstPadProbeReturn OnJpegPacketReceived(GstPad *localPad, GstPadProbeInfo *info, GstVars *vars) {
    // Get the event info if available.
    GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
    const GstStructure *baseEvent = gst_event_get_structure(event);
    if (baseEvent == nullptr)
        return GST_PAD_PROBE_OK;

    // Check if the event contains an http-headers element.
    if (g_ascii_strncasecmp(gst_structure_get_name(baseEvent), Constants::kHttpHeaders, sizeof Constants::kHttpHeaders) == 0) {
        // Get the response-headers element and verify it has the Content-Disposition header.
        const GstStructure *responseHeaders = gst_value_get_structure(gst_structure_get_value(baseEvent, Constants::kResponseHeaders));
        if (gst_structure_has_field(responseHeaders, Constants::kHeaderContentDisposition)) {
            // Parse the Content-Disposition header value.
            string content(gst_structure_get_string(responseHeaders, Constants::kHeaderContentDisposition));
            // Parse the timestamp contained within the Content-Disposition header value.
            boost::regex e("dataSession_(.*)Z_dataSourceId");
            boost::smatch what;
            if (regex_search(content, what, e, boost::match_partial)) {
                boost::smatch::iterator it = what.begin();
                ++it;
                string timestamp = *it;
                if (timestamp.empty())
                    return GST_PAD_PROBE_OK;

                // Format the timestamp string and convert it to a ptime value.
                boost::replace_all(timestamp, "T", Constants::kWhitespace);
                timestamp = timestamp.substr(0, timestamp.size() - 4);
                boost::posix_time::ptime t(boost::posix_time::time_from_string(timestamp));
                // Convert the time to a Unix timestamp.
                static boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
                unsigned int streamTs = static_cast<unsigned int>((t - epoch).total_seconds());
                // The timestamp is the current time on the server.  So we must adjust it for playback.
                if (vars->mode == Controller::kPlayback) {
                    // For playback, the initial time is set using the seek time.  It is then incremented based on the elapsed time between
                    // timestamps and the playback speed.
                    if (vars->lastTimestamp != 0 && vars->lastTimestamp != streamTs) {
                        vars->currentTimestamp += abs(static_cast<int>(streamTs - vars->lastTimestamp)) * static_cast<int>(vars->speed);
                    }
                }
                else {
                    // Substract one second from the time to adjust for latency.
                    vars->currentTimestamp = streamTs - 1;
                }
                // Set the lastTimestamp value to the newly generated value.
                vars->lastTimestamp = streamTs;
            }
        }
        else if (gst_structure_has_field(responseHeaders, Constants::kHeaderResourceTimestamp)) {
            string timestamp(gst_structure_get_string(responseHeaders, Constants::kHeaderResourceTimestamp));
            if (timestamp.length() > 24)
                timestamp = timestamp.substr(0, timestamp.size() - 6);

            long long unixTime = 0;
            tm localTm;
            time_t local = time(nullptr);
            localtime_s(&localTm, &local);
            stringstream timeStream(timestamp);
            timeStream >> get_time(&localTm, "%Y-%m-%dT%H:%M:%S");
            if (timeStream.good()) {
                auto timePoint = chrono::system_clock::from_time_t(mktime(&localTm));
                unixTime = chrono::duration_cast<chrono::seconds>(timePoint.time_since_epoch()).count();
                if (vars->mode == Controller::kPlayback) {
                    unixTime += TzOffset();
                }
            }

            vars->currentTimestamp = static_cast<unsigned long>(unixTime);
            vars->lastTimestamp = static_cast<uint32_t>(unixTime);
        }
    }
    return GST_PAD_PROBE_OK;
}

// This is used as a 'cheap' way to make "SnapShot" a blocking call
static bool _jpegIsWritten = false;
void OnBusMessage(GstBus *bus, GstMessage *msg, GstVars *vars) {
    // GStreamer does not support MJPEG pull sources.  After receiving a JPEG from the server we receive an EOS (End of Stream) message
    // since no further images will be pushed out.  To work around this we set the pipeline to state to NULL when we get an EOS event
    // and then set it back to PLAYING.  This reinitializes the pipeline and it fetches a new image and the process repeats.  We also do
    // this when an error is received since it has the same effect of stopping the pipeline.
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
    case GST_MESSAGE_ERROR:
        // Set the pipeline to NULL.
        gst_element_set_state(vars->pipeline, GST_STATE_READY);
        // Send the latest timestamp parsed in the OnJpegPacketReceived method.
        for (size_t i = 0; i < vars->observerList.size(); i++) {
            if (vars->currentTimestamp == 0)
                break;

            // Send the timestamp event to all observers.
            for (size_t ii = 0; ii < vars->observerList.size(); ii++) {
                TimestampEvent* newEvent = new TimestampEvent();
                newEvent->unixTime = vars->currentTimestamp;
                newEvent->eventData = vars->eventData;
                vars->observerList[ii](newEvent);
                delete newEvent;
            }
        }
        // Set the pipeline back to playing.
        gst_element_set_state(vars->pipeline, GST_STATE_PLAYING);
        break;
    case GST_MESSAGE_ELEMENT: {
        const GstStructure *s = gst_message_get_structure(msg);
        if (gst_structure_has_name(s, "GstBinForwarded")) {
            GstMessage *forward_msg = NULL;

            gst_structure_get(s, "message", GST_TYPE_MESSAGE, &forward_msg, NULL);
            if (GST_MESSAGE_TYPE(forward_msg) == GST_MESSAGE_EOS) {
                g_print("EOS from element %s\n", GST_OBJECT_NAME(GST_MESSAGE_SRC(forward_msg)));
                if (strcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(forward_msg)), "fileSinkJPEG") == 0) {
                    gst_element_set_state(vars->queueSnapShot, GST_STATE_NULL);
                    gst_element_set_state(vars->encSnapShot, GST_STATE_NULL);
                    gst_element_set_state(vars->fileSinkSnapShot, GST_STATE_NULL);

                    gst_bin_remove(GST_BIN(vars->pipeline), vars->queueSnapShot);
                    gst_bin_remove(GST_BIN(vars->pipeline), vars->encSnapShot);
                    gst_bin_remove(GST_BIN(vars->pipeline), vars->fileSinkSnapShot);

                    gst_object_unref(vars->queueSnapShot);
                    gst_object_unref(vars->encSnapShot);
                    gst_object_unref(vars->fileSinkSnapShot);

                    gst_element_release_request_pad(vars->tee, vars->teePadSnapShot);
                    gst_object_unref(vars->teePadSnapShot);
                    _jpegIsWritten = true;
                }
                else if (strcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(forward_msg)), "filesinkVideoRecord") == 0) {
                    gst_element_set_state(vars->queueRecord, GST_STATE_NULL);
                    gst_element_set_state(vars->x264enc, GST_STATE_NULL);
                    gst_element_set_state(vars->mkvMux, GST_STATE_NULL);
                    gst_element_set_state(vars->fileSink, GST_STATE_NULL);

                    gst_bin_remove(GST_BIN(vars->pipeline), vars->queueRecord);
                    gst_bin_remove(GST_BIN(vars->pipeline), vars->x264enc);
                    gst_bin_remove(GST_BIN(vars->pipeline), vars->mkvMux);
                    gst_bin_remove(GST_BIN(vars->pipeline), vars->fileSink);

                    gst_object_unref(vars->queueRecord);
                    gst_object_unref(vars->x264enc);
                    gst_object_unref(vars->mkvMux);
                    gst_object_unref(vars->fileSink);

                    gst_element_release_request_pad(vars->tee, vars->teePad);
                    gst_object_unref(vars->teePad);

                    vars->isRecording = false;
                }
                g_main_loop_unref(vars->loop);

            }
            gst_message_unref (forward_msg);
        }
        break;
    }
    default:
        break;
    }
}

// Called when rtpbin has validated a payload that we can depayload.
static void OnPadAddedAudio(GstElement * rtpbin, GstPad * new_pad, GstElement * depay) {
    g_print("New payload on pad: %s\n", GST_PAD_NAME(new_pad));
    GstPad *sinkpad = gst_element_get_static_pad(depay, Constants::kSink);
    g_assert(sinkpad);
    gst_pad_link(new_pad, sinkpad);
    gst_object_unref(sinkpad);
}

// Called when rtpbin has validated a payload that we can depayload.
static void OnPadAddedVideo(GstElement * rtpbin, GstPad * new_pad, GstVars * vars) {
    g_print("New payload on pad: %s\n", GST_PAD_NAME(new_pad));
    GstPad *sinkpad = gst_element_get_static_pad(vars->videoDepay, Constants::kSink);
    g_assert(sinkpad);
    gst_pad_link(new_pad, sinkpad);
    gst_object_unref(sinkpad);

    // Setup a probe on the Pad for RTP packets (will give time and metadata)
    gst_pad_add_probe(new_pad, GST_PAD_PROBE_TYPE_BUFFER, GstPadProbeCallback(OnRtpPacketReceived), vars, nullptr);
}

static GstPadProbeReturn OnUnlink(GstPad *pad, GstPadProbeInfo *info, GstVars *vars) {
    GstPad *sinkpad = gst_element_get_static_pad(vars->queueRecord, "sink");
    gst_pad_unlink(vars->teePad, sinkpad);
    gst_object_unref(sinkpad);

    gst_element_send_event(vars->x264enc, gst_event_new_eos());

    return GST_PAD_PROBE_REMOVE;
}

static GstPadProbeReturn OnUnlinkSnapShot(GstPad *pad, GstPadProbeInfo *info, GstVars *vars) {
    GstPad *sinkpad = gst_element_get_static_pad(vars->queueSnapShot, "sink");
    gst_pad_unlink(vars->teePadSnapShot, sinkpad);
    gst_object_unref(sinkpad);

    gst_element_send_event(vars->encSnapShot, gst_event_new_eos());

    return GST_PAD_PROBE_REMOVE;
}

static GstBusSyncReply create_window(GstBus * bus, GstMessage * message, GstVars *vars) {
    // ignore anything but 'prepare-window-handle' element messages
    if (!gst_is_video_overlay_prepare_window_handle_message(message))
        return GST_BUS_PASS;

    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(message)), vars->windowHandle);

    gst_message_unref(message);

    return GST_BUS_DROP;
}

GstWrapper::GstWrapper() {
    SetMode(Controller::kStopped);
    Init();
    _gstVars.isRecording = false;
    _gstVars.isPipelineActive = false;
}

GstWrapper::~GstWrapper() { }

void GstWrapper::Init() {
    if (!gst_is_initialized())
        gst_init(nullptr, nullptr);
}

void GstWrapper::SetWindowHandle(guintptr winhandle) {
    _gstVars.windowHandle = winhandle;
}

void GstWrapper::SetLocation(std::string location) {
    _gstVars.location = location;
}

void GstWrapper::SetPorts(int port, int port2) {
    _gstVars.rtpPort = port;
    _gstVars.rtcpPort = port2;
    _gstVars.rtcpSinkPort = port2 + 4;
}

void GstWrapper::SetCaps(std::string caps, bool isMjpeg) {
    _gstVars.rtpCaps = "application/x-rtp,media=(string)" + caps;
    _gstVars.isMjpeg = isMjpeg;
}

void GstWrapper::SetCookie(std::string cookie) {
    _gstVars.cookie = cookie;
}

void GstWrapper::SetRtcpHostIP(std::string hostIp) {
    _gstVars.hostIp = hostIp;
}

void GstWrapper::SetMulticastAddress(std::string multicastAddress) {
    _gstVars.multicastAddress = multicastAddress;
}

void GstWrapper::SetTimestamp(unsigned int seekTime) {
    _gstVars.currentTimestamp = seekTime;
    _gstVars.lastTimestamp = NULL;
}

unsigned int GstWrapper::GetLastTimestamp() const {
    // If the protocol is MjpegPull do not convert the timestamp.
    if (_gstVars.protocol == VxSdk::VxStreamProtocol::kMjpegPull) {
        return _gstVars.currentTimestamp;
    }
    // If the current mode is playback do not convert the timestamp.
    if (_gstVars.mode == Controller::kPlayback) {
        return _gstVars.currentTimestamp;
    }

    return static_cast<unsigned int>(_gstVars.rtcpTimestamp / Constants::kMillisecondsInt);
}

void GstWrapper::SetMode(Controller::Mode mode) {
    _gstVars.rtcpTimestamp = 0;
    _gstVars.mode = mode;
}

bool GstWrapper::IsPipelineActive() const {
    return _gstVars.isPipelineActive;
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

void GstWrapper::ClearObservers() {
    _gstVars.observerList.clear();
    _gstVars.pelcoDataObserverList.clear();
}

void GstWrapper::AddEventData(void* customData) {
    _gstVars.eventData = customData;
}

bool GstWrapper::StartLocalRecord(char* filePath, char* fileName) {
    if (_gstVars.isRecording)
        return false;

    boost::filesystem::path logPath = boost::filesystem::path(filePath);
    if (!exists(logPath))
        if (!create_directories(logPath))
            return false;

    _gstVars.recordingFilePath = logPath.append(std::string(fileName) + ".mp4").generic_string();

    GstPadTemplate *padTemplate = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(_gstVars.tee), "src_%u");
    _gstVars.teePad = gst_element_request_pad(_gstVars.tee, padTemplate, NULL, NULL);
    _gstVars.queueRecord = gst_element_factory_make("queue", "queueRecord");
    _gstVars.x264enc = gst_element_factory_make("x264enc", NULL);
    _gstVars.mkvMux = gst_element_factory_make("mp4mux", NULL);
    _gstVars.fileSink = gst_element_factory_make("filesink", "filesinkVideoRecord");
    g_object_set(_gstVars.fileSink, "location", _gstVars.recordingFilePath.c_str(), NULL);
    g_object_set(_gstVars.x264enc, "tune", 4, NULL);
    _gstVars.recordingFilePath = "";
    
    gst_bin_add_many(GST_BIN(_gstVars.pipeline), GST_ELEMENT(gst_object_ref(_gstVars.queueRecord)), gst_object_ref(_gstVars.x264enc), gst_object_ref(_gstVars.mkvMux), gst_object_ref(_gstVars.fileSink), NULL);
    gst_element_link_many(_gstVars.queueRecord, _gstVars.x264enc, _gstVars.mkvMux, _gstVars.fileSink, NULL);

    gst_element_sync_state_with_parent(_gstVars.queueRecord);
    gst_element_sync_state_with_parent(_gstVars.x264enc);
    gst_element_sync_state_with_parent(_gstVars.mkvMux);
    gst_element_sync_state_with_parent(_gstVars.fileSink);

    GstPad *sinkpad = gst_element_get_static_pad(_gstVars.queueRecord, "sink");
    GstPadLinkReturn linkReturn = gst_pad_link(_gstVars.teePad, sinkpad);
    if (linkReturn == GST_PAD_LINK_OK) g_printerr("Link To Pad ALL Okay");
    gst_object_unref(sinkpad);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_gstVars.pipeline));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(OnBusMessage), &_gstVars);
    gst_object_unref(bus);

    // Start the loop to receive bus messages.
    _gstVars.loop = g_main_loop_new(nullptr, FALSE);
    boost::thread _workerThread(g_main_loop_run, _gstVars.loop);

    _gstVars.isRecording = true;
    return true;
}

void GstWrapper::StopLocalRecord() {
    if (_gstVars.isRecording)
        gst_pad_add_probe(_gstVars.teePad, GST_PAD_PROBE_TYPE_IDLE, GstPadProbeCallback(OnUnlink), &_gstVars, nullptr);
}

// Note - this call will not return until the file is written
//
// This is to keep SnapShot threadsafe - only one call at a time
static boost::mutex _snapShotMutex;
bool GstWrapper::SnapShot(char* filePath, char* fileName) {

    // Bunch of tries with g-streamer - cannot get file to work though equivalent works on the command line
    if (_gstVars.isPipelineActive == false)
        return false;

    boost::filesystem::path logPath = boost::filesystem::path(filePath);
    if (!exists(logPath))
        if (!create_directories(logPath))
            return false;

    _snapShotMutex.lock();
    _jpegIsWritten = false;

    //   This g-streamer seems to work.  It is very important to set snapshot to true.
    //   Also, you must have set the window to the media controller (which sets the window handler for gstreamr)
    //   to store JPEGs or video.
    GstPadTemplate *padTemplate = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(_gstVars.tee), "src_%u");
    _gstVars.teePadSnapShot = gst_element_request_pad(_gstVars.tee, padTemplate, NULL, NULL);
    _gstVars.queueSnapShot = gst_element_factory_make("queue", "queueSnapShot");
    _gstVars.encSnapShot = gst_element_factory_make("jpegenc", NULL);
    _gstVars.fileSinkSnapShot = gst_element_factory_make("filesink", "fileSinkJPEG");
    g_object_set(_gstVars.encSnapShot, "snapshot", 1, NULL);
    g_object_set(_gstVars.fileSinkSnapShot, "location", logPath.append(std::string(fileName) + ".jpg").generic_string().c_str(), NULL);

    gst_bin_add_many(GST_BIN(_gstVars.pipeline), GST_ELEMENT(gst_object_ref(_gstVars.queueSnapShot)), gst_object_ref(_gstVars.encSnapShot), gst_object_ref(_gstVars.fileSinkSnapShot), NULL);
    gst_element_link_many(_gstVars.queueSnapShot, _gstVars.encSnapShot, _gstVars.fileSinkSnapShot, NULL);

    gst_element_sync_state_with_parent(_gstVars.queueSnapShot);
    gst_element_sync_state_with_parent(_gstVars.encSnapShot);
    gst_element_sync_state_with_parent(_gstVars.fileSinkSnapShot);

    GstPad *sinkpad = gst_element_get_static_pad(_gstVars.queueSnapShot, "sink");
    GstPadLinkReturn linkReturn = gst_pad_link(_gstVars.teePadSnapShot, sinkpad);
    if (linkReturn != GST_PAD_LINK_OK) {
        g_printerr("\nLink To Pad FAILED in JPEG snapshot function\n");
        _snapShotMutex.unlock();
        return false;
    }
    gst_object_unref(sinkpad);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_gstVars.pipeline));
    if (bus == NULL) {
        g_printerr("\nCannot get bus in SnapShot\n\n");
        _snapShotMutex.unlock();
        return false;
    }
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(OnBusMessage), &_gstVars);
    gst_object_unref(bus);

    // Start the loop to receive bus messages.
    _gstVars.loop = g_main_loop_new(nullptr, FALSE);
    boost::thread _workerThread(g_main_loop_run, _gstVars.loop);

    // So how do you know if the file is done?  We will actually look for the file, or wait for a timeout
    //   Even if you timeout, we still want to send an EOS to tear down correctly
    const int timeoutMs = 5 * 1000;
    const int sleepTimeMs = 100;
    int i = 0;
    for (; i < timeoutMs / sleepTimeMs; i++) {
        if ((boost::filesystem::exists(logPath) == true) && (boost::filesystem::file_size(logPath)) > 10) {
            break;
        }
        Sleep(sleepTimeMs);
    }
    if (i == (timeoutMs / sleepTimeMs)) {
        g_printerr("Timeout Waiting for JPEG file to exist\n");
    }
    
    // Disconnect from the tee when it is idle, so probe for an idle condition in the tee
    gst_pad_add_probe(_gstVars.teePadSnapShot, GST_PAD_PROBE_TYPE_IDLE, GstPadProbeCallback(OnUnlinkSnapShot), &_gstVars, nullptr);

    // Now wait for the EOS message in the loop
    i = 0;
    for (; i < timeoutMs / sleepTimeMs; i++) {
        if (_jpegIsWritten == true)
        {
            _jpegIsWritten = false;
            break;
        }
        Sleep(sleepTimeMs);
    }

    if (i == (timeoutMs / sleepTimeMs)) {
        g_printerr("Timed Out trying to write JPEG\n");
        _snapShotMutex.unlock();
        return false;
    }

    _snapShotMutex.unlock();
    return true;
}

void GstWrapper::CreatePipeline() {
    // Create the pipeline.
    _gstVars.pipeline = gst_pipeline_new(nullptr);
    g_assert(_gstVars.pipeline);
    _gstVars.isPipelineActive = true;

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_gstVars.pipeline));
    gst_bus_set_sync_handler(bus, (GstBusSyncHandler)create_window, &_gstVars, NULL);
    gst_object_unref(bus);
}

void GstWrapper::CreateVideoRtspPipeline(string encoding) {
    // Create the pipeline.
    CreatePipeline();

    // Determine which depayloader and decoder to use based on the encoding type.
    const char* videoDepayName;
    const char* videoDecName;
    if (encoding == Constants::kEncodingJpeg) {
        videoDepayName = Constants::kRtpJpegDepay;
        videoDecName = Constants::kJpegDec;
    }
    else if (encoding == Constants::kEncodingMpeg) {
        videoDepayName = Constants::kRtpMp4vDepay;
        videoDecName = Constants::kRtpMp4vDec;
    }
    else if (encoding == Constants::kEncodingH265) {
        videoDepayName = Constants::kRtpH265Depay;
        videoDecName = Constants::kRtpH265Dec;        
    }
    else {
        // Default case: (encoding == Constants::kEncodingH264)        
        videoDepayName = Constants::kRtpH264Depay;
        videoDecName = Constants::kRtpH264Dec;
    }

    // Create the depayloader, decoder and video sink.
    _gstVars.rtspSrc = gst_element_factory_make("rtspsrc", "RTSPSrc");
    g_assert(_gstVars.rtspSrc);
    g_object_set(_gstVars.rtspSrc, "location", _gstVars.uriControl.c_str(), NULL);
    if (_gstVars.transport == MediaController::IController::kUDP) {
        // Check for multicast
        if (!_gstVars.multicastAddress.empty()) {
            // g_object_set(_gstVars.rtspSrc, Constants::kAddress, _gstVars.multicastAddress.c_str(), NULL);
            g_object_set(_gstVars.rtspSrc, "protocols", 0x2, NULL);
        }
    }
    else {
        g_object_set(_gstVars.rtspSrc, "protocols", 0x4, NULL);
    }

    _gstVars.videoDepay = gst_element_factory_make(videoDepayName, "videoDepay");
    g_assert(_gstVars.videoDepay);
    _gstVars.videoDec = gst_element_factory_make(videoDecName, "videoDec");
    g_assert(_gstVars.videoDec);
    _gstVars.tee = gst_element_factory_make("tee", "tee");
    g_assert(_gstVars.tee);
    _gstVars.queueView = gst_element_factory_make("queue", "queueView");
    g_assert(_gstVars.queueView);
    _gstVars.videoConvert = gst_element_factory_make(Constants::kVideoConvert, "videoConvert");
    g_assert(_gstVars.videoConvert);
    _gstVars.videoSink = gst_element_factory_make(Constants::kVideoSink, "videoSink");
    g_assert(_gstVars.videoSink);

    gst_bin_add_many(GST_BIN(_gstVars.pipeline), _gstVars.rtspSrc, _gstVars.videoDepay, _gstVars.videoDec, _gstVars.tee, _gstVars.queueView, _gstVars.videoConvert, _gstVars.videoSink, NULL);
    gst_element_link_many(_gstVars.videoDepay, _gstVars.videoDec, _gstVars.tee, _gstVars.queueView, _gstVars.videoConvert, _gstVars.videoSink, NULL);

    // The RTP pad that connects to the depayloader will be created dynamically.
    // So connect to the pad-added signal and pass the depayloader to link to it.
    g_signal_connect(_gstVars.rtspSrc, "pad-added", G_CALLBACK(OnPadAddedVideo), &_gstVars);

    g_object_set(GST_BIN(_gstVars.pipeline), "message-forward", TRUE, NULL);
    _gstVars.protocol = VxSdk::VxStreamProtocol::kRtspRtp;
    g_print("Starting RTP video receiver pipeline.\n");
}

void GstWrapper::CreateAudioRtspPipeline() {
    // Create the pipeline.
    CreatePipeline();

    _gstVars.rtspSrc = gst_element_factory_make("rtspsrc", "RTSPSrc");
    g_assert(_gstVars.rtspSrc);
    g_object_set(_gstVars.rtspSrc, "location", _gstVars.uriControl.c_str(), NULL);
    if (_gstVars.transport == MediaController::IController::kUDP) {
        // Check for multicast
        if (!_gstVars.multicastAddress.empty()) {
            // g_object_set(_gstVars.rtspSrc, Constants::kAddress, _gstVars.multicastAddress.c_str(), NULL);
            g_object_set(_gstVars.rtspSrc, "protocols", 0x2, NULL);
        }
    }
    else {
        g_object_set(_gstVars.rtspSrc, "protocols", 0x4, NULL);
    }
    // Create the depayloader, decoder and audio sink.
    _gstVars.audioDepay = gst_element_factory_make(Constants::kRtpAudioDepay, "audioDepay");
    g_assert(_gstVars.audioDepay);
    _gstVars.audioDec = gst_element_factory_make(Constants::kRtpAudioDec, "audioDec");
    g_assert(_gstVars.audioDec);
    _gstVars.audioSink = gst_element_factory_make(Constants::kAudioSink, "audioSink");
    g_assert(_gstVars.audioSink);

    // Add elements to the pipeline and link.
    gst_bin_add_many(GST_BIN(_gstVars.pipeline), _gstVars.rtspSrc, _gstVars.audioDepay, _gstVars.audioDec, _gstVars.audioSink, NULL);
    gst_element_link_many(_gstVars.audioDepay, _gstVars.audioDec, _gstVars.audioSink, NULL);

    // So connect to the pad-added signal and pass the depayloader to link to it.
    g_signal_connect(_gstVars.rtspSrc, "pad-added", G_CALLBACK(OnPadAddedAudio), _gstVars.audioDepay);

    _gstVars.protocol = VxSdk::VxStreamProtocol::kRtspRtp;
    g_print("Starting RTP audio receiver pipeline.\n");
}

void GstWrapper::CreateMjpegPipeline() {
    // Create the pipeline.
    _gstVars.pipeline = gst_pipeline_new(nullptr);
    g_assert(_gstVars.pipeline);
    _gstVars.isPipelineActive = true;
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_gstVars.pipeline));
    _gstVars.busWatchId = gst_bus_add_watch(bus, GstBusFunc(OnBusMessage), &_gstVars);
    gst_bus_set_sync_handler(bus, (GstBusSyncHandler)create_window, &_gstVars, NULL);
    gst_object_unref(bus);

    // Create the souphttpsrc.
    _gstVars.src = gst_element_factory_make(Constants::kHttpSrc, "httpSrc");
    g_assert(_gstVars.src);
    g_object_set(_gstVars.src, Constants::kRetries, 5, NULL);
    g_object_set(_gstVars.src, Constants::kKeepAlive, TRUE, NULL);
    g_object_set(_gstVars.src, Constants::kLocation, _gstVars.location.c_str(), NULL);
    g_object_set(_gstVars.src, Constants::kHttpLogLevel, SOUP_LOGGER_LOG_HEADERS, NULL);
    g_object_set(_gstVars.src, Constants::kSslStrict, FALSE, NULL);
    static const char *cookie[] = { _gstVars.cookie.c_str(), NULL };
    g_object_set(_gstVars.src, Constants::kCookies, cookie, NULL);

    // Create the decoder and video sink.
    _gstVars.videoDec = gst_element_factory_make(Constants::kJpegDec, "videoDec");
    g_assert(_gstVars.videoDec);
    _gstVars.videoConvert = gst_element_factory_make(Constants::kVideoConvert, "videoConvert");
    g_assert(_gstVars.videoConvert);
    _gstVars.videoSink = gst_element_factory_make(Constants::kVideoSink, "videoSink");
    g_assert(_gstVars.videoSink);

    // Add elements to the pipeline and link.
    gst_bin_add_many(GST_BIN(_gstVars.pipeline), _gstVars.src, _gstVars.videoDec, _gstVars.videoConvert, _gstVars.videoSink, NULL);
    gst_element_link_many(_gstVars.src, _gstVars.videoDec, _gstVars.videoConvert, _gstVars.videoSink, NULL);

    // Add a probe to souphttpsrc.
    GstPad *httpsrcpad = gst_element_get_static_pad(_gstVars.src, Constants::kSrc);
    gst_pad_add_probe(httpsrcpad, GST_PAD_PROBE_TYPE_EVENT_BOTH, GstPadProbeCallback(OnJpegPacketReceived), &_gstVars, nullptr);
    gst_object_unref(httpsrcpad);

    // Start the loop to receive bus messages.
    _gstVars.loop = g_main_loop_new(nullptr, FALSE);
    boost::thread _workerThread(g_main_loop_run, _gstVars.loop);

    _gstVars.protocol = VxSdk::VxStreamProtocol::kMjpegPull;
    g_print("Starting MJPEG receiver pipeline.\n");
}

void GstWrapper::Play(float speed) {
    _gstVars.rtcpTimestamp = 0;
    _gstVars.speed = speed;

    if (_gstVars.pipeline) {
        gst_element_set_state(_gstVars.pipeline, GST_STATE_PLAYING);
    }
}

void GstWrapper::Pause() const {
    gst_element_set_state(_gstVars.pipeline, GST_STATE_PAUSED);
}

void GstWrapper::ClearPipeline() {
    g_print("Stopping receiver pipeline.\n");
    if (_gstVars.protocol == VxSdk::VxStreamProtocol::kMjpegPull) {
        g_source_remove(_gstVars.busWatchId);
        g_main_loop_unref(_gstVars.loop);
    }

    StopLocalRecord();
    gst_element_set_state(_gstVars.pipeline, GST_STATE_NULL);
    gst_object_unref(_gstVars.pipeline);
    _gstVars.isPipelineActive = false;
    _gstVars.lastTimestamp = NULL;
}

void GstWrapper::SetRtspTransport(MediaController::IStream::RTSPNetworkTransport transport)
{
    _gstVars.transport = transport;
}

MediaController::IStream::RTSPNetworkTransport  GstWrapper::GetRtspTransport()
{
    return _gstVars.transport;
}

void GstWrapper::SetControlUri(string uri)
{
    _gstVars.uriControl = uri;
}
