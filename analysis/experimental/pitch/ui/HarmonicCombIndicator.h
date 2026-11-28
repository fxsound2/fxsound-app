// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../HarmonicCombProfile.h"
#if defined(JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED) || defined(JUCE_APP_CONFIG_HEADER)
#include <juce_gui_basics/juce_gui_basics.h>
#else
#include <JuceHeader.h>
#endif
#include <functional>

namespace fxanalysis::experimental
{
class HarmonicCombIndicator final : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    HarmonicCombIndicator();
    void setSnapshot(const HarmonicCombSnapshot& snapshot);
    const HarmonicCombSnapshot& snapshot() const noexcept { return snapshot_; }
    void setSettings(const HarmonicCombSettings& settings);
    void setCompact(bool compact);
    void setStripMode(CompactIndicatorMode mode);
    void setShowDiagnostics(bool show);
    void clear();
    void clearTrail();
    void setUnavailable(); // End the bright current observation; let existing stamps fade.
    void paint(juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    std::function<void(const HarmonicCombSettings&)> onSettingsChanged;
    std::function<void()> onDoubleClick;
    std::function<void(CompactIndicatorMode)> onStripModeChanged;
private:
    void changed();
    void timerCallback() override;
    void updateTrailTimer();
    void drawTrail(juce::Graphics&, juce::Rectangle<float>);
    void drawProfiles(juce::Graphics& graphics, juce::Rectangle<float> area);
    HarmonicCombSettings settings_;
    HarmonicCombSnapshot snapshot_;
    HarmonicMarkerTrail trail_;
    juce::Image trailMask_;
    bool compact_ = false;
    bool diagnosticsVisible_ = false;
    CompactIndicatorMode stripMode_ = CompactIndicatorMode::none;
    juce::TextButton enabled_, strip_, scale_, reference_, mode_, diagnostics_;
    juce::Slider subtraction_, decay_, gain_, range_, spread_, persistence_;
    juce::Label subtractionLabel_, decayLabel_, gainLabel_, rangeLabel_, spreadLabel_, persistenceLabel_;
};
}
