// Numerical model tests ported from d1ccaf6; public runtime union tests live in AnalysisRequestTests.cpp.
// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PitchMotion.h"
#include "SpectralWeighting.h"
#include "../../tests/TestSupport.h"
#include "../../runtime/AnalysisStream.h"
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <memory>
#include <type_traits>
#include <utility>

namespace fxanalysis::test
{
namespace
{
using namespace fxanalysis::experimental;
using Snapshot = std::decay_t<decltype(std::declval<PitchMotion>().snapshot())>;
using Peaks = std::vector<PitchPeak>;

PitchMotionSettings energyBaseline()
{
    PitchMotionSettings settings;
    settings.noiseFloorStrength = 0.0;
    settings.minimumPeakProminenceDb = 0.0;
    settings.peakWeightExponent = 1.0;
    return settings;
}

double frequencyAt(double semitones, double reference = 440.0)
{
    return reference * std::exp2(semitones / 12.0);
}

Peaks shifted(Peaks peaks, double semitones, double powerScale = 1.0)
{
    for (auto& peak : peaks)
    {
        peak.frequencyHz = frequencyAt(semitones, peak.frequencyHz);
        peak.power *= powerScale;
    }
    return peaks;
}

double histogramMass(const std::array<float, pitchHistogramBinCount>& histogram)
{
    return std::accumulate(histogram.begin(), histogram.end(), 0.0);
}

double massNear(const Snapshot& snapshot, double semitones, double radius = 0.26)
{
    double sum = 0.0;
    for (int bin = 0; bin < pitchHistogramBinCount; ++bin)
    {
        const double centre = pitchHistogramMinimumSemitones + bin * pitchHistogramStepSemitones;
        if (std::abs(centre - semitones) <= radius)
            sum += snapshot.rawHistogram[bin];
    }
    return sum;
}

bool finiteSnapshot(const Snapshot& snapshot)
{
    for (float value : snapshot.rawHistogram)
        if (!std::isfinite(value) || value < 0.0f) return false;
    for (float value : snapshot.smoothedHistogram)
        if (!std::isfinite(value) || value < 0.0f) return false;
    for (double value : {snapshot.rawLowerTailMass, snapshot.rawUpperTailMass,
             snapshot.smoothedLowerTailMass, snapshot.smoothedUpperTailMass,
             snapshot.matchedMass, snapshot.unmatchedSourceMass, snapshot.unmatchedTargetMass,
             snapshot.excessSourceMass, snapshot.excessTargetMass,
             snapshot.stationaryMass, snapshot.netDisplacementSemitones,
             snapshot.meanMatchedDisplacementSemitones, snapshot.modeSemitones,
             snapshot.netSemitonesPerSecond, snapshot.integratedSemitones,
             snapshot.comparisonSeconds, snapshot.stepSeconds, snapshot.centreTimeSeconds,
             snapshot.solverResidual})
        if (!std::isfinite(value)) return false;
    return true;
}

double histogramDifference(const Snapshot& left, const Snapshot& right)
{
    double largest = 0.0;
    for (int bin = 0; bin < pitchHistogramBinCount; ++bin)
        largest = std::max(largest, std::abs(static_cast<double>(left.rawHistogram[bin])
            - right.rawHistogram[bin]));
    return largest;
}

Snapshot compare(const Peaks& source, const Peaks& target,
                 PitchMotionSettings settings = energyBaseline())
{
    PitchMotion motion;
    motion.prepare(settings);
    motion.processPeaks(source.data(), static_cast<int>(source.size()), 0.0);
    motion.processPeaks(target.data(), static_cast<int>(target.size()), settings.comparisonSeconds);
    return motion.snapshot();
}

void checkValue(const Check& check, const char* name, double value, double minimum, double maximum)
{
    const bool valid = std::isfinite(value) && value >= minimum && value <= maximum;
    if (!valid)
        std::cerr << name << ": " << value << ", expected [" << minimum << ", " << maximum << "]\n";
    check(name, valid);
}

void runPeakTransportTests(const Check& check)
{
    PitchMotion unprepared;
    const Peaks single{{440.0, 1.0}};
    check("pitch_unprepared_status", unprepared.processPeaks(single.data(), 1, 0.0)
        == PitchMotionResult::notPrepared);

    PitchMotionSettings invalid;
    invalid.comparisonSeconds = -1.0;
    check("pitch_rejects_invalid_comparison", unprepared.prepare(invalid)
        == PitchMotionResult::invalidConfiguration);
    invalid = {};
    invalid.minimumFrequencyHz = invalid.maximumFrequencyHz;
    check("pitch_rejects_invalid_frequency_range", unprepared.prepare(invalid)
        == PitchMotionResult::invalidConfiguration);

    // Adjacent peaks expose an entropy-created two-sided motion distribution.
    // Identity must stay central even when the peaks are only one semitone apart.
    const Peaks close{{440.0, 0.50}, {frequencyAt(1.0), 0.25},
        {frequencyAt(2.0), 0.15}, {880.0, 0.10}};
    const auto identity = compare(close, close);
    check("pitch_identity_is_estimate_and_finite", identity.hasEstimate && finiteSnapshot(identity));
    checkValue(check, "pitch_identity_central_mass", massNear(identity, 0.0, 0.01), 0.999, 1.001);
    checkValue(check, "pitch_identity_no_offcentre_mass", histogramMass(identity.rawHistogram)
        - massNear(identity, 0.0, 0.01), 0.0, 1e-5);
    checkValue(check, "pitch_identity_zero_flux", std::abs(identity.netDisplacementSemitones), 0.0, 1e-8);

    auto slightReweighting = close;
    slightReweighting[0].power *= 1.0001;
    const auto almostIdentity = compare(close, slightReweighting);
    check("pitch_near_identity_finite", almostIdentity.hasEstimate && finiteSnapshot(almostIdentity));
    checkValue(check, "pitch_near_identity_offcentre_bound", histogramMass(almostIdentity.rawHistogram)
        - massNear(almostIdentity, 0.0, 0.01), 0.0, 0.005);

    const auto louder = compare(close, shifted(close, 0.0, 1000.0));
    const auto quieter = compare(close, shifted(close, 0.0, 0.001));
    check("pitch_common_gain_invariance", louder.hasEstimate && quieter.hasEstimate
        && histogramDifference(louder, identity) < 1e-5
        && histogramDifference(quieter, identity) < 1e-5
        && std::abs(louder.netDisplacementSemitones) < 1e-7
        && std::abs(quieter.netDisplacementSemitones) < 1e-7);

    const auto up = compare(single, shifted(single, 1.0));
    const auto down = compare(single, shifted(single, -1.0));
    check("pitch_single_shift_up_down_estimates", up.hasEstimate && down.hasEstimate
        && finiteSnapshot(up) && finiteSnapshot(down));
    checkValue(check, "pitch_known_up_interval", up.meanMatchedDisplacementSemitones, 0.99999, 1.00001);
    checkValue(check, "pitch_known_down_interval", down.meanMatchedDisplacementSemitones, -1.00001, -0.99999);
    check("pitch_shift_sign_and_mode", up.netDisplacementSemitones > 0.5
        && down.netDisplacementSemitones < -0.5
        && std::abs(up.modeSemitones - 1.0) <= pitchHistogramStepSemitones * 0.5
        && std::abs(down.modeSemitones + 1.0) <= pitchHistogramStepSemitones * 0.5);
    check("pitch_reversal_symmetry", std::abs(up.matchedMass - down.matchedMass) < 1e-5
        && std::abs(up.netDisplacementSemitones + down.netDisplacementSemitones) < 1e-5);
    check("pitch_soft_marginal_deficits_and_excesses_accounted",
        std::abs(up.matchedMass + up.unmatchedSourceMass - up.excessSourceMass - 1.0) < 1e-6
        && std::abs(up.matchedMass + up.unmatchedTargetMass - up.excessTargetMass - 1.0) < 1e-6);
    checkValue(check, "pitch_velocity_uses_comparison_time",
        up.netSemitonesPerSecond - up.netDisplacementSemitones / up.comparisonSeconds, -1e-8, 1e-8);

    // A sub-bin displacement must not disappear: linear splatting retains its first moment.
    const auto subBin = compare(single, shifted(single, 0.08));
    double binnedMoment = 0.0;
    for (int bin = 0; bin < pitchHistogramBinCount; ++bin)
        binnedMoment += subBin.rawHistogram[bin]
            * (pitchHistogramMinimumSemitones + bin * pitchHistogramStepSemitones);
    checkValue(check, "pitch_sub_bin_histogram_preserves_moment",
        std::abs(binnedMoment - subBin.netDisplacementSemitones), 0.0, 1e-6);

    const Peaks two{{500.0, 0.5}, {4000.0, 0.5}};
    const Peaks opposed{{frequencyAt(2.0, 500.0), 0.5}, {frequencyAt(-2.0, 4000.0), 0.5}};
    const auto mixed = compare(two, opposed);
    check("pitch_opposed_modes_survive_net_cancellation", mixed.hasEstimate
        && massNear(mixed, 2.0) > 0.25 && massNear(mixed, -2.0) > 0.25
        && std::abs(mixed.netDisplacementSemitones) < 0.01);

    const Peaks dominantStationary{{440.0, 0.9}, {2000.0, 0.1}};
    const Peaks weakMoving{{440.0, 0.9}, {frequencyAt(2.0, 2000.0), 0.1}};
    const auto stationary = compare(dominantStationary, weakMoving);
    check("pitch_stationary_majority_and_weaker_motion_preserved", stationary.hasEstimate
        && massNear(stationary, 0.0, 0.01) > 0.8 && massNear(stationary, 2.0) > 0.05
        && stationary.modeSemitones == 0.0 && stationary.netDisplacementSemitones > 0.05
        && stationary.netDisplacementSemitones < 0.4);
    // Cross links between 440 and 2000 Hz are exponentially negligible here.
    // Each diagonal pair consequently has an independent closed-form KL-UOT mass.
    const PitchMotionSettings defaults = energyBaseline();
    const double denominator = 2.0 * defaults.marginalPenalty + defaults.entropy;
    const double exponent = (2.0 * defaults.marginalPenalty + 2.0 * defaults.entropy) / denominator;
    const double expectedStationaryMass = std::pow(0.9, exponent);
    const double expectedMovingMass = std::pow(0.1, exponent)
        * std::exp(-std::pow(2.0 / defaults.transportScaleSemitones, 2.0) / denominator);
    checkValue(check, "pitch_disconnected_pairs_closed_form_flux",
        std::abs(stationary.netDisplacementSemitones - 2.0 * expectedMovingMass), 0.0, 1e-6);
    checkValue(check, "pitch_disconnected_pairs_closed_form_mass",
        std::abs(stationary.matchedMass - expectedStationaryMass - expectedMovingMass), 0.0, 1e-6);
    check("pitch_disconnected_pairs_converge", stationary.solverConverged);

    const auto upperTail = compare(single, shifted(single, 14.0));
    const auto lowerTail = compare(shifted(single, 14.0), single);
    check("pitch_out_of_range_motion_retained_as_tails", upperTail.hasEstimate && lowerTail.hasEstimate
        && upperTail.rawUpperTailMass > 0.001 && upperTail.rawLowerTailMass == 0.0
        && lowerTail.rawLowerTailMass > 0.001 && lowerTail.rawUpperTailMass == 0.0);
    check("pitch_tails_not_piled_into_edge_bins", histogramMass(upperTail.rawHistogram) < 1e-6
        && histogramMass(lowerTail.rawHistogram) < 1e-6);
    checkValue(check, "pitch_histogram_and_tails_account_for_transport",
        std::abs(histogramMass(upperTail.rawHistogram) + upperTail.rawLowerTailMass
            + upperTail.rawUpperTailMass - upperTail.matchedMass), 0.0, 1e-6);

    const auto unrelated = compare(Peaks{{100.0, 1.0}}, Peaks{{9000.0, 1.0}});
    check("pitch_remote_replacement_has_unmatched_mass", finiteSnapshot(unrelated)
        && unrelated.unmatchedSourceMass > 0.95 && unrelated.unmatchedTargetMass > 0.95);
}

void runTemporalTests(const Check& check)
{
    PitchMotionSettings settings;
    settings.comparisonSeconds = 0.04;
    settings.smoothingSeconds = 0.2;
    settings.integrationSeconds = 2.0;
    settings.integratorLimitSemitones = 3.0;
    PitchMotion frequentReader, sparseReader;
    check("pitch_temporal_prepare", frequentReader.prepare(settings) == PitchMotionResult::ok
        && sparseReader.prepare(settings) == PitchMotionResult::ok);
    Snapshot previous{};
    bool previousEstimate = false, exactEma = true, exactIntegral = true, fixedLag = true;
    bool modeConvention = true, readOnlySnapshots = true;
    int estimateCount = 0;
    for (int step = 0; step <= 100; ++step)
    {
        const double time = step * 0.01;
        const double pitch = 5.0 * time;
        const Peaks peaks{{frequencyAt(pitch), 1.0}};
        frequentReader.processPeaks(peaks.data(), 1, time);
        sparseReader.processPeaks(peaks.data(), 1, time);
        const Snapshot current = frequentReader.snapshot();
        if (step % 7 == 0)
        {
            const Snapshot before = sparseReader.snapshot();
            for (int read = 0; read < 5; ++read) (void)sparseReader.snapshot();
            const auto& after = sparseReader.snapshot();
            readOnlySnapshots = readOnlySnapshots && before.rawHistogram == after.rawHistogram
                && before.smoothedHistogram == after.smoothedHistogram
                && before.integratedSemitones == after.integratedSemitones;
        }
        if (current.hasEstimate)
        {
            ++estimateCount;
            fixedLag = fixedLag && std::abs(current.comparisonSeconds - 0.04) < 1e-9
                && std::abs(current.meanMatchedDisplacementSemitones - 0.2) < 1e-7;
            modeConvention = modeConvention && std::abs(current.modeSemitones - 0.2)
                <= pitchHistogramStepSemitones * 0.5 + 1e-9;
            if (previousEstimate)
            {
                const double alpha = std::exp(-current.stepSeconds / settings.smoothingSeconds);
                for (int bin = 0; bin < pitchHistogramBinCount; ++bin)
                {
                    const double expected = alpha * previous.smoothedHistogram[bin]
                        + (1.0 - alpha) * current.rawHistogram[bin];
                    exactEma = exactEma && std::abs(current.smoothedHistogram[bin] - expected) < 2e-6;
                }
                const double integralAlpha = std::exp(-current.stepSeconds / settings.integrationSeconds);
                const double expected = std::clamp(integralAlpha * previous.integratedSemitones
                    + settings.integrationSeconds * (1.0 - integralAlpha) * current.netSemitonesPerSecond,
                    -settings.integratorLimitSemitones, settings.integratorLimitSemitones);
                exactIntegral = exactIntegral && std::abs(current.integratedSemitones - expected) < 1e-7;
            }
            previous = current;
            previousEstimate = true;
        }
    }
    check("pitch_fixed_lag_and_interval_convention", fixedLag && estimateCount >= 90);
    check("pitch_histogram_mode_uses_bin_centres", modeConvention);
    check("pitch_histogram_ema_uses_audio_step_time", exactEma);
    check("pitch_integrator_no_overlapping_lag_double_count", exactIntegral);
    check("pitch_ui_reads_do_not_advance_analysis", readOnlySnapshots
        && frequentReader.snapshot().rawHistogram == sparseReader.snapshot().rawHistogram
        && frequentReader.snapshot().smoothedHistogram == sparseReader.snapshot().smoothedHistogram
        && frequentReader.snapshot().integratedSemitones == sparseReader.snapshot().integratedSemitones);
    checkValue(check, "pitch_integrator_clamps_stored_state",
        std::abs(frequentReader.snapshot().integratedSemitones), 2.8, 3.000001);

    frequentReader.reset();
    check("pitch_explicit_reset_clears_state", !frequentReader.snapshot().hasEstimate
        && histogramMass(frequentReader.snapshot().smoothedHistogram) == 0.0
        && frequentReader.snapshot().integratedSemitones == 0.0);
    const Peaks single{{440.0, 1.0}};
    frequentReader.processPeaks(single.data(), 1, 5.0);
    check("pitch_reset_requires_a_new_reference", !frequentReader.snapshot().hasEstimate);
    frequentReader.processPeaks(single.data(), 1, 5.04);
    check("pitch_reset_reference_can_resume", frequentReader.snapshot().hasEstimate);
    frequentReader.processPeaks(nullptr, 0, 5.05);
    check("pitch_silence_invalidates_estimate", !frequentReader.snapshot().hasEstimate
        && finiteSnapshot(frequentReader.snapshot()));
    frequentReader.processPeaks(single.data(), 1, 5.06);
    check("pitch_silence_breaks_old_pairing", !frequentReader.snapshot().hasEstimate);

    const Peaks nonFinite{{std::numeric_limits<double>::quiet_NaN(), 1.0}};
    const auto bad = frequentReader.processPeaks(nonFinite.data(), 1, 5.10);
    check("pitch_nonfinite_peak_reported_and_hidden", bad != PitchMotionResult::ok
        && bad != PitchMotionResult::warmingUp && !frequentReader.snapshot().hasEstimate
        && finiteSnapshot(frequentReader.snapshot()));
    frequentReader.processPeaks(single.data(), 1, 5.11);
    check("pitch_nonfinite_breaks_reference", !frequentReader.snapshot().hasEstimate);
}

std::vector<float> glide(int count, double sampleRate, double semitonesPerSecond, bool antiPhase)
{
    std::vector<float> pcm(count * 2);
    const double exponent = std::log(2.0) * semitonesPerSecond / 12.0;
    for (int sample = 0; sample < count; ++sample)
    {
        const double time = sample / sampleRate;
        const double phase = 2.0 * pi * 1500.0
            * (exponent == 0.0 ? time : std::expm1(exponent * time) / exponent);
        const float value = static_cast<float>(0.25 * std::sin(phase));
        pcm[sample * 2] = value;
        pcm[sample * 2 + 1] = antiPhase ? -value : value;
    }
    return pcm;
}

SpectrumFrame flatPowerFrame(float background)
{
    SpectrumFrame frame;
    frame.sampleRate = 48000.0;
    frame.fftSize = 2048;
    frame.hopSize = 512;
    frame.channels = 2;
    frame.power.assign(1025, background);
    return frame;
}

SpectrumFrame gaussianPeakFrame(double frequencyHz)
{
    auto frame = flatPowerFrame(0.0f);
    const double location = frequencyHz * frame.fftSize / frame.sampleRate;
    const int first = (std::max)(1, static_cast<int>(std::floor(location)) - 3);
    const int last = (std::min)(frame.fftSize / 2 - 1, static_cast<int>(std::ceil(location)) + 3);
    for (int bin = first; bin <= last; ++bin)
        frame.power[bin] = static_cast<float>(std::exp(-2.0 * (bin - location) * (bin - location)));
    return frame;
}

Snapshot compareSpectra(SpectrumFrame source, SpectrumFrame target,
                        const PitchMotionSettings& settings)
{
    PitchMotion motion;
    motion.prepare(settings);
    // Four consecutive hops reach the comparison lag without a source gap.
    for (int step = 0; step < 4; ++step)
    {
        source.firstSample = static_cast<std::uint64_t>(step * source.hopSize);
        motion.process(source);
    }
    target.firstSample = static_cast<std::uint64_t>(4 * target.hopSize);
    motion.process(target);
    return motion.snapshot();
}

void runSpectralWeightingTests(const Check& check)
{
    const PitchMotionSettings weighted;
    const auto baseline = energyBaseline();
    check("pitch_weighting_defaults_and_baseline_valid", validSpectralWeightingSettings(weighted)
        && validSpectralWeightingSettings(baseline)
        && weighted.noiseFloorStrength == 1.0 && weighted.minimumPeakProminenceDb == 6.0
        && weighted.peakWeightExponent == 2.0);
    {
        PitchMotion motion;
        auto invalid = weighted;
        invalid.noiseFloorStrength = std::numeric_limits<double>::quiet_NaN();
        check("pitch_weighting_rejects_nonfinite_floor", motion.prepare(invalid) == PitchMotionResult::invalidConfiguration);
        invalid = weighted;
        invalid.minimumPeakProminenceDb = -1.0;
        check("pitch_weighting_rejects_negative_prominence", motion.prepare(invalid) == PitchMotionResult::invalidConfiguration);
        invalid = weighted;
        invalid.peakWeightExponent = 0.5;
        check("pitch_weighting_rejects_compressive_exponent", motion.prepare(invalid) == PitchMotionResult::invalidConfiguration);
    }
    {
        auto frame = flatPowerFrame(0.01f);
        const double floor = frame.power[40];
        frame.power[63] = frame.power[65] = 0.26f;
        frame.power[64] = 1.01f;
        checkValue(check, "pitch_noise_floor_excludes_hann_main_lobe",
            std::abs(spectralNoiseFloor(frame, 64) - floor), 0.0, 1e-15);
        checkValue(check, "pitch_clean_power_subtracts_local_background",
            std::abs(cleanSpectralPower(frame, 64, weighted) - (static_cast<double>(frame.power[64]) - floor)), 0.0, 1e-12);
        frame.power[40] = 0.03f;
        check("pitch_prominence_rejects_weak_background_maximum", cleanSpectralPower(frame, 40, weighted) == 0.0);
        check("pitch_cleaning_baseline_is_exact_bypass", cleanSpectralPower(frame, 40, baseline)
            == static_cast<double>(frame.power[40]));
        auto louder = frame;
        for (auto& power : louder.power) power *= 16.0f;
        checkValue(check, "pitch_clean_power_global_gain_covariance",
            std::abs(cleanSpectralPower(louder, 64, weighted) / 16.0 - cleanSpectralPower(frame, 64, weighted)), 0.0, 1e-12);
        // Fourteen of twenty-four outer bins may be harmonic structure. The
        // lower quartile must still recover the ten unoccupied floor samples.
        for (int offset = 3; offset <= 9; ++offset)
            frame.power[64 - offset] = frame.power[64 + offset] = 100.0f;
        checkValue(check, "pitch_noise_floor_resists_dense_outer_partials",
            std::abs(spectralNoiseFloor(frame, 64) - floor), 0.0, 1e-15);
        const auto flat = flatPowerFrame(0.01f);
        bool cleared = true;
        for (int bin : {1, 2, 64, 1023})
            cleared = cleared && cleanSpectralPower(flat, bin, weighted) == 0.0;
        check("pitch_flat_background_removed_including_frequency_edges", cleared);
    }
    {
        Peaks source{{100.0, 1.0}};
        for (int peak = 0; peak < 63; ++peak)
            source.push_back({400.0 * std::pow(9000.0 / 400.0, peak / 62.0), 0.02});
        auto target = source;
        target.front().frequencyHz = frequencyAt(2.0, 100.0);
        const auto energy = compare(source, target, baseline);
        const auto dominant = compare(source, target, weighted);
        check("pitch_power_exponent_preserves_dominant_motion_against_many_weak_peaks",
            energy.hasEstimate && dominant.hasEstimate && finiteSnapshot(energy) && finiteSnapshot(dominant)
            && massNear(energy, 2.0, 0.01) < 0.5 && massNear(dominant, 2.0, 0.01) > 0.85
            && dominant.modeSemitones == 2.0 && dominant.sourcePeakCount == 64);
        const double inputMass = 1.0 / (1.0 + 63.0 * 0.02 * 0.02);
        const double denominator = 2.0 * weighted.marginalPenalty + weighted.entropy;
        const double expectedMass = std::pow(inputMass,
            (2.0 * weighted.marginalPenalty + 2.0 * weighted.entropy) / denominator)
            * std::exp(-std::pow(2.0 / weighted.transportScaleSemitones, 2.0) / denominator);
        checkValue(check, "pitch_weight_exponent_matches_isolated_transport_oracle",
            std::abs(massNear(dominant, 2.0, 0.01) - expectedMass), 0.0, 1e-6);
        const auto gained = compare(shifted(source, 0.0, 1000.0), shifted(target, 0.0, 1000.0), weighted);
        check("pitch_dominance_exponent_retains_global_gain_invariance", gained.hasEstimate
            && histogramDifference(dominant, gained) < 1e-6
            && std::abs(dominant.netDisplacementSemitones - gained.netDisplacementSemitones) < 1e-6);
    }
    {
        auto source = flatPowerFrame(0.01f);
        for (int bin = 8; bin < 425; bin += 9)
            if (std::abs(bin - 64) > 3 && std::abs(bin - 72) > 3) source.power[bin] = 0.03f;
        auto target = source;
        source.power[63] = source.power[65] = 0.26f;
        source.power[64] = 1.01f;
        target.power[71] = target.power[73] = 0.26f;
        target.power[72] = 1.01f;
        const auto energy = compareSpectra(source, target, baseline);
        const auto cleaned = compareSpectra(source, target, weighted);
        const double displacement = 12.0 * std::log2(72.0 / 64.0);
        check("pitch_fft_cleaning_keeps_dominant_peak_and_rejects_noise_maxima",
            energy.hasEstimate && cleaned.hasEstimate && finiteSnapshot(cleaned)
            && energy.sourcePeakCount > 16 && cleaned.sourcePeakCount == 1 && cleaned.targetPeakCount == 1
            && massNear(cleaned, displacement, 0.26) > 0.8);
        checkValue(check, "pitch_fft_cleaning_preserves_known_frequency_movement",
            std::abs(cleaned.meanMatchedDisplacementSemitones - displacement), 0.0, 1e-5);
        auto louderSource = source, louderTarget = target;
        for (auto& power : louderSource.power) power *= 16.0f;
        for (auto& power : louderTarget.power) power *= 16.0f;
        const auto louder = compareSpectra(louderSource, louderTarget, weighted);
        check("pitch_fft_cleaning_retains_global_gain_invariance", louder.hasEstimate
            && histogramDifference(cleaned, louder) < 1e-6
            && std::abs(cleaned.meanMatchedDisplacementSemitones - louder.meanMatchedDisplacementSemitones) < 1e-6);
    }
    {
        const auto lower = compareSpectra(gaussianPeakFrame(55.0), gaussianPeakFrame(frequencyAt(1.0, 55.0)), weighted);
        check("pitch_fft_refined_fundamental_above_lower_bound_is_retained", lower.hasEstimate
            && lower.sourcePeakCount == 1 && lower.targetPeakCount == 1);
        checkValue(check, "pitch_fft_lower_edge_sub_bin_interval",
            std::abs(lower.meanMatchedDisplacementSemitones - 1.0), 0.0, 0.002);
        const auto upper = compareSpectra(gaussianPeakFrame(9990.0), gaussianPeakFrame(9999.0), weighted);
        check("pitch_fft_refined_peak_below_upper_bound_is_retained", upper.hasEstimate
            && upper.sourcePeakCount == 1 && upper.targetPeakCount == 1);
        checkValue(check, "pitch_fft_upper_edge_sub_bin_interval",
            std::abs(upper.meanMatchedDisplacementSemitones - 12.0 * std::log2(9999.0 / 9990.0)), 0.0, 0.002);
        const auto excluded = compareSpectra(gaussianPeakFrame(49.0), gaussianPeakFrame(49.5), weighted);
        check("pitch_fft_refined_frequency_outside_requested_range_is_excluded", !excluded.hasEstimate);
    }
}
void runPcmTests(const Check& check)
{
    {
        // Independent energy-partition oracle, with exactly representable powers.
        // Two symmetric maxima share bin 65. Their unique powers are 1.75 and
        // 1.50, hence normalized weights 7/13 and 6/13. Counting the valley
        // twice would instead yield 8/15 and 7/15 and miss the identity plan.
        SpectrumFrame frame;
        frame.sampleRate = 48000.0;
        frame.fftSize = 2048;
        frame.hopSize = 512;
        frame.channels = 2;
        frame.power.assign(1025, 0.0f);
        frame.power[63] = frame.power[65] = frame.power[67] = 0.5f;
        frame.power[64] = 1.0f;
        frame.power[66] = 0.75f;
        PitchMotion partition;
        partition.prepare(energyBaseline());
        partition.process(frame);
        const Peaks expected{{1500.0, 7.0}, {1546.875, 6.0}};
        partition.processPeaks(expected.data(), 2, frame.centreTimeSeconds() + 0.04);
        const auto& snapshot = partition.snapshot();
        check("pitch_shared_fft_valley_counted_once", snapshot.hasEstimate
            && snapshot.sourcePeakCount == 2 && snapshot.targetPeakCount == 2
            && snapshot.solverIterations == 0 && snapshot.solverConverged
            && std::abs(snapshot.matchedMass - 1.0) < 1e-10
            && std::abs(snapshot.netDisplacementSemitones) < 1e-10);
    }
    double measuredRates[2]{};
    int rateIndex = 0;
    for (double sampleRate : {44100.0, 48000.0})
    {
        AudioAnalyzer analyzer;
        const bool prepared = analyzer.prepare({sampleRate, 2, 2048, 512}) == AnalysisResult::ok;
        const int sampleCount = static_cast<int>(sampleRate);
        const auto pcm = glide(sampleCount, sampleRate, 5.0, false);
        Frames frames;
        const bool processed = prepared && analyzer.processBlock(pcm.data(), sampleCount, 0, frames)
            == AnalysisResult::ok;
        PitchMotion motion;
        motion.prepare({});
        bool finite = true, consistentTime = true;
        double velocitySum = 0.0;
        int estimates = 0;
        for (const auto& frame : frames.frames)
        {
            motion.process(frame);
            const auto& snapshot = motion.snapshot();
            finite = finite && finiteSnapshot(snapshot);
            if (snapshot.hasEstimate && frame.centreTimeSeconds() > 0.15)
            {
                velocitySum += snapshot.meanMatchedDisplacementSemitones / snapshot.comparisonSeconds;
                ++estimates;
                const double lagSamples = snapshot.comparisonSeconds * sampleRate;
                consistentTime = consistentTime
                    && std::abs(lagSamples / frame.hopSize - std::round(lagSamples / frame.hopSize)) < 1e-6;
            }
        }
        const double velocity = estimates > 0 ? velocitySum / estimates : 0.0;
        measuredRates[rateIndex++] = velocity;
        const std::string tag = sampleRate == 44100.0 ? "44100" : "48000";
        check(("pitch_pcm_glide_processed_" + tag).c_str(), processed && finite && estimates > 50);
        check(("pitch_pcm_lag_in_sample_units_" + tag).c_str(), consistentTime);
        checkValue(check, ("pitch_pcm_glide_velocity_" + tag).c_str(), velocity, 4.5, 5.5);

        // Opposite channel polarity must leave mean channel power, hence motion, unchanged.
        analyzer.reset();
        const auto anti = glide(sampleCount, sampleRate, 5.0, true);
        Frames antiFrames;
        analyzer.processBlock(anti.data(), sampleCount, 0, antiFrames);
        PitchMotion antiMotion;
        antiMotion.prepare({});
        for (const auto& frame : antiFrames.frames) antiMotion.process(frame);
        check(("pitch_pcm_stereo_antiphase_preserved_" + tag).c_str(), !antiFrames.frames.empty()
            && histogramDifference(motion.snapshot(), antiMotion.snapshot()) < 1e-6
            && std::abs(motion.snapshot().integratedSemitones - antiMotion.snapshot().integratedSemitones) < 1e-8);

        if (!frames.frames.empty())
        {
            auto malformed = frames.frames.back();
            malformed.firstSample += 100000;
            motion.process(malformed);
            check(("pitch_pcm_discontinuity_resets_reference_" + tag).c_str(), !motion.snapshot().hasEstimate);
            malformed.sampleRate = sampleRate == 44100.0 ? 48000.0 : 44100.0;
            malformed.firstSample += malformed.hopSize;
            motion.process(malformed);
            check(("pitch_pcm_format_change_resets_reference_" + tag).c_str(), !motion.snapshot().hasEstimate);
            malformed.firstSample += malformed.hopSize;
            malformed.power[3] = std::numeric_limits<float>::quiet_NaN();
            const auto result = motion.process(malformed);
            check(("pitch_pcm_nonfinite_power_hidden_" + tag).c_str(), result != PitchMotionResult::ok
                && !motion.snapshot().hasEstimate && finiteSnapshot(motion.snapshot()));
        }
    }
    checkValue(check, "pitch_pcm_sample_rate_agreement", std::abs(measuredRates[0] - measuredRates[1]), 0.0, 0.25);
}
}

void runPitchMotionTests(const Check& check)
{
    runPeakTransportTests(check);
    runSpectralWeightingTests(check);
    runTemporalTests(check);
    runPcmTests(check);
}

}
