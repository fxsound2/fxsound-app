// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <atomic>
#include <cmath>

namespace fxanalysis
{
struct ClipEvents
{
    bool left = false;
    bool right = false;
};

// Sample-peak warning, not true-peak measurement or detection of prior clipping.
// Pure C++17; no allocation, audio modification, clock, FFT or UI dependency.
class ClipDetector final
{
public:
    // One 16-bit PCM step below full scale also catches 32767/32768 after conversion.
    static constexpr float threshold = 32767.0f / 32768.0f;

    // Interleaved mono/stereo. Mono is shown on both indicators.
    // Returns false for an invalid block; existing events remain pending.
    // NaN does not compare as a peak; infinities exceed the threshold. The host
    // remains responsible for reporting invalid audio data independently.
    bool processBlock(const float* pcm, int frames, int channels) noexcept
    {
        if (frames < 0 || (frames > 0 && pcm == nullptr) || (channels != 1 && channels != 2))
            return false;
        unsigned detected = 0;
        for (int frame = 0; frame < frames; ++frame, pcm += channels)
        {
            if (std::abs(pcm[0]) >= threshold) detected |= 1u;
            if (std::abs(pcm[channels == 1 ? 0 : 1]) >= threshold) detected |= 2u;
            if (detected == 3u) break;
        }
        if (detected != 0) pending_.fetch_or(detected, std::memory_order_relaxed);
        return true;
    }

    // One consumer. Atomic exchange cannot erase a concurrent publication:
    // it is returned now or remains pending for the next read.
    ClipEvents consume() noexcept
    {
        const auto detected = pending_.exchange(0, std::memory_order_relaxed);
        return {(detected & 1u) != 0, (detected & 2u) != 0};
    }

    // Explicit host lifecycle reset, discarding events published before this call.
    void reset() noexcept { pending_.store(0, std::memory_order_relaxed); }

private:
    static_assert(std::atomic<unsigned>::is_always_lock_free, "Audio peak flags must be lock-free");
    std::atomic<unsigned> pending_{0};
};
}
