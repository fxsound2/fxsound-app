/*
FxSound
Copyright (C) 2025  FxSound LLC

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

#include <JuceHeader.h>
#include "FxSettingsPane.h"
#include "FxOutputPreference.h"

class FxAudioSettingsPane : public FxSettingsPane
{
public:
	FxAudioSettingsPane();
	~FxAudioSettingsPane() override;

	void resized() override;
	void paint(Graphics& g) override;

private:
	static constexpr int GROUP_MARGIN = 10;
	static constexpr int ENDPOINT_Y = 50;
	static constexpr int LABEL_WIDTH = 220;
	static constexpr int OUTPUT_PREFERENCE_HEIGHT = 260;
	static constexpr int LABEL_HEIGHT = 14;
	static constexpr int TOGGLE_BUTTON_HEIGHT = 30;
	static constexpr int RESET_PRESETS_BUTTON_WIDTH = 220;
	static constexpr int BUTTON_HEIGHT = 24;
	static constexpr int MAX_BUTTON_WIDTH = 315;

	void setText();
	void resizeResetButton(int x, int y);

	void visibilityChanged() override;
	void mouseEnter(const MouseEvent& mouse_event) override;
	void mouseExit(const MouseEvent& mouse_event) override;

	Label output_preference_title_;
	FxOutputPreference output_preference_;
	ToggleButton prioritize_new_output_toggle_;

	TextButton reset_presets_button_;

	juce::Rectangle<float> output_preference_bounds_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxAudioSettingsPane)
};
