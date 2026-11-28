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
