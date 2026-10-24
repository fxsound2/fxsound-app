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

//==============================================================================
/*
*/

class FxAudioSlider : public Slider, private Timer
{
public:
	FxAudioSlider(String label_format, float default_value);
	~FxAudioSlider() = default;

	void setZeroValueText(const String& text);
	void setMaxValueText(const String& text);
	void setWheelStepsByInterval(bool steps_by_interval);
	// With wheel steps by interval: each notch moves from one interval (slow) up to notch_step (fast spin), growing with
	// the logarithm of the wheel speed. 0 (the default) moves one interval per event however fast the wheel turns.
	void setWheelMaxNotchStep(double notch_step);

	static bool stepByWheel(Slider& slider, const MouseWheelDetails& wheel);

private:
	static constexpr int LABEL_HEIGHT = 14;
	static constexpr int LABEL_WIDTH = 40;
	// The wheel delta JUCE reports on Windows for one notch (WHEEL_DELTA, 120): 0.5 * 120 / 256.
	static constexpr float WHEEL_NOTCH_DELTA = 60.0f / 256.0f;
	// Wheel speeds (notches per second) for the smallest and the largest notch step.
	static constexpr double WHEEL_SLOW_NOTCHES_PER_S = 5.0;
	static constexpr double WHEEL_FAST_NOTCHES_PER_S = 30.0;
	// The thumb glides to the wheel's target: each frame covers this fraction of the distance left (at least one
	// interval), at WHEEL_GLIDE_HZ frames per second.
	static constexpr double WHEEL_GLIDE_FRACTION = 0.4;
	static constexpr int WHEEL_GLIDE_HZ = 60;
	// A wheel event this long or more after the previous one starts again from slow.
	static constexpr int WHEEL_PAUSE_MS = 500;
	// How far each event moves the running wheel speed towards its own speed (0..1; lower is smoother but slower to
	// follow the wheel).
	static constexpr double WHEEL_SPEED_SMOOTHING = 0.45;

	void resized() override;
	void valueChanged() override;
	bool keyPressed(const KeyPress& key) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
	bool stepByWheelAccelerated(const MouseEvent& event, const MouseWheelDetails& wheel);
	void timerCallback() override;
	void stopWheelGlide();

	void updateLabel();

	Label value_label_;
	String label_format_;
	String zero_value_text_;
	String max_value_text_;
	bool wheel_steps_by_interval_ = false;
	double wheel_max_notch_step_ = 0.0;
	Time last_wheel_time_;
	double wheel_speed_ = 0.0;      // running average, in notches per second
	double wheel_remainder_ = 0.0;  // the fraction of a step not yet applied
	int wheel_direction_ = 0;
	bool wheel_gliding_ = false;
	double wheel_target_ = 0.0;      // where the glide is heading
	double wheel_glide_value_ = 0.0; // the value the glide last set; anything else means the value moved elsewhere
	const float default_value_;
};
