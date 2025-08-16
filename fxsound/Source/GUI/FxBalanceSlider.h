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

class FxBalanceSlider : public Slider
{
public:
	FxBalanceSlider(float default_value);
	~FxBalanceSlider() = default;

private:
	static constexpr int X_MARGIN = 15;
	static constexpr int LABEL_HEIGHT = 14;
	static constexpr int LABEL_WIDTH = 40;

	void valueChanged() override;
	void resized() override;
	void paint(Graphics& g) override;
	bool keyPressed(const KeyPress& key) override;
	void mouseDown(const MouseEvent& event) override;

	void updateValueLabel();

	Label value_label_;
	std::unique_ptr<Drawable> slider_thumb_;
	std::unique_ptr<Drawable> slider_thumb_grey_;
	const float default_value_;
};


