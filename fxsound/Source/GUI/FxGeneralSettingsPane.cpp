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
#include "FxGeneralSettingsPane.h"
#include "FxController.h"
#include "FxModel.h"
#include "../Utils/SysInfo/SysInfo.h"

//==============================================================================
FxGeneralSettingsPane::FxGeneralSettingsPane() :
	FxSettingsPane("General Preferences"),
	launch_toggle_(TRANS("Launch on system startup")),
	hide_help_tips_toggle_(TRANS("Hide help tips for audio controls")),
	hide_notifications_toggle_(TRANS("Hide notifications")),
	hotkeys_toggle_(TRANS("Disable keyboard shortcuts"))
{
	StringArray hotKeySettingsKeys = { FxController::HK_CMD_ON_OFF, FxController::HK_CMD_OPEN_CLOSE, FxController::HK_CMD_NEXT_PRESET, FxController::HK_CMD_PREVIOUS_PRESET, FxController::HK_CMD_NEXT_OUTPUT };
	StringArray hotkey_names = { "Turn FxSound On/Off", "Open/Close FxSound",
								   "Use Next Preset", "Use Previous Preset", "Change Playback Device"};

	setFocusContainerType(FocusContainerType::keyboardFocusContainer);

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

	language_switch_.setSelectedLanguage(FxController::getInstance().getLanguage());
	language_switch_.onLanguageChanged = [](String language_code) { FxController::getInstance().setLanguage(language_code); };

	auto os = SystemStats::getOperatingSystemType();
	if (os == SystemStats::OperatingSystemType::Windows7)
	{
		addAndMakeVisible(&launch_toggle_);
	}

	addAndMakeVisible(&hide_help_tips_toggle_);
	addAndMakeVisible(&hide_notifications_toggle_);
	addAndMakeVisible(&hotkeys_toggle_);
	addAndMakeVisible(&language_switch_);

	setText();
}

FxGeneralSettingsPane::~FxGeneralSettingsPane()
{
}

void FxGeneralSettingsPane::resized()
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
}

void FxGeneralSettingsPane::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    setText();

	FxSettingsPane::paint(g);
}

void FxGeneralSettingsPane::setText()
{
    launch_toggle_.setButtonText(TRANS("Launch on system startup"));
    hide_help_tips_toggle_.setButtonText(TRANS("Hide help tips for audio controls"));
	hide_notifications_toggle_.setButtonText(TRANS("Hide notifications"));

    hotkeys_toggle_.setButtonText(TRANS("Disable keyboard shortcuts"));
}
