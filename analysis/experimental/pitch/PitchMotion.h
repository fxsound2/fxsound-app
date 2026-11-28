// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "PitchExperimentSettings.h"
#include "../../AudioAnalyzer.h"
#include <array>

namespace fxanalysis::experimental
{
enum class PitchMotionResult
{
    ok, warmingUp, noSignal, notPrepared, invalidConfiguration,
    invalidFrame, nonFiniteInput, numericalFailure
};
const char* describe(PitchMotionResult result) noexcept;

struct PitchPeak
{
    double frequencyHz = 0.0;
    double power = 0.0;
};

struct PitchMotionSnapshot
{
    PitchMotionResult status = PitchMotionResult::notPrepared;
    bool hasEstimate = false;
    // Common mass scale: unit-normalized source/target net-energy^gamma weights.
    // These are masses, not unit-height or independently normalized curves.
    std::array<float, pitchHistogramBinCount> rawHistogram{}, smoothedHistogram{};
    double rawLowerTailMass = 0.0, rawUpperTailMass = 0.0;
    double smoothedLowerTailMass = 0.0, smoothedUpperTailMass = 0.0;
    double matchedMass = 0.0;
    // KL UOT has soft marginals: deficits and excesses can coexist.
    double unmatchedSourceMass = 0.0, unmatchedTargetMass = 0.0;
    double excessSourceMass = 0.0, excessTargetMass = 0.0;
    double stationaryMass = 0.0; // Absolute displacement <= half a histogram bin.
    double netDisplacementSemitones = 0.0; // Sum T_ij * displacement, including tails.
    double meanMatchedDisplacementSemitones = 0.0; // Net / matched mass.
    double modeSemitones = 0.0; // Highest in-range histogram bin; tails remain separate.
    double netSemitonesPerSecond = 0.0;
    double integratedSemitones = 0.0;
    double comparisonSeconds = 0.0, stepSeconds = 0.0;
    double centreTimeSeconds = 0.0;
    int sourcePeakCount = 0, targetPeakCount = 0;
    int solverIterations = 0;
    double solverResidual = 0.0;
    bool solverConverged = false;
};

// Pure single-threaded numerical experiment. No IO, devices, UI or allocations.
// prepare/reset/process are noexcept and return explicit boundary status.
class PitchMotion
{
public:
    PitchMotionResult prepare(const PitchMotionSettings& settings = {}) noexcept;
    void reset() noexcept;
    PitchMotionResult process(const SpectrumFrame& frame) noexcept;
    // Numerical seam: power is already-cleaned positive peak energy. Gamma is
    // still applied; no FFT-bin background/prominence operation is repeated here.
    // An empty list is ordinary no-signal. Times must be finite and increasing.
    PitchMotionResult processPeaks(const PitchPeak* peaks, int count, double timeSeconds) noexcept;
    const PitchMotionSnapshot& snapshot() const noexcept { return snapshot_; }
    const PitchMotionSettings& settings() const noexcept { return settings_; }

private:
    static constexpr int historyCapacity = 64;
    struct Measure
    {
        std::array<double, pitchMaximumPeaks> position{}, mass{};
        int count = 0;
        double time = 0.0;
    };
    static bool validSettings(const PitchMotionSettings& settings) noexcept;
    PitchMotionResult fail(PitchMotionResult result) noexcept;
    void selectPeak(std::array<PitchPeak, pitchMaximumPeaks>& selected, int& count, PitchPeak peak) const noexcept;
    PitchMotionResult normalizePeaks(const PitchPeak* peaks, int count, double time, Measure& measure) const noexcept;
    PitchMotionResult processMeasure(const Measure& measure) noexcept;
    void remember(const Measure& measure) noexcept;
    bool solve(const Measure& source, const Measure& target) noexcept;
    void addTransport(double displacement, double mass) noexcept;
    void updateMemory(double stepSeconds) noexcept;
    PitchMotionSettings settings_;
    PitchMotionSnapshot snapshot_;
    bool prepared_ = false, haveTime_ = false, haveMemory_ = false;
    double lastTime_ = 0.0, integrated_ = 0.0;
    int historyBegin_ = 0, historyCount_ = 0;
    std::array<Measure, historyCapacity> history_{};
    // Detect source format/discontinuity. The peak seam uses its own time continuity.
    bool haveFormat_ = false;
    double sampleRate_ = 0.0;
    int fftSize_ = 0, hopSize_ = 0, channels_ = 0;
    std::uint64_t lastFirstSample_ = 0;
    std::array<double, pitchMaximumPeaks * pitchMaximumPeaks> logKernel_{};
    std::array<double, pitchMaximumPeaks> logA_{}, logB_{}, logU_{}, logV_{};
};
}