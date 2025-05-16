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

#include "FxPresetExportDialog.h"

FxPresetExportDialog::FxPresetExportDialog() : FxWindow("Export Presets")
{
    setContent(&preset_export_content_);

    centreWithSize(getWidth(), getHeight());
    addToDesktop(0);
    toFront(true);
}

bool FxPresetExportDialog::keyPressed(const KeyPress& key)
{
    if (key == KeyPress::escapeKey)
    {
        exitModalState(0);
        removeFromDesktop();
        return true;
    }

    return Component::keyPressed(key);
}

void FxPresetExportDialog::closeButtonPressed()
{
    exitModalState(0);
    removeFromDesktop();
}

FxPresetExportDialog::PresetExportProgress::PresetExportProgress()
{
    setFramesPerSecond(30);
    colour_gradient_start_ = 0.0;
}

void FxPresetExportDialog::PresetExportProgress::update()
{
    colour_gradient_start_ += 0.01f;
}

void FxPresetExportDialog::PresetExportProgress::paint(Graphics& g)
{
    if (colour_gradient_start_ >= 1.0)
    {
        colour_gradient_start_ = 0.0;
    }

    ColourGradient gradient = ColourGradient::horizontal(Colour(0xe63462).withAlpha(1.0f), colour_gradient_start_* (float)getWidth(), Colour(0xf3f3f3).withAlpha(1.0f), getWidth());

    g.setFillType(FillType(gradient));
    auto area = juce::Rectangle<float>(0, 0, getWidth(), getHeight());
    g.fillRoundedRectangle(area, getHeight() / 2);
}

FxPresetExportDialog::PresetExportComponent::PresetExportComponent() : export_button_(TRANS("Export"))
{
    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

    select_presets_label_.setText(TRANS("Select the presets to export..."), NotificationType::dontSendNotification);
    select_presets_label_.setFont(theme.getNormalFont());
    select_presets_label_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
    select_presets_label_.setJustificationType(Justification::centredLeft);

    preset_list_.setColour(ListBox::ColourIds::backgroundColourId, Colour(0x000000).withAlpha(1.0f));
    preset_list_.setColour(ListBox::ColourIds::outlineColourId, Colour(0x000000).withAlpha(1.0f));
    preset_list_.setColour(ListBox::ColourIds::textColourId, Colour(0xb1b1b1).withAlpha(1.0f));
    preset_list_.setRowHeight(TEXT_HEIGHT + 6);
    preset_list_.setModel(this);
    preset_list_.setMultipleSelectionEnabled(true);
    preset_list_.setClickingTogglesRowSelection(true);

    export_button_.setMouseCursor(MouseCursor::PointingHandCursor);
    export_button_.setEnabled(false);
    export_button_.setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    export_button_.addListener(this);

    preset_export_progress_.setSize(WIDTH, 2);
    
    font_ = theme.getNormalFont();

    addAndMakeVisible(select_presets_label_);
    addAndMakeVisible(export_button_);
    addChildComponent(preset_export_progress_);
    addAndMakeVisible(preset_list_);

    setSize(WIDTH, HEIGHT);
}

void FxPresetExportDialog::PresetExportComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.setTop(10);
    bounds.reduce(20, 0);

    RectanglePlacement placement(RectanglePlacement::xLeft
                                | RectanglePlacement::yTop
                                | RectanglePlacement::doNotResize);
    auto component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), TEXT_HEIGHT);
    select_presets_label_.setBounds(placement.appliedTo(component_area, bounds));

    bounds.setTop(select_presets_label_.getBottom() + 10);

    component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), LIST_HEIGHT);
    preset_list_.setBounds(placement.appliedTo(component_area, bounds));

    bounds.setTop(preset_list_.getBottom() + 10);

    preset_export_progress_.setBounds(0, bounds.getY(), WIDTH, 2);

    bounds.setTop(preset_export_progress_.getBottom() + 10);
    
    component_area = juce::Rectangle<int>(0, 0, export_button_.getWidth(), export_button_.getHeight());
    placement = RectanglePlacement(RectanglePlacement::xRight
                                  | RectanglePlacement::yTop
                                  | RectanglePlacement::doNotResize);
    export_button_.setBounds(placement.appliedTo(component_area, bounds));
}

int FxPresetExportDialog::PresetExportComponent::getNumRows()
{
    return FxModel::getModel().getPresetCount();
}

void FxPresetExportDialog::PresetExportComponent::paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.fillAll(Colour(0xe63462).withAlpha(1.0f));
    }

    auto preset = FxModel::getModel().getPreset(rowNumber);
    auto area = juce::Rectangle<float>(0, 0, width, height);
    area.reduce(10, 0);
    g.setColour(Colour(0xffffff).withAlpha(1.0f));
    g.setFont(font_);
    g.drawText(preset.name, area, Justification::centredLeft, true);
}

void FxPresetExportDialog::PresetExportComponent::selectedRowsChanged(int)
{
    if (preset_list_.getNumSelectedRows() > 0)
    {
        export_button_.setEnabled(true);
    }
    else
    {
        export_button_.setEnabled(false);
    }
}

void FxPresetExportDialog::PresetExportComponent::buttonClicked(Button* button)
{
    if (button == &export_button_)
    {
        bool show_explorer = false;
        export_button_.setEnabled(false);

        auto selected_presets = preset_list_.getSelectedRows();
        if (selected_presets.size() > 0)
        {
            preset_export_progress_.setVisible(true);
            Component::getParentComponent()->toFront(true);

            Array<FxModel::Preset> presets;

            for (auto i=0; i<selected_presets.size(); i++)
            {
                presets.add(FxModel::getModel().getPreset(selected_presets[i]));
            }

            show_explorer = FxController::getInstance().exportPresets(presets);
        }
        
        if (show_explorer)
        {
            FxConfirmationMessage::showMessage(TRANS("Presets are exported successfully!"), FxConfirmationMessage::Style::OK);
            File(File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getFullPathName()) + L"FxSound\\Presets\\Export\\").revealToUser();
        }

        Component::getParentComponent()->exitModalState(0);
        Component::getParentComponent()->removeFromDesktop();
    }
}