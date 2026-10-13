#pragma once
#include "audio-devices.h"
#include <stdexcept>
#include <functional>

namespace fxsound {
class RoutingGuard {
public:
    RoutingGuard() : previousUID_(deviceUID(defaultOutput())) {
        if (previousUID_ == virtualUID) {
            previousUID_.clear();
            for (const auto& device : devices()) if (device.outputs && device.uid != virtualUID) {
                previousUID_ = device.uid; break;
            }
        }
    }
    ~RoutingGuard() { restore(); }
    void recoverySink(std::function<void(const std::string&)> sink) { recovery_ = std::move(sink); }
    void activate(AudioDeviceID virtualDevice) {
        const auto current = deviceUID(defaultOutput());
        if (current != virtualUID) previousUID_ = current;
        if (previousUID_.empty()) throw std::runtime_error("no physical output available for recovery");
        if (recovery_) recovery_(previousUID_);
        setDefaultOutput(virtualDevice); owned_ = true;
    }
    bool restore() noexcept {
        if (!owned_) return true;
        try {
            if (deviceUID(defaultOutput()) == virtualUID) {
                AudioDeviceID fallback = 0;
                for (const auto& device : devices()) if (device.outputs && device.uid != virtualUID) {
                    if (!fallback || device.uid == previousUID_) fallback = device.id;
                    if (device.uid == previousUID_) break;
                }
                if (!fallback) return false;
                setDefaultOutput(fallback);
                if (deviceUID(defaultOutput()) == virtualUID) return false;
            }
        } catch (...) { return false; }
        owned_ = false;
        return true;
    }
    bool owned() const noexcept { return owned_; }
private:
    std::string previousUID_;
    bool owned_ = false;
    std::function<void(const std::string&)> recovery_;
};
}
