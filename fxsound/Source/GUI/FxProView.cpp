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
#include "FxProView.h"
#include "FxController.h"

//==============================================================================
FxProView::FxProView() : tool_tip_(this)
{
	addAndMakeVisible(effects_);
	addAndMakeVisible(equalizer_);
    addChildComponent(visualizer_);

	equalizer_.addMouseListener(this, true);
	effects_.addMouseListener(this, true);

    auto& theme = dynamic_cast<LookAndFeel_V4&>(getLookAndFeel());

    tool_tip_.setColour(TooltipWindow::ColourIds::textColourId, theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::defaultText));
    tool_tip_.setOpaque(false);

	setOpaque(false);
	setSize(WIDTH, HEIGHT);
}

void FxProView::startVisualizer()
{
	visualizer_.start();
}

void FxProView::pauseVisualizer()
{
	visualizer_.pause();
}

void FxProView::update()
{
	effects_.update();
	equalizer_.update();

    if (FxController::getInstance().isAudioProcessing())
    {
        visualizer_.reset();
    }

	visualizer_.setVisible(true);
	setSize(WIDTH, HEIGHT + 20);
}

void FxProView::resized()
{
    auto component_bounds = juce::Rectangle<int>(PRESET_LIST_X, LIST_Y, LIST_WIDTH, LIST_HEIGHT);
    preset_list_.setBounds(component_bounds);

    component_bounds = juce::Rectangle<int>(OUTPUT_LIST_X, LIST_Y, LIST_WIDTH, LIST_HEIGHT);
    endpoint_list_.setBounds(component_bounds);

    auto visualizer_offset = 0;
    if (visualizer_.isVisible())
    {
        visualizer_.setBounds(40, preset_list_.getBottom() + 20, visualizer_.getWidth(), visualizer_.getHeight());
        visualizer_offset = visualizer_.getHeight() + 20;
    }

    auto bounds = getLocalBounds();

	effects_.setBounds(effects_.getBounds().withX(EFFECTS_X).withY(EFFECTS_Y+visualizer_offset));
	equalizer_.setBounds(equalizer_.getBounds().withX(effects_.getRight() + 16).withY(EFFECTS_Y+visualizer_offset));
}

void FxProView::paint(Graphics& g)
{
	auto& theme = dynamic_cast<LookAndFeel_V4&>(getLookAndFeel());

    auto visualizer_offset = 0;
    if (visualizer_.isVisible())
    {
        visualizer_offset = visualizer_.getHeight() + 20;
    }

	g.setFillType(FillType(theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::windowBackground)));
	g.fillAll();

	g.setFillType(FillType(Colour(0x0).withAlpha(0.2f)));
	g.fillRoundedRectangle(20, 16, 920, 330+visualizer_offset, 8);

    auto enable_controls = FxModel::getModel().getPowerState() && !FxModel::getModel().isMonoOutputSelected();

    preset_list_.setEnabled(enable_controls);
    effects_.setEnabled(enable_controls);
    equalizer_.setEnabled(enable_controls);
	visualizer_.setEnabled(enable_controls);
}

void FxProView::comboBoxChanged(ComboBox* combobox)
{
	FxView::comboBoxChanged(combobox);
}

void FxProView::modelChanged(FxModel::Event model_event)
{
	FxView::modelChanged(model_event);

	if (model_event == FxModel::Event::PresetSelected)
	{
		update();
	}
}

void FxProView::mouseEnter(const MouseEvent& mouse_event)
{
	FxView::mouseEnter(mouse_event);

	equalizer_.showValues(equalizer_.isMouseOver(true));
	effects_.showValues(effects_.isMouseOver(true));
}

void FxProView::mouseExit(const MouseEvent& mouse_event)
{
	FxView::mouseEnter(mouse_event);

	equalizer_.showValues(equalizer_.isMouseOver(true));
	effects_.showValues(effects_.isMouseOver(true));
}