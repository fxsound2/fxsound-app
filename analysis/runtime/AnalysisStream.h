// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../BarPeakAccumulator.h"
#include <atomic>
#include <chrono>
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

// Optional host adapter. AudioAnalyzer itself knows nothing about these threads.
// Exactly one producer calls push(). Stop the producer before destroying this object.
class AnalysisStream final : private SpectrumSink
{
public:
    AnalysisStream();
    ~AnalysisStream();
    AnalysisStream(const AnalysisStream&) = delete;
    AnalysisStream& operator=(const AnalysisStream&) = delete;
    void setEnabled(bool enabled) noexcept;
    bool enabled() const noexcept { return enabled_.load(std::memory_order_acquire); }
    void invalidate() noexcept;
    // Non-blocking, allocation-free PCM copy. Mono/stereo only in this first version.
    bool push(const float* pcm, int frames, int channels, int sampleRate,
              std::uint64_t firstSample) noexcept;
    bool latest(SpectrumFrame& frame); // Non-destructive, instantaneous scientific data.
    bool consumeBars(BarSnapshot& bars); // One display consumer; peaks since its last read.
    void discardBarPeaks(); // Non-audio thread, when a display hides/resumes.
    std::uint64_t droppedFrames() const noexcept { return dropped_.load(); }
    AnalysisResult lastResult() const noexcept { return result_.load(); }
    std::string failureMessage(); // Non-audio thread only; empty unless the worker stopped.

private:
    static constexpr int blockFrames = 1024;
    static constexpr unsigned capacity = 64;
    struct Block
    {
        std::array<float, blockFrames * 2> pcm{};
        int frames = 0, channels = 0, sampleRate = 0;
        std::uint64_t first = 0, revision = 0;
    };
    void run() noexcept;
    void fail(const char* message) noexcept;
    void onSpectrum(const SpectrumFrame& frame) override;
    std::unique_ptr<Block[]> queue_;
    std::atomic<unsigned> read_{0}, write_{0};
    std::atomic<bool> enabled_{false}, stopping_{false};
    std::atomic<bool> failed_{false};
    std::array<char, 256> failure_{};
    std::atomic<std::uint64_t> revision_{1}, dropped_{0};
    std::atomic<AnalysisResult> result_{AnalysisResult::ok};
    AudioAnalyzer analyzer_;
    std::uint64_t processingRevision_ = 0, publishedRevision_ = 0;
    std::mutex snapshotMutex_;
    SpectrumFrame snapshot_;
    BarPeakAccumulator barPeaks_;
    std::uint64_t barGeneration_ = 0;
    std::chrono::steady_clock::time_point publishedAt_;
    std::thread worker_;
};
}
