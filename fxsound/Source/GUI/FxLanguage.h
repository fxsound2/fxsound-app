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

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class FxLanguage : public Component
{
public:
    static constexpr int WIDTH = 180;
    static constexpr int HEIGHT = 30;

    FxLanguage();
    ~FxLanguage() = default;

private:
    static constexpr int BUTTON_WIDTH = 14;
    static constexpr int BUTTON_HEIGHT = 22;
    static constexpr int LABEL_HEIGHT = 22;

    void paint(Graphics& g) override;

    void onNextLanguage();
    void onPrevLanguage();
    
    Label language_;
    DrawableButton next_button_;
    DrawableButton prev_button_;

    StringArray languages_;
    int language_index_;
};