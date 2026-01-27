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

#include "DeviceConfig.h"
#include "Settings.h"

namespace FxSound
{
    void DeviceConfig::initDeviceConfigs(Settings& settings, std::vector<SoundDevice>& sound_devices)
    {
        juce::Array<DeviceConfig> device_configs;

        std::sort(sound_devices.begin(), sound_devices.end(),
            [](const SoundDevice& a, const SoundDevice& b)
            {
                return a.isActive > b.isActive;
            });

        for (auto sound_device : sound_devices)
        {
            DeviceConfig device_config = { sound_device.pwszID.c_str() , sound_device.deviceFriendlyName.c_str(), "" };
            device_configs.add(device_config);
        }

        saveDeviceConfigs(settings, "device_configs", device_configs);
    }

    void DeviceConfig::updateDeviceConfigs(Settings& settings, const std::vector<SoundDevice>& sound_devices)
    {
        juce::Array<DeviceConfig> device_configs = loadDeviceConfigs(settings, "device_configs");

        bool save_config = false;
        for (auto sound_device : sound_devices)
        {
            bool device_found = false;
            for (auto device_config : device_configs)
            {
                if (device_config.device_id == sound_device.pwszID.c_str())
                {
                    device_found = true;
                    break;
                }
            }
            if (!device_found)
            {
                save_config = true;
                DeviceConfig device_config = { sound_device.pwszID.c_str() , sound_device.deviceFriendlyName.c_str(), "" };
                device_configs.add(device_config);
            }
        }

        if (save_config)
        {
            saveDeviceConfigs(settings, "device_configs", device_configs);
        }
    }

    DeviceConfig DeviceConfig::getDeviceConfig(Settings& settings, juce::String device_id)
    {
        juce::Array<DeviceConfig> device_configs = loadDeviceConfigs(settings, "device_configs");

        for (auto device_config : device_configs)
        {
            if (device_config.device_id == device_id)
            {
                return device_config;
            }
        }

        return {};
    }

    juce::var DeviceConfig::toJson(const DeviceConfig& device_config)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("device_id", device_config.device_id);
        obj->setProperty("device_name", device_config.device_name);
        obj->setProperty("preset", device_config.preset);

        return juce::var(obj);
    }

    DeviceConfig DeviceConfig::fromJson(const juce::var& v)
    {
        DeviceConfig device_config;

        if (auto* obj = v.getDynamicObject())
        {
            device_config.device_id = obj->getProperty("device_id").toString();
            device_config.device_name = obj->getProperty("device_name").toString();
            device_config.preset = obj->getProperty("preset").toString();
        }

        return device_config;
    }

    juce::Array<DeviceConfig> DeviceConfig::loadDeviceConfigs(Settings& settings, juce::StringRef key)
    {
        juce::Array<DeviceConfig> device_configs;

        auto json = settings.getJson(key);
        if (!json.isArray())
            return device_configs;

        const auto& jsonArray = *json.getArray();

        for (const auto& device_config : jsonArray)
        {
            device_configs.add(fromJson(device_config));
        }

        return device_configs;
    }

    void DeviceConfig::saveDeviceConfigs(Settings& settings, juce::StringRef key, const juce::Array<DeviceConfig>& device_configs)
    {
        juce::Array<juce::var> jsonArray;
        jsonArray.ensureStorageAllocated(device_configs.size());

        for (const auto& device_config : device_configs)
        {
            jsonArray.add(toJson(device_config));
        }

        settings.setJson(key, juce::var(jsonArray));
    }
}