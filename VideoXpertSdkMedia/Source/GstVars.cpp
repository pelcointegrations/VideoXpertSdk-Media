#include "stdafx.h"
#include "GstVars.h"

using namespace MediaController;

GstVars::GstVars() {
    pipeline = nullptr;
    includeDateTimeInOverlay = false;
    stringToOverlay.clear();
    overlayPositionH = 0;
    overlayPositionV = 2;
    overlayLineAlignment = 0;
    streamEventObserverList.clear();
    observerList.clear();
    pelcoDataObserverList.clear();
    currentTimestamp = 0;
    lastTimestamp = 0;
    rtcpTimestamp = 0;
    mode = Controller::Mode::kStopped;
    speed = 1.0;
    seekTime = 0;
    endTime = 0;
    eventData = nullptr;
    aspectRatio = Controller::AspectRatios::k16x9;
    stretchToFit = false;
    isMjpeg = false;
    isRecording = false;
    isStoringVideo = false;
    isPaused = false;
    cookie.clear();
    windowHandle = NULL;
    busWatchId = 0;
    loop = nullptr;
    timerId = 0;
    transport = IStream::kUDP;
    workerThread = nullptr;
    videoSinkName = nullptr;
}

GstVars::~GstVars() { }