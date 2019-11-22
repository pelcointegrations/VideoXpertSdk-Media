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
    
        GstElement *pipeline;
        GstElement *src;
        GstElement *rtspSrc;
        GstElement *videoDepay;
        GstElement *videoDec;
        GstElement *textOverlay;
        GstElement *tee;
        GstElement *queueView;
        GstElement *videoConvert;
        GstElement* aspectRatioCrop;
        GstElement *videoSink;
        GstElement* actualVideoSink;
        GstElement *audioDepay;
        GstElement *audioDec;
        GstElement *audioSink;
        GstElement *queueRecord;
        GstElement *x264enc;
        GstElement *mkvMux;
        GstElement *fileSink;
        GstElement *rtpBinManager;
        GstCaps *caps;
        GstPadLinkReturn linkReturn;
        GstPad *srcPad;
        GstPad *sinkPad;
        GstPad *teePad;

        GstElement *queueSnapShot;
        GstElement *encSnapShot;
        GstElement *fileSinkSnapShot;
        GstPad *teePadSnapShot;

        bool includeDateTimeInOverlay;
        std::string stringToOverlay;
        int overlayPositionH;
        int overlayPositionV;
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
        uint32_t endTime;

        /// <summary>
        /// Store the custom data from caller and send back on event callback.
        /// </summary>
        void* eventData;

        /// <summary>
        /// Store the preferred RSTP transport (for RTSP only).
        /// </summary>
        IStream::RTSPNetworkTransport transport;

        /// <summary>
        /// The current aspect ratio of the video stream.
        /// </summary>
        Controller::AspectRatios aspectRatio;

        /// <summary>
        /// Whether or not the rendered video should stretch to fit its display window.
        /// </summary>
        bool stretchToFit;

        bool isPipelineActive;
        bool isMjpeg;
        bool isRecording;
        bool storeVideoFast;
        std::string rtpCaps;
        std::string cookie;
        std::string hostIp;
        std::string multicastAddress;
        std::string location;
        std::string recordingFilePath;
        std::string uriControl;
        gint rtpPort;
        gint rtcpPort;
        gint rtcpSinkPort;
        guintptr windowHandle;
        guint busWatchId;
        GMainLoop *loop;
        VxSdk::VxStreamProtocol::Value protocol;
        guint timerId;
    };
}
#endif // GstVars_h__
