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
#include "FxHelpSettingsPane.h"
#include "FxController.h"
#include "FxTheme.h"

//==============================================================================
FxHelpSettingsPane::FxHelpSettingsPane() : FxSettingsPane("Help"), auto_updates_toggle_(TRANS("Automatic updates"))
{
	version_title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	version_title_.setJustificationType(Justification::centredLeft);
	version_text_.setJustificationType(Justification::centredLeft);
	support_title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	support_title_.setJustificationType(Justification::centredLeft);
	maintenance_title_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	maintenance_title_.setJustificationType(Justification::centredLeft);

	changelog_link_.setURL(URL(L"https://www.fxsound.com/changelog"));
	changelog_link_.setJustificationType(Justification::topLeft);
	quicktour_link_.setJustificationType(Justification::topLeft);
	submitlogs_link_.setJustificationType(Justification::topLeft);
	helpcenter_link_.setURL(URL(L"https://www.fxsound.com/learning-center"));
	helpcenter_link_.setJustificationType(Justification::topLeft);
    feedback_link_.setURL(URL("https://james722808.typeform.com/to/QfEP5QrP"));
	feedback_link_.setJustificationType(Justification::topLeft);

	auto_updates_toggle_.setMouseCursor(MouseCursor::PointingHandCursor);
	auto_updates_toggle_.setToggleState(FxController::getInstance().getAutoUpdates(), NotificationType::dontSendNotification);
	auto_updates_toggle_.setColour(ToggleButton::ColourIds::tickColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
	auto_updates_toggle_.setColour(ToggleButton::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));

	auto_updates_toggle_.onClick = [this]() {
		FxController::getInstance().setAutoUpdates(auto_updates_toggle_.getToggleState());
	};

    setText();

	addAndMakeVisible(version_title_);
	addAndMakeVisible(version_text_);
	addAndMakeVisible(support_title_);
	addAndMakeVisible(maintenance_title_);
	addAndMakeVisible(changelog_link_);
	addChildComponent(quicktour_link_);
	addChildComponent(submitlogs_link_);
	addAndMakeVisible(helpcenter_link_);
	addAndMakeVisible(auto_updates_toggle_);
}

void FxHelpSettingsPane::resized()
{
	auto bounds = getLocalBounds().withLeft(X_MARGIN).withTop(Y_MARGIN).withHeight(TITLE_HEIGHT);
	title_.setBounds(bounds);

	version_title_.setBounds(X_MARGIN, TEXT_Y, getWidth()-X_MARGIN, TITLE_HEIGHT);
	version_text_.setBounds(X_MARGIN, version_title_.getBottom()+10, getWidth()-X_MARGIN, TEXT_HEIGHT);
	changelog_link_.setBounds(X_MARGIN+5, version_text_.getBottom()+10, getWidth()-X_MARGIN, HYPERLINK_HEIGHT);
	support_title_.setBounds(X_MARGIN, changelog_link_.getBottom()+20, getWidth()-X_MARGIN, TITLE_HEIGHT);
	helpcenter_link_.setBounds(X_MARGIN+5, support_title_.getBottom()+10, getWidth()-X_MARGIN, HYPERLINK_HEIGHT);
	maintenance_title_.setBounds(X_MARGIN, helpcenter_link_.getBottom()+20, getWidth()-X_MARGIN, TITLE_HEIGHT);
	auto_updates_toggle_.setBounds(X_MARGIN + 5, maintenance_title_.getBottom() + 10, BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT);
}

void FxHelpSettingsPane::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    setText();

	FxSettingsPane::paint(g);
}

void FxHelpSettingsPane::setText()
{
    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

    version_title_.setText(TRANS("Version"), NotificationType::dontSendNotification);
    version_title_.setFont(theme.getNormalFont());

    version_text_.setText(L"v" + JUCEApplication::getInstance()->getApplicationVersion(), NotificationType::dontSendNotification);
    version_text_.setFont(theme.getSmallFont());

    support_title_.setText(TRANS("Support"), NotificationType::dontSendNotification);
    support_title_.setFont(theme.getNormalFont());

    maintenance_title_.setText(TRANS("Maintenance"), NotificationType::dontSendNotification);
    maintenance_title_.setFont(theme.getNormalFont());

    changelog_link_.setButtonText(TRANS("Changelog"));
    quicktour_link_.setButtonText(TRANS("Quick tour"));
    submitlogs_link_.setButtonText(TRANS("Submit debug logs"));
    helpcenter_link_.setButtonText(TRANS("Help center"));
    feedback_link_.setButtonText(TRANS("Feedback"));
	auto_updates_toggle_.setButtonText(TRANS("Automatic updates"));
}
