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
#include "FxAudioSlider.h"
#include "FxBalanceSlider.h"
#include "FxTheme.h"

//==============================================================================
/*
*/
class FxEffects : public Component
{
public:
	enum EffectType {Fidelity=0, Ambience=1, Surround=2, DynamicBoost=3, Bass=4, NumEffects=5};

	FxEffects();
	~FxEffects() = default;

	void update();
	void showValues(bool show);

private:
	class FxEffectSlider : public Slider
	{
	public:
		FxEffectSlider(EffectType effect);
		~FxEffectSlider() = default;

		void setEffectValue(float value);
		void showValue(bool show);

        void enablementChanged() override;

	private:
		static constexpr int LABEL_HEIGHT = 12;

		void resized() override;
		void valueChanged() override;
		bool keyPressed(const KeyPress& key) override;

		Label value_label_;

		EffectType effect_;
	};

	static constexpr int LABEL_HEIGHT = 14;
	static constexpr int SLIDER_WIDTH = 160;
	static constexpr int SLIDER_HEIGHT = 18;
	static constexpr int X_MARGIN = 8;
	static constexpr int Y_MARGIN = 21;

	void resized() override;
	void paint(Graphics& g) override;

	std::vector<std::unique_ptr<Label>> labels_;
	std::vector<std::unique_ptr<FxEffectSlider>> effects_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxEffects)
};

class FxEqualizerControl : public Component
{
public:
	FxEqualizerControl();
	~FxEqualizerControl();

	void update();

    void setLookAndFeel(FxTheme& theme);

private:
	void resized() override;
	void paint(Graphics& g) override;

	static constexpr int X_MARGIN = 8;
	static constexpr int Y_MARGIN = 28;
	static constexpr int ROW_GAP = 8;
	static constexpr int LABEL_WIDTH = 52;
	static constexpr int CONTROL_GAP = 4;
	static constexpr int CONTROL_WIDTH = 100;
	static constexpr int COMBOBOX_HEIGHT = 20;
	static constexpr int SLIDER_WIDTH = 160;
	static constexpr int SLIDER_HEIGHT = 18;
	static constexpr int LABEL_HEIGHT = 14;
    static constexpr int BUTTON_WIDTH = 18;
	static constexpr int BUTTON_HEIGHT = 18;
	
	std::vector<int> equalizer_bands_ = { 5, 10, 15, 20, 31 };

	void setText();
	void updateEqualizerBandsText();
	void selectEqualizerBands();
	void restoreDefaults();

	void visibilityChanged() override;

	Label master_gain_title_;
	Label volume_leveling_title_;
	Label filter_q_title_;
	Label balance_title_;
	Label left_label_;
	Label right_label_;

	ComboBox equalizer_;
	FxAudioSlider master_gain_slider_;
	FxAudioSlider volume_leveling_slider_;
	FxAudioSlider filter_q_slider_;
	FxBalanceSlider balance_slider_;
	DrawableButton restore_defaults_button_;

	std::unique_ptr<Drawable> restore_defaults_image_;
	std::unique_ptr<Drawable> restore_defaults_hover_image_;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxEqualizerControl)
};

class FxAudioControls : public Component
{
public:
	FxAudioControls();
	~FxAudioControls() = default;

	void update();
	void showValues(bool show);

	void setLookAndFeel();

private:
	static constexpr int WIDTH = 168;
	static constexpr int HEIGHT = 257;
	static constexpr int BUTTON_WIDTH = 18;
	static constexpr int BUTTON_HEIGHT = 18;

	void resized() override;
	void paint(Graphics& g) override;

	FxEffects effects_;
	FxEqualizerControl equalizer_control_;
	DrawableButton flip_button_;

	std::unique_ptr<Drawable> flip_image_;
	std::unique_ptr<Drawable> flip_hover_image_;

	bool effects_shown_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxAudioControls)
};