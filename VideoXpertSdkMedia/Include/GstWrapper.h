#ifndef GstWrapper_h__
#define GstWrapper_h__

#include "Controller.h"
#include "GstVars.h"
#include <gst/gstelement.h>

namespace MediaController {

    /// <summary>
    /// Manages the GStreamer instance.
    /// </summary>
    class GstWrapper {
    public:
        /// <summary>
        /// Constructor.
        /// </summary>
        GstWrapper();

        /// <summary>
        /// Destructor.
        /// </summary>
        ~GstWrapper();

        /// <summary>
        /// Set the display window using the given window handle.
        /// </summary>
        /// <param name="winhandle">The window handle of the display.</param>
        void SetWindowHandle(guintptr winhandle);

        /// <summary>
        /// Set the stream location.
        /// </summary>
        /// <param name="location">The URI of the new stream location.</param>
        void SetLocation(std::string location);

        /// <summary>
        /// Set the receiver ports.
        /// </summary>
        /// <param name="rtpPort">The port to receive RTP data.</param>
        /// <param name="rtcpPort">The port to receive RTCP data.</param>
        void SetPorts(int rtpPort, int rtcpPort);

        /// <summary>
        /// Set the stream capabilities.
        /// </summary>
        /// <param name="caps">The stream capabilities.</param>
        void SetCaps(std::string caps, bool isMjpeg = false);

        /// <summary>
        /// Set the cookie for MJPEG streams.
        /// </summary>
        /// <param name="cookie">The cookie value.</param>
        void SetCookie(std::string cookie);

        /// <summary>
        /// Set the RTCP host IP.
        /// </summary>
        /// <param name="hostIp">The IP of the RTCP host.</param>
        void SetRtcpHostIP(std::string hostIp);

        /// <summary>
        /// Set the multicast group address.
        /// </summary>
        /// <param name="multicastAddress">The multicast group address.</param>
        void SetMulticastAddress(std::string multicastAddress);

        /// <summary>
        /// Set the internal timestamp variable.
        /// </summary>
        /// <param name="seekTime">A unix timestamp.</param>
        void SetTimestamp(unsigned int seekTime);

        /// <summary>
        /// Get the last timestamp received from the stream.
        /// </summary>
        /// <returns>The unix timestamp.</returns>
        unsigned int GetLastTimestamp() const;

        /// <summary>
        /// Set the playback mode.
        /// </summary>
        /// <param name="mode">The stream <see cref="Controller::Mode"/> to set.</param>
        void SetMode(Controller::Mode mode);

        /// <summary>
        /// Get the current playback mode.
        /// </summary>
        /// <returns>The current stream <see cref="Controller::Mode"/>.</returns>
        Controller::Mode GetMode() const { return _gstVars.mode; }

        /// <summary>
        /// Get the current playback speed.
        /// </summary>
        /// <returns>The current stream speed.</returns>
        float GetSpeed() const { return _gstVars.speed; }

        /// <summary>
        /// Set the current playback speed.
        /// </summary>
        void SetSpeed(float speed);

        /// <summary>
        /// Get the status of the pipeline.
        /// </summary>
        /// <returns>True if pipeline is active, otherwise false.</returns>
        bool IsPipelineActive() const;

        /// <summary>
        /// Add a new subscriber to timestamp events.
        /// </summary>
        /// <param name="observer">The <see cref="TimestampEventCallback"/> event handler.</param>
        void AddObserver(TimestampEventCallback observer);

        /// <summary>
        /// Remove an existing timestamp event subscriber.
        /// </summary>
        /// <param name="observer">The <see cref="TimestampEventCallback"/> event handler.</param>
        void RemoveObserver(TimestampEventCallback observer);

        /// <summary>
        /// Add a new subscriber to Pelco Data events.
        /// </summary>
        /// <param name="observer">The <see cref="PelcoDataEventCallback"/> event handler.</param>
        void AddPelcoDataObserver(PelcoDataEventCallback observer);

        /// <summary>
        /// Remove an existing Pelco Data event subscriber.
        /// </summary>
        /// <param name="observer">The <see cref="PelcoDataEventCallback"/> event handler.</param>
        void RemovePelcoDataObserver(PelcoDataEventCallback observer);

        /// <summary>
        /// Add a new subscriber to Stream events.
        /// </summary>
        /// <param name="observer">The <see cref="PelcoDataEventCallback"/> event handler.</param>
        void AddStreamObserver(StreamEventCallback observer);

        /// <summary>
        /// Remove an existing stream event subscriber.
        /// </summary>
        /// <param name="observer">The <see cref="PelcoDataEventCallback"/> event handler.</param>
        void RemoveStreamObserver(StreamEventCallback observer);

        /// <summary>
        /// Remove all existing timestamp event subscribers.
        /// </summary>
        void ClearObservers();

        /// <summary>
        /// Add custom data to be stored in here, which will be send back to caller inside <see cref="TimestampEvent"/> on <see cref="TimestampEventCallback"/>.
        /// </summary>
        /// <param name="customData">Custom data pointer.</param>
        void AddEventData(void* customData);

        /// <summary>
        /// Create the pipeline for an RTSP video stream.
        /// </summary>
        /// <param name="encoding">The video encoding type.</param>
        void CreateVideoRtspPipeline(std::string encoding, float speed, unsigned int seekTime);

        /// <summary>
        /// Create the pipeline for an RTSP audio stream.
        /// </summary>
        void CreateAudioRtspPipeline(float speed, unsigned int unixTime);

        /// <summary>
        /// Create the pipeline for an MJPEG stream.
        /// </summary>
        void CreateMjpegPipeline(float speed);

        /// <summary>
        /// Set the pipeline state to playing and update the speed value for determining the framerate.
        /// </summary>
        void Play();

        /// <summary>
        /// Set the pipeline state to paused.
        /// </summary>
        void Pause() const;

        /// <summary>
        /// Starts recording the current video stream to a local file.
        /// </summary>
        /// <param name="filePath">The directory to store the generated video file.</param>
        /// <param name="fileName">The name to use for the generated video file.</param>
        bool StartLocalRecord(char* filePath, char* fileName);

        /// <summary>
        /// Stops the current local recording in progress, if any.
        /// </summary>
        void StopLocalRecord();

        /// <summary>
        /// Captures a snapshot from the current video stream to a local file.
        /// </summary>
        /// <param name="filePath">The directory to store the generated snapshot file.</param>
        /// <param name="fileName">The name to use for the generated snapshot file.</param>
        bool SnapShot(char* filePath, char* fileName);

        /// <summary>
        /// Clear the pipeline and display window.
        /// </summary>
        void ClearPipeline();

        /// <summary>
        /// Set the RTSP Transport
        /// </summary>
        void SetRtspTransport(IStream::RTSPNetworkTransport transport);
        IStream::RTSPNetworkTransport  GetRtspTransport();

        /// <summary>
        /// Set the control URI
        /// </summary>
        void SetControlUri(std::string uri);
       
        /// <summary>
        /// Set the Overlay String.  You may include date/time to the overlay
        /// </summary>
        bool SetOverlayString(std::string stringToOverlay, MediaController::IController::VideoOverlayDataPosition position, bool inludeDateTime);

    private:
        static void Init();
        void CreatePipeline();
        GstVars _gstVars;
    };
}
#endif // GstWrapper_h__
