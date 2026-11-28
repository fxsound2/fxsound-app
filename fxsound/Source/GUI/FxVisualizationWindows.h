// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "FxVisualizationOptions.h"
#include "runtime/AnalysisStream.h"
#include "experimental/pitch/PitchExperimentSettings.h"
#include <functional>
#include <memory>

namespace fxanalysis { class WaterfallWindow; }
namespace fxanalysis::experimental { class PitchExperimentWindow; struct HarmonicCombSnapshot; }

// Message-thread ownership of the optional observation windows. This adapter has
// no devices, presets or profile; its source is the existing analysis worker.
class FxVisualizationWindows final
{
public:
    FxVisualizationWindows(fxanalysis::AnalysisStream& stream,
        std::function<void()> requestsChanged, std::function<void()> layoutChanged,
        std::function<void(const juce::String&)> reportFailure);
    ~FxVisualizationWindows();
    void showSpectrumHistory();
    void showCarrierHistory();
    void showPitchExperiment();
    void setCompactIndicator(FxCompactIndicator choice);
    FxCompactIndicator compactIndicator() const noexcept { return compact_; }
    const fxanalysis::experimental::PitchMotionSettings& settings() const noexcept { return settings_; }
    void refreshClocks(bool outputEligible);
    fxanalysis::AnalysisRequests requests() const noexcept;
    void refreshSnapshots();
    void shutdown();

private:
    void notifyRequestsChanged();
    void applySettings(const fxanalysis::experimental::PitchMotionSettings& settings);
    void bindHistory(fxanalysis::WaterfallWindow& window, bool carriers);
    static bool windowIsShowing(const juce::Component* window) noexcept;
    fxanalysis::AnalysisStream& stream_;
    std::function<void()> requestsChanged_, layoutChanged_;
    std::function<void(const juce::String&)> reportFailure_;
    std::unique_ptr<fxanalysis::WaterfallWindow> spectrum_, carriers_;
    std::unique_ptr<fxanalysis::experimental::PitchExperimentWindow> pitch_;
    std::unique_ptr<fxanalysis::experimental::HarmonicCombSnapshot> harmonicSnapshot_;
    fxanalysis::experimental::PitchMotionSettings settings_;
    FxCompactIndicator compact_ = FxCompactIndicator::none;
    bool outputEligible_ = false, overlayRequested_ = false;
    bool spectrumFailed_ = false, carriersFailed_ = false;
    bool refreshingClocks_ = false, shuttingDown_ = false;
};
