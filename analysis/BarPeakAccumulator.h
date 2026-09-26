// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "AudioAnalyzer.h"
#include <algorithm>

namespace fxanalysis
{
struct StereoBarLevels
{
    BandLevels left{}, right{};
};

// Finite, nonnegative display heights. One owner/consumer per instance; the host
// supplies synchronization. No allocation, clock, decay or audio processing.
class BarPeakAccumulator final
{
public:
    void add(const BandLevels& left, const BandLevels& right) noexcept
    {
        latest_ = {left, right};
        for (int b = 0; b < displayBandCount; ++b)
        {
            peaks_.left[b] = (std::max)(peaks_.left[b], left[b]);
            peaks_.right[b] = (std::max)(peaks_.right[b], right[b]);
        }
        pending_ = true;
    }

    // A second read without new data returns the latest instantaneous heights,
    // never the previous interval's maximum. Views consume only when pending.
    StereoBarLevels consume() noexcept
    {
        const auto result = pending_ ? peaks_ : latest_;
        discardPending();
        return result;
    }
    bool hasPending() const noexcept { return pending_; }
    void discardPending() noexcept { peaks_ = {}; pending_ = false; }
    void reset() noexcept { latest_ = {}; discardPending(); }

private:
    StereoBarLevels latest_, peaks_;
    bool pending_ = false;
};
}
