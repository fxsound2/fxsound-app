// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#if defined(JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED) || defined(JUCE_APP_CONFIG_HEADER)
#include <juce_gui_basics/juce_gui_basics.h>
#else
#include <JuceHeader.h>
#endif
#include "../SpectralCorrelation.h"
#include <array>

namespace fxanalysis::experimental
{
// Message-thread rendering only. History is bounded and advances with source time.
class SpectralCorrelationIndicator final : public juce::Component, public juce::SettableTooltipClient
{
public:
    SpectralCorrelationIndicator();
    void setSnapshot(const SpectralCorrelationSnapshot& snapshot);
    void clear();
    void setTrajectoryLimitSemitones(double limit);
    const SpectralCorrelationSnapshot& snapshot() const noexcept { return snapshot_; }
    int historySize() const noexcept { return historyCount_; }
    void paint(juce::Graphics& graphics) override;
private:
    static constexpr int historyCapacity = 512;
    static constexpr double historySeconds = 10.0;
    struct HistoryPoint
    {
        double time = 0.0;
        std::array<double, spectralCorrelationLagCount> displacement{};
        std::array<bool, spectralCorrelationLagCount> valid{};
    };
    void drawCorrelation(juce::Graphics& graphics, juce::Rectangle<float> plot);
    void drawHistory(juce::Graphics& graphics, juce::Rectangle<float> plot);
    void drawIntegral(juce::Graphics& graphics, juce::Rectangle<float> area);
    SpectralCorrelationSnapshot snapshot_;
    std::array<HistoryPoint, historyCapacity> history_{};
    int historyBegin_ = 0, historyCount_ = 0;
    double lastInputTime_ = 0.0, trajectoryLimitSemitones_ = 3.0;
    bool haveInputTime_ = false;
};
}