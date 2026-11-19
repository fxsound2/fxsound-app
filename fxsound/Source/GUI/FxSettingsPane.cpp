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
#include "FxSettingsPane.h"
#include "FxTheme.h"

//==============================================================================
FxSettingsPane::FxSettingsPane(String name)
{
	name_ = name;

	title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	title_.setJustificationType(Justification::centredLeft);
	updateTitle();
	addAndMakeVisible(title_);
}

void FxSettingsPane::lookAndFeelChanged()
{
	updateTitle();
	setText();
	repaint();
}

FxTheme* FxSettingsPane::getTheme()
{
	return dynamic_cast<FxTheme*>(&LookAndFeel::getDefaultLookAndFeel());
}

void FxSettingsPane::updateTitle()
{
	if (auto* theme = getTheme())
	{
		title_.setFont(theme->getTitleFont());
	}
	title_.setText(TRANS(name_), NotificationType::dontSendNotification);
}
