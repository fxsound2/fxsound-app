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
#include "FxSettingsDialog.h"
#include "FxAudioSettingsPane.h"
#include "FxGeneralSettingsPane.h"
#include "FxHelpSettingsPane.h"
#include "FxTheme.h"

//==============================================================================
FxSettingsDialog::FxSettingsDialog() : FxWindow("Settings"), tooltip_window_(this)
{
	setContent(&settings_content_);
	centreWithSize(getWidth(), getHeight());
	addToDesktop(0);
	toFront(true);
}

void FxSettingsDialog::closeButtonPressed()
{
	exitModalState(0);
	removeFromDesktop();
}

void FxSettingsDialog::paint(Graphics& g)
{
	FxWindow::paint(g);

	g.setColour(Colour(FXCOLOR(Outline)).withAlpha(1.0f));
	g.drawLine((float)SEPARATOR_X, (float)title_bar_.getBottom(), (float)SEPARATOR_X, (float)getLocalBounds().getBottom());
}

void FxSettingsDialog::SettingsButton::paint(Graphics& g)
{
	auto bounds = getLocalBounds();

	if (getToggleState())
	{
		g.setColour(Colour(FXCOLOR(MenuHighlightBackground)).withAlpha(1.0f));
	}
	else
	{
		g.setColour(Colour(FXCOLOR(MenuBackground)).withAlpha(1.0f));
	}
	auto rect = juce::Rectangle<float>(0, 0, (float)bounds.getHeight(), (float)bounds.getHeight());
	g.fillRoundedRectangle(rect, (float)bounds.getHeight()/4);

	image_->drawWithin(g, rect.reduced(10, 10), RectanglePlacement::centred, 1.0f);

	if (getToggleState())
	{
		g.setColour(Colour(FXCOLOR(HighlightedText)).withAlpha(1.0f));
	}
	else
	{
		g.setColour(Colour(FXCOLOR(DefaultText)).withAlpha(1.0f));
	}
	auto w = bounds.getWidth() - bounds.getHeight() + 5;

	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
	g.setFont(theme.getNormalFont());
	g.drawText(TRANS(getName()), juce::Rectangle<int>(bounds.getHeight()+5, 0, w, bounds.getHeight()), Justification::centredLeft);
}

bool FxSettingsDialog::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::escapeKey)
	{
		exitModalState(0);
		removeFromDesktop();
		return true;
	}

	return Component::keyPressed(key);
}

FxSettingsDialog::SettingsComponent::SettingsComponent()
{
	addPane("Audio", BinaryData::speaker_svg, BinaryData::speaker_svgSize, std::make_unique<FxAudioSettingsPane>());
	addPane("General", BinaryData::settings_svg, BinaryData::settings_svgSize, std::make_unique<FxGeneralSettingsPane>());
	addPane("Help", BinaryData::question_svg, BinaryData::question_svgSize, std::make_unique<FxHelpSettingsPane>());

    setSize(WIDTH, HEIGHT);
}

void FxSettingsDialog::SettingsComponent::addPane(const String& name, const void* icon_data, int icon_data_size, std::unique_ptr<FxSettingsPane> pane)
{
	auto button = std::make_unique<SettingsButton>(name);
	button->setToggleState(panes_.empty(), NotificationType::dontSendNotification);
	button->setImage(Drawable::createFromImageData(icon_data, icon_data_size).get());
	button->addListener(this);
	addAndMakeVisible(button.get(), (int) panes_.size());

	if (panes_.empty())
	{
		addAndMakeVisible(pane.get());
	}
	else
	{
		addChildComponent(pane.get());
	}

	panes_.push_back({ std::move(button), std::move(pane) });
}

void FxSettingsDialog::SettingsComponent::resized()
{
	int y = BUTTON_Y;
	for (auto& entry : panes_)
	{
		entry.button->setBounds(BUTTON_X, y, BUTTON_WIDTH, BUTTON_HEIGHT);
		y = entry.button->getBottom() + 20;
	}

	juce::Rectangle<int> pane_rect(SEPARATOR_X + 1, 1, getWidth() - SEPARATOR_X + 1, getHeight() - 1);
	for (auto& entry : panes_)
	{
		entry.pane->setBounds(pane_rect);
	}
}

void  FxSettingsDialog::SettingsComponent::buttonClicked(Button* button)
{
	for (auto& entry : panes_)
	{
		bool selected = (entry.button.get() == button);
		entry.button->setToggleState(selected, NotificationType::dontSendNotification);
		entry.pane->setVisible(selected);
	}
}
