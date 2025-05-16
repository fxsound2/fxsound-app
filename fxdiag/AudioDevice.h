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

#pragma once

enum class DeviceState : BYTE { Active = 0, Unplugged, Disabled, NotPresent  };

struct AudioDevice
{
	std::wstring deviceName_;
	std::wstring deviceType_;
	DeviceState deviceState_;
	WORD channels_;
	DWORD samplesPerSec_;
	WORD bitsPerSample_;
	float volumeLevel_;

	AudioDevice(IMMDevice* pDevice);
	AudioDevice() = delete;
	~AudioDevice() = default;

private:
	void initProperties(IMMDevice* pDevice);
};

std::vector<AudioDevice> EnumAudioDevices();
void ReportAudioDevices(const std::vector<AudioDevice>& audioDevices);
std::wstring ColorFormat(int colorCode);
