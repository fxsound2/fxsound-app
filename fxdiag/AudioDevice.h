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
