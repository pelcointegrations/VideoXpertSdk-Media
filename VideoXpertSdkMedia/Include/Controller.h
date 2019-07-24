#ifndef Controller_h__
#define Controller_h__

#include "IController.h"

namespace MediaController {
    class StreamState;
    struct MediaRequest;
    class StreamBase;

    /// <summary>
    /// Implements the IController interface methods.
    /// </summary>
    class Controller : public IController {
    public:

        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="request">The requested media.</param>
        Controller(MediaRequest& request);

        /// <summary>
        /// Virtual destructor.
        /// </summary>
        virtual ~Controller();
        void SetWindow(void* handle) override;
        bool GoToLive() override;
        bool Play(float speed, unsigned int unixTime, RTSPNetworkTransport transport) override;
        void PlayStream(float speed, unsigned int unixTime, RTSPNetworkTransport transport) override;
        void Pause() override;
        void Stop() override;
        bool StartLocalRecording(char* filePath, char* fileName) override;
        void StopLocalRecording() override;
        bool SnapShot(char* filePath, char* fileName) override;
        void NewRequest(MediaRequest& request) override;
        void AddObserver(TimestampEventCallback observer) override;
        void AddStreamObserver(StreamEventCallback observer) override;
        void AddPelcoDataObserver(PelcoDataEventCallback observer) override;
        void RemoveObserver(TimestampEventCallback observer) override;
        void RemoveStreamObserver(StreamEventCallback observer) override;
        void RemovePelcoDataObserver(PelcoDataEventCallback observer) override;
        void ClearObservers() override;
        void ClearStreamObservers() override;
        Mode GetMode() override;
        bool IsPipelineActive() override;
        void AddEventData(void* customData) override;
        bool AddVideoOverlayData(std::string overlayData, VideoOverlayDataPosition position, bool inlcudeDateTime) override;

        /// <summary>
        /// The current video stream instance.
        /// </summary>
        StreamBase* stream;

        /// <summary>
        /// The current audio stream instance.
        /// </summary>
        StreamBase* audioStream;

    private:
        static void CallSetupStream(StreamBase* stream, float speed, unsigned int unixTime, RTSPNetworkTransport transport, bool* result);
        static void CallPlayStream(StreamBase* stream, float speed, unsigned int unixTime, RTSPNetworkTransport transport);

        // Keep the transport layer around
        RTSPNetworkTransport _transport;
    };
}
#endif // Controller_h__
