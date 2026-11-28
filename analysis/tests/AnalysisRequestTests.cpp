// SPDX-License-Identifier: AGPL-3.0-or-later
#include "TestSupport.h"
#include "../runtime/AnalysisStream.h"
#include "../WaterfallRaster.h"
#include <chrono>
#include <memory>
#include <thread>

namespace fxanalysis
{
struct AnalysisStreamTestPeer
{
    static std::array<std::uint64_t, 3> modelCalls(const AnalysisStream& stream)
    {
        return {stream.modelCalculations_[0].load(), stream.modelCalculations_[1].load(),
            stream.modelCalculations_[2].load()};
    }
    static std::uint64_t inputRevision(const AnalysisStream& stream) { return stream.revision_.load(); }
    static void useSynchronousWorker(AnalysisStream& stream)
    {
        { std::lock_guard<std::mutex> lock(stream.workerMutex_); stream.stopping_.store(true); }
        stream.workerWake_.notify_one();
        stream.worker_.join();
        // No live worker remains. The peer now controls the actual queue consumer.
    }
    static void processQueue(AnalysisStream& stream)
    {
        while (stream.read_.load() != stream.write_.load())
        {
            const auto read = stream.read_.load();
            const auto& block = stream.queue_[read];
            if (stream.enabled() && block.revision == stream.revision_.load()) stream.processQueuedBlock(block);
            stream.read_.store((read + 1) % stream.capacity);
        }
    }
};
}

namespace fxanalysis::test
{
namespace
{
using namespace experimental;
struct Feed
{
    AnalysisStream& stream;
    std::uint64_t first = 0;
    SpectrumFrame latest;
    bool send(int frames = 4096, int rate = 48000)
    {
        const auto pcm = tone(frames, 375, 1, rate);
        const auto expected = first + frames - 2048;
        const bool accepted = stream.push(pcm.data(), frames, 2, rate, first);
        first += frames;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (accepted && std::chrono::steady_clock::now() < deadline)
        {
            if (stream.latest(latest) && latest.firstSample == expected) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return false;
    }
};

void singleConsumerTests(const Check& check)
{
    for (int mode = 0; mode < 7; ++mode)
    {
        AnalysisStream stream;
        AnalysisRequests requests;
        requests.mainSpectrum = mode == 0;
        requests.fftHistory = mode == 1 || mode == 6;
        requests.carrierHistory = mode == 2;
        requests.pitchWindow = mode == 3;
        requests.compactPitch = mode == 4;
        requests.compactCarriers = mode == 5;
        requests.fftCarrierOverlay = mode == 6;
        stream.setRequests(requests);
        Feed feed{stream};
        const bool ready = feed.send();
        const auto calls = AnalysisStreamTestPeer::modelCalls(stream);
        const bool motion = mode == 3 || mode == 4, correlation = mode == 3;
        const bool harmonic = mode == 2 || mode == 3 || mode == 5 || mode == 6;
        const std::string prefix = "requests_single_consumer_" + std::to_string(mode);
        check((prefix + "_runs_exact_model_union").c_str(), ready && stream.enabled()
            && calls[0] == (motion ? 5u : 0u) && calls[1] == (correlation ? 5u : 0u)
            && calls[2] == (harmonic ? 5u : 0u));
        PitchMotionSnapshot pitch;
        auto comb = std::make_unique<HarmonicCombSnapshot>();
        check((prefix + "_publishes_only_requested_models").c_str(),
            stream.latestPitchMotion(pitch) == motion && stream.latestHarmonicComb(*comb) == harmonic);
        BarSnapshot bars; AxisSnapshot axis;
        const bool haveBars = stream.consumeBars(bars, axis);
        check((prefix + "_main_axis_is_independent").c_str(), haveBars == (mode == 0)
            && (!haveBars || (axis.generation == bars.generation && axis.sampleRate == feed.latest.sampleRate
                && axis.bandHz == feed.latest.bandHz && axis.bandEdgesHz == feed.latest.bandEdgesHz)));
        std::vector<WaterfallFrame> fft, carriers;
        stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
        check((prefix + "_independent_history_inboxes").c_str(), fft.size() == (requests.fftHistory ? 5u : 0u)
            && carriers.size() == (requests.carrierHistory ? 5u : 0u));
        if (mode == 3)
        {
            SpectralCorrelationSnapshot pair;
            check("requests_pitch_window_trio_is_one_observation", stream.latestPitchExperiment(pitch, pair, *comb)
                && pitch.centreTimeSeconds == pair.centreTimeSeconds && pitch.centreTimeSeconds == comb->centreTimeSeconds
                && pitch.centreTimeSeconds == feed.latest.centreTimeSeconds());
        }
    }
    AnalysisStream dormant;
    AnalysisRequests requests; requests.fftCarrierOverlay = true;
    dormant.setRequests(requests);
    SpectrumFrame frame;
    check("requests_dormant_overlay_does_not_enable_fft_or_models", !dormant.enabled()
        && dormant.beginInput().revision == 0 && !dormant.latest(frame)
        && AnalysisStreamTestPeer::modelCalls(dormant) == std::array<std::uint64_t, 3>{});
}

void independentLifetimeTests(const Check& check)
{
    AnalysisStream stream;
    AnalysisRequests requests{true, true, true, true, true, true, true};
    stream.setRequests(requests);
    Feed feed{stream};
    const bool initial = feed.send();
    const auto revision = AnalysisStreamTestPeer::inputRevision(stream);
    BarSnapshot before, after;
    stream.consumeBars(before);
    auto harmonic = std::make_unique<HarmonicCombSnapshot>();
    stream.latestHarmonicComb(*harmonic);
    const auto harmonicGeneration = harmonic->generation;
    const auto calls = AnalysisStreamTestPeer::modelCalls(stream);
    requests.mainSpectrum = requests.pitchWindow = false;
    stream.setRequests(requests);
    PitchMotionSnapshot motion; SpectralCorrelationSnapshot correlation;
    std::vector<WaterfallFrame> fft, carriers;
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("requests_closing_main_and_window_preserves_auxiliary_queues", initial && fft.size() == 5 && carriers.size() == 5
        && AnalysisStreamTestPeer::inputRevision(stream) == revision && !stream.consumeBars(after)
        && !stream.latestPitchExperiment(motion, correlation, *harmonic));
    const bool continued = feed.send(512);
    const auto nextCalls = AnalysisStreamTestPeer::modelCalls(stream);
    check("requests_closing_pitch_window_keeps_shared_motion_and_harmonic_memory", continued
        && nextCalls[0] == calls[0] + 1 && nextCalls[1] == calls[1] && nextCalls[2] == calls[2] + 1
        && stream.latestPitchMotion(motion) && motion.stepSeconds > 0
        && stream.latestHarmonicComb(*harmonic) && harmonic->generation == harmonicGeneration);
    requests.fftHistory = false;
    stream.setRequests(requests);
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("requests_closing_fft_clears_only_its_inbox", fft.empty() && carriers.size() == 1);
    const bool carrierContinued = feed.send(512);
    stream.drainCarrierWaterfall(carriers);
    check("requests_auxiliaries_continue_with_classic_or_hidden_main", carrierContinued && carriers.size() == 1
        && !stream.consumeBars(after) && AnalysisStreamTestPeer::inputRevision(stream) == revision);
    stream.setRequests({});
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("requests_last_close_hides_every_consumer", !stream.enabled() && !stream.latest(feed.latest)
        && !stream.latestPitchMotion(motion) && !stream.latestHarmonicComb(*harmonic)
        && fft.empty() && carriers.empty());
    requests = {}; requests.compactPitch = true;
    stream.setRequests(requests);
    check("requests_reopening_consumer_never_reuses_previous_publication", !stream.latestPitchMotion(motion));
    check("requests_reopened_auxiliary_accepts_audio_without_main", feed.send() && stream.latestPitchMotion(motion));
}

void overlayAndSettingsTests(const Check& check)
{
    AnalysisStream stream;
    AnalysisRequests requests; requests.mainSpectrum = requests.fftHistory = requests.carrierHistory = requests.fftCarrierOverlay = true;
    stream.setRequests(requests);
    Feed feed{stream};
    const bool ready = feed.send();
    std::vector<WaterfallFrame> fft, carriers;
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    const bool first = ready && !fft.empty() && !carriers.empty();
    WaterfallFrame old = first ? fft.back() : WaterfallFrame{};
    const auto generation = AnalysisStreamTestPeer::inputRevision(stream);
    BarSnapshot before, after; stream.consumeBars(before);
    const bool uiQueued = feed.send(512);
    PitchMotionSettings sameSettings; sameSettings.harmonicComb.showInStrip = false;
    stream.setPitchMotionSettings(sameSettings);
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("requests_ui_only_settings_preserve_model_and_history_publications", uiQueued && fft.size() == 1 && carriers.size() == 1
        && fft[0].carrierCapture != 0 && AnalysisStreamTestPeer::inputRevision(stream) == generation);
    const bool queued = feed.send(512);
    requests.fftCarrierOverlay = false; stream.setRequests(requests);
    requests.fftCarrierOverlay = true; stream.setRequests(requests);
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("requests_overlay_reopen_discards_only_old_gold", first && queued && fft.size() == 1 && carriers.size() == 1
        && fft[0].carrierCapture == 0 && stream.consumeBars(after) && before.generation == after.generation
        && AnalysisStreamTestPeer::inputRevision(stream) == generation);
    bool resumed = true;
    for (int i = 0; i < 4; ++i)
    {
        resumed = feed.send(512) && resumed;
        if (i < 3) { stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers); }
    }
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    const bool newSource = resumed && fft.size() == 1 && fft[0].carrierCapture != 0
        && fft[0].sourceRevision != old.sourceRevision && carriers.size() == 1 && fft[0].profile == carriers[0].profile;
    WaterfallRaster overlay(0xffffbe5c); WaterfallRaster::Rows rows;
    old.hasProfile = true;
    bool restarted = false;
    if (newSource) { auto next = fft[0]; next.hasProfile = true; restarted = overlay.process(old, false, rows)
        && overlay.process(next, false, rows) && rows.reset; }
    check("requests_overlay_source_epoch_restarts_real_raster", newSource && restarted);
    const bool pending = feed.send(512);
    PitchMotionSettings settings; settings.harmonicComb.displayGain = 9;
    stream.setPitchMotionSettings(settings);
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("requests_settings_keep_fft_and_bars_but_hide_old_carriers", pending && fft.size() == 1
        && fft[0].carrierCapture == 0 && carriers.empty() && stream.consumeBars(after)
        && before.generation == after.generation && AnalysisStreamTestPeer::inputRevision(stream) == generation);
    check("requests_settings_apply_on_next_worker_observation", feed.send(512));
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    check("requests_settings_publish_same_frame_to_both_histories", fft.size() == 1 && carriers.size() == 1
        && fft[0].carrierCapture != 0 && fft[0].time == carriers[0].time && fft[0].profile == carriers[0].profile);
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    auto comb = std::make_unique<HarmonicCombSnapshot>();
    check("requests_all_observers_expire_after_400_ms", !stream.latest(feed.latest) && !stream.consumeBars(after)
        && !stream.latestHarmonicComb(*comb) && fft.empty() && carriers.empty());
}

void backlogSessionTests(const Check& check)
{
    AnalysisStream stream;
    AnalysisStreamTestPeer::useSynchronousWorker(stream);
    AnalysisRequests all; all.mainSpectrum = all.fftHistory = all.carrierHistory = all.pitchWindow = true;
    stream.setRequests(all);
    auto pcm = tone(4096, 375);
    const bool seeded = stream.push(pcm.data(), 4096, 2, 48000, 0);
    AnalysisStreamTestPeer::processQueue(stream);
    std::vector<WaterfallFrame> fft, carriers;
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    const auto before = AnalysisStreamTestPeer::modelCalls(stream);
    AnalysisRequests onlyFft; onlyFft.fftHistory = true;
    stream.setRequests(onlyFft);
    const auto oldToken = stream.beginInput();
    const bool queued = stream.push(pcm.data(), 4096, 2, 48000, 4096, oldToken);
    stream.setRequests(all);
    AnalysisStreamTestPeer::processQueue(stream);
    stream.drainWaterfall(fft); stream.drainCarrierWaterfall(carriers);
    BarSnapshot bars;
    PitchMotionSnapshot motion; SpectralCorrelationSnapshot correlation;
    auto harmonic = std::make_unique<HarmonicCombSnapshot>();
    check("requests_reopen_does_not_relabel_queued_pcm_as_fresh", seeded && queued && fft.size() == 8 && carriers.empty()
        && !stream.consumeBars(bars) && !stream.latestPitchExperiment(motion, correlation, *harmonic)
        && AnalysisStreamTestPeer::modelCalls(stream) == before);
    // An already-begun audio callback can finish after the reopen. Its token must
    // remain old even though push() itself is now called in the new UI session.
    const bool delayed = stream.push(pcm.data(), 1024, 2, 48000, 8192, oldToken);
    AnalysisStreamTestPeer::processQueue(stream);
    stream.drainCarrierWaterfall(carriers);
    check("requests_pre_reopen_input_token_stays_in_old_session", delayed && carriers.empty()
        && !stream.consumeBars(bars) && AnalysisStreamTestPeer::modelCalls(stream) == before);
    const bool fresh = stream.push(pcm.data(), 4096, 2, 48000, 9216);
    AnalysisStreamTestPeer::processQueue(stream);
    stream.drainCarrierWaterfall(carriers);
    const auto after = AnalysisStreamTestPeer::modelCalls(stream);
    check("requests_reopen_waits_for_complete_new_observation_without_resetting_fft", fresh && carriers.size() == 5
        && carriers.front().time == (9216.0 + 1024) / 48000
        && stream.consumeBars(bars) && stream.latestPitchExperiment(motion, correlation, *harmonic)
        && after[0] == before[0] + 5 && after[1] == before[1] + 5 && after[2] == before[2] + 5);
}
}

void runAnalysisRequestTests(const Check& check)
{
    singleConsumerTests(check);
    independentLifetimeTests(check);
    overlayAndSettingsTests(check);
    backlogSessionTests(check);
}
}
