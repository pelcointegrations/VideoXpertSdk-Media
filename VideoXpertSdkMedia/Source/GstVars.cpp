#include "stdafx.h"
#include "GstVars.h"

using namespace MediaController;

GstVars::GstVars() {
    pipeline = nullptr;
    audioSource = nullptr;
    videoSource = nullptr;
    videoDecoder = nullptr;
    videoParse = nullptr;
    videoTee = nullptr;
    videoQueue = nullptr;
    videoSink = nullptr;
    teeQueue = nullptr;
    encoder = nullptr;
    muxer = nullptr;
    fileSink = nullptr;
    rtpBinManager = nullptr;
    videoTeePad = nullptr;
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
}

GstVars::~GstVars() { }