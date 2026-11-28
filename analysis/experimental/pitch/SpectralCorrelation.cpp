// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PitchExperimentSettings.h"
#include "SpectralCorrelation.h"
#include "SpectralWeighting.h"
#include <algorithm>
#include <cmath>

namespace fxanalysis::experimental
{
namespace
{
constexpr double timeTolerance = 1.0e-9;
constexpr int zeroShiftBin = pitchHistogramBinCount / 2;
constexpr int peakNeighbourhoodBins = 2;
bool inRange(double value, double low, double high) noexcept
{
    return std::isfinite(value) && value >= low && value <= high;
}
}

bool SpectralCorrelation::validSettings(const PitchMotionSettings& settings) noexcept
{
    return inRange(settings.minimumFrequencyHz, 1.0, 192000.0)
        && inRange(settings.maximumFrequencyHz, settings.minimumFrequencyHz, 192000.0)
        && settings.maximumFrequencyHz > settings.minimumFrequencyHz
        && inRange(settings.minimumTotalPower, 0.0, 1.0)
        && validSpectralWeightingSettings(settings)
        && inRange(settings.integrationSeconds, 0.01, 30.0)
        && inRange(settings.integratorLimitSemitones, 0.01, 48.0);
}

PitchMotionResult SpectralCorrelation::prepare(const PitchMotionSettings& settings) noexcept
{
    if (!validSettings(settings))
    {
        prepared_ = false;
        return fail(PitchMotionResult::invalidConfiguration);
    }
    const double cells = std::ceil(12.0 * std::log2(settings.maximumFrequencyHz
        / settings.minimumFrequencyHz) / pitchHistogramStepSemitones);
    if (!(cells >= 1.0 && cells <= featureCapacity))
    {
        prepared_ = false;
        return fail(PitchMotionResult::invalidConfiguration);
    }
    settings_ = settings;
    featureCount_ = static_cast<int>(cells);
    for (int cell = 0; cell <= featureCount_; ++cell)
        cellEdges_[cell] = settings.minimumFrequencyHz
            * std::exp2(cell * pitchHistogramStepSemitones / 12.0);
    cellEdges_[featureCount_] = settings.maximumFrequencyHz;
    prepared_ = true;
    reset();
    return PitchMotionResult::ok;
}

void SpectralCorrelation::reset() noexcept
{
    snapshot_ = {};
    snapshot_.status = prepared_ ? PitchMotionResult::warmingUp : PitchMotionResult::notPrepared;
    haveFormat_ = haveTime_ = false;
    historyBegin_ = historyCount_ = 0;
    sampleRate_ = lastTime_ = integrated_ = 0.0;
    fftSize_ = hopSize_ = channels_ = 0;
    lastFirstSample_ = 0;
}

PitchMotionResult SpectralCorrelation::fail(PitchMotionResult status) noexcept
{
    reset();
    snapshot_.status = status;
    return status;
}

PitchMotionResult SpectralCorrelation::validateFrame(const SpectrumFrame& frame) noexcept
{
    if (!std::isfinite(frame.sampleRate)) return PitchMotionResult::nonFiniteInput;
    if (!inRange(frame.sampleRate, AnalysisLimits::minimumSampleRate, AnalysisLimits::maximumSampleRate)
        || frame.fftSize < AnalysisLimits::minimumFftSize || frame.fftSize > AnalysisLimits::maximumFftSize
        || (frame.fftSize & (frame.fftSize - 1)) != 0
        || frame.hopSize < 1 || frame.hopSize > frame.fftSize
        || (frame.channels != 1 && frame.channels != 2)
        || frame.power.size() != static_cast<std::size_t>(frame.fftSize / 2 + 1))
        return PitchMotionResult::invalidFrame;
    for (float power : frame.power)
    {
        if (!std::isfinite(power)) return PitchMotionResult::nonFiniteInput;
        if (power < 0.0f) return PitchMotionResult::invalidFrame;
    }
    return PitchMotionResult::ok;
}

bool SpectralCorrelation::extract(const SpectrumFrame& frame, FeatureFrame& feature) noexcept
{
    cellPower_.fill(0.0);
    const double binHz = frame.sampleRate / frame.fftSize;
    int firstCell = 0;
    // Each eligible FFT bin owns one Hz interval of width binHz. Distribute its
    // cleaned power by overlap length, so log sampling cannot introduce a 1/f
    // weighting. DC and Nyquist are not used as musical pitch evidence.
    for (int bin = 1; bin < frame.fftSize / 2; ++bin)
    {
        const double left = (bin - 0.5) * binHz;
        const double right = (bin + 0.5) * binHz;
        if (right <= settings_.minimumFrequencyHz) continue;
        if (left >= settings_.maximumFrequencyHz) break;
        const double power = cleanSpectralPower(frame, bin, settings_);
        if (!(power > 0.0)) continue;
        while (firstCell + 1 < featureCount_ && cellEdges_[firstCell + 1] <= left)
            ++firstCell;
        for (int cell = firstCell; cell < featureCount_ && cellEdges_[cell] < right; ++cell)
        {
            const double overlap = std::min(right, cellEdges_[cell + 1])
                - std::max(left, cellEdges_[cell]);
            if (overlap > 0.0) cellPower_[cell] += power * (overlap / binHz);
        }
    }
    double total = 0.0, largest = 0.0;
    for (int cell = 0; cell < featureCount_; ++cell)
    {
        total += cellPower_[cell];
        largest = std::max(largest, cellPower_[cell]);
    }
    if (!(largest > 0.0) || total < settings_.minimumTotalPower) return false;
    double squaredNorm = 0.0;
    const double exponent = 0.5 * settings_.peakWeightExponent;
    for (int cell = 0; cell < featureCount_; ++cell)
    {
        // Common scaling before pow avoids overflow/underflow under gain changes.
        const double value = std::pow(cellPower_[cell] / largest, exponent);
        cellPower_[cell] = value;
        squaredNorm += value * value;
    }
    if (!(squaredNorm > 0.0) || !std::isfinite(squaredNorm)) return false;
    const double inverseNorm = 1.0 / std::sqrt(squaredNorm);
    for (int cell = 0; cell < featureCount_; ++cell)
        feature.values[cell] = static_cast<float>(cellPower_[cell] * inverseNorm);
    feature.time = (static_cast<double>(frame.firstSample) + frame.fftSize * 0.5) / frame.sampleRate;
    return std::isfinite(feature.time);
}

const SpectralCorrelation::FeatureFrame* SpectralCorrelation::referenceAt(double targetTime) const noexcept
{
    const FeatureFrame* reference = nullptr;
    for (int index = 0; index < historyCount_; ++index)
    {
        const auto& candidate = history_[(historyBegin_ + index) % historyCapacity];
        if (candidate.time <= targetTime + timeTolerance
            && (reference == nullptr || candidate.time > reference->time)) reference = &candidate;
    }
    return reference;
}

void SpectralCorrelation::remember(const FeatureFrame& feature) noexcept
{
    // Only storage is decimated at exceptionally small hops. Leave enough
    // coverage for all three lags while still processing each incoming frame.
    if (historyCount_ > 0)
    {
        const auto& last = history_[(historyBegin_ + historyCount_ - 1) % historyCapacity];
        if (feature.time - last.time < spectralCorrelationRequestedLags.back() / (historyCapacity - 2)) return;
    }
    if (historyCount_ == historyCapacity)
    {
        historyBegin_ = (historyBegin_ + 1) % historyCapacity;
        --historyCount_;
    }
    history_[(historyBegin_ + historyCount_) % historyCapacity] = feature;
    ++historyCount_;
}

void SpectralCorrelation::correlate(const FeatureFrame& previous, const FeatureFrame& current,
    SpectralCorrelationLagSnapshot& lag) const noexcept
{
    lag.hasEstimate = true;
    lag.comparisonSeconds = current.time - previous.time;
    int peak = zeroShiftBin;
    for (int index = 0; index < pitchHistogramBinCount; ++index)
    {
        const int shift = index - zeroShiftBin;
        double dot = 0.0, previousEnergy = 0.0, currentEnergy = 0.0;
        const int begin = std::max(0, -shift);
        const int end = std::min(featureCount_, featureCount_ - shift);
        for (int cell = begin; cell < end; ++cell)
        {
            const double a = previous.values[cell], b = current.values[cell + shift];
            dot += a * b;
            previousEnergy += a * a;
            currentEnergy += b * b;
        }
        // Use the same actual overlap for the product and both RMS norms.
        // The common sample count cancels in their ratio. Uniform spectra
        // therefore have a flat score instead of the overlap triangle.
        // Features have unit full-vector energy: reject negligible tail-only
        // overlaps rather than divide by roundoff or an empty interval.
        const double score = previousEnergy > 1.0e-12 && currentEnergy > 1.0e-12
            ? dot / std::sqrt(previousEnergy * currentEnergy) : 0.0;
        lag.correlation[index] = static_cast<float>(std::clamp(score, 0.0, 1.0));
        if (lag.correlation[index] > lag.correlation[peak]
            || (lag.correlation[index] == lag.correlation[peak]
                && std::abs(index - zeroShiftBin) < std::abs(peak - zeroShiftBin))) peak = index;
    }
    lag.peakCorrelation = lag.correlation[peak];
    lag.atSearchBoundary = peak == 0 || peak == pitchHistogramBinCount - 1;
    double offset = 0.0;
    if (!lag.atSearchBoundary)
    {
        const double left = lag.correlation[peak - 1], centre = lag.correlation[peak], right = lag.correlation[peak + 1];
        const double curvature = left - 2.0 * centre + right;
        if (curvature < -1.0e-12)
            offset = std::clamp(0.5 * (left - right) / curvature, -0.5, 0.5);
    }
    lag.displacementSemitones = pitchHistogramMinimumSemitones
        + (peak + offset) * pitchHistogramStepSemitones;
    double competitor = 0.0;
    for (int index = 0; index < pitchHistogramBinCount; ++index)
        if (std::abs(index - peak) > peakNeighbourhoodBins)
            competitor = std::max(competitor, static_cast<double>(lag.correlation[index]));
    lag.prominence = std::clamp(lag.peakCorrelation - competitor, 0.0, 1.0);
}

void SpectralCorrelation::updateIntegral() noexcept
{
    double weightedVelocity = 0.0, totalWeight = 0.0;
    for (const auto& lag : snapshot_.lags)
    {
        if (!lag.hasEstimate || lag.atSearchBoundary || !(lag.comparisonSeconds > 0.0)) continue;
        const double weight = std::clamp(lag.prominence, 0.0, 1.0);
        weightedVelocity += weight * lag.displacementSemitones / lag.comparisonSeconds;
        totalWeight += weight;
    }
    snapshot_.combinedSemitonesPerSecond = totalWeight > 1.0e-8 ? weightedVelocity / totalWeight : 0.0;
    const double blend = -std::expm1(-snapshot_.stepSeconds / settings_.integrationSeconds);
    integrated_ = std::clamp((1.0 - blend) * integrated_
        + settings_.integrationSeconds * blend * snapshot_.combinedSemitonesPerSecond,
        -settings_.integratorLimitSemitones, settings_.integratorLimitSemitones);
    snapshot_.integratedSemitones = integrated_;
}

PitchMotionResult SpectralCorrelation::process(const SpectrumFrame& frame) noexcept
{
    if (!prepared_) return fail(PitchMotionResult::notPrepared);
    const auto validation = validateFrame(frame);
    if (validation != PitchMotionResult::ok) return fail(validation);
    if (haveFormat_ && (sampleRate_ != frame.sampleRate || fftSize_ != frame.fftSize
        || hopSize_ != frame.hopSize || channels_ != frame.channels
        || frame.firstSample < lastFirstSample_
        || frame.firstSample - lastFirstSample_ != static_cast<std::uint64_t>(frame.hopSize))) reset();
    haveFormat_ = true;
    sampleRate_ = frame.sampleRate;
    fftSize_ = frame.fftSize;
    hopSize_ = frame.hopSize;
    channels_ = frame.channels;
    lastFirstSample_ = frame.firstSample;
    FeatureFrame current;
    if (!extract(frame, current)) return fail(PitchMotionResult::noSignal);
    if (haveTime_ && current.time <= lastTime_) return fail(PitchMotionResult::invalidFrame);
    const double step = haveTime_ ? current.time - lastTime_ : 0.0;
    haveTime_ = true;
    lastTime_ = current.time;
    snapshot_ = {};
    snapshot_.centreTimeSeconds = current.time;
    snapshot_.stepSeconds = step;
    snapshot_.integratedSemitones = integrated_;
    for (int index = 0; index < spectralCorrelationLagCount; ++index)
    {
        const auto* reference = referenceAt(current.time - spectralCorrelationRequestedLags[index]);
        if (reference == nullptr) continue;
        correlate(*reference, current, snapshot_.lags[index]);
        snapshot_.hasEstimate = true;
    }
    snapshot_.status = snapshot_.hasEstimate ? PitchMotionResult::ok : PitchMotionResult::warmingUp;
    if (snapshot_.hasEstimate) updateIntegral();
    remember(current);
    return snapshot_.status;
}
}
