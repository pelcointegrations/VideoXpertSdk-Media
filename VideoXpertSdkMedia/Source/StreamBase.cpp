#include "stdafx.h"
#include "StreamBase.h"

#include "GstWrapper.h"

using namespace std;
using namespace MediaController;
using namespace VxSdk;

StreamBase::StreamBase() {}

StreamBase::StreamBase(MediaRequest& request) :
   _mediaRequest(request)  {
    // Build a new pipeline using the description generated above.
    this->_gst = new GstWrapper();
}

StreamBase::~StreamBase() {
    if (this->_gst != nullptr) {
        delete this->_gst;
        this->_gst = nullptr;
    }

    if (this->state != nullptr) {
        delete this->state;
        this->state = nullptr;
    }
}

bool StreamBase::Play(float speed, unsigned int unixTime, RTSPNetworkTransport transport) { return false; }

void StreamBase::Pause() {}

void StreamBase::Stop() {}

void StreamBase::NewRequest(MediaRequest& request) {}

bool StreamBase::Resume(float speed, unsigned int unixTime, RTSPNetworkTransport transport) { return false; }

bool StreamBase::StoreStream(unsigned int startTime, unsigned int stopTime, char* filePath, char* fileName) { return false; }

bool StreamBase::StartLocalRecording(char* filePath, char* fileName) { return false; }

void StreamBase::StopLocalRecording() { }

bool StreamBase::SnapShot(char* filePath, char* fileName) { return false; }

GstWrapper* StreamBase::GetGstreamer() const {
    return this->_gst;
}

unsigned int StreamBase::GetLastTimestamp() const {
    return this->_gst->GetLastTimestamp();
}

Controller::Mode StreamBase::GetMode() {
    return this->_gst->GetMode();
}

void StreamBase::AddObserver(StreamEventCallback observer) {
    observerList.push_back(observer);
}

void StreamBase::RemoveObserver(StreamEventCallback observer) {
    observerList.erase(remove(observerList.begin(), observerList.end(), observer), observerList.end());
}

void StreamBase::ClearObservers() {
    observerList.clear();
}
