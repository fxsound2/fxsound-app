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

#include "FxPresetImportDialog.h"

class FxImportCompleteMessage : public FxWindow
{
public:
    FxImportCompleteMessage(const StringArray& imported_presets, const StringArray& skipped_presets)
        : message_content_(imported_presets, skipped_presets)
    {
        setContent(&message_content_);
        centreWithSize(getWidth(), getHeight());
        addToDesktop(0);
        setAlwaysOnTop(true);
    }
    ~FxImportCompleteMessage() = default;

    void closeButtonPressed() override
    {
        exitModalState(0);
        removeFromDesktop();
    }

private:
    class MessageComponent : public Component, public Button::Listener
    {
    public:
        MessageComponent(const StringArray& imported_presets, const StringArray& skipped_presets) : ok_button_(TRANS("OK"))
        {
            auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

            imported_presets_label_.setText(TRANS("Presets successfully imported"), NotificationType::dontSendNotification);
            imported_presets_label_.setFont(theme.getNormalFont());
            imported_presets_label_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
            imported_presets_label_.setJustificationType(Justification::topLeft);
            addAndMakeVisible(imported_presets_label_);

            imported_presets_text_.setMultiLine(true);
            imported_presets_text_.setReadOnly(true);
            imported_presets_text_.setCaretVisible(false);
            imported_presets_text_.setScrollbarsShown(true);
            imported_presets_text_.setScrollBarThickness(10);
            addAndMakeVisible(imported_presets_text_);
            for (auto i = 0; i < imported_presets.size(); i++)
            {
                auto text = imported_presets[i];
                if (i != imported_presets.size())
                {
                    text += "\n";
                }
                imported_presets_text_.setText(imported_presets_text_.getText() + text);
            }

            skipped_presets_label_.setText(TRANS("Duplicate presets not imported"), NotificationType::dontSendNotification);
            skipped_presets_label_.setFont(theme.getNormalFont());
            skipped_presets_label_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
            skipped_presets_label_.setJustificationType(Justification::topLeft);
            addAndMakeVisible(skipped_presets_label_);

            skipped_presets_text_.setMultiLine(true);
            skipped_presets_text_.setReadOnly(true);
            skipped_presets_text_.setCaretVisible(false);
            skipped_presets_text_.setScrollbarsShown(true);
            skipped_presets_text_.setScrollBarThickness(10);
            addAndMakeVisible(skipped_presets_text_);
            for (auto i = 0; i < skipped_presets.size(); i++)
            {
                auto text = skipped_presets[i];
                if (i != skipped_presets.size())
                {
                    text += "\n";
                }
                skipped_presets_text_.setText(skipped_presets_text_.getText() + text);
            }

            ok_button_.setMouseCursor(MouseCursor::PointingHandCursor);            
            ok_button_.setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
            ok_button_.addListener(this);
            addAndMakeVisible(ok_button_);


            setSize(WIDTH, HEIGHT);
        }
        ~MessageComponent() = default;

    private:
        static constexpr int WIDTH = 350;
        static constexpr int HEIGHT = 340;
        static constexpr int BUTTON_WIDTH = 50;
        static constexpr int BUTTON_HEIGHT = 30;
        static constexpr int TEXT_HEIGHT = 20;
        static constexpr int LIST_HEIGHT = 100;

        void resized() override
        {
            auto bounds = getLocalBounds();
            bounds.setTop(10);
            bounds.reduce(20, 0);

            RectanglePlacement placement(RectanglePlacement::xLeft
                | RectanglePlacement::yTop
                | RectanglePlacement::doNotResize);

            auto component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), TEXT_HEIGHT);
            imported_presets_label_.setBounds(placement.appliedTo(component_area, bounds));

            bounds.setTop(imported_presets_label_.getBottom() + 10);

            component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), LIST_HEIGHT);
            imported_presets_text_.setBounds(placement.appliedTo(component_area, bounds));

            bounds.setTop(imported_presets_text_.getBottom() + 10);

            component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), TEXT_HEIGHT);
            skipped_presets_label_.setBounds(placement.appliedTo(component_area, bounds));

            bounds.setTop(skipped_presets_label_.getBottom() + 10);

            component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), LIST_HEIGHT);
            skipped_presets_text_.setBounds(placement.appliedTo(component_area, bounds));

            bounds.setTop(skipped_presets_text_.getBottom() + 20);

            component_area = juce::Rectangle<int>(0, 0, ok_button_.getWidth(), ok_button_.getHeight());
            placement = RectanglePlacement(RectanglePlacement::xMid
                | RectanglePlacement::yTop
                | RectanglePlacement::doNotResize);
            ok_button_.setBounds(placement.appliedTo(component_area, bounds));
        }

        void buttonClicked(Button* button) override
        {
            if (button == &ok_button_)
            {
                Component::getParentComponent()->exitModalState(0);
                Component::getParentComponent()->removeFromDesktop();
            }
        }

        Label imported_presets_label_;
        Label skipped_presets_label_;
        TextEditor imported_presets_text_;
        TextEditor skipped_presets_text_;
        TextButton ok_button_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MessageComponent)
    };

    MessageComponent message_content_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxImportCompleteMessage)
};

FxPresetImportDialog::FxPresetImportDialog() : FxWindow("Import Presets")
{
    setContent(&preset_import_content_);

    centreWithSize(getWidth(), getHeight());
    addToDesktop(0);
    toFront(true);
}

bool FxPresetImportDialog::keyPressed(const KeyPress& key)
{
    if (key == KeyPress::escapeKey)
    {
        exitModalState(0);
        removeFromDesktop();
        return true;
    }

    return Component::keyPressed(key);
}

void FxPresetImportDialog::closeButtonPressed()
{
    exitModalState(0);
    removeFromDesktop();
}

FxPresetImportDialog::PresetImportComponent::PresetImportComponent() : 
    import_dir_select_(FileBrowserComponent::FileChooserFlags::openMode | FileBrowserComponent::FileChooserFlags::canSelectDirectories, 
                       File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory), nullptr, nullptr),
    import_button_(TRANS("Import"))
{
    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

    select_dir_label_.setText(TRANS("Select the folder which contains the presets..."), NotificationType::dontSendNotification);
    select_dir_label_.setFont(theme.getNormalFont());
    select_dir_label_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));

    import_dir_select_.setColour(FileBrowserComponent::ColourIds::currentPathBoxBackgroundColourId, Colour(0x000000).withAlpha(1.0f));
    import_dir_select_.setColour(FileBrowserComponent::ColourIds::currentPathBoxTextColourId, Colour(0xb1b1b1).withAlpha(1.0f));
    import_dir_select_.setColour(FileBrowserComponent::ColourIds::filenameBoxBackgroundColourId, Colour(0x000000).withAlpha(1.0f));
    import_dir_select_.setColour(FileBrowserComponent::ColourIds::filenameBoxTextColourId, Colour(0xb1b1b1).withAlpha(1.0f));
    import_dir_select_.setFilenameBoxLabel(TRANS("Folder:"));

    select_dir_label_.setJustificationType(Justification::centredLeft);

    import_button_.setMouseCursor(MouseCursor::PointingHandCursor);
    import_button_.setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    import_button_.addListener(this);

    addAndMakeVisible(select_dir_label_);
    addAndMakeVisible(import_button_);
    addAndMakeVisible(import_dir_select_);

    setSize(WIDTH, HEIGHT);
}

void FxPresetImportDialog::PresetImportComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.setTop(10);
    bounds.reduce(20, 0);

    RectanglePlacement placement(RectanglePlacement::xLeft
                                | RectanglePlacement::yTop
                                | RectanglePlacement::doNotResize);
    auto component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), TEXT_HEIGHT);
    select_dir_label_.setBounds(placement.appliedTo(component_area, bounds));

    bounds.setTop(select_dir_label_.getBottom() + 10);

    component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), DIR_SELECTION_HEIGHT);
    import_dir_select_.setBounds(placement.appliedTo(component_area, bounds));

    bounds.setTop(import_dir_select_.getBottom() + 10);

    component_area = juce::Rectangle<int>(0, 0, import_button_.getWidth(), import_button_.getHeight());
    placement = RectanglePlacement(RectanglePlacement::xRight
                                  | RectanglePlacement::yTop
                                  | RectanglePlacement::doNotResize);
    import_button_.setBounds(placement.appliedTo(component_area, bounds));
}

void FxPresetImportDialog::PresetImportComponent::buttonClicked(Button* button)
{
    if (button == &import_button_)
    {
        auto import_path = import_dir_select_.getSelectedFile(0);
        FileSearchPath preset_search_path(import_path.getFullPathName());
        auto preset_paths = preset_search_path.findChildFiles(File::findFiles, false, "*.fac");
        if (preset_paths.isEmpty())
        {
            FxConfirmationMessage::showMessage(TRANS("Preset files not found in the selected folder."), FxConfirmationMessage::Style::OK);
            return;
        }
        StringArray imported_presets;
        StringArray skipped_presets;

        FxController::getInstance().importPresets(preset_paths, imported_presets, skipped_presets);

        Component::getParentComponent()->exitModalState(0);
        Component::getParentComponent()->removeFromDesktop();

        FxImportCompleteMessage import_complete_message(imported_presets, skipped_presets);
        import_complete_message.runModalLoop();
    }
}

