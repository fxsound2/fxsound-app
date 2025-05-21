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

#include "FxPowerButton.h"

FxPowerButton::FxPowerButton(const String &button_name) : DrawableButton(button_name, DrawableButton::ButtonStyle::ImageFitted)
{
	power_state_ = false;

	power_on_image_ = Drawable::createFromImageData(BinaryData::power_on_svg, BinaryData::power_on_svgSize);
	power_off_image_ = Drawable::createFromImageData(BinaryData::power_off_svg, BinaryData::power_off_svgSize);
}

void FxPowerButton::paint(Graphics& g)
{
	Rectangle<int> image_area(0, 0, image_width_, image_width_);
	auto bounds = getLocalBounds();
	RectanglePlacement placement(RectanglePlacement::xMid
		| RectanglePlacement::yMid
		| RectanglePlacement::doNotResize);

	if (power_state_)
	{
		power_on_image_->drawWithin(g, placement.appliedTo(image_area, bounds).toFloat(),
			RectanglePlacement(RectanglePlacement::Flags::stretchToFit | RectanglePlacement::Flags::centred), 1.0f);
	}
	else
	{
		power_off_image_->drawWithin(g, placement.appliedTo(image_area, bounds).toFloat(),
			RectanglePlacement(RectanglePlacement::Flags::stretchToFit | RectanglePlacement::Flags::centred), 1.0f);
	}
}

bool FxPowerButton::keyPressed(const KeyPress& key)
{
	if (isEnabled() && key.isKeyCode(KeyPress::spaceKey))
	{
		triggerClick();
		return true;
	}

	return false;
}

bool FxPowerButton::getPowerState()
{
	return power_state_;
}

void FxPowerButton::setPowerState(bool power_state)
{
	power_state_ = power_state;
	repaint();
}

int FxPowerButton::getImageWidth()
{
	return image_width_;
}

void FxPowerButton::setImageWidth(int image_width)
{
	image_width_ = image_width;
}