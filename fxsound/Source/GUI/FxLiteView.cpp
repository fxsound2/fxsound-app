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

#include <JuceHeader.h>
#include "FxLiteView.h"
#include "FxController.h"

//==============================================================================
FxLiteView::FxLiteView()
{	
	setOpaque(false);
	setSize(WIDTH, HEIGHT);
}

void FxLiteView::resized()
{
	RectanglePlacement placement(RectanglePlacement::xMid | RectanglePlacement::doNotResize);

	auto bounds = getLocalBounds();
	auto component_bounds = preset_list_.getLocalBounds();

	component_bounds.setX(PRESET_LIST_X);
	component_bounds.setY(LIST_Y);
	preset_list_.setBounds(component_bounds);

	component_bounds = endpoint_list_.getLocalBounds();
	component_bounds.setX(OUTPUT_LIST_X);
	component_bounds.setY(LIST_Y);
	endpoint_list_.setBounds(component_bounds);
}

void FxLiteView::paint(Graphics& g)
{
	auto& theme = dynamic_cast<LookAndFeel_V4&>(getLookAndFeel());

	g.setFillType(FillType(theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::windowBackground)));
	g.fillAll();

	g.setFillType(FillType(Colour(0x0).withAlpha(0.2f)));
	g.fillRoundedRectangle(20, 22, BACKGROUND_WIDTH, BACKGROUND_HEIGHT, 10);

    auto power_state = FxModel::getModel().getPowerState();
    preset_list_.setEnabled(power_state);
}