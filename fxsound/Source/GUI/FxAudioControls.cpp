/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
	www.theremino.com (2025)

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
#include "FxAudioControls.h"
#include "FxController.h"

FxAudioControls::FxAudioControls() : flip_button_("flipButton", DrawableButton::ButtonStyle::ImageFitted)
{
	page_ = EffectsPage;

	setLookAndFeel();

	addAndMakeVisible(effects_);
	addChildComponent(equalizer_control_);
	addChildComponent(filters_control_);

	effects_.addMouseListener(this, true);
	equalizer_control_.addMouseListener(this, true);
	filters_control_.addMouseListener(this, true);

	flip_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	flip_button_.setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	flip_button_.setWantsKeyboardFocus(true);
	flip_button_.onClick = [this]() {
		showPage((page_ + 1) % NumPages);
		};

	addAndMakeVisible(flip_button_);

	setSize(WIDTH, HEIGHT);
}

void FxAudioControls::showPage(int page)
{
	page_ = page;

	effects_.setVisible(page_ == EffectsPage);
	equalizer_control_.setVisible(page_ == EqualizerPage);
	filters_control_.setVisible(page_ == FiltersPage);
}

void FxAudioControls::update()
{
	effects_.update();
	effects_.showValues(true);

	equalizer_control_.update();
	filters_control_.update();
}

void FxAudioControls::showValues(bool show)
{
	effects_.showValues(show);
}

void FxAudioControls::setLookAndFeel()
{
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

	flip_image_ = Drawable::createFromImageData(FXIMAGE(FlipButton), FXIMAGESIZE(FlipButton));
	flip_hover_image_ = Drawable::createFromImageData(FXIMAGE(FlipButtonHover), FXIMAGESIZE(FlipButtonHover));

	flip_button_.setImages(flip_image_.get(), flip_hover_image_.get());

	equalizer_control_.setLookAndFeel(theme);
	filters_control_.setLookAndFeel(theme);
}

void FxAudioControls::resized()
{
	effects_.setBounds(0, 0, WIDTH, HEIGHT);
	equalizer_control_.setBounds(0, 0, WIDTH, HEIGHT);
	filters_control_.setBounds(0, 0, WIDTH, HEIGHT);

	flip_button_.setBounds(getWidth() - BUTTON_WIDTH - 5, 5, BUTTON_WIDTH, BUTTON_HEIGHT);
}

void FxAudioControls::paint(Graphics& g)
{
	g.setFillType(FillType(Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f)));
	g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
}

FxEffects::FxEffects()
{
	StringArray texts = { TRANS("Clarity"), TRANS("Ambience"), TRANS("Surround Sound"), TRANS("Dynamic Boost"), TRANS("Bass Boost") };
    
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

	labels_.resize(EffectType::NumEffects);
	effects_.resize(EffectType::NumEffects);

	for (int i = EffectType::Fidelity; i < EffectType::NumEffects; i++)
	{
		labels_[i].reset(new Label(texts[i], texts[i]));
		labels_[i]->setFont(theme.getNormalFont().withHeight(12.0f));
		labels_[i]->setJustificationType(Justification::topLeft);
		auto border = labels_[i]->getBorderSize();
		border.setLeft(0);
		labels_[i]->setBorderSize(border);
		addAndMakeVisible(labels_[i].get());

		effects_[i].reset(new FxEffectSlider(static_cast<EffectType>(i)));
		effects_[i]->setSliderStyle(Slider::LinearHorizontal);
		effects_[i]->setRange(0, 10, 1.0);
		effects_[i]->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
		addAndMakeVisible(effects_[i].get());
	}
}

void FxEffects::update()
{
	auto& controller = FxController::getInstance();

	for (int i = EffectType::Fidelity; i < EffectType::NumEffects; i++)
	{
		auto value = controller.getEffectValue(static_cast<EffectType>(i));
		if (value >= 0.0 && value <= 1.0)
		{
			effects_[i]->setEffectValue(value*10.0f);
		}
	}
}

void FxEffects::showValues(bool show)
{
	for (int i = EffectType::Fidelity; i < EffectType::NumEffects; i++)
	{
		effects_[i]->showValue(show);
	}
}

void FxEffects::resized()
{
	auto bounds = getLocalBounds();

	int y = Y_MARGIN;
	for (int i = EffectType::Fidelity; i < EffectType::NumEffects; i++)
	{
		labels_[i]->setBounds(X_MARGIN+FxTheme::SLIDER_THUMB_RADIUS, y, SLIDER_WIDTH, LABEL_HEIGHT);
		effects_[i]->setBounds(X_MARGIN, labels_[i]->getBottom() + 1, SLIDER_WIDTH, SLIDER_HEIGHT);
		y = effects_[i]->getBottom() + 10;
	}
}

void FxEffects::paint([[maybe_unused]] Graphics& g)
{
    StringArray texts = { TRANS("Clarity"), TRANS("Ambience"), TRANS("Surround Sound"), TRANS("Dynamic Boost"), TRANS("Bass Boost") };
    StringArray tool_tips = { TRANS("Enhances and elevates high end\r\nfidelity and presence"),
                              TRANS("Thickens and smooths audio\r\nwith controlled reverberation"),
                              TRANS("Widens the left-right balance\r\nfor expansive, wide sound"),
                              TRANS("Increases overall volume and balance\r\nwith responsive processing"),
                              TRANS("Boosts low end for full,\r\nimpactful response") };

    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

    for (int i = EffectType::Fidelity; i < EffectType::NumEffects; i++)
    {
        labels_[i]->setFont(theme.getNormalFont().withHeight(12.0f));
        labels_[i]->setText(texts[i], NotificationType::dontSendNotification);
        if (!FxController::getInstance().isHelpTooltipsHidden())
        {
            effects_[i]->setTooltip(tool_tips[i]);
        }
        else
        {
            effects_[i]->setTooltip("");
        }
    }
}

FxEffects::FxEffectSlider::FxEffectSlider(EffectType effect)
{
	effect_ = effect;

	setMouseCursor(MouseCursor::PointingHandCursor);

	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

	value_label_.setFont(theme.getSmallFont().withHeight(11.0f));
	value_label_.setJustificationType(Justification::centredLeft);
	value_label_.setInterceptsMouseClicks(false, false);
	addChildComponent(value_label_);

	setWantsKeyboardFocus(true);
}

void FxEffects::FxEffectSlider::setEffectValue(float value)
{
	setValue(value, NotificationType::dontSendNotification);

	auto text = String::formatted("%.0f", value);
	value_label_.setText(text, NotificationType::dontSendNotification);

	auto pos = getPositionOfValue(value);
	auto x = static_cast<int>(pos) + FxTheme::SLIDER_THUMB_RADIUS + 1;
	value_label_.setBounds(value_label_.getBounds().withX(x));
}

void FxEffects::FxEffectSlider::showValue(bool show)
{
	value_label_.setVisible(show && isEnabled());
}

void FxEffects::FxEffectSlider::enablementChanged()
{
    if (isEnabled())
    {
        setMouseCursor(MouseCursor::PointingHandCursor);
    }
    else
    {
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void FxEffects::FxEffectSlider::resized()
{
	Slider::resized();

	value_label_.setBounds(value_label_.getX(), (getHeight() - LABEL_HEIGHT) / 2, FxTheme::SLIDER_THUMB_RADIUS * 3, LABEL_HEIGHT);
}

void FxEffects::FxEffectSlider::valueChanged()
{
	auto value = static_cast<float>(getValue());

	if (value != FxController::getInstance().getEffectValue(effect_)*10.0)
	{
		FxController::getInstance().setEffectValue(effect_, value);
		
		auto text = String::formatted("%.0f", value);
		value_label_.setText(text, NotificationType::dontSendNotification);

		auto pos = getPositionOfValue(value);
		auto x = static_cast<int>(pos) + FxTheme::SLIDER_THUMB_RADIUS + 1;
		value_label_.setBounds(value_label_.getBounds().withX(x));
	}
}

bool FxEffects::FxEffectSlider::keyPressed(const KeyPress& key)
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

void FxEffects::FxEffectSlider::mouseDown(const juce::MouseEvent& event)
{
	// Reset on right-click or double-click, matching the other EQ/audio sliders.
	// See FxAudioSlider::mouseDown for why the reset is handled here rather than
	// in mouseDoubleClick.
	if (event.mods.isRightButtonDown() || event.getNumberOfClicks() >= 2)
	{
		setValue(0.0, NotificationType::sendNotification);
	}
	else
	{
		Slider::mouseDown(event);
	}
}

FxEqualizerControl::FxEqualizerControl() :
	master_gain_slider_("%0.0f dB", 0.0f),
	volume_leveling_slider_("%.1f dB", 0.0f),
	filter_q_slider_("%.1fx", 1.0f),
	balance_slider_(0.0f),
	restore_defaults_button_("restoreDefaultsButton", DrawableButton::ButtonStyle::ImageFitted)
{
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
	auto& controller = FxController::getInstance();

	setLookAndFeel(theme);

	equalizer_.setMouseCursor(MouseCursor::PointingHandCursor);
	equalizer_.setWantsKeyboardFocus(true);
	equalizer_.onChange = [this]() {
		auto id = equalizer_.getSelectedId();

		auto num_eq_bands = FxController::DEFAULT_NUM_EQ_BANDS;
		for (auto bands : equalizer_bands_)
		{
			if (bands == id)
			{
				num_eq_bands = bands;
				break;
			}
		}

		FxController::getInstance().setNumEqBands(num_eq_bands);
	};

	equalizer_.clear(NotificationType::dontSendNotification);
	for (auto bands : equalizer_bands_)
	{
		equalizer_.addItem(String(bands) + TRANS(" Bands"), bands);
	}
	selectEqualizerBands();

	master_gain_title_.setFont(theme.getNormalFont().withHeight(12.0f));
	master_gain_title_.setJustificationType(Justification::topLeft);
	auto border = master_gain_title_.getBorderSize();
	border.setLeft(0);
	master_gain_title_.setBorderSize(border);

	master_gain_slider_.setSliderStyle(Slider::LinearHorizontal);
	master_gain_slider_.setRange(-20, 20, 2);
	master_gain_slider_.setValue(controller.getMasterGain());
	master_gain_slider_.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	master_gain_slider_.onValueChange = [this]() {
		auto value = master_gain_slider_.getValue();
		auto& controller = FxController::getInstance();

		if (controller.getMasterGain() != value)
			controller.setMasterGain((float)value);
		};

	volume_leveling_title_.setFont(theme.getNormalFont().withHeight(12.0f));
	volume_leveling_title_.setJustificationType(Justification::topLeft);
	border = volume_leveling_title_.getBorderSize();
	border.setLeft(0);
	volume_leveling_title_.setBorderSize(border);

	volume_leveling_slider_.setSliderStyle(Slider::LinearHorizontal);
	volume_leveling_slider_.setRange(0, 4, 0.5);
	volume_leveling_slider_.setValue(controller.getVolumeLeveling());
	volume_leveling_slider_.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	volume_leveling_slider_.onValueChange = [this]() {
		auto value = volume_leveling_slider_.getValue();
		auto& controller = FxController::getInstance();

		if (controller.getVolumeLeveling() != value)
			controller.setVolumeLeveling((float)value);
		};

	filter_q_title_.setFont(theme.getNormalFont().withHeight(12.0f));
	filter_q_title_.setJustificationType(Justification::topLeft);
	border = filter_q_title_.getBorderSize();
	border.setLeft(0);
	filter_q_title_.setBorderSize(border);

	filter_q_slider_.setSliderStyle(Slider::LinearHorizontal);
	filter_q_slider_.setRange(1, 3, 0.5);
	filter_q_slider_.setValue(controller.getFilterQ());
	filter_q_slider_.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	filter_q_slider_.onValueChange = [this]() {
		auto value = filter_q_slider_.getValue();
		auto& controller = FxController::getInstance();

		if (controller.getFilterQ() != value)
			controller.setFilterQ((float)value);
		};

	balance_title_.setFont(theme.getNormalFont().withHeight(12.0f));
	balance_title_.setJustificationType(Justification::topLeft);
	border = balance_title_.getBorderSize();
	border.setLeft(0);
	balance_title_.setBorderSize(border);

	left_label_.setFont(theme.getSmallFont().withHeight(11.0f));
	left_label_.setJustificationType(Justification::centredLeft);
	border = left_label_.getBorderSize();
	border.setLeft(0);
	border.setTop(0);
	left_label_.setBorderSize(border);

	right_label_.setFont(theme.getSmallFont().withHeight(11.0f));
	right_label_.setJustificationType(Justification::centredRight);
    border = right_label_.getBorderSize();
    border.setRight(0);
    border.setTop(0);
    right_label_.setBorderSize(border);

	restore_defaults_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	restore_defaults_button_.setWantsKeyboardFocus(true);
	restore_defaults_button_.onClick = [this]() {
		restoreDefaults();
		};

	setText();

	addAndMakeVisible(equalizer_);
	addAndMakeVisible(master_gain_title_);
	addAndMakeVisible(master_gain_slider_);
	addAndMakeVisible(volume_leveling_title_);
	addAndMakeVisible(volume_leveling_slider_);
	addAndMakeVisible(filter_q_title_);
	addAndMakeVisible(filter_q_slider_);
	addAndMakeVisible(balance_title_);
	addAndMakeVisible(balance_slider_);
	addAndMakeVisible(left_label_);
	addAndMakeVisible(right_label_);
	addAndMakeVisible(restore_defaults_button_);
}

FxEqualizerControl::~FxEqualizerControl()
{
	equalizer_.onChange = nullptr;
	master_gain_slider_.onValueChange = nullptr;
	volume_leveling_slider_.onValueChange = nullptr;
	filter_q_slider_.onValueChange = nullptr;
	restore_defaults_button_.onClick = nullptr;
}

void FxEqualizerControl::update()
{
	auto& controller = FxController::getInstance();

	selectEqualizerBands();

	master_gain_slider_.setValue(controller.getMasterGain(), NotificationType::dontSendNotification);
	volume_leveling_slider_.setValue(controller.getVolumeLeveling(), NotificationType::dontSendNotification);
	filter_q_slider_.setValue(controller.getFilterQ(), NotificationType::dontSendNotification);
	balance_slider_.setValue(controller.getBalance(), NotificationType::dontSendNotification);
}

void FxEqualizerControl::setLookAndFeel([[maybe_unused]]FxTheme& theme)
{
	restore_defaults_image_ = Drawable::createFromImageData(FXIMAGE(RestoreDefaultsButton), FXIMAGESIZE(RestoreDefaultsButton));
	restore_defaults_hover_image_ = Drawable::createFromImageData(FXIMAGE(RestoreDefaultsButtonHover), FXIMAGESIZE(RestoreDefaultsButtonHover));

	restore_defaults_button_.setImages(restore_defaults_image_.get(), restore_defaults_hover_image_.get());
}

void FxEqualizerControl::resized()
{
	auto width = getWidth() - (X_MARGIN * 2);

	int y = Y_MARGIN;

	equalizer_.setBounds(X_MARGIN, y, width, COMBOBOX_HEIGHT);

	y = equalizer_.getBottom() + ROW_GAP;
	master_gain_title_.setBounds(X_MARGIN + FxTheme::SLIDER_THUMB_RADIUS, y, SLIDER_WIDTH, LABEL_HEIGHT);
	master_gain_slider_.setBounds(X_MARGIN, master_gain_title_.getBottom() + 1, SLIDER_WIDTH, SLIDER_HEIGHT);

	y = master_gain_slider_.getBottom() + ROW_GAP;
	volume_leveling_title_.setBounds(X_MARGIN + FxTheme::SLIDER_THUMB_RADIUS, y, SLIDER_WIDTH, LABEL_HEIGHT);
	volume_leveling_slider_.setBounds(X_MARGIN, volume_leveling_title_.getBottom() + 1, SLIDER_WIDTH, SLIDER_HEIGHT);

	y = volume_leveling_slider_.getBottom() + ROW_GAP;
	filter_q_title_.setBounds(X_MARGIN + FxTheme::SLIDER_THUMB_RADIUS, y, SLIDER_WIDTH, LABEL_HEIGHT);
	filter_q_slider_.setBounds(X_MARGIN, filter_q_title_.getBottom() + 1, SLIDER_WIDTH, SLIDER_HEIGHT);

	y = filter_q_slider_.getBottom() + ROW_GAP;
	balance_title_.setBounds(X_MARGIN + FxTheme::SLIDER_THUMB_RADIUS, y, SLIDER_WIDTH, LABEL_HEIGHT);
	balance_slider_.setBounds(X_MARGIN, balance_title_.getBottom() + 1, SLIDER_WIDTH, SLIDER_HEIGHT);

	y = balance_slider_.getBottom();
	left_label_.setBounds(X_MARGIN, y, SLIDER_WIDTH / 2, LABEL_HEIGHT);
	right_label_.setBounds(left_label_.getRight(), y, SLIDER_WIDTH / 2 - (FxTheme::SLIDER_THUMB_RADIUS * 4), LABEL_HEIGHT);

	y = left_label_.getBottom() + ROW_GAP;
	restore_defaults_button_.setBounds(X_MARGIN, y, BUTTON_WIDTH, BUTTON_HEIGHT);
}

void FxEqualizerControl::paint([[maybe_unused]]Graphics& g)
{
	setText();
	updateEqualizerBandsText();
}

void FxEqualizerControl::setText()
{
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
	auto font = theme.getNormalFont().withHeight(12.0f);

	master_gain_title_.setFont(font);
	master_gain_title_.setText(TRANS("Master Gain"), NotificationType::dontSendNotification);

	volume_leveling_title_.setFont(font);
	volume_leveling_title_.setText(TRANS("Volume Leveling"), NotificationType::dontSendNotification);

	filter_q_title_.setFont(font);
	filter_q_title_.setText(TRANS("Filter Q"), NotificationType::dontSendNotification);

	balance_title_.setFont(font);
	balance_title_.setText(TRANS("Balance"), NotificationType::dontSendNotification);

	left_label_.setFont(theme.getSmallFont().withHeight(11.0f));
	left_label_.setText(TRANS("Left"), NotificationType::dontSendNotification);

	right_label_.setFont(theme.getSmallFont().withHeight(11.0f));
	right_label_.setText(TRANS("Right"), NotificationType::dontSendNotification);

	restore_defaults_button_.setTooltip(TRANS("Restore Defaults"));
}

void FxEqualizerControl::updateEqualizerBandsText()
{
	auto id = equalizer_.getSelectedId();

	for (auto bands : equalizer_bands_)
	{
		equalizer_.changeItemText(bands, String(bands) + TRANS(" Bands"));
	}

	if (equalizer_.getSelectedId() == 0 && id != 0)
	{
		equalizer_.setSelectedId(id, juce::dontSendNotification);
	}
}

void FxEqualizerControl::selectEqualizerBands()
{
	auto& controller = FxController::getInstance();
	auto num_eq_bands = controller.getNumEqBands();

	switch (num_eq_bands)
	{
	case 5:
	case 10:
	case 15:
	case 20:
	case 31:
		equalizer_.setSelectedId(num_eq_bands, NotificationType::dontSendNotification);
		break;

	default:
		equalizer_.setSelectedId(FxController::DEFAULT_NUM_EQ_BANDS, NotificationType::dontSendNotification);
	}
}

void FxEqualizerControl::restoreDefaults()
{
	auto& controller = FxController::getInstance();

	controller.setNumEqBands(FxController::DEFAULT_NUM_EQ_BANDS);
	controller.setVolumeLeveling(FxController::DEFAULT_VOLUME_LEVELING);
	controller.setBalance(FxController::DEFAULT_BALANCE);
	controller.setFilterQ(FxController::DEFAULT_FILTER_Q);
	controller.setMasterGain(FxController::DEFAULT_MASTER_GAIN);

	equalizer_.setSelectedId(controller.getNumEqBands(), NotificationType::dontSendNotification);
	master_gain_slider_.setValue(controller.getMasterGain());
	volume_leveling_slider_.setValue(controller.getVolumeLeveling());
	filter_q_slider_.setValue(controller.getFilterQ());
	balance_slider_.setValue(controller.getBalance());
}

void FxEqualizerControl::visibilityChanged()
{
	if (isVisible())
	{
		update();
	}
}

namespace
{
	// The mono bass stage is a Linkwitz-Riley LR4 on the side signal: two identical 12dB/Oct Butterworth
	// sections at the cut Q (FiltChain's filtChainDesignLinkwitzRiley).
	constexpr int MONO_BASS_SLOPE_DB = 24;

	// "48dB/Oct (maximally flat)": the low and high cut's selected slope. They are true Butterworth cascades, each
	// section with its own Q, so no single Q is named.
	String cutSlope()
	{
		return String(FxController::getInstance().getCutSlope()) + "dB/Oct (" + TRANS("maximally flat") + ")";
	}

	String lowCutToggleTooltip(int hp_cutoff_hz)
	{
		String type = TRANS("Butterworth high-pass") + ", " + cutSlope();

		if (hp_cutoff_hz <= 0)
		{
			return type + ". " + TRANS("The cutoff is Off (0Hz), so audio passes through unfiltered.");
		}

		return type + ": " + TRANS("removes content below") + " " + String(hp_cutoff_hz) + "Hz. " + TRANS("Adds no delay.");
	}

	String highCutToggleTooltip(int lp_cutoff_hz)
	{
		String type = TRANS("Butterworth low-pass") + ", " + cutSlope();

		if (lp_cutoff_hz >= DfxDsp::HIGH_CUT_OFF_HZ)
		{
			return type + ". " + TRANS("The cutoff is Off, so audio passes through unfiltered.");
		}

		return type + ": " + TRANS("removes content above") + " " + String(lp_cutoff_hz / 1000.0, 1) + "kHz. " + TRANS("Adds no delay.");
	}

	String monoBassToggleTooltip(int mono_cutoff_hz)
	{
		String type = TRANS("Mono bass, Linkwitz-Riley (LR4) high-pass on the side signal") + ", " +
			String(MONO_BASS_SLOPE_DB) + "dB/Oct, " + TRANS("two standard Q") + " (" + String(DfxDsp::CUT_Q, 2) + ") " +
			TRANS("Butterworth stages");

		if (mono_cutoff_hz <= 0)
		{
			return type + ". " + TRANS("The cutoff is Off (0Hz), so stereo passes through untouched.");
		}

		return type + ": " + TRANS("folds bass below") + " " + String(mono_cutoff_hz) + "Hz " + TRANS("to mono; everything above stays stereo.");
	}

	// The high cut slider works in kHz; the controller works in Hz.
	constexpr double HZ_PER_KHZ = 1000.0;

	// The most one mouse wheel notch (on a fast spin) moves the low cut and mono bass sliders, in slider steps
	// (1 Hz). A slow notch moves one step. The high cut is scaled to move its thumb as fast (see the constructor).
	constexpr double WHEEL_MAX_NOTCH_STEPS = 6.0;

	// Number of steps across a slider's range.
	double sliderSteps(const Slider& slider)
	{
		return (slider.getMaximum() - slider.getMinimum()) / slider.getInterval();
	}
}

FxFiltersControl::FxFiltersControl() :
	low_cut_(L"lowCutPowerButton", "%0.0f Hz", (float)DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ, DfxDsp::PreviewLowCut),
	high_cut_(L"highCutPowerButton", "%0.1f kHz", (float)(DfxDsp::HIGH_CUT_DEFAULT_CUTOFF_HZ / HZ_PER_KHZ), DfxDsp::PreviewHighCut),
	mono_bass_(L"monoBassPowerButton", "%0.0f Hz", (float)DfxDsp::MONO_BASS_DEFAULT_CUTOFF_HZ, DfxDsp::PreviewMonoBass),
	global_power_button_(L"filtersPowerButton"),
	restore_defaults_button_("restoreDefaultsButton", DrawableButton::ButtonStyle::ImageFitted)
{
	low_cut_.cutoff.setRange(DfxDsp::LOW_CUT_MIN_CUTOFF_HZ, DfxDsp::LOW_CUT_MAX_CUTOFF_HZ, 1);
	// One step when dragging or for a slow wheel notch; up to WHEEL_MAX_NOTCH_STEPS per notch as the wheel spins faster.
	low_cut_.cutoff.setWheelMaxNotchStep(WHEEL_MAX_NOTCH_STEPS * low_cut_.cutoff.getInterval());
	initBlock(low_cut_,
		[](bool on) { FxController::getInstance().setLowCutOn(on); },
		[](double value) {
			auto& controller = FxController::getInstance();
			int cutoff_hz = roundToInt(value);
			if (controller.getLowCutCutoff() != cutoff_hz)
			{
				controller.setLowCutCutoff(cutoff_hz);
			}
		});

	high_cut_.cutoff.setRange(DfxDsp::HIGH_CUT_MIN_CUTOFF_HZ / HZ_PER_KHZ, DfxDsp::HIGH_CUT_OFF_HZ / HZ_PER_KHZ, DfxDsp::HIGH_CUT_STEP_HZ / HZ_PER_KHZ);
	// The high cut has far fewer steps (61) than the low cut (200), so the same steps per notch would move its thumb
	// about three times as far. Its fast-spin step covers about the same share of the slider as the low cut's (2 steps,
	// 200 Hz); a slow notch still moves one 100 Hz step, the smallest the high cut takes.
	high_cut_.cutoff.setWheelMaxNotchStep(high_cut_.cutoff.getInterval() *
		jmax(1.0, std::round(WHEEL_MAX_NOTCH_STEPS * sliderSteps(high_cut_.cutoff) / sliderSteps(low_cut_.cutoff))));
	initBlock(high_cut_,
		[](bool on) { FxController::getInstance().setHighCutOn(on); },
		[](double value) {
			auto& controller = FxController::getInstance();
			int cutoff_hz = roundToInt(value * HZ_PER_KHZ);
			if (controller.getHighCutCutoff() != cutoff_hz)
			{
				controller.setHighCutCutoff(cutoff_hz);
			}
		});

	mono_bass_.cutoff.setRange(DfxDsp::MONO_BASS_MIN_CUTOFF_HZ, DfxDsp::MONO_BASS_MAX_CUTOFF_HZ, 1);
	mono_bass_.cutoff.setWheelMaxNotchStep(WHEEL_MAX_NOTCH_STEPS * mono_bass_.cutoff.getInterval());
	initBlock(mono_bass_,
		[](bool on) { FxController::getInstance().setMonoBassOn(on); },
		[](double value) {
			auto& controller = FxController::getInstance();
			int cutoff_hz = roundToInt(value);
			if (controller.getMonoBassCutoff() != cutoff_hz)
			{
				controller.setMonoBassCutoff(cutoff_hz);
			}
		});

	global_power_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	global_power_button_.setImageWidth(POWER_ICON_SIZE);
	global_power_button_.setWantsKeyboardFocus(true);
	global_power_button_.onClick = [this]() {
		bool on = !global_power_button_.getPowerState();

		global_power_button_.setPowerState(on);
		FxController::getInstance().setFiltersOn(on);
		refresh();
		};

	// One slope for the low and high cut (the mono bass stage is a fixed LR4).
	slope_.setMouseCursor(MouseCursor::PointingHandCursor);
	slope_.setWantsKeyboardFocus(true);
	for (auto slope_db : DfxDsp::CUT_SLOPES_DB)
	{
		slope_.addItem(String(slope_db) + " dB/Oct", slope_db);
	}
	slope_.onChange = [this]() {
		auto slope_db = slope_.getSelectedId();
		if (slope_db != 0 && slope_db != FxController::getInstance().getCutSlope())
		{
			FxController::getInstance().setCutSlope(slope_db);
			refresh();
		}
		};

	restore_defaults_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	restore_defaults_button_.setWantsKeyboardFocus(true);
	restore_defaults_button_.onClick = [this]() {
		restoreDefaults();
		};

	global_delta_button_.setWantsKeyboardFocus(true);
	global_delta_button_.onClick = [this]() {
		auto& controller = FxController::getInstance();

		controller.setFilterPreview(DfxDsp::toggledFilterPreview(controller.getFilterPreview(), DfxDsp::PreviewAll));
		update();
		};

	setText();
	update();

	for (auto* block : { &low_cut_, &high_cut_, &mono_bass_ })
	{
		addAndMakeVisible(block->power);
		addAndMakeVisible(block->title);
		addAndMakeVisible(block->cutoff);
		addAndMakeVisible(block->delta);
	}
	addAndMakeVisible(global_power_button_);
	addAndMakeVisible(slope_);
	addAndMakeVisible(restore_defaults_button_);
	addAndMakeVisible(global_delta_button_);
}

FxFiltersControl::~FxFiltersControl()
{
	for (auto* block : { &low_cut_, &high_cut_, &mono_bass_ })
	{
		block->power.onClick = nullptr;
		block->cutoff.onValueChange = nullptr;
		block->delta.onClick = nullptr;
	}
	global_power_button_.onClick = nullptr;
	slope_.onChange = nullptr;
	restore_defaults_button_.onClick = nullptr;
	global_delta_button_.onClick = nullptr;
}

void FxFiltersControl::initBlock(FilterBlock& block, std::function<void(bool)> set_on, std::function<void(double)> set_cutoff)
{
	block.power.setMouseCursor(MouseCursor::PointingHandCursor);
	block.power.setImageWidth(FILTER_POWER_ICON_SIZE);
	block.power.setWantsKeyboardFocus(true);
	block.power.onClick = [this, &block, set_on]() {
		bool on = !block.power.getPowerState();

		block.power.setPowerState(on);
		set_on(on);
		refresh();
		};

	// Same title style as the Effects and EQ pages (normal font at 12, default label colour).
	block.title.setJustificationType(Justification::topLeft);
	block.title.setInterceptsMouseClicks(false, false);
	auto border = block.title.getBorderSize();
	border.setLeft(0);
	block.title.setBorderSize(border);

	block.cutoff.setSliderStyle(Slider::LinearHorizontal);
	block.cutoff.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	block.cutoff.setMouseCursor(MouseCursor::PointingHandCursor);
	block.cutoff.setWantsKeyboardFocus(true);
	block.cutoff.setWheelStepsByInterval(true);
	// The value moves with the thumb, just right of it and vertically centred on the slider (FxAudioSlider's default;
	// the track leaves room for it at its right end).
	block.cutoff.onValueChange = [this, &block, set_cutoff]() {
		set_cutoff(block.cutoff.getValue());
		refresh();
		};

	block.delta.setWantsKeyboardFocus(true);
	// Individual deltas combine (their removed signals are summed); pressing one while the global delta is active
	// starts again from just this one.
	block.delta.onClick = [this, &block]() {
		auto& controller = FxController::getInstance();

		controller.setFilterPreview(DfxDsp::toggledFilterPreview(controller.getFilterPreview(), block.preview));
		update();
		};
}

void FxFiltersControl::update()
{
	auto& controller = FxController::getInstance();

	global_power_button_.setPowerState(controller.isFiltersOn());
	low_cut_.power.setPowerState(controller.isLowCutOn());
	high_cut_.power.setPowerState(controller.isHighCutOn());
	mono_bass_.power.setPowerState(controller.isMonoBassOn());
	slope_.setSelectedId(controller.getCutSlope(), NotificationType::dontSendNotification);

	// Cutoff sliders notify synchronously so FxAudioSlider refreshes its value label; the setters are guarded.
	low_cut_.cutoff.setValue(static_cast<double>(controller.getLowCutCutoff()), NotificationType::sendNotificationSync);
	high_cut_.cutoff.setValue(controller.getHighCutCutoff() / HZ_PER_KHZ, NotificationType::sendNotificationSync);
	mono_bass_.cutoff.setValue(static_cast<double>(controller.getMonoBassCutoff()), NotificationType::sendNotificationSync);

	refresh();
}

void FxFiltersControl::refresh()
{
	updatePreviewButtons();
	updateLabels();
	updateEnabled();
}

void FxFiltersControl::setLookAndFeel(FxTheme&)
{
	restore_defaults_image_ = Drawable::createFromImageData(FXIMAGE(RestoreDefaultsButton), FXIMAGESIZE(RestoreDefaultsButton));
	restore_defaults_hover_image_ = Drawable::createFromImageData(FXIMAGE(RestoreDefaultsButtonHover), FXIMAGESIZE(RestoreDefaultsButtonHover));

	restore_defaults_button_.setImages(restore_defaults_image_.get(), restore_defaults_hover_image_.get());
}

void FxFiltersControl::updatePreviewButtons()
{
	// The controller/DSP owns the preview state (it can drop a preview that is no longer
	// available, e.g. when a slider reaches Off), so the delta buttons always mirror it.
	int preview = FxController::getInstance().getFilterPreview();

	bool global_preview = (preview & DfxDsp::PreviewAll) != 0;

	for (auto* block : { &low_cut_, &high_cut_, &mono_bass_ })
	{
		block->delta.setPlaying((preview & block->preview) != 0);

		// With the global switch off every filter is dimmed (none is heard and no delta can play). While a delta
		// plays, the filters being heard in it are highlighted and the others dimmed; the global delta plays the
		// filters whose own switch is on. Otherwise a filter switched off on its own toggle has its title and slider
		// dimmed; its switch and delta stay bright, since it can still be switched on or auditioned.
		bool in_preview = global_preview ? block->power.getPowerState() : (preview & block->preview) != 0;
		bool dimmed = !global_power_button_.getPowerState() || (preview != DfxDsp::PreviewNone && !in_preview);
		bool switched_off = !dimmed && preview == DfxDsp::PreviewNone && !block->power.getPowerState();

		if (preview != DfxDsp::PreviewNone && in_preview)
		{
			block->title.setColour(Label::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
		}
		else
		{
			block->title.removeColour(Label::textColourId);
		}
		for (auto* component : std::initializer_list<Component*>{ &block->title, &block->power, &block->cutoff, &block->delta })
		{
			bool faded = dimmed || (switched_off && (component == &block->title || component == &block->cutoff));

			component->setAlpha(faded ? DIMMED_ALPHA : 1.0f);
		}
	}
	// The slope dropdown dims with the filters when the global switch is off (still usable, like the sliders).
	slope_.setAlpha(global_power_button_.getPowerState() ? 1.0f : DIMMED_ALPHA);
	global_delta_button_.setPlaying(global_preview);
}

void FxFiltersControl::updateLabels()
{
	auto& controller = FxController::getInstance();
	bool show_help = !controller.isHelpTooltipsHidden();

	low_cut_.cutoff.setTooltip(show_help ? TRANS("Frequencies below this are removed. Set to Off (0Hz) to pass audio through unfiltered.") + "\n" +
		TRANS("-3dB at the cutoff itself.") : String());
	low_cut_.power.setTooltip(show_help ? lowCutToggleTooltip(controller.getLowCutCutoff()) : String());

	high_cut_.cutoff.setTooltip(show_help ? TRANS("Frequencies above this are removed. Set to Off to pass audio through unfiltered.") : String());
	high_cut_.power.setTooltip(show_help ? highCutToggleTooltip(controller.getHighCutCutoff()) : String());

	mono_bass_.cutoff.setTooltip(show_help ? TRANS("Bass below this is folded to mono; everything above stays stereo. Set to Off (0Hz) to leave stereo untouched.") : String());
	mono_bass_.power.setTooltip(show_help ? monoBassToggleTooltip(controller.getMonoBassCutoff()) : String());

	global_power_button_.setTooltip(show_help ? TRANS("Turns all filters on or off together. Each filter keeps its own on/off setting.") : String());
	slope_.setTooltip(show_help ? TRANS("Slope of the low cut and high cut: how fast they roll off past the cutoff. Steeper removes more just past the cutoff. Mono Bass always uses 24dB/Oct.") : String());
}

void FxFiltersControl::updateEnabled()
{
	// Switches never grey anything out: a filter can be set up and auditioned before it is switched on. Only the
	// deltas depend on availability (an individual delta needs its slider not at "Off"; the global delta needs such a
	// filter with its own switch on).
	auto& controller = FxController::getInstance();

	for (auto* block : { &low_cut_, &high_cut_, &mono_bass_ })
	{
		block->delta.setEnabled(controller.isFilterPreviewAvailable(block->preview));
	}

	global_delta_button_.setEnabled(controller.isFilterPreviewAvailable(DfxDsp::PreviewAll));
	restore_defaults_button_.setEnabled(true);
}

void FxFiltersControl::restoreDefaults()
{
	FxController::getInstance().restoreFilterDefaults();

	update();
}

int FxFiltersControl::powerColumnX() const
{
	return getWidth() - FLIP_BUTTON_MARGIN - HEADER_HEIGHT;
}

int FxFiltersControl::deltaColumnX() const
{
	return powerColumnX() - TOGGLE_GAP - HEADER_HEIGHT;
}

int FxFiltersControl::layoutBlock(FilterBlock& block, int y)
{
	// Title row: [delta] title ...... [power]. The delta sits at the left, lined up with the slider's left edge (as on
	// the EQ page, X_MARGIN in); the
	// power button is centred in the right-hand power column (under the global one).
	// Then the slider row, the full width, its value moving with the thumb.
	block.power.setBounds(powerColumnX() + (HEADER_HEIGHT - FILTER_POWER_SIZE) / 2, y + (LABEL_HEIGHT - FILTER_POWER_SIZE) / 2,
		FILTER_POWER_SIZE, FILTER_POWER_SIZE);
	block.delta.setBounds(X_MARGIN, y + (LABEL_HEIGHT - BUTTON_HEIGHT) / 2, BUTTON_WIDTH, BUTTON_HEIGHT);
	auto title_x = block.delta.getRight() + TOGGLE_GAP;
	block.title.setBounds(title_x, y, powerColumnX() - TOGGLE_GAP - title_x, LABEL_HEIGHT);

	auto slider_y = block.title.getBottom() + TITLE_GAP;
	block.cutoff.setBounds(X_MARGIN, slider_y, SLIDER_WIDTH, SLIDER_HEIGHT);

	return block.cutoff.getBottom();
}

void FxFiltersControl::resized()
{
	// Rows in the 168x257 panel, lined up with the EQ page (the flip button alone on the top line): the slope dropdown
	// 28-47 the full width (like the EQ bands dropdown), the first filter title ROW_GAP below it, then the filter
	// blocks spread evenly down to the bottom row 233-252: [restore defaults] ...... [delta][power] (restore defaults
	// at the EQ page's height). A divider in the gap above the bottom row separates the filters from the page-wide
	// controls (see paint).
	// Each filter block: title row (14 high, [delta] title ... [power]), 10 px gap, slider row (18 high, the value
	// moving beside the thumb).
	slope_.setBounds(X_MARGIN, SLOPE_Y, getWidth() - 2 * X_MARGIN, SLOPE_HEIGHT);

	// Bottom row, centred on the restore defaults button: restore defaults on the left, the global delta and the global
	// power button (both 20x20) in the right-hand columns.
	auto bottom_y = BUTTONS_Y + (BUTTON_HEIGHT - HEADER_HEIGHT) / 2;
	restore_defaults_button_.setBounds(X_MARGIN, BUTTONS_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
	global_delta_button_.setBounds(deltaColumnX(), bottom_y, HEADER_HEIGHT, HEADER_HEIGHT);
	global_power_button_.setBounds(powerColumnX(), bottom_y, HEADER_HEIGHT, HEADER_HEIGHT);

	// The first block ROW_GAP below the dropdown (as on the EQ page), the rest spread evenly down to the bottom row:
	// three equal gaps, between the blocks and below the last.
	int top = slope_.getBottom() + ROW_GAP;
	int gap = jmax(0, global_power_button_.getY() - top - 3 * FILTER_BLOCK_HEIGHT) / 3;
	int y = layoutBlock(low_cut_, top) + gap;
	y = layoutBlock(high_cut_, y) + gap;
	layoutBlock(mono_bass_, y);
}

void FxFiltersControl::paint(Graphics& g)
{
	// Divider centred in the gap between the mono bass slider and the bottom row (restore defaults, global delta and
	// power), separating the per-filter controls from the page-wide ones; X_MARGIN in from both sides.
	auto divider_y = (mono_bass_.cutoff.getBottom() + global_power_button_.getY()) / 2;
	g.setColour(Colour(FXCOLOR(Outline)).withAlpha(1.0f));
	g.drawHorizontalLine(divider_y, static_cast<float>(X_MARGIN), static_cast<float>(getWidth() - X_MARGIN));

	setText();
}

void FxFiltersControl::setText()
{
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

	low_cut_.title.setText(TRANS("Low Cut"), NotificationType::dontSendNotification);
	high_cut_.title.setText(TRANS("High Cut"), NotificationType::dontSendNotification);
	mono_bass_.title.setText(TRANS("Mono Bass"), NotificationType::dontSendNotification);
	for (auto* title : { &low_cut_.title, &high_cut_.title, &mono_bass_.title })
	{
		title->setFont(theme.getNormalFont().withHeight(TITLE_FONT_HEIGHT));
	}

	low_cut_.cutoff.setZeroValueText(TRANS("Off"));
	high_cut_.cutoff.setMaxValueText(TRANS("Off"));
	mono_bass_.cutoff.setZeroValueText(TRANS("Off"));

	restore_defaults_button_.setTooltip(TRANS("Restore Defaults"));
	for (auto* block : { &low_cut_, &high_cut_, &mono_bass_ })
	{
		block->delta.setTooltip(TRANS("Plays back only the content this filter is removing, so you can hear what it affects. When the filter is off, plays what it would remove."));
	}
	global_delta_button_.setTooltip(TRANS("Plays back everything the switched-on filters are removing. Available while the global switch is on."));

	updateLabels();
}

void FxFiltersControl::visibilityChanged()
{
	if (isVisible())
	{
		update();
	}
	else
	{
		auto& controller = FxController::getInstance();

		// Leaving the page releases any delta preview. Same effect as FxController::stopFilterPreview()
		// (which is private) on the message thread.
		if (controller.getFilterPreview() != DfxDsp::PreviewNone)
		{
			controller.setFilterPreview(DfxDsp::PreviewNone);
		}
		updatePreviewButtons();
	}
}

void FxFiltersControl::PreviewButton::setPlaying(bool playing)
{
	setToggleState(playing, NotificationType::dontSendNotification);
	if (playing && !isTimerRunning())
	{
		startTimer(PULSE_FRAME_MS);
	}
	else if (!playing && isTimerRunning())
	{
		stopTimer();
		repaint();
	}
}

void FxFiltersControl::PreviewButton::timerCallback()
{
	repaint();
}

float FxFiltersControl::PreviewButton::pulseAlpha()
{
	// A cosine from full brightness down to PULSE_DIM_ALPHA and back, once per period. The phase comes from the
	// shared millisecond clock, so every playing delta pulses in step.
	auto ms = Time::getMillisecondCounter() % (uint32)PULSE_PERIOD_MS;
	auto phase = MathConstants<float>::twoPi * (float)ms / (float)PULSE_PERIOD_MS;

	return PULSE_DIM_ALPHA + (1.0f - PULSE_DIM_ALPHA) * 0.5f * (1.0f + std::cos(phase));
}

void FxFiltersControl::PreviewButton::paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	auto bounds = getLocalBounds().toFloat().reduced(2.0f);

	Colour colour;
	if (getToggleState())
	{
		colour = Colour(FXCOLOR(SliderTrack)).withAlpha(pulseAlpha());
	}
	else if (isEnabled())
	{
		colour = Colour(FXCOLOR(DefaultText)).withAlpha(1.0f);
	}
	else
	{
		colour = Colour(FXCOLOR(DefaultText)).withAlpha(0.5f);
	}

	if (isEnabled() && (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown))
	{
		colour = colour.brighter(0.3f);
	}

	constexpr float ICON_BOX = 24.0f;
	constexpr float pi = MathConstants<float>::pi;

	Path outer_path;
	outer_path.addCentredArc(12.5f, 8.5f, 6.5f, 6.5f, 0.0f, -pi * 0.5f, pi * 0.5f, true);
	outer_path.cubicTo(19.0f, 14.5f, 13.0f, 14.5f, 13.0f, 18.5f);
	outer_path.addCentredArc(9.5f, 18.5f, 3.5f, 3.5f, 0.0f, pi * 0.5f, pi * 1.5f, false);

	Path inner_path;
	inner_path.addCentredArc(12.5f, 8.5f, 2.5f, 2.5f, 0.0f, pi * 0.5f, -pi * 0.5f, true);
	inner_path.lineTo(10.0f, 9.5f);
	inner_path.addCentredArc(10.0f, 11.5f, 2.0f, 2.0f, 0.0f, 0.0f, pi, false);

	auto scale = jmin(bounds.getWidth(), bounds.getHeight()) / ICON_BOX;
	auto offset_x = bounds.getX() + (bounds.getWidth() - ICON_BOX * scale) * 0.5f;
	auto offset_y = bounds.getY() + (bounds.getHeight() - ICON_BOX * scale) * 0.5f;
	auto transform = AffineTransform::scale(scale).translated(offset_x, offset_y);

	outer_path.applyTransform(transform);
	inner_path.applyTransform(transform);

	PathStrokeType stroke(ICON_BOX * scale * 0.09f, PathStrokeType::curved, PathStrokeType::rounded);
	g.setColour(colour);
	g.strokePath(outer_path, stroke);
	g.strokePath(inner_path, stroke);
}
