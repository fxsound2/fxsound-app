/*
FxSound — Linux build: PipeWire realtime process block

The body of the pw_filter process callback, kept free of PipeWire so it can be
driven directly by tests. Takes planar stereo float buffers, runs DfxDsp on an
interleaved copy and hard-clips the result.
*/

#pragma once

#include "DfxDsp.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

class FxProcessor
{
public:
    static constexpr int kChannels = 2;

    void setDsp(DfxDsp* dsp) { dsp_ = dsp; }
    void setMuted(bool mute) { muted_.store(mute, std::memory_order_relaxed); }

    // in[c] may be null (port not connected); out[c] must be valid for n frames.
    void process(const float* const in[kChannels], float* const out[kChannels], uint32_t n, int rate)
    {
        if (muted_.load(std::memory_order_relaxed)) {
            for (int c = 0; c < kChannels; ++c) std::memset(out[c], 0, n * sizeof(float));
            return;
        }

        bool have_in = in[0] && in[1];
        if (dsp_ && have_in) {
            // Process in native 32-bit float (PipeWire's format) so there is no
            // 16-bit quantization loss. The DSP must be told the format or it
            // produces no output; bps=32 routes it through the float path.
            if (configured_rate_ != rate) {
                dsp_->setSignalFormat(32, kChannels, rate, 32);
                configured_rate_ = rate;
            }
            if (ileave_in_.size() < n * kChannels) {
                ileave_in_.resize(n * kChannels);
                ileave_out_.resize(n * kChannels);
            }
            for (uint32_t i = 0; i < n; ++i) {
                ileave_in_[i*2]     = in[0][i];
                ileave_in_[i*2 + 1] = in[1][i];
            }
            // processAudio takes short*; for bps=32 the engine reinterprets the
            // buffers as 32-bit float internally.
            dsp_->processAudio(reinterpret_cast<short*>(ileave_in_.data()),
                               reinterpret_cast<short*>(ileave_out_.data()), (int)n, 0);
            for (uint32_t i = 0; i < n; ++i) {
                float l = ileave_out_[i*2];
                float r = ileave_out_[i*2 + 1];
                // Final hard-clip to prevent digital distortion reaching PipeWire/mixer.
                if (l > 1.0f) l = 1.0f; else if (l < -1.0f) l = -1.0f;
                if (r > 1.0f) r = 1.0f; else if (r < -1.0f) r = -1.0f;
                out[0][i] = l;
                out[1][i] = r;
            }
        } else if (have_in) {
            for (int c = 0; c < kChannels; ++c) std::memcpy(out[c], in[c], n * sizeof(float));
        } else {
            for (int c = 0; c < kChannels; ++c) std::memset(out[c], 0, n * sizeof(float));
        }
    }

private:
    DfxDsp* dsp_ = nullptr;
    std::atomic<bool> muted_{false};
    int configured_rate_ = 0;   // last rate passed to dsp->setSignalFormat
    std::vector<float> ileave_in_;     // interleaved L/R, 32-bit float
    std::vector<float> ileave_out_;
};
