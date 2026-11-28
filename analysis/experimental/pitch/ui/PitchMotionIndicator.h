// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#if defined(JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED) || defined(JUCE_APP_CONFIG_HEADER)
#include <juce_gui_basics/juce_gui_basics.h>
#else
#include <JuceHeader.h>
#endif
#include "../PitchMotion.h"
#include <functional>

namespace fxanalysis::experimental
{
// Message-thread view only. The estimator, not repaint cadence, owns all integration.
class PitchMotionIndicator final : public juce::Component, public juce::SettableTooltipClient
{
public:
    PitchMotionIndicator();
    void setSnapshot(const PitchMotionSnapshot& snapshot);
    void clear();
    void setCompact(bool compact);
    void setShowSmoothed(bool smoothed);
    void setTrajectoryLimitSemitones(double limit);
    bool showSmoothed() const noexcept { return showSmoothed_; }
    const PitchMotionSnapshot& snapshot() const noexcept { return snapshot_; }
    std::function<void()> onDoubleClick;
    void paint(juce::Graphics& graphics) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
private:
    void drawHistogram(juce::Graphics& graphics, juce::Rectangle<float> plot);
    void drawTrajectory(juce::Graphics& graphics, juce::Rectangle<float> area);
    PitchMotionSnapshot snapshot_;
    double trajectoryLimitSemitones_ = 3.0;
    bool compact_ = false;
    bool showSmoothed_ = true;
};
}