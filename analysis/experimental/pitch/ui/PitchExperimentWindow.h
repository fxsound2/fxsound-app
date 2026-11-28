// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "PitchMotionIndicator.h"
#include "SpectralCorrelationIndicator.h"
#include "HarmonicCombIndicator.h"
#include "../PitchExperimentSettings.h"

namespace fxanalysis::experimental
{
class PitchExperimentContent;

// The host owns this window and supplies snapshots on its message thread.
class PitchExperimentWindow final : public juce::DocumentWindow
{
public:
    explicit PitchExperimentWindow(const juce::String& title = "Pitch movement - experiment");
    ~PitchExperimentWindow() override;
    void setSnapshot(const PitchMotionSnapshot& snapshot);
    void setCorrelationSnapshot(const SpectralCorrelationSnapshot& snapshot);
    void setHarmonicSnapshot(const HarmonicCombSnapshot& snapshot);
    void clear();
    void setSettings(const PitchMotionSettings& settings);
    void setCompactIndicatorMode(CompactIndicatorMode mode);
    const PitchMotionSettings& settings() const noexcept;
    PitchMotionIndicator& indicator() noexcept;
    SpectralCorrelationIndicator& correlationIndicator() noexcept;
    HarmonicCombIndicator& harmonicIndicator() noexcept;
    void setCorrelationView(bool correlation);
    void setHarmonicView();
    void show();
    void closeButtonPressed() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;
    std::function<void(const PitchMotionSettings&)> onSettingsChanged;
    std::function<void()> onClosed;
    std::function<void()> onVisibilityChanged;
    std::function<void(CompactIndicatorMode)> onCompactIndicatorChanged;
private:
    PitchExperimentContent* content_ = nullptr; // Owned by DocumentWindow.
};
}
