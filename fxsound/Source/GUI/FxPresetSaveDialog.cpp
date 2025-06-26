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
#include "FxPresetSaveDialog.h"

FxPresetSaveDialog::FxPresetSaveDialog() : FxWindow(TRANS("Save Preset"))
{
    setContent(&preset_save_component_);

    centreWithSize(getWidth(), getHeight());
    addToDesktop(0);

    setAlwaysOnTop(true);
    toFront(true);
}

bool FxPresetSaveDialog::keyPressed(const KeyPress& key)
{
    if (key == KeyPress::escapeKey)
    {
        exitModalState(0);
        removeFromDesktop();
        return true;
    }

    return Component::keyPressed(key);
}

void FxPresetSaveDialog::closeButtonPressed()
{
    exitModalState(0);
    removeFromDesktop();
}

FxPresetSaveDialog::PresetSaveComponent::PresetSaveComponent() : save_button_(TRANS("Save")), cancel_button_(TRANS("Cancel"))
{
    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

    message_.setText(TRANS("Changes to your preset are not saved.\r\nDo you want to save?"), NotificationType::dontSendNotification);
    message_.setFont(theme.getSmallFont().withHeight(17.0f));
    message_.setJustificationType(Justification::centredTop);
    addAndMakeVisible(message_);

    addAndMakeVisible(preset_name_editor_);
    preset_name_editor_.setWantsKeyboardFocus(true);

    save_button_.setMouseCursor(MouseCursor::PointingHandCursor);
    save_button_.setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    addAndMakeVisible(save_button_);
    save_button_.onClick = [this]() {
        if (preset_name_editor_.getStatus() == FxPresetNameEditor::Status::Valid)
        {
            auto preset_name = preset_name_editor_.getPresetName();
            FxController::getInstance().savePreset(preset_name);

            getParentComponent()->exitModalState(0);
            getParentComponent()->removeFromDesktop();
        }
        };

    cancel_button_.setMouseCursor(MouseCursor::PointingHandCursor);
    cancel_button_.setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    addAndMakeVisible(cancel_button_);
    cancel_button_.onClick = [this]() {
        getParentComponent()->exitModalState(0);
        getParentComponent()->removeFromDesktop();
        };

    setSize(WIDTH, HEIGHT);
}


void FxPresetSaveDialog::PresetSaveComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.setTop(10);
    bounds.reduce(20, 0);

    RectanglePlacement placement(RectanglePlacement::xMid
        | RectanglePlacement::yTop
        | RectanglePlacement::doNotResize);

    auto component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), MESSAGE_HEIGHT);
    message_.setBounds(placement.appliedTo(component_area, bounds));

    auto editor_rect = preset_name_editor_.getLocalBounds();
    component_area = juce::Rectangle<int>((WIDTH - editor_rect.getWidth()) /2, message_.getBottom() + 10, editor_rect.getWidth(), editor_rect.getHeight());
    preset_name_editor_.setBounds(component_area);

    component_area = juce::Rectangle<int>((WIDTH - ((BUTTON_WIDTH * 2) + 20)) / 2, preset_name_editor_.getBottom() + 20, BUTTON_WIDTH, BUTTON_HEIGHT);
    save_button_.setBounds(component_area);

    component_area = juce::Rectangle<int>(save_button_.getRight() + 20, preset_name_editor_.getBottom() + 20, BUTTON_WIDTH, BUTTON_HEIGHT);
    cancel_button_.setBounds(component_area);
}