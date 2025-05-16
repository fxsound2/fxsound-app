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

#ifndef FXPRESETEXPORTDIALOG_H
#define FXPRESETEXPORTDIALOG_H

#include <JuceHeader.h>
#include "FxWindow.h"
#include "FxTheme.h"
#include "FxController.h"
#include "FxMessage.h"

//==============================================================================
/*
*/
class FxPresetExportDialog : public FxWindow
{
public:
    FxPresetExportDialog();
    ~FxPresetExportDialog() = default;

    bool keyPressed(const KeyPress& key) override;

    void closeButtonPressed() override;

private:
    class PresetExportProgress : public AnimatedAppComponent
    {
    public:
        PresetExportProgress();

        void update() override;
        void paint(Graphics& g) override;

    private:
        float colour_gradient_start_;
    };

    class PresetExportComponent : public Component, public ListBoxModel, public Button::Listener
    {
    public:
        PresetExportComponent();
        ~PresetExportComponent() = default;

        void resized() override;

        int getNumRows() override;
        void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;
        void selectedRowsChanged(int) override;

    private:
        static constexpr int WIDTH = 400;
        static constexpr int HEIGHT = 405;
        static constexpr int BUTTON_WIDTH = 80;
        static constexpr int BUTTON_HEIGHT = 30;
        static constexpr int TEXT_HEIGHT = 20;
        static constexpr int LIST_HEIGHT = 310;

        void buttonClicked(Button* button) override;

        Label select_presets_label_;
        ToggleButton select_all_presets_;
        ListBox preset_list_;
        PresetExportProgress preset_export_progress_;
        TextButton export_button_;
        Font font_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetExportComponent)
    };

    PresetExportComponent preset_export_content_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPresetExportDialog)
};

#endif
