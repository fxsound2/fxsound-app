// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../WaterfallRaster.h"
#include "../runtime/AnalysisStream.h"
#include "../experimental/pitch/HarmonicCarrierDisplay.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <thread>

using namespace fxanalysis;

namespace
{
using Check = std::function<void(const char*, bool)>;

void runProfileRasterTests(const Check& check, const WaterfallFrame& format)
{
    WaterfallRaster raster;
    WaterfallRaster::Rows rows;
    auto frame = format;
    frame.hasProfile = true;
    // A live edit applies at the next observation. The previous level and the
    // retained image survive; a departed pixel and a new pixel respond immediately.
    struct Reference { double percent, carrierMs; };
    constexpr Reference references[] = {{0, 0}, {25, 0}, {37.5, 8}, {50, 16}, {75, 72}, {100, 128}};
    for (const auto& reference : references)
    {
        const auto percent = reference.percent;
        auto changed = frame;
        WaterfallRaster plain, tinted(0xffffbe5c);
        for (auto* renderer : {&plain, &tinted})
        {
            changed.time = format.time;
            changed.displayTimingPercent = 100 - percent;
            changed.profile = {};
            changed.profile[0] = .8f; changed.profile[1] = .7f;
            const bool started = renderer->process(changed, false, rows) && rows.reset;
            changed.time += changed.hopSize / changed.sampleRate;
            changed.displayTimingPercent = percent;
            changed.profile[0] = .2f; changed.profile[1] = 0; changed.profile[2] = .65f;
            const double carrierMs = reference.carrierMs;
            const double retained = carrierMs > 0 ? std::exp(-changed.hopSize / changed.sampleRate * 1000.0 / carrierMs) : 0.0;
            const auto name = std::string(renderer == &plain ? "carrier_history_" : "carrier_overlay_")
                + std::to_string(percent) + "_live_timing";
            check(name.c_str(), started && renderer->process(changed, false, rows) && !rows.reset
                && std::abs(renderer->currentLevels()[0] - (.2 + .6 * retained)) < 1e-6
                && renderer->currentLevels()[1] == 0 && renderer->currentLevels()[2] == changed.profile[2]);
        }
    }
    frame.profile[150] = .2f; frame.profile[151] = .8f;
    const double hop = frame.hopSize / frame.sampleRate;
    check("carrier_raster_accepts_all_300_profile_columns", raster.process(frame, false, rows)
        && rows.reset && raster.currentLevels() == frame.profile && rows.current == rows.midpoint
        && raster.rowSeconds() == hop / 2);
    frame.time += hop;
    frame.profile[150] = .6f; frame.profile[151] = .4f;
    const auto valid = frame;
    frame.profile[40] = std::numeric_limits<float>::quiet_NaN();
    check("carrier_raster_rejects_nonfinite_profile", !raster.process(frame, false, rows));
    frame = valid; frame.profile[40] = 1.01f;
    check("carrier_raster_rejects_profile_above_one", !raster.process(frame, false, rows));
    frame.profile[40] = -.01f;
    check("carrier_raster_rejects_negative_profile", !raster.process(frame, false, rows));
    frame = valid;
    const float retention = static_cast<float>(std::exp(-hop / DisplayTiming::seconds(DisplayTiming::atPercent(frame.displayTimingPercent).carrierLevelMs)));
    check("carrier_raster_invalid_profile_does_not_advance_time", raster.process(frame, false, rows)
        && !rows.reset && std::abs(raster.currentLevels()[150] - (.6f - .4f * retention)) < 1e-6f
        && std::abs(raster.currentLevels()[151] - (.4f + .4f * retention)) < 1e-6f);
    frame.time += hop;
    frame.profile.fill(0.0f); frame.profile[180] = .9f;
    check("carrier_raster_moving_note_has_immediate_onset_and_no_departed_lobe",
        raster.process(frame, false, rows) && !rows.reset && raster.currentLevels() == frame.profile
        && rows.current[150] == WaterfallRaster::background && rows.current[180] != WaterfallRaster::background);
    frame.time += hop; ++frame.sourceRevision;
    frame.profile[180] = .1f;
    check("carrier_raster_setting_revision_restarts_interpolation", raster.process(frame, false, rows)
        && rows.reset && rows.current == rows.midpoint && raster.currentLevels() == frame.profile);
    frame.time += hop; frame.hasProfile = false;
    frame.levels.left.fill(.3f); frame.levels.right.fill(.3f);
    check("carrier_raster_source_change_restarts_interpolation", raster.process(frame, false, rows)
        && rows.reset && rows.current == rows.midpoint);
    frame.time += hop; frame.hasProfile = true; frame.profile.fill(0.0f);
    check("carrier_raster_empty_observation_is_dark", raster.process(frame, true, rows)
        && rows.reset && std::all_of(rows.current.begin(), rows.current.end(),
            [](std::uint32_t pixel) { return pixel == WaterfallRaster::background; }));

    constexpr std::uint32_t gold = 0xffffbe5c;
    WaterfallRaster overlay(gold);
    frame.profile[150] = .02f;
    frame.profile[151] = .18f;
    frame.profile[152] = .3f;
    check("carrier_overlay_masks_spectrum_at_the_core_with_soft_edges", overlay.process(frame, false, rows)
        && rows.current[149] == 0
        && (rows.current[150] >> 24) > 0 && (rows.current[150] >> 24) < 32
        && (rows.current[151] >> 24) >= 224 && rows.current[152] == gold);

    // Independently condition uniform amplitudes in time. Given the same resulting
    // levels, the two sources must still agree on every shaded and corrected row.
    WaterfallRaster bandRaster, profileRaster;
    WaterfallRaster::Rows bandRows, profileRows;
    bool identical = true;
    auto bands = format;
    float expected = 0.0f;
    for (int n = 0; n < 8; ++n)
    {
        bands.time = n * hop;
        const float input = n % 3 == 0 ? .8f : .2f;
        expected = n == 0 ? input : input + retention * (expected - input);
        bands.levels.left.fill(expected);
        bands.levels.right.fill(expected);
        auto profile = bands;
        profile.hasProfile = true;
        profile.profile.fill(input);
        identical &= bandRaster.process(bands, true, bandRows) && profileRaster.process(profile, true, profileRows)
            && bandRows.current == profileRows.current && bandRows.midpoint == profileRows.midpoint
            && bandRows.reset == profileRows.reset && bandRaster.rowSeconds() == profileRaster.rowSeconds()
            && bandRaster.correctedRowCount() == profileRaster.correctedRowCount();
        for (int row = 0; identical && row < bandRaster.correctedRowCount(); ++row)
            identical &= bandRaster.correctedRows()[row] == profileRaster.correctedRows()[row];
    }
    check("carrier_raster_reuses_exact_fft_lighting_and_scrolling", identical);
}

void runCarrierProfileTests(const Check& check)
{
    using namespace fxanalysis::experimental;
    auto observation = std::make_unique<HarmonicCombSnapshot>();
    auto& snapshot = *observation;
    snapshot.hasEstimate = true; snapshot.enabled = true; snapshot.iterativeExtraction = true;
    snapshot.maximumFrequencyHz = 20000; snapshot.displayGain = 8; snapshot.displayRangeDb = 60;
    snapshot.componentCount = 2;
    snapshot.components[0] = {375, 1.0, .05};
    snapshot.components[1] = {390, 2.0, .025};
    const FrequencyScale scale(20000);
    const double ratio = std::exp2(.5 / 12);
    for (bool logarithmic : {false, true})
    {
        snapshot.logarithmicDisplay = logarithmic;
        const auto actual = harmonicCarrierProfile(snapshot);
        double maximumError = 0.0;
        for (int x = 0; x < WaterfallSettings::width; ++x)
        {
            double expected = 0.0;
            for (int i = 0; i < snapshot.componentCount; ++i)
            {
                const auto& component = snapshot.components[i];
                const double scaled = component.displayAmplitude * snapshot.displayGain;
                const double height = std::clamp(logarithmic
                    ? 1 + 20 * std::log10(scaled) / snapshot.displayRangeDb : scaled, 0.0, 1.0);
                const double centre = scale.position(component.frequencyHz) * WaterfallSettings::width;
                const double sigma = .4 * (std::max)(5.0, .5 * WaterfallSettings::width
                    * (scale.position(component.frequencyHz * ratio) - scale.position(component.frequencyHz / ratio)));
                const double distance = (x + .5 - centre) / sigma;
                if (std::abs(distance) <= 3)
                    expected = (std::max)(expected, height * std::exp(-.5 * distance * distance));
            }
            maximumError = (std::max)(maximumError, std::abs(actual[x] - expected));
        }
        check(logarithmic ? "carrier_gaussian_db_height_precedes_envelope" : "carrier_gaussian_matches_linear_envelope",
            maximumError < 2e-7);
        snapshot.highResReference = false;
        check(logarithmic ? "carrier_db_does_not_apply_reference_twice" : "carrier_linear_does_not_apply_reference_twice",
            harmonicCarrierProfile(snapshot) == actual);
        snapshot.highResReference = true;
    }
    snapshot.logarithmicDisplay = false;
    snapshot.components[1].frequencyHz = snapshot.components[0].frequencyHz;
    const auto overlap = harmonicCarrierProfile(snapshot);
    snapshot.componentCount = 1;
    check("carrier_overlapping_gaussians_use_maximum_not_sum", harmonicCarrierProfile(snapshot) == overlap);
    const int harmonicColumn = static_cast<int>(scale.position(750) * WaterfallSettings::width);
    check("carrier_profile_contains_carrier_without_synthetic_harmonics", overlap[harmonicColumn] == 0.0f);
    bool boundaryAlignment = true;
    for (double hz : {100.0, 3000.0})
    {
        snapshot.components[0].frequencyHz = hz;
        const auto profile = harmonicCarrierProfile(snapshot);
        const auto peak = std::max_element(profile.begin(), profile.end());
        const double centre = scale.position(hz) * WaterfallSettings::width;
        // The nearest pixel centre can lie half a pixel from the peak.
        const double minimumPixelHeight = .4 * std::exp(-.5 * std::pow(.5 / 2.0, 2));
        boundaryAlignment &= *peak >= minimumPixelHeight - 1e-6
            && std::abs(static_cast<double>(peak - profile.begin()) + .5 - centre) <= .500001;
    }
    check("carrier_search_boundaries_align_with_shared_frequency_axis", boundaryAlignment);
    const auto isDark = [](const WaterfallProfile& profile) {
        return std::all_of(profile.begin(), profile.end(), [](float level) { return level == 0.0f; });
    };
    snapshot.hasEstimate = false;
    check("carrier_profile_no_estimate_is_empty", isDark(harmonicCarrierProfile(snapshot)));
    snapshot.hasEstimate = true; snapshot.enabled = false;
    check("carrier_profile_disabled_is_empty", isDark(harmonicCarrierProfile(snapshot)));
    snapshot.enabled = true; snapshot.iterativeExtraction = false;
    check("carrier_profile_legacy_mode_is_empty", isDark(harmonicCarrierProfile(snapshot)));
    snapshot.iterativeExtraction = true; snapshot.componentCount = 0;
    check("carrier_profile_silence_does_not_retain_previous_note", isDark(harmonicCarrierProfile(snapshot)));
}

void runCarrierStreamTests(const Check& check)
{
    using namespace fxanalysis::experimental;
    auto owner = std::make_unique<AnalysisStream>();
    auto& stream = *owner;
    PitchMotionSettings settings;
    settings.noiseFloorStrength = 0; settings.minimumPeakProminenceDb = 0;
    settings.harmonicComb.maximumComponents = 1; settings.harmonicComb.maximumHarmonics = 1;
    settings.harmonicComb.harmonicWideningPercent = 0; settings.harmonicComb.highResReference = false;
    settings.harmonicComb.displayGain = 2;
    stream.setPitchMotionSettings(settings);
    AnalysisRequests requests;
    requests.mainSpectrum = requests.fftHistory = requests.carrierHistory = true;
    stream.setRequests(requests); // Carrier history alone requests the harmonic detector.
    constexpr int count = 4096, hopSamples = 512, referenceDelay = 2048;
    std::vector<float> pcm(count * 2);
    for (int i = 0; i < count; ++i)
        pcm[2 * i] = pcm[2 * i + 1] = .25f * static_cast<float>(std::sin(2 * 3.141592653589793 * 375 * i / 48000));
    const auto original = pcm;
    std::uint64_t nextSample = count;
    const auto waitFor = [&](std::uint64_t first)
    {
        SpectrumFrame position;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (stream.latest(position) && position.firstSample == first) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return false;
    };
    const auto pushHop = [&]
    {
        const bool accepted = stream.push(pcm.data(), hopSamples, 2, 48000, nextSample);
        nextSample += hopSamples;
        return accepted && waitFor(nextSample - referenceDelay);
    };
    const bool ready = stream.push(pcm.data(), count, 2, 48000, 0) && waitFor(count - referenceDelay);
    std::vector<WaterfallFrame> fft, carriers;
    const auto pushFreshObservation = [&]
    {
        // A reopened consumer waits for one complete public 2048-sample window.
        // Drain the three warm-up hops explicitly; the ongoing consumers remain live.
        bool accepted = true;
        for (int i = 0; i < 3; ++i)
        { accepted = pushHop() && accepted; stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers); }
        return pushHop() && accepted;
    };
    stream.drainWaterfall(fft);
    stream.drainCarrierWaterfall(carriers);
    bool aligned = ready && fft.size() == 5 && carriers.size() == fft.size();
    bool detected = false;
    for (int i = 0; aligned && i < static_cast<int>(fft.size()); ++i)
    {
        const auto& a = fft[i]; const auto& b = carriers[i];
        aligned &= !a.hasProfile && b.hasProfile && a.frequencies == b.frequencies && a.time == b.time
            && a.hopSize == b.hopSize && a.sampleRate == b.sampleRate && a.revision == b.revision
            && (i == 0 || std::abs(b.time - carriers[i - 1].time - 512.0 / 48000) < 1e-12);
        detected |= *std::max_element(b.profile.begin(), b.profile.end()) > .1f;
    }
    check("carrier_stream_autonomous_pitch_request_publishes_each_fft", aligned && detected && pcm == original
        && stream.droppedFrames() == 0);
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("carrier_stream_independent_queues_do_not_repeat_reads", fft.empty() && carriers.empty());

    requests.fftHistory = false; stream.setRequests(requests);
    bool continued = pushHop();
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("carrier_stream_closing_fft_history_keeps_carriers", continued && fft.empty() && carriers.size() == 1);
    requests.carrierHistory = false; requests.fftHistory = true; stream.setRequests(requests);
    continued = pushFreshObservation();
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("carrier_stream_closing_carriers_keeps_fft_history", continued && fft.size() == 1 && carriers.empty());

    requests.fftCarrierOverlay = true; stream.setRequests(requests);
    continued = pushFreshObservation();
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("carrier_overlay_alone_requests_pitch_and_keeps_fft_bands", continued && fft.size() == 1
        && !fft[0].hasProfile && fft[0].carrierCapture != 0 && carriers.empty()
        && *std::max_element(fft[0].profile.begin(), fft[0].profile.end()) > .1f);

    requests.carrierHistory = true; stream.setRequests(requests);
    continued = pushFreshObservation();
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("combined_and_standalone_share_the_same_observation", continued && fft.size() == 1
        && carriers.size() == 1 && fft[0].time == carriers[0].time && fft[0].profile == carriers[0].profile);

    continued = pushHop();
    continued = pushHop() && continued;
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("queued_fft_and_carrier_views_use_documented_fixed_display_timing",
        continued && fft.size() == 2 && carriers.size() == 2 && fft[0].displayTimingPercent == DisplayTiming::defaultPercent
        && fft[1].displayTimingPercent == DisplayTiming::defaultPercent
        && carriers[0].displayTimingPercent == DisplayTiming::defaultPercent && carriers[1].displayTimingPercent == DisplayTiming::defaultPercent
        && fft[0].carrierCapture != 0 && fft[1].carrierCapture != 0 && fft[1].revision == fft[0].revision);

    continued = pushHop();
    requests.fftCarrierOverlay = false; stream.setRequests(requests);
    requests.fftCarrierOverlay = true; stream.setRequests(requests);
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("carrier_toggle_discards_stale_overlay_but_keeps_its_spectrum", continued && fft.size() == 1
        && fft[0].carrierCapture == 0 && carriers.size() == 1);

    continued = pushFreshObservation();
    settings.harmonicComb.displayGain = 4;
    stream.setPitchMotionSettings(settings);
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("carrier_stream_settings_hide_queued_old_profiles", continued && carriers.empty());
    check("carrier_overlay_settings_preserve_spectrum", continued && fft.size() == 1 && fft[0].carrierCapture == 0);
    continued = pushHop();
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    const bool changedSettings = continued && carriers.size() == 1 && fft.size() == 1
        && fft[0].carrierCapture != 0 && fft[0].profile == carriers[0].profile
        && *std::max_element(carriers[0].profile.begin(), carriers[0].profile.end()) > .8f;
    check("carrier_stream_new_settings_reach_next_observation", changedSettings);

    constexpr int silenceCount = 8192;
    std::vector<float> silence(silenceCount * 2, 0.0f);
    const bool silentAccepted = stream.push(silence.data(), silenceCount, 2, 48000, nextSample);
    nextSample += silenceCount;
    const bool silentReady = silentAccepted && waitFor(nextSample - referenceDelay);
    stream.drainCarrierWaterfall(carriers);
    check("carrier_stream_silence_scrolls_empty_current_observation", silentReady && carriers.size() == 16
        && carriers.back().hasProfile && std::all_of(carriers.back().profile.begin(), carriers.back().profile.end(),
            [](float level) { return level == 0.0f; }));
    continued = pushHop();
    stream.invalidate();
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("carrier_stream_invalidation_rejects_both_queued_sources", continued && fft.empty() && carriers.empty());
}
}

void runWaterfallTests(const std::function<void(const char*, bool)>& check)
{
    WaterfallFrame frame;
    frame.sampleRate = 48000; frame.hopSize = 512; frame.channels = 2;
    const double hop = frame.hopSize / frame.sampleRate;
    const FrequencyScale scale(20000);
    for (int b = 0; b < displayBandCount; ++b) frame.frequencies[b] = static_cast<float>(scale.frequency((b + 0.5) / displayBandCount));
    runProfileRasterTests(check, frame);
    runCarrierProfileTests(check);
    runCarrierStreamTests(check);
    WaterfallRaster raster;
    WaterfallRaster::Rows rows;
    const auto colour = [](float level) { return WaterfallRaster::colour(std::lround(level * 255) / 255.0f); };
    BandLevels bands{};
    for (int b = 0; b < displayBandCount; ++b) bands[b] = static_cast<float>((b * 13) % 101) / 100;
    const auto interpolated = raster.interpolate(bands);
    bool centres = true, bounded = true;
    for (int b = 0; b < displayBandCount; ++b)
    {
        centres &= interpolated[3 * b + 1] == bands[b];
        if (b == displayBandCount - 1) continue;
        for (int x = 3 * b + 2; x <= 3 * b + 3; ++x)
            bounded &= interpolated[x] >= (std::min)(bands[b], bands[b + 1])
                && interpolated[x] <= (std::max)(bands[b], bands[b + 1]);
    }
    check("waterfall_exactly_300_columns", interpolated.size() == 300);
    check("waterfall_interpolation_preserves_original_centres", centres);
    check("waterfall_interpolation_cannot_overshoot_neighbors", bounded);
    check("waterfall_interpolation_clamps_edge_pixels", interpolated.front() == bands.front() && interpolated.back() == bands.back());
    bands.fill(.5f);
    const auto uniform = raster.interpolate(bands);
    check("waterfall_interpolation_preserves_constant_field", std::all_of(uniform.begin(), uniform.end(), [](float v) { return v == .5f; }));
    frame.levels.left[25] = .8f; frame.levels.right[80] = .6f;
    check("waterfall_first_frame_immediately_visible", raster.process(frame, false, rows) && rows.reset
        && rows.current[76] == colour(.8f) && rows.current[241] == colour(.6f) && rows.current == rows.midpoint);
    check("waterfall_half_hop_time_scale", std::abs(raster.rowSeconds() - 512.0 / 48000 / 2) < 1e-12);
    frame.time += hop; frame.levels.left[25] = .2f; frame.levels.right[80] = .8f;
    raster.process(frame, false, rows);
    check("waterfall_each_fft_produces_current_and_midpoint", !rows.reset && rows.current[76] == colour(.2f)
        && rows.midpoint[76] == colour(.5f) && rows.midpoint[241] == colour(.7f));
    frame.time += hop; frame.levels.left[25] = .9f; frame.levels.right[25] = .1f;
    raster.process(frame, false, rows);
    check("waterfall_stereo_display_uses_stronger_channel", rows.current[76] == colour(.9f));
    frame.time += hop;
    const auto valid = frame;
    frame.levels.left[0] = std::numeric_limits<float>::quiet_NaN();
    check("waterfall_invalid_frame_rejected", !raster.process(frame, false, rows));
    frame = valid;
    check("waterfall_rejected_frame_does_not_advance_state", raster.process(frame, false, rows) && !rows.reset);
    const auto restarts = [&] { return raster.process(frame, false, rows) && rows.reset && rows.current == rows.midpoint; };
    frame.time += 1;
    check("waterfall_gap_restarts_without_false_interpolation", restarts());
    frame.time = 0;
    check("waterfall_backward_seek_restarts", restarts());
    frame.time += hop; ++frame.generation;
    check("waterfall_generation_change_restarts", restarts());
    frame.time += hop; ++frame.revision;
    check("waterfall_revision_change_restarts", restarts());
    frame.time += hop; frame.discontinuity = true;
    check("waterfall_explicit_discontinuity_restarts", restarts());
    frame.discontinuity = false; frame.time += hop; frame.frequencies[50] += .1f;
    check("waterfall_axis_change_restarts", restarts() && raster.frequencies() == frame.frequencies);
    frame.time += hop; frame.channels = 1;
    check("waterfall_channel_change_restarts", restarts());
    frame.sampleRate = 44100; frame.time += 512.0 / 44100;
    check("waterfall_rate_change_updates_row_duration", restarts() && std::abs(raster.rowSeconds() - 256.0 / 44100) < 1e-12);
    frame.hopSize = 1024; frame.time += 1024.0 / 44100;
    check("waterfall_hop_change_updates_row_duration", restarts() && std::abs(raster.rowSeconds() - 512.0 / 44100) < 1e-12);
    frame.sampleRate = 8000;
    check("waterfall_slow_format_still_has_two_rows_per_fft", restarts() && raster.rowSeconds() == .064);
    frame.sampleRate = 48000; frame.hopSize = 512; frame.channels = 2;
    frame.levels.left.fill(.5f); frame.levels.right.fill(.5f);
    raster.process(frame, false, rows);
    const auto plain = rows.current;
    raster.process(frame, true, rows);
    check("waterfall_shading_change_restarts", rows.reset);
    check("waterfall_uniform_field_has_no_artificial_relief", rows.current == plain && rows.midpoint == plain);
    check("waterfall_palette_fixed_not_autoscaled", rows.current[0] == colour(.5f));
    frame.time += hop; frame.levels = {};
    raster.process(frame, true, rows);
    check("waterfall_silence_stays_dark_with_relief", std::all_of(rows.current.begin(), rows.current.end(),
        [](std::uint32_t p) { return p == WaterfallRaster::background; }));
    frame.time += hop; frame.levels.left.fill(.3f); frame.levels.right.fill(.3f);
    raster.reset(); raster.process(frame, true, rows);
    frame.time += hop; frame.levels.left[30] = 1;
    raster.process(frame, true, rows);
    const auto crest = raster.interpolate(frame.levels.left);
    bool relief = false;
    for (int x = 0; x < WaterfallSettings::width; ++x) relief |= rows.current[x] != colour(crest[x]);
    check("waterfall_shading_changes_peak_relief", relief);
    frame.levels.left.fill(.3f);
    for (int i = 0; i < 6; ++i) { frame.time += hop; raster.process(frame, true, rows); }
    check("waterfall_shadow_does_not_propagate_up_into_newer_rows", rows.current[103] == colour(.3f));
    frame.time += hop; frame.discontinuity = true;
    raster.process(frame, true, rows);
    check("waterfall_discontinuity_clears_shadow", rows.current[103] == colour(.3f));
    frame.discontinuity = false;

    // New rows appear above old ones. At the same final height, an ascent in
    // time faces away from top light; a descent faces toward it. Uniform bands
    // isolate Y from the frequency gradient and compare actual output light.
    const auto brightness = [](std::uint32_t pixel) {
        return static_cast<int>((pixel >> 16) & 255) + static_cast<int>((pixel >> 8) & 255)
            + static_cast<int>(pixel & 255);
    };
    const auto temporalFace = [&](float previousLevel) {
        WaterfallRaster directionRaster;
        WaterfallRaster::Rows directionRows;
        auto directionFrame = frame;
        directionFrame.discontinuity = false;
        directionFrame.levels.left.fill(previousLevel);
        directionFrame.levels.right.fill(previousLevel);
        directionRaster.process(directionFrame, true, directionRows);
        directionFrame.time += hop;
        directionFrame.levels.left.fill(.5f);
        directionFrame.levels.right.fill(.5f);
        directionRaster.process(directionFrame, true, directionRows);
        return brightness(directionRows.current[150]);
    };
    check("waterfall_top_light_shades_temporal_ascent", temporalFace(.3f) < brightness(colour(.5f)));
    check("waterfall_top_light_highlights_temporal_descent", temporalFace(.7f) > brightness(colour(.5f)));
    // An isolated uniform ascent ends at the same palette level for every case.
    // Two half-hop rows give q = timeRelief * delta/2 * (1 + exp(-row/tau)).
    // This analytic fixture separates the shadow threshold from palette changes.
    const double temporalRetention = std::exp(-hop / WaterfallSettings::linesPerFrame / WaterfallStyle::temporalSeconds);
    const double profileRetention = std::exp(-hop / DisplayTiming::seconds(DisplayTiming::atPercent(frame.displayTimingPercent).carrierLevelMs));
    const double attackFactor = WaterfallStyle::timeRelief * .5 * (1 + temporalRetention);
    const auto uniformAscent = [&](float rawShadow) {
        WaterfallRaster testRaster;
        WaterfallRaster::Rows testRows;
        auto testFrame = frame;
        testFrame.discontinuity = false;
        const float initial = .6f - static_cast<float>(rawShadow / attackFactor);
        testFrame.levels.left.fill(initial); testFrame.levels.right.fill(initial);
        testRaster.process(testFrame, true, testRows);
        testFrame.time += hop;
        testFrame.levels.left.fill(.6f); testFrame.levels.right.fill(.6f);
        testRaster.process(testFrame, true, testRows);
        return testRows.current[150];
    };
    const auto litColour = [&](double light, float level = .6f) {
        // Independent sRGB calculation, including the existing 1/64 light LUT.
        light = 1.0 + .70 * (std::round(light * 64) / 64 - 1.0);
        const auto base = colour(level);
        std::uint32_t result = 0xff000000;
        for (int shift = 0; shift <= 16; shift += 8)
        {
            const double c = ((base >> shift) & 255) / 255.0;
            const double linear = c <= .04045 ? c / 12.92 : std::pow((c + .055) / 1.055, 2.4);
            const double shaded = (std::min)(1.0, linear * light);
            const double encoded = shaded <= .0031308 ? 12.92 * shaded : 1.055 * std::pow(shaded, 1 / 2.4) - .055;
            result |= static_cast<std::uint32_t>(std::lround(255 * encoded)) << shift;
        }
        return result;
    };
    const float reference = WaterfallStyle::shadowReferenceSlope;
    check("waterfall_reference_keeps_weak_ascent_neutral", uniformAscent(reference * .5f) == colour(.6f));
    check("waterfall_reference_boundary_is_neutral", uniformAscent(reference) == colour(.6f));
    bool transfer = true, monotonic = true;
    int previousBrightness = brightness(colour(.6f));
    for (float q : {.0f, .15f, .29f, .30f, .31f, .4f, .7f, 1.0f, 1.35f})
    {
        const double effective = (std::min)(1.0, (std::max)(0.0, static_cast<double>(q) - reference));
        const double light = (std::max)(static_cast<double>(WaterfallStyle::minimumLight), 1 - WaterfallStyle::shadow * effective);
        const auto actual = uniformAscent(q);
        transfer &= actual == litColour(light);
        monotonic &= brightness(actual) <= previousBrightness;
        previousBrightness = brightness(actual);
    }
    check("waterfall_reference_matches_independent_linear_light_transfer", transfer);
    check("waterfall_reference_shadow_increases_monotonically", monotonic);
    check("waterfall_reference_strong_ascent_reaches_spectrum_shadow_limit",
        uniformAscent(1.35f) == litColour(WaterfallStyle::minimumLight));
    check("waterfall_reference_no_colour_jump_just_above_threshold",
        uniformAscent(reference + .001f) == colour(.6f));
    // Positive temporal relief has no shadow in this independent descent fixture.
    {
        WaterfallRaster testRaster;
        WaterfallRaster::Rows testRows;
        auto testFrame = frame; testFrame.discontinuity = false;
        testFrame.levels.left.fill(.62f); testFrame.levels.right.fill(.62f);
        testRaster.process(testFrame, true, testRows);
        testFrame.time += hop;
        testFrame.levels.left.fill(.6f); testFrame.levels.right.fill(.6f);
        testRaster.process(testFrame, true, testRows);
        const double slope = attackFactor * .02;
        const double light = 1 + WaterfallStyle::highlight * slope * std::exp(-slope / WaterfallStyle::specularPeakSlope);
        check("waterfall_specular_small_descent_matches_analytic_response", testRows.current[150] == litColour(light));
    }
    // At column 94 both interpolated left samples lie on the two-pixel crest.
    // The outgoing ray starts with a frequency difference of .5*(.3-1).
    {
        WaterfallRaster testRaster;
        WaterfallRaster::Rows testRows;
        auto testFrame = frame; testFrame.discontinuity = false;
        testFrame.hasProfile = true; testFrame.profile.fill(.3f);
        for (int n = 0; n < 32; ++n) { testFrame.time += hop; testRaster.process(testFrame, true, testRows); }
        testFrame.time += hop;
        testFrame.profile[90] = testFrame.profile[91] = 1;
        testRaster.process(testFrame, true, testRows);
        const auto* image = testRaster.correctedRows();
        check("waterfall_shadow_reads_above_left_neighbor", image[2][96] != colour(.3f));
        check("waterfall_shadow_does_not_cast_below_left", image[7][85] == colour(.3f));
        check("waterfall_shadow_does_not_cast_above_right", image[0][101] == colour(.3f));
        const double seed = .5 * (1.0 - .3) * (1.0 - profileRetention) * WaterfallStyle::frequencyRelief;
        const int lastDistance = (std::min)(48, testRaster.correctedRowCount() - 1);
        // Compare retained rows only, and require coverage through the visible tail.
        const double lastRaw = seed * std::exp(-lastDistance * hop / WaterfallSettings::linesPerFrame / WaterfallStyle::spectrumShadowSeconds);
        bool decay = lastDistance >= 2 && lastRaw <= (std::max)(static_cast<double>(reference),
            static_cast<double>(WaterfallStyle::shadowSilenceThreshold));
        for (int d = 2; d <= lastDistance && decay; ++d)
        {
            const double raw = seed * std::exp(-d * hop / WaterfallSettings::linesPerFrame / WaterfallStyle::spectrumShadowSeconds);
            const double light = (std::max)(static_cast<double>(WaterfallStyle::minimumLight),
                1 - WaterfallStyle::shadow * (std::min)(1.0, (std::max)(0.0, raw - reference)));
            decay &= image[d][94 + d] == litColour(light, .3f);
        }
        check("waterfall_shadow_distance_matches_independent_ray_decay", decay);
        testRaster.cropHistory(4);
        testRaster.cropHistory(100);
        check("waterfall_cropped_render_rows_never_reappear", testRaster.correctedRowCount() == 4);
        testFrame.time += hop; testFrame.profile.fill(.3f);
        testRaster.process(testFrame, true, testRows);
        check("waterfall_growth_contains_only_retained_and_new_rows", testRaster.correctedRowCount() == 6);
    }

    {
        WaterfallRaster goldRaster(0xffffbe5c);
        WaterfallRaster::Rows goldRows;
        auto goldFrame = frame;
        goldFrame.hasProfile = true;
        goldFrame.profile.fill(0);
        for (int n = 0; n < 32; ++n) { goldFrame.time += hop; goldRaster.process(goldFrame, true, goldRows); }
        goldFrame.time += hop;
        goldFrame.profile[90] = goldFrame.profile[91] = 1;
        goldRaster.process(goldFrame, true, goldRows);
        // The 5.6-pixel radius samples the complete crest at column 96.
        // On the empty background only the black cast shadow has alpha.
        const auto* goldImage = goldRaster.correctedRows();
        bool decay = goldRaster.correctedRowCount() > 32;
        for (int d : {5, 15, 25, 32})
        {
            const double raw = 1.2 * std::exp(-d * hop / WaterfallSettings::linesPerFrame / WaterfallStyle::carrierShadowSeconds);
            const auto alpha = static_cast<std::uint32_t>(std::lround(96 * std::clamp(raw - reference, 0.0, 1.0)));
            decay &= goldImage[d][96 + d] == (alpha << 24);
        }
        check("waterfall_gold_cast_shadow_uses_full_opacity_scale_and_longer_decay", decay);
        check("waterfall_gold_core_stays_opaque_under_relief", (goldRows.current[90] >> 24) == 255);
    }

    // The horizontal screen gradient is +dx; the vertical screen gradient is
    // -dt. Opposed components cancel, aligned ones add, in both directions.
    {
        bool cancel = true, facing = true, away = true;
        for (int xSign : {-1, 1}) for (int timeSign : {-1, 1})
        {
            WaterfallRaster testRaster;
            WaterfallRaster::Rows testRows;
            auto testFrame = frame; testFrame.discontinuity = false;
            testFrame.hasProfile = true;
            WaterfallProfile finalLevels{};
            constexpr double faceSlope = .4;
            for (int x = 0; x < WaterfallSettings::width; ++x)
                finalLevels[x] = .5f + static_cast<float>(xSign * faceSlope / WaterfallStyle::frequencyRelief)
                    * std::clamp((x - 151) / 3.6f, -1.5f, 1.5f);
            const float delta = static_cast<float>(timeSign * faceSlope / attackFactor);
            for (int x = 0; x < WaterfallSettings::width; ++x) testFrame.profile[x] = finalLevels[x] - delta;
            testRaster.process(testFrame, true, testRows);
            testFrame.time += hop;
            // Compensate the display smoothing to exercise the prescribed face
            // slopes at a fixed final palette level in this lighting test.
            for (int x = 0; x < WaterfallSettings::width; ++x)
                testFrame.profile[x] = finalLevels[x] + static_cast<float>(delta * profileRetention / (1.0 - profileRetention));
            testRaster.process(testFrame, true, testRows);
            const double slope = xSign * faceSlope - timeSign * faceSlope;
            const double light = slope > 0 ? 1 + WaterfallStyle::highlight * slope * std::exp(-slope / WaterfallStyle::specularPeakSlope)
                : (std::max)(static_cast<double>(WaterfallStyle::minimumLight), 1 - WaterfallStyle::shadow * (std::max)(0.0, -slope - reference));
            const bool match = testRows.current[151] == litColour(light, .5f);
            if (xSign == timeSign) cancel &= match;
            else if (xSign > 0) facing &= match;
            else away &= match;
        }
        check("waterfall_screen_gradient_components_cancel", cancel);
        check("waterfall_screen_top_left_facing_gradients_add", facing);
        check("waterfall_screen_bottom_right_away_gradients_add", away);
    }    // Keep the final palette level fixed while varying the positive code slope.
    // No inherited shadow exists on a monotone uniform descent.
    constexpr float descentEndLevel = .06f;
    const auto uniformDescent = [&](float positiveSlope) {
        WaterfallRaster testRaster;
        WaterfallRaster::Rows testRows;
        auto testFrame = frame; testFrame.discontinuity = false;
        testFrame.levels.left.fill(descentEndLevel + static_cast<float>(positiveSlope / attackFactor));
        testFrame.levels.right = testFrame.levels.left;
        testRaster.process(testFrame, true, testRows);
        testFrame.time += hop;
        testFrame.levels.left.fill(descentEndLevel); testFrame.levels.right.fill(descentEndLevel);
        testRaster.process(testFrame, true, testRows);
        return testRows.current[150];
    };
    const float peak = WaterfallStyle::specularPeakSlope;
    bool specularTransfer = true;
    for (float slope : {.0f, .02f, .15f, .3f, .45f, .6f, .75f, .9f, 1.2f, 1.8f, 2.3f})
    {
        const double light = 1 + WaterfallStyle::highlight * slope * std::exp(-slope / peak);
        specularTransfer &= uniformDescent(slope) == litColour(light, descentEndLevel);
    }
    check("waterfall_specular_matches_independent_exponential_curve", specularTransfer);
    check("waterfall_specular_flat_surface_remains_exactly_nominal", uniformDescent(0) == colour(descentEndLevel));
    check("waterfall_specular_rises_before_peak", brightness(uniformDescent(peak)) > brightness(uniformDescent(peak * .5f)));
    check("waterfall_specular_falls_after_peak", brightness(uniformDescent(peak * 2)) < brightness(uniformDescent(peak)));
    check("waterfall_specular_strong_slope_returns_toward_nominal", brightness(uniformDescent(2.3f)) > brightness(colour(descentEndLevel))
        && brightness(uniformDescent(2.3f)) < brightness(uniformDescent(peak * 2)));
    // A spatial ramp exercises the other gradient axis at the analytic maximum.
    {
        WaterfallRaster testRaster;
        WaterfallRaster::Rows testRows;
        auto testFrame = frame; testFrame.discontinuity = false;
        testFrame.hasProfile = true;
        for (int x = 0; x < WaterfallSettings::width; ++x)
            testFrame.profile[x] = .6f + peak / WaterfallStyle::frequencyRelief
                * std::clamp((x - 151) / 3.6f, -1.5f, 1.5f);
        testRaster.process(testFrame, true, testRows);
        check("waterfall_specular_spatial_ramp_peaks_at_requested_slope",
            testRows.current[151] == litColour(1 + WaterfallStyle::highlight * peak / std::exp(1.0)));
    }
    // A periodic input must give the same fully settled image after additional
    // ring rotations. At 384 kHz the diagonal-width cap, not decay, limits state.
    {
        bool periodic = true, widthBound = false;
        for (double rate : {48000.0, 384000.0})
        {
            WaterfallRaster shortRun, longRun;
            WaterfallRaster::Rows output;
            auto testFrame = frame; testFrame.sampleRate = rate; testFrame.discontinuity = false;
            const int firstCount = rate == 48000 ? 204 : 1020;
            const auto run = [&](WaterfallRaster& target, int count) {
                for (int n = 0; n < count; ++n)
                {
                    testFrame.time = n * testFrame.hopSize / rate;
                    for (int b = 0; b < displayBandCount; ++b)
                        testFrame.levels.left[b] = testFrame.levels.right[b] = .15f + .7f * ((n + 3*b) % 17) / 16;
                    target.process(testFrame, true, output);
                }
            };
            run(shortRun, firstCount); run(longRun, firstCount + 340);
            const int count = shortRun.correctedRowCount();
            periodic &= count == longRun.correctedRowCount()
                && std::equal(shortRun.correctedRows(), shortRun.correctedRows() + count, longRun.correctedRows());
            if (rate == 384000)
            {
                WaterfallRaster goldRaster(0xffffbe5c);
                run(goldRaster, firstCount);
                widthBound = count < WaterfallSettings::width + 1
                    && goldRaster.correctedRowCount() == WaterfallSettings::width + 1;
            }
        }
        check("waterfall_periodic_render_survives_multiple_ring_wraps", periodic);
        check("waterfall_high_rate_shadow_reach_stops_at_image_width", widthBound);
    }
    WaterfallInbox inbox;
    std::vector<WaterfallFrame> batch;
    inbox.publish(frame);
    check("waterfall_closed_inbox_discards_input", inbox.drain(batch) == 0 && batch.empty());
    inbox.setEnabled(true);
    for (int i = 0; i < 600; ++i) { frame.time = i; inbox.publish(frame); }
    const auto lost = inbox.drain(batch);
    check("waterfall_overflow_keeps_latest_bounded_data", lost == 88 && batch.size() == 512 && batch.front().time == 88 && batch.back().time == 599);
    check("waterfall_overflow_marks_discontinuity", batch.front().discontinuity);
    inbox.drain(batch);
    check("waterfall_draining_never_repeats_frames", batch.empty());
    inbox.publish(frame); inbox.setEnabled(false); inbox.setEnabled(true); inbox.drain(batch);
    check("waterfall_reopen_discards_obsolete_frames", batch.empty());
    std::atomic<bool> finished{false};
    std::thread producer([&]
    {
        auto f = frame;
        for (int i = 0; i < 20000; ++i) { f.time = i; inbox.publish(f); }
        finished.store(true);
    });
    double last = -1; std::uint64_t observed = 0, dropped = 0; bool ordered = true;
    do
    {
        dropped = inbox.drain(batch);
        for (const auto& f : batch) { ordered &= f.time > last; last = f.time; ++observed; }
    } while (!finished.load() || !batch.empty());
    producer.join();
    dropped = inbox.drain(batch);
    for (const auto& f : batch) { ordered &= f.time > last; last = f.time; ++observed; }
    check("waterfall_concurrent_delivery_order_and_loss_accounting", ordered && observed + dropped == 20000);

    AnalysisStream stream;
    AnalysisRequests requests; requests.mainSpectrum = requests.fftHistory = true;
    stream.setRequests(requests);
    const int warmup = 2048;
    const int count = warmup + 4096, delay = 0;
    std::vector<float> pcm(count * 2, 0.0f);
    for (int i = warmup - 2048; i < warmup - 2048 + 512; ++i)
        pcm[2 * i] = .25f * static_cast<float>(std::sin(2.0 * 3.141592653589793 * 1500 * i / 48000));
    const auto original = pcm;
    stream.push(pcm.data(), count, 2, 48000, 0);
    SpectrumFrame latest;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    bool ready = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (stream.latest(latest) && latest.firstSample == count - 2048 - delay) { ready = true; break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    BarSnapshot bars; stream.consumeBars(bars);
    stream.drainWaterfall(batch);
    BarPeakAccumulator expected;
    for (const auto& f : batch) expected.add(f.levels.left, f.levels.right);
    const auto maxima = expected.consume();
    check("waterfall_captures_every_analysis_independently_of_bar_reads", ready && batch.size() == 9
        && *std::max_element(maxima.left.begin(), maxima.left.end()) > 0.0f
        && bars.levels.left == maxima.left && bars.levels.right == maxima.right);
    check("waterfall_does_not_modify_pcm", pcm == original);
    stream.push(pcm.data(), 2048, 2, 48000, count); stream.invalidate();
    stream.drainWaterfall(batch);
    check("waterfall_invalidated_stream_hides_queued_frames", batch.empty());

    raster.reset(); frame.sampleRate = 48000; frame.hopSize = 512; frame.time = 0; frame.generation++;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 2000; ++i)
    {
        frame.time = i * hop;
        for (int b = 0; b < displayBandCount; ++b) frame.levels.left[b] = static_cast<float>((i * 7 + b * 13) % 101) / 100;
        raster.process(frame, true, rows);
    }
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / 2000;
    check("waterfall_long_stream_keeps_fft_cadence", !rows.reset && raster.rowSeconds() == hop / 2);
    check("waterfall_shadow_patch_is_bounded", raster.correctedRowCount() <= WaterfallSettings::width + 1);
    std::cerr << "Waterfall 300x2 shaded rows mean: " << ms << " ms (2000 frames)\n";
}
