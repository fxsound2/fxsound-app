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
#include "FxHotkeyLabel.h"
#include "FxLanguage.h"

class FxGeneralSettingsPane : public FxSettingsPane
{
public:
	FxGeneralSettingsPane();
	~FxGeneralSettingsPane() override;

	void resized() override;
	void paint(Graphics& g) override;

private:
	static constexpr int LANGUAGE_SWITCH_Y = 50;
	static constexpr int TOGGLE_BUTTON_HEIGHT = 30;
	static constexpr int HOTKEY_LABEL_X = X_MARGIN + 25;
	static constexpr int HOTKEY_LABEL_HEIGHT = 20;

    void setText();

    ToggleButton launch_toggle_;
    ToggleButton hide_help_tips_toggle_;
	ToggleButton hide_notifications_toggle_;
	ToggleButton hotkeys_toggle_;
	OwnedArray<FxHotkeyLabel> hotkey_labels_;
	FxLanguage language_switch_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxGeneralSettingsPane)
};
