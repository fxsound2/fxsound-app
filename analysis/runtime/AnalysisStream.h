/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
    Sylvain Petitgrand (2026)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#include "../BarPeakAccumulator.h"
#include "../ClipDetector.h"
#include "WaterfallInbox.h"
#include "../experimental/pitch/PitchMotion.h"
#include "../experimental/pitch/SpectralCorrelation.h"
#include "../experimental/pitch/HarmonicCombSnapshot.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace fxanalysis
{
struct BarSnapshot
{
    StereoBarLevels levels;
    std::uint64_t generation = 0; // Changes on input discontinuity or format/reset.
};

// Effective consumers, supplied by the host after visibility/audio eligibility.
// Preferences stay in the host; an overlay alone never requests an FFT history.
struct AnalysisRequests
{
    bool mainSpectrum = false, fftHistory = false, carrierHistory = false;
    bool pitchWindow = false, compactPitch = false, compactCarriers = false;
    bool fftCarrierOverlay = false;
};

struct AxisSnapshot
{
    double sampleRate = 0.0;
    std::uint64_t generation = 0;
    std::array<float, displayBandCount> bandHz{};
    std::array<float, displayBandCount + 1> bandEdgesHz{};
};

// Optional host adapter. AudioAnalyzer itself knows nothing about these threads.
// Exactly one producer calls push(). Stop the producer before destroying this object.
class AnalysisStream final : private SpectrumSink
{
public:
    AnalysisStream();
    ~AnalysisStream();
    AnalysisStream(const AnalysisStream&) = delete;
    AnalysisStream& operator=(const AnalysisStream&) = delete;
    // The host serializes setEnabled/resetInput on its control paths. Neither
    // operation belongs on the audio producer; old tokens discard stale output.
    void setEnabled(bool enabled) noexcept;
    // Control-thread only, serialized with resetInput. Changing one consumer does
    // not invalidate another consumer's FFT, peaks, model memory or history.
    void setRequests(const AnalysisRequests& requests) noexcept;
    bool enabled() const noexcept { return enabled_.load(std::memory_order_acquire); }
    void invalidate() noexcept;
    void resetInput() noexcept;
    struct InputToken { std::uint64_t revision; ClipDetector::Generation clips; std::uint64_t requestSession; };
    InputToken beginInput() const noexcept;
    // Non-blocking, allocation-free PCM copy. Mono/stereo only in this first version.
    bool push(const float* pcm, int frames, int channels, int sampleRate,
              std::uint64_t firstSample) noexcept;
    bool push(const float* pcm, int frames, int channels, int sampleRate,
              std::uint64_t firstSample, InputToken input) noexcept;
    bool latest(SpectrumFrame& frame); // Non-destructive, instantaneous scientific data.
    bool consumeBars(BarSnapshot& bars); // One display consumer; peaks since its last read.
    bool consumeBars(BarSnapshot& bars, AxisSnapshot& axis); // One coherent publication, no complex FFT copy.
    void discardBarPeaks(); // Non-audio thread, when a display hides/resumes.
    std::uint64_t drainWaterfall(std::vector<WaterfallFrame>& frames);
    std::uint64_t drainCarrierWaterfall(std::vector<WaterfallFrame>& frames);
    void setPitchMotionSettings(const experimental::PitchMotionSettings& settings);
    bool latestPitchMotion(experimental::PitchMotionSnapshot& snapshot);
    bool latestHarmonicComb(experimental::HarmonicCombSnapshot& snapshot);
    bool latestPitchExperiment(experimental::PitchMotionSnapshot& motion,
        experimental::SpectralCorrelationSnapshot& correlation);
    bool latestPitchExperiment(experimental::PitchMotionSnapshot& motion,
        experimental::SpectralCorrelationSnapshot& correlation, experimental::HarmonicCombSnapshot& harmonic);
    ClipEvents consumeClipEvents() noexcept { return clipDetector_.consume(); }
    std::uint64_t droppedFrames() const noexcept { return dropped_.load(); }
    AnalysisResult lastResult() const noexcept { return result_.load(); }
    std::string failureMessage(); // Non-audio thread only; empty unless the worker stopped.

private:
    friend struct AnalysisStreamTestPeer;
    enum Consumer : unsigned { mainSpectrum, fftHistory, carrierHistory, pitchWindow,
        compactPitch, compactCarriers, fftCarrierOverlay, consumerCount };
    enum Model : unsigned { motionModel, correlationModel, harmonicModel, modelCount };
    using Epochs = std::array<std::uint64_t, consumerCount>;
    struct ModelState;
    struct ModelPublication;
    struct WorkRequest
    {
        unsigned mask = 0;
        Epochs epochs{};
        std::array<std::uint64_t, modelCount> modelEpochs{};
        std::uint64_t settingsRevision = 0, overlaySource = 0, carrierSource = 0;
        experimental::PitchMotionSettings settings;
    };
    static constexpr unsigned bit(Consumer consumer) noexcept { return 1u << consumer; }
    static unsigned requestMask(const AnalysisRequests& requests) noexcept;
    static bool needsModel(unsigned mask, Model model) noexcept;
    bool publicationIsCurrent() const; // Caller owns snapshotMutex_.
    bool modelPublicationIsCurrent(Model model) const;
    WorkRequest captureWorkRequest(const SpectrumFrame& frame);
    void calculateModels(const SpectrumFrame& frame, const WorkRequest& request);
    void publishObservation(const SpectrumFrame& frame, const WorkRequest& request,
        const WaterfallProfile& profile);
    std::uint64_t drainHistory(std::vector<WaterfallFrame>& frames, bool carriers);
    void resetWorkerModels() noexcept;
    static bool validInput(const float* pcm, int frames, int channels, int rate, std::uint64_t first) noexcept;
    static constexpr auto publicationLifetime = std::chrono::milliseconds(400);
    static constexpr int blockFrames = 1024;
    static constexpr unsigned capacity = 64;
    struct Block
    {
        std::array<float, blockFrames * 2> pcm{};
        int frames = 0, channels = 0, sampleRate = 0;
        std::uint64_t first = 0, revision = 0, requestSession = 0;
    };
    void run() noexcept;
    void processQueuedBlock(const Block& block);
    void fail(const char* message) noexcept;
    void onSpectrum(const SpectrumFrame& frame) override;
    std::unique_ptr<Block[]> queue_;
    std::atomic<unsigned> read_{0}, write_{0};
    std::atomic<bool> enabled_{false}, stopping_{false};
    // Raw peak monitoring is main-view-only and survives an FFT worker failure.
    std::atomic<bool> inputRequested_{false}, monitoringEnabled_{false};
    ClipDetector clipDetector_;
    std::atomic<bool> failed_{false};
    std::array<char, 256> failure_{};
    std::atomic<std::uint64_t> revision_{2}, dropped_{0}, requestSession_{1};
    std::atomic<AnalysisResult> result_{AnalysisResult::ok};
    AudioAnalyzer analyzer_;
    std::uint64_t processingRevision_ = 0, publishedRevision_ = 0;
    std::uint64_t nextInput_ = 0;
    bool haveInput_ = false;
    std::uint64_t formatGeneration_ = 0;
    std::mutex snapshotMutex_;
    unsigned requests_ = 0; // Control/worker snapshots, under snapshotMutex_.
    Epochs consumerEpochs_{};
    Epochs consumerStartSessions_{}; // Control session in which each view opened.
    Epochs inputConsumerEpochs_{}, inputConsumerFirst_{}; // Worker-only observation bounds.
    std::array<std::uint64_t, modelCount> modelEpochs_{};
    experimental::PitchMotionSettings pitchSettings_;
    std::uint64_t settingsRevision_ = 1, overlaySource_ = 1, carrierSource_ = 1;
    // Bounded models and their large publications live off the host stack. Models
    // are created lazily on the worker only when a consumer actually needs them.
    std::unique_ptr<ModelState> models_;
    std::unique_ptr<ModelPublication> modelPublication_;
    // Private validation counters record actual model calls, never audio-callback work.
    std::array<std::atomic<std::uint64_t>, modelCount> modelCalculations_{{0, 0, 0}};
    SpectrumFrame snapshot_;
    BarPeakAccumulator barPeaks_;
    AxisSnapshot axis_;
    std::uint64_t barGeneration_ = 0, publishedMainEpoch_ = 0;
    WaterfallInbox waterfall_, carrierWaterfall_; // Lock order: snapshot then inbox.
    std::chrono::steady_clock::time_point publishedAt_;
    std::mutex workerMutex_;
    std::condition_variable workerWake_;
    std::thread worker_;
};
}
