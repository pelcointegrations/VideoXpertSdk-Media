#ifndef RtspStream_h__
#define RtspStream_h__

#include "StreamBase.h"
#include "RtspCommands.h"

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
            /// <param name="isVideo">Specifies wheather this Stream handles audio or video.</param>
            Stream(MediaRequest& request, bool isVideo);

            /// <summary>
            /// Virtual destructor.
            /// </summary>
            virtual ~Stream();
            bool Play(float speed = 0, unsigned int unixTime = 0, RTSPNetworkTransport transport = kUDP) override;
            void PlayStream(float speed, unsigned int unixTime, RTSPNetworkTransport transport = kUDP) override;
            void Pause() override;
            void Stop() override;
            bool GoToLive() override;
            void NewRequest(MediaRequest& request) override;
            bool Resume(float speed = 0, unsigned int unixTime = 0, RTSPNetworkTransport transport = kUDP) override;
            bool StartLocalRecording(char* filePath, char* fileName) override;
            void StopLocalRecording() override;
            bool SnapShot(char* filePath, char* fileName) override;

        private:
            Commands* _rtspCommands;
        };
    }
}
#endif // RtspStream_h__
