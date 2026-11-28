// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "PitchMotion.h"

namespace fxanalysis::experimental
{
inline constexpr int spectralCorrelationLagCount = 3;
inline constexpr std::array<double, spectralCorrelationLagCount> spectralCorrelationRequestedLags{{0.04, 0.08, 0.12}};

struct SpectralCorrelationLagSnapshot
{
    bool hasEstimate = false;
    double comparisonSeconds = 0.0;
    // Nonnegative cosine scores, not transported masses or probabilities.
    // Positive displacement aligns a past frequency with a higher current one.
    std::array<float, pitchHistogramBinCount> correlation{};
    double displacementSemitones = 0.0;
    double peakCorrelation = 0.0;
    // Peak minus the best score outside its +/-0.5-semitone neighbourhood.
    // This is a bounded contrast diagnostic, not a confidence probability.
    double prominence = 0.0;
    bool atSearchBoundary = false;
};

struct SpectralCorrelationSnapshot
{
    PitchMotionResult status = PitchMotionResult::notPrepared;
    bool hasEstimate = false; // At least one lag is available; inspect each lag.
    double centreTimeSeconds = 0.0, stepSeconds = 0.0;
    double integratedSemitones = 0.0, combinedSemitonesPerSecond = 0.0;
    std::array<SpectralCorrelationLagSnapshot, spectralCorrelationLagCount> lags{};
};

// Single-threaded bounded numerical experiment. No IO, devices or processing
// allocations. Input is the short raw mean-channel FFT power, before bar mapping.
// Background rejection precedes conservative power redistribution to log cells.
// Features are (cellPower / maximumCellPower)^(gamma/2), then L2 normalized.
// Correlation uses the actual overlap for the product and both energy norms,
// never wraparound. Empty or negligible-energy overlaps have zero score.
// The three lag velocities are combined
// once per new audio hop; snapshot reads do not advance any state.
class SpectralCorrelation
{
public:
    PitchMotionResult prepare(const PitchMotionSettings& settings = {}) noexcept;
    void reset() noexcept;
    PitchMotionResult process(const SpectrumFrame& frame) noexcept;
    const SpectralCorrelationSnapshot& snapshot() const noexcept { return snapshot_; }

private:
    // 1..192000 Hz spans fewer than 844 quarter-semitone cells.
    static constexpr int featureCapacity = 1024;
    static constexpr int historyCapacity = 64;
    struct FeatureFrame
    {
        std::array<float, featureCapacity> values{};
        double time = 0.0;
    };
    static bool validSettings(const PitchMotionSettings& settings) noexcept;
    static PitchMotionResult validateFrame(const SpectrumFrame& frame) noexcept;
    PitchMotionResult fail(PitchMotionResult status) noexcept;
    bool extract(const SpectrumFrame& frame, FeatureFrame& feature) noexcept;
    void remember(const FeatureFrame& feature) noexcept;
    const FeatureFrame* referenceAt(double targetTime) const noexcept;
    void correlate(const FeatureFrame& previous, const FeatureFrame& current,
        SpectralCorrelationLagSnapshot& lag) const noexcept;
    void updateIntegral() noexcept;

    PitchMotionSettings settings_;
    SpectralCorrelationSnapshot snapshot_;
    bool prepared_ = false, haveFormat_ = false, haveTime_ = false;
    int featureCount_ = 0, historyBegin_ = 0, historyCount_ = 0;
    double sampleRate_ = 0.0, lastTime_ = 0.0, integrated_ = 0.0;
    int fftSize_ = 0, hopSize_ = 0, channels_ = 0;
    std::uint64_t lastFirstSample_ = 0;
    std::array<double, featureCapacity + 1> cellEdges_{};
    std::array<double, featureCapacity> cellPower_{};
    std::array<FeatureFrame, historyCapacity> history_{};
};
}
