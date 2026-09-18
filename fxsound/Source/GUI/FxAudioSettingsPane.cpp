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
#include "FxAudioSettingsPane.h"
#include "FxController.h"
#include "FxModel.h"
#include "FxTheme.h"

//==============================================================================
FxAudioSettingsPane::FxAudioSettingsPane() :
	FxSettingsPane("Audio"),
	prioritize_new_output_toggle_(TRANS("Prioritize new output devices")),
	reset_presets_button_(TRANS("Reset presets to factory defaults"))
{
	setFocusContainerType(FocusContainerType::keyboardFocusContainer);

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

FxAudioSettingsPane::~FxAudioSettingsPane()
{
}

void FxAudioSettingsPane::resized()
{
	auto bounds = getLocalBounds().withLeft(X_MARGIN).withTop(Y_MARGIN).withHeight(TITLE_HEIGHT);
	title_.setBounds(bounds);

	output_preference_title_.setBounds(X_MARGIN, ENDPOINT_Y, LABEL_WIDTH, LABEL_HEIGHT);
	int y = output_preference_title_.getBottom() + 10;
	auto width = getWidth() - ((X_MARGIN + 5) * 2);
	output_preference_.setBounds(X_MARGIN, y, width, OUTPUT_PREFERENCE_HEIGHT);

    y = output_preference_.getBottom() + 10;
    prioritize_new_output_toggle_.setBounds(X_MARGIN, y, width, TOGGLE_BUTTON_HEIGHT);

	auto group_x = static_cast<float>(output_preference_title_.getX() - GROUP_MARGIN);
	auto group_y = static_cast<float>(output_preference_title_.getY() - GROUP_MARGIN);
	auto group_width = static_cast<float>(output_preference_.getRight() - group_x + GROUP_MARGIN);
	auto group_height = static_cast<float>(prioritize_new_output_toggle_.getBottom() - group_y + GROUP_MARGIN);
	output_preference_bounds_ = juce::Rectangle<float>(group_x, group_y, group_width, group_height);

	y = prioritize_new_output_toggle_.getBottom() + 30;
	resizeResetButton(X_MARGIN, y);
}

void FxAudioSettingsPane::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

	g.setFillType(FillType(Colour(FXCOLOR(DefaultFill)).withAlpha(0.2f)));
	g.fillRoundedRectangle(output_preference_bounds_, 8);

	setText();

	FxSettingsPane::paint(g);
}

void FxAudioSettingsPane::setText()
{
	auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

	output_preference_title_.setFont(theme.getNormalFont().withHeight(15.0f));
	output_preference_title_.setText(TRANS("Output Device Preference"), NotificationType::dontSendNotification);

	prioritize_new_output_toggle_.setButtonText(TRANS("Prioritize new output devices"));

	reset_presets_button_.setButtonText(TRANS("Reset presets to factory defaults"));
	resizeResetButton(reset_presets_button_.getX(), reset_presets_button_.getY());
}

void FxAudioSettingsPane::resizeResetButton(int x, int y)
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

	int buttonWidth = jmin(reset_presets_button_.getBestWidthForHeight(BUTTON_HEIGHT * lineCount), MAX_BUTTON_WIDTH);
	if (buttonWidth < RESET_PRESETS_BUTTON_WIDTH)
	{
		buttonWidth = RESET_PRESETS_BUTTON_WIDTH;
	}

	reset_presets_button_.setBounds(x, y, buttonWidth, BUTTON_HEIGHT * lineCount);
}

void FxAudioSettingsPane::visibilityChanged()
{
	if (isVisible())
	{
		output_preference_.update();
    }
}

void FxAudioSettingsPane::mouseEnter(const MouseEvent& mouse_event)
{
	Component::mouseEnter(mouse_event);
}

void FxAudioSettingsPane::mouseExit(const MouseEvent& mouse_event)
{
	Component::mouseExit(mouse_event);
}
