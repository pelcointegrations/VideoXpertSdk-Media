#ifndef PelcoDataEvent_h__
#define PelcoDataEvent_h__

namespace MediaController {

    /// <summary>
    /// Represents an event that has been sent from a stream.
    /// </summary>
    struct PelcoDataEvent {

        /// <summary>
        /// Values that represent event types sent from a stream.
        /// </summary>
        enum Type {
            /// <summary>An error or unknown value was returned.</summary>
            kUnknown,
            /// <summary>The connection to the stream was lost.</summary>
            kConnectionLost
        };

        /// <summary>
        /// The event type.
        /// </summary>
        Type eventType;

        unsigned char pelcoData[500];
    };
}
#endif // PelcoDataEvent_h__