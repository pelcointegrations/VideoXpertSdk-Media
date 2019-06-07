#ifndef StreamBase_h__
#define StreamBase_h__

#include "IStream.h"
#include "MediaRequest.h"
#include "StreamState.h"

namespace MediaController {
    class GstWrapper;
    class StreamState;

    /// <summary>
    /// The base class for the different stream types.
    /// </summary>
    class StreamBase : public IStream {
    public:

        /// <summary>
        /// Virtual destructor.
        /// </summary>
        virtual ~StreamBase();
        bool Play(float speed = 0, unsigned int unixTime = 0) override;
        void Pause() override;
        void Stop() override;
        void NewRequest(MediaRequest& request) override;
        bool StartLocalRecording(char* filePath, char* fileName);
        void StopLocalRecording();
        bool SnapShot(char* filePath, char* fileName);

        /// <summary>
        /// Send PLAY on an existing stream.
        /// </summary>
        /// <param name="speed">The playback speed.  Negative values can be used for reverse
        /// playback. A value of 0 will resume a paused stream.</param>
        /// <param name="unixTime">The start time for playback. A value of 0 will start a live stream.</param>
        virtual bool Resume(float speed = 0, unsigned int unixTime = 0);

        /// <summary>
        /// Get the current GStreamer wrapper instance.
        /// </summary>
        /// <returns>The current <see cref="GstWrapper"/> instance.</returns>
        GstWrapper* GetGstreamer() const;

        /// <summary>
        /// Gets the current time of the stream.
        /// </summary>
        /// <returns>The last timestamp received.</returns>
        unsigned int GetLastTimestamp() const;

        /// <summary>
        /// Add a new subscriber to stream events.
        /// </summary>
        /// <param name="observer">The <see cref="StreamEventCallback"/> event handler.</param>
        void AddObserver(StreamEventCallback observer);

        /// <summary>
        /// Remove an existing stream event subscriber.
        /// </summary>
        /// <param name="observer">The <see cref="StreamEventCallback"/> event handler.</param>
        void RemoveObserver(StreamEventCallback observer);

        /// <summary>
        /// Remove all existing stream event subscribers.
        /// </summary>
        void ClearObservers();

        /// <summary>
        /// The current state of the stream.
        /// </summary>
        StreamState* state;

        Mode GetMode() override;

        /// <summary>
        /// The protocol of the stream.
        /// </summary>
        VxSdk::VxStreamProtocol::Value protocol;

        /// <summary>
        /// The list of stream event observers.
        /// </summary>
        std::vector<StreamEventCallback> observerList;

    protected:
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="request">The requested media.</param>
        StreamBase(MediaRequest& request);

    private:
        /// <summary>
        /// Default constructor.
        /// </summary>
        StreamBase();

    protected:
        MediaRequest _mediaRequest;
        GstWrapper* _gst;
    };
}
#endif // StreamBase_h__
