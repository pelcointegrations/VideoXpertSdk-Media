#include "stdafx.h"
#include "Controller.h"

#include "MediaController.h"
#include "MediaRequest.h"
#include "StreamFactory.h"
#include "StreamBase.h"
#include "StreamState.h"
#include "GstWrapper.h"
#include <boost/thread.hpp>

using namespace MediaController;

//=======================================================
// Entry Point
//=======================================================

namespace MediaController {
    void GetController(MediaRequest* request, IController** control) {
        *control = nullptr;
        if (!request) return;
        *control = new Controller(*request);
    }
}

//=======================================================
// Controller
//=======================================================

Controller::Controller(MediaRequest& request) {
    this->stream = StreamFactory::CreateStream(request);
}

Controller::~Controller() {
    if (this->stream != nullptr) {
        delete this->stream;
        this->stream = nullptr;
    }
}

void Controller::SetWindow(void* handle) {
    if (this->stream != nullptr)
        this->stream->GetGstreamer()->SetWindowHandle(guintptr(handle));
}

void Controller::NewRequest(MediaRequest& request) {
    if (this->stream != nullptr)
        this->stream->NewRequest(request);
}

bool Controller::GoToLive() {
    bool result = true;
    if (this->stream != nullptr) {
        result &= this->stream->state->GoToLive(*this->stream);
    }

    return result;
}

bool Controller::Play(float speed, unsigned int unixTime, IStream::RTSPNetworkTransport transport) {
    if (stream != nullptr) {
        return  stream->state->Play(*stream, speed, unixTime, transport);
    }

    return false;
}

void Controller::Pause() {
    if (this->stream != nullptr)
        this->stream->state->Pause(*this->stream);
}

void Controller::Stop() {
    if (this->stream != nullptr)
        this->stream->state->Stop(*this->stream);
}

bool Controller::StoreStream(unsigned int startTime, unsigned int stopTime, char* filePath, char* fileName) {
    if (filePath == nullptr || fileName == nullptr || startTime == 0 || stopTime == 0 || stopTime <= startTime)
        return false;

    if (std::string(filePath).empty() || std::string(fileName).empty())
        return false;

    if (this->stream != nullptr)
        return this->stream->StoreStream(startTime, stopTime, filePath, fileName);

    return false;
}


bool Controller::StartLocalRecording(char* filePath, char* fileName) {
    if (filePath == nullptr || fileName == nullptr)
        return false;

    if (std::string(filePath).empty() || std::string(fileName).empty())
        return false;

    if (this->stream != nullptr)
        return this->stream->StartLocalRecording(filePath, fileName);

    return false;
}

void Controller::StopLocalRecording() {
    if (this->stream != nullptr)
        this->stream->StopLocalRecording();
}

bool Controller::SnapShot(char* filePath, char* fileName) {
    if (filePath == nullptr || fileName == nullptr)
        return false;

    if (std::string(filePath).empty() || std::string(fileName).empty())
        return false;

    if (this->stream != nullptr)
        return this->stream->SnapShot(filePath, fileName);

    return false;
}

void Controller::AddObserver(TimestampEventCallback observer) {
    if (this->stream != nullptr)
        this->stream->GetGstreamer()->AddObserver(observer);
}

void Controller::AddStreamObserver(StreamEventCallback observer) {
    if (this->stream != nullptr)
        this->stream->GetGstreamer()->AddStreamObserver(observer);
}

void Controller::AddPelcoDataObserver(PelcoDataEventCallback observer) {
    if (this->stream != nullptr)
        this->stream->GetGstreamer()->AddPelcoDataObserver(observer);
}

void Controller::AddEventData(void* customData) {
    if (this->stream != nullptr)
        this->stream->GetGstreamer()->AddEventData(customData);
}

void Controller::RemoveObserver(TimestampEventCallback observer) {
    if (this->stream != nullptr)
        this->stream->GetGstreamer()->RemoveObserver(observer);
}

void Controller::RemoveStreamObserver(StreamEventCallback observer) {
    if (this->stream != nullptr)
        this->stream->GetGstreamer()->RemoveStreamObserver(observer);
}

void Controller::RemovePelcoDataObserver(PelcoDataEventCallback observer) {
    if (this->stream != nullptr)
        this->stream->GetGstreamer()->RemovePelcoDataObserver(observer);
}


void Controller::ClearObservers() {
    if (this->stream != nullptr)
        this->stream->GetGstreamer()->ClearObservers();
}

void Controller::ClearStreamObservers() {
    if (this->stream != nullptr)
        this->stream->ClearObservers();
}

Controller::Mode Controller::GetMode() {
    if (this->stream != nullptr)
        return this->stream->GetGstreamer()->GetMode();

    return Controller::Mode::kStopped;
}

bool Controller::IsPipelineActive() {
    if (this->stream != nullptr)
        return this->stream->GetGstreamer()->IsPipelineActive();
    return false;
}

bool Controller::AddVideoOverlayData(std::string overlayData, VideoOverlayDataPosition position, bool includeDateTime) {
    if (this->stream != nullptr) {
        return this->stream->GetGstreamer()->SetOverlayString(overlayData, position, includeDateTime);
    }
    return false;
}

Controller::AspectRatios Controller::GetAspectRatio() {
    if (this->stream != nullptr) {
        return this->stream->GetGstreamer()->GetAspectRatio();
    }

    return Controller::AspectRatios::k16x9;
}

void Controller::SetAspectRatio(Controller::AspectRatios aspectRatio) {
    if (this->stream != nullptr) {
        this->stream->GetGstreamer()->SetAspectRatio(aspectRatio);
    }
}

bool Controller::GetStretchToFit() {
    if (this->stream != nullptr) {
        return this->stream->GetGstreamer()->GetStretchToFit();
    }

    return false;
}

void Controller::SetStretchToFit(bool stretchToFit) {
    if (this->stream != nullptr) {
        this->stream->GetGstreamer()->SetStretchToFit(stretchToFit);
    }
}

