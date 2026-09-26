// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../AudioAnalyzer.h"
#include "../runtime/AnalysisStream.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <thread>

using namespace fxanalysis;
constexpr double pi = 3.14159265358979323846;
struct Frames final : SpectrumSink
{
    std::vector<SpectrumFrame> frames;
    void onSpectrum(const SpectrumFrame& f) override { frames.push_back(f); }
};
std::vector<float> tone(int count, double frequency, float rightGain = 1.0f, double sampleRate = 48000.0)
{
    std::vector<float> pcm(count * 2);
    for (int i = 0; i < count; ++i)
    {
        pcm[i * 2] = 0.25f * static_cast<float>(std::sin(2.0 * pi * frequency * i / sampleRate));
        pcm[i * 2 + 1] = pcm[i * 2] * rightGain;
    }
    return pcm;
}
int main()
{
    int failures = 0;
    bool comma = false;
    std::cout << "{\"tests\":[";
    const auto check = [&](const char* name, bool passed)
    {
        if (comma) std::cout << ',';
        comma = true;
        std::cout << "{\"name\":\"" << name << "\",\"passed\":" << (passed ? "true" : "false") << '}';
        if (!passed) ++failures;
    };
    AudioAnalyzer analyzer;
    Frames output;
    check("unprepared_status", analyzer.processBlock(nullptr, 0, 0, output) == AnalysisResult::notPrepared);
    check("invalid_configuration", analyzer.prepare({48000, 3, 4096, 512}) == AnalysisResult::invalidConfiguration
        && analyzer.prepare({48000, 2, 1000, 512}) == AnalysisResult::invalidConfiguration
        && analyzer.prepare({48000, 2, 4096, 0}) == AnalysisResult::invalidConfiguration);
    check("prepare", analyzer.prepare({48000, 2, 4096, 512}) == AnalysisResult::ok);
    auto pcm = tone(12288, 1500.0);
    const auto original = pcm;
    check("process", analyzer.processBlock(pcm.data(), 12288, 0, output) == AnalysisResult::ok);
    check("input_unchanged", pcm == original);
    check("window_count_and_time", output.frames.size() == 17 && output.frames[0].firstSample == 0
        && output.frames.back().firstSample == 8192 && std::abs(output.frames[0].centreTimeSeconds() - 2048.0 / 48000.0) < 1e-12);
    check("bin_amplitude", std::abs(output.frames[0].power[128] - 0.0625f) < 2e-6f);
    Frames chopped;
    analyzer.reset();
    int pos = 0;
    while (pos < 12288)
    {
        const int count = std::min(12288 - pos, 37 + pos % 743);
        analyzer.processBlock(pcm.data() + pos * 2, count, pos, chopped);
        pos += count;
    }
    bool equal = chopped.frames.size() == output.frames.size();
    for (int i = 0; equal && i < static_cast<int>(output.frames.size()); ++i)
        equal = output.frames[i].power == chopped.frames[i].power
            && output.frames[i].barLeft == chopped.frames[i].barLeft
            && output.frames[i].barRight == chopped.frames[i].barRight
            && output.frames[i].firstSample == chopped.frames[i].firstSample;
    check("independent_of_input_block_sizes", equal);
    Frames opposite;
    analyzer.reset();
    auto anti = tone(4096, 1500.0, -1.0f);
    analyzer.processBlock(anti.data(), 4096, 0, opposite);
    check("stereo_antiphase_preserved", opposite.frames[0].power == output.frames[0].power
        && std::abs(opposite.frames[0].left[128] + opposite.frames[0].right[128]) < 1e-6f);
    Frames oneSide;
    analyzer.reset();
    auto left = tone(4096, 1500.0, 0.0f);
    analyzer.processBlock(left.data(), 4096, 0, oneSide);
    check("channel_power_mean", std::abs(oneSide.frames[0].power[128] - 0.03125f) < 1e-6f);
    check("bar_stereo_left_only", *std::max_element(oneSide.frames[0].barLeft.begin(), oneSide.frames[0].barLeft.end()) > 0.01f
        && *std::max_element(oneSide.frames[0].barRight.begin(), oneSide.frames[0].barRight.end()) == 0.0f);
    Frames rightSide;
    analyzer.reset();
    for (int i = 0; i < 4096; ++i) std::swap(left[i * 2], left[i * 2 + 1]);
    analyzer.processBlock(left.data(), 4096, 0, rightSide);
    check("bar_stereo_swap", rightSide.frames[0].barRight == oneSide.frames[0].barLeft
        && rightSide.frames[0].barLeft == oneSide.frames[0].barRight);
    check("bar_antiphase_preserved", opposite.frames[0].barLeft == output.frames[0].barLeft
        && opposite.frames[0].barRight == output.frames[0].barRight);
    Frames silent;
    analyzer.reset();
    std::vector<float> zeros(8192, 0.0f);
    analyzer.processBlock(zeros.data(), 4096, 0, silent);
    check("silence", *std::max_element(silent.frames[0].bandDb.begin(), silent.frames[0].bandDb.end()) <= -119.9f);
    check("null_block_status", analyzer.processBlock(nullptr, 1, 0, silent) == AnalysisResult::invalidBlock);
    pcm[0] = std::numeric_limits<float>::quiet_NaN();
    check("non_finite_status", analyzer.processBlock(pcm.data(), 1, 0, silent) == AnalysisResult::nonFiniteInput);
    Frames gap;
    analyzer.reset();
    analyzer.processBlock(original.data(), 3000, 0, gap);
    analyzer.processBlock(original.data(), 4096, 10000, gap);
    check("gap_resets_window", gap.frames.size() == 1 && gap.frames[0].firstSample == 10000);
    check("gap_resets_bar_history", gap.frames[0].barLeft == output.frames[0].barLeft);
    Frames changed;
    check("change_rate_and_mono", analyzer.prepare({44100, 1, 1024, 256}) == AnalysisResult::ok);
    std::vector<float> mono(1024, 0.1f);
    analyzer.processBlock(mono.data(), 1024, 0, changed);
    check("mono_dc_and_metadata", changed.frames[0].channels == 1 && changed.frames[0].sampleRate == 44100
        && std::abs(changed.frames[0].power[0] - 0.01f) < 1e-6f);

    // Independent O(N^2) reference checks the complex transform and window scaling.
    analyzer.prepare({48000, 1, 64, 16});
    Frames dftFrames;
    std::vector<float> random(64);
    for (int i = 0; i < 64; ++i) random[i] = static_cast<float>((i * 37 % 101) - 50) / 100.0f;
    analyzer.processBlock(random.data(), 64, 0, dftFrames);
    double maxError = 0;
    for (int k = 0; k <= 32; ++k)
    {
        std::complex<double> expected{};
        for (int i = 0; i < 64; ++i)
            expected += static_cast<double>(random[i]) * (0.5 - 0.5 * std::cos(2.0 * pi * i / 64))
                * std::polar(1.0, -2.0 * pi * k * i / 64);
        expected *= (k == 0 || k == 32 ? 1.0 : 2.0) / 32.0;
        maxError = std::max(maxError, std::abs(expected - static_cast<std::complex<double>>(dftFrames.frames[0].left[k])));
    }
    check("fft_against_direct_dft", maxError < 1e-6);

    // Behavioral checks on display amplitudes, independent of FFT leakage.
    {
        BarDynamics dynamics;
        const float own = BarSettings::currentChannelGain;
        const float cross = BarSettings::oppositeChannelGain;
        // Exercise envelopes below the display ceiling even with user-tuned gain.
        const float inputScale = 1.0f / (std::max)(1.0f, BarSettings::displayGain);
        const auto process = [inputScale](BarDynamics& target, float left, float right, const BarCoefficients& c)
        { return target.process(left * inputScale, right * inputScale, c); };
        const float gain = BarSettings::displayGain * inputScale;
        const float direct = BarSettings::directMix;
        const float singleGain = gain * own;
        const auto retention = BarCoefficients::forHop(0.01);
        const int settleSteps = static_cast<int>(std::ceil(18.0
            * (BarSettings::decaySeconds + BarSettings::peakRetentionSeconds) / 0.01));
        // Continuous two-pole release, independent of the discrete implementation.
        const auto envelope = [](double t)
        {
            const double d = BarSettings::decaySeconds, p = BarSettings::peakRetentionSeconds;
            if (p == 0.0) return std::exp(-t / d);
            if (d == p) return (1.0 + t / d) * std::exp(-t / d);
            return (d * std::exp(-t / d) - p * std::exp(-t / p)) / (d - p);
        };
        auto value = process(dynamics, 0.5f, 0.0f, retention);
        check("bar_attack_follows_rise", std::abs(value.left - 0.5f * singleGain) < 1e-6f && value.right == 0.0f);
        for (int i = 0; i < 100; ++i) value = process(dynamics, 0.5f, 0.0f, retention);
        check("bar_constant_decays_to_direct_floor", std::abs(value.left - 0.5 * singleGain
            * (direct + (1.0 - direct) * envelope(1.0))) < 2e-6);
        for (int i = 0; i < settleSteps; ++i) value = process(dynamics, 0.5f, 0.0f, retention);
        check("bar_constant_keeps_configured_direct_mix", std::abs(value.left - 0.5f * singleGain * direct) < 1e-6f && value.right == 0.0f);
        value = process(dynamics, 0.7f, 0.0f, retention);
        check("bar_new_rise_after_plateau", std::abs(value.left - singleGain * (direct * 0.7f + (1.0f - direct) * 0.2f)) < 1e-6f);
        value = process(dynamics, 0.1f, 0.0f, retention);
        check("bar_fall_preserves_attack_tail", std::abs(value.left
            - singleGain * (direct * 0.1 + (1.0 - direct) * 0.2 * envelope(0.01))) < 2e-6 && value.right == 0.0f);
        const float fallen = value.left;
        value = process(dynamics, 0.3f, 0.0f, retention);
        check("bar_reattack_accumulates_positive_rises", value.left > fallen
            && value.left > singleGain * (direct * 0.3f + (1.0f - direct) * 0.2f));
        value = process(dynamics, 0.0f, 0.0f, retention);
        check("bar_note_off_keeps_release", value.left > 0.0f && value.right == 0.0f);
        for (int i = 0; i < settleSteps; ++i) value = process(dynamics, 0.0f, 0.0f, retention);
        check("bar_eventually_returns_to_silence", value.left == 0.0f && value.right == 0.0f);
        dynamics.reset();
        value = process(dynamics, 0.5f, 0.2f, retention);
        check("bar_stereo_matrix_and_negative_clip", std::abs(value.left
            - std::clamp(gain * (own * 0.5f - cross * 0.2f), 0.0f, 1.0f)) < 1e-6f
            && std::abs(value.right - std::clamp(gain * (own * 0.2f - cross * 0.5f), 0.0f, 1.0f)) < 1e-6f);
        dynamics.reset();
        value = process(dynamics, 0.5f, 0.5f, retention);
        check("bar_center_retained", std::abs(value.left - gain * 0.5f * (own - cross)) < 1e-6f && value.left == value.right);
        dynamics.reset();
        value = process(dynamics, 1.0f, 0.0f, retention);
        check("bar_output_bounded", value.left == 1.0f && value.right == 0.0f);
        BarDynamics slower;
        const auto slowerRetention = BarCoefficients::forHop(0.02);
        dynamics.reset();
        process(dynamics, 0.5f, 0.0f, retention);
        process(slower, 0.5f, 0.0f, slowerRetention);
        for (int i = 0; i < 50; ++i) value = process(dynamics, 0.5f, 0.0f, retention);
        StereoBar slowerValue;
        for (int i = 0; i < 25; ++i) slowerValue = process(slower, 0.5f, 0.0f, slowerRetention);
        check("bar_decay_independent_of_hop", std::abs(value.left - slowerValue.left) < 2e-6f);

        dynamics.reset();
        process(dynamics, 0.5f, 0.0f, retention);
        bool monotoneRelease = true;
        float last = 0.5f * singleGain;
        for (int i = 1; i <= 100; ++i)
        {
            value = process(dynamics, 0.5f, 0.0f, retention);
            monotoneRelease = monotoneRelease && value.left <= last + 1e-6f
                && std::abs(value.left - 0.5 * singleGain * (direct + (1.0 - direct) * envelope(i * 0.01))) < 2e-6 + BarSettings::displayGain * BarSettings::silenceThreshold;
            last = value.left;
        }
        check("bar_rounded_peak_release_matches_continuous_solution", monotoneRelease);
        check("bar_peak_retention_exceeds_single_pole", envelope(0.1) >= std::exp(-0.1 / BarSettings::decaySeconds));
        // A 2 Hz tremolo can accumulate attacks by design, but must remain bounded
        // and clear all states after silence. It must not leak to the other side.
        bool tremoloBounded = true;
        dynamics.reset();
        for (int i = 0; i < 1000; ++i)
        {
            value = process(dynamics, 0.2f + 0.1f * static_cast<float>(std::sin(2.0 * pi * 2.0 * i * 0.01)), 0.0f, retention);
            tremoloBounded = tremoloBounded && std::isfinite(value.left) && value.left >= 0.0f
                && value.left <= 1.0f && value.right == 0.0f;
        }
        for (int i = 0; i < settleSteps; ++i) value = process(dynamics, 0.0f, 0.0f, retention);
        check("bar_tremolo_bounded_and_clears", tremoloBounded && value.left == 0.0f);
        dynamics.reset();
        process(dynamics, 0.2f, 0.0f, retention);
        value = process(dynamics, 0.7f, 0.0f, retention);
        check("bar_new_peak_captured_without_smoothing_delay", std::abs(value.left - singleGain * (direct * 0.7
            + (1.0 - direct) * (0.5 + 0.2 * std::exp(-0.01 / BarSettings::decaySeconds)))) < 2e-6);
        for (double peakSeconds : {0.0, BarSettings::decaySeconds})
        {
            const auto special = BarCoefficients::forHop(0.01, BarSettings::decaySeconds, peakSeconds);
            dynamics.reset();
            process(dynamics, 0.5f, 0.0f, special);
            for (int i = 0; i < 50; ++i) value = process(dynamics, 0.5f, 0.0f, special);
            const double t = 0.5 / BarSettings::decaySeconds;
            const double expected = std::exp(-t) * (peakSeconds == 0.0 ? 1.0 : 1.0 + t);
            check(peakSeconds == 0.0 ? "bar_peak_retention_bypass" : "bar_equal_release_time_constants",
                std::abs(value.left - 0.5 * singleGain * (direct + (1.0 - direct) * expected)) < 2e-6);
        }

        // An attack that remains below the opposite channel's rejection threshold
        // must stay hidden. Filtering each raw channel first violates this case.
        dynamics.reset();
        const float threshold = cross / own;
        for (int i = 0; i < settleSteps; ++i) process(dynamics, 0.5f * threshold, 1.0f, retention);
        value = process(dynamics, 0.9f * threshold, 1.0f, retention);
        check("bar_separation_rejects_attack_below_opposite_level", value.left == 0.0f);

        // Crossing zero starts from the rectified level, with no negative debt.
        value = process(dynamics, 1.5f * threshold, 1.0f, retention);
        check("bar_attack_starts_at_separated_zero", std::abs(value.left - gain * 0.5f * cross) < 1e-6f);

        // A falling opposite channel reveals a rise in the separated envelope,
        // even when the current channel itself is steady.
        dynamics.reset();
        for (int i = 0; i < settleSteps; ++i) process(dynamics, 0.5f, 0.6f, retention);
        value = process(dynamics, 0.5f, 0.2f, retention);
        const float revealedLevel = own * 0.5f - cross * 0.2f;
        const float revealedAttack = cross * 0.4f;
        check("bar_opposite_fall_reveals_attack", std::abs(value.left
            - gain * (direct * revealedLevel + (1.0f - direct) * revealedAttack)) < 1e-6f);

        dynamics.reset();
        for (int i = 0; i < settleSteps; ++i) process(dynamics, 1.0f, 0.0f, retention);
        value = process(dynamics, 0.9f, 0.0f, retention);
        check("bar_separation_keeps_headroom_before_detection", std::abs(value.left
            - std::clamp(singleGain * 0.9f * direct, 0.0f, 1.0f)) < 1e-6f);

        dynamics.reset();
        value = process(dynamics, 0.0f, 0.0f, retention);
        check("bar_reset_clears_separated_history", value.left == 0.0f && value.right == 0.0f);
    }
    {
        analyzer.prepare({48000, 2, 4096, 512});
        const int sustainedCount = static_cast<int>(std::ceil(8.0 * BarSettings::decaySeconds)) * 48000;
        auto sustained = tone(sustainedCount, 1500.0);
        for (auto& value : sustained) value *= 0.04f; // -40 dBFS: visible with the current floor, below display saturation.
        Frames sustainedFrames;
        analyzer.processBlock(sustained.data(), sustainedCount, 0, sustainedFrames);
        const auto& first = sustainedFrames.frames.front();
        const auto& last = sustainedFrames.frames.back();
        const float attack = *std::max_element(first.barLeft.begin(), first.barLeft.end());
        const float plateau = *std::max_element(last.barLeft.begin(), last.barLeft.end());
        check("sustained_tone_keeps_direct_floor_and_spectrum", std::abs(last.power[128] - 1.0e-4f) < 1.0e-9f
            && attack > 0.01f && std::abs(plateau / attack - BarSettings::directMix) < 0.002f);
    }

    // A window-centred impulse has flat interior FFT power. A normalized
    // projection must preserve it, whatever the band width or input format.
    bool flatProjection = true, finiteEdges = true, reprepareStable = true;
    for (const auto cfg : {AnalyzerConfig{48000, 1, 4096, 512}, AnalyzerConfig{44100, 1, 4096, 512},
                           AnalyzerConfig{8000, 1, 64, 16}, AnalyzerConfig{384000, 1, 64, 64},
                           AnalyzerConfig{48000, 1, 32768, 1024}})
    {
        analyzer.prepare(cfg);
        std::vector<float> impulse(cfg.fftSize, 0.0f);
        impulse[cfg.fftSize / 2] = 1.0f;
        Frames flat;
        analyzer.processBlock(impulse.data(), cfg.fftSize, 0, flat);
        const double expectedDb = 10.0 * std::log10(16.0 / (cfg.fftSize * static_cast<double>(cfg.fftSize)));
        const auto& f = flat.frames.front();
        for (int b = 0; b < displayBandCount; ++b)
        {
            finiteEdges = finiteEdges && std::isfinite(f.bandDb[b])
                && f.bandDb[b] <= expectedDb + 0.001 && f.bandDb[b] >= expectedDb - 6.021;
            // Endpoints have half the amplitude scaling. Exclude their support
            // here; their convex bounds are checked above, including coarse FFTs.
            const double sigma = std::hypot(BandSettings::sigmaBandSpacing * (f.bandEdgesHz[b + 1] - f.bandEdgesHz[b]),
                                            BandSettings::minimumSigmaBins * cfg.sampleRate / cfg.fftSize);
            if (f.bandHz[b] > BandSettings::supportSigma * sigma + cfg.sampleRate / cfg.fftSize
                && f.bandHz[b] + BandSettings::supportSigma * sigma < cfg.sampleRate * 0.5 - cfg.sampleRate / cfg.fftSize)
                flatProjection = flatProjection && std::abs(f.bandDb[b] - expectedDb) < 0.001;
        }
        AudioAnalyzer fresh;
        fresh.prepare(cfg);
        Frames freshFrames;
        fresh.processBlock(impulse.data(), cfg.fftSize, 0, freshFrames);
        reprepareStable = reprepareStable && f.bandDb == freshFrames.frames.front().bandDb;
    }
    check("smooth_projection_preserves_flat_power", flatProjection);
    check("smooth_projection_finite_at_format_limits", finiteEdges);
    check("smooth_projection_reprepare_matches_fresh", reprepareStable);

    // Smooth overlap: a narrow tone must not disappear between band centres.
    // Follow its peak through both former low-bin transitions and high grouping.
    bool peakTracks = true, noHoles = true;
    double maximumSweepStepDb = 0.0, maximumStepHz = 0.0, minimumSweepPeakDb = 0.0;
    double maximumHighResStepDb = 0.0;
    for (double rate : {44100.0, 48000.0})
    {
        analyzer.prepare({rate, 2, 2048, 512});
        int previousPeak = -1;
        double previousPeakDb = 0.0, previousHighResDb = 0.0;
        for (double frequency = 40.0; frequency < 19000.0; frequency *= 1.01)
        {
            analyzer.reset();
            auto signal = tone(2048, frequency, 1.0f, rate);
            Frames sweep;
            analyzer.processBlock(signal.data(), 2048, 0, sweep);
            const auto& f = sweep.frames.front();
            const int peak = static_cast<int>(std::max_element(f.bandDb.begin(), f.bandDb.end()) - f.bandDb.begin());
            peakTracks = peakTracks && peak >= previousPeak
                && std::abs(f.bandHz[peak] - frequency) < std::max(rate / 2048.0, frequency * 0.08);
            if (previousPeak >= 0 && std::abs(f.bandDb[peak] - previousPeakDb) > maximumSweepStepDb)
            {
                maximumSweepStepDb = std::abs(f.bandDb[peak] - previousPeakDb);
                maximumStepHz = frequency;
            }
            double highResDb = -120.0;
            for (int band = 0; band < displayBandCount; ++band)
                highResDb = std::max(highResDb, static_cast<double>(f.bandFloorDb[band] * (1.0f - f.bandLevelLeft[band])));
            if (previousPeak >= 0)
                maximumHighResStepDb = std::max(maximumHighResStepDb, std::abs(highResDb - previousHighResDb));
            previousHighResDb = highResDb;
            minimumSweepPeakDb = std::min(minimumSweepPeakDb, static_cast<double>(f.bandDb[peak]));
            // Wider compressed treble bands average more FFT bins, so a fixed
            // sine has lower mean power there. Keep a -37 dB floor for this 0.25 tone.
            noHoles = noHoles && f.bandDb[peak] > -37.0f;
            previousPeak = peak;
            previousPeakDb = f.bandDb[peak];
        }
    }
    check("smooth_projection_tone_peak_tracks_frequency", peakTracks);
    // Sample the revised treble transition more finely: under 1 dB per 1% frequency step.
    check("smooth_projection_no_sweep_holes", noHoles && maximumSweepStepDb < 1.0);
    check("order_four_sweep_has_no_holes", maximumHighResStepDb < 0.7);

    {
        bool flatUnchanged = true;
        for (float level : {0.0f, 0.15f, 0.5f, 1.0f})
        {
            BandLevels flat;
            flat.fill(level);
            const auto enhanced = enhanceSpectralContrast(flat);
            for (float value : enhanced) flatUnchanged = flatUnchanged && std::abs(value - level) < 1e-6f;
        }
        check("spectral_contrast_preserves_flat_profiles_and_silence", flatUnchanged);

        BandLevels peak;
        peak.fill(0.2f);
        peak[50] = 0.8f;
        const auto before = peak;
        const auto enhanced = enhanceSpectralContrast(peak);
        check("spectral_contrast_raises_peak_and_deepens_neighbours", enhanced[50] > peak[50]
            && enhanced[49] < peak[49] && enhanced[51] < peak[51] && std::abs(enhanced[40] - peak[40]) < 1e-6f);
        check("spectral_contrast_preserves_input", peak == before);
        check("spectral_contrast_zero_gain_is_exact_bypass", enhanceSpectralContrast(peak, 0.0f) == peak);
        check("spectral_contrast_retains_headroom", enhanced[50] > 1.0f);

        BandLevels isolated{};
        isolated[50] = 1.0f;
        const auto isolatedResult = enhanceSpectralContrast(isolated);
        check("spectral_contrast_clips_negative_lobes", isolatedResult[49] == 0.0f && isolatedResult[51] == 0.0f
            && *std::min_element(isolatedResult.begin(), isolatedResult.end()) >= 0.0f);
        isolated = {};
        isolated.front() = 1.0f;
        const auto bass = enhanceSpectralContrast(isolated);
        std::reverse(isolated.begin(), isolated.end());
        auto treble = enhanceSpectralContrast(isolated);
        check("spectral_contrast_does_not_wrap_edges", bass.back() == 0.0f && treble.front() == 0.0f);
        std::reverse(treble.begin(), treble.end());
        check("spectral_contrast_symmetric_edges", bass == treble);

        BandLevels slope;
        for (int b = 0; b < displayBandCount; ++b) slope[b] = 0.2f + 0.004f * b;
        const auto slopeResult = enhanceSpectralContrast(slope);
        bool slopeUnchanged = true;
        for (int b = 2; b < displayBandCount - 2; ++b)
            slopeUnchanged = slopeUnchanged && std::abs(slopeResult[b] - slope[b]) < 1e-6f;
        check("spectral_contrast_preserves_interior_linear_slope", slopeUnchanged);
    }

    // Integration: a centred stereo tone must be accentuated on its very first
    // frame, while the raw levels remain the non-accentuated reference.
    analyzer.prepare({48000, 2, 4096, 512});
    auto contrastTone = tone(4096, 1500.0);
    for (auto& value : contrastTone) value *= 0.08f; // Keep the contrast visible, below the display ceiling.
    Frames contrastFrames;
    analyzer.processBlock(contrastTone.data(), 4096, 0, contrastFrames);
    const auto& contrastFrame = contrastFrames.frames.front();
    const float rawPeak = *std::max_element(contrastFrame.bandLevelLeft.begin(), contrastFrame.bandLevelLeft.end());
    const float displayedPeak = *std::max_element(contrastFrame.barLeft.begin(), contrastFrame.barLeft.end());
    const float plainPeak = rawPeak * BarSettings::displayGain
        * (BarSettings::currentChannelGain - BarSettings::oppositeChannelGain);
    check("spectral_contrast_reaches_highres_bars", displayedPeak > plainPeak + 0.001f);

    // Different tones on L and R must still give the mean channel power in dB.
    // This is a linear projection of powers, not a maximum that chooses winners.
    analyzer.prepare({48000, 2, 4096, 512});
    auto stereo = tone(4096, 1500.0);
    const auto otherTone = tone(4096, 1560.0);
    for (int i = 0; i < 4096; ++i) stereo[2 * i + 1] = otherTone[2 * i];
    Frames projected;
    analyzer.processBlock(stereo.data(), 4096, 0, projected);
    bool channelMean = true;
    int comparedBands = 0;
    const auto& stereoFrame = projected.frames.front();
    std::vector<float> monoLeft(4096), monoRight(4096);
    for (int i = 0; i < 4096; ++i) { monoLeft[i] = stereo[2 * i]; monoRight[i] = stereo[2 * i + 1]; }
    analyzer.prepare({48000, 1, 4096, 512});
    Frames projectedLeft, projectedRight;
    analyzer.processBlock(monoLeft.data(), 4096, 0, projectedLeft);
    analyzer.reset();
    analyzer.processBlock(monoRight.data(), 4096, 0, projectedRight);
    for (int b = 0; b < displayBandCount; ++b)
    {
        const float l = projectedLeft.frames.front().bandDb[b], r = projectedRight.frames.front().bandDb[b];
        if (l > -100.0f && r > -100.0f)
        {
            ++comparedBands;
            const double mean = 0.5 * (std::pow(10.0, l / 10.0) + std::pow(10.0, r / 10.0));
            channelMean = channelMean && std::abs(stereoFrame.bandDb[b] - 10.0 * std::log10(mean)) < 0.0001;
        }
    }
    check("smooth_projection_matches_mean_channel_power", channelMean && comparedBands >= 2);

    {
        const FrequencyScale scale(20000.0), logarithmic(20000.0, 0.0, 1.0);
        const FrequencyScale shifted(20000.0, 200.0, 1.0), almostLinear(20000.0, 1.0e10, 1.0);
        check("mapping_endpoints", scale.frequency(0.0) == 30.0 && scale.frequency(1.0) == 20000.0
            && scale.position(30.0) == 0.0 && scale.position(20000.0) == 1.0);
        check("mapping_clamps_outside_display", scale.position(0.0) == 0.0 && scale.position(30000.0) == 1.0
            && scale.frequency(-1.0) == 30.0 && scale.frequency(2.0) == 20000.0);
        bool monotone = true, inverse = true, legacy = true, intermediate = true, linear = true;
        for (double maximum : {1000.0, 4000.0, 8000.0, 20000.0})
            for (double offset : {0.0, 200.0, 1.0e10})
            {
                const FrequencyScale candidate(maximum, offset);
                double previous = 0.0;
                for (int i = 0; i <= 1000; ++i)
                {
                    const double x = i / 1000.0, hz = candidate.frequency(x);
                    monotone = monotone && std::isfinite(hz) && hz > previous && hz <= maximum;
                    inverse = inverse && std::abs(candidate.position(hz) - x) < 1e-10;
                    previous = hz;
                }
            }
        for (int i = 1; i < 100; ++i)
        {
            const double x = i / 100.0, hz = 30.0 + x * (20000.0 - 30.0);
            legacy = legacy && std::abs(logarithmic.frequency(x) - 30.0 * std::pow(20000.0 / 30.0, x)) < 1e-8;
            intermediate = intermediate && shifted.position(hz) > x && shifted.position(hz) < logarithmic.position(hz);
            linear = linear && std::abs(almostLinear.position(hz) - x) < 3e-7;
        }
        check("mapping_monotone_across_formats_and_offsets", monotone);
        check("mapping_round_trip", inverse);
        check("mapping_can_recover_original_log_axis", legacy);
        check("mapping_shift_between_log_and_linear", intermediate);
        check("mapping_large_offset_approaches_linear", linear);
        check("mapping_treble_occupies_twenty_bars", std::abs(scale.position(2300.0) - 0.8) < 1e-12
            && std::abs(scale.frequency(0.8) - 2300.0) < 1e-8);
        const FrequencyScale shortRange(2300.0), shortUncompressed(2300.0, FrequencySettings::offsetHz, 1.0);
        check("mapping_below_treble_has_no_compression", shortRange.frequency(0.7) == shortUncompressed.frequency(0.7));
        // Derivatives agree at both ends of the smooth transition as well as the knee.
        const double f0 = FrequencySettings::offsetHz, low = 30.0 + f0;
        const double span = std::log((20000.0 + f0) / low);
        const double knee = std::log((2300.0 + f0) / low) / span;
        const double halfWidth = 0.5 * FrequencySettings::transitionOctaves * std::log(2.0) / span;
        bool smoothJoins = true;
        for (double u : {knee - halfWidth, knee, knee + halfWidth})
        {
            const double hz = low * std::exp(u * span) - f0, step = 0.01;
            const double before = (scale.position(hz) - scale.position(hz - step)) / step;
            const double after = (scale.position(hz + step) - scale.position(hz)) / step;
            smoothJoins = smoothJoins && std::abs(after - before) < before * 0.001;
        }
        check("mapping_smooth_transition_slopes", smoothJoins);
        bool bandsMatch = true;
        for (int b = 0; b < displayBandCount; ++b)
            bandsMatch = bandsMatch && std::abs(scale.position(stereoFrame.bandHz[b]) - (b + 0.5) / displayBandCount) < 1e-7
                && std::abs(scale.position(stereoFrame.bandEdgesHz[b]) - b / static_cast<double>(displayBandCount)) < 1e-7
                && stereoFrame.bandEdgesHz[b] < stereoFrame.bandHz[b] && stereoFrame.bandHz[b] < stereoFrame.bandEdgesHz[b + 1];
        check("mapping_engine_centres_and_edges_match_axis", bandsMatch && stereoFrame.bandEdgesHz.back() == 20000.0f);
    }

    {
        bool floorMonotone = true;
        float previous = displayFloorDb(0.0);
        for (double hz = 30.0; hz <= 20000.0; hz *= 1.03)
        {
            const float floor = displayFloorDb(hz);
            floorMonotone = floorMonotone && floor <= previous && floor < 0.0f
                && floor >= BarSettings::floorDb && floor <= BarSettings::floorDb + BarSettings::bassFloorLiftDb;
            previous = floor;
        }
        check("frequency_floor_bounded_and_monotone", floorMonotone);
        check("frequency_floor_limits", displayFloorDb(0.0) == BarSettings::floorDb + BarSettings::bassFloorLiftDb
            && std::abs(displayFloorDb(1.0e12) - BarSettings::floorDb) < 1e-5f);
        // Order-4 levels dominate the quadratic reference, with the same per-band floor.
        bool floorApplied = true, haveVisible = false, haveHidden = false;
        const auto& frame = contrastFrame; // Identical tones on both channels.
        for (int b = 0; b < displayBandCount; ++b)
        {
            const float floor = displayFloorDb(frame.bandHz[b]);
            const float expected = std::clamp((frame.bandDb[b] - floor) / -floor, 0.0f, 1.0f);
            floorApplied = floorApplied && std::abs(frame.bandFloorDb[b] - floor) < 1e-5f
                && frame.bandLevelLeft[b] >= expected - 1e-5f && frame.bandLevelLeft[b] <= 1.0f && frame.bandLevelLeft[b] == frame.bandLevelRight[b];
            haveVisible = haveVisible || expected > 0.0f;
            haveHidden = haveHidden || expected == 0.0f;
        }
        check("frequency_floor_reaches_both_display_channels", floorApplied && haveVisible && haveHidden);
    }

    {
        AnalysisStream stream;
        SpectrumFrame frame;
        check("stream_disabled_initially", !stream.latest(frame));
        stream.setEnabled(true);
        auto block = tone(8192, 1500.0);
        const auto before = block;
        stream.push(block.data(), 8192, 2, 48000, 0);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        bool have = false;
        while (!(have = stream.latest(frame)) && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        check("stream_publishes_and_preserves_pcm", have && block == before && std::abs(frame.power[static_cast<int>(1500.0 * frame.fftSize / frame.sampleRate)] - 0.0625f) < 2e-6f);
        stream.invalidate();
        check("invalidate_hides_old_frames", !stream.latest(frame));
        block.back() = -1.2f; // The last sample is dropped whenever push cannot accept the full block.
        for (int i = 0; i < 256; ++i)
        {
            stream.push(block.data(), 8192, 2, 48000, static_cast<std::uint64_t>(i) * 8192);
        }
        check("overflow_is_bounded_and_counted", stream.droppedFrames() > 0);
        stream.setEnabled(false);
        check("disable_hides_frames", !stream.latest(frame));
    }
    {
        AnalysisStream stream;
        stream.setEnabled(true);
        auto block = tone(4096, 1500.0);
        SpectrumFrame frame;
        check("stream_rejects_position_overflow", !stream.push(block.data(), 4096, 2, 48000,
              std::numeric_limits<std::uint64_t>::max() - 1) && stream.lastResult() == AnalysisResult::invalidBlock);
        block[0] = std::numeric_limits<float>::quiet_NaN();
        stream.push(block.data(), 4096, 2, 48000, 0);
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (stream.lastResult() != AnalysisResult::nonFiniteInput && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        check("stream_nonfinite_is_reported", stream.lastResult() == AnalysisResult::nonFiniteInput && !stream.latest(frame));
        block = tone(4096, 1500.0);
        stream.push(block.data(), 4096, 2, 48000, 10000);
        deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        bool have = false;
        while (!(have = stream.latest(frame)) && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        check("stream_recovers_after_bad_input", have && frame.firstSample >= 10000 && frame.firstSample <= 12048);
        std::this_thread::sleep_for(std::chrono::milliseconds(450));
        check("stream_hides_stale_frames", !stream.latest(frame));
    }
    // Peaks are local to each consumer, retained through batched deliveries,
    // and released after actual presentation without extending the envelope.
    {
        BarPeakAccumulator worker, view, independent;
        BandLevels leftHeights{}, rightHeights{};
        check("bar_peaks_initially_empty", !worker.hasPending() && worker.consume().left == leftHeights);
        leftHeights[8] = 0.8f; rightHeights[9] = 0.3f;
        worker.add(leftHeights, rightHeights); independent.add(leftHeights, rightHeights);
        leftHeights[8] = 0.2f; rightHeights[9] = 0.9f;
        worker.add(leftHeights, rightHeights);
        const auto peak = worker.consume();
        check("bar_peaks_both_channels_and_bands", peak.left[8] == 0.8f && peak.right[9] == 0.9f
            && peak.left[9] == 0.0f && peak.right[8] == 0.0f);
        const auto instantaneous = worker.consume();
        check("bar_peaks_not_replayed_on_second_read", instantaneous.left == leftHeights && instantaneous.right == rightHeights);
        check("bar_peak_consumers_independent", independent.hasPending() && independent.consume().right[9] == 0.3f);
        view.add(peak.left, peak.right);
        view.add(instantaneous.left, instantaneous.right); // Two updates before a single paint.
        auto drawn = view.consume();
        check("bar_peaks_survive_coalesced_paint", drawn.left[8] == 0.8f && drawn.right[9] == 0.9f);
        if (view.hasPending()) drawn = view.consume(); // An exposure repaint must reuse drawn values.
        check("bar_repaint_without_update_retains_drawn_values", drawn.left[8] == 0.8f);
        view.add(leftHeights, rightHeights); drawn = view.consume();
        check("bar_next_paint_can_fall", drawn.left[8] == 0.2f);
        worker.add(peak.left, peak.right); worker.add(leftHeights, rightHeights); worker.discardPending();
        check("bar_hide_discards_old_maximum", worker.consume().left[8] == 0.2f);
        worker.reset();
        check("bar_reset_clears_latest_and_pending", !worker.hasPending() && worker.consume().left == BandLevels{});
    }
    {
        const AnalyzerConfig config;
        check("default_fft_2048_hop_512", config.fftSize == 2048 && config.hopSize == 512);
        auto signal = tone(8192, 1500.0);
        AudioAnalyzer whole, chunks;
        whole.prepare(config); chunks.prepare(config);
        Frames once, partitioned;
        whole.processBlock(signal.data(), 8192, 0, once);
        for (int first = 0; first < 8192;)
        {
            const int count = (std::min)(137, 8192 - first);
            chunks.processBlock(signal.data() + 2 * first, count, first, partitioned);
            first += count;
        }
        bool partitionEqual = once.frames.size() == 13 && partitioned.frames.size() == once.frames.size();
        for (int i = 0; partitionEqual && i < static_cast<int>(once.frames.size()); ++i)
            partitionEqual = once.frames[i].left == partitioned.frames[i].left
                && once.frames[i].barLeft == partitioned.frames[i].barLeft
                && once.frames[i].firstSample == static_cast<std::uint64_t>(i * 512);
        check("short_fft_chunk_invariance_and_timestamps", partitionEqual);
        const auto& f = once.frames.front();
        check("short_fft_tone_normalization", std::abs(f.power[64] - 0.0625f) < 2e-6f);
    }
    {
        // Sweep the impulse position across an entire hop, away from stream boundaries.
        // With Hann/N=2048/H=512 the nearest centre is at most 256 samples away.
        bool captured = true;
        double worst = 1.0;
        for (int offset = 0; offset < 512; offset += 16)
        {
            AudioAnalyzer local;
            local.prepare({48000, 1});
            std::vector<float> signal(6144, 0.0f);
            signal[3072 + offset] = 1.0f;
            Frames frames;
            local.processBlock(signal.data(), static_cast<int>(signal.size()), 0, frames);
            double best = 0.0;
            BarPeakAccumulator peaks;
            for (const auto& f : frames.frames)
            {
                best = (std::max)(best, static_cast<double>(std::abs(f.left[100])) * 512.0);
                peaks.add(f.barLeft, f.barRight);
            }
            worst = (std::min)(worst, best);
            // A unit impulse can lie below the chosen display floor.
            // Test the FFT window coverage independently of that visual threshold.
            captured = captured && best >= 0.85355;
        }
        check("short_fft_impulses_across_hop_retain_hann_bound", captured && worst < 0.854);
    }
    {
        AudioAnalyzer local;
        local.prepare({48000, 2});
        auto signal = tone(2048, 6000.0, 0.0f);
        for (auto& value : signal) value *= 0.16f;
        Frames first, doubled;
        local.processBlock(signal.data(), 2048, 0, first);
        local.reset();
        for (auto& value : signal) value *= 2.0f;
        local.processBlock(signal.data(), 2048, 0, doubled);
        const auto& a = first.frames.front();
        const auto& b = doubled.frames.front();
        const int peak = static_cast<int>(std::max_element(a.bandLevelLeft.begin(), a.bandLevelLeft.end()) - a.bandLevelLeft.begin());
        const float db = a.bandFloorDb[peak] * (1.0f - a.bandLevelLeft[peak]);
        const float db2 = b.bandFloorDb[peak] * (1.0f - b.bandLevelLeft[peak]);
        // Raw combined power halves a left-only tone; undo that for comparison.
        const float quadraticDb = a.bandDb[peak] + 3.0103f;
        check("order_four_emphasizes_narrow_tone_over_rms", db > quadraticDb + 1.0f && db <= -27.957f);
        check("order_four_doubling_adds_six_db", std::abs((db2 - db) - 6.0206f) < 0.002f);
        check("order_four_preserves_silent_channel", a.bandLevelRight == BandLevels{});
        // A flat spectrum is unchanged by the order of the normalized mean.
        local.reset();
        std::fill(signal.begin(), signal.end(), 0.0f);
        signal[1024 * 2] = signal[1024 * 2 + 1] = 8.0f; // Flat spectrum above the display floor.
        Frames flat;
        local.processBlock(signal.data(), 2048, 0, flat);
        const auto& f = flat.frames.front();
        const int band = 70;
        const float flatDb = f.bandFloorDb[band] * (1.0f - f.bandLevelLeft[band]);
        check("order_four_preserves_flat_spectrum", std::abs(flatDb - f.bandDb[band]) < 0.002f);
        const FrequencyScale scale(20000.0), oldScale(20000.0, 200.0);
        const double spacing = scale.frequency(0.015) - scale.frequency(0.005);
        check("short_fft_bass_centres_less_redundant", spacing > 11.0 && spacing < 12.0
            && spacing > 1.6 * (oldScale.frequency(0.015) - oldScale.frequency(0.005)));
    }
    {
        AnalysisStream stream;
        stream.setEnabled(true);
        AudioAnalyzer reference;
        reference.prepare({48000, 2});
        auto signal = tone(8192, 1500.0);
        for (int i = 3072 * 2; i < static_cast<int>(signal.size()); ++i) signal[i] = 0.0f;
        Frames expected;
        reference.processBlock(signal.data(), 8192, 0, expected);
        BarPeakAccumulator peaks;
        for (const auto& f : expected.frames) peaks.add(f.barLeft, f.barRight);
        const auto expectedPeak = peaks.consume();
        SpectrumFrame latest;
        const auto waitFor = [&](std::uint64_t first)
        {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
            while (std::chrono::steady_clock::now() < deadline)
            {
                if (stream.latest(latest) && latest.firstSample == first) return true;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            return false;
        };
        const bool accepted = stream.push(signal.data(), 8192, 2, 48000, 0);
        const bool ready = waitFor(6144);
        BarSnapshot bars;
        check("stream_peaks_survive_multiple_fft_frames", accepted && ready && stream.consumeBars(bars)
            && bars.levels.left == expectedPeak.left && bars.levels.right == expectedPeak.right
            && stream.droppedFrames() == 0);
        const auto generation = bars.generation;
        check("stream_peak_read_does_not_replace_raw_frame", latest.barLeft == expected.frames.back().barLeft
            && stream.consumeBars(bars) && bars.levels.left == latest.barLeft);
        std::vector<float> silenceBlock(2048 * 2, 0.0f);
        // A backward seek can produce a firstSample that looks like the next
        // ordinary FFT window; detect the discontinuity at the input boundary.
        stream.push(silenceBlock.data(), 2048, 2, 48000, 6656);
        check("stream_backward_gap_clears_peak_generation", waitFor(6656) && stream.consumeBars(bars)
            && bars.generation != generation && bars.levels.left == BandLevels{});
        stream.invalidate();
        check("stream_invalidate_hides_peak_snapshot", !stream.consumeBars(bars) && bars.levels.left == BandLevels{});
        stream.push(silenceBlock.data(), 2048, 2, 44100, 0);
        check("stream_format_change_restarts_peaks", waitFor(0) && stream.consumeBars(bars)
            && latest.sampleRate == 44100.0 && bars.levels.left == BandLevels{});
        std::this_thread::sleep_for(std::chrono::milliseconds(450));
        check("stream_stale_peak_snapshot_hidden", !stream.consumeBars(bars));
    }
    std::cout << "],\"dft_max_error\":" << maxError << ",\"sweep_max_step_db\":" << maximumSweepStepDb
              << ",\"highres_sweep_max_step_db\":" << maximumHighResStepDb
              << ",\"sweep_max_step_hz\":" << maximumStepHz << ",\"sweep_min_peak_db\":" << minimumSweepPeakDb
              << ",\"valid\":" << (failures == 0 ? "true" : "false") << "}\n";
    return failures == 0 ? 0 : 1;
}
