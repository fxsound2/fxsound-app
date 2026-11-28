// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "BarPeakAccumulator.h"
#include "DisplayTiming.h"
#include <array>
#include <cstdint>

namespace fxanalysis
{
struct WaterfallSettings
{
    static constexpr int horizontalScale = 3;
    static constexpr int width = displayBandCount * horizontalScale;
    // Fixed algorithm: one temporal midpoint followed by the current observation.
    static constexpr int linesPerFrame = 2;
    static constexpr int queueCapacity = 512;
    static constexpr int refreshHz = 60;
};

using WaterfallProfile = std::array<float, WaterfallSettings::width>;

// Transient worker/UI message, not a retained history of numerical spectra.
struct WaterfallFrame
{
    StereoBarLevels levels;
    BandLevels frequencies{};
    double time = 0.0, sampleRate = 0.0;
    int hopSize = 0, channels = 0;
    std::uint64_t generation = 0, revision = 0;
    bool discontinuity = false;
    // A normalized mono profile on the same grid, used either in place of the
    // FFT bands (hasProfile) or over them (carrierCapture != 0).
    WaterfallProfile profile{};
    bool hasProfile = false;
    std::uint64_t sourceRevision = 0, carrierCapture = 0;
    double displayTimingPercent = DisplayTiming::defaultPercent;
    static WaterfallFrame fromSpectrum(const SpectrumFrame& f, std::uint64_t generation = 0)
    {
        WaterfallFrame frame{{f.barLeft, f.barRight}, f.bandHz, f.centreTimeSeconds(), f.sampleRate,
                f.hopSize, f.channels, generation, 0, false};
        return frame;
    }
};
}
