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
	addChildComponent(low_cut_control_);

	effects_.addMouseListener(this, true);
	equalizer_control_.addMouseListener(this, true);
	low_cut_control_.addMouseListener(this, true);

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
	low_cut_control_.setVisible(page_ == LowCutPage);
}

void FxAudioControls::update()
{
	effects_.update();
	effects_.showValues(true);

	equalizer_control_.update();
	low_cut_control_.update();
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
	low_cut_control_.setLookAndFeel(theme);
}

void FxAudioControls::resized()
{
	effects_.setBounds(0, 0, WIDTH, HEIGHT);
	equalizer_control_.setBounds(0, 0, WIDTH, HEIGHT);
	low_cut_control_.setBounds(0, 0, WIDTH, HEIGHT);

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
	constexpr int LOW_CUT_FIRST_ORDER_SLOPE_DB = 6;

	String lowCutSlopeValue(int slope_db)
	{
		return String(slope_db) + " dB";
	}

	String lowCutSlopeTooltip(int slope_db)
	{
		String meaning = TRANS("The dB value is the slope in dB per octave: how much the level drops for every halving of frequency below the cutoff.");

		switch (slope_db)
		{
		case 6:
			return TRANS("6 dB/oct: the gentlest rolloff, a single first-order stage. Q has no effect.") + "\n" + meaning;
		case 12:
			return TRANS("12 dB/oct: gentle rolloff.") + "\n" + meaning;
		case 18:
			return TRANS("18 dB/oct: moderate rolloff.") + "\n" + meaning;
		case 24:
			return TRANS("24 dB/oct: firm rolloff. Recommended for most listening.") + "\n" + meaning;
		case 36:
			return TRANS("36 dB/oct: steep rolloff, a firmer guard against subsonic content.") + "\n" + meaning;
		case 48:
		default:
			return TRANS("48 dB/oct: steepest rolloff, at the cost of more phase shift near the cutoff.") + "\n" + meaning;
		}
	}

	int lowCutSlopePosition(int slope_db)
	{
		for (int position = 0; position < DfxDsp::LOW_CUT_NUM_SLOPES; position++)
		{
			if (DfxDsp::LOW_CUT_SLOPES_DB[position] == slope_db)
			{
				return position;
			}
		}

		return 0;
	}

	String lowCutToggleTooltip(int hp_cutoff_hz)
	{
		if (hp_cutoff_hz <= 0)
		{
			return TRANS("Butterworth high-pass. The cutoff is Off (0Hz), so audio passes through unfiltered.");
		}

		return TRANS("Butterworth high-pass: removes content below") + " " + String(hp_cutoff_hz) + "Hz. " + TRANS("Adds no delay.");
	}
}

FxLowCutControl::ValueSlider::ValueSlider()
{
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

	value_label_.setFont(theme.getSmallFont().withHeight(11.0f));
	value_label_.setJustificationType(Justification::centredLeft);
	value_label_.setInterceptsMouseClicks(false, false);
}

void FxLowCutControl::ValueSlider::setValueText(const String& text)
{
	value_label_.setText(text, NotificationType::dontSendNotification);
	positionValueLabel();
}

void FxLowCutControl::ValueSlider::resized()
{
	Slider::resized();
	positionValueLabel();
}

void FxLowCutControl::ValueSlider::moved()
{
	positionValueLabel();
}

void FxLowCutControl::ValueSlider::valueChanged()
{
	positionValueLabel();
}

void FxLowCutControl::ValueSlider::parentHierarchyChanged()
{
	auto* parent = getParentComponent();

	if (parent != nullptr && value_label_.getParentComponent() != parent)
	{
		parent->addAndMakeVisible(value_label_);
		positionValueLabel();
	}
}

void FxLowCutControl::ValueSlider::enablementChanged()
{
	Slider::enablementChanged();
	value_label_.setEnabled(isEnabled());
}

void FxLowCutControl::ValueSlider::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
	if (!FxAudioSlider::stepByWheel(*this, wheel))
	{
		Slider::mouseWheelMove(event, wheel);
	}
}

void FxLowCutControl::ValueSlider::positionValueLabel()
{
	auto text_width = static_cast<int>(GlyphArrangement::getStringWidth(value_label_.getFont(), value_label_.getText())) +
		value_label_.getBorderSize().getLeftAndRight() + 2;
	auto pos = (int)getPositionOfValue(getValue());
	auto x = getX() + pos + FxTheme::SLIDER_THUMB_RADIUS / 2 + 1;

	value_label_.setBounds(x, getY() + (getHeight() - LABEL_HEIGHT) / 2, text_width, LABEL_HEIGHT);
}

FxLowCutControl::FxLowCutControl() :
	power_button_(L"lowCutPowerButton"),
	hp_cutoff_slider_("%0.0f Hz", (float)DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ),
	q_slider_("%.2f", DfxDsp::LOW_CUT_DEFAULT_Q),
	restore_defaults_button_("restoreDefaultsButton", DrawableButton::ButtonStyle::ImageFitted)
{
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

	power_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	power_button_.setImageWidth(POWER_ICON_SIZE);
	power_button_.setWantsKeyboardFocus(true);
	power_button_.onClick = [this]() {
		bool on = !power_button_.getPowerState();

		power_button_.setPowerState(on);
		FxController::getInstance().setLowCutOn(on);
		if (!on)
		{
			preview_button_.setToggleState(false, NotificationType::dontSendNotification);
		}
		updateEnabled();
		};

	for (auto* title : { &slope_title_, &hp_cutoff_title_, &q_title_ })
	{
		title->setFont(theme.getNormalFont().withHeight(12.0f));
		title->setJustificationType(Justification::topLeft);
		auto border = title->getBorderSize();
		border.setLeft(0);
		title->setBorderSize(border);
	}

	for (auto* slider : std::initializer_list<Slider*>{ &slope_slider_, &hp_cutoff_slider_, &q_slider_ })
	{
		slider->setSliderStyle(Slider::LinearHorizontal);
		slider->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
		slider->setMouseCursor(MouseCursor::PointingHandCursor);
		slider->setWantsKeyboardFocus(true);
	}

	slope_slider_.setRange(0, DfxDsp::LOW_CUT_NUM_SLOPES - 1, 1);
	slope_slider_.onValueChange = [this]() {
		int position = jlimit(0, DfxDsp::LOW_CUT_NUM_SLOPES - 1, static_cast<int>(slope_slider_.getValue()));

		FxController::getInstance().setLowCutSlope(DfxDsp::LOW_CUT_SLOPES_DB[position]);
		updateLabels();
		updateEnabled();
		};

	hp_cutoff_slider_.setRange(DfxDsp::LOW_CUT_MIN_CUTOFF_HZ, DfxDsp::LOW_CUT_MAX_CUTOFF_HZ, 1);
	hp_cutoff_slider_.setWheelStepsByInterval(true);
	hp_cutoff_slider_.onValueChange = [this]() {
		auto& controller = FxController::getInstance();
		int cutoff_hz = static_cast<int>(hp_cutoff_slider_.getValue());

		if (controller.getLowCutCutoff() != cutoff_hz)
		{
			controller.setLowCutCutoff(cutoff_hz);
		}
		updateLabels();
		};

	q_slider_.setRange(DfxDsp::LOW_CUT_MIN_Q, DfxDsp::LOW_CUT_MAX_Q, 0.01);
	q_slider_.setSkewFactorFromMidPoint(DfxDsp::LOW_CUT_DEFAULT_Q);
	q_slider_.setWheelStepsByInterval(true);
	q_slider_.onValueChange = [this]() {
		auto& controller = FxController::getInstance();
		auto q = static_cast<float>(q_slider_.getValue());

		if (std::abs(controller.getLowCutQ() - q) > 0.001f)
		{
			controller.setLowCutQ(q);
		}
		updateLabels();
		};

	restore_defaults_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	restore_defaults_button_.setWantsKeyboardFocus(true);
	restore_defaults_button_.onClick = [this]() {
		restoreDefaults();
		};

	preview_button_.setWantsKeyboardFocus(true);
	preview_button_.onClick = [this]() {
		FxController::getInstance().setLowCutPreviewOn(preview_button_.getToggleState());
		};

	setText();
	update();

	filter_title_.setColour(Label::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	filter_title_.setJustificationType(Justification::centredLeft);
	filter_title_.setInterceptsMouseClicks(false, false);
	auto filter_title_border = filter_title_.getBorderSize();
	filter_title_border.setLeft(0);
	filter_title_.setBorderSize(filter_title_border);

	addAndMakeVisible(power_button_);
	addAndMakeVisible(filter_title_);
	addAndMakeVisible(slope_title_);
	addAndMakeVisible(slope_slider_);
	addAndMakeVisible(hp_cutoff_title_);
	addAndMakeVisible(hp_cutoff_slider_);
	addAndMakeVisible(q_title_);
	addAndMakeVisible(q_slider_);
	addAndMakeVisible(restore_defaults_button_);
	addAndMakeVisible(preview_button_);
}

FxLowCutControl::~FxLowCutControl()
{
	power_button_.onClick = nullptr;
	slope_slider_.onValueChange = nullptr;
	hp_cutoff_slider_.onValueChange = nullptr;
	q_slider_.onValueChange = nullptr;
	restore_defaults_button_.onClick = nullptr;
	preview_button_.onClick = nullptr;
}

void FxLowCutControl::update()
{
	auto& controller = FxController::getInstance();

	power_button_.setPowerState(controller.isLowCutOn());
	slope_slider_.setValue(static_cast<double>(lowCutSlopePosition(controller.getLowCutSlope())), NotificationType::dontSendNotification);
	hp_cutoff_slider_.setValue(static_cast<double>(controller.getLowCutCutoff()), NotificationType::sendNotificationSync);
	q_slider_.setValue(static_cast<double>(controller.getLowCutQ()), NotificationType::sendNotificationSync);
	preview_button_.setToggleState(controller.isLowCutPreviewOn(), NotificationType::dontSendNotification);

	updateLabels();
	updateEnabled();
}

void FxLowCutControl::setLookAndFeel(FxTheme&)
{
	restore_defaults_image_ = Drawable::createFromImageData(FXIMAGE(RestoreDefaultsButton), FXIMAGESIZE(RestoreDefaultsButton));
	restore_defaults_hover_image_ = Drawable::createFromImageData(FXIMAGE(RestoreDefaultsButtonHover), FXIMAGESIZE(RestoreDefaultsButtonHover));

	restore_defaults_button_.setImages(restore_defaults_image_.get(), restore_defaults_hover_image_.get());
}

void FxLowCutControl::updateLabels()
{
	auto& controller = FxController::getInstance();
	int slope_db = controller.getLowCutSlope();
	int hp_cutoff_hz = static_cast<int>(hp_cutoff_slider_.getValue());
	bool show_help = !controller.isHelpTooltipsHidden();

	slope_title_.setText(TRANS("Steepness"), NotificationType::dontSendNotification);
	slope_slider_.setValueText(lowCutSlopeValue(slope_db));
	slope_slider_.setTooltip(show_help ? lowCutSlopeTooltip(slope_db) : String());

	hp_cutoff_title_.setText(TRANS("Cutoff Frequency"), NotificationType::dontSendNotification);
	hp_cutoff_slider_.setTooltip(show_help ? TRANS("Frequencies below this are removed. Set to Off (0Hz) to pass audio through unfiltered.") + "\n" +
		TRANS("-3dB at the cutoff itself.") : String());

	q_title_.setText(TRANS("Filter Q"), NotificationType::dontSendNotification);
	q_slider_.setTooltip(show_help ? TRANS("Resonance at the cutoff. 0.71 is neutral. Higher values add a bass boost just above the cutoff (up to about +6dB, which can clip loud audio); lower values soften the knee.") +
		(slope_db == LOW_CUT_FIRST_ORDER_SLOPE_DB ? "\n" + TRANS("Q has no effect at 6 dB/oct.") : String()) : String());
	power_button_.setTooltip(show_help ? lowCutToggleTooltip(hp_cutoff_hz) : String());
}

void FxLowCutControl::updateEnabled()
{
	bool on = power_button_.getPowerState();
	bool q_applies = FxController::getInstance().getLowCutSlope() != LOW_CUT_FIRST_ORDER_SLOPE_DB;

	for (auto* component : std::initializer_list<Component*>{ &slope_title_, &slope_slider_,
		&hp_cutoff_title_, &hp_cutoff_slider_, &restore_defaults_button_, &preview_button_ })
	{
		component->setEnabled(on);
	}
	q_title_.setEnabled(on && q_applies);
	q_slider_.setEnabled(on && q_applies);
}

void FxLowCutControl::restoreDefaults()
{
	auto& controller = FxController::getInstance();

	controller.setLowCutPreviewOn(false);
	controller.setLowCutSlope(DfxDsp::LOW_CUT_DEFAULT_SLOPE_DB);
	controller.setLowCutQ(DfxDsp::LOW_CUT_DEFAULT_Q);
	controller.setLowCutCutoff(DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ);

	update();
}

void FxLowCutControl::resized()
{
	auto width = getWidth() - (X_MARGIN * 2);

	int y = Y_MARGIN;
	power_button_.setBounds(X_MARGIN, y, TOGGLE_HEIGHT, TOGGLE_HEIGHT);
	filter_title_.setBounds(power_button_.getRight() + TOGGLE_GAP, y, width - TOGGLE_HEIGHT - TOGGLE_GAP, TOGGLE_HEIGHT);

	y = power_button_.getBottom() + ROW_GAP;
	slope_title_.setBounds(X_MARGIN + FxTheme::SLIDER_THUMB_RADIUS, y, SLIDER_WIDTH, LABEL_HEIGHT);
	slope_slider_.setBounds(X_MARGIN, slope_title_.getBottom() + 1, SLIDER_WIDTH, SLIDER_HEIGHT);

	y = slope_slider_.getBottom() + ROW_GAP;
	hp_cutoff_title_.setBounds(X_MARGIN + FxTheme::SLIDER_THUMB_RADIUS, y, SLIDER_WIDTH, LABEL_HEIGHT);
	hp_cutoff_slider_.setBounds(X_MARGIN, hp_cutoff_title_.getBottom() + 1, SLIDER_WIDTH, SLIDER_HEIGHT);

	y = hp_cutoff_slider_.getBottom() + ROW_GAP;
	q_title_.setBounds(X_MARGIN + FxTheme::SLIDER_THUMB_RADIUS, y, SLIDER_WIDTH, LABEL_HEIGHT);
	q_slider_.setBounds(X_MARGIN, q_title_.getBottom() + 1, SLIDER_WIDTH, SLIDER_HEIGHT);

	restore_defaults_button_.setBounds(X_MARGIN, BUTTONS_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
	preview_button_.setBounds(restore_defaults_button_.getRight() + BUTTON_GAP, BUTTONS_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
}

void FxLowCutControl::paint(Graphics&)
{
	setText();
}

void FxLowCutControl::setText()
{
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

	filter_title_.setText(TRANS("Low Cut Filter"), NotificationType::dontSendNotification);
	filter_title_.setFont(theme.getNormalFont().withHeight(14.0f));
	for (auto* title : { &slope_title_, &hp_cutoff_title_, &q_title_ })
	{
		title->setFont(theme.getNormalFont().withHeight(12.0f));
	}
	hp_cutoff_slider_.setZeroValueText(TRANS("Off"));

	restore_defaults_button_.setTooltip(TRANS("Restore Defaults"));
	preview_button_.setTooltip(TRANS("Plays back only the content the filter is removing, so you can hear what it affects."));

	updateLabels();
}

void FxLowCutControl::visibilityChanged()
{
	if (isVisible())
	{
		update();
	}
	else if (preview_button_.getToggleState())
	{
		FxController::getInstance().setLowCutPreviewOn(false);
		preview_button_.setToggleState(false, NotificationType::dontSendNotification);
	}
}

void FxLowCutControl::PreviewButton::paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	auto bounds = getLocalBounds().toFloat().reduced(2.0f);

	Colour colour;
	if (getToggleState())
	{
		colour = Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f);
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
