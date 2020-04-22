#include "stdafx.h"
#include "StreamFactory.h"

#include "StreamBase.h"
#include "MediaRequest.h"
#include "RtspStream.h"
#include "MjpegPullStream.h"
#include <StreamState.h>

using namespace MediaController;
using namespace VxSdk;

StreamBase* StreamFactory::CreateStream(MediaRequest& request) {
    StreamBase* stream = nullptr;
    if (!request.dataSource) return stream;

    if (request.dataInterface.protocol == VxStreamProtocol::kRtspRtp) {
        stream = new Rtsp::Stream(request);
        stream->protocol = request.dataInterface.protocol;
        stream->state = new StoppedState();
    }
    else if (request.dataInterface.protocol == VxStreamProtocol::kMjpegPull) {
        stream = new MjpegPull::Stream(request);
        stream->protocol = request.dataInterface.protocol;
        stream->state = new StoppedState();
    }
    return stream;
}