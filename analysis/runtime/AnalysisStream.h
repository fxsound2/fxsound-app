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
    bool enabled() const noexcept { return enabled_.load(std::memory_order_acquire); }
    void invalidate() noexcept;
    void resetInput() noexcept;
    struct InputToken { std::uint64_t revision; ClipDetector::Generation clips; };
    InputToken beginInput() const noexcept;
    // Non-blocking, allocation-free PCM copy. Mono/stereo only in this first version.
    bool push(const float* pcm, int frames, int channels, int sampleRate,
              std::uint64_t firstSample) noexcept;
    bool push(const float* pcm, int frames, int channels, int sampleRate,
              std::uint64_t firstSample, InputToken input) noexcept;
    bool latest(SpectrumFrame& frame); // Non-destructive, instantaneous scientific data.
    bool consumeBars(BarSnapshot& bars); // One display consumer; peaks since its last read.
    void discardBarPeaks(); // Non-audio thread, when a display hides/resumes.
    ClipEvents consumeClipEvents() noexcept { return clipDetector_.consume(); }
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
    void processQueuedBlock(const Block& block);
    void fail(const char* message) noexcept;
    void onSpectrum(const SpectrumFrame& frame) override;
    std::unique_ptr<Block[]> queue_;
    std::atomic<unsigned> read_{0}, write_{0};
    std::atomic<bool> enabled_{false}, stopping_{false};
    // Raw peak monitoring follows the requested mode even if the FFT worker fails.
    std::atomic<bool> monitoringEnabled_{false};
    ClipDetector clipDetector_;
    std::atomic<bool> failed_{false};
    std::array<char, 256> failure_{};
    std::atomic<std::uint64_t> revision_{2}, dropped_{0};
    std::atomic<AnalysisResult> result_{AnalysisResult::ok};
    AudioAnalyzer analyzer_;
    std::uint64_t processingRevision_ = 0, publishedRevision_ = 0;
    std::uint64_t nextInput_ = 0;
    bool haveInput_ = false;
    std::mutex snapshotMutex_;
    SpectrumFrame snapshot_;
    BarPeakAccumulator barPeaks_;
    std::uint64_t barGeneration_ = 0;
    std::chrono::steady_clock::time_point publishedAt_;
    std::mutex workerMutex_;
    std::condition_variable workerWake_;
    std::thread worker_;
};
}
