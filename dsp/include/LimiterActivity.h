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
#include <cstring>
#include <limits>
#include <cstdint>

namespace fxdsp
{
struct LimiterLevels
{
    float leftDb = 0.0f;
    float rightDb = 0.0f;
};

// One audio producer, one UI consumer. Retains the largest reduction per channel.
// Input is reciprocal gain (>1 while limiting); logarithms run on the consumer.
// The existing limiter supplies envelope / ceiling.
class LimiterActivity final
{
public:
    using Generation = std::uint64_t;

    Generation generation() const noexcept { return generation_.load(std::memory_order_acquire); }

    void publish(float leftRatio, float rightRatio) noexcept
    {
        publish(leftRatio, rightRatio, generation());
    }

    void publish(float leftRatio, float rightRatio, Generation generation) noexcept
    {
        accumulate(left_, leftRatio, generation);
        accumulate(right_, rightRatio, generation);
    }

    // Called on the control thread when the displayed output changes or is muted.
    void discard() noexcept
    {
        const auto next = generation() + generationStep;
        left_.store(next, std::memory_order_release);
        right_.store(next, std::memory_order_release);
        generation_.store(next, std::memory_order_release);
    }
    LimiterLevels consume() noexcept
    {
        return {read(left_), read(right_)};
    }

private:
    static_assert(ATOMIC_LLONG_LOCK_FREE == 2, "Audio telemetry requires lock-free integers");
    static constexpr Generation generationStep = Generation{1} << 32;
    static constexpr Generation ratioMask = generationStep - 1;
    static_assert(sizeof(unsigned) == sizeof(float) && std::numeric_limits<float>::is_iec559,
                  "Positive float bit patterns must preserve ordering");
    static void accumulate(std::atomic<Generation>& pending, float ratio, Generation generation) noexcept
    {
        if (!(ratio > 1.0f) || !std::isfinite(ratio)) return;
        unsigned bits;
        std::memcpy(&bits, &ratio, sizeof(bits));
        auto previous = pending.load(std::memory_order_relaxed);
        while ((previous & ~ratioMask) == generation)
        {
            // Confirm the maximum atomically even when it equals the prior peak:
            // a reader may have cleared the latch since the initial load.
            const auto peak = (previous & ratioMask) > bits ? (previous & ratioMask) : bits;
            if (pending.compare_exchange_weak(previous, generation | peak, std::memory_order_relaxed)) break;
        }
    }
    static float read(std::atomic<Generation>& pending) noexcept
    {
        const auto bits = static_cast<unsigned>(pending.fetch_and(~ratioMask, std::memory_order_relaxed));
        if (bits == 0) return 0.0f;
        float ratio;
        std::memcpy(&ratio, &bits, sizeof(ratio));
        return 20.0f * std::log10(ratio);
    }
    std::atomic<Generation> generation_{0}, left_{0}, right_{0};
};
}
