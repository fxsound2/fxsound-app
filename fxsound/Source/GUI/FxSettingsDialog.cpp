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
#include "FxSettingsDialog.h"
#include "../Utils/SysInfo/SysInfo.h"

//==============================================================================
FxVolumeSlider::FxVolumeSlider()
{
	setMouseCursor(MouseCursor::PointingHandCursor);

	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

	value_label_.setFont(theme.getNormalFont().withHeight(12.0f));
	value_label_.setJustificationType(Justification::centredLeft);
	addChildComponent(value_label_);

	setWantsKeyboardFocus(true);
}

void FxVolumeSlider::setVolumeValue(float value)
{
	int display_value = int(value / 0.125f);
	auto text = String::formatted("%d", display_value);
	value_label_.setText(text, NotificationType::dontSendNotification);

	auto pos = getPositionOfValue(value);
	auto x = pos + FxTheme::SLIDER_THUMB_RADIUS + 1;
	value_label_.setBounds(value_label_.getBounds().withX(x));
}

void FxVolumeSlider::showValue(bool show)
{
	value_label_.setVisible(show && isEnabled());
}

void FxVolumeSlider::resized()
{
	Slider::resized();

	value_label_.setBounds(value_label_.getX(), (getHeight() - LABEL_HEIGHT) / 2, FxTheme::SLIDER_THUMB_RADIUS * 3, LABEL_HEIGHT);
}

void FxVolumeSlider::valueChanged()
{
	auto value = getValue();

	auto& controller = FxController::getInstance();

	if (value != controller.getVolumeNormalization())
	{
		setVolumeValue(value);

		controller.setVolumeNormalization((float)value);
	}
}

void FxVolumeSlider::enablementChanged()
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

bool FxVolumeSlider::keyPressed(const KeyPress& key)
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

FxSettingsDialog::FxSettingsDialog() : FxWindow("Settings"), tooltip_window_(this)
{
	setContent(&settings_content_);
	centreWithSize(getWidth(), getHeight());
	addToDesktop(0);
	toFront(true);
}

void FxSettingsDialog::closeButtonPressed()
{
	exitModalState(0);
	removeFromDesktop();
}

void FxSettingsDialog::paint(Graphics& g)
{
	FxWindow::paint(g);

	g.setColour(Colour(0x2b2b2b).withAlpha(1.0f));
	g.drawLine((float)SEPARATOR_X, (float)title_bar_.getBottom(), (float)SEPARATOR_X, (float)getLocalBounds().getBottom());
}

void FxSettingsDialog::SettingsButton::paint(Graphics& g)
{
	auto bounds = getLocalBounds();

	if (getToggleState())
	{
		g.setColour(Colour(0x414141).withAlpha(1.0f));
	}
	else
	{
		g.setColour(Colour(0x383838).withAlpha(1.0f));
	}
	auto rect = juce::Rectangle<float>(0, 0, (float)bounds.getHeight(), (float)bounds.getHeight());
	g.fillRoundedRectangle(rect, (float)bounds.getHeight()/4);

	image_->drawWithin(g, rect.reduced(10, 10), RectanglePlacement::centred, 1.0f);

	if (getToggleState())
	{
		g.setColour(Colour(0xffffffff));
	}
	else
	{
		g.setColour(Colour(0xB1B1B1).withAlpha(1.0f));
	}
	auto w = bounds.getWidth() - bounds.getHeight() + 5;

	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
	g.setFont(theme.getNormalFont());
	g.drawText(TRANS(getName()), juce::Rectangle<int>(bounds.getHeight()+5, 0, w, bounds.getHeight()), Justification::centredLeft);
}

bool FxSettingsDialog::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::escapeKey)
	{
		exitModalState(0);
		removeFromDesktop();
		return true;
	}

	return Component::keyPressed(key);
}

FxSettingsDialog::SettingsComponent::SettingsComponent()
    : donate_button_("Donate")
{
	audio_button_ = std::make_unique<SettingsButton>("Audio");
	audio_button_->setToggleState(true, NotificationType::dontSendNotification);
	audio_button_->setImage(Drawable::createFromImageData(BinaryData::speaker_svg, BinaryData::speaker_svgSize).get());
	audio_button_->addListener(this);

	general_button_ = std::make_unique<SettingsButton>("General");
	general_button_->setToggleState(true, NotificationType::dontSendNotification);
	general_button_->setImage(Drawable::createFromImageData(BinaryData::settings_svg, BinaryData::settings_svgSize).get());
	general_button_->addListener(this);
	
	help_button_ = std::make_unique<SettingsButton>("Help");
	help_button_->setToggleState(false, NotificationType::dontSendNotification);
	help_button_->setImage(Drawable::createFromImageData(BinaryData::question_svg, BinaryData::question_svgSize).get());
	help_button_->addListener(this);

    donate_button_.setMouseCursor(MouseCursor::PointingHandCursor);
    donate_button_.onClick = [this]() {
        URL url("https://www.paypal.com/donate/?hosted_button_id=JVNQGYXCQ2GPG");
        url.launchInDefaultBrowser();
    };

	addAndMakeVisible(audio_button_.get());
	addAndMakeVisible(general_button_.get());
	addAndMakeVisible(help_button_.get());
    addAndMakeVisible(&donate_button_);

	addAndMakeVisible(audio_settings_pane_);
	addChildComponent(general_settings_pane_);
	addChildComponent(help_settings_pane_);

    setSize(WIDTH, HEIGHT);
}

void FxSettingsDialog::SettingsComponent::resized()
{
	audio_button_->setBounds(BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
	general_button_->setBounds(BUTTON_X, audio_button_->getBottom() + 20, BUTTON_WIDTH, BUTTON_HEIGHT);
	help_button_->setBounds(BUTTON_X, general_button_->getBottom() + 20, BUTTON_WIDTH, BUTTON_HEIGHT);
    donate_button_.setBounds(BUTTON_X, help_button_->getBottom() + 20, DONATE_BUTTON_WIDTH, DONATE_BUTTON_HEIGHT);

	juce::Rectangle<int> pane_rect(SEPARATOR_X + 1, 1, getWidth() - SEPARATOR_X + 1, HEIGHT - 1);
	
	audio_settings_pane_.setBounds(pane_rect);
	general_settings_pane_.setBounds(pane_rect);
	help_settings_pane_.setBounds(pane_rect);
}

void  FxSettingsDialog::SettingsComponent::paint(Graphics&)
{
    donate_button_.setButtonText(TRANS("Donate"));
}

void  FxSettingsDialog::SettingsComponent::buttonClicked(Button* button)
{
	if (button == audio_button_.get())
	{
		button->setToggleState(true, NotificationType::dontSendNotification);
		general_button_->setToggleState(false, NotificationType::dontSendNotification);
		help_button_->setToggleState(false, NotificationType::dontSendNotification);

		audio_settings_pane_.setVisible(true);
		general_settings_pane_.setVisible(false);
		help_settings_pane_.setVisible(false);
	}
	else if (button == general_button_.get())
	{
		button->setToggleState(true, NotificationType::dontSendNotification);
		audio_button_->setToggleState(false, NotificationType::dontSendNotification);
		help_button_->setToggleState(false, NotificationType::dontSendNotification);

		general_settings_pane_.setVisible(true);
		audio_settings_pane_.setVisible(false);
		help_settings_pane_.setVisible(false);
	}
	else if (button == help_button_.get())
	{
		button->setToggleState(true, NotificationType::dontSendNotification);
		audio_button_->setToggleState(false, NotificationType::dontSendNotification);
		general_button_->setToggleState(false, NotificationType::dontSendNotification);

		help_settings_pane_.setVisible(true);
		audio_settings_pane_.setVisible(false);
		general_settings_pane_.setVisible(false);
	}
}

FxSettingsDialog::SettingsPane::SettingsPane(String name)
{
	name_ = name;
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

    title_.setFont(theme.getTitleFont());
	title_.setText(TRANS(name_), NotificationType::dontSendNotification);	
	title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	title_.setJustificationType(Justification::centredLeft);
	addAndMakeVisible(title_);
}

void FxSettingsDialog::SettingsPane::paint(Graphics&)
{
    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

    title_.setFont(theme.getTitleFont());
    title_.setText(TRANS(name_), NotificationType::dontSendNotification);    
}

FxSettingsDialog::AudioSettingsPane::AudioSettingsPane() :
	SettingsPane("Audio"),
	volume_normalizer_toggle_(TRANS("Normalize Volume"))
{
	FxModel::getModel().addListener(this);

	setFocusContainer(true);

	endpoint_title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	endpoint_title_.setJustificationType(Justification::centredLeft);

	preferred_endpoint_.setMouseCursor(MouseCursor::PointingHandCursor);
	preferred_endpoint_.setWantsKeyboardFocus(true);
	preferred_endpoint_.setEnabled(true);
	preferred_endpoint_.onChange = [this]() {
			auto endpoints = FxModel::getModel().getOutputDevices();
			auto id = preferred_endpoint_.getSelectedId();
			if (id > 0 && id <= endpoints.size())
			{
				auto pref_device_id = endpoints[id - 1].pwszID;
				auto pref_device_name = endpoints[id - 1].deviceFriendlyName;
				FxController::getInstance().setPreferredOutput(pref_device_id.c_str(), pref_device_name.c_str());
				FxController::getInstance().setOutput(id - 1);
			}
			else
			{
				if (id == preferred_endpoint_.getNumItems() - 1)
				{
					FxController::getInstance().setPreferredOutput("", "Auto");
				}
				else
				{
					FxController::getInstance().setPreferredOutput("", "None");
				}
			}
		};

	volume_normalizer_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	volume_normalizer_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	volume_normalizer_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	volume_normalizer_toggle_.setWantsKeyboardFocus(true);

	auto& controller = FxController::getInstance();
	
	bool enabled = controller.isVolumeNormalizationEnbabled();
	volume_normalizer_toggle_.setToggleState(enabled, NotificationType::dontSendNotification);
	volume_normalizer_toggle_.onClick = [this]() {
		if (volume_normalizer_toggle_.getToggleState()) {
			FxController::getInstance().setVolumeNormalizationEnabled(true);
			volume_.setEnabled(true);
		}
		else {
			FxController::getInstance().setVolumeNormalizationEnabled(false);
			volume_.setEnabled(false);
		}
	};

	auto volume_normalization_rms = controller.getVolumeNormalization();
	volume_.setSliderStyle(Slider::LinearHorizontal);
	volume_.setRange(0.125, 0.5, 0.125);
	volume_.setValue(volume_normalization_rms);
	volume_.setVolumeValue(volume_normalization_rms);
	volume_.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	volume_.setEnabled(enabled);
	addAndMakeVisible(&volume_);
	
	addAndMakeVisible(&endpoint_title_);
	addAndMakeVisible(&preferred_endpoint_);
	addAndMakeVisible(&volume_normalizer_toggle_);

	setText();
}

FxSettingsDialog::AudioSettingsPane::~AudioSettingsPane()
{
	preferred_endpoint_.onChange = nullptr;

	FxModel::getModel().removeListener(this);
}

void FxSettingsDialog::AudioSettingsPane::resized()
{
	auto bounds = getLocalBounds().withLeft(X_MARGIN).withTop(Y_MARGIN).withHeight(TITLE_HEIGHT);
	title_.setBounds(bounds);

	endpoint_title_.setBounds(X_MARGIN, ENDPOINT_Y, ENDPOINT_LABEL_WIDTH, ENDPOINT_LIST_HEIGHT);
	auto width = getWidth() - ((X_MARGIN + 5) * 2) - ENDPOINT_LABEL_WIDTH;
	preferred_endpoint_.setBounds(ENDPOINT_LABEL_WIDTH + X_MARGIN + 15, ENDPOINT_Y, width, ENDPOINT_LIST_HEIGHT);

	int y = preferred_endpoint_.getBottom() + 20;

	volume_normalizer_toggle_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN, TOGGLE_BUTTON_HEIGHT);
	y = volume_normalizer_toggle_.getBottom() + 10;

	volume_.setBounds(X_MARGIN, y, SLIDER_WIDTH, SLIDER_HEIGHT);
}

void FxSettingsDialog::AudioSettingsPane::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

	setText();

	SettingsPane::paint(g);
}

void FxSettingsDialog::AudioSettingsPane::setText()
{
	auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

	endpoint_title_.setText(TRANS("Preferred output:"), NotificationType::dontSendNotification);
	endpoint_title_.setFont(theme.getNormalFont());
	preferred_endpoint_.setTextWhenNothingSelected(TRANS("Select preferred output"));

	updateEndpointList();
}

void FxSettingsDialog::AudioSettingsPane::modelChanged(FxModel::Event model_event)
{
	if (model_event == FxModel::Event::OutputListUpdated)
	{
		updateEndpointList();
	}
}

void FxSettingsDialog::AudioSettingsPane::updateEndpointList()
{
	auto& controller = FxController::getInstance();
	auto endpoints = FxModel::getModel().getOutputDevices();
	auto i = 1;
	auto pref_device_index = 0;

	preferred_endpoint_.clear(NotificationType::dontSendNotification);
	for (auto endpoint : endpoints)
	{
		preferred_endpoint_.addItem(endpoint.deviceFriendlyName.c_str(), i);
		if (endpoint.deviceNumChannel == 1)
		{
			preferred_endpoint_.setItemEnabled(i, false);
		}
		if (endpoint.pwszID == controller.getPreferredOutputId().toWideCharPointer())
		{
			pref_device_index = i;
		}
		i++;
	}
	preferred_endpoint_.addItem(TRANS("Newly connected output device"), i);
	auto auto_index = i++;
	preferred_endpoint_.addItem(TRANS("None"), i);
	auto none_index = i;

	if (controller.getPreferredOutputName().equalsIgnoreCase(L"Auto"))
	{
		pref_device_index = auto_index;
	}
	else if (controller.getPreferredOutputName().equalsIgnoreCase(L"None"))
	{
		pref_device_index = none_index;
	}

	preferred_endpoint_.setSelectedId(pref_device_index, NotificationType::dontSendNotification);
}

void FxSettingsDialog::AudioSettingsPane::mouseEnter(const MouseEvent& mouse_event)
{
	Component::mouseEnter(mouse_event);

	volume_.showValue(volume_.isMouseOver(false));
}

void FxSettingsDialog::AudioSettingsPane::mouseExit(const MouseEvent& mouse_event)
{
	Component::mouseEnter(mouse_event);

	volume_.showValue(volume_.isMouseOver(false));
}

FxSettingsDialog::GeneralSettingsPane::GeneralSettingsPane() :
	SettingsPane("General Preferences"),
	launch_toggle_(TRANS("Launch on system startup")),
	hide_help_tips_toggle_(TRANS("Hide help tips for audio controls")),
	hide_notifications_toggle_(TRANS("Hide notifications")),
	hotkeys_toggle_(TRANS("Disable keyboard shortcuts")),
	reset_presets_button_(TRANS("Reset presets to factory defaults"))
{
	StringArray hotKeySettingsKeys = { FxController::HK_CMD_ON_OFF, FxController::HK_CMD_OPEN_CLOSE, FxController::HK_CMD_NEXT_PRESET, FxController::HK_CMD_PREVIOUS_PRESET, FxController::HK_CMD_NEXT_OUTPUT };
	StringArray hotkey_names = { "Turn FxSound On/Off", "Open/Close FxSound",
								   "Use Next Preset", "Use Previous Preset", "Change Playback Device"};

	setFocusContainer(true);

	launch_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	launch_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	launch_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	launch_toggle_.setWantsKeyboardFocus(true);
	
	hide_help_tips_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
    hide_help_tips_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
    hide_help_tips_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	hide_help_tips_toggle_.setWantsKeyboardFocus(true);
	hide_notifications_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	hide_notifications_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	hide_notifications_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	hide_notifications_toggle_.setWantsKeyboardFocus(true);
	hotkeys_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	hotkeys_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	hotkeys_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	hotkeys_toggle_.setWantsKeyboardFocus(true);

	bool hotkey_enabled = FxModel::getModel().getHotkeySupport();
	for (int i=0; i<hotkey_names.size(); i++)
	{
		auto label = new FxHotkeyLabel(hotkey_names[i], hotKeySettingsKeys[i]);
		label->setEnabled(hotkey_enabled);
		hotkey_labels_.add(label);
		addAndMakeVisible(label);
	}

    if (SysInfo::canSupportHotkeys())
    {
        hotkeys_toggle_.setToggleState(!FxModel::getModel().getHotkeySupport(), NotificationType::dontSendNotification);
    }
    else
    {
        hotkeys_toggle_.setToggleState(true, NotificationType::dontSendNotification);
        hotkeys_toggle_.setEnabled(false);
    }
	hotkeys_toggle_.onClick = [this]()  { 
											FxController::getInstance().enableHotkeys(!hotkeys_toggle_.getToggleState());
											bool enabled = FxModel::getModel().getHotkeySupport();
											for (auto& hotkey_label : hotkey_labels_)
											{
												hotkey_label->setEnabled(enabled);
											}
										};

	launch_toggle_.setToggleState(FxController::getInstance().isLaunchOnStartup(), NotificationType::dontSendNotification);
	launch_toggle_.onClick = [this]() { FxController::getInstance().setLaunchOnStartup(launch_toggle_.getToggleState()); };

    hide_help_tips_toggle_.setToggleState(FxController::getInstance().isHelpTooltipsHidden(), NotificationType::dontSendNotification);
    hide_help_tips_toggle_.onClick = [this]() { FxController::getInstance().setHelpTooltipsHidden(hide_help_tips_toggle_.getToggleState()); };
	
	hide_notifications_toggle_.setToggleState(FxController::getInstance().isNotificationsHidden(), NotificationType::dontSendNotification);
	hide_notifications_toggle_.onClick = [this]() { FxController::getInstance().setNotificationsHidden(hide_notifications_toggle_.getToggleState()); };

	reset_presets_button_.setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	reset_presets_button_.setEnabled(FxModel::getModel().getUserPresetCount() > 0);
    reset_presets_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	reset_presets_button_.onClick = [this]() {
		FxController::getInstance().resetPresets();
		reset_presets_button_.setEnabled(false);
	};

	auto os = SystemStats::getOperatingSystemType();
	if (os == SystemStats::OperatingSystemType::Windows7)
	{
		addAndMakeVisible(&launch_toggle_);
	}
	
	addAndMakeVisible(&hide_help_tips_toggle_);
	addAndMakeVisible(&hide_notifications_toggle_);
	addAndMakeVisible(&hotkeys_toggle_);
	addAndMakeVisible(&reset_presets_button_);
	addAndMakeVisible(&language_switch_);

	setText();
}

FxSettingsDialog::GeneralSettingsPane::~GeneralSettingsPane()
{
}

void FxSettingsDialog::GeneralSettingsPane::resized()
{
	auto bounds = getLocalBounds().withLeft(X_MARGIN).withTop(Y_MARGIN).withHeight(TITLE_HEIGHT);
	title_.setBounds(bounds);

    language_switch_.setBounds(X_MARGIN, LANGUAGE_SWITCH_Y, FxLanguage::WIDTH, FxLanguage::HEIGHT);

    int y = language_switch_.getBottom() + 20;
	if (launch_toggle_.isVisible())
	{
		launch_toggle_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN, TOGGLE_BUTTON_HEIGHT);
		y = launch_toggle_.getBottom() + 20;
	}

    hide_help_tips_toggle_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN, TOGGLE_BUTTON_HEIGHT);

	y = hide_help_tips_toggle_.getBottom() + 10;
	hide_notifications_toggle_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN, TOGGLE_BUTTON_HEIGHT);

    y = hide_notifications_toggle_.getBottom() + 10;
	hotkeys_toggle_.setBounds(X_MARGIN, y, getWidth()-X_MARGIN, TOGGLE_BUTTON_HEIGHT);

	y = hotkeys_toggle_.getBottom() + 5;
	for (auto hotkey_label : hotkey_labels_)
	{
		hotkey_label->setBounds(HOTKEY_LABEL_X, y, getWidth()-HOTKEY_LABEL_X, HOTKEY_LABEL_HEIGHT);
		y += HOTKEY_LABEL_HEIGHT + 10;
	}
	
	resizeResetButton(X_MARGIN, y + 10);
}

void FxSettingsDialog::GeneralSettingsPane::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    setText();

	SettingsPane::paint(g);    
}

void FxSettingsDialog::GeneralSettingsPane::setText()
{
    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

    int height = FxSettingsDialog::SettingsComponent::HEIGHT;
    launch_toggle_.setButtonText(TRANS("Launch on system startup"));
    hide_help_tips_toggle_.setButtonText(TRANS("Hide help tips for audio controls"));
	hide_notifications_toggle_.setButtonText(TRANS("Hide notifications"));

    hotkeys_toggle_.setButtonText(TRANS("Disable keyboard shortcuts"));
    reset_presets_button_.setButtonText(TRANS("Reset presets to factory defaults"));
	
	resizeResetButton(reset_presets_button_.getX(), reset_presets_button_.getY());
}

void FxSettingsDialog::GeneralSettingsPane::resizeResetButton(int x, int y)
{
	String buttonText = reset_presets_button_.getButtonText();
	
	int index = 0;
	int lineCount = 1;
	do {
		index = buttonText.indexOfChar(index, L'\n');
		if (index >= 0)
		{
			index++;
			lineCount++;
		}
		else
		{
			break;
		}
	} while (lineCount <= 3); // Resize the button height for upto 3 lines of text

	int buttonWidth = min(reset_presets_button_.getBestWidthForHeight(BUTTON_HEIGHT * lineCount), MAX_BUTTON_WIDTH);
	if (buttonWidth < BUTTON_WIDTH)
	{
		buttonWidth = BUTTON_WIDTH;
	}

	reset_presets_button_.setBounds(x, y, buttonWidth, BUTTON_HEIGHT * lineCount);
}

FxSettingsDialog::HelpSettingsPane::HelpSettingsPane() : SettingsPane("Help"), debug_log_toggle_(TRANS("Disable debug logging"))
{	
	version_title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	version_title_.setJustificationType(Justification::centredLeft);
	version_text_.setJustificationType(Justification::centredLeft);	
	support_title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	support_title_.setJustificationType(Justification::centredLeft);	
	maintenance_title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	maintenance_title_.setJustificationType(Justification::centredLeft);

	changelog_link_.setURL(URL(L"https://www.fxsound.com/changelog"));
	changelog_link_.setJustificationType(Justification::topLeft);
	quicktour_link_.setJustificationType(Justification::topLeft);
	submitlogs_link_.setJustificationType(Justification::topLeft);
	helpcenter_link_.setURL(URL(L"https://www.fxsound.com/learning-center"));
	helpcenter_link_.setJustificationType(Justification::topLeft);
    feedback_link_.setURL(URL("https://james722808.typeform.com/to/QfEP5QrP"));
	feedback_link_.setJustificationType(Justification::topLeft);

	debug_log_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	debug_log_toggle_.setToggleState(!FxModel::getModel().getDebugLogging(), NotificationType::dontSendNotification);
	debug_log_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	debug_log_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));

    setText();

	addAndMakeVisible(version_title_);
	addAndMakeVisible(version_text_);
	addAndMakeVisible(support_title_);
	addAndMakeVisible(maintenance_title_);
	addAndMakeVisible(changelog_link_);
	addChildComponent(quicktour_link_);
	addChildComponent(submitlogs_link_);
	addAndMakeVisible(helpcenter_link_);
	addAndMakeVisible(updates_button_);
	addChildComponent(debug_log_toggle_);
}

void FxSettingsDialog::HelpSettingsPane::resized()
{
	auto bounds = getLocalBounds().withLeft(X_MARGIN).withTop(Y_MARGIN).withHeight(TITLE_HEIGHT);
	title_.setBounds(bounds);

	version_title_.setBounds(X_MARGIN, TEXT_Y, getWidth()-X_MARGIN, TITLE_HEIGHT);
	version_text_.setBounds(X_MARGIN, version_title_.getBottom()+10, getWidth()-X_MARGIN, TEXT_HEIGHT);
	changelog_link_.setBounds(X_MARGIN+5, version_text_.getBottom()+10, getWidth()-X_MARGIN, HYPERLINK_HEIGHT);
	support_title_.setBounds(X_MARGIN, changelog_link_.getBottom()+20, getWidth()-X_MARGIN, TITLE_HEIGHT);
	helpcenter_link_.setBounds(X_MARGIN+5, support_title_.getBottom()+10, getWidth()-X_MARGIN, HYPERLINK_HEIGHT);
	maintenance_title_.setBounds(X_MARGIN, helpcenter_link_.getBottom()+20, getWidth()-X_MARGIN, TITLE_HEIGHT);
	updates_button_.setBounds(X_MARGIN+5, maintenance_title_.getBottom()+10, BUTTON_WIDTH, HYPERLINK_HEIGHT);
}

void FxSettingsDialog::HelpSettingsPane::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    setText();

	SettingsPane::paint(g);
}

void FxSettingsDialog::HelpSettingsPane::setText()
{
    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

    version_title_.setText(TRANS("Version"), NotificationType::dontSendNotification);
    version_title_.setFont(theme.getNormalFont());
    
    version_text_.setText(L"v" + JUCEApplication::getInstance()->getApplicationVersion(), NotificationType::dontSendNotification);
    version_text_.setFont(theme.getSmallFont());
    
    support_title_.setText(TRANS("Support"), NotificationType::dontSendNotification);
    support_title_.setFont(theme.getNormalFont());
    
    maintenance_title_.setText(TRANS("Maintenance"), NotificationType::dontSendNotification);
    maintenance_title_.setFont(theme.getNormalFont());

    changelog_link_.setButtonText(TRANS("Changelog"));    
    quicktour_link_.setButtonText(TRANS("Quick tour"));    
    submitlogs_link_.setButtonText(TRANS("Submit debug logs"));    
    helpcenter_link_.setButtonText(TRANS("Help center"));        
    feedback_link_.setButtonText(TRANS("Feedback"));    
    updates_button_.setButtonText(TRANS("Check for updates"));

	updates_button_.onClick = [this]() {
		ChildProcess child_process;
		child_process.start("updater.exe /checknow");
	};
}

void FxSettingsDialog::HelpSettingsPane::buttonStateChanged(Button* button)
{
	if (button == &debug_log_toggle_)
	{
		FxModel::getModel().setDebugLogging(!debug_log_toggle_.getToggleState());
	}
}
