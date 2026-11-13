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
    void publish(float leftRatio, float rightRatio) noexcept
    {
        accumulate(left_, leftRatio);
        accumulate(right_, rightRatio);
    }
    LimiterLevels consume() noexcept
    {
        return {read(left_), read(right_)};
    }

private:
    static_assert(ATOMIC_INT_LOCK_FREE == 2, "Audio telemetry requires lock-free integers");
    static_assert(sizeof(unsigned) == sizeof(float) && std::numeric_limits<float>::is_iec559,
                  "Positive float bit patterns must preserve ordering");
    static void accumulate(std::atomic<unsigned>& pending, float ratio) noexcept
    {
        if (!(ratio > 1.0f) || !std::isfinite(ratio)) return;
        unsigned bits;
        std::memcpy(&bits, &ratio, sizeof(bits));
        auto previous = pending.load(std::memory_order_relaxed);
        while (bits > previous && !pending.compare_exchange_weak(previous, bits,
               std::memory_order_relaxed, std::memory_order_relaxed)) {}
    }
    static float read(std::atomic<unsigned>& pending) noexcept
    {
        const auto bits = pending.exchange(0, std::memory_order_relaxed);
        if (bits == 0) return 0.0f;
        float ratio;
        std::memcpy(&ratio, &bits, sizeof(ratio));
        return 20.0f * std::log10(ratio);
    }
    std::atomic<unsigned> left_{0}, right_{0};
};
}
