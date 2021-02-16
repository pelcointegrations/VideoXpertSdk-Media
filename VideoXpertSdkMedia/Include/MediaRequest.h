#ifndef MediaRequest_h__
#define MediaRequest_h__

#include "VxSdk.h"

namespace MediaController {

    /// <summary>
    /// Contains the information needed to start a new media stream.
    /// </summary>
    struct MediaRequest {

        MediaRequest() {
            dataSource = nullptr;
            audioDataSource = nullptr;
            rtspVideoEndpoint = nullptr;
            rtspAudioEndpoint = nullptr;
            username = nullptr;
            password = nullptr;
        }

        /// <summary>
        /// The data source to use to create the new video stream.
        /// </summary>
        VxSdk::IVxDataSource* dataSource;

        /// <summary>
        /// The protocol to use for the new video stream.
        /// </summary>
        VxSdk::IVxDataInterface dataInterface;

        /// <summary>
        /// The data source to use to create the new audio stream.
        /// </summary>
        VxSdk::IVxDataSource* audioDataSource;

        /// <summary>
        /// The protocol to use for the new audio stream.
        /// </summary>
        VxSdk::IVxDataInterface audioDataInterface;

        /// <summary>
        /// The RTSP video endpoint URI.  If set, this will be used instead of the dataSource/dataInterface.
        /// </summary>
        char* rtspVideoEndpoint;

        /// <summary>
        /// The RTSP audio endpoint URI. If set, this will be used instead of the audioDataSource/audioDataInterface.
        /// </summary>
        char* rtspAudioEndpoint;

        /// <summary>
        /// The username for RTSP authentication.
        /// </summary>
        char* username;

        /// <summary>
        /// The password for RTSP authentication.
        /// </summary>
        char* password;
    };
}
#endif // MediaRequest_h__
