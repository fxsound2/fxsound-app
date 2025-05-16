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
#include "FxComboBox.h"

//==============================================================================
FxComboBox::FxComboBox()
{
    setMouseCursor(MouseCursor::PointingHandCursor);

    auto& theme = dynamic_cast<LookAndFeel_V4&>(getLookAndFeel());
    setColour(ComboBox::ColourIds::textColourId, theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::defaultText));

    error_ = false;
}

void FxComboBox::highlightText(bool highlight)
{
    auto& theme = dynamic_cast<LookAndFeel_V4&>(getLookAndFeel());

    if (!isItemEnabled(getSelectedId()))
    {
        setColour(ComboBox::ColourIds::textColourId, theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::defaultText));
        return;
    }
    
    if (highlight && isEnabled())
    {
        setColour(ComboBox::ColourIds::textColourId, theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::highlightedText));
    }
    else
    {
        setColour(ComboBox::ColourIds::textColourId, theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::defaultText));
    }
}

bool FxComboBox::getError()
{
    return error_;
}

void FxComboBox::setError(bool enable)
{
    if (enable)
    {
        error_ = true;
        setColour(ComboBox::ColourIds::outlineColourId, Colour(0xffe33250));
    }
    else
    {
        error_ = false;
        setColour(ComboBox::ColourIds::outlineColourId, Colour(0x000000).withAlpha(1.0f));
    }
}