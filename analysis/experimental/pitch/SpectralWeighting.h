// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "PitchExperimentSettings.h"
#include "../../AudioAnalyzer.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace fxanalysis::experimental
{
inline bool validSpectralWeightingSettings(const PitchMotionSettings& settings) noexcept
{
    return std::isfinite(settings.noiseFloorStrength)
        && settings.noiseFloorStrength >= 0.0 && settings.noiseFloorStrength <= 8.0
        && std::isfinite(settings.minimumPeakProminenceDb)
        && settings.minimumPeakProminenceDb >= 0.0 && settings.minimumPeakProminenceDb <= 60.0
        && std::isfinite(settings.peakWeightExponent)
        && settings.peakWeightExponent >= 1.0 && settings.peakWeightExponent <= 4.0;
}

// Callers validate the full frame first, so malformed data retains their explicit
// nonFiniteInput/invalidFrame status. These local defenses only prevent bad reads.
// Bounded conservative floor: empirical lower quartile of at most 24 neighbors.
// Exclude the +/-2-bin Hann main lobe. No distribution-dependent calibration or
// per-frequency whitening is applied; this is an estimated background, not an SNR.
inline double spectralNoiseFloor(const SpectrumFrame& frame, int bin) noexcept
{
    const int last = frame.fftSize / 2 - 1;
    if (bin < 1 || bin > last || last < 1
        || frame.power.size() <= static_cast<std::size_t>(last)) return 0.0;
    std::array<double, 24> neighbors{};
    int count = 0;
    for (int offset = 3; offset <= 14; ++offset)
    {
        for (int neighbor : {bin - offset, bin + offset})
        {
            if (neighbor < 1 || neighbor > last) continue;
            const double power = frame.power[neighbor];
            if (std::isfinite(power) && power >= 0.0) neighbors[count++] = power;
        }
    }
    if (count == 0) return 0.0;
    const int quartile = (count - 1) / 4;
    std::nth_element(neighbors.begin(), neighbors.begin() + quartile, neighbors.begin() + count);
    return neighbors[quartile];
}

// Return cleaned POWER, before peak-energy accumulation, frequency regridding or
// exponentiation. UOT later uses E^gamma; module correlation uses E^(gamma/2).
// Prominence is P/background before subtraction. It is relative to this local
// floor estimate, not a calibrated signal-to-noise ratio.
inline double cleanSpectralPower(const SpectrumFrame& frame, int bin,
    const PitchMotionSettings& settings) noexcept
{
    if (bin < 0 || static_cast<std::size_t>(bin) >= frame.power.size()) return 0.0;
    const double power = frame.power[bin];
    if (!std::isfinite(power) || power <= 0.0) return 0.0;
    if (settings.noiseFloorStrength == 0.0 && settings.minimumPeakProminenceDb == 0.0)
        return power;
    const double background = spectralNoiseFloor(frame, bin);
    if (background > 0.0 && settings.minimumPeakProminenceDb > 0.0
        && power < background * std::pow(10.0, settings.minimumPeakProminenceDb / 10.0))
        return 0.0;
    return (std::max)(0.0, power - settings.noiseFloorStrength * background);
}
}