#include "stdafx.h"
#include "RtspStream.h"

#include "Controller.h"
#include "GstWrapper.h"
#include <StreamState.h>

using namespace std;
using namespace MediaController;
using namespace Constants;
using namespace VxSdk;

Rtsp::Stream::Stream(MediaRequest& request) : StreamBase(request) { }

Rtsp::Stream::~Stream() { }

bool Rtsp::Stream::Play(float speed, unsigned int unixTime, RTSPNetworkTransport transport) {
    if (this->_gst->GetMode() != IController::kStopped)
        this->Stop();

    if (speed == 0 && unixTime == 0)
        this->_gst->SetMode(IController::kLive);
    else
        this->_gst->SetMode(IController::kPlayback);

    _gst->CreateRtspPipeline(speed, unixTime, this->_mediaRequest, transport);
    this->_gst->Play();
    if (this->state != nullptr) {
        delete this->state;
        this->state = nullptr;
    }

    this->state = new PlayingState();

    return true;
}

void Rtsp::Stream::Pause() {
    // Pause the stream.
    this->_gst->Pause();
    this->state = new PausedState();
}

void Rtsp::Stream::Stop() {
    this->_gst->ClearPipeline();
    this->_gst->SetMode(IController::kStopped);
    if (this->state != nullptr) {
        delete this->state;
        this->state = nullptr;
    }

    this->state = new StoppedState();
}

bool Rtsp::Stream::GoToLive() { return true; }

bool Rtsp::Stream::Resume(float speed, unsigned int unixTime, RTSPNetworkTransport transport) {
    if (speed == 0) {
        speed = 1.0;
    }

    this->_gst->SetSpeed(speed);
    if (this->state != nullptr) {
        delete this->state;
        this->state = nullptr;
    }

    this->state = new PlayingState();
    return true;
}

bool Rtsp::Stream::StoreStream(unsigned int startTime, unsigned int stopTime, char* filePath, char* fileName) {
    if (this->_gst->GetMode() != IController::kStopped)
        this->Stop();

    this->_gst->SetMode(IController::kPlayback);
    this->_gst->StoreVideo(filePath, fileName, startTime, stopTime, this->_mediaRequest);
    this->_gst->Play();
    if (this->state != nullptr) {
        delete this->state;
        this->state = nullptr;
    }

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
    this->_mediaRequest = request;
    this->_gst->ChangeRtspLocation(request);
}
