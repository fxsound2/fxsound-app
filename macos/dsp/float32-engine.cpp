#include "float32-engine.h"
#include "portable-runtime.h"
#include <cstdint>
namespace fxsound {
Float32Engine::Float32Engine() : dsp_(std::make_unique<DfxDsp>()) {}
Float32Engine::~Float32Engine() = default;
bool Float32Engine::prepare(int rate, int channels) {
    channels_ = sampleRate_ = 0;
    if (channels != 2 && channels != 4 && channels != 6 && channels != 8) return false;
    // Existing decimation is safe up to 96 kHz, with a separate 192 kHz /4 path.
    if (rate < 16000 || (rate > 96000 && rate != 192000)) return false;
    if (dsp_->setSignalFormat(32, channels, rate, 32) != 0) return false;
    sampleRate_ = rate;
    channels_ = channels;
    return true;
}
bool Float32Engine::loadPreset(const std::string &path) {
    try {
        if (dsp_->loadPreset(dspWide(path)) != 0) return false;
        // The Windows controller replays loaded values to update the live DSP.
        for (int index = 0; index < 5; ++index) {
            auto effect = static_cast<DfxDsp::Effect>(index);
            dsp_->setEffectValue(effect, dsp_->getEffectValue(effect) * 10);
        }
        for (int band = 0; band < dsp_->getNumEqBands(); ++band) {
            dsp_->setEqBandFrequency(band, dsp_->getEqBandFrequency(band));
            dsp_->setEqBandBoostCut(band, dsp_->getEqBandBoostCut(band));
        }
        return true;
    }
    catch (const std::range_error &) { return false; }
}
bool Float32Engine::process(const float *input, float *output, std::size_t frames) noexcept {
    if (!channels_ || !input || !output || frames > 16384) return false;
    if ((reinterpret_cast<std::uintptr_t>(input) % alignof(float)) ||
        (reinterpret_cast<std::uintptr_t>(output) % alignof(float))) return false;
    if (frames == 0) return true;
    if (bypass_) {
        std::memmove(output, input, frames * channels_ * sizeof(float));
        return true;
    }
    auto source = reinterpret_cast<std::uintptr_t>(input);
    auto destination = reinterpret_cast<std::uintptr_t>(output);
    auto bytes = frames * channels_ * sizeof(float);
    if (source != destination && (source < destination ? destination - source : source - destination) < bytes)
        return false;
    return dsp_->processAudio(reinterpret_cast<short *>(const_cast<float *>(input)),
        reinterpret_cast<short *>(output), static_cast<int>(frames), 0) == 0;
}
void Float32Engine::setPower(bool enabled) { bypass_ = !enabled; dsp_->powerOn(enabled); }
}
