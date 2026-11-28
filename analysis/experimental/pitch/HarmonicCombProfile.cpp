// SPDX-License-Identifier: AGPL-3.0-or-later
#include "HarmonicCombProfile.h"
#include "SpectralWeighting.h"
#include "../../DisplayTiming.h"
#include "../../FrequencyScale.h"
#include <algorithm>
#include <cmath>
#include <complex>

namespace fxanalysis::experimental
{
namespace
{
constexpr double duplicateTemplateCosine = 0.999;
constexpr double duplicateTemplateCosineSquared = duplicateTemplateCosine * duplicateTemplateCosine;
constexpr double negligibleLobeMagnitude = 1e-12;
constexpr double minimumKktScale = 1e-30;
constexpr double minimumCholeskyPivot = 1e-10;
constexpr double negligibleRelativeAmplitude = 0.001;
constexpr int frequencyRefinementRounds = 3;
constexpr int legacyFitSweeps = 4;
constexpr double pi = 3.14159265358979323846;
std::complex<double> dirichlet(double x, int n) noexcept
{
    constexpr double nearZeroOffsetBins = 1e-12;
    if (std::abs(x) < nearZeroOffsetBins) return {1.0, 0.0};
    const double ratio = std::sin(pi * x) / (n * std::sin(pi * x / n));
    const double phase = -pi * x * (n - 1) / n;
    return {ratio * std::cos(phase), ratio * std::sin(phase)};
}

// Sub-bin centre of a resolved Hann peak. Sparse spectra can have a zero neighbour.
double interpolatedPeakHz(int bin, double binHz, double left, double peak, double right) noexcept
{
    double leftDrop = peak - left, rightDrop = peak - right;
    if (left > 0.0 && right > 0.0)
    {
        leftDrop = std::log(peak / left);
        rightDrop = std::log(peak / right);
    }
    return (bin + 0.5 * (leftDrop - rightDrop) / (leftDrop + rightDrop)) * binHz;
}

// A resolved tooth must turn over on both sides in the original short FFT.
// Looking below the carrier limit distinguishes a real 100 Hz line from the
// descending wing of a louder out-of-band note, before cleaning can cut it off.
double resolvedToothSupport(const SpectrumFrame& frame, double fundamental, int order, double expectedMagnitude) noexcept
{
    const double binHz = frame.sampleRate / frame.fftSize;
    using Evidence = BassCarrierEvidence;
    const double expectedHz = order * fundamental;
    const int nearest = static_cast<int>(std::lround(expectedHz / binHz));
    const int last = frame.fftSize / 2 - 1;
    constexpr int firstPositiveBin = 1;
    const int firstPeak = std::max(firstPositiveBin + 1, nearest - Evidence::peakSearchRadiusBins);
    const int lastPeak = std::min(last - 1, nearest + Evidence::peakSearchRadiusBins);
    const double edgeBiasHz = std::min(Evidence::edgeBiasLimitHz, Evidence::edgeBiasBins * binHz);
    const double matchingRadiusHz = std::min(Evidence::matchingRadiusBins * binHz,
        Evidence::matchingRadiusFundamentalFraction * fundamental);
    double strongest = 0.0;
    for (int bin = firstPeak; bin <= lastPeak; ++bin)
    {
        const double left = frame.power[bin - 1], peak = frame.power[bin], right = frame.power[bin + 1];
        if (!(peak > left && peak >= right)) continue;
        const double measuredHz = interpolatedPeakHz(bin, binHz, left, peak, right);
        if (measuredHz < order * harmonicCarrierMinimumHz - edgeBiasHz) continue;
        const double offset = std::abs(measuredHz - expectedHz) / matchingRadiusHz;
        if (offset >= 1.0) continue;

        // Compare with the higher shoulder. Broad bass energy gives little evidence.
        const double shoulder = std::max(frame.power[std::max(firstPositiveBin, bin - Evidence::shoulderDistanceBins)],
                                         frame.power[std::min(last, bin + Evidence::shoulderDistanceBins)]);
        const double contrast = 1.0 - shoulder / peak;
        const double prominence = std::clamp((contrast - Evidence::minimumContrast)
            / (Evidence::fullContrast - Evidence::minimumContrast), 0.0, 1.0);
        const double magnitudeSupport = std::min(1.0, Evidence::hannMagnitudeAllowance * std::sqrt(peak) / expectedMagnitude);
        const double detuning = std::max(0.0, (offset - Evidence::detuningDeadband) / (1.0 - Evidence::detuningDeadband));
        strongest = std::max(strongest, magnitudeSupport * prominence * (1.0 - detuning * detuning));
    }
    return strongest;
}
}

PitchMotionResult HarmonicCombProfile::prepare(const PitchMotionSettings& settings) noexcept
{
    const auto& c = settings.harmonicComb;
    if (!validSpectralWeightingSettings(settings)
        || !harmonicControls::subtraction.contains(c.subtractionFraction)
        || !harmonicControls::decay.contains(c.harmonicDecay)
        || c.maximumHarmonics < 1 || c.maximumHarmonics > harmonicMaximumHarmonics
        || !harmonicControls::components.contains(c.maximumComponents)
        || !std::isfinite(c.minimumRelativeAmplitude) || c.minimumRelativeAmplitude < 0 || c.minimumRelativeAmplitude > 1
        || !std::isfinite(c.minimumErrorImprovement) || c.minimumErrorImprovement < 0 || c.minimumErrorImprovement > 1
        || !std::isfinite(c.minimumComponentDistanceSemitones) || c.minimumComponentDistanceSemitones < 0 || c.minimumComponentDistanceSemitones > semitonesPerOctave
        || !std::isfinite(c.harmonicSupportThreshold) || c.harmonicSupportThreshold < 0 || c.harmonicSupportThreshold > 1
        || !harmonicControls::wideningPercent.contains(c.harmonicWideningPercent)
        || !harmonicControls::trailMs.contains(c.trailHalfLifeSeconds * DisplayTiming::millisecondsPerSecond)
        || !harmonicControls::gain.contains(c.displayGain)
        || !harmonicControls::rangeDb.contains(c.displayRangeDb)
        || !std::isfinite(settings.minimumTotalPower) || settings.minimumTotalPower < 0.0)
    {
        prepared_ = false;
        return fail(PitchMotionResult::invalidConfiguration);
    }
    if (c.maximumHarmonics != settings_.harmonicComb.maximumHarmonics
        || c.harmonicDecay != settings_.harmonicComb.harmonicDecay
        || c.harmonicWideningPercent != settings_.harmonicComb.harmonicWideningPercent
        || c.iterativeExtraction != settings_.harmonicComb.iterativeExtraction) cacheValid_ = false;
    settings_ = settings;
    prepared_ = true;
    reset();
    return PitchMotionResult::ok;
}

void HarmonicCombProfile::beginSnapshot() noexcept
{
    snapshot_ = {};
    snapshot_.enabled = settings_.harmonicComb.enabled;
    snapshot_.iterativeExtraction = settings_.harmonicComb.iterativeExtraction;
    snapshot_.displayGain = settings_.harmonicComb.displayGain;
    snapshot_.logarithmicDisplay = settings_.harmonicComb.logarithmicDisplay;
    snapshot_.highResReference = settings_.harmonicComb.highResReference;
    snapshot_.displayRangeDb = settings_.harmonicComb.displayRangeDb;
    snapshot_.subtractionFraction = settings_.harmonicComb.subtractionFraction;
    snapshot_.harmonicWideningPercent = settings_.harmonicComb.harmonicWideningPercent;
    snapshot_.trailHalfLifeSeconds = settings_.harmonicComb.trailHalfLifeSeconds;
}

void HarmonicCombProfile::reset() noexcept
{
    beginSnapshot();
    snapshot_.generation = ++generation_;
    snapshot_.status = prepared_ && snapshot_.enabled ? PitchMotionResult::warmingUp : PitchMotionResult::notPrepared;
    haveTime_ = false;
    lastTime_ = 0.0;
}

PitchMotionResult HarmonicCombProfile::fail(PitchMotionResult result) noexcept
{
    reset();
    snapshot_.status = result;
    return result;
}

double HarmonicCombProfile::lobe(double offset, int fftSize) noexcept
{
    if (std::abs(offset) >= hannHalfWidthBins) return 0.0;
    return std::abs(dirichlet(offset, fftSize) - 0.5 * dirichlet(offset - 1.0, fftSize)
        - 0.5 * dirichlet(offset + 1.0, fftSize));
}

double HarmonicCombProfile::partialWideningScale(double frequency, int harmonic, double wideningPercent) noexcept
{
    // Called for each fitted FFT tooth, not for the gold marker's display radius.
    // The extra width fades with frequency and never propagates from H1 to H2+.
    const double relativeFrequency = frequency / bassWideningCornerHz;
    const double bassBoost = harmonic == 1
        ? bassWideningBoost / (1.0 + relativeFrequency * relativeFrequency) : 0.0;
    return 1.0 + wideningPercent / 100.0 * (1.0 + bassBoost);
}

HarmonicCombProfile::Partial HarmonicCombProfile::partial(double frequency, double binHz, int fftSize,
    double wideningPercent, int harmonic) noexcept
{
    Partial result;
    const double centre = frequency / binHz;
    const double scale = partialWideningScale(frequency, harmonic, wideningPercent);
    const double halfWidth = hannHalfWidthBins * scale;
    const int first = std::max(1, static_cast<int>(std::ceil(centre - halfWidth)));
    const int last = std::min({fftSize / 2 - 1, static_cast<int>(std::floor(centre + halfWidth)),
        static_cast<int>(std::floor(FrequencySettings::maximumHz / binHz))});
    const double normalization = lobe(0.0, fftSize);
    for (int bin = first; bin <= last; ++bin)
    {
        const double value = lobe((bin - centre) / scale, fftSize) / normalization;
        if (value <= negligibleLobeMagnitude) continue;
        result.bins[result.count] = bin;
        result.values[result.count++] = value;
        result.normSquared += value * value;
    }
    return result;
}

void HarmonicCombProfile::prepareTemplates(const SpectrumFrame& frame) noexcept
{
    const double high = std::min(FrequencySettings::maximumHz, frame.sampleRate * 0.5);
    const double carrierHigh = std::min(harmonicCarrierMaximumHz, high);
    const double span = semitonesPerOctave * std::log2(carrierHigh / harmonicCarrierMinimumHz);
    const double binHz = frame.sampleRate / frame.fftSize;
    const int logCount = static_cast<int>(std::ceil(span / harmonicProfileStepSemitones)) + 1;
    // Keep candidate spacing below half an FFT bin at the upper carrier edge.
    // The capacity floor bounds exceptionally large alternate FFT geometries.
    const double linearStep = std::max(binHz * 0.5,
        (carrierHigh - harmonicCarrierMinimumHz) / (harmonicProfileCapacity - logCount - 1));
    count_ = 0;
    double candidate = harmonicCarrierMinimumHz;
    while (candidate < carrierHigh && count_ < harmonicProfileCapacity - 1)
    {
        frequencies_[count_++] = static_cast<float>(candidate);
        candidate += std::min(candidate * (std::exp2(harmonicProfileStepSemitones / semitonesPerOctave) - 1.0), linearStep);
    }
    frequencies_[count_++] = static_cast<float>(carrierHigh);
    for (int index = 0; index < count_; ++index) buildTemplate(frame, frequencies_[index], templates_[index]);
    cachedRate_ = frame.sampleRate;
    cachedFftSize_ = frame.fftSize;
    cacheValid_ = true;
}

void HarmonicCombProfile::buildTemplate(const SpectrumFrame& frame, double frequency, Template& shape,
    bool refineSupport) const noexcept
{
    shape = {};
    shape.wideningPercent = settings_.harmonicComb.iterativeExtraction
        ? settings_.harmonicComb.harmonicWideningPercent : 0.0;
    const double high = std::min(FrequencySettings::maximumHz, frame.sampleRate * 0.5);
    const double binHz = frame.sampleRate / frame.fftSize;
    int harmonics = std::min(settings_.harmonicComb.maximumHarmonics, static_cast<int>(high / frequency));
    if (refineSupport && settings_.harmonicComb.harmonicSupportThreshold > 0.0)
    {
        std::array<double, harmonicMaximumHarmonics> peaks{};
        double largest = 0.0;
        for (int h = 1; h <= harmonics; ++h)
        {
            const int centre = static_cast<int>(std::round(h * frequency / binHz));
            for (int bin = std::max(1, centre - 1); bin <= std::min(frame.fftSize / 2 - 1, centre + 1); ++bin)
                peaks[h - 1] = std::max(peaks[h - 1], input_[bin]);
            largest = std::max(largest, peaks[h - 1]);
        }
        while (harmonics > 0 && (largest <= 0.0
            || peaks[harmonics - 1] < largest * settings_.harmonicComb.harmonicSupportThreshold)) --harmonics;
    }
    for (int h = 1; h <= harmonics; ++h)
        {
            const auto basis = partial(h * frequency, binHz, frame.fftSize,
                shape.wideningPercent, h);
            const double weight = std::pow(static_cast<double>(h), -settings_.harmonicComb.harmonicDecay);
            for (int j = 0; j < basis.count; ++j)
            {
                int slot = 0;
                while (slot < shape.count && shape.bins[slot] != basis.bins[j]) ++slot;
                if (slot == shape.count)
                {
                    shape.bins[shape.count++] = static_cast<std::uint16_t>(basis.bins[j]);
                }
                shape.values[slot] += static_cast<float>(weight * basis.values[j]);
            }
        }
    for (int j = 0; j < shape.count; ++j)
        shape.normSquared += static_cast<double>(shape.values[j]) * shape.values[j];
}

double HarmonicCombProfile::amplitude(const Template& shape,
    const std::array<double, AnalysisLimits::maximumFftSize / 2 + 1>& data) const noexcept
{
    double dot = 0.0;
    for (int j = 0; j < shape.count; ++j) dot += shape.values[j] * data[shape.bins[j]];
    return shape.normSquared > 0.0 ? std::max(0.0, dot / shape.normSquared) : 0.0;
}

double HarmonicCombProfile::bassSupport(const SpectrumFrame& frame, double frequency, double projection) const noexcept
{
    if (frequency >= BassCarrierEvidence::weightingEndsAtHz) return 1.0;
    if (!(projection > 0.0)) return 0.0;
    // Accept a resolved fundamental, or the weaker of two corroborating harmonics.
    // A single upper partial cannot establish a missing fundamental.
    double weight = resolvedToothSupport(frame, frequency, 1, projection);
    const auto& comb = settings_.harmonicComb;
    if (weight < 1.0 && comb.maximumHarmonics >= 3)
    {
        const auto harmonicSupport = [&](int order)
        {
            const double expectedMagnitude = projection * std::pow(static_cast<double>(order), -comb.harmonicDecay);
            return resolvedToothSupport(frame, frequency, order, expectedMagnitude);
        };
        weight = std::max(weight, std::min(harmonicSupport(2), harmonicSupport(3)));
    }
    constexpr double transitionHz = BassCarrierEvidence::weightingEndsAtHz - BassCarrierEvidence::fullWeightingThroughHz;
    const double t = std::clamp((frequency - BassCarrierEvidence::fullWeightingThroughHz) / transitionHz, 0.0, 1.0);
    // Cubic smoothstep releases the bass requirement continuously across the transition.
    return weight + (1.0 - weight) * t * t * (3.0 - 2.0 * t);
}

double HarmonicCombProfile::selectionGain(const SpectrumFrame& frame, double frequency, const Template& shape) const noexcept
{
    const double reference = harmonicReferenceGain(frequency, settings_.harmonicComb.highResReference);
    return frequency >= BassCarrierEvidence::weightingEndsAtHz ? reference : reference * bassSupport(frame, frequency, amplitude(shape, input_));
}

double HarmonicCombProfile::overlap(const Template& a, const Template& b) noexcept
{
    double dot = 0.0;
    int i = 0, j = 0;
    while (i < a.count && j < b.count)
    {
        if (a.bins[i] < b.bins[j]) ++i;
        else if (b.bins[j] < a.bins[i]) ++j;
        else { dot += static_cast<double>(a.values[i++]) * b.values[j++]; }
    }
    return dot;
}

bool HarmonicCombProfile::similarToSelected(const Template& shape, double frequency, int count) const noexcept
{
    if (!(shape.normSquared > 0.0)) return true;
    for (int i = 0; i < count; ++i)
    {
        if (std::abs(semitonesPerOctave * std::log2(frequency / selectedFrequencies_[i]))
            < settings_.harmonicComb.minimumComponentDistanceSemitones) return true;
        const auto& other = selectedTemplates_[i];
        const double dot = overlap(shape, other);
        if (dot * dot > duplicateTemplateCosineSquared * shape.normSquared * other.normSquared) return true;
    }
    return false;
}

void HarmonicCombProfile::jointFit(int count, double inputEnergy,
    std::array<double, harmonicMaximumComponents>& amplitudes) noexcept
{
    constexpr int capacity = harmonicMaximumComponents;
    // Enumerate <=16 active supports. Each positive definite principal system
    // has <=4 unknowns; normalized Cholesky rejects numerically duplicate atoms.
    double gram[capacity][capacity]{}, rhs[capacity]{}, norm[capacity]{};
    for (int i = 0; i < count; ++i) norm[i] = std::sqrt(selectedTemplates_[i].normSquared);
    for (int i = 0; i < count; ++i)
    {
        const auto& t = selectedTemplates_[i];
        for (int k = 0; k < t.count; ++k) rhs[i] += t.values[k] * input_[t.bins[k]] / norm[i];
        for (int j = 0; j <= i; ++j)
            gram[i][j] = gram[j][i] = overlap(t, selectedTemplates_[j]) / (norm[i] * norm[j]);
    }
    amplitudes.fill(0.0);
    double bestError = inputEnergy;
    for (int mask = 1; mask < (1 << count); ++mask)
    {
        int indices[capacity]{}, active = 0;
        for (int i = 0; i < count; ++i) if (mask & (1 << i)) indices[active++] = i;
        double lower[capacity][capacity]{}, y[capacity]{}, x[capacity]{}, values[capacity]{};
        bool valid = true;
        for (int i = 0; i < active && valid; ++i)
            for (int j = 0; j <= i; ++j)
            {
                double value = gram[indices[i]][indices[j]];
                for (int k = 0; k < j; ++k) value -= lower[i][k] * lower[j][k];
                if (i == j)
                {
                    if (value <= minimumCholeskyPivot) { valid = false; break; }
                    lower[i][j] = std::sqrt(value);
                }
                else lower[i][j] = value / lower[j][j];
            }
        if (!valid) continue;
        for (int i = 0; i < active; ++i)
        {
            double value = rhs[indices[i]];
            for (int j = 0; j < i; ++j) value -= lower[i][j] * y[j];
            y[i] = value / lower[i][i];
        }
        for (int i = active - 1; i >= 0; --i)
        {
            double value = y[i];
            for (int j = i + 1; j < active; ++j) value -= lower[j][i] * x[j];
            x[i] = value / lower[i][i];
            if (!std::isfinite(x[i]) || x[i] < 0.0) valid = false;
            values[indices[i]] = x[i];
        }
        if (!valid) continue;
        double error = inputEnergy;
        for (int i = 0; i < count; ++i)
        {
            error -= 2.0 * values[i] * rhs[i];
            for (int j = 0; j < count; ++j) error += values[i] * gram[i][j] * values[j];
        }
        if (error < bestError)
        {
            bestError = error;
            for (int i = 0; i < count; ++i) amplitudes[i] = values[i] / norm[i];
        }
    }
    double kkt = 0.0;
    for (int i = 0; i < count; ++i)
    {
        double gradient = rhs[i];
        for (int j = 0; j < count; ++j) gradient -= gram[i][j] * amplitudes[j] * norm[j];
        kkt = std::max(kkt, amplitudes[i] > 0.0 ? std::abs(gradient) : std::max(0.0, gradient));
    }
    snapshot_.fitKktResidual = kkt / std::max(minimumKktScale, std::sqrt(inputEnergy));
}

double HarmonicCombProfile::reconstruct(const SpectrumFrame& frame, int count,
    const std::array<double, harmonicMaximumComponents>& amplitudes) noexcept
{
    std::fill(fitted_.begin(), fitted_.end(), 0.0);
    for (int i = 0; i < count; ++i)
    {
        const auto& t = selectedTemplates_[i];
        for (int k = 0; k < t.count; ++k) fitted_[t.bins[k]] += amplitudes[i] * t.values[k];
    }
    double error = 0.0;
    snapshot_.reconstructionError = 0.0;
    for (int bin = 0; bin <= frame.fftSize / 2; ++bin)
    {
        residual_[bin] = input_[bin] - fitted_[bin];
        error += residual_[bin] * residual_[bin];
        snapshot_.reconstructionError = std::max(snapshot_.reconstructionError,
            std::abs(input_[bin] - (fitted_[bin] + residual_[bin])));
    }
    return error;
}

void HarmonicCombProfile::extractComponents(const SpectrumFrame& frame, double inputEnergy) noexcept
{
    std::array<double, harmonicMaximumComponents> amplitudes{}, frequencies{}, atSelection{};
    snapshot_.inputEnergy = inputEnergy;
    snapshot_.errorEnergy[0] = inputEnergy;
    double error = inputEnergy, firstScore = 0.0;
    int accepted = 0;
    for (int pass = 0; pass < settings_.harmonicComb.maximumComponents; ++pass)
    {
        int best = -1;
        double bestScore = 0.0;
        for (int i = 0; i < count_; ++i)
        {
            const double score = amplitude(templates_[i], residual_) * supportWeights_[i]
                * harmonicReferenceGain(frequencies_[i], settings_.harmonicComb.highResReference);
            if (score > bestScore && !similarToSelected(templates_[i], frequencies_[i], accepted))
            { best = i; bestScore = score; }
        }
        if (best < 0 || !(bestScore > 0.0)) break;
        Template chosen = templates_[best], trial;
        double frequency = frequencies_[best];
        // Refine the proposal's observed harmonic tail once against original M,
        // then use this same fixed shape for all joint fits and reconstructions.
        buildTemplate(frame, frequency, chosen, true);
        bestScore = amplitude(chosen, residual_) * selectionGain(frame, frequency, chosen);
        if (!(chosen.normSquared > 0.0) || !(bestScore > 0.0)) break;
        double step = best > 0 ? frequency - frequencies_[best - 1]
            : frequencies_[1] - frequency;
        if (best + 1 < count_) step = std::min(step, static_cast<double>(frequencies_[best + 1]) - frequency);
        // Bounded local refinement avoids leaving a neighboring grid atom behind.
        for (int round = 0; round < frequencyRefinementRounds; ++round)
        {
            step *= 0.5;
            const double centre = frequency;
            for (int direction : {-1, 1})
            {
                const double f = std::clamp(centre + direction * step, harmonicCarrierMinimumHz, harmonicCarrierMaximumHz);
                buildTemplate(frame, f, trial, true);
                const double score = amplitude(trial, residual_) * selectionGain(frame, f, trial);
                if (score > bestScore && !similarToSelected(trial, f, accepted))
                { bestScore = score; frequency = f; chosen = trial; }
            }
        }
        if (frequency - step >= harmonicCarrierMinimumHz && frequency + step <= harmonicCarrierMaximumHz)
        {
            buildTemplate(frame, frequency - step, trial, true);
            const double left = amplitude(trial, residual_) * selectionGain(frame, frequency - step, trial);
            buildTemplate(frame, frequency + step, trial, true);
            const double right = amplitude(trial, residual_) * selectionGain(frame, frequency + step, trial);
            const double curvature = left - 2.0 * bestScore + right;
            if (curvature < 0.0)
            {
                const double f = frequency + std::clamp(0.5 * (left - right) / curvature, -1.0, 1.0) * step;
                buildTemplate(frame, f, trial, true);
                const double score = amplitude(trial, residual_) * selectionGain(frame, f, trial);
                if (score > bestScore && !similarToSelected(trial, f, accepted))
                { frequency = f; chosen = trial; bestScore = score; }
            }
        }
        if (pass == 0) firstScore = bestScore;
        if (bestScore < firstScore * settings_.harmonicComb.minimumRelativeAmplitude) break;
        selectedTemplates_[accepted] = chosen;
        selectedFrequencies_[accepted] = frequency;
        frequencies[accepted] = frequency;
        atSelection[accepted] = amplitude(chosen, residual_);
        auto next = amplitudes;
        const double previousKkt = snapshot_.fitKktResidual;
        jointFit(accepted + 1, inputEnergy, next);
        const double nextError = reconstruct(frame, accepted + 1, next);
        if (!(error - nextError > settings_.harmonicComb.minimumErrorImprovement * inputEnergy))
        {
            reconstruct(frame, accepted, amplitudes);
            snapshot_.fitKktResidual = previousKkt;
            break;
        }
        amplitudes = next;
        error = nextError;
        snapshot_.errorEnergy[++accepted] = error;
    }
    snapshot_.extractionSteps = accepted;
    double largest = 0.0;
    for (int i = 0; i < accepted; ++i) largest = std::max(largest, amplitudes[i]);
    std::array<int, harmonicMaximumComponents> ranks{};
    int kept = 0;
    for (int i = 0; i < accepted; ++i)
    {
        // Earlier harmonic aliases can collapse to tiny coefficients when the
        // fundamental is added. Remove these, then refit the published support.
        if (!(amplitudes[i] > largest * negligibleRelativeAmplitude)) continue;
        selectedTemplates_[kept] = selectedTemplates_[i];
        selectedFrequencies_[kept] = selectedFrequencies_[i];
        frequencies[kept] = frequencies[i];
        atSelection[kept] = atSelection[i];
        ranks[kept++] = i + 1;
    }
    if (kept != accepted)
    {
        jointFit(kept, inputEnergy, amplitudes);
        error = reconstruct(frame, kept, amplitudes);
        snapshot_.errorEnergy[accepted] = error;
    }
    for (int i = 0; i < kept; ++i)
    {
        if (!(amplitudes[i] > 0.0)) continue;
        auto& component = snapshot_.components[snapshot_.componentCount++];
        component.frequencyHz = frequencies[i];
        component.fitAmplitude = amplitudes[i];
        component.displayAmplitude = amplitudes[i] * selectionGain(frame, frequencies[i], selectedTemplates_[i]);
        component.amplitudeAtSelection = atSelection[i];
        component.extractionRank = ranks[i];
        component.wideningPercent = selectedTemplates_[i].wideningPercent;
    }
    snapshot_.residualErrorEnergy = error;
    snapshot_.fitFraction = std::clamp(1.0 - error / inputEnergy, 0.0, 1.0);
    snapshot_.removedEnergyFraction = snapshot_.fitFraction; // Legacy field: now reduction of squared fitting error.
    if (snapshot_.componentCount > 0)
    {
        snapshot_.selectedFrequencyHz = snapshot_.components[0].frequencyHz;
        snapshot_.selectedAmplitude = snapshot_.components[0].fitAmplitude;
        snapshot_.fittedHarmonics = std::min(settings_.harmonicComb.maximumHarmonics,
            static_cast<int>(snapshot_.maximumFrequencyHz / snapshot_.selectedFrequencyHz));
    }
}

void HarmonicCombProfile::subtractFamily(const SpectrumFrame& frame, int selected) noexcept
{
    std::fill(fitted_.begin(), fitted_.end(), 0.0);
    std::array<Partial, harmonicMaximumHarmonics> partials{};
    int count = 0;
    const double frequency = frequencies_[selected], binHz = frame.sampleRate / frame.fftSize;
    const double high = std::min(FrequencySettings::maximumHz, frame.sampleRate * 0.5);
    for (int h = 1; h <= settings_.harmonicComb.maximumHarmonics && h * frequency <= high; ++h)
        partials[count++] = partial(h * frequency, binHz, frame.fftSize);
    // Nonnegative coordinate descent refits observed partial amplitudes. Four
    // bounded sweeps account for overlapping main lobes; no exact NNLS claim.
    for (int pass = 0; pass < legacyFitSweeps; ++pass)
        for (int h = 0; h < count; ++h)
        {
            auto& basis = partials[h];
            double dot = 0.0;
            for (int j = 0; j < basis.count; ++j)
            {
                const int bin = basis.bins[j];
                dot += basis.values[j] * (input_[bin] - fitted_[bin] + basis.amplitude * basis.values[j]);
            }
            const double next = basis.normSquared > 0.0 ? std::max(0.0, dot / basis.normSquared) : 0.0;
            for (int j = 0; j < basis.count; ++j)
                fitted_[basis.bins[j]] += (next - basis.amplitude) * basis.values[j];
            basis.amplitude = next;
        }
    double inputEnergy = 0.0, errorEnergy = 0.0, residualEnergy = 0.0;
    for (int bin = 0; bin <= frame.fftSize / 2; ++bin)
    {
        const double fitted = std::clamp(fitted_[bin], 0.0, input_[bin]);
        const double error = input_[bin] - fitted;
        residual_[bin] = input_[bin] - settings_.harmonicComb.subtractionFraction * fitted;
        inputEnergy += input_[bin] * input_[bin];
        errorEnergy += error * error;
        residualEnergy += residual_[bin] * residual_[bin];
    }
    for (int h = 0; h < count; ++h)
        if (partials[h].amplitude > snapshot_.selectedAmplitude * negligibleRelativeAmplitude) ++snapshot_.fittedHarmonics;
    snapshot_.fitFraction = inputEnergy > 0.0 ? std::clamp(1.0 - errorEnergy / inputEnergy, 0.0, 1.0) : 0.0;
    snapshot_.removedEnergyFraction = inputEnergy > 0.0 ? std::clamp(1.0 - residualEnergy / inputEnergy, 0.0, 1.0) : 0.0;
}

PitchMotionResult HarmonicCombProfile::process(const SpectrumFrame& frame) noexcept
{
    if (!prepared_) return fail(PitchMotionResult::notPrepared);
    if (!settings_.harmonicComb.enabled) { reset(); return snapshot_.status; }
    if (!std::isfinite(frame.sampleRate)) return fail(PitchMotionResult::nonFiniteInput);
    if (frame.sampleRate < AnalysisLimits::minimumSampleRate || frame.sampleRate > AnalysisLimits::maximumSampleRate
        || frame.fftSize < AnalysisLimits::minimumFftSize || frame.fftSize > AnalysisLimits::maximumFftSize
        || (frame.fftSize & (frame.fftSize - 1)) != 0 || frame.hopSize < 1 || frame.hopSize > frame.fftSize
        || (frame.channels != 1 && frame.channels != 2)
        || frame.power.size() != static_cast<std::size_t>(frame.fftSize / 2 + 1)) return fail(PitchMotionResult::invalidFrame);
    for (float power : frame.power)
    {
        if (!std::isfinite(power)) return fail(PitchMotionResult::nonFiniteInput);
        if (power < 0.0f) return fail(PitchMotionResult::invalidFrame);
    }
    if (haveTime_ && (frame.sampleRate != cachedRate_ || frame.fftSize != cachedFftSize_
        || frame.hopSize != lastHop_ || frame.channels != lastChannels_
        || frame.firstSample < lastFirstSample_
        || frame.firstSample - lastFirstSample_ != static_cast<std::uint64_t>(frame.hopSize))) reset();
    // Same short-window centre convention, without linking the analyzer into
    // the standalone numerical probe.
    const double time = (static_cast<double>(frame.firstSample) + frame.fftSize * 0.5) / frame.sampleRate;
    const double step = haveTime_ ? time - lastTime_ : 0.0;
    if (!cacheValid_ || cachedRate_ != frame.sampleRate || cachedFftSize_ != frame.fftSize) prepareTemplates(frame);
    beginSnapshot();
    snapshot_.generation = generation_;
    snapshot_.count = count_;
    snapshot_.frequencyHz = frequencies_;
    snapshot_.maximumFrequencyHz = std::min(FrequencySettings::maximumHz, frame.sampleRate * 0.5);
    snapshot_.centreTimeSeconds = time;
    snapshot_.stepSeconds = step;
    haveTime_ = true;
    lastTime_ = time;
    lastFirstSample_ = frame.firstSample;
    lastHop_ = frame.hopSize;
    lastChannels_ = frame.channels;
    snapshot_.inputBinHz = frame.sampleRate / frame.fftSize;
    snapshot_.firstInputBin = std::max(1, static_cast<int>(std::floor(harmonicCarrierMinimumHz / snapshot_.inputBinHz)));
    const int lastInputBin = std::min(frame.fftSize / 2 - 1,
        static_cast<int>(std::ceil(harmonicCarrierMaximumHz / snapshot_.inputBinHz)));
    snapshot_.inputSpectrumCount = lastInputBin - snapshot_.firstInputBin + 1;
    // For every valid frame (Fs>=8k, N<=32768), the carrier band plus edge bins
    // fits the bounded publication buffer. No UI FFT or processing allocation.
    if (snapshot_.inputSpectrumCount < 0 || snapshot_.inputSpectrumCount > harmonicInputSpectrumCapacity)
        return fail(PitchMotionResult::invalidFrame);
    for (int index = 0; index < snapshot_.inputSpectrumCount; ++index)
        snapshot_.inputMagnitude[index] = std::sqrt(frame.power[snapshot_.firstInputBin + index]);
    const auto noCarrier = [this]()
    {
        // Publish the current raw FFT even when cleaning rejects all candidates.
        // The freshly zeroed profile never retains an old carrier estimate.
        snapshot_.status = PitchMotionResult::noSignal;
        haveTime_ = false;
        return snapshot_.status;
    };
    double energy = 0.0;
    for (int bin = 0; bin <= frame.fftSize / 2; ++bin)
    {
        const double hz = bin * frame.sampleRate / frame.fftSize;
        input_[bin] = bin > 0 && bin < frame.fftSize / 2 && hz <= snapshot_.maximumFrequencyHz
            ? std::sqrt(cleanSpectralPower(frame, bin, settings_)) : 0.0;
        residual_[bin] = input_[bin];
        energy += input_[bin] * input_[bin];
    }
    if (!(energy > settings_.minimumTotalPower)) return noCarrier();
    snapshot_.inputEnergy = energy;
    int selected = 0;
    for (int index = 0; index < count_; ++index)
    {
        const double projected = amplitude(templates_[index], input_);
        supportWeights_[index] = bassSupport(frame, frequencies_[index], projected);
        snapshot_.before[index] = static_cast<float>(projected * supportWeights_[index]);
        if (snapshot_.before[index] > snapshot_.before[selected]) selected = index;
    }
    if (settings_.harmonicComb.iterativeExtraction)
    {
        extractComponents(frame, energy);
    }
    else if (snapshot_.before[selected] > 0.0f)
    {
        snapshot_.selectedFrequencyHz = frequencies_[selected];
        snapshot_.selectedAmplitude = static_cast<float>(amplitude(templates_[selected], input_));
        subtractFamily(frame, selected);
        snapshot_.componentCount = 1;
        snapshot_.components[0] = {snapshot_.selectedFrequencyHz, snapshot_.selectedAmplitude,
            snapshot_.before[selected] * harmonicReferenceGain(snapshot_.selectedFrequencyHz, settings_.harmonicComb.highResReference),
            snapshot_.selectedAmplitude, 1};
    }
    if (snapshot_.componentCount == 0)
    {
        snapshot_.after = snapshot_.before;
        snapshot_.residualErrorEnergy = energy;
        for (int i = 0; i < snapshot_.inputSpectrumCount; ++i)
            snapshot_.errorMagnitude[i] = static_cast<float>(input_[snapshot_.firstInputBin + i]);
        return noCarrier();
    }
    for (int index = 0; index < count_; ++index)
        snapshot_.after[index] = !settings_.harmonicComb.iterativeExtraction && settings_.harmonicComb.subtractionFraction == 0.0 ? snapshot_.before[index]
            : static_cast<float>(amplitude(templates_[index], residual_) * supportWeights_[index]);
    for (int index = 0; index < snapshot_.inputSpectrumCount; ++index)
    {
        const int bin = snapshot_.firstInputBin + index;
        snapshot_.modelMagnitude[index] = static_cast<float>(input_[bin] - residual_[bin]);
        snapshot_.errorMagnitude[index] = static_cast<float>(residual_[bin]);
    }
    snapshot_.hasEstimate = true;
    snapshot_.status = PitchMotionResult::ok;
    return snapshot_.status;
}
}
