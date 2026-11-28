// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "HarmonicCombProfile.h"
#include "../../FrequencyScale.h"
#include "../../WaterfallFrame.h"

namespace fxanalysis::experimental
{
inline constexpr std::uint32_t harmonicCarrierGold = 0xffffbe5c;
inline constexpr double markerHalfWidthSemitones = 0.5;
inline constexpr double minimumMarkerSigmaPixels = 5.0;
inline constexpr double markerSupportSigmas = 3.0;
inline constexpr double carrierHistorySigmaScale = 0.4;

inline double harmonicPlotPosition(const HarmonicCombSnapshot& snapshot, bool compact, double hz) noexcept
{
    const double high = snapshot.maximumFrequencyHz > FrequencySettings::minimumHz
        ? snapshot.maximumFrequencyHz : FrequencySettings::maximumHz;
    const FrequencyScale scale(high);
    // The compact strip and carrier History share the full spectrum axis.
    if (compact) return scale.position(hz);
    const double low = scale.position(harmonicCarrierMinimumHz);
    return (scale.position(hz) - low) / (scale.position(harmonicCarrierMaximumHz) - low);
}

// Display footprint, unrelated to the adaptive width of the fitted FFT teeth.
// Sigma is half a semitone, with a five-pixel floor on the target grid.
inline double harmonicMarkerSigmaPixels(const HarmonicCombSnapshot& snapshot, bool compact,
    double hz, double width) noexcept
{
    const double ratio = std::exp2(markerHalfWidthSemitones / semitonesPerOctave);
    return (std::max)(minimumMarkerSigmaPixels, 0.5 * width * (harmonicPlotPosition(snapshot, compact, hz * ratio)
        - harmonicPlotPosition(snapshot, compact, hz / ratio)));
}

inline double harmonicMarkerLobe(double offset, double sigma) noexcept
{
    const double x = offset / sigma;
    return std::abs(x) <= markerSupportSigmas ? std::exp(-0.5 * x * x) : 0.0;
}

struct HarmonicDisplayLobe
{
    double centre = 0.0, height = 0.0, sigma = minimumMarkerSigmaPixels;
};

inline HarmonicDisplayLobe harmonicDisplayLobe(const HarmonicCombSnapshot& snapshot,
    bool compact, const HarmonicComponent& component, double width) noexcept
{
    if (!std::isfinite(component.frequencyHz) || component.frequencyHz <= 0.0
        || !std::isfinite(width) || width <= 0.0) return {};
    return {harmonicPlotPosition(snapshot, compact, component.frequencyHz) * width,
        harmonicDisplayHeight(component.displayAmplitude, snapshot.displayGain,
            snapshot.logarithmicDisplay, snapshot.displayRangeDb),
        harmonicMarkerSigmaPixels(snapshot, compact, component.frequencyHz, width)};
}

// Current carrier observation only. History owns scrolling; the compact strip's
// fading stamps must not be folded into these rows. Levels are already normalized:
// apply the selected amplitude scale before the Gaussian, exactly as in the strip.
inline WaterfallProfile harmonicCarrierProfile(const HarmonicCombSnapshot& snapshot) noexcept
{
    WaterfallProfile profile{};
    if (!snapshot.enabled || !snapshot.hasEstimate || !snapshot.iterativeExtraction) return profile;
    std::array<HarmonicDisplayLobe, harmonicMaximumComponents> lobes{};
    const int count = std::clamp(snapshot.componentCount, 0, harmonicMaximumComponents);
    for (int i = 0; i < count; ++i)
    {
        lobes[i] = harmonicDisplayLobe(snapshot, true, snapshot.components[i], WaterfallSettings::width);
        lobes[i].sigma *= carrierHistorySigmaScale;
    }
    for (int x = 0; x < WaterfallSettings::width; ++x)
    {
        double height = 0.0;
        for (int i = 0; i < count; ++i)
            height = (std::max)(height, lobes[i].height
                * harmonicMarkerLobe(x + 0.5 - lobes[i].centre, lobes[i].sigma));
        profile[x] = static_cast<float>(height);
    }
    return profile;
}
}
