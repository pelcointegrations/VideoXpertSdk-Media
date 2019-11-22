#include "stdafx.h"
#include "GstVars.h"

using namespace MediaController;

GstVars::GstVars() {
    isRecording = false;
    isPipelineActive = false;
    includeDateTimeInOverlay = false;
    stringToOverlay.clear();
    overlayPositionV = 2;
    overlayPositionH = 0;
    overlayLineAlignment = 0;
    seekTime = 0;
    speed = 1.0;
    timerId = 0;
    storeVideoFast = false;
    stretchToFit = false;
    textOverlay = nullptr;
}

GstVars::~GstVars() { }