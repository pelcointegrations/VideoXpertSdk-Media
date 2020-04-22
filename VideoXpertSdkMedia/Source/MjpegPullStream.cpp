#include "stdafx.h"
#include "MjpegPullStream.h"

#include "GstWrapper.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <StreamState.h>

using namespace MediaController;
using namespace VxSdk;

MjpegPull::Stream::Stream(MediaRequest& request) : StreamBase(request) {
    Stream::NewRequest(request);
}

MjpegPull::Stream::~Stream() {}

bool MjpegPull::Stream::Play(float speed, unsigned int unixTime, RTSPNetworkTransport transport) {
    if (_dataSession == nullptr)
        return false;

    if (speed < 1.0f && speed > -1.0f) {
        if (speed >= 0.0f)
            speed = 1.0f;
        else
            speed = -1.0f;
    }

    // If the jpegUri is empty send a new stream request.
    if (_dataSession->jpegUri[0] == '\0')
        NewRequest(_mediaRequest);

    if (!this->_gst->IsPipelineActive())
        this->_gst->CreateMjpegPipeline(speed, _dataSession->jpegUri);

    if (unixTime == 0) {
        VxSdk::VxResult::Value ret = _dataSession->SetSpeed(speed);
        if (ret != VxSdk::VxResult::kOK)
            return false;

        ret = _dataSession->GoLive();
        if (ret != VxSdk::VxResult::kOK)
            return false;

        this->_gst->SetMode(IController::kLive);
    }
    else
    {
        VxSdk::VxResult::Value ret = _dataSession->Seek(unixTime, speed);
        if (ret != VxSdk::VxResult::kOK)
            return false;

        this->_gst->SetMode(IController::kPlayback);
        // Set the initial timestamp to the seek time.
        this->_gst->SetTimestamp(unixTime);
    }

    this->_gst->SetSpeed(speed);
    this->_gst->Play();
    this->state = new PlayingState();
    return true;
}

void MjpegPull::Stream::Pause() {
    if (_dataSession == nullptr)
        return;

    // If the jpegUri is empty do not send a request.
    if (_dataSession->jpegUri[0] == '\0')
        return;

    // Pausing the stream moves it to playback mode so the live stream session needs to be deleted.
    _dataSession->Pause();
    this->_gst->Pause();
    // The timestamp is set to the last received timestamp so Resume will start at the correct time.
    this->_gst->SetTimestamp(this->_gst->GetLastTimestamp());
    this->state = new PausedState();
}

void MjpegPull::Stream::Stop() {
    if (_dataSession == nullptr)
        return;

    // If the jpegUri is empty do not send a request.
    if (_dataSession->jpegUri[0] == '\0')
        return;

    _dataSession->DeleteDataSession();
    this->_gst->ClearPipeline();
    this->_gst->SetMode(IController::kStopped);
    this->state = new StoppedState();
}

bool MjpegPull::Stream::GoToLive() { return true; }

bool MjpegPull::Stream::Resume(float speed, unsigned int unixTime, RTSPNetworkTransport transport) {
    if (_dataSession == nullptr)
        return false;

    if (speed < 1.0f && speed > -1.0f) {
        if (speed >= 0.0f)
            speed = 1.0f;
        else
            speed = -1.0f;
    }

    // If the jpegUri is empty send a new stream request.
    if (_dataSession->jpegUri[0] == '\0')
        NewRequest(_mediaRequest);

    if (!this->_gst->IsPipelineActive())
        this->_gst->CreateMjpegPipeline(speed, _dataSession->jpegUri);

    unsigned int seekTime = unixTime == 0 ? this->_gst->GetLastTimestamp() : unixTime;
    // Seek to the last received timestamp generated during the Pause call.
    VxSdk::VxResult::Value ret = _dataSession->Seek(seekTime, speed);
    if (ret != VxSdk::VxResult::kOK)
        return false;

    // Set the mode to playback.
    this->_gst->SetMode(IController::kPlayback);
    // Set the initial timestamp to the seek time.
    this->_gst->SetTimestamp(seekTime);

    this->_gst->SetSpeed(speed);

    this->_gst->Play();
    this->state = new PlayingState();
    return true;
}

void MjpegPull::Stream::NewRequest(MediaRequest& request) {
    _mediaRequest = request;
    _mediaRequest.dataSource->CreateMjpegDataSession(_dataSession);
    if (_dataSession != nullptr) {
        char* authToken = nullptr;
        int size = 0;
        VxSdk::VxResult::Value result = _dataSession->GetAuthToken(authToken, size);
        if (result == VxSdk::VxResult::kInsufficientSize) {
            // Allocate enough space for authToken
            authToken = new char[size];
            // The result should now be kOK since we have allocated enough space
            _dataSession->GetAuthToken(authToken, size);
        }

        if (authToken != nullptr)
            this->_gst->SetCookie(std::string(authToken));
    }
}

bool MjpegPull::Stream::StoreStream(unsigned int startTime, unsigned int stopTime, char* filePath, char* fileName) { return false; }

bool MjpegPull::Stream::StartLocalRecording(char* filePath, char* fileName) { return false; }

void MjpegPull::Stream::StopLocalRecording() { }

bool MjpegPull::Stream::SnapShot(char* filePath, char* fileName) { return false; }

