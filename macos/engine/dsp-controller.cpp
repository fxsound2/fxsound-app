#include "dsp-controller.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <filesystem>
#include <sstream>
#include <chrono>
#include <thread>
#ifdef FXSOUND_HAVE_DSP
#include "float32-engine.h"
#else
namespace fxsound { class Float32Engine {}; }
#endif

namespace fxsound {
static_assert(std::atomic<bool>::is_always_lock_free && std::atomic<unsigned>::is_always_lock_free);
DSPController::DSPController() {
#ifdef FXSOUND_HAVE_DSP
    dsp_ = std::make_unique<Float32Engine>();
#endif
    refreshState();
}
DSPController::~DSPController() = default;
bool DSPController::available() const noexcept { return bool(dsp_); }
void DSPController::prepare(AudioBridge& bridge, const std::string& outputUID) {
    bridge.stop();
#ifdef FXSOUND_HAVE_DSP
    const auto format = streamFormat(findDevice(outputUID, false).id, false);
    if (!dsp_->prepare(static_cast<int>(format.mSampleRate), 2)) throw std::runtime_error("DSP format preparation failed");
    mix_ = bypass_.load() ? 0 : 1;
    refreshState();
    bridge.setProcessor(process, this);
#else
    (void)outputUID;
    bridge.setProcessor(nullptr, nullptr);
#endif
}
void DSPController::edit(const std::function<void()>& mutation) {
    bool expected = false;
    if (!configuring_.compare_exchange_strong(expected, true))
        throw std::runtime_error("DSP edit already in progress");
    struct Reset { std::atomic<bool>& flag; ~Reset() { flag.store(false); } } reset{configuring_};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (active_.load() != 0) {
        if (std::chrono::steady_clock::now() >= deadline)
            throw std::runtime_error("DSP callback did not finish within one second");
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    try { mutation(); refreshState(); }
    catch (...) {
        try { refreshState(); } catch (...) {}
        throw;
    }
}
void DSPController::bypass(bool enabled) noexcept { bypass_.store(enabled); }
void DSPController::process(void* context, float* samples, size_t frames) noexcept {
    auto& self = *static_cast<DSPController*>(context);
#ifdef FXSOUND_HAVE_DSP
    if (self.configuring_.load()) { self.mix_ = 0; return; }
    self.active_.fetch_add(1);
    struct Leave { std::atomic<unsigned>& count; ~Leave() { count.fetch_sub(1); } } leave{self.active_};
    // An edit may begin between the first flag check and registering this callback.
    if (self.configuring_.load()) { self.mix_ = 0; return; }
    if (frames > self.wet_.size() / 2) return;
    if (!self.dsp_->process(samples, self.wet_.data(), frames)) {
        self.bypass_ = true; self.mix_ = 0; return;
    }
    const double goal = self.bypass_.load(std::memory_order_relaxed) ? 0 : 1;
    for (size_t i = 0; i < frames; ++i) {
        self.mix_ += std::clamp(goal - self.mix_, -1.0 / 128, 1.0 / 128);
        for (size_t channel = 0; channel < 2; ++channel) {
            const size_t n = i * 2 + channel;
            if (self.mix_ == 1) samples[n] = self.wet_[n];
            else if (self.mix_ != 0) samples[n] += static_cast<float>((self.wet_[n] - samples[n]) * self.mix_);
        }
    }
#else
    (void)self; (void)samples; (void)frames;
#endif
}
void DSPController::preset(const std::string& path) {
#ifdef FXSOUND_HAVE_DSP
    if (!dsp_->loadPreset(path)) throw std::runtime_error("preset parse/apply failed");
#else
    (void)path; throw std::runtime_error("DSP unavailable in this build");
#endif
}
}
