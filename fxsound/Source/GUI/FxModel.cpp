/*
FxSound
Copyright (C) 2023  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FxModel.h"

FxModel::FxModel()
{
	power_state_ = false;
	
	selected_preset_ = 0;
	selected_output_ = 0;
	preset_modified_ = false;
	hotkey_support_ = true;
	language_ = 1;
	debug_logging_ = false;

	selected_output_device_ = {};
}

void FxModel::initOutputs(const std::vector<SoundDevice>& output_devices)
{
	output_names_.clear();
	output_devices_ = output_devices;

	for (auto output_device : output_devices_)
	{
		if (output_device.deviceNumChannel < 2)
		{
			output_names_.add(String(output_device.deviceFriendlyName.c_str()) + String(" [Mono]"));
		}
		else
		{
			output_names_.add(output_device.deviceFriendlyName.c_str());
		}
	}
	

	notifyListeners(Event::OutputListUpdated);
}

void FxModel::initPresets(const Array<Preset>& presets)
{
	presets_.clear();
	presets_ = presets;

	notifyListeners(Event::PresetListUpdated);
}

int FxModel::addPreset(const Preset& preset)
{
	presets_.add(preset);
	return presets_.size();
}

void FxModel::removePreset(int preset)
{
	if (preset >= 0 && preset < presets_.size())
	{
		presets_.remove(preset);

		notifyListeners(Event::PresetListUpdated);
	}
}

int FxModel::selectPreset(const String& selected_preset, bool notify)
{
	if (selected_preset.isNotEmpty())
	{
		auto i = 0;
		for (auto preset : presets_)
		{
			if (preset.name == selected_preset)
			{
				selected_preset_ = i;
				break;
			}
			i++;
		}
	}
	else
	{
		selected_preset_ = 0;
	}

	if (notify)
	{
		notifyListeners(Event::PresetSelected);
	}

	return selected_preset_;
}

void FxModel::selectPreset(int selected_preset, bool notify)
{
	if (selected_preset >= 0 && selected_preset < presets_.size())
	{
		selected_preset_ = selected_preset;
	}

	if (notify)
	{
		notifyListeners(Event::PresetSelected);
	}
}

int FxModel::getSelectedPreset() const
{
	return selected_preset_;
}

int FxModel::getPresetCount() const
{
	return presets_.size();
}

int FxModel::getUserPresetCount() const
{
	int count = 0;
	for (auto preset : presets_)
	{
		if (preset.type == PresetType::UserPreset)
		{
			count++;
		}
	}

	return count;
}

FxModel::Preset FxModel::getPreset(int preset) const
{
	if (preset >= 0 && preset < presets_.size())
	{
		return presets_[preset];
	}

	return {};
}

bool FxModel::isPresetModified() const
{
	return preset_modified_;
}

void FxModel::setPresetModified(bool preset_modified)
{
	bool notify = (preset_modified_ != preset_modified);
	preset_modified_ = preset_modified;

	if (notify)
	{
		notifyListeners(Event::PresetModified);
	}
}

bool FxModel::isPresetNameValid(const String& preset_name)
{
    for (auto preset : presets_)
    {
        if (preset.name.equalsIgnoreCase(preset_name))
        {
            return false;
        }
    }

    return true;
}

void FxModel::notifyListeners(Event model_event)
{
	auto& listeners = listeners_.getListeners();
	for (auto i = 0; i < listeners.size(); i++)
	{
		listeners.getUnchecked(i)->modelChanged(model_event);
	}
}