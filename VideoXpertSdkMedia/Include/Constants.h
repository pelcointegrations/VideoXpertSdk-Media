#ifndef Constants_h__
#define Constants_h__

namespace MediaController {
    namespace Constants {
        // Numerics
        static const unsigned long long kMsInNs = 1000000;
        static const unsigned long long kNtpToEpochDiffSec = 2208988800;
        static const unsigned int kClockRate = 90000;
        static const unsigned int kIntraFrameDiv = 111;
        static const unsigned short kPayloadType = 96;
        static const int kSnapshotTimeoutMs = 5 * 1000;
        static const int kSnapshotSleepTimeMs = 100;
        static const int kMjpegShutdownSleepTimeMs = 1000;
        static const unsigned short kDateMaxSize = 32;
        static const unsigned short kMillisecondsInt = 1000;
        static const float kMillisecondsFloat = 1000.0;

        // Header names
        static const char* kHeaderFrames = "Frames";
        static const char* kHeaderRateControl = "Rate-Control";
        static const char* kHeaderResourceTimestamp = "X-Resource-Timestamp";

        // Rtsp
        static const char* kWhitespace = " ";
        static const char* kForwardSlash = "/";
        static const char* kIntraPrefix = "intra";
        static const char* kRateControlValue = "no";
        static const char* kFramesAllValue = "all";

        // Mjpeg
        static const char* kHttpHeaders = "http-headers";
        static const char* kResponseHeaders = "response-headers";

        // GStreamer
        static const char* kSrc = "src";
        static const char* kSink = "sink";
        static const char* kRetries = "retries";
        static const char* kKeepAlive = "keep-alive";
        static const char* kLocation = "location";
        static const char* kHttpLogLevel = "http-log-level";
        static const char* kSslStrict = "ssl-strict";
        static const char* kCookies = "cookies";

        // GStreamer elements
        static const char* kHttpSrc = "souphttpsrc";
        static const char* kJpegDec = "jpegdec";
        static const char* kJpegEnc = "jpegenc";
        static const char* kVideoConvert = "videoconvert";
        static const char* kAspectRatioCrop = "aspectratiocrop";
        static const char* kTextOverlay = "textoverlay";
        static const char* kUriDecodeBin = "uridecodebin";
        static const char* kDecodeBin = "decodebin";
        static const char* kTee = "tee";
        static const char* kQueue = "queue";
        static const char* kPlaybin = "playbin";
        static const char* kX264Enc = "x264enc";
        static const char* kMp4Mux = "mp4mux";
        static const char* kFilesink = "filesink";
        static const char* kRtspSrc = "rtspsrc";
        static const char* kVideoSink = "autovideosink";
        static const char* kH264Depay = "rtph264depay";
        static const char* kH264Parse = "h264parse";
        static const char* kMkvMux = "matroskamux";

    }
}
#endif // Constants_h__
