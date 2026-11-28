#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>

namespace fxsoundtiming
{
    constexpr unsigned defaultReserveMs = 90;
    constexpr unsigned maxReserveMs = 300;

    // FxSound statically links AudioPassthru. Its UI and renderer share this control object.
    struct RelayControl
    {
        std::atomic<unsigned> targetMs{defaultReserveMs};
        std::atomic<int> observedMs{-1}, appliedTargetMs{0};
        std::atomic<bool> transitioning{false};
        std::atomic<unsigned long long> lastWriteTick{0};
    };
    inline RelayControl& playbackReserveControl()
    {
        static RelayControl value;
        return value;
    }
    inline unsigned targetFrames(unsigned rate, unsigned capacity, unsigned packet, unsigned ms)
    {
        if (capacity <= packet) return 0;
        const auto wanted = static_cast<unsigned>(static_cast<std::uint64_t>(rate) *
            (std::min)(ms, maxReserveMs) / 1000);
        return (std::min)(wanted, capacity - packet);
    }
    struct PacketPlan
    {
        unsigned frames = 0;
        bool transition = false;
    };

    // The finite correction debt is changed only by a user request or a new stream.
    // Normal playback uses a bit-identical copy and never chases padding jitter.
    struct ReserveTransition
    {
        unsigned requested = defaultReserveMs;
        unsigned effective = 0;
        int remaining = 0;
        unsigned fractionalBudget = 0;
        bool ready = false;

        void prime(unsigned ms, unsigned target)
        {
            requested = ms;
            effective = target;
            remaining = 0;
            fractionalBudget = 0;
            ready = true;
        }
        PacketPlan plan(unsigned frames, unsigned ms, unsigned target, unsigned padding)
        {
            if (!ready) prime(ms, target);
            if (ms != requested)
            {
                remaining = remaining ? remaining + static_cast<int>(target) - static_cast<int>(effective)
                                      : static_cast<int>(target) - static_cast<int>(padding);
                requested = ms;
                effective = target;
            }
            if (!remaining || frames < 2) return {frames, remaining != 0};
            // A 2% maximum speed change during a transition. Accumulate sub-frame steps.
            const unsigned budget = frames + fractionalBudget;
            fractionalBudget = budget % 50;
            const int limit = static_cast<int>((std::min)(budget / 50, frames - 2));
            const int delta = (std::max)(-limit, (std::min)(remaining, limit));
            remaining -= delta;
            return {static_cast<unsigned>(static_cast<int>(frames) + delta), true};
        }
    };

    inline void writeFrames(float* dst, const float* input, unsigned inputFrames, unsigned outputFrames,
                            unsigned first, unsigned count, unsigned channels)
    {
        if (inputFrames == outputFrames)
        {
            const auto offset = static_cast<std::size_t>(first) * channels;
            std::copy_n(input + offset, static_cast<std::size_t>(count) * channels, dst);
            return;
        }
        for (unsigned i = 0; i < count; ++i)
        {
            const auto numerator = static_cast<std::uint64_t>(first + i) * (inputFrames - 1);
            const unsigned left = static_cast<unsigned>(numerator / (outputFrames - 1));
            const unsigned right = (std::min)(left + 1, inputFrames - 1);
            const float fraction = static_cast<float>(numerator % (outputFrames - 1)) / (outputFrames - 1);
            for (unsigned c = 0; c < channels; ++c)
            {
                const float a = input[static_cast<std::size_t>(left) * channels + c];
                const float b = input[static_cast<std::size_t>(right) * channels + c];
                *dst++ = a + fraction * (b - a);
            }
        }
    }
}
