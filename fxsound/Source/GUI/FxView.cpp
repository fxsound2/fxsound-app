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

#include "FxView.h"
#include "FxController.h"
#include "FxSettingsDialog.h"
#include "FxPresetExportDialog.h"
#include "FxPresetImportDialog.h"

FxView::FxView()
{
	FxModel::getModel().addListener(this);

	preset_list_.setJustificationType(Justification::centredLeft);
	preset_list_.setTextWhenNoChoicesAvailable(L"");
	preset_list_.setSize(LIST_WIDTH, LIST_HEIGHT);
	preset_list_.setDescription(TRANS("Preset List"));
	preset_list_.setWantsKeyboardFocus(true);
	addAndMakeVisible(&preset_list_);
	preset_list_.addListener(this);
	preset_list_.addMouseListener(this, true);

	endpoint_list_.setJustificationType(Justification::centredLeft);
	endpoint_list_.setTextWhenNoChoicesAvailable(L"");
	endpoint_list_.setSize(LIST_WIDTH, LIST_HEIGHT);
	endpoint_list_.setDescription(TRANS("Playback Device List"));
	endpoint_list_.setWantsKeyboardFocus(true);
	addAndMakeVisible(&endpoint_list_);
	endpoint_list_.addListener(this);
	endpoint_list_.addMouseListener(this, true);

    addChildComponent(&error_notification_);
}

void FxView::showErrorNotification(bool show)
{
    if (show)
    {
        error_notification_.setMessage(TRANS("FxSound is unable to play processed audio through the selected output device.\nAnother application could be using it in exclusive mode or the device could be\ndisconnected. To disable exclusive mode follow these "),
            { TRANS("steps."), "https://www.fxsound.com/learning-center/no-sound-with-fxsound-realtek" });
        auto component_bounds = endpoint_list_.getBounds();
        auto x = component_bounds.getX() - (FxNotification::MAX_WIDTH - component_bounds.getWidth());
        component_bounds.setX(x);
        component_bounds.setY(component_bounds.getBottom() + 5);
        component_bounds.setWidth(FxNotification::MAX_WIDTH);
        component_bounds.setHeight(FxNotification::MAX_HEIGHT);
        error_notification_.setBounds(component_bounds);
        error_notification_.showMessage(false);
    }
    else
    {
        error_notification_.setVisible(false);
    }
}

void FxView::comboBoxChanged(ComboBox* combobox)
{
	auto index = combobox->getSelectedItemIndex();
	if (index >= 0 && index < combobox->getNumItems())
	{
		if (combobox == &preset_list_)
		{
			FxController::getInstance().setPreset(index);
		}
		else if (combobox == &endpoint_list_)
		{
			FxController::getInstance().setOutput(index, false);
		}
	}
}

void FxView::modelChanged(FxModel::Event model_event)
{
	if (model_event == FxModel::Event::OutputListUpdated)
	{
		StringArray output_names = FxModel::getModel().getOutputNames();

		endpoint_list_.clear(NotificationType::dontSendNotification);
		int id = 1;
		for (auto name : output_names)
		{
			endpoint_list_.addItem(name, id);
			if (name.endsWith("[Mono]"))
			{
				endpoint_list_.setItemEnabled(id, false);
			}

            id++;
		}
	}

	if (model_event == FxModel::Event::OutputSelected)
	{
		endpoint_list_.setSelectedId(FxModel::getModel().getSelectedOutput() + 1, NotificationType::dontSendNotification);

        if (endpoint_list_.getError())
        {
            showErrorNotification(true);
        }
        else
        {
            showErrorNotification(false);
        }
	}

    if (model_event == FxModel::Event::OutputError)
    {
        if (!FxController::getInstance().isPlaybackDeviceAvailable())
        {
            endpoint_list_.setItemEnabled(FxModel::getModel().getSelectedOutput() + 1, false);
            endpoint_list_.setError(true);
        }
        else
        {
            endpoint_list_.setItemEnabled(FxModel::getModel().getSelectedOutput() + 1, true);
            endpoint_list_.setError(false);
            error_notification_.setVisible(false);
        }
    }
	
	if (model_event == FxModel::Event::PresetSelected)
	{
		preset_list_.setSelectedId(FxModel::getModel().getSelectedPreset() + 1, NotificationType::dontSendNotification);
	}

	if (model_event == FxModel::Event::PresetListUpdated)
	{
		preset_list_.clear(NotificationType::dontSendNotification);
		
		auto count = FxModel::getModel().getPresetCount();
		auto preset_type = FxModel::PresetType::AppPreset;
		for (auto i=0; i<count; i++)
		{
			auto preset = FxModel::getModel().getPreset(i);
			if (preset_type != preset.type)
			{
				preset_list_.addSeparator();
				preset_type = preset.type;
			}

			auto name = preset.name;
			if (i == FxModel::getModel().getSelectedPreset() && FxModel::getModel().isPresetModified())
			{
				name = name + L" *";
			}
			preset_list_.addItem(name, i+1);						
		}

		preset_list_.setSelectedId(FxModel::getModel().getSelectedPreset() + 1, NotificationType::dontSendNotification);
	}

	if (model_event == FxModel::Event::PresetModified)
	{
		auto& model = FxModel::getModel();
		auto preset = model.getPreset(model.getSelectedPreset());
		if (preset.name.isEmpty())
		{
			return;
		}
		if (model.isPresetModified())
		{
			preset_list_.changeItemText(model.getSelectedPreset()+1, preset.name + L" *");
			if (!preset_list_.isPopupActive())
			{
				preset_list_.setText(preset.name + L" *", NotificationType::dontSendNotification);
			}
		}
		else
		{
			auto count = FxModel::getModel().getPresetCount();
			for (auto i=0; i<count; i++)
			{
				preset_list_.changeItemText(i + 1, FxModel::getModel().getPreset(i).name);
			}
			if (!preset_list_.isPopupActive())
			{
				preset_list_.setText(preset.name, NotificationType::dontSendNotification);
			}
		}
		
		return;
	}
}

void FxView::mouseEnter(const MouseEvent&)
{
	preset_list_.highlightText(preset_list_.isMouseOver(true));
	endpoint_list_.highlightText(endpoint_list_.isMouseOver(true));

    if (endpoint_list_.getError() && (endpoint_list_.isMouseOver(true) || error_notification_.isMouseOver(true)))
    {   
        showErrorNotification(true);
    }
    else 
    {
        showErrorNotification(false);
    }
}

void FxView::mouseExit(const MouseEvent&)
{
	preset_list_.highlightText(preset_list_.isMouseOver(true));
	endpoint_list_.highlightText(endpoint_list_.isMouseOver(true));
}