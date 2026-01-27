/*
FxSound
Copyright (C) 2026  FxSound LLC

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
#include "AudioPassthru.h"
#include "Settings.h"

namespace FxSound
{
    struct DeviceConfig
    {
        juce::String device_id;
        juce::String device_name;
        juce::String preset;

        static void initDeviceConfigs(Settings& settings, std::vector<SoundDevice>& sound_devices);
        static void updateDeviceConfigs(Settings& settings, const std::vector<SoundDevice>& sound_devices);

        static juce::Array<DeviceConfig> loadDeviceConfigs(Settings& settings, juce::StringRef key);
        static void saveDeviceConfigs(Settings& settings, juce::StringRef key, const juce::Array<DeviceConfig>& device_configs);

        static DeviceConfig getDeviceConfig(Settings& settings, juce::String device_id);

    private:
        static juce::var toJson(const DeviceConfig& device_config);
        static DeviceConfig fromJson(const juce::var& v);        
    };
}

