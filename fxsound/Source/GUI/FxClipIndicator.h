// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <chrono>
#include <cmath>
#include <deque>

// UI-owned hold state. The interface supplies a monotonic clock; no audio-thread access.
class FxClipIndicator
{
public:
    using Clock = std::chrono::steady_clock;
    static constexpr auto holdTime = std::chrono::milliseconds(500);
    static constexpr float activityThresholdDb = 0.5f;
    static constexpr float strongReductionDb = 6.0f;

    void update(bool detected, Clock::time_point now) noexcept
    {
        if (detected) until_ = now + holdTime;
    }
    void updateLimiter(float reductionDb, Clock::time_point now)
    {
        while (!reductions_.empty() && reductions_.front().until <= now) reductions_.pop_front();
        if (std::isfinite(reductionDb) && reductionDb >= activityThresholdDb)
        {
            // UI-thread rolling maximum: a weaker event must neither prolong
            // an older strong warning nor disappear when that warning expires.
            while (!reductions_.empty() && reductions_.back().db <= reductionDb) reductions_.pop_back();
            reductions_.push_back({now + holdTime, reductionDb});
        }
    }
    bool isClipped(Clock::time_point now) const noexcept { return now < until_; }
    float reductionDb(Clock::time_point now) const noexcept
    {
        for (const auto& reduction : reductions_)
            if (now < reduction.until) return reduction.db;
        return 0.0f;
    }
    bool isLit(Clock::time_point now) const noexcept
    {
        return isClipped(now) || reductionDb(now) >= activityThresholdDb;
    }
    void reset() noexcept { until_ = {}; reductions_.clear(); }

private:
    Clock::time_point until_{};
    struct Reduction { Clock::time_point until; float db; };
    std::deque<Reduction> reductions_; // UI only; contains at most one entry per refresh in 500 ms.
};
