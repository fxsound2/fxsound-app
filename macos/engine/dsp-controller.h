#pragma once
#include "audio-bridge.h"
#include <memory>
#include <atomic>
#include <functional>
#include <mutex>

namespace fxsound {
class Float32Engine;
class DSPController {
public:
    DSPController();
    ~DSPController();
    bool available() const noexcept;
    void prepare(AudioBridge& bridge, const std::string& outputUID);
    // Control edits keep transport running; callback processing resumes after the edit.
    void edit(const std::function<void()>& mutation);
    void bypass(bool enabled) noexcept;
    bool bypassed() const noexcept { return bypass_.load(); }
    void preset(const std::string& path);
    void savePreset(const std::string& path);
    void parameter(const std::string& name, double value);
    static void validateParameter(const std::string& name, double value);
    void validateCurrentParameter(const std::string& name, double value) const;
    void numBands(int count);
    std::string stateJSON() const;
private:
    static void process(void*, float*, size_t) noexcept;
    void refreshState();
    mutable std::mutex stateMutex_;
    std::string controlsJSON_;
    std::unique_ptr<Float32Engine> dsp_;
#ifdef FXSOUND_HAVE_DSP
    std::array<float, 8192> wet_{};
    double mix_ = 0;
#endif
    std::atomic<bool> bypass_{true};
    std::atomic<bool> configuring_{false};
    std::atomic<unsigned> active_{0};
};
}
