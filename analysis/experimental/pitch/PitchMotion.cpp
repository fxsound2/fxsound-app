// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PitchExperimentSettings.h"
#include "PitchMotion.h"
#include "SpectralWeighting.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace fxanalysis::experimental
{
namespace
{
constexpr double referenceHz = 440.0;
constexpr double timeTolerance = 1.0e-9;

template<class Term>
double logSum(int count, Term term) noexcept
{
    double largest = -std::numeric_limits<double>::infinity();
    for (int i = 0; i < count; ++i) largest = std::max(largest, term(i));
    double sum = 0.0;
    for (int i = 0; i < count; ++i)
    {
        const double relativeLog = term(i) - largest;
        // At most 64 terms: omitted relative sum <= 64*exp(-40) < 2.8e-16.
        // The maximum is local to this row/column, so weak but relevant paths
        // remain represented without evaluating exp() on distant FFT peaks.
        if (relativeLog >= -40.0) sum += std::exp(relativeLog);
    }
    return largest + std::log(sum);
}

bool inRange(double value, double low, double high) noexcept
{
    return std::isfinite(value) && value >= low && value <= high;
}
}

const char* describe(PitchMotionResult result) noexcept
{
    switch (result)
    {
    case PitchMotionResult::ok: return "Pitch motion estimate available";
    case PitchMotionResult::warmingUp: return "Waiting for comparison history";
    case PitchMotionResult::noSignal: return "No retained spectral energy";
    case PitchMotionResult::notPrepared: return "Pitch motion is not prepared";
    case PitchMotionResult::invalidConfiguration: return "Invalid pitch motion settings";
    case PitchMotionResult::invalidFrame: return "Invalid pitch spectrum or time";
    case PitchMotionResult::nonFiniteInput: return "Pitch input contains a non-finite value";
    case PitchMotionResult::numericalFailure: return "Pitch transport calculation failed";
    }
    return "Unknown pitch motion status";
}

bool PitchMotion::validSettings(const PitchMotionSettings& s) noexcept
{
    return validSpectralWeightingSettings(s) && inRange(s.minimumFrequencyHz, 1.0, 192000.0)
        && inRange(s.maximumFrequencyHz, s.minimumFrequencyHz, 192000.0)
        && s.maximumFrequencyHz > s.minimumFrequencyHz
        && s.maximumPeaks >= 1 && s.maximumPeaks <= pitchMaximumPeaks
        && inRange(s.minimumPeakRelativePower, 0.0, 1.0)
        && inRange(s.minimumTotalPower, 0.0, 1.0)
        && inRange(s.comparisonSeconds, 0.001, 1.0)
        && inRange(s.smoothingSeconds, 0.0, 10.0)
        && inRange(s.integrationSeconds, 0.01, 30.0)
        && inRange(s.integratorLimitSemitones, 0.01, 48.0)
        && inRange(s.transportScaleSemitones, 0.1, 48.0)
        && inRange(s.marginalPenalty, 0.01, 100.0)
        && inRange(s.entropy, 1.0e-5, 0.1)
        && s.maximumIterations >= 1 && s.maximumIterations <= 512
        && inRange(s.convergenceTolerance, 1.0e-10, 0.1);
}

PitchMotionResult PitchMotion::prepare(const PitchMotionSettings& settings) noexcept
{
    if (!validSettings(settings))
    {
        prepared_ = false;
        return fail(PitchMotionResult::invalidConfiguration);
    }
    settings_ = settings;
    prepared_ = true;
    reset();
    return PitchMotionResult::ok;
}

void PitchMotion::reset() noexcept
{
    snapshot_ = {};
    snapshot_.status = prepared_ ? PitchMotionResult::warmingUp : PitchMotionResult::notPrepared;
    haveTime_ = false;
    haveMemory_ = false;
    lastTime_ = integrated_ = 0.0;
    historyBegin_ = historyCount_ = 0;
    haveFormat_ = false;
    sampleRate_ = 0.0;
    fftSize_ = hopSize_ = channels_ = 0;
    lastFirstSample_ = 0;
}

PitchMotionResult PitchMotion::fail(PitchMotionResult result) noexcept
{
    reset();
    snapshot_.status = result;
    return result;
}

void PitchMotion::selectPeak(std::array<PitchPeak, pitchMaximumPeaks>& selected,
    int& count, PitchPeak peak) const noexcept
{
    if (count < settings_.maximumPeaks)
    {
        selected[count++] = peak;
        return;
    }
    int least = 0;
    for (int i = 1; i < count; ++i)
        if (selected[i].power < selected[least].power) least = i;
    if (peak.power > selected[least].power) selected[least] = peak;
}

PitchMotionResult PitchMotion::normalizePeaks(const PitchPeak* peaks, int count,
    double time, Measure& measure) const noexcept
{
    if (!std::isfinite(time)) return PitchMotionResult::nonFiniteInput;
    if (time < 0.0 || count < 0 || count > AnalysisLimits::maximumFftSize
        || (count > 0 && peaks == nullptr)) return PitchMotionResult::invalidFrame;
    std::array<PitchPeak, pitchMaximumPeaks> selected{};
    int selectedCount = 0;
    for (int i = 0; i < count; ++i)
    {
        const auto peak = peaks[i];
        if (!std::isfinite(peak.frequencyHz) || !std::isfinite(peak.power))
            return PitchMotionResult::nonFiniteInput;
        if (peak.power < 0.0 || (peak.power > 0.0 && peak.frequencyHz <= 0.0))
            return PitchMotionResult::invalidFrame;
        if (peak.power > 0.0 && peak.frequencyHz >= settings_.minimumFrequencyHz
            && peak.frequencyHz <= settings_.maximumFrequencyHz)
            selectPeak(selected, selectedCount, peak);
    }
    if (selectedCount == 0) return PitchMotionResult::noSignal;
    double largest = 0.0;
    for (int i = 0; i < selectedCount; ++i) largest = std::max(largest, selected[i].power);
    // Scale before summing: finite huge powers and quiet gains must not overflow.
    double energySum = 0.0, weightSum = 0.0;
    for (int i = 0; i < selectedCount; ++i)
    {
        const double relative = selected[i].power / largest;
        if (!(relative > 0.0) || relative < settings_.minimumPeakRelativePower) continue;
        selected[measure.count++] = selected[i];
        energySum += relative;
        weightSum += settings_.peakWeightExponent == 1.0 ? relative
            : std::pow(relative, settings_.peakWeightExponent);
    }
    // The silence gate remains in net-energy units, independently of gamma.
    if (measure.count == 0 || largest < settings_.minimumTotalPower / energySum)
        return PitchMotionResult::noSignal;
    std::sort(selected.begin(), selected.begin() + measure.count,
        [](const PitchPeak& a, const PitchPeak& b) { return a.frequencyHz < b.frequencyHz; });
    for (int i = 0; i < measure.count; ++i)
    {
        measure.position[i] = 12.0 * std::log2(selected[i].frequencyHz / referenceHz);
        const double relative = selected[i].power / largest;
        measure.mass[i] = (settings_.peakWeightExponent == 1.0 ? relative
            : std::pow(relative, settings_.peakWeightExponent)) / weightSum;
    }
    measure.time = time;
    return PitchMotionResult::ok;
}

PitchMotionResult PitchMotion::processPeaks(const PitchPeak* peaks, int count,
    double timeSeconds) noexcept
{
    if (!prepared_) return fail(PitchMotionResult::notPrepared);
    Measure measure;
    const auto result = normalizePeaks(peaks, count, timeSeconds, measure);
    if (result != PitchMotionResult::ok) return fail(result);
    return processMeasure(measure);
}

PitchMotionResult PitchMotion::process(const SpectrumFrame& frame) noexcept
{
    if (!prepared_) return fail(PitchMotionResult::notPrepared);
    if (!std::isfinite(frame.sampleRate)) return fail(PitchMotionResult::nonFiniteInput);
    if (!inRange(frame.sampleRate, AnalysisLimits::minimumSampleRate, AnalysisLimits::maximumSampleRate)
        || frame.fftSize < AnalysisLimits::minimumFftSize || frame.fftSize > AnalysisLimits::maximumFftSize
        || (frame.fftSize & (frame.fftSize - 1)) != 0
        || frame.hopSize < 1 || frame.hopSize > frame.fftSize
        || (frame.channels != 1 && frame.channels != 2)
        || frame.power.size() != static_cast<std::size_t>(frame.fftSize / 2 + 1))
        return fail(PitchMotionResult::invalidFrame);
    for (float power : frame.power)
    {
        if (!std::isfinite(power)) return fail(PitchMotionResult::nonFiniteInput);
        if (power < 0.0f) return fail(PitchMotionResult::invalidFrame);
    }
    if (haveFormat_ && (sampleRate_ != frame.sampleRate || fftSize_ != frame.fftSize
        || hopSize_ != frame.hopSize || channels_ != frame.channels
        || frame.firstSample < lastFirstSample_
        || frame.firstSample - lastFirstSample_ != static_cast<std::uint64_t>(frame.hopSize)))
        reset();
    haveFormat_ = true;
    sampleRate_ = frame.sampleRate;
    fftSize_ = frame.fftSize;
    hopSize_ = frame.hopSize;
    channels_ = frame.channels;
    lastFirstSample_ = frame.firstSample;

    const double binHz = frame.sampleRate / frame.fftSize;
    const int low = std::max(1, static_cast<int>(std::floor(settings_.minimumFrequencyHz / binHz)));
    const int high = std::min(frame.fftSize / 2 - 1,
        static_cast<int>(std::ceil(settings_.maximumFrequencyHz / binHz)));
    std::array<PitchPeak, pitchMaximumPeaks> peaks{};
    int count = 0;
    double largest = 0.0;
    for (int k = low; k <= high; ++k) largest = std::max(largest, static_cast<double>(frame.power[k]));
    if (!(largest > 0.0)) return fail(PitchMotionResult::noSignal);
    for (int k = low; k <= high; ++k)
    {
        const double left = frame.power[k - 1], centre = frame.power[k], right = frame.power[k + 1];
        if (!(centre > left && centre >= right)
            || centre / largest < settings_.minimumPeakRelativePower) continue;
        const double netCentre = cleanSpectralPower(frame, k, settings_);
        if (!(netCentre > 0.0)) continue;
        const double floor = centre * 1.0e-12;
        const double l = std::log(std::max(left, floor));
        const double c = std::log(centre);
        const double r = std::log(std::max(right, floor));
        const double curvature = l - 2.0 * c + r;
        const double offset = curvature < -1.0e-12
            ? std::clamp(0.5 * (l - r) / curvature, -0.5, 0.5) : 0.0;
        const double frequency = (k + offset) * binHz;
        if (frequency < settings_.minimumFrequencyHz || frequency > settings_.maximumFrequencyHz) continue;
        // Hann-lobe energy is less sensitive to sub-bin scalloping than peak height.
        // Maxima two bins apart share one valley bin; each owns half of it.
        // Partition before the dominant-peak selection so that retained close
        // peaks cannot count the same valley energy twice.
        const bool sharedLeft = k - 2 >= low
            && frame.power[k - 2] > frame.power[k - 3] && frame.power[k - 2] >= frame.power[k - 1];
        const bool sharedRight = k + 2 <= high
            && frame.power[k + 2] > frame.power[k + 1] && frame.power[k + 2] >= frame.power[k + 3];
        const double lobePower = netCentre
            + cleanSpectralPower(frame, k - 1, settings_) * (sharedLeft ? 0.5 : 1.0)
            + cleanSpectralPower(frame, k + 1, settings_) * (sharedRight ? 0.5 : 1.0);
        selectPeak(peaks, count, {frequency, lobePower});
    }
    Measure measure;
    const double time = (static_cast<double>(frame.firstSample) + frame.fftSize * 0.5) / frame.sampleRate;
    const auto result = normalizePeaks(peaks.data(), count, time, measure);
    if (result != PitchMotionResult::ok) return fail(result);
    return processMeasure(measure);
}

void PitchMotion::remember(const Measure& measure) noexcept
{
    // Decimate only stored history at extremely short hops. Its 64 fixed slots
    // still cover the requested lag; processing/output remain at every input hop.
    if (historyCount_ > 0)
    {
        const int last = (historyBegin_ + historyCount_ - 1) % historyCapacity;
        if (measure.time - history_[last].time < settings_.comparisonSeconds / (historyCapacity - 2)) return;
    }
    if (historyCount_ == historyCapacity)
    {
        historyBegin_ = (historyBegin_ + 1) % historyCapacity;
        --historyCount_;
    }
    history_[(historyBegin_ + historyCount_) % historyCapacity] = measure;
    ++historyCount_;
}

PitchMotionResult PitchMotion::processMeasure(const Measure& measure) noexcept
{
    if (haveTime_ && measure.time <= lastTime_) return fail(PitchMotionResult::invalidFrame);
    if (haveTime_ && measure.time - lastTime_ > std::max(0.25, 4.0 * settings_.comparisonSeconds)) reset();
    const double step = haveTime_ ? measure.time - lastTime_ : 0.0;
    haveTime_ = true;
    lastTime_ = measure.time;
    PitchMotionSnapshot next;
    next.smoothedHistogram = snapshot_.smoothedHistogram;
    next.smoothedLowerTailMass = snapshot_.smoothedLowerTailMass;
    next.smoothedUpperTailMass = snapshot_.smoothedUpperTailMass;
    next.integratedSemitones = integrated_;
    next.centreTimeSeconds = measure.time;
    next.stepSeconds = step;
    next.targetPeakCount = measure.count;
    next.status = PitchMotionResult::warmingUp;
    snapshot_ = next;
    const Measure* reference = nullptr;
    const double targetTime = measure.time - settings_.comparisonSeconds;
    for (int i = 0; i < historyCount_; ++i)
    {
        const auto& old = history_[(historyBegin_ + i) % historyCapacity];
        if (old.time <= targetTime + timeTolerance && (reference == nullptr || old.time > reference->time))
            reference = &old;
    }
    if (reference == nullptr)
    {
        remember(measure);
        return snapshot_.status;
    }
    snapshot_.comparisonSeconds = measure.time - reference->time;
    snapshot_.sourcePeakCount = reference->count;
    if (!(snapshot_.comparisonSeconds > 0.0) || !solve(*reference, measure))
        return fail(PitchMotionResult::numericalFailure);
    snapshot_.netSemitonesPerSecond = snapshot_.netDisplacementSemitones / snapshot_.comparisonSeconds;
    updateMemory(step);
    snapshot_.hasEstimate = true;
    snapshot_.status = PitchMotionResult::ok;
    remember(measure);
    return snapshot_.status;
}

void PitchMotion::addTransport(double displacement, double mass) noexcept
{
    snapshot_.matchedMass += mass;
    snapshot_.netDisplacementSemitones += mass * displacement;
    if (std::abs(displacement) <= pitchHistogramStepSemitones * 0.5)
        snapshot_.stationaryMass += mass;
    const double coordinate = (displacement - pitchHistogramMinimumSemitones) / pitchHistogramStepSemitones;
    if (coordinate < 0.0) { snapshot_.rawLowerTailMass += mass; return; }
    if (coordinate > pitchHistogramBinCount - 1) { snapshot_.rawUpperTailMass += mass; return; }
    const int lower = static_cast<int>(std::floor(coordinate));
    const double fraction = coordinate - lower;
    snapshot_.rawHistogram[lower] += static_cast<float>(mass * (1.0 - fraction));
    if (lower + 1 < pitchHistogramBinCount)
        snapshot_.rawHistogram[lower + 1] += static_cast<float>(mass * fraction);
}

bool PitchMotion::solve(const Measure& source, const Measure& target) noexcept
{
    const int rows = source.count, columns = target.count;
    bool identical = rows == columns;
    for (int i = 0; identical && i < rows; ++i)
        identical = std::abs(source.position[i] - target.position[i]) <= 1.0e-10
            && std::abs(source.mass[i] - target.mass[i]) <= 1.0e-10;
    if (identical)
    {
        // A known unchanged normalized measure has an exact zero-motion identity
        // plan. Do not invent symmetric motion from entropic regularization.
        for (int i = 0; i < rows; ++i) addTransport(0.0, source.mass[i]);
        snapshot_.solverConverged = true;
    }
    else
    {
        for (int i = 0; i < rows; ++i) { logA_[i] = std::log(source.mass[i]); logU_[i] = 0.0; }
        for (int j = 0; j < columns; ++j) { logB_[j] = std::log(target.mass[j]); logV_[j] = 0.0; }
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < columns; ++j)
            {
                const double scaled = (target.position[j] - source.position[i]) / settings_.transportScaleSemitones;
                logKernel_[i * pitchMaximumPeaks + j] = logA_[i] + logB_[j]
                    - scaled * scaled / settings_.entropy;
            }
        // Initialize separated components from their exact one-pair solution.
        // This is only a numerical starting point: the full coupling remains free.
        // Without it, tiny entropy makes different-cost disconnected components
        // converge very slowly even after global dual centering.
        std::array<int, pitchMaximumPeaks> nearestColumn{}, nearestRow{};
        for (int i = 0; i < rows; ++i)
        {
            double best = std::numeric_limits<double>::infinity();
            for (int j = 0; j < columns; ++j)
            {
                const double distance = std::abs(target.position[j] - source.position[i]);
                if (distance < best) { best = distance; nearestColumn[i] = j; }
            }
        }
        for (int j = 0; j < columns; ++j)
        {
            double best = std::numeric_limits<double>::infinity();
            for (int i = 0; i < rows; ++i)
            {
                const double distance = std::abs(target.position[j] - source.position[i]);
                if (distance < best) { best = distance; nearestRow[j] = i; }
            }
        }
        for (int i = 0; i < rows; ++i)
        {
            const int j = nearestColumn[i];
            if (nearestRow[j] != i) continue;
            const double scaled = (target.position[j] - source.position[i]) / settings_.transportScaleSemitones;
            const double logMass = ((settings_.marginalPenalty + settings_.entropy) * (logA_[i] + logB_[j])
                - scaled * scaled) / (2.0 * settings_.marginalPenalty + settings_.entropy);
            logU_[i] = -settings_.marginalPenalty / settings_.entropy * (logMass - logA_[i]);
            logV_[j] = -settings_.marginalPenalty / settings_.entropy * (logMass - logB_[j]);
        }
        const double theta = settings_.marginalPenalty / (settings_.marginalPenalty + settings_.entropy);
        for (int iteration = 0; iteration < settings_.maximumIterations; ++iteration)
        {
            const auto previousU = logU_, previousV = logV_;
            for (int i = 0; i < rows; ++i)
                logU_[i] = theta * (logA_[i] - logSum(columns, [&](int j)
                    { return logKernel_[i * pitchMaximumPeaks + j] + logV_[j]; }));
            for (int j = 0; j < columns; ++j)
                logV_[j] = theta * (logB_[j] - logSum(rows, [&](int i)
                    { return logKernel_[i * pitchMaximumPeaks + j] + logU_[i]; }));
            // Exact dual line optimization along (f+s,g-s). The coupling is
            // unchanged by this centering; low-entropy global mass converges faster.
            const double ratio = settings_.entropy / settings_.marginalPenalty;
            const double a = logSum(rows, [&](int i) { return logA_[i] - ratio * logU_[i]; });
            const double b = logSum(columns, [&](int j) { return logB_[j] - ratio * logV_[j]; });
            const double shift = (a - b) / (2.0 * ratio);
            double residual = 0.0;
            for (int i = 0; i < rows; ++i)
            {
                logU_[i] += shift;
                residual = std::max(residual, settings_.entropy * std::abs(logU_[i] - previousU[i]));
            }
            for (int j = 0; j < columns; ++j)
            {
                logV_[j] -= shift;
                residual = std::max(residual, settings_.entropy * std::abs(logV_[j] - previousV[j]));
            }
            if (!std::isfinite(residual)) return false;
            snapshot_.solverIterations = iteration + 1;
            snapshot_.solverResidual = residual;
            if (residual <= settings_.convergenceTolerance)
            {
                snapshot_.solverConverged = true;
                break;
            }
        }
        std::array<double, pitchMaximumPeaks> rowMass{}, columnMass{};
        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < columns; ++j)
            {
                const double mass = std::exp(logKernel_[i * pitchMaximumPeaks + j] + logU_[i] + logV_[j]);
                if (!std::isfinite(mass)) return false;
                rowMass[i] += mass;
                columnMass[j] += mass;
                addTransport(target.position[j] - source.position[i], mass);
            }
        for (int i = 0; i < rows; ++i)
        {
            snapshot_.unmatchedSourceMass += std::max(0.0, source.mass[i] - rowMass[i]);
            snapshot_.excessSourceMass += std::max(0.0, rowMass[i] - source.mass[i]);
        }
        for (int j = 0; j < columns; ++j)
        {
            snapshot_.unmatchedTargetMass += std::max(0.0, target.mass[j] - columnMass[j]);
            snapshot_.excessTargetMass += std::max(0.0, columnMass[j] - target.mass[j]);
        }
    }
    if (!std::isfinite(snapshot_.matchedMass) || !std::isfinite(snapshot_.netDisplacementSemitones)) return false;
    snapshot_.meanMatchedDisplacementSemitones = snapshot_.matchedMass > 0.0
        ? snapshot_.netDisplacementSemitones / snapshot_.matchedMass : 0.0;
    int mode = pitchHistogramBinCount / 2;
    for (int i = 0; i < pitchHistogramBinCount; ++i)
        if (snapshot_.rawHistogram[i] > snapshot_.rawHistogram[mode]) mode = i;
    snapshot_.modeSemitones = pitchHistogramMinimumSemitones + mode * pitchHistogramStepSemitones;
    return true;
}

void PitchMotion::updateMemory(double stepSeconds) noexcept
{
    const double blend = settings_.smoothingSeconds > 0.0
        ? -std::expm1(-stepSeconds / settings_.smoothingSeconds) : 1.0;
    if (!haveMemory_)
    {
        snapshot_.smoothedHistogram = snapshot_.rawHistogram;
        snapshot_.smoothedLowerTailMass = snapshot_.rawLowerTailMass;
        snapshot_.smoothedUpperTailMass = snapshot_.rawUpperTailMass;
        haveMemory_ = true;
    }
    else
    {
        for (int i = 0; i < pitchHistogramBinCount; ++i)
            snapshot_.smoothedHistogram[i] += static_cast<float>(blend
                * (snapshot_.rawHistogram[i] - snapshot_.smoothedHistogram[i]));
        snapshot_.smoothedLowerTailMass += blend * (snapshot_.rawLowerTailMass - snapshot_.smoothedLowerTailMass);
        snapshot_.smoothedUpperTailMass += blend * (snapshot_.rawUpperTailMass - snapshot_.smoothedUpperTailMass);
    }
    const double decay = std::exp(-stepSeconds / settings_.integrationSeconds);
    // Lag estimates overlap. Integrate velocity over this new hop exactly once,
    // not the full comparison displacement on every publication.
    integrated_ = std::clamp(decay * integrated_ + settings_.integrationSeconds * (1.0 - decay)
        * snapshot_.netSemitonesPerSecond, -settings_.integratorLimitSemitones, settings_.integratorLimitSemitones);
    snapshot_.integratedSemitones = integrated_;
}
}
