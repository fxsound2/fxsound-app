#include "../engine/audio-bridge.h"
#include <cassert>
#include <iostream>
#include <stdexcept>
int main() {
    fxsound::AudioBridge bridge;
    assert(!bridge.ready());
    assert(bridge.inputDevice() == 0 && bridge.outputDevice() == 0);
    const auto status = bridge.metricsJSON();
    assert(status.find("\"ready\":false") != std::string::npos);
    assert(status.find("\"dspReady\":false") != std::string::npos);
    bool rejected = false;
    try { bridge.start(fxsound::virtualUID); }
    catch (const std::runtime_error& error) {
        rejected = std::string(error.what()).find("feedback") != std::string::npos;
    }
    assert(rejected);
    assert(!bridge.ready() && bridge.inputDevice() == 0 && bridge.outputDevice() == 0);
    bridge.stop(); bridge.stop();
    assert(!bridge.ready());
    std::cout << "bridge: initial readiness/feedback rejection/idempotent stop passed; no HAL calls\n";
}
