// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SpectralCorrelation.h"
#include "../../tests/TestSupport.h"
#include <iostream>
#include <memory>
#include <string>

namespace fxanalysis::test
{
namespace
{
using namespace fxanalysis::experimental;
struct Component { double frequency, power; };

PitchMotionSettings baseline()
{
    PitchMotionSettings settings;
    settings.noiseFloorStrength = 0.0;
    settings.minimumPeakProminenceDb = 0.0;
    settings.peakWeightExponent = 1.0;
    return settings;
}

// Known continuous power density on the musical axis, sampled into fine FFT
// bins. The 1/f Jacobian makes component.power an energy, independent of pitch.
// These numerical spectra are separate from the actual PCM fixtures below.
SpectrumFrame logSpectrum(double displacement, const std::vector<Component>& components = {
    {500.0, 1.0}, {1200.0, 0.6}, {3300.0, 0.4}, {7500.0, 0.2}})
{
    SpectrumFrame frame;
    frame.sampleRate = 48000.0;
    frame.fftSize = 32768;
    frame.hopSize = 512;
    frame.channels = 2;
    frame.power.resize(frame.fftSize / 2 + 1);
    constexpr double width = 0.22;
    const double binHz = frame.sampleRate / frame.fftSize;
    for (int bin = 1; bin < frame.fftSize / 2; ++bin)
    {
        const double frequency = bin * binHz;
        double density = 0.0;
        for (const auto component : components)
        {
            const double position = 12.0 * std::log2(frequency / component.frequency) - displacement;
            density += component.power * std::exp(-0.5 * position * position / (width * width));
        }
        frame.power[bin] = static_cast<float>(density * binHz * 12.0
            / (std::log(2.0) * frequency * width * std::sqrt(2.0 * pi)));
    }
    return frame;
}

SpectralCorrelationSnapshot compare(SpectrumFrame source, SpectrumFrame target,
    PitchMotionSettings settings = baseline())
{
    auto motion = std::make_unique<SpectralCorrelation>();
    motion->prepare(settings);
    for (int step = 0; step < 4; ++step)
    {
        source.firstSample = static_cast<std::uint64_t>(step) * source.hopSize;
        motion->process(source);
    }
    target.firstSample = static_cast<std::uint64_t>(4) * target.hopSize;
    motion->process(target);
    return motion->snapshot();
}

bool finite(const SpectralCorrelationSnapshot& snapshot)
{
    for (double value : {snapshot.centreTimeSeconds, snapshot.stepSeconds,
        snapshot.integratedSemitones, snapshot.combinedSemitonesPerSecond})
        if (!std::isfinite(value)) return false;
    for (const auto& lag : snapshot.lags)
    {
        for (float value : lag.correlation)
            if (!std::isfinite(value) || value < 0.0f || value > 1.0f) return false;
        for (double value : {lag.comparisonSeconds, lag.displacementSemitones,
            lag.peakCorrelation, lag.prominence})
            if (!std::isfinite(value)) return false;
        if (lag.prominence < 0.0 || lag.prominence > 1.0) return false;
    }
    return true;
}

void valueCheck(const Check& check, const char* name, double value, double low, double high)
{
    const bool passed = std::isfinite(value) && value >= low && value <= high;
    if (!passed) std::cerr << name << ": " << value << ", expected [" << low << ", " << high << "]\n";
    check(name, passed);
}

void runKnownSpectrumTests(const Check& check)
{
    const auto source = logSpectrum(0.0);
    auto engine = std::make_unique<SpectralCorrelation>();
    check("correlation_unprepared", engine->process(source) == PitchMotionResult::notPrepared);
    auto badSettings = baseline();
    badSettings.peakWeightExponent = std::numeric_limits<double>::quiet_NaN();
    check("correlation_invalid_weight_settings", engine->prepare(badSettings) == PitchMotionResult::invalidConfiguration);

    const auto identity = compare(source, source);
    check("correlation_identity_available_and_finite", identity.hasEstimate && finite(identity)
        && identity.lags[0].hasEstimate && !identity.lags[1].hasEstimate && !identity.lags[2].hasEstimate);
    valueCheck(check, "correlation_identity_maximum", identity.lags[0].peakCorrelation, 0.99999, 1.0);
    valueCheck(check, "correlation_identity_zero_shift", std::abs(identity.lags[0].displacementSemitones), 0.0, 1e-7);
    auto gainChanged = source;
    for (auto& power : gainChanged.power) power *= 0.001f;
    const auto gain = compare(source, gainChanged);
    double gainDifference = 0.0;
    for (int bin = 0; bin < pitchHistogramBinCount; ++bin)
        gainDifference = std::max(gainDifference, std::abs(static_cast<double>(gain.lags[0].correlation[bin])
            - identity.lags[0].correlation[bin]));
    valueCheck(check, "correlation_global_gain_invariance", gainDifference, 0.0, 1e-6);

    const auto up = compare(source, logSpectrum(1.0));
    const auto down = compare(source, logSpectrum(-1.0));
    valueCheck(check, "correlation_known_up_semitone", up.lags[0].displacementSemitones, 0.94, 1.06);
    valueCheck(check, "correlation_known_down_semitone", down.lags[0].displacementSemitones, -1.06, -0.94);
    check("correlation_shift_alignment_strength", up.lags[0].peakCorrelation > 0.98
        && down.lags[0].peakCorrelation > 0.98 && !up.lags[0].atSearchBoundary);
    valueCheck(check, "correlation_reversing_pair_changes_sign",
        std::abs(compare(logSpectrum(1.0), source).lags[0].displacementSemitones
            + up.lags[0].displacementSemitones), 0.0, 1e-5);

    // Equal component energy must not gain an implicit low-frequency advantage.
    // Overlap normalization intentionally allows a cropped component to align
    // perfectly even when unmatched energy exists outside the common interval.
    const auto edgeSource = logSpectrum(0.0, {{4000.0, 1.0}, {8000.0, 4.0}});
    const auto edgeTarget = logSpectrum(0.0, {{8000.0, 1.0}});
    const auto edge = compare(edgeSource, edgeTarget);
    valueCheck(check, "correlation_conservative_power_weighting", edge.lags[0].correlation[48], 0.88, 0.91);
    valueCheck(check, "correlation_norms_use_only_actual_overlap", edge.lags[0].correlation[96], 0.98, 1.0);
    check("correlation_actual_overlap_does_not_wrap_octaves", edge.lags[0].correlation[0] < 1e-6
        && edge.lags[0].atSearchBoundary && edge.lags[0].displacementSemitones == 12.0);

    // Power density proportional to 1/f gives uniform energy per log cell.
    // A fixed full-vector denominator would give (N-|shift|)/N here.
    auto uniform = source;
    for (int bin = 1; bin < uniform.fftSize / 2; ++bin)
        uniform.power[bin] = 1.0f / static_cast<float>(bin);
    auto uniformSettings = baseline();
    uniformSettings.minimumFrequencyHz = 1000.0;
    uniformSettings.maximumFrequencyHz = 8000.0;
    const auto plateau = compare(uniform, uniform, uniformSettings);
    const auto& scores = plateau.lags[0].correlation;
    check("correlation_uniform_log_energy_removes_overlap_triangle", finite(plateau)
        && *std::min_element(scores.begin(), scores.end()) > 0.99999f);
    check("correlation_flat_overlap_plateau_prefers_zero_shift", plateau.lags[0].displacementSemitones == 0.0
        && plateau.lags[0].prominence < 1e-5 && plateau.combinedSemitonesPerSecond == 0.0);
    uniformSettings.maximumFrequencyHz = 2000.0;
    const auto narrow = compare(uniform, uniform, uniformSettings);
    check("correlation_empty_overlap_is_finite_zero", finite(narrow)
        && narrow.lags[0].correlation.front() == 0.0f && narrow.lags[0].correlation.back() == 0.0f);

    const auto boundary = compare(logSpectrum(0.0, {{1000.0, 1.0}}),
        logSpectrum(12.0, {{1000.0, 1.0}}));
    check("correlation_search_boundary_flagged", boundary.lags[0].hasEstimate
        && boundary.lags[0].atSearchBoundary && boundary.lags[0].displacementSemitones == 12.0
        && boundary.combinedSemitonesPerSecond == 0.0);
}

void runTimeAndBoundaryTests(const Check& check)
{
    auto motion = std::make_unique<SpectralCorrelation>();
    auto observer = std::make_unique<SpectralCorrelation>();
    const auto settings = baseline();
    motion->prepare(settings);
    observer->prepare(settings);
    bool correctLags = true, correctIntegral = true, finiteAll = true;
    SpectralCorrelationSnapshot previous;
    int availableAll = 0;
    constexpr double stepSeconds = 512.0 / 48000.0;
    for (int step = 0; step <= 80; ++step)
    {
        auto frame = logSpectrum(5.0 * step * stepSeconds);
        frame.firstSample = static_cast<std::uint64_t>(step) * 512;
        motion->process(frame);
        observer->process(frame);
        const auto current = motion->snapshot();
        finiteAll = finiteAll && finite(current);
        int available = 0;
        for (int lag = 0; lag < spectralCorrelationLagCount; ++lag)
        {
            if (!current.lags[lag].hasEstimate) continue;
            ++available;
            const double expected = std::ceil(spectralCorrelationRequestedLags[lag] / stepSeconds - 1e-8) * stepSeconds;
            correctLags = correctLags && std::abs(current.lags[lag].comparisonSeconds - expected) < 1e-9;
        }
        if (available == spectralCorrelationLagCount) ++availableAll;
        if (current.hasEstimate)
        {
            const double alpha = std::exp(-current.stepSeconds / settings.integrationSeconds);
            const double expected = std::clamp(alpha * previous.integratedSemitones
                + settings.integrationSeconds * (1.0 - alpha) * current.combinedSemitonesPerSecond,
                -settings.integratorLimitSemitones, settings.integratorLimitSemitones);
            correctIntegral = correctIntegral && std::abs(current.integratedSemitones - expected) < 1e-8;
        }
        for (int read = 0; read < (step % 7); ++read) (void)observer->snapshot();
        previous = current;
    }
    check("correlation_three_lags_use_actual_sample_time", finiteAll && correctLags && availableAll > 60);
    check("correlation_integrates_new_hop_once_not_each_lag", correctIntegral);
    valueCheck(check, "correlation_known_glide_combined_velocity",
        motion->snapshot().combinedSemitonesPerSecond, 4.0, 6.0);
    check("correlation_ui_cadence_does_not_advance_state",
        motion->snapshot().integratedSemitones == observer->snapshot().integratedSemitones
        && motion->snapshot().lags[0].correlation == observer->snapshot().lags[0].correlation);

    auto frame = logSpectrum(0.0);
    frame.firstSample = 100000;
    motion->process(frame);
    check("correlation_gap_discards_history_and_integral", !motion->snapshot().hasEstimate
        && motion->snapshot().integratedSemitones == 0.0);
    frame.firstSample += 512;
    frame.sampleRate = 44100.0;
    motion->process(frame);
    check("correlation_format_change_discards_history", !motion->snapshot().hasEstimate);
    frame.power[4] = std::numeric_limits<float>::quiet_NaN();
    check("correlation_nonfinite_is_explicit", motion->process(frame) == PitchMotionResult::nonFiniteInput
        && !motion->snapshot().hasEstimate && finite(motion->snapshot()));
    frame.power[4] = -1.0f;
    check("correlation_negative_power_rejected", motion->process(frame) == PitchMotionResult::invalidFrame);
    frame.power.assign(frame.power.size(), 0.0f);
    check("correlation_silence_does_not_normalize_noise", motion->process(frame) == PitchMotionResult::noSignal
        && !motion->snapshot().hasEstimate && motion->snapshot().integratedSemitones == 0.0);
    frame = logSpectrum(0.0);
    frame.firstSample = 200000;
    motion->process(frame);
    check("correlation_resume_requires_new_reference", !motion->snapshot().hasEstimate);
    motion->reset();
    check("correlation_reset_clears_curves", !motion->snapshot().hasEstimate
        && motion->snapshot().lags[0].correlation == std::array<float, pitchHistogramBinCount>{});
}

void runPcmCorrelationTests(const Check& check)
{
    double velocities[2]{};
    int index = 0;
    for (double rate : {44100.0, 48000.0})
    {
        constexpr double velocity = 5.0;
        const int count = static_cast<int>(rate);
        std::vector<float> pcm(count * 2);
        const double exponent = std::log(2.0) * velocity / 12.0;
        for (int sample = 0; sample < count; ++sample)
        {
            const double time = sample / rate;
            const double phase = 2.0 * pi * 600.0 * std::expm1(exponent * time) / exponent;
            double value = 0.0;
            for (int harmonic = 1; harmonic <= 8; ++harmonic)
                value += 0.15 * std::sin(harmonic * phase) / harmonic;
            pcm[sample * 2] = static_cast<float>(value);
            pcm[sample * 2 + 1] = -pcm[sample * 2];
        }
        AudioAnalyzer analyzer;
        const bool prepared = analyzer.prepare({rate, 2, 2048, 512}) == AnalysisResult::ok;
        Frames frames;
        const bool processed = prepared && analyzer.processBlock(pcm.data(), count, 0, frames) == AnalysisResult::ok;
        auto correlation = std::make_unique<SpectralCorrelation>();
        correlation->prepare({});
        double sum = 0.0;
        int estimates = 0;
        bool good = true;
        for (auto frame : frames.frames)
        {
            // Deterministic broad spectral floor tests the shared rejection stage.
            for (auto& power : frame.power) power += 1e-4f;
            correlation->process(frame);
            const auto& snapshot = correlation->snapshot();
            good = good && finite(snapshot);
            if (snapshot.hasEstimate && snapshot.lags[2].hasEstimate && frame.centreTimeSeconds() > 0.3)
            {
                sum += snapshot.lags[2].displacementSemitones / snapshot.lags[2].comparisonSeconds;
                ++estimates;
            }
        }
        velocities[index++] = estimates > 0 ? sum / estimates : 0.0;
        const std::string tag = rate == 44100.0 ? "44100" : "48000";
        check(("correlation_pcm_harmonics_floor_and_antiphase_" + tag).c_str(), processed && good && estimates > 40);
        valueCheck(check, ("correlation_pcm_glide_velocity_" + tag).c_str(), velocities[index - 1], 4.2, 5.8);
    }
    valueCheck(check, "correlation_pcm_sample_rate_agreement", std::abs(velocities[0] - velocities[1]), 0.0, 0.5);
    auto noise = logSpectrum(0.0);
    std::fill(noise.power.begin(), noise.power.end(), 1e-4f);
    auto reject = std::make_unique<SpectralCorrelation>();
    reject->prepare({});
    check("correlation_flat_floor_is_not_a_pitch_estimate", reject->process(noise) == PitchMotionResult::noSignal
        && !reject->snapshot().hasEstimate);
}
}

void runSpectralCorrelationTests(const Check& check)
{
    runKnownSpectrumTests(check);
    runTimeAndBoundaryTests(check);
    runPcmCorrelationTests(check);
}
}
