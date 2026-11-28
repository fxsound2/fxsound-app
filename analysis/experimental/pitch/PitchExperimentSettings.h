// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

// Optional observations are enabled by runtime requests, never by playback.

namespace fxanalysis::experimental
{
// Observation UI state, separate from whether a numerical model is enabled.
enum class CompactIndicatorMode { none, pitch, carriers };

inline constexpr int pitchMaximumPeaks = 64;
inline constexpr int pitchHistogramBinCount = 97;
inline constexpr double pitchHistogramMinimumSemitones = -12.0;
inline constexpr double pitchHistogramStepSemitones = 0.25;

inline constexpr double harmonicMaximumWideningPercent = 80.0;
inline constexpr double harmonicDefaultWideningPercent = 50.0;
inline constexpr double harmonicCarrierMinimumHz = 100.0;
inline constexpr double harmonicCarrierMaximumHz = 3000.0;
inline constexpr int harmonicMaximumHarmonics = 64, harmonicMaximumComponents = 4;
inline constexpr double semitonesPerOctave = 12.0;

// The validator and the experiment sliders use the same endpoints.
struct HarmonicControlRange
{
    double minimum, maximum, step;
    constexpr bool contains(double value) const noexcept { return value >= minimum && value <= maximum; }
};
namespace harmonicControls
{
inline constexpr HarmonicControlRange subtraction{0.0, 1.0, 0.05};
inline constexpr HarmonicControlRange decay{0.0, 2.0, 0.1};
inline constexpr HarmonicControlRange components{1, harmonicMaximumComponents, 1};
inline constexpr HarmonicControlRange gain{0.1, 128.0, 0.1};
inline constexpr HarmonicControlRange rangeDb{20.0, 100.0, 5.0};
inline constexpr HarmonicControlRange wideningPercent{0.0, harmonicMaximumWideningPercent, 1.0};
inline constexpr HarmonicControlRange trailMs{0.0, 500.0, 25.0};
}

// Evidence for a proposed bass note, measured on the original short FFT.
struct BassCarrierEvidence
{
    static constexpr double fullWeightingThroughHz = 300.0, weightingEndsAtHz = 400.0;
    static constexpr int peakSearchRadiusBins = 1, shoulderDistanceBins = 2;
    static constexpr double edgeBiasLimitHz = 1.0, edgeBiasBins = 0.035;
    static constexpr double matchingRadiusBins = 0.65, matchingRadiusFundamentalFraction = 0.20;
    static constexpr double minimumContrast = 0.45, fullContrast = 0.90;
    static constexpr double hannMagnitudeAllowance = 1.25;
    static constexpr double detuningDeadband = 0.08; // Fraction of matching radius.
};
static_assert(BassCarrierEvidence::weightingEndsAtHz > BassCarrierEvidence::fullWeightingThroughHz
    && BassCarrierEvidence::fullContrast > BassCarrierEvidence::minimumContrast
    && BassCarrierEvidence::detuningDeadband >= 0.0 && BassCarrierEvidence::detuningDeadband < 1.0);

struct HarmonicCombSettings
{
    bool enabled = true;
    bool showInStrip = true;
    double subtractionFraction = 0.75;
    double harmonicDecay = 1.3; // Amplitude template proportional to 1/h^decay.
    int maximumHarmonics = harmonicMaximumHarmonics;
    bool iterativeExtraction = true;
    int maximumComponents = harmonicMaximumComponents;
    double minimumRelativeAmplitude = 0.01;
    double minimumErrorImprovement = 0.0001;
    double minimumComponentDistanceSemitones = 0.5;
    double harmonicSupportThreshold = 0.02; // Tail support refinement; zero keeps all configured teeth.
    double harmonicWideningPercent = harmonicDefaultWideningPercent; // Extra bass widening applies to the fundamental only.
    double trailHalfLifeSeconds = 0.20; // Display only; 0 disables the compact marker trail.
    double displayGain = 30.0; // Fixed amplitude gain for the marker display.
    bool logarithmicDisplay = false;
    bool highResReference = true; // Shared frequency reference for pursuit ranking and display.
    double displayRangeDb = 60.0; // Fixed visible range below 1/displayGain.
};

struct PitchMotionSettings
{
    HarmonicCombSettings harmonicComb;
    double minimumFrequencyHz = 50.0;
    double maximumFrequencyHz = 10000.0;
    int maximumPeaks = pitchMaximumPeaks;
    double minimumPeakRelativePower = 1.0e-4;
    double noiseFloorStrength = 1.0; // 0 with zero prominence: exact cleaning bypass.
    double minimumPeakProminenceDb = 6.0; // Power above the conservative local floor.
    double peakWeightExponent = 2.0; // 1: net energy; 2: emphasize dominant peaks.
    double minimumTotalPower = 1.0e-12;
    double comparisonSeconds = 0.04;
    double smoothingSeconds = 0.20; // 0: instantaneous histogram.
    double integrationSeconds = 2.0; // Leaky integration of weighted velocity.
    double integratorLimitSemitones = 3.0; // State clamp also prevents windup.
    double transportScaleSemitones = 6.0;
    double marginalPenalty = 1.0;
    double entropy = 0.001; // Kept small to avoid fictitious stationary spread.
    int maximumIterations = 64;
    double convergenceTolerance = 1.0e-5;
};
}
