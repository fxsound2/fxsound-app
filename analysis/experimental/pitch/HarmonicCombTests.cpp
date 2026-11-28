// SPDX-License-Identifier: AGPL-3.0-or-later
#include "HarmonicCombProfile.h"
#include "../../tests/TestSupport.h"
#include <memory>
#include <iostream>

namespace fxanalysis::test
{
namespace
{
using namespace experimental;
SpectrumFrame combFrame(double fundamental = 375.0)
{
    SpectrumFrame frame;
    frame.sampleRate = 48000;
    frame.fftSize = 2048;
    frame.hopSize = 512;
    frame.channels = 2;
    frame.power.resize(1025);
    const double binHz = frame.sampleRate / frame.fftSize;
    for (int bin = 1; bin < 1024; ++bin)
    {
        double magnitude = 0.0;
        for (int h = 1; h <= 24; ++h)
        {
            const double x = bin - h * fundamental / binHz;
            const auto sinc = [](double value) { return std::abs(value) < 1e-10 ? 1.0 : std::sin(pi * value) / (pi * value); };
            if (std::abs(x) < 2.0)
                magnitude += 0.2 / std::sqrt(static_cast<double>(h))
                    * std::abs(sinc(x) + 0.5 * sinc(x - 1.0) + 0.5 * sinc(x + 1.0));
        }
        frame.power[bin] = static_cast<float>(magnitude * magnitude);
    }
    return frame;
}
int near(const HarmonicCombSnapshot& s, double hz)
{
    int best = 0;
    for (int i = 1; i < s.count; ++i)
        if (std::abs(s.frequencyHz[i] - hz) < std::abs(s.frequencyHz[best] - hz)) best = i;
    return best;
}
bool finite(const HarmonicCombSnapshot& s)
{
    if (!std::isfinite(s.fitFraction) || !std::isfinite(s.removedEnergyFraction)
        || s.fitFraction < 0 || s.fitFraction > 1 || s.removedEnergyFraction < 0 || s.removedEnergyFraction > 1) return false;
    for (int i = 0; i < s.count; ++i)
        if (!std::isfinite(s.before[i]) || !std::isfinite(s.after[i])
            || s.before[i] < 0 || s.after[i] < 0 || s.after[i] > s.before[i] + 1e-6f) return false;
    return s.count >= 0 && s.count <= harmonicProfileCapacity;
}
}
void runHarmonicPursuitTests(const Check& check);
void runHarmonicCombTests(const Check& check)
{
    PitchMotionSettings legacy;
    legacy.harmonicComb.harmonicDecay = 0.5; // Match the fixed synthetic comb oracle.
    legacy.harmonicComb.iterativeExtraction = false;
    legacy.harmonicComb.maximumHarmonics = 24;
    auto engine = std::make_unique<HarmonicCombProfile>();
    auto frame = combFrame();
    const auto original = frame.power;
    check("harmonic_unprepared_status", engine->process(frame) == PitchMotionResult::notPrepared);
    check("harmonic_prepare_defaults", engine->prepare(legacy) == PitchMotionResult::ok);
    const auto result = engine->process(frame);
    const auto sampleOwner = std::make_unique<HarmonicCombSnapshot>(engine->snapshot());
    const auto& sample = *sampleOwner;
    check("harmonic_frame_estimate_is_finite", result == PitchMotionResult::ok && sample.hasEstimate && finite(sample));
    check("harmonic_profile_limits_carriers_to_100_3000_hz", sample.count > 300
        && sample.frequencyHz[0] == 100.0f && sample.frequencyHz[sample.count - 1] == 3000.0f);
    check("harmonic_profile_frequencies_are_increasing", std::adjacent_find(sample.frequencyHz.begin(),
        sample.frequencyHz.begin() + sample.count, [](float a, float b) { return a >= b; }) == sample.frequencyHz.begin() + sample.count);
    check("harmonic_known_comb_selected", std::abs(12.0 * std::log2(sample.selectedFrequencyHz / 375.0)) < 0.15);
    check("harmonic_fit_contains_observed_partials", sample.fittedHarmonics >= 20 && sample.fitFraction > 0.9);
    check("harmonic_selected_marker_preserves_original_amplitude", sample.selectedAmplitude == sample.before[near(sample, sample.selectedFrequencyHz)]);
    check("harmonic_partial_subtraction_reduces_alias_2f", sample.after[near(sample, 750)] < 0.5f * sample.before[near(sample, 750)]);
    check("harmonic_partial_subtraction_reduces_alias_3f", sample.after[near(sample, 1125)] < 0.5f * sample.before[near(sample, 1125)]);
    check("harmonic_subtraction_never_increases_any_candidate", finite(sample) && sample.removedEnergyFraction > 0.8);
    check("harmonic_does_not_modify_input_spectrum", frame.power == original);
    bool rawExact = sample.inputSpectrumCount > 0;
    for (int i = 0; i < sample.inputSpectrumCount; ++i)
        rawExact = rawExact && sample.inputMagnitude[i] == std::sqrt(frame.power[sample.firstInputBin + i]);
    check("harmonic_raw_spectrum_is_exact_fft_module_before_cleaning", rawExact
        && sample.inputBinHz == frame.sampleRate / frame.fftSize);
    auto mixture = frame;
    const auto second = combFrame(530.0);
    for (int bin = 0; bin <= 1024; ++bin) mixture.power[bin] += 0.16f * second.power[bin];
    engine->reset(); engine->process(mixture);
    const auto polyphonicOwner = std::make_unique<HarmonicCombSnapshot>(engine->snapshot());
    const auto& polyphonic = *polyphonicOwner;
    check("harmonic_one_pass_retains_nonharmonic_companion", finite(polyphonic)
        && std::abs(12.0 * std::log2(polyphonic.selectedFrequencyHz / 375.0)) < 0.2
        && polyphonic.after[near(polyphonic, 530.0)] > 0.25f * polyphonic.before[near(polyphonic, 530.0)]);
    auto amplified = frame;
    for (auto& p : amplified.power) p *= 4.0f;
    engine->reset();
    engine->process(amplified);
    const auto gainOwner = std::make_unique<HarmonicCombSnapshot>(engine->snapshot());
    const auto& gain = *gainOwner;
    check("harmonic_global_gain_preserves_selected_frequency", gain.selectedFrequencyHz == sample.selectedFrequencyHz);
    check("harmonic_amplitude_scales_with_module_not_power", std::abs(gain.selectedAmplitude / sample.selectedAmplitude - 2.0) < 1e-5);
    bool rawGain = gain.inputSpectrumCount == sample.inputSpectrumCount;
    for (int i = 0; i < sample.inputSpectrumCount; ++i)
        rawGain = rawGain && std::abs(gain.inputMagnitude[i] - 2.0f * sample.inputMagnitude[i]) < 1e-6f;
    check("harmonic_raw_spectrum_scales_with_module", rawGain);
    PitchMotionSettings settings = legacy;
    settings.harmonicComb.subtractionFraction = 0;
    engine->prepare(settings); engine->process(frame);
    check("harmonic_zero_subtraction_is_exact_bypass", engine->snapshot().before == engine->snapshot().after
        && engine->snapshot().removedEnergyFraction == 0.0);
    settings.harmonicComb.subtractionFraction = 1;
    engine->prepare(settings); engine->process(frame);
    check("harmonic_full_subtraction_removes_fitted_energy", engine->snapshot().removedEnergyFraction > sample.removedEnergyFraction
        && engine->snapshot().removedEnergyFraction > 0.9 && finite(engine->snapshot()));
    settings.harmonicComb.displayGain = 9;
    engine->prepare(settings); engine->process(frame);
    const auto displayOwner = std::make_unique<HarmonicCombSnapshot>(engine->snapshot());
    const auto& display = *displayOwner;
    settings.harmonicComb.displayGain = 1;
    engine->prepare(settings); engine->process(frame);
    check("harmonic_display_gain_does_not_change_numerical_profile", engine->snapshot().before == display.before
        && engine->snapshot().after == display.after);
    frame.firstSample = 512;
    engine->process(frame);
    check("harmonic_time_uses_audio_hop", std::abs(engine->snapshot().stepSeconds - 512.0 / 48000.0) < 1e-12);
    frame.firstSample = 0;
    engine->process(frame);
    check("harmonic_seek_does_not_retain_temporal_state", engine->snapshot().stepSeconds == 0.0 && engine->snapshot().hasEstimate);
    frame.power.assign(1025, 0.0f);
    check("harmonic_silence_hides_old_profile", engine->process(frame) == PitchMotionResult::noSignal
        && !engine->snapshot().hasEstimate && engine->snapshot().selectedAmplitude == 0.0);
    frame = combFrame(); frame.power[20] = std::numeric_limits<float>::quiet_NaN();
    check("harmonic_nonfinite_input_is_explicit", engine->process(frame) == PitchMotionResult::nonFiniteInput && !engine->snapshot().hasEstimate);
    frame = combFrame(); frame.power[20] = -1;
    check("harmonic_negative_power_rejected", engine->process(frame) == PitchMotionResult::invalidFrame);
    frame = combFrame(); frame.power.pop_back();
    check("harmonic_wrong_frame_shape_rejected", engine->process(frame) == PitchMotionResult::invalidFrame);
    settings.harmonicComb.subtractionFraction = std::numeric_limits<double>::infinity();
    check("harmonic_invalid_subtraction_rejected", engine->prepare(settings) == PitchMotionResult::invalidConfiguration);
    settings = legacy; settings.harmonicComb.maximumHarmonics = 65;
    check("harmonic_capacity_guard", engine->prepare(settings) == PitchMotionResult::invalidConfiguration);
    settings = legacy; settings.harmonicComb.enabled = false;
    engine->prepare(settings);
    check("harmonic_runtime_disable_clears_estimate", !engine->snapshot().enabled && !engine->snapshot().hasEstimate);
    settings = legacy; engine->prepare(settings);
    frame = combFrame(); frame.sampleRate = 8000;
    engine->process(frame);
    check("harmonic_frequency_range_obeys_nyquist", finite(engine->snapshot()) && engine->snapshot().maximumFrequencyHz == 4000.0
        && engine->snapshot().frequencyHz[engine->snapshot().count - 1] == 3000.0f);
    engine->reset();
    check("harmonic_reset_hides_profile", !engine->snapshot().hasEstimate && engine->snapshot().count == 0);
    auto edgeFrame = combFrame(3000.0);
    PitchMotionSettings edgeSettings = legacy;
    edgeSettings.harmonicComb.maximumHarmonics = 6;
    engine->prepare(edgeSettings); engine->process(edgeFrame);
    double observedEnergy = 0.0;
    for (int bin = 1; bin * edgeFrame.sampleRate / edgeFrame.fftSize <= 20000.0; ++bin)
        observedEnergy += edgeFrame.power[bin];
    check("harmonic_partials_above_carrier_band_still_fit",
        observedEnergy > 0.0 && std::abs(engine->snapshot().before[engine->snapshot().count - 1] - 0.2) < 1e-5);
    PitchMotionSettings equalSettings = legacy;
    equalSettings.noiseFloorStrength = 0.0;
    equalSettings.minimumPeakProminenceDb = 0.0;
    engine->prepare(equalSettings);
    engine->process(combFrame());
    const auto gridOwner = std::make_unique<HarmonicCombSnapshot>(engine->snapshot());
    const auto& grid = *gridOwner;
    double minimum = 1.0, maximum = 0.0;
    for (double hz : {100.0, 150.0, 250.0, 500.0, 1000.0, 2000.0})
    {
        const auto equalFrame = combFrame(grid.frequencyHz[near(grid, hz)]);
        engine->reset(); engine->process(equalFrame);
        const double value = engine->snapshot().before[near(engine->snapshot(), hz)];
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
    }
    check("harmonic_equivalent_fundamental_has_no_harmonic_count_boost", minimum > 0.19 && maximum < 0.205
        && maximum / minimum < 1.06);
    equalSettings.harmonicComb.maximumHarmonics = 1;
    engine->prepare(equalSettings);
    bool bandCovered = true;
    for (double hz = 100.0; hz < 1950.0; hz += 37.25)
    {
        auto tone = combFrame(hz);
        engine->reset(); engine->process(tone);
        bandCovered = bandCovered && engine->snapshot().hasEstimate
            && engine->snapshot().selectedAmplitude > 0.19
            && std::abs(engine->snapshot().selectedFrequencyHz - hz) < tone.sampleRate / tone.fftSize * 0.3;
    }
    check("harmonic_carrier_band_sweep_has_no_gaps_between_candidate_lobes", bandCovered);
    auto outside = combFrame(5000.0);
    engine->reset();
    check("harmonic_single_tooth_outside_carrier_band_is_not_selected",
        engine->process(outside) == PitchMotionResult::noSignal && !engine->snapshot().hasEstimate);
    std::array<float, harmonicProfileCapacity> peaks{};
    peaks[0] = 2; peaks[1] = 1; peaks[2] = 3; peaks[3] = 3; peaks[4] = 1; peaks[5] = 2;
    check("harmonic_local_maxima_include_edges_and_one_plateau_marker",
        harmonicLocalMaximum(peaks, 6, 0) && harmonicLocalMaximum(peaks, 6, 2)
        && harmonicLocalMaximum(peaks, 6, 5) && !harmonicLocalMaximum(peaks, 6, 1)
        && !harmonicLocalMaximum(peaks, 6, 3) && !harmonicLocalMaximum(peaks, 6, 4));
    peaks.fill(1.0f);
    check("harmonic_flat_curve_has_no_local_maximum", !harmonicLocalMaximum(peaks, 6, 0)
        && !harmonicLocalMaximum(peaks, 6, 5));
    check("harmonic_db_scale_shows_weak_components_at_fixed_reference",
        std::abs(harmonicDisplayHeight(0.25, 4.0, true, 60.0) - 1.0) < 1e-10
        && std::abs(harmonicDisplayHeight(0.0025, 4.0, true, 60.0) - 1.0 / 3.0) < 1e-10
        && harmonicDisplayHeight(0.00025, 4.0, true, 60.0) < 1e-10
        && harmonicDisplayHeight(0.0025, 4.0, false, 60.0) == 0.01
        && harmonicDisplayHeight(0.0, 4.0, true, 60.0) == 0.0);
    check("harmonic_defaults_use_linear_highres_reference", !PitchMotionSettings{}.harmonicComb.logarithmicDisplay
        && PitchMotionSettings{}.harmonicComb.highResReference);
    check("harmonic_highres_reference_is_anchored_and_compensates_bass",
        harmonicReferenceGain(1000.0, true) == 1.0 && harmonicReferenceGain(100.0, false) == 1.0
        && std::abs(20.0 * std::log10(harmonicReferenceGain(100.0, true)) + 12.0) < 1e-4
        && std::abs(20.0 * std::log10(harmonicReferenceGain(2000.0, true)
            / harmonicReferenceGain(100.0, true)) - 13.81818) < 1e-4);
    equalSettings.harmonicComb.logarithmicDisplay = false;
    equalSettings.harmonicComb.highResReference = false;
    equalSettings.harmonicComb.displayRangeDb = 100;
    engine->prepare(equalSettings); engine->process(combFrame());
    const auto linearOwner = std::make_unique<HarmonicCombSnapshot>(engine->snapshot());
    const auto& linear = *linearOwner;
    equalSettings.harmonicComb.logarithmicDisplay = true;
    equalSettings.harmonicComb.highResReference = true;
    equalSettings.harmonicComb.displayRangeDb = 20;
    engine->prepare(equalSettings); engine->process(combFrame());
    check("harmonic_axis_and_reference_do_not_change_fit_or_selection", linear.before == engine->snapshot().before
        && linear.after == engine->snapshot().after && linear.inputMagnitude == engine->snapshot().inputMagnitude
        && linear.selectedFrequencyHz == engine->snapshot().selectedFrequencyHz);
    equalSettings.harmonicComb.displayRangeDb = 0;
    check("harmonic_invalid_db_range_rejected", engine->prepare(equalSettings) == PitchMotionResult::invalidConfiguration);
    auto background = combFrame();
    background.power.assign(1025, 1e-6f);
    engine->prepare(legacy);
    check("harmonic_raw_fft_remains_visible_when_background_is_rejected",
        engine->process(background) == PitchMotionResult::noSignal && !engine->snapshot().hasEstimate
        && engine->snapshot().inputSpectrumCount > 0 && engine->snapshot().inputMagnitude[0] > 0.0f);
    PitchMotionSettings largeSettings;
    largeSettings.noiseFloorStrength = 0;
    largeSettings.minimumPeakProminenceDb = 0;
    largeSettings.harmonicComb.maximumHarmonics = 1;
    engine->prepare(largeSettings);
    SpectrumFrame largeFrame;
    largeFrame.sampleRate = 8000; largeFrame.fftSize = AnalysisLimits::maximumFftSize;
    largeFrame.hopSize = 512; largeFrame.channels = 1;
    largeFrame.power.resize(largeFrame.fftSize / 2 + 1);
    largeFrame.power[1536] = 0.04f;
    check("harmonic_raw_fft_capacity_covers_largest_supported_geometry",
        engine->process(largeFrame) == PitchMotionResult::ok && engine->snapshot().inputSpectrumCount > 11000
        && engine->snapshot().inputSpectrumCount <= harmonicInputSpectrumCapacity && finite(engine->snapshot()));
    for (int rate : {44100, 48000})
        for (bool missingFundamental : {false, true})
        {
            auto pcmEngine = std::make_unique<HarmonicCombProfile>();
            pcmEngine->prepare(legacy);
            struct Sink final : SpectrumSink
            {
                HarmonicCombProfile& engine;
                int frames = 0;
                bool good = true;
                explicit Sink(HarmonicCombProfile& value) : engine(value) {}
                void onSpectrum(const SpectrumFrame& frame) override
                {
                    const auto result = engine.process(frame);
                    const auto& sample = engine.snapshot();
                    good = good && result == PitchMotionResult::ok && finite(sample)
                        && std::abs(12.0 * std::log2(sample.selectedFrequencyHz / 375.0)) < 0.3;
                    ++frames;
                }
            } sink(*pcmEngine);
            AudioAnalyzer analyzer;
            const bool prepared = analyzer.prepare({static_cast<double>(rate), 2, 2048, 512}) == AnalysisResult::ok;
            std::vector<float> pcm(8192 * 2);
            for (int pcmSample = 0; pcmSample < 8192; ++pcmSample)
            {
                double value = 0.0;
                for (int h = missingFundamental ? 2 : 1; h <= 12; ++h)
                    value += 0.12 / std::sqrt(static_cast<double>(h))
                        * std::sin(2.0 * pi * 375.0 * h * pcmSample / rate + 0.17 * h);
                pcm[2 * pcmSample] = static_cast<float>(value);
                pcm[2 * pcmSample + 1] = -pcm[2 * pcmSample];
            }
            const auto preserved = pcm;
            const bool processed = prepared && analyzer.processBlock(pcm.data(), 8192, 0, sink) == AnalysisResult::ok;
            const std::string name = "harmonic_pcm_antiphase_" + std::to_string(rate)
                + (missingFundamental ? "_missing_fundamental" : "_full_comb");
            check(name.c_str(), processed && sink.good && sink.frames > 8 && pcm == preserved);
        }
    runHarmonicPursuitTests(check);
}

void runHarmonicPursuitTests(const Check& check)
{
    auto engine = std::make_unique<HarmonicCombProfile>();
    PitchMotionSettings settings;
    settings.harmonicComb.harmonicDecay = 0.5; // Match the fixed synthetic comb oracle.
    settings.noiseFloorStrength = 0;
    settings.minimumPeakProminenceDb = 0;
    settings.harmonicComb.highResReference = false;
    settings.harmonicComb.harmonicWideningPercent = 0.0; // Sharp-comb reference cases remain exact.
    const auto fixture = [](const std::vector<std::pair<double, double>>& notes, double decay = 0.5)
    {
        SpectrumFrame frame;
        frame.sampleRate = 48000; frame.fftSize = 2048; frame.hopSize = 512; frame.channels = 2;
        frame.power.resize(1025);
        const auto sinc = [](double x) { return std::abs(x) < 1e-12 ? 1.0 : std::sin(pi * x) / (pi * x); };
        for (int bin = 1; bin < 1024; ++bin)
        {
            double value = 0.0;
            for (const auto& note : notes)
                for (int h = 1; h <= 64 && h * note.first <= 20000; ++h)
                {
                    const double x = bin - h * note.first / (48000.0 / 2048);
                    if (std::abs(x) < 2)
                        value += note.second / std::pow(static_cast<double>(h), decay)
                            * std::abs(sinc(x) + 0.5 * sinc(x - 1) + 0.5 * sinc(x + 1));
                }
            frame.power[bin] = static_cast<float>(value * value);
        }
        return frame;
    };
    const auto has = [](const HarmonicCombSnapshot& s, double hz, double amplitude)
    {
        for (int i = 0; i < s.componentCount; ++i)
            if (std::abs(12 * std::log2(s.components[i].frequencyHz / hz)) < 0.12
                && std::abs(s.components[i].fitAmplitude - amplitude) < amplitude * 0.08) return true;
        return false;
    };
    const auto valid = [](const HarmonicCombSnapshot& s)
    {
        if (!s.iterativeExtraction || s.componentCount < 0 || s.componentCount > 4
            || s.extractionSteps > 4 || !std::isfinite(s.fitKktResidual) || s.fitKktResidual > 1e-6) return false;
        for (int i = 1; i <= s.extractionSteps; ++i)
            if (!(s.errorEnergy[i] < s.errorEnergy[i - 1])) return false;
        for (int i = 0; i < s.componentCount; ++i)
            if (!std::isfinite(s.components[i].fitAmplitude) || s.components[i].fitAmplitude <= 0
                || s.components[i].frequencyHz < 100 || s.components[i].frequencyHz > 3000) return false;
        return s.reconstructionError < 1e-10 && s.residualErrorEnergy <= s.inputEnergy;
    };
    {
        auto bassSettings = settings;
        bassSettings.harmonicComb.maximumComponents = 1;
        bassSettings.harmonicComb.maximumHarmonics = 1;
        auto bass = fixture({{375, 0.2}});
        bass.power[3] = 1.0f; bass.power[4] = 4.0f; bass.power[5] = 1.0f;
        engine->prepare(bassSettings); engine->process(bass);
        check("pursuit_sub100_bass_does_not_spend_the_single_carrier_slot",
            engine->snapshot().componentCount == 1 && has(engine->snapshot(), 375, 0.2));
        bass.power.assign(1025, 0.0f);
        // A lower tone whose upper Hann wing overlaps the first candidate.
        bass.power[1] = 1.0f; bass.power[2] = 4.0f; bass.power[3] = 1.0f;
        engine->reset();
        check("pursuit_isolated_sub100_bass_has_no_carrier", engine->process(bass) == PitchMotionResult::noSignal
            && engine->snapshot().componentCount == 0);
    }
    check("pursuit_defaults_four_components_and_64_harmonics", settings.harmonicComb.iterativeExtraction
        && settings.harmonicComb.maximumComponents == 4 && settings.harmonicComb.maximumHarmonics == 64);
    {
        auto partialSettings = settings;
        partialSettings.harmonicComb.maximumHarmonics = 3;
        engine->prepare(partialSettings);
        auto partials = fixture({});
        // For the 187.5 Hz proposal, H2 and H3 fall at bins 16 and 24.
        partials.power[15] = partials.power[17] = .0025f; partials.power[16] = .01f;
        engine->process(partials);
        const int index = near(engine->snapshot(), 187.5);
        const float oneTooth = engine->snapshot().before[index];
        partials.power[23] = partials.power[25] = .0016f; partials.power[24] = .0064f;
        partials.firstSample += partials.hopSize;
        engine->process(partials);
        check("bass_missing_fundamental_requires_two_resolved_upper_teeth",
            oneTooth == 0.0f && engine->snapshot().before[index] > .01f);
    }
    // A changing rumble has considerable energy inside the search band, but
    // no resolved bass line. Keep the real upper note in the same observations.
    for (bool iterative : {true, false})
    {
        PitchMotionSettings live;
        live.harmonicComb.iterativeExtraction = iterative;
        engine->prepare(live);
        int edgeHits = 0, bassHits = 0, upperHits = 0;
        const auto upper = fixture({{660.0, 0.12}}, 1.3);
        for (int n = 0; n < 16; ++n)
        {
            auto rumble = upper;
            rumble.firstSample = n * rumble.hopSize;
            for (int bin = 1; bin < rumble.fftSize / 2; ++bin)
            {
                const double hz = bin * rumble.sampleRate / rumble.fftSize;
                const double skirt = 1.0 / (1.0 + std::pow(hz / (75.0 + n), 6.0));
                const double dome = std::exp(-0.5 * std::pow((hz - 205.0) / 68.0, 2));
                rumble.power[bin] += static_cast<float>(0.4 * skirt + 0.012 * dome);
            }
            engine->process(rumble);
            bool atEdge = false, inBass = false, atUpper = false;
            for (int i = 0; i < engine->snapshot().componentCount; ++i)
            {
                const double hz = engine->snapshot().components[i].frequencyHz;
                atEdge |= hz < 105.0;
                inBass |= hz <= 300.0;
                atUpper |= std::abs(hz - 660.0) < 8.0;
            }
            edgeHits += atEdge; bassHits += inBass; upperHits += atUpper;
        }
        std::cerr << "Rumble + 660 Hz (" << (iterative ? "pursuit" : "legacy")
            << "): edge=" << edgeHits << ", bass=" << bassHits << ", upper=" << upperHits << "/16\n";
        check(iterative ? "pursuit_rumble_does_not_draw_a_bass_line" : "legacy_rumble_does_not_draw_a_bass_line",
            bassHits == 0 && upperHits == 16);
    }
    engine->prepare(settings);
    auto frame = fixture({{375, 0.2}});
    const auto preserved = frame.power;
    engine->process(frame);
    const auto report = [](const char* label, const HarmonicCombSnapshot& s)
    {
        std::cerr << label << " components=" << s.componentCount << " fit=" << s.fitFraction
            << " kkt=" << s.fitKktResidual << " identity=" << s.reconstructionError;
        for (int i = 0; i < s.componentCount; ++i)
            std::cerr << " [" << s.components[i].frequencyHz << "," << s.components[i].fitAmplitude << "]";
        std::cerr << '\n';
    };
    report("pursuit mono", engine->snapshot());
    check("pursuit_single_comb_not_repeated_four_times", engine->snapshot().componentCount == 1
        && has(engine->snapshot(), 375, 0.2) && valid(engine->snapshot()));
    check("pursuit_near_complete_single_comb_fit", engine->snapshot().fitFraction > 0.999);
    check("pursuit_fft_input_is_immutable", frame.power == preserved);
    for (const auto& notes : std::vector<std::vector<std::pair<double, double>>>{
        {{210, 0.2}, {337, 0.13}},
        {{210, 0.2}, {337, 0.13}, {613, 0.08}},
        {{210, 0.2}, {337, 0.13}, {613, 0.08}, {1250, 0.05}}})
    {
        engine->reset(); engine->process(fixture(notes));
        report("pursuit mixture", engine->snapshot());
        bool found = engine->snapshot().componentCount == static_cast<int>(notes.size()) && valid(engine->snapshot());
        for (const auto& note : notes) found = found && has(engine->snapshot(), note.first, note.second);
        const std::string name = "pursuit_recovers_" + std::to_string(notes.size()) + "_families";
        check(name.c_str(), found);
    }
    frame = fixture({{250, 0.2}, {500, 0.10}});
    engine->reset(); engine->process(frame);
    report("pursuit octave", engine->snapshot());
    check("pursuit_true_octave_is_allowed_and_jointly_refitted", engine->snapshot().componentCount == 2
        && has(engine->snapshot(), 250, 0.2) && has(engine->snapshot(), 500, 0.10) && valid(engine->snapshot()));
    frame = fixture({{100, 0.2}, {3000, 0.08}});
    engine->reset(); engine->process(frame);
    check("pursuit_includes_both_frequency_boundaries", has(engine->snapshot(), 100, 0.2)
        && has(engine->snapshot(), 3000, 0.08) && valid(engine->snapshot()));
    frame = fixture({{200, 0.16}, {1000, 0.09}});
    settings.harmonicComb.highResReference = true;
    engine->prepare(settings); engine->process(frame);
    check("pursuit_selects_highres_gold_maximum_before_clipping",
        std::abs(engine->snapshot().components[0].frequencyHz - 1000) < 2);
    settings.harmonicComb.highResReference = false;
    engine->prepare(settings); engine->process(frame);
    check("pursuit_flat_reference_changes_ranking_consistently", std::abs(engine->snapshot().components[0].frequencyHz - 200) < 2);
    const auto base = std::make_unique<HarmonicCombSnapshot>(engine->snapshot());
    settings.harmonicComb.displayGain = 32;
    settings.harmonicComb.logarithmicDisplay = true;
    engine->prepare(settings); engine->process(frame);
    bool same = base->componentCount == engine->snapshot().componentCount;
    for (int i = 0; i < base->componentCount; ++i)
        same = same && base->components[i].frequencyHz == engine->snapshot().components[i].frequencyHz
            && base->components[i].fitAmplitude == engine->snapshot().components[i].fitAmplitude;
    check("pursuit_graphic_gain_and_scale_do_not_change_extraction", same);
    for (auto& p : frame.power) p *= 4;
    engine->reset(); engine->process(frame);
    same = base->componentCount == engine->snapshot().componentCount;
    for (int i = 0; i < base->componentCount; ++i)
        same = same && std::abs(base->components[i].frequencyHz - engine->snapshot().components[i].frequencyHz) < 0.01
            && std::abs(2 * base->components[i].fitAmplitude - engine->snapshot().components[i].fitAmplitude) < 1e-5;
    check("pursuit_global_gain_preserves_frequencies_and_scales_amplitudes", same);
    settings.harmonicComb.harmonicWideningPercent = 20;
    settings.harmonicComb.displayGain = 128;
    settings.harmonicComb.maximumComponents = 4;
    engine->prepare(settings); engine->process(frame);
    check("pursuit_broadened_comb_joint_fit_and_reconstruction", valid(engine->snapshot()));
    settings.harmonicComb.harmonicWideningPercent = 0;
    engine->prepare(settings); engine->process(frame);
    check("pursuit_spread_change_invalidates_cached_templates", engine->snapshot().componentCount == base->componentCount
        && std::abs(engine->snapshot().components[0].fitAmplitude - 2 * base->components[0].fitAmplitude) < 1e-5);
    settings.harmonicComb.harmonicWideningPercent = 81;
    check("pursuit_spread_bound_guard", engine->prepare(settings) == PitchMotionResult::invalidConfiguration);
    settings.harmonicComb.harmonicWideningPercent = 0;
    settings.harmonicComb.trailHalfLifeSeconds = -0.1;
    check("pursuit_trail_configuration_guard", engine->prepare(settings) == PitchMotionResult::invalidConfiguration);
    settings.harmonicComb.trailHalfLifeSeconds = 0.2;
    settings.harmonicComb.maximumComponents = 1;
    engine->prepare(settings); engine->process(frame);
    check("pursuit_requested_component_limit", engine->snapshot().componentCount == 1 && engine->snapshot().extractionSteps == 1);
    settings.harmonicComb.maximumComponents = 5;
    check("pursuit_component_capacity_guard", engine->prepare(settings) == PitchMotionResult::invalidConfiguration);
    {
        auto comparison = PitchMotionSettings{};
        comparison.noiseFloorStrength = 0; comparison.minimumPeakProminenceDb = 0;
        comparison.harmonicComb.maximumComponents = 1; comparison.harmonicComb.highResReference = false;
        comparison.harmonicComb.harmonicSupportThreshold = 0; // Isolate decay mismatch from intentional tail truncation.
        const auto controlled = fixture({{220, 0.2}}, 1.2);
        double wrongFit = 0, matchedFit = 0;
        for (double decay : {0.5, 1.0, 1.2}) for (double spread : {0.0, 20.0})
        {
            comparison.harmonicComb.harmonicDecay = decay; comparison.harmonicComb.harmonicWideningPercent = spread;
            engine->prepare(comparison); engine->process(controlled);
            const auto& s = engine->snapshot();
            std::cerr << "harmonic profile comparison p=" << decay << " spread=" << spread
                << " fit=" << s.fitFraction << " frequency=" << s.selectedFrequencyHz << '\n';
            if (decay == 0.5 && spread == 0.0) wrongFit = s.fitFraction;
            if (decay == 1.2 && spread == 0.0) matchedFit = s.fitFraction;
        }
        check("pursuit_matching_harmonic_envelope_improves_controlled_fit", matchedFit > 0.999 && matchedFit > wrongFit + 0.05);
    }
    {
        PitchMotionSettings wideningSettings;
        wideningSettings.harmonicComb.harmonicDecay = 0.5; // Isolate tooth width from envelope mismatch.
        wideningSettings.noiseFloorStrength = 0;
        wideningSettings.minimumPeakProminenceDb = 0;
        wideningSettings.harmonicComb.highResReference = false;
        wideningSettings.harmonicComb.maximumComponents = 1;
        wideningSettings.harmonicComb.maximumHarmonics = 8;
        wideningSettings.harmonicComb.harmonicSupportThreshold = 0;
        bool fitsValid = true, inputPreserved = true;
        const auto fitFixture = [&](double frequency, double widening)
        {
            wideningSettings.harmonicComb.harmonicWideningPercent = widening;
            auto input = fixture({{frequency, 0.2}});
            const auto original = input.power;
            fitsValid &= engine->prepare(wideningSettings) == PitchMotionResult::ok;
            fitsValid &= engine->process(input) == PitchMotionResult::ok;
            const auto& s = engine->snapshot();
            fitsValid &= valid(s) && s.componentCount == 1
                && std::abs(s.components[0].frequencyHz - frequency) < 1.0;
            inputPreserved &= input.power == original;
            return std::make_unique<HarmonicCombSnapshot>(s);
        };
        // Inspect the actual fitted FFT model of bin-centred tones. Its shape
        // must change in the bass fundamental without spreading that change to H2.
        const auto modelSideRatio = [](const HarmonicCombSnapshot& s, double frequency, int offset)
        {
            const int centre = static_cast<int>(std::lround(frequency / s.inputBinHz)) - s.firstInputBin;
            const int side = centre + offset;
            if (centre < 0 || centre >= s.inputSpectrumCount || side < 0 || side >= s.inputSpectrumCount
                || s.modelMagnitude[centre] <= 0.0f) return std::numeric_limits<double>::quiet_NaN();
            return static_cast<double>(s.modelMagnitude[side]) / s.modelMagnitude[centre];
        };
        const auto bass = fitFixture(187.5, 50);
        const auto higher = fitFixture(750.0, 50);
        const auto zero = fitFixture(187.5, 0);
        const auto maximum = fitFixture(234.375, harmonicMaximumWideningPercent);
        const double bassWing = modelSideRatio(*bass, 187.5, 3);
        const double secondWing = modelSideRatio(*bass, 375.0, 3);
        const double higherWing = modelSideRatio(*higher, 750.0, 3);
        std::cerr << "harmonic bass widening: H1 wing=" << bassWing << " H2 wing=" << secondWing
            << " higher H1 wing=" << higherWing << '\n';
        check("pursuit_bass_widening_fits_are_finite_and_coherent", fitsValid);
        check("pursuit_bass_widening_preserves_input_fft", inputPreserved);
        check("pursuit_bass_fundamental_extends_beyond_uniform_widening", bassWing > 0.01
            && secondWing < bassWing * 0.1);
        check("pursuit_bass_boost_fades_at_higher_fundamentals", higherWing < bassWing * 0.4);
        check("pursuit_upper_harmonic_keeps_its_width", std::abs(modelSideRatio(*bass, 375.0, 2)
            - modelSideRatio(*higher, 1500.0, 2)) < 0.005);
        check("pursuit_zero_widening_keeps_the_base_hann_support",
            modelSideRatio(*zero, 187.5, 3) == 0 && modelSideRatio(*zero, 375.0, 3) == 0);
        // At this bin centre, 80% needs nine bins for H1 and eight or fewer for H2+.
        check("pursuit_maximum_bass_widening_keeps_both_outer_bins",
            modelSideRatio(*maximum, 234.375, 4) > 0
            && modelSideRatio(*maximum, 234.375, -4) > 0);
    }
    settings = {};
    settings.harmonicComb.minimumComponentDistanceSemitones = -1;
    check("pursuit_separation_configuration_guard", engine->prepare(settings) == PitchMotionResult::invalidConfiguration);
    settings = {};
    settings.noiseFloorStrength = 0;
    settings.minimumPeakProminenceDb = 0;
    engine->prepare(settings);
    engine->process(fixture({{200, 0.15}, {203, 0.14}}));
    bool separated = true;
    for (int i = 0; i < engine->snapshot().componentCount; ++i)
        for (int j = 0; j < i; ++j)
            separated = separated && std::abs(12 * std::log2(engine->snapshot().components[i].frequencyHz
                / engine->snapshot().components[j].frequencyHz)) >= settings.harmonicComb.minimumComponentDistanceSemitones;
    check("pursuit_markers_respect_minimum_separation", separated);
    engine->prepare();
    frame.power.assign(1025, 0);
    check("pursuit_silence_clears_components", engine->process(frame) == PitchMotionResult::noSignal
        && engine->snapshot().componentCount == 0 && engine->snapshot().inputSpectrumCount > 0);
    frame.power[10] = std::numeric_limits<float>::quiet_NaN();
    check("pursuit_invalid_frame_clears_components_and_diagnostics", engine->process(frame) == PitchMotionResult::nonFiniteInput
        && engine->snapshot().componentCount == 0 && engine->snapshot().inputSpectrumCount == 0);
    {
        HarmonicMarkerTrail trail;
        auto observation = std::make_unique<HarmonicCombSnapshot>();
        observation->hasEstimate = true; observation->generation = 1;
        observation->componentCount = 1; observation->centreTimeSeconds = 1;
        observation->components[0] = {220, 0.1, 0.1, 0.1, 1};
        trail.observe(*observation, 10);
        check("harmonic_trail_preserves_position_and_amplitude", trail.count() == 1
            && trail.marker(0).frequencyHz == 220 && trail.marker(0).amplitude == 0.1);
        trail.observe(*observation, 10.2);
        check("harmonic_trail_duplicate_publication_does_not_refresh", trail.marker(0).time == 10
            && std::abs(HarmonicMarkerTrail::opacity(trail.marker(0), 10.2, 0.2) - 0.5) < 1e-12);
        observation->centreTimeSeconds += 0.01;
        trail.observe(*observation, 10.2);
        check("harmonic_trail_steady_note_uses_one_display_cell", trail.count() == 1 && trail.marker(0).time == 10.2);
        observation->components[0].frequencyHz = 230;
        observation->centreTimeSeconds += 0.01;
        trail.observe(*observation, 10.3);
        check("harmonic_trail_keeps_old_pitch_without_matching", trail.count() == 2
            && trail.marker(0).frequencyHz == 220 && trail.marker(1).frequencyHz == 230);
        observation->hasEstimate = false; observation->centreTimeSeconds += 0.01;
        trail.observe(*observation, 10.4);
        check("harmonic_trail_silence_fades_without_new_stamps", trail.count() == 2);
        trail.expire(11.31, 0.2);
        check("harmonic_trail_bounded_lifetime", trail.count() == 0);
        observation->hasEstimate = true; observation->centreTimeSeconds += 0.01;
        trail.observe(*observation, 12);
        ++observation->generation; observation->components[0].frequencyHz = 440;
        trail.observe(*observation, 12.1);
        check("harmonic_trail_generation_clears_old_source", trail.count() == 1 && trail.marker(0).frequencyHz == 440);
        observation->centreTimeSeconds = 0; observation->components[0].frequencyHz = 330;
        trail.observe(*observation, 12.2);
        check("harmonic_trail_seek_back_clears_old_source", trail.count() == 1 && trail.marker(0).frequencyHz == 330);
        for (int i = 0; i < HarmonicMarkerTrail::capacity + 30; ++i)
        {
            observation->centreTimeSeconds += 0.01; observation->components[0].frequencyHz = 500 + i;
            trail.observe(*observation, 12.3 + i * 0.001);
        }
        check("harmonic_trail_fixed_memory_capacity", trail.count() == HarmonicMarkerTrail::capacity);
        observation->trailHalfLifeSeconds = 0;
        trail.observe(*observation, 13);
        check("harmonic_trail_zero_half_life_disables", trail.count() == 0);
        check("harmonic_carrier_defaults", HarmonicCombSettings{}.displayGain == 30
            && HarmonicCombSettings{}.harmonicDecay == 1.3 && HarmonicCombSettings{}.maximumComponents == 4
            && HarmonicCombSettings{}.harmonicWideningPercent == 50 && HarmonicCombSettings{}.trailHalfLifeSeconds == 0.2
            && observation->harmonicWideningPercent == 50);
    }
    for (double phaseCurvature : {100.0, 1000.0})
    {
        // Independent PCM chirps: exercise useful broadening without synthesizing
        // the same magnitude kernel as the implementation.
        std::vector<float> pcm(8192 * 2);
        for (int i = 0; i < 8192; ++i)
        {
            const double t = i / 48000.0, phase = 2 * pi * (1200 * t + phaseCurvature * t * t);
            double value = 0;
            for (int h = 1; h <= 8; ++h) value += 0.04 / std::sqrt(static_cast<double>(h)) * std::sin(h * phase);
            pcm[2 * i] = static_cast<float>(value); pcm[2 * i + 1] = -pcm[2 * i];
        }
        struct ChirpSink final : SpectrumSink
        {
            HarmonicCombProfile& engine;
            double error = 0;
            double wideningPercent;
            bool usesSelectedWidth = true;
            bool valid = true;
            ChirpSink(HarmonicCombProfile& e, double width) : engine(e), wideningPercent(width) {}
            void onSpectrum(const SpectrumFrame& f) override
            {
                engine.process(f);
                const auto& s = engine.snapshot();
                valid = valid && s.hasEstimate && s.reconstructionError < 1e-8 && s.fitKktResidual < 1e-6;
                error += s.residualErrorEnergy;
                for (int i = 0; i < s.componentCount; ++i)
                    usesSelectedWidth &= s.components[i].wideningPercent == wideningPercent;
            }
        };
        double sharpError = 0;
        bool sharpValid = false;
        for (double spread : {0.0, harmonicMaximumWideningPercent})
        {
            PitchMotionSettings chirpSettings;
            chirpSettings.noiseFloorStrength = 0; chirpSettings.minimumPeakProminenceDb = 0;
            chirpSettings.harmonicComb.highResReference = false;
            chirpSettings.harmonicComb.maximumHarmonics = 8;
            chirpSettings.harmonicComb.maximumComponents = 1;
            chirpSettings.harmonicComb.harmonicWideningPercent = spread;
            engine->prepare(chirpSettings);
            AudioAnalyzer analyzer;
            analyzer.prepare({48000, 2, 2048, 512});
            ChirpSink sink(*engine, spread);
            analyzer.processBlock(pcm.data(), 8192, 0, sink);
            std::cerr << "harmonic PCM chirp rate=" << 2 * phaseCurvature << " Hz/s spread=" << spread
                << " error=" << sink.error << '\n';
            if (spread == 0) { sharpError = sink.error; sharpValid = sink.valid && sink.usesSelectedWidth; }
            else if (phaseCurvature == 100.0)
                check("pursuit_slow_chirp_uses_slider_widening", sharpValid && sink.valid && sink.usesSelectedWidth);
            else check("pursuit_fast_chirp_uses_slider_widening", sharpValid && sink.valid
                && sink.usesSelectedWidth && sink.error < sharpError * 0.9999);
        }
    }
    // Exercise the production short FFT at two rates. Count actual detected
    // notes: receiving any estimate is insufficient to check the lower edge.
    engine->prepare();
    for (double rate : {44100.0, 48000.0})
    {
        AudioAnalyzer analyzer;
        analyzer.prepare({rate, 2, 2048, 512});
        for (double hz : {80.0, 100.0, 125.0, 196.0, 300.0})
        {
            const auto pcm = tone(6144, hz, 1.0f, rate);
            Frames captured;
            analyzer.reset();
            engine->reset();
            analyzer.processBlock(pcm.data(), 6144, 0, captured);
            int matches = 0, bassFrames = 0;
            double firstCarrier = 0.0;
            for (const auto& observation : captured.frames)
            {
                engine->process(observation);
                const auto& s = engine->snapshot();
                if (&observation == &captured.frames.front()) firstCarrier = s.selectedFrequencyHz;
                bool found = false, inBass = false;
                for (int i = 0; i < s.componentCount; ++i)
                {
                    const double f = s.components[i].frequencyHz;
                    found |= std::abs(f - hz) < 3.0;
                    inBass |= f <= 300.0;
                }
                matches += found; bassFrames += inBass;
            }
            const auto name = "carrier_pcm_sine_" + std::to_string(static_cast<int>(hz))
                + "_at_" + std::to_string(static_cast<int>(rate));
            const int frames = static_cast<int>(captured.frames.size());
            std::cerr << name << ": first=" << firstCarrier << ", matching=" << matches << ", in bass=" << bassFrames << "/" << frames << '\n';
            check(name.c_str(), frames == 9 && (hz < 100 ? bassFrames == 0 : matches == frames));
        }
    }
    for (double fundamental : {150.0, 3000.0})
    {
        engine->prepare();
        struct Sink final : SpectrumSink
        {
            HarmonicCombProfile& engine;
            double fundamental;
            int frames = 0, recovered = 0;
            bool good = true;
            Sink(HarmonicCombProfile& value, double hz) : engine(value), fundamental(hz) {}
            void onSpectrum(const SpectrumFrame& input) override
            {
                const auto result = engine.process(input);
                const auto& s = engine.snapshot();
                good = good && result == PitchMotionResult::ok && s.componentCount > 0 && s.componentCount <= 4
                    && s.fitKktResidual < 1e-6 && s.reconstructionError < 1e-8 && s.residualErrorEnergy <= s.inputEnergy;
                bool nearFundamental = false;
                for (int i = 0; i < s.componentCount; ++i)
                    nearFundamental = nearFundamental || std::abs(12 * std::log2(s.components[i].frequencyHz / fundamental)) < 0.5;
                if (nearFundamental) ++recovered;
                ++frames;
            }
        } sink(*engine, fundamental);
        AudioAnalyzer analyzer;
        const bool prepared = analyzer.prepare({48000, 2, 2048, 512}) == AnalysisResult::ok;
        std::vector<float> pcm(8192 * 2);
        for (int i = 0; i < 8192; ++i)
        {
            double value = 0;
            for (int h = 1; h <= 64 && h * fundamental <= 20000; ++h)
                value += 0.05 / std::sqrt(static_cast<double>(h)) * std::sin(2 * pi * h * fundamental * i / 48000 + 0.17 * h);
            pcm[2 * i] = static_cast<float>(value);
            pcm[2 * i + 1] = -pcm[2 * i];
        }
        const auto originalPcm = pcm;
        const bool processed = prepared && analyzer.processBlock(pcm.data(), 8192, 0, sink) == AnalysisResult::ok;
        // The rich 150 Hz fixture measures solver validity. Its fundamental-recovery count is diagnostic.
        const auto name = "pursuit_rich_pcm_fit_valid_" + std::to_string(static_cast<int>(fundamental));
        check(name.c_str(), processed && sink.good && sink.frames > 8 && pcm == originalPcm);
        if (fundamental == harmonicCarrierMaximumHz)
            check("pursuit_rich_pcm_recovers_upper_endpoint", sink.frames > 0 && sink.recovered == sink.frames);
        std::cerr << "pursuit PCM " << fundamental << " Hz: near-fundamental frames " << sink.recovered << '/' << sink.frames << '\n';
    }
}
}
