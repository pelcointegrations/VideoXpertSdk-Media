#include "stdafx.h"
#include "RtspKeepAlive.h"
#include <thread>
#include <atomic>

struct MediaController::Rtsp::KeepAlive::ThreadInfo {
    std::thread _keepAliveThread;
    std::atomic<bool> _shutdownRequested;
};

MediaController::Rtsp::KeepAlive::KeepAlive(Commands* commands, std::vector<StreamEventCallback> observerList) :
    _commands(commands), _observerList(observerList), d_ptr(new ThreadInfo()) {
    d_ptr->_shutdownRequested = false;
    d_ptr->_keepAliveThread = std::thread(&KeepAlive::GetParamsLoop, this);
}

MediaController::Rtsp::KeepAlive::~KeepAlive() {
    d_ptr->_shutdownRequested = true;
    d_ptr->_keepAliveThread.join();
}

void MediaController::Rtsp::KeepAlive::GetParamsLoop() {
    try
    {
        int _count = 0;
        int _failCount = 0;
        while (!d_ptr->_shutdownRequested) {
            if (_count < Constants::kKeepAliveRefreshSec) {
                ++_count;
            }
            else {
                if (_commands->GetParameter())
                    _failCount = 0;
                else
                    _failCount += 1;

                _count = 0;
                if (_failCount >= 2) {
                    for (size_t i = 0; i < _observerList.size(); i++) {
                        StreamEvent* newEvent = new StreamEvent();
                        newEvent->eventType = StreamEvent::Type::kConnectionLost;
                        _observerList[i](newEvent);
                        delete newEvent;
                    }
                    return;
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    catch (...) { }
}
