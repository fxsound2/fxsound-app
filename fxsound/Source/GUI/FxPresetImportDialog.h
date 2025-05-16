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

#ifndef FXPRESETIMPORTDIALOG_H
#define FXPRESETIMPORTDIALOG_H

#include <JuceHeader.h>
#include "FxWindow.h"
#include "FxTheme.h"
#include "FxController.h"
#include "FxMessage.h"

//==============================================================================
/*
*/
class FxPresetImportDialog : public FxWindow
{
public:
    FxPresetImportDialog();
    ~FxPresetImportDialog() = default;

    bool keyPressed(const KeyPress& key) override;

    void closeButtonPressed() override;

private:
    class PresetImportComponent : public Component, public Button::Listener
    {
    public:
        PresetImportComponent();
        ~PresetImportComponent() = default;

        void resized() override;

    private:
        static constexpr int WIDTH = 400;
        static constexpr int HEIGHT = 400;
        static constexpr int BUTTON_WIDTH = 80;
        static constexpr int BUTTON_HEIGHT = 30;
        static constexpr int TEXT_HEIGHT = 20;
        static constexpr int DIR_SELECTION_HEIGHT = 310;

        void buttonClicked(Button* button) override;

        Label select_dir_label_;
        FileBrowserComponent import_dir_select_;
        TextButton import_button_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetImportComponent)
    };

    PresetImportComponent preset_import_content_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPresetImportDialog)
};

#endif
