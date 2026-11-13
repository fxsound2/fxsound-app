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
#include "AnalysisStream.h"
#include <algorithm>
#include <cstring>
#include <exception>
#include <limits>

namespace fxanalysis
{
AnalysisStream::AnalysisStream() : worker_(&AnalysisStream::run, this) {}
AnalysisStream::~AnalysisStream()
{
    {
        const std::lock_guard<std::mutex> lock(workerMutex_);
        stopping_.store(true, std::memory_order_release);
    }
    workerWake_.notify_one();
    if (worker_.joinable()) worker_.join();
}
void AnalysisStream::setEnabled(bool value) noexcept
{
    {
        const std::lock_guard<std::mutex> lock(workerMutex_);
        if (monitoringEnabled_.load(std::memory_order_relaxed) == value) return;
        monitoringEnabled_.store(false, std::memory_order_release);
        enabled_.store(false, std::memory_order_release);
        resetInput();
        if (value && !queue_ && !failed_.load(std::memory_order_acquire))
        {
            try { queue_ = std::make_unique<Block[]>(capacity); }
            catch (const std::exception& error) { fail(error.what()); }
        }
        enabled_.store(value && !failed_.load(std::memory_order_acquire), std::memory_order_release);
        monitoringEnabled_.store(value, std::memory_order_release);
    }
    workerWake_.notify_one();
}
void AnalysisStream::invalidate() noexcept { revision_.fetch_add(2, std::memory_order_acq_rel); }

bool AnalysisStream::push(const float* pcm, int frames, int channels, int rate, std::uint64_t first) noexcept
{
    return push(pcm, frames, channels, rate, first, beginInput());
}

AnalysisStream::InputToken AnalysisStream::beginInput() const noexcept
{
    const auto revision = revision_.load(std::memory_order_acquire);
    const auto clips = clipDetector_.generation();
    return {(revision & 1u) == 0 && monitoringEnabled_.load(std::memory_order_acquire)
        && revision == revision_.load(std::memory_order_acquire)
        ? revision : 0, clips};
}

void AnalysisStream::resetInput() noexcept
{
    // Odd revisions mark the transition while the control thread resets the peak latch.
    revision_.fetch_add(1, std::memory_order_acq_rel);
    clipDetector_.reset();
    revision_.fetch_add(1, std::memory_order_release);
}

bool AnalysisStream::push(const float* pcm, int frames, int channels, int rate, std::uint64_t first,
                          InputToken input) noexcept
{
    if (input.revision == 0 || input.revision != revision_.load(std::memory_order_acquire)) return true;
    if (frames < 0 || rate < 8000 || rate > 384000
        || static_cast<std::uint64_t>(frames) > std::numeric_limits<std::uint64_t>::max() - first
        || !clipDetector_.processBlock(pcm, frames, channels, input.clips))
    {
        auto revision = input.revision;
        if (revision_.compare_exchange_strong(revision, revision + 2, std::memory_order_acq_rel))
            result_.store(AnalysisResult::invalidBlock);
        return false;
    }
    // Peaks are latched from the raw block before a full queue can drop FFT input.
    if (!enabled()) return true;
    const auto revision = input.revision;
    for (int offset = 0; offset < frames;)
    {
        const unsigned write = write_.load(std::memory_order_relaxed);
        const unsigned next = (write + 1) % capacity;
        if (next == read_.load(std::memory_order_acquire))
        {
            dropped_.fetch_add(static_cast<std::uint64_t>(frames - offset));
            // The next accepted sample position exposes the gap to AudioAnalyzer.
            return false;
        }
        auto& block = queue_[write];
        block.frames = std::min(blockFrames, frames - offset);
        block.channels = channels;
        block.sampleRate = rate;
        block.first = first + static_cast<std::uint64_t>(offset);
        block.revision = revision;
        std::memcpy(block.pcm.data(), pcm + static_cast<std::size_t>(offset) * channels,
                    static_cast<std::size_t>(block.frames) * channels * sizeof(float));
        write_.store(next, std::memory_order_release);
        offset += block.frames;
    }
    return true;
}

bool AnalysisStream::latest(SpectrumFrame& frame)
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    if (!enabled() || publishedRevision_ != revision_.load(std::memory_order_acquire)
        || std::chrono::steady_clock::now() - publishedAt_ > std::chrono::milliseconds(400)) return false;
    frame = snapshot_;
    return true;
}

bool AnalysisStream::consumeBars(BarSnapshot& bars)
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    if (!enabled() || publishedRevision_ != revision_.load(std::memory_order_acquire)
        || std::chrono::steady_clock::now() - publishedAt_ > std::chrono::milliseconds(400))
    {
        barPeaks_.reset();
        bars = {};
        return false;
    }
    bars = {barPeaks_.consume(), barGeneration_};
    return true;
}

void AnalysisStream::discardBarPeaks()
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    barPeaks_.discardPending();
}

std::string AnalysisStream::failureMessage()
{
    // The buffer is written once, before publishing failed_, then remains immutable.
    return failed_.load(std::memory_order_acquire) ? failure_.data() : "";
}

void AnalysisStream::fail(const char* message) noexcept
{
    std::strncpy(failure_.data(), message, failure_.size() - 1);
    failed_.store(true, std::memory_order_release);
    result_.store(AnalysisResult::notPrepared);
    enabled_.store(false);
    invalidate();
}

void AnalysisStream::onSpectrum(const SpectrumFrame& frame)
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    if (publishedRevision_ != processingRevision_
        || frame.firstSample != snapshot_.firstSample + static_cast<std::uint64_t>(frame.hopSize))
    {
        barPeaks_.reset();
        ++barGeneration_;
    }
    barPeaks_.add(frame.barLeft, frame.barRight);
    snapshot_ = frame;
    publishedRevision_ = processingRevision_;
    publishedAt_ = std::chrono::steady_clock::now();
}

void AnalysisStream::run() noexcept
{
    // Exception boundary for allocation failures in preparation / snapshot copies.
    try
    {
        bool haveInput = false;
        std::uint64_t nextInput = 0;
        while (!stopping_.load(std::memory_order_acquire))
        {
            const unsigned read = read_.load(std::memory_order_relaxed);
            if (read == write_.load(std::memory_order_acquire))
            {
                std::unique_lock<std::mutex> lock(workerMutex_);
                workerWake_.wait(lock, [this] { return stopping_.load() || enabled(); });
                if (!stopping_.load())
                    workerWake_.wait_for(lock, std::chrono::milliseconds(2),
                        [this] { return stopping_.load() || !enabled(); });
                continue;
            }
            const auto& block = queue_[read];
            if (enabled() && block.revision == revision_.load(std::memory_order_acquire))
            {
                AnalysisResult result = AnalysisResult::ok;
                const bool reconfigure = processingRevision_ != block.revision
                    || analyzer_.config().sampleRate != block.sampleRate || analyzer_.config().channels != block.channels;
                if (reconfigure || (haveInput && block.first != nextInput))
                {
                    // Detect gaps from input positions, even a backward seek whose
                    // next FFT window would happen to look consecutive. Hide the old
                    // result immediately while the replacement window fills.
                    std::lock_guard<std::mutex> guard(snapshotMutex_);
                    publishedRevision_ = 0;
                    barPeaks_.reset();
                }
                if (reconfigure)
                {
                    result = analyzer_.prepare({static_cast<double>(block.sampleRate), block.channels});
                    processingRevision_ = block.revision;
                }
                haveInput = true;
                nextInput = block.first + static_cast<std::uint64_t>(block.frames);
                if (result == AnalysisResult::ok)
                    result = analyzer_.processBlock(block.pcm.data(), block.frames, block.first, *this);
                result_.store(result);
                if (result != AnalysisResult::ok) invalidate();
            }
            read_.store((read + 1) % capacity, std::memory_order_release);
        }
    }
    catch (const std::exception& error)
    {
        fail(error.what());
    }
    catch (...)
    {
        fail("Unknown analysis worker exception");
    }
}
}
