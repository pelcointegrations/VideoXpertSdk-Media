#ifndef IStream_h__
#define IStream_h__

#include "MediaRequest.h"

namespace MediaController {

    /// <summary>
    /// Defines the stream interface.
    /// </summary>
    class IStream {
    public:
        /// <summary>
        /// Values that represent the aspect ratio of the video stream.
        /// </summary>
        enum AspectRatios {
            /// <summary>16:9 aspect ratio (Default).</summary>
            k16x9,

            /// <summary>4:3 aspect ratio.</summary>
            k4x3,

            /// <summary>1:1 aspect ratio.</summary>
            k1x1,

            /// <summary>3:2 aspect ratio.</summary>
            k3x2,

            /// <summary>5:49 aspect ratio.</summary>
            k5x4
        };

        /// <summary>
        /// Values that represent the different playback modes.
        /// </summary>
        enum Mode {
            /// <summary>The stream is stopped.</summary>
            kStopped,

            /// <summary>The stream is playing live video.</summary>
            kLive,

            /// <summary>The stream is playing recorded video.</summary>
            kPlayback
        };

        /// <summary>
        ///  This value is used to choose the transport protocol for RTSP streams
        ///     It is ignored for JPEG pull or other protocols.
        /// </summary>
        enum RTSPNetworkTransport {
            /// <summary> use UDP </summary>
            kUDP,
            
            /// <summary> try to use TCP </summary>
            kRTPOverRTSP
        };

        /// <summary>
        /// Virtual destructor.
        /// </summary>
        virtual ~IStream() {}

        /// <summary>
        /// Call Play on the stream.
        /// </summary>
        /// <param name="speed">The playback speed.  Negative values can be used for reverse
        /// playback. A value of 0 will resume a paused stream.</param>
        /// <param name="unixTime">The start time for playback. A value of 0 will start a live stream.</param>
        virtual bool Play(float speed = 0, unsigned int unixTime = 0, RTSPNetworkTransport transport = kUDP) = 0;

        /// <summary>
        /// Records a stream directly to a local file.
        /// </summary>
        /// <param name="startTime">The start time of the stream to record.</param>
        /// <param name="stopTime">The end time of the stream to record.</param>
        /// <param name="filePath">The directory to store the generated video file.</param>
        /// <param name="fileName">The name to use for the generated video file.</param>
        /// <returns>True if the recording started successfully, otherwise false.</returns>
        virtual bool StoreStream(unsigned int startTime, unsigned int stopTime, char* filePath, char* fileName) = 0;

        /// <summary>
        /// Starts recording the current video stream to a local file.
        /// </summary>
        /// <param name="filePath">The directory to store the generated video file.</param>
        /// <param name="fileName">The name to use for the generated video file.</param>
        virtual bool StartLocalRecording(char* filePath, char* fileName) = 0;

        /// <summary>
        /// Stops the current local recording in progress, if any.
        /// </summary>
        virtual void StopLocalRecording() = 0;

        /// <summary>
        /// Starts recording the current video stream to a local file.
        /// </summary>
        /// <param name="filePath">The directory to store the generated video file.</param>
        /// <param name="fileName">The name to use for the generated video file.</param>
        virtual bool SnapShot(char* filePath, char* fileName) = 0;

        /// <summary>
        /// Call Pause on the stream.
        /// </summary>
        virtual void Pause() = 0;

        /// <summary>
        /// Call TearDown on the stream.
        /// </summary>
        virtual void Stop() = 0;

        /// <summary>
        /// Set the stream to Live and call Play.
        /// </summary>
        virtual bool GoToLive() = 0;

        /// <summary>
        /// Set the stream to a new source.
        /// </summary>
        /// <param name="request">The new <see cref="MediaRequest"/> to reset the stream to.</param>
        virtual void NewRequest(MediaRequest& request) = 0;

        /// <summary>
        /// Get the current playback mode.
        /// </summary>
        /// <returns>The current stream <see cref="Mode"/>.</returns>
        virtual Mode GetMode() = 0;
    };
}
#endif // IStream_h__
