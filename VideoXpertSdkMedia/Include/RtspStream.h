#ifndef RtspStream_h__
#define RtspStream_h__

#include "StreamBase.h"

namespace MediaController {

    /// <summary>
    /// Contains classes that manage RTSP streams.
    /// </summary>
    namespace Rtsp {

        /// <summary>
        /// Represents an RTSP stream.
        /// </summary>
        class Stream : public StreamBase {
        public:

            /// <summary>
            /// Constructor.
            /// </summary>
            /// <param name="request">The requested media.</param>
            Stream(MediaRequest& request);

            /// <summary>
            /// Virtual destructor.
            /// </summary>
            virtual ~Stream();
            bool Play(float speed = 0, unsigned int unixTime = 0, RTSPNetworkTransport transport = kUDP) override;
            bool StoreStream(unsigned int startTime, unsigned int stopTime, char* filePath, char* fileName) override;
            void Pause() override;
            void Stop() override;
            bool GoToLive() override;
            void NewRequest(MediaRequest& request) override;
            bool Resume(float speed = 0, unsigned int unixTime = 0, RTSPNetworkTransport transport = kUDP) override;
            bool StartLocalRecording(char* filePath, char* fileName, bool includeOverlays = true) override;
            void StopLocalRecording() override;
            bool SnapShot(char* filePath, char* fileName) override;
        };
    }
}
#endif // RtspStream_h__
