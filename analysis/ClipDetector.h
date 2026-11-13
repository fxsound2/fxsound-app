/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
    Sylvain Petitgrand (2026)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>

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
    using Generation = std::uint64_t;

    Generation generation() const noexcept { return pending_.load(std::memory_order_acquire) & ~flags; }

    bool processBlock(const float* pcm, int frames, int channels) noexcept
    {
        return processBlock(pcm, frames, channels, generation());
    }

    bool processBlock(const float* pcm, int frames, int channels, Generation generation) noexcept
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
        if (detected != 0)
        {
            auto previous = pending_.load(std::memory_order_relaxed);
            while ((previous & ~flags) == generation
                && !pending_.compare_exchange_weak(previous, previous | detected, std::memory_order_relaxed)) {}
        }
        return true;
    }

    // One consumer. Atomic exchange cannot erase a concurrent publication:
    // it is returned now or remains pending for the next read.
    ClipEvents consume() noexcept
    {
        const auto detected = pending_.fetch_and(~flags, std::memory_order_relaxed);
        return {(detected & 1u) != 0, (detected & 2u) != 0};
    }

    // Reset also rejects a producer that already began scanning the previous block.
    void reset() noexcept
    {
        auto previous = pending_.load(std::memory_order_relaxed);
        while (!pending_.compare_exchange_weak(previous, (previous & ~flags) + flags + 1,
                                                std::memory_order_acq_rel)) {}
    }

private:
    static constexpr Generation flags = 3;
    static_assert(std::atomic<Generation>::is_always_lock_free, "Audio peak flags must be lock-free");
    std::atomic<Generation> pending_{0};
};
}
