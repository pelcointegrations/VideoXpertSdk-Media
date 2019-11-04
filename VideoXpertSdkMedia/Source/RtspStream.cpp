#include "stdafx.h"
#include "RtspStream.h"

#include "Controller.h"
#include "RtspCommands.h"
#include "GstWrapper.h"
#include <StreamState.h>

using namespace std;
using namespace MediaController;
using namespace Constants;
using namespace VxSdk;

Rtsp::Stream::Stream(MediaRequest& request, bool isVideo) :
    StreamBase(request),
    _rtspCommands(new Commands(request.dataInterface.dataEndpoint, isVideo)),
    _startUrl(request.dataInterface.dataEndpoint)
{
}

Rtsp::Stream::~Stream() {
    delete _rtspCommands;
    _rtspCommands = nullptr;
}

bool Rtsp::Stream::Play(float speed, unsigned int unixTime, RTSPNetworkTransport transport) {
    if ((this->_mediaRequest.dataInterface.supportsMulticast == true) && (transport != RTSPNetworkTransport::kUDP)) {
        return false;
    }
    _gst->SetRtspTransport(transport);
    _gst->SetControlUri(this->_rtspCommands->GetControlUri());
    if (this->_gst->GetMode() != IController::kStopped)
        this->Stop();

    if (speed == 0 && unixTime == 0)
        this->_gst->SetMode(IController::kLive);
    else
        this->_gst->SetMode(IController::kPlayback);

    // Reset to the base URI 
    this->_rtspCommands->ResetPath(_startUrl);

    // Send the sequence of RTSP commands needed to start a new stream.
    if (!this->_rtspCommands->Options())
        return false;

    if (!this->_rtspCommands->Describe(true))
        return false;

    bool useTCP = (transport == RTSPNetworkTransport::kRTPOverRTSP) ? true : false;
    if (!this->_rtspCommands->Setup(useTCP, true))
        return false;

    if (this->_rtspCommands->SetupStream(this->_gst, speed, unixTime))
        return true;

    return false;
}

void Rtsp::Stream::PlayStream(float speed, unsigned int unixTime, RTSPNetworkTransport transport) {
    _gst->SetRtspTransport(transport);
    _gst->SetControlUri(this->_rtspCommands->GetControlUri());
    if (speed == 0) {
        speed = 1;
    }
    this->_rtspCommands->PlayStream(this->_gst, speed, unixTime);
    this->_gst->Play();

    this->state = new PlayingState();
}

void Rtsp::Stream::Pause() {
    // Pause the stream.
    this->_rtspCommands->Pause();
    this->_gst->Pause();
    this->state = new PausedState();
}

void Rtsp::Stream::Stop() {
    this->_gst->ClearPipeline();
    this->_gst->SetMode(IController::kStopped);
    this->state = new StoppedState();
}

bool Rtsp::Stream::GoToLive() { return true; }

bool Rtsp::Stream::Resume(float speed, unsigned int unixTime, RTSPNetworkTransport transport) {
    _gst->SetRtspTransport(transport);
    _gst->SetControlUri(this->_rtspCommands->GetControlUri());
    if (speed == 0) {
        speed = 1.0;
    }

    this->_gst->SetSpeed(speed);
    this->state = new PlayingState();
    return true;
}

bool Rtsp::Stream::StartLocalRecording(char* filePath, char* fileName) {
    if (this->_gst->GetMode() == IController::kStopped)
        return false;

    return this->_gst->StartLocalRecord(filePath, fileName);
}

void Rtsp::Stream::StopLocalRecording() {
        this->_gst->StopLocalRecord();
}

bool Rtsp::Stream::SnapShot(char* filePath, char* fileName) {
    if (this->_gst->GetMode() == IController::kStopped)
        return false;

    return this->_gst->SnapShot(filePath, fileName);
}

void Rtsp::Stream::NewRequest(MediaRequest& request) {
    this->_rtspCommands->ResetPath(request.dataInterface.dataEndpoint);
}
