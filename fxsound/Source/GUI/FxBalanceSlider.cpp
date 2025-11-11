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
#include "FxController.h"
#include "FxTheme.h"
#include "FxBalanceSlider.h"

//==============================================================================
FxBalanceSlider::FxBalanceSlider(float default_value) : default_value_(default_value)
{
	slider_thumb_ = Drawable::createFromImageData(FXIMAGE(SliderThumb), FXIMAGESIZE(SliderThumb));
	slider_thumb_grey_ = Drawable::createFromImageData(FXIMAGE(SliderThumbBW), FXIMAGESIZE(SliderThumbBW));

	setMouseCursor(MouseCursor::PointingHandCursor);

	auto& controller = FxController::getInstance();
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

	setSliderStyle(Slider::LinearHorizontal);
	setRange(-20, 20, 2);
	setValue(controller.getBalance());
	setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	setWantsKeyboardFocus(true);

	value_label_.setFont(theme.getNormalFont().withHeight(12.0f));
	value_label_.setJustificationType(Justification::centredLeft);

	addAndMakeVisible(value_label_);
}

void FxBalanceSlider::valueChanged()
{
	auto value = getValue();
	auto& controller = FxController::getInstance();

	if (controller.getBalance() != value)
		controller.setBalance((float)value);

	updateValueLabel();
}

void FxBalanceSlider::resized()
{
	Slider::resized();

	updateValueLabel();
}

void FxBalanceSlider::paint(Graphics& g)
{
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
	auto layout = theme.getSliderLayout(*this);

	auto radius = FxTheme::SLIDER_THUMB_RADIUS;
	auto bounds = layout.sliderBounds;
	auto x = bounds.getX();
	auto y = bounds.getY();
	auto width = bounds.getWidth();
	auto height = bounds.getHeight();
	auto pos = getPositionOfValue(getValue());

	auto value = getValue();
	float scaled_value = (value - (-20.0f)) / (20.0f - (-20.0f));

	Colour left_colour = Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f - scaled_value);
	Colour right_colour = Colour(FXCOLOR(SliderTrack)).withAlpha(scaled_value);
	if (!isEnabled())
	{
		left_colour = left_colour.withSaturation(0.0);
		right_colour = right_colour.withSaturation(0.0);
	}

	ColourGradient gradient = ColourGradient::horizontal(left_colour, x, right_colour, width);

	g.setGradientFill(gradient);
	g.fillRoundedRectangle(x, y + (height - 3) / 2, width, 3, 5.6f);

	if (isEnabled())
		slider_thumb_->drawWithin(g, juce::Rectangle<float>(pos - radius, y + height / 2 - radius, radius * 2, radius * 2), { RectanglePlacement::centred }, 1.0f);
	else
		slider_thumb_grey_->drawWithin(g, juce::Rectangle<float>(pos - radius, y + height / 2 - radius, radius * 2, radius * 2), { RectanglePlacement::centred }, 1.0f);

	if (hasKeyboardFocus(true))
	{
		Colour colour = Colour(FXCOLOR(SliderHighlight)).withAlpha(0.1f);
		g.setFillType(colour);
		g.fillRoundedRectangle(juce::Rectangle<float>(x, y, width, height).expanded(FxTheme::SLIDER_THUMB_RADIUS / 2, FxTheme::SLIDER_THUMB_RADIUS / 2), height + FxTheme::SLIDER_THUMB_RADIUS);
	}
}

bool FxBalanceSlider::keyPressed(const KeyPress& key)
{
	if (isEnabled())
	{
		if (key.isKeyCode(KeyPress::upKey))
		{
			setValue(getValue() + getInterval());
			return true;
		}
		else if (key.isKeyCode(KeyPress::downKey))
		{
			setValue(getValue() - getInterval());
			return true;
		}
	}

	return false;
}

void FxBalanceSlider::mouseDown(const juce::MouseEvent& event)
{
	// Check if right mouse button is pressed
	if (event.mods.isRightButtonDown())
	{
		// Reset the slider to 0
		setValue(default_value_, NotificationType::sendNotification);
	}
	else
	{
		// Default behavior for other buttons
		Slider::mouseDown(event);
	}
}

void FxBalanceSlider::updateValueLabel()
{
	auto value = getValue();

	// Update value label text
	auto text = String::formatted("%0.0f dB", std::fabs(value));
	value_label_.setText(text, NotificationType::dontSendNotification);

	// Position label based on slider thumb position
	auto pos = getPositionOfValue(value);
	auto x = pos + FxTheme::SLIDER_THUMB_RADIUS / 2 + 1;
	value_label_.setBounds(x, (getHeight() - LABEL_HEIGHT) / 2, LABEL_WIDTH, LABEL_HEIGHT);
}
