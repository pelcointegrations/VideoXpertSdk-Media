#ifndef TimestampEvent_h__
#define TimestampEvent_h__

namespace MediaController {

    /// <summary>
    /// Represents an event that has been sent from a stream.
    /// </summary>
    struct TimestampEvent {

        /// <summary>
        /// The absolute time of the current stream in unix timestamp format.
        ///    This will contain seconds
        /// </summary>
        unsigned int unixTime;

        /// <summary>
        /// The absolute time of the current stream in unix timestamp format.
        ///    This will contain the microseconds
        ///  unixTime and unixTimeMicroseconds can be used together to gain better resolution
        /// </summary>
        unsigned int unixTimeMicroSeconds;

        /// <summary>
        /// The event data assigned by caller during event subscription.
        /// </summary>
        void* eventData;
    };
}
#endif // TimestampEvent_h__
