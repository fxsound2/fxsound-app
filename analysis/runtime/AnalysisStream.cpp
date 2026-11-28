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
#include "../experimental/pitch/HarmonicCombProfile.h"
#include "../experimental/pitch/HarmonicCarrierDisplay.h"
#include <algorithm>
#include <cstring>
#include <exception>
#include <limits>
#include <tuple>

namespace fxanalysis
{
namespace
{
auto modelSettings(const experimental::PitchMotionSettings& s)
{
    const auto& h = s.harmonicComb;
    // showInStrip is a UI preference. Reapplying it must not reset a model or an
    // independently open history; effective view ownership belongs to setRequests.
    return std::tie(s.minimumFrequencyHz, s.maximumFrequencyHz, s.maximumPeaks, s.minimumPeakRelativePower,
        s.noiseFloorStrength, s.minimumPeakProminenceDb, s.peakWeightExponent, s.minimumTotalPower,
        s.comparisonSeconds, s.smoothingSeconds, s.integrationSeconds, s.integratorLimitSemitones,
        s.transportScaleSemitones, s.marginalPenalty, s.entropy, s.maximumIterations, s.convergenceTolerance,
        h.enabled, h.subtractionFraction, h.harmonicDecay, h.maximumHarmonics, h.iterativeExtraction,
        h.maximumComponents, h.minimumRelativeAmplitude, h.minimumErrorImprovement, h.minimumComponentDistanceSemitones,
        h.harmonicSupportThreshold, h.harmonicWideningPercent, h.trailHalfLifeSeconds, h.displayGain,
        h.logarithmicDisplay, h.highResReference, h.displayRangeDb);
}
}
struct AnalysisStream::ModelState
{
    std::unique_ptr<experimental::PitchMotion> motion;
    std::unique_ptr<experimental::SpectralCorrelation> correlation;
    std::unique_ptr<experimental::HarmonicCombProfile> harmonic;
    std::array<std::uint64_t, modelCount> epochs{}, settings{};
    std::array<bool, modelCount> prepared{};
};
struct AnalysisStream::ModelPublication
{
    experimental::PitchMotionSnapshot motion;
    experimental::SpectralCorrelationSnapshot correlation;
    experimental::HarmonicCombSnapshot harmonic;
    Epochs consumers{};
    std::array<std::uint64_t, modelCount> epochs{}, settings{};
};

AnalysisStream::AnalysisStream() : models_(std::make_unique<ModelState>()),
    modelPublication_(std::make_unique<ModelPublication>()), worker_(&AnalysisStream::run, this) {}
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
    AnalysisRequests requests;
    requests.mainSpectrum = value;
    setRequests(requests);
}

unsigned AnalysisStream::requestMask(const AnalysisRequests& r) noexcept
{
    return (r.mainSpectrum ? bit(mainSpectrum) : 0u) | (r.fftHistory ? bit(fftHistory) : 0u)
        | (r.carrierHistory ? bit(carrierHistory) : 0u) | (r.pitchWindow ? bit(pitchWindow) : 0u)
        | (r.compactPitch ? bit(compactPitch) : 0u) | (r.compactCarriers ? bit(compactCarriers) : 0u)
        | (r.fftHistory && r.fftCarrierOverlay ? bit(fftCarrierOverlay) : 0u);
}

bool AnalysisStream::needsModel(unsigned mask, Model model) noexcept
{
    const unsigned consumers = model == motionModel ? bit(pitchWindow) | bit(compactPitch)
        : model == correlationModel ? bit(pitchWindow)
        : bit(pitchWindow) | bit(compactCarriers) | bit(carrierHistory) | bit(fftCarrierOverlay);
    return (mask & consumers) != 0;
}

void AnalysisStream::setRequests(const AnalysisRequests& requests) noexcept
{
    const unsigned next = requestMask(requests);
    {
        const std::lock_guard<std::mutex> lock(workerMutex_);
        const std::lock_guard<std::mutex> publication(snapshotMutex_);
        const unsigned previous = requests_;
        if (previous == next) return;
        const auto session = requestSession_.fetch_add(1, std::memory_order_acq_rel) + 1;
        for (unsigned i = 0; i < consumerCount; ++i)
            if ((previous ^ next) & (1u << i))
            {
                ++consumerEpochs_[i];
                if (next & (1u << i)) consumerStartSessions_[i] = session;
            }
        for (unsigned i = 0; i < modelCount; ++i)
            if (needsModel(previous, static_cast<Model>(i)) != needsModel(next, static_cast<Model>(i))) ++modelEpochs_[i];
        if ((previous ^ next) & bit(mainSpectrum))
        {
            monitoringEnabled_.store(false, std::memory_order_release);
            clipDetector_.reset();
            barPeaks_.reset(); publishedMainEpoch_ = 0; ++barGeneration_;
        }
        if ((previous ^ next) & bit(fftHistory)) waterfall_.setEnabled((next & bit(fftHistory)) != 0);
        if ((previous ^ next) & bit(carrierHistory))
        {
            ++carrierSource_;
            carrierWaterfall_.setEnabled((next & bit(carrierHistory)) != 0);
        }
        if ((previous ^ next) & bit(fftCarrierOverlay)) ++overlaySource_;
        if ((previous == 0) != (next == 0))
        {
            inputRequested_.store(false, std::memory_order_release);
            enabled_.store(false, std::memory_order_release);
            resetInput();
        }
        if (next && !queue_ && !failed_.load(std::memory_order_acquire))
        {
            try { queue_ = std::make_unique<Block[]>(capacity); }
            catch (const std::exception& error) { fail(error.what()); }
        }
        requests_ = next;
        enabled_.store(next && !failed_.load(std::memory_order_acquire), std::memory_order_release);
        monitoringEnabled_.store((next & bit(mainSpectrum)) != 0, std::memory_order_release);
        inputRequested_.store(next != 0, std::memory_order_release);
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
    return {(revision & 1u) == 0 && inputRequested_.load(std::memory_order_acquire)
        && revision == revision_.load(std::memory_order_acquire)
        ? revision : 0, clips, requestSession_.load(std::memory_order_acquire)};
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
    if (!validInput(pcm, frames, channels, rate, first)
        || (monitoringEnabled_.load(std::memory_order_acquire)
            && !clipDetector_.processBlock(pcm, frames, channels, input.clips)))
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
        block.requestSession = input.requestSession;
        std::memcpy(block.pcm.data(), pcm + static_cast<std::size_t>(offset) * channels,
                    static_cast<std::size_t>(block.frames) * channels * sizeof(float));
        write_.store(next, std::memory_order_release);
        offset += block.frames;
    }
    return true;
}

bool AnalysisStream::validInput(const float* pcm, int frames, int channels, int rate, std::uint64_t first) noexcept
{
    return frames >= 0 && (frames == 0 || pcm != nullptr) && (channels == 1 || channels == 2)
        && rate >= AnalysisLimits::minimumSampleRate && rate <= AnalysisLimits::maximumSampleRate
        && static_cast<std::uint64_t>(frames) <= std::numeric_limits<std::uint64_t>::max() - first;
}

bool AnalysisStream::publicationIsCurrent() const
{
    return enabled() && publishedRevision_ == revision_.load(std::memory_order_acquire)
        && std::chrono::steady_clock::now() - publishedAt_ <= publicationLifetime;
}

bool AnalysisStream::latest(SpectrumFrame& frame)
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    if (!publicationIsCurrent()) return false;
    frame = snapshot_;
    return true;
}

bool AnalysisStream::consumeBars(BarSnapshot& bars)
{
    AxisSnapshot axis;
    return consumeBars(bars, axis);
}

bool AnalysisStream::consumeBars(BarSnapshot& bars, AxisSnapshot& axis)
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    if (!publicationIsCurrent() || !(requests_ & bit(mainSpectrum)) || publishedMainEpoch_ != consumerEpochs_[mainSpectrum])
    {
        barPeaks_.reset();
        bars = {};
        axis = {};
        return false;
    }
    bars = {barPeaks_.consume(), barGeneration_};
    axis = axis_;
    return true;
}

void AnalysisStream::discardBarPeaks()
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    barPeaks_.discardPending();
}

void AnalysisStream::setPitchMotionSettings(const experimental::PitchMotionSettings& settings)
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    if (modelSettings(pitchSettings_) == modelSettings(settings)) { pitchSettings_ = settings; return; }
    pitchSettings_ = settings;
    ++settingsRevision_; ++overlaySource_; ++carrierSource_;
    // Harmonic settings invalidate gold/carrier rows, never the underlying FFT.
    carrierWaterfall_.setEnabled((requests_ & bit(carrierHistory)) != 0);
}

bool AnalysisStream::modelPublicationIsCurrent(Model model) const
{
    return publicationIsCurrent() && needsModel(requests_, model)
        && modelPublication_->epochs[model] == modelEpochs_[model]
        && modelPublication_->settings[model] == settingsRevision_;
}

bool AnalysisStream::latestPitchMotion(experimental::PitchMotionSnapshot& snapshot)
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    const Consumer consumer = requests_ & bit(compactPitch) ? compactPitch : pitchWindow;
    if (!modelPublicationIsCurrent(motionModel) || !(requests_ & bit(consumer))
        || modelPublication_->consumers[consumer] != consumerEpochs_[consumer])
    { snapshot = {}; return false; }
    snapshot = modelPublication_->motion;
    return true;
}

bool AnalysisStream::latestHarmonicComb(experimental::HarmonicCombSnapshot& snapshot)
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    const Consumer consumer = requests_ & bit(compactCarriers) ? compactCarriers
        : requests_ & bit(carrierHistory) ? carrierHistory
        : requests_ & bit(fftCarrierOverlay) ? fftCarrierOverlay : pitchWindow;
    if (!modelPublicationIsCurrent(harmonicModel) || !(requests_ & bit(consumer))
        || modelPublication_->consumers[consumer] != consumerEpochs_[consumer])
    { snapshot = {}; return false; }
    snapshot = modelPublication_->harmonic;
    return true;
}

bool AnalysisStream::latestPitchExperiment(experimental::PitchMotionSnapshot& motion,
    experimental::SpectralCorrelationSnapshot& correlation)
{
    // This overload preserves the inherited observer contract without allocating
    // a large harmonic snapshot on the caller's stack.
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    if (!(requests_ & bit(pitchWindow)) || modelPublication_->consumers[pitchWindow] != consumerEpochs_[pitchWindow]
        || !modelPublicationIsCurrent(motionModel) || !modelPublicationIsCurrent(correlationModel))
    { motion = {}; correlation = {}; return false; }
    motion = modelPublication_->motion; correlation = modelPublication_->correlation;
    return true;
}

bool AnalysisStream::latestPitchExperiment(experimental::PitchMotionSnapshot& motion,
    experimental::SpectralCorrelationSnapshot& correlation, experimental::HarmonicCombSnapshot& harmonic)
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    if (!(requests_ & bit(pitchWindow)) || modelPublication_->consumers[pitchWindow] != consumerEpochs_[pitchWindow]
        || !modelPublicationIsCurrent(motionModel) || !modelPublicationIsCurrent(correlationModel)
        || !modelPublicationIsCurrent(harmonicModel))
    { motion = {}; correlation = {}; harmonic = {}; return false; }
    motion = modelPublication_->motion; correlation = modelPublication_->correlation; harmonic = modelPublication_->harmonic;
    return true;
}

std::uint64_t AnalysisStream::drainWaterfall(std::vector<WaterfallFrame>& frames)
{ return drainHistory(frames, false); }

std::uint64_t AnalysisStream::drainCarrierWaterfall(std::vector<WaterfallFrame>& frames)
{ return drainHistory(frames, true); }

std::uint64_t AnalysisStream::drainHistory(std::vector<WaterfallFrame>& frames, bool carriers)
{
    // All allocation precedes the publication -> inbox lock pair.
    frames.reserve(WaterfallSettings::queueCapacity);
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    auto& inbox = carriers ? carrierWaterfall_ : waterfall_;
    const auto dropped = inbox.drain(frames);
    const Consumer consumer = carriers ? carrierHistory : fftHistory;
    if (!publicationIsCurrent() || !(requests_ & bit(consumer))) { frames.clear(); return dropped; }
    const auto revision = revision_.load(std::memory_order_acquire);
    frames.erase(std::remove_if(frames.begin(), frames.end(), [&](const WaterfallFrame& frame)
    { return frame.revision != revision || (carriers && frame.sourceRevision != carrierSource_); }), frames.end());
    if (!carriers)
        for (auto& frame : frames)
            if (!(requests_ & bit(fftCarrierOverlay)) || frame.sourceRevision != overlaySource_
                || frame.carrierCapture != consumerEpochs_[fftCarrierOverlay])
            { frame.profile = {}; frame.sourceRevision = frame.carrierCapture = 0; }
    return dropped;
}

AnalysisStream::WorkRequest AnalysisStream::captureWorkRequest(const SpectrumFrame& frame)
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    unsigned eligible = requests_;
    for (unsigned i = 0; i < consumerCount; ++i)
        if (inputConsumerEpochs_[i] != consumerEpochs_[i] || frame.firstSample < inputConsumerFirst_[i])
            eligible &= ~(1u << i);
    if (!(eligible & bit(fftHistory))) eligible &= ~bit(fftCarrierOverlay);
    return {eligible, consumerEpochs_, modelEpochs_, settingsRevision_, overlaySource_, carrierSource_, pitchSettings_};
}

void AnalysisStream::calculateModels(const SpectrumFrame& frame, const WorkRequest& request)
{
    using namespace experimental;
    // These are worker-only objects. No snapshot/inbox lock covers preparation,
    // peak transport, correlation, harmonic pursuit or carrier rasterization.
    for (unsigned i = 0; i < modelCount; ++i)
    {
        const auto model = static_cast<Model>(i);
        if (!needsModel(request.mask, model)) continue;
        const bool prepare = models_->epochs[i] != request.modelEpochs[i]
            || models_->settings[i] != request.settingsRevision;
        if (model == motionModel)
        {
            if (!models_->motion) models_->motion = std::make_unique<PitchMotion>();
            if (prepare) models_->prepared[i] = models_->motion->prepare(request.settings) == PitchMotionResult::ok;
            if (models_->prepared[i]) { models_->motion->process(frame); ++modelCalculations_[i]; }
        }
        else if (model == correlationModel)
        {
            if (!models_->correlation) models_->correlation = std::make_unique<SpectralCorrelation>();
            if (prepare) models_->prepared[i] = models_->correlation->prepare(request.settings) == PitchMotionResult::ok;
            if (models_->prepared[i]) { models_->correlation->process(frame); ++modelCalculations_[i]; }
        }
        else
        {
            if (!models_->harmonic) models_->harmonic = std::make_unique<HarmonicCombProfile>();
            if (prepare) models_->prepared[i] = models_->harmonic->prepare(request.settings) == PitchMotionResult::ok
                && request.settings.harmonicComb.enabled;
            if (models_->prepared[i]) { models_->harmonic->process(frame); ++modelCalculations_[i]; }
        }
        models_->epochs[i] = request.modelEpochs[i];
        models_->settings[i] = request.settingsRevision;
    }
}

void AnalysisStream::resetWorkerModels() noexcept
{
    models_->settings.fill(0); // The next requested observation prepares fresh model history.
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
    const auto request = captureWorkRequest(frame);
    calculateModels(frame, request);
    WaterfallProfile profile{};
    if ((request.mask & (bit(carrierHistory) | bit(fftCarrierOverlay)))
        && models_->harmonic && models_->prepared[harmonicModel])
        profile = experimental::harmonicCarrierProfile(models_->harmonic->snapshot());
    publishObservation(frame, request, profile);
}

void AnalysisStream::publishObservation(const SpectrumFrame& frame, const WorkRequest& request,
    const WaterfallProfile& profile)
{
    std::lock_guard<std::mutex> guard(snapshotMutex_);
    if (!enabled() || processingRevision_ != revision_.load(std::memory_order_acquire)) return;
    const bool consecutive = publishedRevision_ == processingRevision_
        && frame.firstSample == snapshot_.firstSample + static_cast<std::uint64_t>(frame.hopSize);
    const auto currentConsumer = [&](Consumer consumer)
    { return (request.mask & requests_ & bit(consumer)) && request.epochs[consumer] == consumerEpochs_[consumer]; };
    if (currentConsumer(mainSpectrum))
    {
        if (!consecutive || publishedMainEpoch_ != consumerEpochs_[mainSpectrum])
        { barPeaks_.reset(); ++barGeneration_; }
        barPeaks_.add(frame.barLeft, frame.barRight);
        publishedMainEpoch_ = consumerEpochs_[mainSpectrum];
        axis_ = {frame.sampleRate, barGeneration_, frame.bandHz, frame.bandEdgesHz};
    }
    snapshot_ = frame;
    publishedRevision_ = processingRevision_;
    publishedAt_ = std::chrono::steady_clock::now();
    const bool currentSettings = request.settingsRevision == settingsRevision_;
    for (unsigned i = 0; i < modelCount; ++i)
    {
        const auto model = static_cast<Model>(i);
        if (!currentSettings || !needsModel(request.mask, model) || !needsModel(requests_, model)
            || request.modelEpochs[i] != modelEpochs_[i]) continue;
        if (model == motionModel) modelPublication_->motion = models_->motion->snapshot();
        else if (model == correlationModel) modelPublication_->correlation = models_->correlation->snapshot();
        else modelPublication_->harmonic = models_->harmonic->snapshot();
        modelPublication_->epochs[i] = request.modelEpochs[i];
        modelPublication_->settings[i] = request.settingsRevision;
    }
    for (unsigned i = 0; i < consumerCount; ++i)
        if (currentConsumer(static_cast<Consumer>(i))) modelPublication_->consumers[i] = request.epochs[i];
    if (currentConsumer(fftHistory))
    {
        auto display = WaterfallFrame::fromSpectrum(frame, formatGeneration_);
        display.revision = processingRevision_;
        if (currentSettings && currentConsumer(fftCarrierOverlay) && request.overlaySource == overlaySource_)
        {
            display.profile = profile; display.sourceRevision = request.overlaySource;
            display.carrierCapture = request.epochs[fftCarrierOverlay];
        }
        waterfall_.publish(display);
    }
    if (currentSettings && currentConsumer(carrierHistory) && request.carrierSource == carrierSource_)
    {
        auto display = WaterfallFrame::fromSpectrum(frame, formatGeneration_);
        display.hasProfile = true; display.profile = profile;
        display.sourceRevision = request.carrierSource; display.revision = processingRevision_;
        carrierWaterfall_.publish(display);
    }
}

void AnalysisStream::processQueuedBlock(const Block& block)
{
    AnalysisResult result = AnalysisResult::ok;
    const bool reconfigure = processingRevision_ != block.revision
        || analyzer_.config().sampleRate != block.sampleRate || analyzer_.config().channels != block.channels;
    if (reconfigure || (haveInput_ && block.first != nextInput_))
    {
        // Detect gaps from input positions, even a backward seek whose
        // next FFT window would happen to look consecutive. Hide the old
        // result immediately while the replacement window fills.
        std::lock_guard<std::mutex> guard(snapshotMutex_);
        publishedRevision_ = 0;
        barPeaks_.reset();
        ++formatGeneration_;
        waterfall_.setEnabled((requests_ & bit(fftHistory)) != 0);
        carrierWaterfall_.setEnabled((requests_ & bit(carrierHistory)) != 0);
        resetWorkerModels();
        inputConsumerEpochs_.fill(0);
    }
    if (reconfigure)
    {
        result = analyzer_.prepare({static_cast<double>(block.sampleRate), block.channels});
        processingRevision_ = block.revision;
    }
    {
        // A reopened view waits for a complete window from PCM tokenized in its
        // new session. Shared queued PCM still serves the older consumers.
        std::lock_guard<std::mutex> guard(snapshotMutex_);
        for (unsigned i = 0; i < consumerCount; ++i)
            if ((requests_ & (1u << i)) && inputConsumerEpochs_[i] != consumerEpochs_[i]
                && block.requestSession >= consumerStartSessions_[i])
            {
                inputConsumerEpochs_[i] = consumerEpochs_[i];
                inputConsumerFirst_[i] = block.first;
            }
    }
    haveInput_ = true;
    nextInput_ = block.first + static_cast<std::uint64_t>(block.frames);
    if (result == AnalysisResult::ok)
        result = analyzer_.processBlock(block.pcm.data(), block.frames, block.first, *this);
    result_.store(result);
    if (result != AnalysisResult::ok) invalidate();
}

void AnalysisStream::run() noexcept
{
    // Exception boundary for allocation failures in preparation / snapshot copies.
    try
    {
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
                processQueuedBlock(block);
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
