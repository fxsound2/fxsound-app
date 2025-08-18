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

#ifndef FXPRESETSAVEDIALOG_H
#define FXPRESETSAVEDIALOG_H

#include <JuceHeader.h>
#include "FxWindow.h"
#include "FxTheme.h"
#include "FxController.h"
#include "FxMessage.h"
#include "FxPresetNameEditor.h"

//==============================================================================
/*
*/
class FxPresetSaveDialog : public FxWindow
{
public:
    FxPresetSaveDialog();
    ~FxPresetSaveDialog() = default;

    bool keyPressed(const KeyPress& key) override;

    void closeButtonPressed() override;

private:
    class PresetSaveComponent : public Component
    {
    public:
        PresetSaveComponent();
        ~PresetSaveComponent() = default;

    private:
        static constexpr int WIDTH = 450;
        static constexpr int HEIGHT = 142;

        static constexpr int MESSAGE_HEIGHT = 40;
        static constexpr int BUTTON_WIDTH = 120;
        static constexpr int BUTTON_HEIGHT = 30;

        void resized() override;

        Label message_;
        FxPresetNameEditor preset_name_editor_;
        TextButton save_button_;
        TextButton cancel_button_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetSaveComponent);
    };
    
    PresetSaveComponent preset_save_component_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPresetSaveDialog)
};

#endif
