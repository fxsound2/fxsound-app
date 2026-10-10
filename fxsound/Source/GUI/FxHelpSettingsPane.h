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
#include "FxHyperlink.h"

class FxHelpSettingsPane : public FxSettingsPane
{
public:
	FxHelpSettingsPane();
	~FxHelpSettingsPane() override = default;

	void resized() override;
	void paint(Graphics& g) override;

private:
	static constexpr int TEXT_Y = 50;
	static constexpr int TEXT_HEIGHT = 20;
	static constexpr int HYPERLINK_HEIGHT = 24;
	static constexpr int TOGGLE_BUTTON_HEIGHT = 24;
	static constexpr int BUTTON_WIDTH = 220;

    void setText();

	Label version_title_;
	Label support_title_;
	Label maintenance_title_;
	Label version_text_;
	FxHyperlink changelog_link_;
	FxHyperlink quicktour_link_;
	FxHyperlink submitlogs_link_;
	FxHyperlink helpcenter_link_;
	FxHyperlink feedback_link_;
	ToggleButton auto_updates_toggle_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxHelpSettingsPane)
};
