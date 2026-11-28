// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "PitchMotion.h"
#include <array>
#include <cstdint>

namespace fxanalysis::experimental
{
inline constexpr int harmonicProfileCapacity = 4096;
inline constexpr double harmonicProfileStepSemitones = 0.125;
inline constexpr int harmonicInputSpectrumCapacity =
    (static_cast<int>(harmonicCarrierMaximumHz - harmonicCarrierMinimumHz) * AnalysisLimits::maximumFftSize
        + AnalysisLimits::minimumSampleRate - 1) / AnalysisLimits::minimumSampleRate + 3;

struct HarmonicComponent
{
    double frequencyHz = 0.0, fitAmplitude = 0.0, displayAmplitude = 0.0;
    double amplitudeAtSelection = 0.0;
    int extractionRank = 0;
    double wideningPercent = 0.0;
};

struct HarmonicCombSnapshot
{
    PitchMotionResult status = PitchMotionResult::notPrepared;
    bool enabled = HarmonicCombSettings{}.enabled, hasEstimate = false;
    bool iterativeExtraction = HarmonicCombSettings{}.iterativeExtraction;
    std::uint64_t generation = 0;
    int componentCount = 0, extractionSteps = 0;
    std::array<HarmonicComponent, harmonicMaximumComponents> components{};
    std::array<double, harmonicMaximumComponents + 1> errorEnergy{};
    double inputEnergy = 0.0, residualErrorEnergy = 0.0, reconstructionError = 0.0, fitKktResidual = 0.0;
    int count = 0, fittedHarmonics = 0;
    double centreTimeSeconds = 0.0, stepSeconds = 0.0;
    double maximumFrequencyHz = 0.0;
    double selectedFrequencyHz = 0.0, selectedAmplitude = 0.0;
    double fitFraction = 0.0, removedEnergyFraction = 0.0;
    double subtractionFraction = 0.0; // No fit/subtraction in an empty observation.
    double displayGain = HarmonicCombSettings{}.displayGain, displayRangeDb = HarmonicCombSettings{}.displayRangeDb;
    double harmonicWideningPercent = HarmonicCombSettings{}.harmonicWideningPercent;
    double trailHalfLifeSeconds = HarmonicCombSettings{}.trailHalfLifeSeconds;
    bool logarithmicDisplay = HarmonicCombSettings{}.logarithmicDisplay, highResReference = HarmonicCombSettings{}.highResReference;
    int inputSpectrumCount = 0, firstInputBin = 0;
    double inputBinHz = 0.0;
    // Exact short-FFT modules before floor/prominence cleaning, no interpolation.
    // The two edge bins may sit just outside the carrier plot and are clipped by UI.
    std::array<float, harmonicInputSpectrumCapacity> inputMagnitude{};
    // FFT-domain diagnostic: fitted model and signed error of the cleaned input.
    std::array<float, harmonicInputSpectrumCapacity> modelMagnitude{}, errorMagnitude{};
    // Profiles are equivalent fundamental amplitudes weighted by observed bass support.
    // Each component's fitAmplitude retains its unweighted FFT-model coefficient.
    std::array<float, harmonicProfileCapacity> frequencyHz{}, before{}, after{};
};
}
