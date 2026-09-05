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
#include <Psapi.h>

//==============================================================================
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

	g.setColour(Colour(FXCOLOR(Outline)).withAlpha(1.0f));
	g.drawLine((float)SEPARATOR_X, (float)title_bar_.getBottom(), (float)SEPARATOR_X, (float)getLocalBounds().getBottom());
}

void FxSettingsDialog::SettingsButton::paint(Graphics& g)
{
	auto bounds = getLocalBounds();

	if (getToggleState())
	{
		g.setColour(Colour(FXCOLOR(MenuHighlightBackground)).withAlpha(1.0f));
	}
	else
	{
		g.setColour(Colour(FXCOLOR(MenuBackground)).withAlpha(1.0f));
	}
	auto rect = juce::Rectangle<float>(0, 0, (float)bounds.getHeight(), (float)bounds.getHeight());
	g.fillRoundedRectangle(rect, (float)bounds.getHeight()/4);

	image_->drawWithin(g, rect.reduced(10, 10), RectanglePlacement::centred, 1.0f);

	if (getToggleState())
	{
		g.setColour(Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
	}
	else
	{
		g.setColour(Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
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
{
	audio_button_ = std::make_unique<SettingsButton>("Audio");
	audio_button_->setToggleState(true, NotificationType::dontSendNotification);
	audio_button_->setImage(Drawable::createFromImageData(BinaryData::speaker_svg, BinaryData::speaker_svgSize).get());
	audio_button_->addListener(this);


	general_button_ = std::make_unique<SettingsButton>("General");
	general_button_->setToggleState(false, NotificationType::dontSendNotification);
	general_button_->setImage(Drawable::createFromImageData(BinaryData::settings_svg, BinaryData::settings_svgSize).get());
	general_button_->addListener(this);
	
	help_button_ = std::make_unique<SettingsButton>("Help");
	help_button_->setToggleState(false, NotificationType::dontSendNotification);
	help_button_->setImage(Drawable::createFromImageData(BinaryData::question_svg, BinaryData::question_svgSize).get());
	help_button_->addListener(this);    

	addAndMakeVisible(audio_button_.get());
	addAndMakeVisible(general_button_.get());
	addAndMakeVisible(help_button_.get());

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

	juce::Rectangle<int> pane_rect(SEPARATOR_X + 1, 1, getWidth() - SEPARATOR_X + 1, getHeight() - 1);
	audio_settings_pane_.setBounds(pane_rect);
	general_settings_pane_.setBounds(pane_rect);
	help_settings_pane_.setBounds(pane_rect);
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
	prioritize_new_output_toggle_(TRANS("Prioritize new output devices")),
	reset_presets_button_(TRANS("Reset presets to factory defaults"))
{
	setFocusContainer(true);

	output_preference_title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	output_preference_title_.setJustificationType(Justification::centredLeft);

	output_preference_.setMouseCursor(MouseCursor::PointingHandCursor);
	output_preference_.setWantsKeyboardFocus(true);
	output_preference_.setEnabled(true);

	prioritize_new_output_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	prioritize_new_output_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	prioritize_new_output_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	prioritize_new_output_toggle_.setWantsKeyboardFocus(true);

	reset_presets_button_.setSize(RESET_PRESETS_BUTTON_WIDTH, BUTTON_HEIGHT);
	reset_presets_button_.setMouseCursor(MouseCursor::PointingHandCursor);

	prioritize_new_output_toggle_.setToggleState(FxController::getInstance().isNewOutputPrioritized(), NotificationType::dontSendNotification);
	prioritize_new_output_toggle_.onClick = [this]() { FxController::getInstance().setNewOutputPrioritized(prioritize_new_output_toggle_.getToggleState()); };

	auto preset_modified = false;
	auto preset_count = FxModel::getModel().getPresetCount();
	for (auto i = 0; i < preset_count; i++)
	{
		if (FxModel::getModel().isPresetModified(i))
		{
			preset_modified = true;
			break;
		}
	}
	reset_presets_button_.setEnabled(FxModel::getModel().getUserPresetCount() > 0 || preset_modified);

	reset_presets_button_.onClick = [this]() {
		auto& controller = FxController::getInstance();

		controller.resetPresets();
		reset_presets_button_.setEnabled(false);
		};

	setText();

	addAndMakeVisible(&output_preference_title_);
	addAndMakeVisible(&output_preference_);
	addAndMakeVisible(&prioritize_new_output_toggle_);
	addAndMakeVisible(&reset_presets_button_);
}

FxSettingsDialog::AudioSettingsPane::~AudioSettingsPane()
{
}

void FxSettingsDialog::AudioSettingsPane::resized()
{
	auto bounds = getLocalBounds().withLeft(X_MARGIN).withTop(Y_MARGIN).withHeight(TITLE_HEIGHT);
	title_.setBounds(bounds);

	output_preference_title_.setBounds(X_MARGIN, ENDPOINT_Y, LABEL_WIDTH, LABEL_HEIGHT);
	int y = output_preference_title_.getBottom() + 10;
	auto width = getWidth() - ((X_MARGIN + 5) * 2);
	output_preference_.setBounds(X_MARGIN, y, width, OUTPUT_PREFERENCE_HEIGHT);

    y = output_preference_.getBottom() + 10;
    prioritize_new_output_toggle_.setBounds(X_MARGIN, y, width, TOGGLE_BUTTON_HEIGHT);

	auto group_x = output_preference_title_.getX() - GROUP_MARGIN;
	auto group_y = output_preference_title_.getY() - GROUP_MARGIN;
	auto group_width = output_preference_.getRight() - group_x + GROUP_MARGIN;
	auto group_height = prioritize_new_output_toggle_.getBottom() - group_y + GROUP_MARGIN;
	output_preference_bounds_ = juce::Rectangle<float>(group_x, group_y, group_width, group_height);

	y = prioritize_new_output_toggle_.getBottom() + 30;
	resizeResetButton(X_MARGIN, y);
}

void FxSettingsDialog::AudioSettingsPane::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

	g.setFillType(FillType(Colour(FXCOLOR(DefaultFill)).withAlpha(0.2f)));
	g.fillRoundedRectangle(output_preference_bounds_, 8);

	setText();

	SettingsPane::paint(g);
}

void FxSettingsDialog::AudioSettingsPane::setText()
{
	auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

	output_preference_title_.setFont(theme.getNormalFont());
	output_preference_title_.setText(TRANS("Output Device Preference"), NotificationType::dontSendNotification);

	prioritize_new_output_toggle_.setButtonText(TRANS("Prioritize new output devices"));

	reset_presets_button_.setButtonText(TRANS("Reset presets to factory defaults"));
	resizeResetButton(reset_presets_button_.getX(), reset_presets_button_.getY());
}

void FxSettingsDialog::AudioSettingsPane::resizeResetButton(int x, int y)
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
	if (buttonWidth < RESET_PRESETS_BUTTON_WIDTH)
	{
		buttonWidth = RESET_PRESETS_BUTTON_WIDTH;
	}

	reset_presets_button_.setBounds(x, y, buttonWidth, BUTTON_HEIGHT * lineCount);
}

void FxSettingsDialog::AudioSettingsPane::visibilityChanged()
{
	if (isVisible())
	{
		output_preference_.update();
    }
}

void FxSettingsDialog::AudioSettingsPane::mouseEnter(const MouseEvent& mouse_event)
{
	Component::mouseEnter(mouse_event);
}

void FxSettingsDialog::AudioSettingsPane::mouseExit(const MouseEvent& mouse_event)
{
	Component::mouseEnter(mouse_event);
}

FxSettingsDialog::GeneralSettingsPane::GeneralSettingsPane() :
	SettingsPane("General Preferences"),
	launch_toggle_(TRANS("Launch on system startup")),
	hide_help_tips_toggle_(TRANS("Hide help tips for audio controls")),
	notification_mode_label_(TRANS("Notifications"), ""),
	notification_default_label_(TRANS("When in fullscreen"), ""),
	notification_exceptions_label_(TRANS("App exceptions"), ""),
	notification_add_button_(TRANS("+ Add"), ""),
	notification_remove_button_(TRANS("Remove"), ""),
	hotkeys_toggle_(TRANS("Disable keyboard shortcuts"))
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

	notification_mode_label_.setColour(Label::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	notification_mode_box_.addItemList({ TRANS("Follow system settings"), TRANS("Always show"), TRANS("Custom rules") }, 1);
	notification_mode_box_.onChange = [this]() {
		FxController::getInstance().setNotificationMode(static_cast<NotificationMode>(notification_mode_box_.getSelectedId() - 1));
		updateNotificationControlsVisibility();
	};

	notification_default_label_.setColour(Label::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	notification_default_box_.addItemList({ TRANS("Show notifications"), TRANS("Hide notifications") }, 1);
	notification_default_box_.onChange = [this]() {
		FxController::getInstance().setNotificationDefault(notification_default_box_.getSelectedId() == 2);
	};

	notification_exceptions_label_.setColour(Label::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));

	notification_exceptions_ = FxController::getInstance().getNotificationExceptions();
	notification_exceptions_list_.setModel(this);
	notification_exceptions_list_.setColour(ListBox::backgroundColourId, getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
	notification_exceptions_list_.setColour(ListBox::outlineColourId, getLookAndFeel().findColour(TextButton::textColourOffId).withAlpha(0.3f));

	// Populate process selector with running processes
	DWORD processIds[1024];
	DWORD cbNeeded;
	if (EnumProcesses(processIds, sizeof(processIds), &cbNeeded))
	{
		int numProcesses = cbNeeded / sizeof(DWORD);
		for (int i = 0; i < numProcesses; i++)
		{
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processIds[i]);
			if (hProcess)
			{
				WCHAR path[MAX_PATH] = L"";
				if (GetModuleFileNameExW(hProcess, NULL, path, MAX_PATH))
				{
					String processName = File(path).getFileName();
					bool alreadyAdded = false;
					for (int j = 1; j <= notification_process_selector_.getNumItems(); j++)
					{
						if (notification_process_selector_.getItemText(j) == processName)
						{
							alreadyAdded = true;
							break;
						}
					}
					if (processName.isNotEmpty() && !alreadyAdded)
						notification_process_selector_.addItem(processName, notification_process_selector_.getNumItems() + 1);
				}
				CloseHandle(hProcess);
			}
		}
	}
	notification_process_selector_.setTextWhenNoChoicesAvailable(TRANS("No processes"));

	notification_add_button_.onClick = [this]() {
		String selected = notification_process_selector_.getText();
		if (selected.isNotEmpty() && notification_exceptions_.indexOf(selected, true) < 0)
		{
			notification_exceptions_.add(selected);
			FxController::getInstance().setNotificationExceptions(notification_exceptions_);
			notification_exceptions_list_.updateContent();
		}
	};

	notification_remove_button_.onClick = [this]() {
		int row = notification_exceptions_list_.getSelectedRow();
		if (row >= 0 && row < notification_exceptions_.size())
		{
			notification_exceptions_.remove(row);
			FxController::getInstance().setNotificationExceptions(notification_exceptions_);
			notification_exceptions_list_.updateContent();
		}
	};

	// Load current settings
	NotificationMode mode = FxController::getInstance().getNotificationMode();
	notification_mode_box_.setSelectedId(static_cast<int>(mode) + 1, NotificationType::dontSendNotification);
	notification_default_box_.setSelectedId(FxController::getInstance().getNotificationDefault() ? 2 : 1, NotificationType::dontSendNotification);

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

	auto os = SystemStats::getOperatingSystemType();
	if (os == SystemStats::OperatingSystemType::Windows7)
	{
		addAndMakeVisible(&launch_toggle_);
	}
	
	addAndMakeVisible(&hide_help_tips_toggle_);
	addAndMakeVisible(&notification_mode_label_);
	addAndMakeVisible(&notification_mode_box_);
	addAndMakeVisible(&notification_default_label_);
	addAndMakeVisible(&notification_default_box_);
	addAndMakeVisible(&notification_exceptions_label_);
	addAndMakeVisible(&notification_exceptions_list_);
	addAndMakeVisible(&notification_add_button_);
	addAndMakeVisible(&notification_remove_button_);
	addAndMakeVisible(&notification_process_selector_);
	addAndMakeVisible(&hotkeys_toggle_);
	addAndMakeVisible(&language_switch_);

	updateNotificationControlsVisibility();
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
	y = hide_help_tips_toggle_.getBottom() + 15;

	notification_mode_label_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN, 20);
	y += 20;
	notification_mode_box_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN, TOGGLE_BUTTON_HEIGHT);
	y = notification_mode_box_.getBottom() + 10;

	if (notification_mode_box_.getSelectedId() == 3) // CustomRules
	{
		notification_default_label_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN, 20);
		y += 20;
		notification_default_box_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN, TOGGLE_BUTTON_HEIGHT);
		y = notification_default_box_.getBottom() + 10;

		notification_exceptions_label_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN, 20);
		y += 20;

		int listHeight = 80;
		notification_exceptions_list_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN * 2 - 100, listHeight);

		int buttonX = notification_exceptions_list_.getRight() + 5;
		notification_remove_button_.setBounds(buttonX, y, 95, 24);
		y += listHeight + 5;

		notification_process_selector_.setBounds(X_MARGIN, y, getWidth() - X_MARGIN - 100, TOGGLE_BUTTON_HEIGHT);
		notification_add_button_.setBounds(notification_process_selector_.getRight() + 5, y, 95, TOGGLE_BUTTON_HEIGHT);
		y = notification_add_button_.getBottom() + 10;
	}
	else
	{
		y = notification_mode_box_.getBottom() + 5;
	}

	hotkeys_toggle_.setBounds(X_MARGIN, y, getWidth()-X_MARGIN, TOGGLE_BUTTON_HEIGHT);

	y = hotkeys_toggle_.getBottom() + 5;
	for (auto hotkey_label : hotkey_labels_)
	{
		hotkey_label->setBounds(HOTKEY_LABEL_X, y, getWidth()-HOTKEY_LABEL_X, HOTKEY_LABEL_HEIGHT);
		y += HOTKEY_LABEL_HEIGHT + 10;
	}
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

	notification_mode_label_.setText(TRANS("Notifications"), NotificationType::dontSendNotification);
	notification_default_label_.setText(TRANS("When in fullscreen"), NotificationType::dontSendNotification);
	notification_exceptions_label_.setText(TRANS("App exceptions"), NotificationType::dontSendNotification);
	notification_add_button_.setButtonText(TRANS("+ Add"));
	notification_remove_button_.setButtonText(TRANS("Remove"));

    hotkeys_toggle_.setButtonText(TRANS("Disable keyboard shortcuts"));
}

void FxSettingsDialog::GeneralSettingsPane::updateNotificationControlsVisibility()
{
	bool isCustomRules = (notification_mode_box_.getSelectedId() == 3);
	notification_default_label_.setVisible(isCustomRules);
	notification_default_box_.setVisible(isCustomRules);
	notification_exceptions_label_.setVisible(isCustomRules);
	notification_exceptions_list_.setVisible(isCustomRules);
	notification_add_button_.setVisible(isCustomRules);
	notification_remove_button_.setVisible(isCustomRules);
	notification_process_selector_.setVisible(isCustomRules);
	resized();
}

int FxSettingsDialog::GeneralSettingsPane::getNumRows()
{
	return notification_exceptions_.size();
}

void FxSettingsDialog::GeneralSettingsPane::paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
{
	if (rowNumber < 0 || rowNumber >= notification_exceptions_.size())
		return;

	g.fillAll(rowIsSelected ? findColour(TextEditor::highlightColourId) : findColour(ListBox::backgroundColourId));
	g.setColour(findColour(ListBox::textColourId));
	g.setFont(height * 0.7f);
	g.drawText(notification_exceptions_[rowNumber], 10, 0, width - 20, height, Justification::centredLeft, true);
}

FxSettingsDialog::HelpSettingsPane::HelpSettingsPane() : SettingsPane("Help"), auto_updates_toggle_(TRANS("Automatic updates"))
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

	auto_updates_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	auto_updates_toggle_.setToggleState(FxController::getInstance().getAutoUpdates(), NotificationType::dontSendNotification);
	auto_updates_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	auto_updates_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));

	auto_updates_toggle_.onClick = [this]() {
		FxController::getInstance().setAutoUpdates(auto_updates_toggle_.getToggleState());
	};

    setText();

	addAndMakeVisible(version_title_);
	addAndMakeVisible(version_text_);
	addAndMakeVisible(support_title_);
	addAndMakeVisible(maintenance_title_);
	addAndMakeVisible(changelog_link_);
	addChildComponent(quicktour_link_);
	addChildComponent(submitlogs_link_);
	addAndMakeVisible(helpcenter_link_);
	addAndMakeVisible(auto_updates_toggle_);
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
	auto_updates_toggle_.setBounds(X_MARGIN + 5, maintenance_title_.getBottom() + 10, BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT);
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
	auto_updates_toggle_.setButtonText(TRANS("Automatic updates"));;
}
