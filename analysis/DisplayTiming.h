// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>

namespace fxanalysis
{
// All four durations are audio-clock time constants in milliseconds.
struct DisplayTiming
{
    double levelReleaseMs; // A dip rearms the next bar attack. Zero follows the input directly.
    double attackDecayMs;  // Release of the accumulated positive changes in level.
    double peakReleaseMs;  // Second pole rounds the falling attack peak.
    double carrierLevelMs; // Smooth amplitude only where a carrier pixel persists across two hops.

    static constexpr int minimumPercent = 0, maximumPercent = 100;
    static constexpr int defaultPercent = (minimumPercent + maximumPercent) / 2;
    // Reserve the central-left segment for every position from the previous slider.
    static constexpr int previousStartPercent = (minimumPercent + defaultPercent) / 2;
    static constexpr double previousScale = static_cast<double>(defaultPercent - previousStartPercent)
        / (maximumPercent - minimumPercent);
    // One slider step can select every integer percentage saved on the previous scale.
    static constexpr double stepPercent = previousScale;
    static constexpr double millisecondsPerSecond = 1000.0;

    static constexpr double fromPreviousPercent(int previousPercent) noexcept
    {
        return previousStartPercent + std::clamp(previousPercent, minimumPercent, maximumPercent) * previousScale;
    }
    static double limitPercent(double percent) noexcept
    {
        if (!std::isfinite(percent)) return defaultPercent;
        const double bounded = std::clamp(percent, double(minimumPercent), double(maximumPercent));
        return std::round(bounded / stepPercent) * stepPercent;
    }
    static DisplayTiming atPercent(double percent) noexcept;
    static constexpr double seconds(double milliseconds) noexcept { return milliseconds / millisecondsPerSecond; }
    static double retention(double hopSeconds, double timeMs) noexcept
    {
        return timeMs > 0.0 ? std::exp(-hopSeconds / seconds(timeMs)) : 0.0;
    }
};

// Editable anchors in milliseconds, from left to right on the slider.
// The historical and reference columns preserve the complete former response.
namespace displayTimingRanges
{
struct Milliseconds
{
    double sharp, historical, reference, smooth;
    constexpr double at(double percent) const noexcept
    {
        const auto between = [percent](double leftPercent, double rightPercent, double leftMs, double rightMs)
        {
            const double mix = (percent - leftPercent) / (rightPercent - leftPercent);
            return leftMs + mix * (rightMs - leftMs);
        };
        if (percent < DisplayTiming::previousStartPercent)
            return between(DisplayTiming::minimumPercent, DisplayTiming::previousStartPercent, sharp, historical);
        if (percent < DisplayTiming::defaultPercent)
            return between(DisplayTiming::previousStartPercent, DisplayTiming::defaultPercent, historical, reference);
        if (percent == DisplayTiming::maximumPercent) return smooth;
        return between(DisplayTiming::defaultPercent, DisplayTiming::maximumPercent, reference, smooth);
    }
    constexpr double shortest() const noexcept
    {
        return (std::min)({sharp, historical, reference, smooth});
    }
};

// Above the default, slow the rearming of small attacks and the amplitude of persistent
// carrier pixels. Keep the two attack-release poles at their reference durations.
inline constexpr double referenceAttackDecayMs = 60.0;
inline constexpr double referencePeakReleaseMs = 24.0;
//                                            Sharp   Historical   Reference                Smooth
inline constexpr Milliseconds levelRelease {    0.0,       0.0,     22.0,                    132.0 };
inline constexpr Milliseconds attackDecay  {   20.0,      70.0,     referenceAttackDecayMs,  referenceAttackDecayMs };
inline constexpr Milliseconds peakRelease  {    0.0,      30.0,     referencePeakReleaseMs,  referencePeakReleaseMs };
inline constexpr Milliseconds carrierLevel {    0.0,       0.0,     16.0,                    128.0 };
static_assert(attackDecay.shortest() > 0.0 && levelRelease.shortest() >= 0.0
    && peakRelease.shortest() >= 0.0 && carrierLevel.shortest() >= 0.0);
}

inline DisplayTiming DisplayTiming::atPercent(double percent) noexcept
{
    const double position = limitPercent(percent);
    return {displayTimingRanges::levelRelease.at(position), displayTimingRanges::attackDecay.at(position),
            displayTimingRanges::peakRelease.at(position), displayTimingRanges::carrierLevel.at(position)};
}
}
