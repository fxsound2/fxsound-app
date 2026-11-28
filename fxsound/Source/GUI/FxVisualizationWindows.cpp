// SPDX-License-Identifier: AGPL-3.0-or-later
#include "FxVisualizationWindows.h"
#include "ui/WaterfallWindow.h"
#include "experimental/pitch/ui/PitchExperimentWindow.h"
#include "experimental/pitch/HarmonicCarrierDisplay.h"
#include <exception>

using namespace juce;
using namespace fxanalysis;
using namespace fxanalysis::experimental;

FxVisualizationWindows::FxVisualizationWindows(AnalysisStream& stream,
    std::function<void()> requestsChanged, std::function<void()> layoutChanged,
    std::function<void(const String&)> reportFailure)
    : stream_(stream), requestsChanged_(std::move(requestsChanged)),
      layoutChanged_(std::move(layoutChanged)), reportFailure_(std::move(reportFailure))
{
    // The numerical defaults are inherited; embedding a band is an explicit UI
    // choice, independent of opening any of the analysis windows.
    settings_.harmonicComb.showInStrip = false;
    stream_.setPitchMotionSettings(settings_);
}

FxVisualizationWindows::~FxVisualizationWindows() { shutdown(); }

bool FxVisualizationWindows::windowIsShowing(const Component* window) noexcept
{
    return window != nullptr && window->isShowing();
}

void FxVisualizationWindows::notifyRequestsChanged()
{
    // setActive() can synchronously report its overlay request. The controller
    // reads requests after refreshClocks(), so this needs no recursive refresh.
    if (!shuttingDown_ && !refreshingClocks_ && requestsChanged_) requestsChanged_();
}

void FxVisualizationWindows::bindHistory(WaterfallWindow& window, bool carrierHistory)
{
    window.view().setSource([this, carrierHistory](std::vector<WaterfallFrame>& frames)
    {
        return carrierHistory ? stream_.drainCarrierWaterfall(frames) : stream_.drainWaterfall(frames);
    });
    window.view().onFailure = [this, carrierHistory](const String& message)
    {
        (carrierHistory ? carriersFailed_ : spectrumFailed_) = true;
        if (reportFailure_) reportFailure_(message);
        notifyRequestsChanged();
    };
    window.onClosed = [this] { notifyRequestsChanged(); };
    window.onVisibilityChanged = [this] { notifyRequestsChanged(); };
}

void FxVisualizationWindows::showSpectrumHistory()
{
    if (shuttingDown_) return;
    try
    {
        if (!spectrum_)
        {
            spectrum_ = std::make_unique<WaterfallWindow>(TRANS("FxSound - Spectrum history"), "FFT");
            bindHistory(*spectrum_, false);
            spectrum_->view().enableCarrierOverlay(harmonicCarrierGold, [this](bool requested)
            {
                overlayRequested_ = requested;
                notifyRequestsChanged();
            });
        }
        if (spectrumFailed_) spectrum_->view().clearHistory();
        spectrumFailed_ = false;
        spectrum_->show();
        notifyRequestsChanged();
    }
    catch (const std::exception& error)
    {
        spectrumFailed_ = true;
        if (reportFailure_) reportFailure_(TRANS("Cannot open spectrum history") + ": " + error.what());
        notifyRequestsChanged();
    }
}

void FxVisualizationWindows::showCarrierHistory()
{
    if (shuttingDown_) return;
    try
    {
        if (!carriers_)
        {
            carriers_ = std::make_unique<WaterfallWindow>(TRANS("FxSound - Carrier history"), TRANS("Carriers"));
            bindHistory(*carriers_, true);
        }
        if (carriersFailed_) carriers_->view().clearHistory();
        carriersFailed_ = false;
        carriers_->show();
        notifyRequestsChanged();
    }
    catch (const std::exception& error)
    {
        carriersFailed_ = true;
        if (reportFailure_) reportFailure_(TRANS("Cannot open carrier history") + ": " + error.what());
        notifyRequestsChanged();
    }
}

void FxVisualizationWindows::showPitchExperiment()
{
    if (shuttingDown_) return;
    try
    {
        if (!pitch_)
        {
            if (!harmonicSnapshot_) harmonicSnapshot_ = std::make_unique<HarmonicCombSnapshot>();
            pitch_ = std::make_unique<PitchExperimentWindow>(TRANS("FxSound - Experimental pitch analysis"));
            pitch_->setSettings(settings_);
            pitch_->setCompactIndicatorMode(compact_);
            pitch_->onSettingsChanged = [this](const PitchMotionSettings& settings) { applySettings(settings); };
            pitch_->onClosed = [this] { notifyRequestsChanged(); };
            pitch_->onVisibilityChanged = [this] { notifyRequestsChanged(); };
            pitch_->onCompactIndicatorChanged = [this](CompactIndicatorMode mode) { setCompactIndicator(mode); };
        }
        pitch_->show();
        notifyRequestsChanged();
    }
    catch (const std::exception& error)
    {
        if (reportFailure_) reportFailure_(TRANS("Cannot open pitch analysis") + ": " + error.what());
    }
}

void FxVisualizationWindows::applySettings(const PitchMotionSettings& settings)
{
    settings_ = settings;
    settings_.harmonicComb.showInStrip = compact_ == FxCompactIndicator::carriers;
    stream_.setPitchMotionSettings(settings_);
    notifyRequestsChanged();
}

void FxVisualizationWindows::setCompactIndicator(FxCompactIndicator choice)
{
    if (shuttingDown_ || compact_ == choice) return;
    compact_ = choice;
    settings_.harmonicComb.showInStrip = choice == FxCompactIndicator::carriers;
    // showInStrip is presentation state, not an estimator setting. In
    // particular, changing this band must not reset an open history window.
    if (pitch_)
    {
        pitch_->setSettings(settings_);
        pitch_->setCompactIndicatorMode(compact_);
    }
    if (layoutChanged_) layoutChanged_();
    notifyRequestsChanged();
}

void FxVisualizationWindows::refreshClocks(bool outputEligible)
{
    const ScopedValueSetter<bool> refreshing(refreshingClocks_, true);
    outputEligible_ = outputEligible && !shuttingDown_;
    if (spectrum_) spectrum_->view().setActive(outputEligible_ && windowIsShowing(spectrum_.get()) && !spectrumFailed_);
    if (carriers_) carriers_->view().setActive(outputEligible_ && windowIsShowing(carriers_.get()) && !carriersFailed_);
}

AnalysisRequests FxVisualizationWindows::requests() const noexcept
{
    AnalysisRequests requested;
    if (!outputEligible_ || shuttingDown_) return requested;
    requested.fftHistory = windowIsShowing(spectrum_.get()) && !spectrumFailed_;
    requested.carrierHistory = windowIsShowing(carriers_.get()) && !carriersFailed_;
    requested.pitchWindow = windowIsShowing(pitch_.get());
    requested.fftCarrierOverlay = requested.fftHistory && overlayRequested_;
    return requested;
}

void FxVisualizationWindows::refreshSnapshots()
{
    if (!pitch_ || !windowIsShowing(pitch_.get())) return;
    // The worker publishes all three results for one observation/settings revision.
    // Large harmonic state is kept off the host stack.
    PitchMotionSnapshot movement;
    SpectralCorrelationSnapshot correlation;
    if (!outputEligible_ || !harmonicSnapshot_ || !stream_.latestPitchExperiment(movement, correlation, *harmonicSnapshot_))
    {
        pitch_->clear();
        return;
    }
    pitch_->setSnapshot(movement);
    pitch_->setCorrelationSnapshot(correlation);
    pitch_->setHarmonicSnapshot(*harmonicSnapshot_);
}

void FxVisualizationWindows::shutdown()
{
    if (shuttingDown_) return;
    shuttingDown_ = true;
    outputEligible_ = false;
    for (auto* window : {spectrum_.get(), carriers_.get()})
    {
        if (window == nullptr) continue;
        window->view().setActive(false);
        window->view().setSource({});
        window->view().onFailure = {};
        window->onClosed = {};
        window->onVisibilityChanged = {};
    }
    if (pitch_)
    {
        pitch_->onSettingsChanged = {};
        pitch_->onClosed = {};
        pitch_->onVisibilityChanged = {};
        pitch_->onCompactIndicatorChanged = {};
    }
    spectrum_.reset();
    carriers_.reset();
    pitch_.reset();
    harmonicSnapshot_.reset();
}
