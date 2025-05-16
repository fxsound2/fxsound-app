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

#include "FxMessage.h"
#include "FxModel.h"

FxMessage::FxMessage(String message, const std::pair<String, String>& link) : message_content_(message, link)
{
	setContent(&message_content_);
	centreWithSize(getWidth(), getHeight());
	addToDesktop(ComponentPeer::windowAppearsOnTaskbar);
	toFront(true);
	setAlwaysOnTop(true);
}

bool FxMessage::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::escapeKey)
	{
		exitModalState(0);
		removeFromDesktop();
		return true;
	}

	return Component::keyPressed(key);
}

void FxMessage::closeButtonPressed()
{
	exitModalState(0);
	removeFromDesktop();
}

void FxMessage::showMessage(String message, const std::pair<String, String>& link)
{
	FxMessage message_window(message, link);
	message_window.runModalLoop();
}

FxMessage::MessageComponent::MessageComponent(String message, const std::pair<String, String>& link)
{
	auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

	message_.setText(message, NotificationType::dontSendNotification);
	message_.setFont(theme.getSmallFont().withHeight(17.0f));
	message_.setJustificationType(Justification::centredTop);
	addAndMakeVisible(message_);

	if (link.first.isNotEmpty() && link.second.isNotEmpty())
	{
		link_.setButtonText(TRANS(link.first));
		link_.setURL(URL(link.second));
		link_.setJustificationType(Justification::centredTop);
		addAndMakeVisible(link_);
	}
	
	setSize(WIDTH, HEIGHT);
}

void FxMessage::MessageComponent::resized()
{
	auto bounds = getLocalBounds();
	bounds.setTop(10);
	bounds.reduce(20, 0);

	RectanglePlacement placement(RectanglePlacement::xMid
		| RectanglePlacement::yTop
		| RectanglePlacement::doNotResize);
	
	auto component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), MESSAGE_HEIGHT);
	message_.setBounds(placement.appliedTo(component_area, bounds));

	if (link_.getButtonText().isNotEmpty())
	{
		bounds.setTop(message_.getBottom() + 10);
		component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), HYPERLINK_HEIGHT);
		link_.setBounds(placement.appliedTo(component_area, bounds));
	}	
}