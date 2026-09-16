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
#include "FxAudioSlider.h"

//==============================================================================
FxAudioSlider::FxAudioSlider(String label_format, float default_value) : default_value_(default_value)
{
	label_format_ = label_format;

	setValue(default_value);

	setMouseCursor(MouseCursor::PointingHandCursor);

	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

	value_label_.setFont(theme.getSmallFont().withHeight(11.0f));
	value_label_.setJustificationType(Justification::centredLeft);
	addAndMakeVisible(value_label_);

	setWantsKeyboardFocus(true);
}

void FxAudioSlider::resized()
{
	Slider::resized();

	// Always update label position & text after layout
	updateLabel();
}

void FxAudioSlider::valueChanged()
{
	updateLabel();
}

bool FxAudioSlider::keyPressed(const KeyPress& key)
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

void FxAudioSlider::setWheelStepsByInterval(bool steps_by_interval)
{
	wheel_steps_by_interval_ = steps_by_interval;
}

bool FxAudioSlider::stepByWheel(Slider& slider, const MouseWheelDetails& wheel)
{
	if (wheel.isSmooth || !slider.isEnabled() || !slider.isScrollWheelEnabled())
	{
		return false;
	}

	float delta = (std::abs(wheel.deltaX) > std::abs(wheel.deltaY)) ? -wheel.deltaX : wheel.deltaY;
	if (wheel.isReversed)
	{
		delta = -delta;
	}
	if (delta != 0.0f)
	{
		slider.setValue(slider.getValue() + ((delta > 0.0f) ? slider.getInterval() : -slider.getInterval()), NotificationType::sendNotificationSync);
	}
	return true;
}

void FxAudioSlider::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
	if (!wheel_steps_by_interval_ || !stepByWheel(*this, wheel))
	{
		Slider::mouseWheelMove(event, wheel);
	}
}

void FxAudioSlider::mouseDown(const juce::MouseEvent& event)
{
	// Check if right mouse button is pressed, or this is a double-click
	if (event.mods.isRightButtonDown() || event.getNumberOfClicks() >= 2)
	{
		// Reset the slider to its default value. Handled here (rather than in
		// mouseDoubleClick) and returning without calling the base class so the
		// base Slider never starts its own drag-tracking - otherwise the drag
		// started by this same mouseDown gets finalized on mouseUp using the
		// pre-reset value, silently undoing the reset performed in between by
		// mouseDoubleClick.
		setValue(default_value_, NotificationType::sendNotification);
	}
	else
	{
		// Default behavior for other buttons
		Slider::mouseDown(event);
	}
}

void FxAudioSlider::setZeroValueText(const String& text)
{
	if (zero_value_text_ != text)
	{
		zero_value_text_ = text;
		updateLabel();
	}
}

void FxAudioSlider::updateLabel()
{
	auto value = getValue();

	// Update label text
	auto text = (zero_value_text_.isNotEmpty() && value == 0.0) ? zero_value_text_ : String::formatted(label_format_, value);
	value_label_.setText(text, NotificationType::dontSendNotification);

	// Position label based on slider thumb position
	auto pos = getPositionOfValue(value);
	auto x = static_cast<int>(pos) + FxTheme::SLIDER_THUMB_RADIUS / 2 + 1;
	value_label_.setBounds(x, (getHeight() - LABEL_HEIGHT) / 2, LABEL_WIDTH, LABEL_HEIGHT);
}