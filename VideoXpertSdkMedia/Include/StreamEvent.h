#ifndef StreamEvent_h__
#define StreamEvent_h__

namespace MediaController {

    /// <summary>
    /// Represents an event that has been sent from a stream.
    /// </summary>
    struct StreamEvent {

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
    };
}
#endif // StreamEvent_h__
