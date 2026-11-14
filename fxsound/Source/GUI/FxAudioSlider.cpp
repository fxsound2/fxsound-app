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
	// The label sits on the slider: clicks on it go to the slider, so a right-click or double-click on the value resets
	// it like anywhere else on the slider.
	value_label_.setInterceptsMouseClicks(false, false);
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

void FxAudioSlider::setWheelMaxNotchStep(double notch_step)
{
	wheel_max_notch_step_ = notch_step;
}

static float wheelDelta(const MouseWheelDetails& wheel)
{
	float delta = (std::abs(wheel.deltaX) > std::abs(wheel.deltaY)) ? -wheel.deltaX : wheel.deltaY;

	return wheel.isReversed ? -delta : delta;
}

bool FxAudioSlider::stepByWheel(Slider& slider, const MouseWheelDetails& wheel)
{
	if (wheel.isSmooth || !slider.isEnabled() || !slider.isScrollWheelEnabled())
	{
		return false;
	}

	float delta = wheelDelta(wheel);
	if (delta != 0.0f)
	{
		slider.setValue(slider.getValue() + ((delta > 0.0f) ? slider.getInterval() : -slider.getInterval()), NotificationType::sendNotificationSync);
	}
	return true;
}

// Every notch counts (Windows merges the notches of a fast spin into one event; finer wheels report less than a notch
// per event and count as one) and moves by a step that grows with the logarithm of the wheel speed: one interval up to
// WHEEL_SLOW_NOTCHES_PER_S, wheel_max_notch_step_ from WHEEL_FAST_NOTCHES_PER_S, the same increase for every doubling in
// between. The speed is a running average, so one late or early event does not make the step jump, and the fraction
// of a step the value cannot take (it moves in whole intervals) carries over to the next notch.
bool FxAudioSlider::stepByWheelAccelerated(const MouseEvent& event, const MouseWheelDetails& wheel)
{
	if (wheel.isSmooth || !isEnabled() || !isScrollWheelEnabled())
	{
		return false;
	}

	float delta = wheelDelta(wheel);
	if (delta == 0.0f)
	{
		return true;
	}

	int notches = jmax(1, roundToInt(std::abs(delta) / WHEEL_NOTCH_DELTA));
	int direction = (delta > 0.0f) ? 1 : -1;
	auto since_last = (last_wheel_time_ != Time()) ? (event.eventTime - last_wheel_time_).inMilliseconds() : (int64)WHEEL_PAUSE_MS;

	last_wheel_time_ = event.eventTime;
	// The first notch, one after a pause and one that turns back start again from slow.
	if (since_last >= WHEEL_PAUSE_MS || direction != wheel_direction_)
	{
		wheel_speed_ = 0.0;
		wheel_remainder_ = 0.0;
		stopWheelGlide();
	}
	else
	{
		double notches_per_s = notches * 1000.0 / (double)jmax((int64)1, since_last);
		wheel_speed_ += WHEEL_SPEED_SMOOTHING * (notches_per_s - wheel_speed_);
	}
	wheel_direction_ = direction;

	double speed = jlimit(0.0, 1.0, std::log(jmax(wheel_speed_, WHEEL_SLOW_NOTCHES_PER_S) / WHEEL_SLOW_NOTCHES_PER_S) /
		std::log(WHEEL_FAST_NOTCHES_PER_S / WHEEL_SLOW_NOTCHES_PER_S));
	double interval = getInterval();
	double move = wheel_remainder_ + notches * (interval + (wheel_max_notch_step_ - interval) * speed);
	double whole = interval * std::floor(move / interval + 1e-9);

	wheel_remainder_ = move - whole;
	if (whole > 0.0)
	{
		// Further notches during a glide extend its target; the first frame runs now, so a notch responds at once.
		double base = wheel_gliding_ ? wheel_target_ : getValue();

		wheel_target_ = jlimit(getMinimum(), getMaximum(), base + direction * whole);
		if (!wheel_gliding_)
		{
			wheel_gliding_ = true;
			wheel_glide_value_ = getValue();
			startTimerHz(WHEEL_GLIDE_HZ);
		}
		timerCallback();
	}
	return true;
}

void FxAudioSlider::timerCallback()
{
	// A drag, a reset or a preset load moved the value: that wins over the glide.
	if (getValue() != wheel_glide_value_)
	{
		stopWheelGlide();
		return;
	}

	double interval = getInterval();
	double left = wheel_target_ - getValue();
	if (std::abs(left) < interval * 0.5)
	{
		stopWheelGlide();
		return;
	}

	double move = jmin(std::abs(left), interval * std::ceil(std::abs(left) * WHEEL_GLIDE_FRACTION / interval - 1e-9));
	setValue(getValue() + ((left > 0.0) ? move : -move), NotificationType::sendNotificationSync);
	wheel_glide_value_ = getValue();
}

void FxAudioSlider::stopWheelGlide()
{
	stopTimer();
	wheel_gliding_ = false;
}

void FxAudioSlider::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
	bool handled = wheel_steps_by_interval_ &&
		((wheel_max_notch_step_ > getInterval()) ? stepByWheelAccelerated(event, wheel) : stepByWheel(*this, wheel));

	if (!handled)
	{
		Slider::mouseWheelMove(event, wheel);
	}
}

void FxAudioSlider::mouseDown(const juce::MouseEvent& event)
{
	stopWheelGlide();

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

void FxAudioSlider::setMaxValueText(const String& text)
{
	if (max_value_text_ != text)
	{
		max_value_text_ = text;
		updateLabel();
	}
}

void FxAudioSlider::updateLabel()
{
	auto value = getValue();

	// Update label text
	String text;
	if (zero_value_text_.isNotEmpty() && value == 0.0)
	{
		text = zero_value_text_;
	}
	else if (max_value_text_.isNotEmpty() && value >= getMaximum())
	{
		text = max_value_text_;
	}
	else
	{
		text = String::formatted(label_format_, value);
	}
	value_label_.setText(text, NotificationType::dontSendNotification);

	// Position label based on slider thumb position
	auto pos = getPositionOfValue(value);
	auto x = static_cast<int>(pos) + FxTheme::SLIDER_THUMB_RADIUS / 2 + 1;
	value_label_.setBounds(x, (getHeight() - LABEL_HEIGHT) / 2, LABEL_WIDTH, LABEL_HEIGHT);
}