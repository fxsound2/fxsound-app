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
	effects_shown_ = true;

	setLookAndFeel();

	addAndMakeVisible(effects_);
	addChildComponent(equalizer_control_);

	effects_.addMouseListener(this, true);
	equalizer_control_.addMouseListener(this, true);

	flip_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	flip_button_.setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	flip_button_.setWantsKeyboardFocus(true);
	flip_button_.onClick = [this]() {
		effects_shown_ = !effects_shown_;

		effects_.setVisible(effects_shown_);
		equalizer_control_.setVisible(!effects_shown_);
		};

	addAndMakeVisible(flip_button_);

	setSize(WIDTH, HEIGHT);
}

void FxAudioControls::update()
{
	effects_.update();
	effects_.showValues(true);

	equalizer_control_.update();
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
}

void FxAudioControls::resized()
{
	effects_.setBounds(0, 0, WIDTH, HEIGHT);
	equalizer_control_.setBounds(0, 0, WIDTH, HEIGHT);

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
		labels_[i]->setFont(theme.getNormalFont().withHeight(14));
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
			effects_[i]->setEffectValue(value*10.0);
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

void FxEffects::paint(Graphics& g)
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
        labels_[i]->setFont(theme.getNormalFont().withHeight(14));
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

	value_label_.setFont(theme.getNormalFont().withHeight(12.0f));
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
	auto x = pos + FxTheme::SLIDER_THUMB_RADIUS + 1;
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
	auto value = getValue();

	if (value != FxController::getInstance().getEffectValue(effect_)*10.0)
	{
		FxController::getInstance().setEffectValue(effect_, value);
		
		auto text = String::formatted("%.0f", value);
		value_label_.setText(text, NotificationType::dontSendNotification);

		auto pos = getPositionOfValue(value);
		auto x = pos + FxTheme::SLIDER_THUMB_RADIUS + 1;
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

	master_gain_title_.setFont(theme.getNormalFont().withHeight(14));
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

	volume_leveling_title_.setFont(theme.getNormalFont().withHeight(14));
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

	filter_q_title_.setFont(theme.getNormalFont().withHeight(14));
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

	balance_title_.setFont(theme.getNormalFont().withHeight(14));
	balance_title_.setJustificationType(Justification::topLeft);
	border = balance_title_.getBorderSize();
	border.setLeft(0);
	balance_title_.setBorderSize(border);

	left_label_.setFont(theme.getNormalFont().withHeight(12.0f));
	left_label_.setJustificationType(Justification::centredLeft);
	border = left_label_.getBorderSize();
	border.setLeft(0);
	border.setTop(0);
	left_label_.setBorderSize(border);

	right_label_.setFont(theme.getNormalFont().withHeight(12.0f));
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

void FxEqualizerControl::setLookAndFeel(FxTheme& theme)
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

void FxEqualizerControl::paint(Graphics& g)
{
	setText();
	updateEqualizerBandsText();
}

void FxEqualizerControl::setText()
{
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
	auto font = theme.getNormalFont().withHeight(14);

	master_gain_title_.setFont(font);
	master_gain_title_.setText(TRANS("Master Gain"), NotificationType::dontSendNotification);

	volume_leveling_title_.setFont(font);
	volume_leveling_title_.setText(TRANS("Volume Leveling"), NotificationType::dontSendNotification);

	filter_q_title_.setFont(font);
	filter_q_title_.setText(TRANS("Filter Q"), NotificationType::dontSendNotification);

	balance_title_.setFont(font);
	balance_title_.setText(TRANS("Balance"), NotificationType::dontSendNotification);

	left_label_.setFont(theme.getNormalFont().withHeight(12.0f));
	left_label_.setText(TRANS("Left"), NotificationType::dontSendNotification);

	right_label_.setFont(theme.getNormalFont().withHeight(12.0f));
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