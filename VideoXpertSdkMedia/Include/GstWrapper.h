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
        /// Set the cookie for MJPEG streams.
        /// </summary>
        /// <param name="cookie">The cookie value.</param>
        void SetCookie(std::string cookie);

        /// <summary>
        /// Set the internal timestamp variable.
        /// </summary>
        /// <param name="seekTime">A unix timestamp.</param>
        void SetTimestamp(unsigned int seekTime);

        /// <summary>
        /// Get the last timestamp received from the stream.
        /// </summary>
        /// <returns>The unix timestamp.</returns>
        unsigned int GetLastTimestamp() const { return _gstVars.currentTimestamp; }

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
        bool IsPipelineActive() const { return _gstVars.pipeline != nullptr; }

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
        /// Records a stream directly to a local file.
        /// </summary>
        /// <param name="filePath">The directory to store the generated video file.</param>
        /// <param name="fileName">The name to use for the generated video file.</param>
        /// <param name="startTime">The start time of the stream to record.</param>
        /// <param name="endTime">The end time of the stream to record.</param>
        /// <param name="request">The streaming media to request.</param>
        /// <returns>True if the recording started successfully, otherwise false.</returns>
        bool StoreVideo(char* filePath, char* fileName, unsigned int startTime, unsigned int endTime, MediaRequest request);

        /// <summary>
        /// Create the pipeline for an RTSP video stream.
        /// </summary>
        /// <param name="speed">The playback speed.</param>
        /// <param name="seekTime">The start time for playback.</param>
        /// <param name="request">The streaming media to request.</param>
        void CreateRtspPipeline(float speed, unsigned int seekTime, MediaRequest request, IStream::RTSPNetworkTransport transport);

        /// <summary>
        /// Create the pipeline for an MJPEG stream.
        /// </summary>
        /// <param name="speed">The playback speed.</param>
        /// <param name="jpegUri">The URI of the MJPEG stream.</param>
        void CreateMjpegPipeline(float speed, char* jpegUri);

        /// <summary>
        /// Set the pipeline state to playing and update the speed value for determining the framerate.
        /// </summary>
        void Play();

        /// <summary>
        /// Set the pipeline state to paused.
        /// </summary>
        void Pause();

        /// <summary>
        /// Change the RTSP URL for the currently playing stream.
        /// </summary>
        void ChangeRtspLocation(MediaRequest request);

        /// <summary>
        /// Starts recording the current video stream to a local file.
        /// </summary>
        /// <param name="filePath">The directory to store the generated video file.</param>
        /// <param name="fileName">The name to use for the generated video file.</param>
        /// <returns>True if the recording started successfully, otherwise false.</returns>
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
        /// <returns>True if the snapshot was successful, otherwise false.</returns>
        bool SnapShot(char* filePath, char* fileName);

        /// <summary>
        /// Clear the pipeline and display window.
        /// </summary>
        void ClearPipeline();

        /// <summary>
        /// Set the Overlay String.  You may include date/time to the overlay
        /// </summary>
        /// <param name="stringToOverlay">The overlay text.</param>
        /// <param name="position">The position of the overlay.</param>
        /// <param name="includeDateTime">True to include the date and time in the overlay, otherwise false.</param>
        /// <returns>True if the overlay was successfully set, otherwise false.</returns>
        bool SetOverlayString(std::string stringToOverlay, IController::VideoOverlayDataPosition position, bool includeDateTime);

        /// <summary>
        /// Get the current aspect ratio.
        /// </summary>
        /// <returns>The current aspect ratio.</returns>
        Controller::AspectRatios GetAspectRatio() const { return _gstVars.aspectRatio; }

        /// <summary>
        /// Set the current aspect ratio.
        /// </summary>
        void SetAspectRatio(Controller::AspectRatios aspectRatio);

        /// <summary>
        /// Gets whether or not the rendered video should stretch to fit its display window.
        /// </summary>
        /// <returns>True if the rendered video should stretch to fit its display window, otherwise false.</returns>
        bool GetStretchToFit() const { return _gstVars.stretchToFit; }

        /// <summary>
        /// Sets whether or not the rendered video should stretch to fit its display window.
        /// </summary>
        /// <param name="stretchToFit">True to stretch the rendered video to fit its display window, otherwise false.</param>
        void SetStretchToFit(bool stretchToFit);

    private:
        GstVars _gstVars;
    };
}
#endif // GstWrapper_h__
