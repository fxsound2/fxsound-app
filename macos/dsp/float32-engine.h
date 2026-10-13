#pragma once
#include "DfxDsp.h"
#include <cstddef>
#include <memory>
#include <string>
namespace fxsound {
// One processing owner across instances: vocal and dither histories are shared.
// Serialize control calls; never configure or destroy an instance while it processes.
// Configuring another instance is safe for presets, effect values and fixed-band EQ gains.
// Shared binaural coefficients and EQ band count require all processing to stop.
class Float32Engine {
public:
    Float32Engine();
    ~Float32Engine();
    Float32Engine(const Float32Engine &) = delete;
    Float32Engine &operator=(const Float32Engine &) = delete;
    bool prepare(int sampleRate, int channels);
    bool loadPreset(const std::string &path);
    bool process(const float *input, float *output, std::size_t frames) noexcept;
    void setPower(bool enabled);
    DfxDsp &controls() noexcept { return *dsp_; }
    int channels() const noexcept { return channels_; }
    int sampleRate() const noexcept { return sampleRate_; }
private:
    std::unique_ptr<DfxDsp> dsp_;
    bool bypass_ = false;
    int channels_ = 0;
    int sampleRate_ = 0;
};
}
