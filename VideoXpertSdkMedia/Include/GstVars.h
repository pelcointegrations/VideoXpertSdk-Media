#ifndef GstVars_h__
#define GstVars_h__

#include "Controller.h"
#include <gst/gstelement.h>

namespace MediaController {

    /// <summary>
    /// Variables required for GStreamer.
    /// </summary>
    struct GstVars {
    public:
        GstVars();
        ~GstVars();

        // GStreamer Pipeline Elements
        GstElement *pipeline;
        GstElement* audioPlaybin;
        GstElement* audioSource;
        GstElement* videoSource;
        GstElement* videoDecoder;
        GstElement* videoParse;
        GstElement* videoTee;
        GstElement* videoQueue;
        GstElement* videoSink;

        // Dynamic Pipeline Elements
        GstElement* teeQueue;
        GstElement* encoder;
        GstElement* muxer;
        GstElement* fileSink;
        GstElement* rtpBinManager;
        GstPad* videoTeePad;

        /// <summary>
        /// Indicates whether the date and time should be included in the text overlay.
        /// </summary>
        bool includeDateTimeInOverlay;

        /// <summary>
        /// The text to display in the overlay.
        /// </summary>
        std::string stringToOverlay;

        /// <summary>
        /// The horizontal alignment of the overlay text
        /// </summary>
        int overlayPositionH;

        /// <summary>
        /// The vertical alignment of the overlay text
        /// </summary>
        int overlayPositionV;

        /// <summary>
        /// The alignment of overlay text lines relative to each other.
        /// </summary>
        int overlayLineAlignment;

        /// <summary>
        /// The list of StreamEvent observers.
        /// </summary>
        std::vector<StreamEventCallback> streamEventObserverList;

        /// <summary>
        /// The list of timestamp event observers.
        /// </summary>
        std::vector<TimestampEventCallback> observerList;

        /// <summary>
        /// The list of PelcoData event observers.
        /// </summary>
        std::vector<PelcoDataEventCallback> pelcoDataObserverList;

        /// <summary>
        /// The current timestamp of the stream.
        /// </summary>
        unsigned long currentTimestamp;

        /// <summary>
        /// The last timestamp received from the stream.
        /// </summary>
        uint32_t  lastTimestamp;

        /// <summary>
        /// The latest timestamp received from an RTCP packet.
        /// </summary>
        uint64_t  rtcpTimestamp;

        /// <summary>
        /// The current <see cref="Controller::Mode"/> of the stream.
        /// </summary>
        Controller::Mode mode;

        /// <summary>
        /// The current speed of the stream.
        /// </summary>
        float speed;

        /// <summary>
        /// The current seek time of the stream.
        /// </summary>
        uint32_t seekTime;

        /// <summary>
        /// The end time to use when using a playback time range.
        /// </summary>
        uint32_t endTime;

        /// <summary>
        /// Store the custom data from caller and send back on event callback.
        /// </summary>
        void* eventData;

        /// <summary>
        /// The current aspect ratio of the video stream.
        /// </summary>
        Controller::AspectRatios aspectRatio;

        /// <summary>
        /// Indicates whether or not the rendered video should stretch to fit its display window.
        /// </summary>
        bool stretchToFit;

        /// <summary>
        /// Indicates whether or not the current stream is MJPEG.
        /// </summary>
        bool isMjpeg;

        /// <summary>
        /// Indicates whether or not the current stream is being recorder to a local file.
        /// </summary>
        bool isRecording;

        /// <summary>
        /// Indicates whether or not a stream is being stored as a local file.
        /// </summary>
        bool isStoringVideo;

        /// <summary>
        /// Indicates whether or not the current stream is paused.
        /// </summary>
        bool isPaused;

        /// <summary>
        /// The cookie data to use for MJPEG stream authentication.
        /// </summary>
        std::string cookie;

        /// <summary>
        /// The handle of the window that will be used to display video.
        /// </summary>
        guintptr windowHandle;

        /// <summary>
        /// The event source ID returned from the bus.
        /// </summary>
        guint busWatchId;

        /// <summary>
        /// The GStreamer event loop.
        /// </summary>
        GMainLoop *loop;

        /// <summary>
        /// The timer ID used to monitor for connection loss.
        /// </summary>
        guint timerId;

        /// <summary>
        /// The transport protocol.
        /// </summary>
        IStream::RTSPNetworkTransport transport;
    };
}
#endif // GstVars_h__
