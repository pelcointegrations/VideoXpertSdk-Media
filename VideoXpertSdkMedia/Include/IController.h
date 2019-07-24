#ifndef IController_h_
#define IController_h_

#include "IStream.h"
#include "TimestampEvent.h"
#include "StreamEvent.h"
#include "PelcoDataEvent.h"
#include <string>

namespace MediaController {

    /// <summary>
    /// The timestamp event callback function pointer.
    /// </summary>
    /// <param>A <see cref="TimestampEvent"/>.</param>
    typedef void(*TimestampEventCallback)(TimestampEvent*);

    /// <summary>
    /// The stream event callback function pointer.
    /// </summary>
    /// <param>A <see cref="StreamEvent"/>.</param>
    typedef void(*StreamEventCallback)(StreamEvent*);

    /// <summary>
    /// The Pelco Data event callback function pointer.
    /// </summary>
    /// <param>A <see cref="PelcoDataEvent"/>.</param>
    typedef void(*PelcoDataEventCallback)(PelcoDataEvent*);

    /// <summary>
    /// Defines the controller interface.
    /// </summary>
    class IController : public IStream {
    public:
        /// <summary>
        /// Virtual destructor.
        /// </summary>
        virtual ~IController() {}

        /// <summary>
        /// Set the display window using the given window handle.
        /// </summary>
        /// <param name="handle">The window handle of the display.</param>
        virtual void SetWindow(void* handle) = 0;

        /// <summary>
        /// Add a new subscriber to timestamp events.
        /// </summary>
        /// <param name="observer">The <see cref="TimestampEventCallback"/> event handler.</param>
        virtual void AddObserver(TimestampEventCallback observer) = 0;

        /// <summary>
        /// Add a new subscriber to stream events.
        /// </summary>
        /// <param name="observer">The <see cref="StreamEventCallback"/> event handler.</param>
        virtual void AddStreamObserver(StreamEventCallback observer) = 0;

        /// <summary>
        /// Add a new subscriber to Pelco Data events.
        /// </summary>
        /// <param name="observer">The <see cref="StreamEventCallback"/> event handler.</param>
        virtual void AddPelcoDataObserver(PelcoDataEventCallback observer) = 0;

        /// <summary>
        /// Add event data to be send back during timestamp events.
        /// </summary>
        /// <param name="customData">Custom data pointer.</param>
        virtual void AddEventData(void* customData) = 0;

        /// <summary>
        /// Remove an existing timestamp event subscriber.
        /// </summary>
        /// <param name="observer">The <see cref="TimestampEventCallback"/> event handler.</param>
        virtual void RemoveObserver(TimestampEventCallback observer) = 0;

        /// <summary>
        /// Remove an existing stream event subscriber.
        /// </summary>
        /// <param name="observer">The <see cref="StreamEventCallback"/> event handler.</param>
        virtual void RemoveStreamObserver(StreamEventCallback observer) = 0;

        /// <summary>
        /// Remove an existing stream event subscriber.
        /// </summary>
        /// <param name="observer">The <see cref="StreamEventCallback"/> event handler.</param>
        virtual void RemovePelcoDataObserver(PelcoDataEventCallback observer) = 0;

        /// <summary>
        /// Remove all existing timestamp event subscribers.
        /// </summary>
        virtual void ClearObservers() = 0;

        /// <summary>
        /// Remove all existing stream event subscribers.
        /// </summary>
        virtual void ClearStreamObservers() = 0;

        /// <summary>
        /// Get the status of the pipeline.
        /// </summary>
        /// <returns>True if pipeline is active, otherwise false.</returns>
        virtual bool IsPipelineActive() = 0;

        /// <summary>
        /// Allows you to place data on the video being played
        ///  If 'includeDateTime' is true, then you can use a format string as in
        ///  the "put_time" funciton in the c++ standard library.  
        ///    e.g.  %Y-%m-%dT%H:%M:%S in a string will print
        ///    2011-07-11T12:15:33
        /// </summary>
        /// <returns>True if data can be displayed (there is a limit to string size)</returns>
        enum VideoOverlayDataPosition {
            kTopLeft,
            kTopCenter,
            kTopRight,
            kMiddleLeft,
            kMiddleCenter,
            kMiddleRight,
            kBottomLeft,
            kBottomCenter,
            kBottomRight
        };
        virtual bool AddVideoOverlayData(std::string overlayData, VideoOverlayDataPosition position, bool includeDateTime) = 0;
    };
}
#endif // IController_h_
