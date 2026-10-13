#pragma once
#include "audio-devices.h"
#include "adaptive-resampler.h"
#include <atomic>
#include <array>

namespace fxsound {
class AudioBridge {
public:
    AudioBridge() = default;
    ~AudioBridge();
    void start(const std::string& outputUID);
    void stop() noexcept;
    bool ready() const noexcept;
    bool changed() noexcept { return changed_.exchange(false); }
    using Processor = void (*)(void*, float*, size_t) noexcept;
    void setProcessor(Processor processor, void* context);
    AudioDeviceID inputDevice() const noexcept { return input_; }
    AudioDeviceID outputDevice() const noexcept { return output_; }
    std::string metricsJSON() const;
private:
    static OSStatus capture(AudioDeviceID, const AudioTimeStamp*, const AudioBufferList*,
                            const AudioTimeStamp*, AudioBufferList*, const AudioTimeStamp*, void*);
    static OSStatus render(AudioDeviceID, const AudioTimeStamp*, const AudioBufferList*,
                           const AudioTimeStamp*, AudioBufferList*, const AudioTimeStamp*, void*);
    static OSStatus propertyChanged(AudioObjectID, UInt32, const AudioObjectPropertyAddress*, void*);
    void listeners(bool add);
    AudioDeviceID input_ = 0, output_ = 0;
    AudioDeviceIOProcID captureID_ = nullptr, renderID_ = nullptr;
    bool listening_ = false;
    StereoRing ring_;
    AdaptiveResampler resampler_;
    std::array<StereoFrame, 4096> scratch_{};
    std::array<float, 8192> renderScratch_{};
    Processor processor_ = nullptr;
    void* processorContext_ = nullptr;
    std::atomic<uint64_t> captured_{0}, rendered_{0}, delivered_{0}, overruns_{0}, underruns_{0}, hostTime_{0};
    std::atomic<uint64_t> captureTick_{0}, renderTick_{0};
    std::atomic<double> driftPPM_{0};
    std::atomic<float> peakLeft_{0}, peakRight_{0};
    std::atomic<bool> changed_{false};
    double outputRate_ = 0;
    double tickNanoseconds_ = 1;
};
}
