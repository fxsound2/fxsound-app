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
#include "FxPowerButton.h"
#include "FxTheme.h"
#include "DfxDsp.h"

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
		void mouseDown(const juce::MouseEvent& event) override;

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

class FxFiltersControl : public Component
{
public:
	FxFiltersControl();
	~FxFiltersControl();

	void update();

	void setLookAndFeel(FxTheme& theme);

private:
	class PreviewButton : public Button, private Timer
	{
	public:
		PreviewButton() : Button("HearWhatsRemoved")
		{
			setMouseCursor(MouseCursor::PointingHandCursor);
			setClickingTogglesState(true);
		}
		~PreviewButton() = default;

		// Shows the playing state: the icon pulses (fades out and back in) while on, steady otherwise.
		void setPlaying(bool playing);

		void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	private:
		static constexpr int PULSE_PERIOD_MS = 1050;
		static constexpr int PULSE_FRAME_MS = 33;
		static constexpr float PULSE_DIM_ALPHA = 0.3f;

		void timerCallback() override;
		static float pulseAlpha();
	};

	// One filter: header row title ...... [power][delta], then its cutoff slider.
	struct FilterBlock
	{
		FilterBlock(const String& power_name, const String& label_format, float default_value, DfxDsp::FilterPreview preview_stage) :
			power(power_name), cutoff(label_format, default_value), preview(preview_stage)
		{
		}

		FxPowerButton power;
		Label title;
		FxAudioSlider cutoff;
		PreviewButton delta;
		const DfxDsp::FilterPreview preview;
	};

	static constexpr int X_MARGIN = 8;
	// The page flip button FxAudioControls puts at the top right (5 px from the top and right edges); the power buttons
	// and the slider rows use its margin.
	static constexpr int FLIP_BUTTON_MARGIN = 5;
	// The low and high cut slope dropdown, below the flip button at the EQ page's dropdown position and size.
	static constexpr int SLOPE_Y = 28;
	static constexpr int SLOPE_HEIGHT = 20;
	static constexpr int ROW_GAP = 8;
	static constexpr int TITLE_GAP = 10;
	// The slider rows match the EQ page's: X_MARGIN from the left, SLIDER_WIDTH wide.
	static constexpr int SLIDER_WIDTH = 160;
	static constexpr int HEADER_HEIGHT = 20;
	static constexpr int TOGGLE_GAP = 6;
	static constexpr int POWER_ICON_SIZE = 16;
	static constexpr int FILTER_POWER_SIZE = 16;
	static constexpr int FILTER_POWER_ICON_SIZE = 12;
	static constexpr float TITLE_FONT_HEIGHT = 12.0f;
	// Alpha of a filter block that is not part of the playing delta.
	static constexpr float DIMMED_ALPHA = 0.4f;
	static constexpr int SLIDER_HEIGHT = 18;
	static constexpr int LABEL_HEIGHT = 14;
	static constexpr int BUTTON_WIDTH = 18;
	static constexpr int BUTTON_HEIGHT = 18;
	static constexpr int BUTTONS_Y = 234;
	// Title row, gap, slider row.
	static constexpr int FILTER_BLOCK_HEIGHT = LABEL_HEIGHT + TITLE_GAP + SLIDER_HEIGHT;

	void resized() override;
	void paint(Graphics& g) override;
	void visibilityChanged() override;

	void setText();
	void updateLabels();
	void updateEnabled();
	void updatePreviewButtons();
	void refresh();
	void restoreDefaults();

	void initBlock(FilterBlock& block, std::function<void(bool)> set_on, std::function<void(double)> set_cutoff);
	int layoutBlock(FilterBlock& block, int y);
	// The two HEADER_HEIGHT-wide button columns at the right: the power buttons at the very right (in the flip button's
	// margin, shared by the filter rows and the bottom row), the global delta just left of them. The filter rows' deltas
	// sit at the left of their titles instead.
	int powerColumnX() const;
	int deltaColumnX() const;

	FilterBlock low_cut_;
	FilterBlock high_cut_;
	FilterBlock mono_bass_;
	FxPowerButton global_power_button_;
	ComboBox slope_;
	DrawableButton restore_defaults_button_;
	PreviewButton global_delta_button_;

	std::unique_ptr<Drawable> restore_defaults_image_;
	std::unique_ptr<Drawable> restore_defaults_hover_image_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxFiltersControl)
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
	FxFiltersControl filters_control_;
	DrawableButton flip_button_;

	std::unique_ptr<Drawable> flip_image_;
	std::unique_ptr<Drawable> flip_hover_image_;

	enum Page { EffectsPage = 0, EqualizerPage = 1, FiltersPage = 2, NumPages = 3 };

	void showPage(int page);

	int page_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxAudioControls)
};