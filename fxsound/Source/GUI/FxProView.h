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
#include "FxView.h"
#include "FxEffects.h"
#include "FxEqualizer.h"
#include "FxVisualizer.h"

//==============================================================================
/*
*/
class FxProView  : public FxView
{
public:
    FxProView();
    ~FxProView() = default;

	void startVisualizer();
	void pauseVisualizer();

	void update();

	void resized() override;
	void paint(Graphics& g) override;

private:
	static constexpr int WIDTH = 960;
	static constexpr int HEIGHT = 474;
	static constexpr int PRESET_LIST_X = 40;
	static constexpr int OUTPUT_LIST_X = 500;
	static constexpr int LIST_Y = 32;
	static constexpr int EFFECTS_X = 40;
	static constexpr int EFFECTS_Y = 88;
	static constexpr int LIST_WIDTH = 420;
	static constexpr int LIST_HEIGHT = 40;

	void comboBoxChanged(ComboBox* combobox) override;
	void modelChanged(FxModel::Event model_event) override;

	void mouseEnter(const MouseEvent& mouse_event) override;
	void mouseExit(const MouseEvent& mouse_event) override;

	FxEffects effects_;
	FxEqualizer equalizer_;
    TooltipWindow tool_tip_;
    FxVisualizer visualizer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxProView)
};
