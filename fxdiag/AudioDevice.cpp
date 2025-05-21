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

#include "fxdiag.h"
#include "AudioDevice.h"

const wchar_t* RESET_COLOR_FORMAT = L"\033[0m\n";

void ReportDeviceFeatures(const AudioDevice& audioDevice);

AudioDevice::AudioDevice(IMMDevice* pDevice) : deviceState_(DeviceState::NotPresent), channels_(0), samplesPerSec_(0), bitsPerSample_(0)
{
	if (pDevice == NULL)
	{
		return;
	}

	initProperties(pDevice);
}

void AudioDevice::initProperties(IMMDevice* pDevice)
{
	IPropertyStore* pPropStore = NULL;
	PROPVARIANT propName;
	PROPVARIANT propFormFactor;

	PropVariantInit(&propName);
	PropVariantInit(&propFormFactor);

	if (SUCCEEDED(pDevice->OpenPropertyStore(STGM_READ, &pPropStore)))
	{
		if (SUCCEEDED(pPropStore->GetValue(PKEY_Device_FriendlyName, &propName)))
		{
			if (propName.vt == VT_LPWSTR)
			{
				deviceName_ = propName.pwszVal;
			}
		}

		if (SUCCEEDED(pPropStore->GetValue(PKEY_AudioEndpoint_FormFactor, &propFormFactor)))
		{
			UINT formFactor = propFormFactor.uintVal;

			switch (formFactor) 
			{
			case Speakers:
				deviceType_ = L"Speakers";
				break;
			case LineLevel:
				deviceType_ = L"LineLevel";
				break;
			case Headphones:
				deviceType_ = L"Headphones";
				break;
			case Headset:
				deviceType_ = L"Headset";
				break;
			case Handset:
				deviceType_ = L"Handset";
				break;
			case SPDIF:
				deviceType_ = L"SPDIF";
				break;
			case DigitalAudioDisplayDevice:
				deviceType_ = L"DisplayAudio";
				break;
			case UnknownFormFactor:
				deviceType_ = L"Unknown";
				break;
			default:
				deviceType_ = L"Others";
			}
		}
	}

	PropVariantClear(&propName);
	PropVariantClear(&propFormFactor);

	DWORD state;
	pDevice->GetState(&state);

	deviceState_ = DeviceState::NotPresent;
	if (state & DEVICE_STATE_ACTIVE)
	{
		deviceState_ = DeviceState::Active;
	} 
	else if (state & DEVICE_STATE_NOTPRESENT)
	{
		deviceState_ = DeviceState::NotPresent;
	} 
	else if (state & DEVICE_STATE_DISABLED)
	{
		deviceState_ = DeviceState::Disabled;
	} 
	else if (state & DEVICE_STATE_UNPLUGGED)
	{
		deviceState_ = DeviceState::Unplugged;
	}

	if (state & DEVICE_STATE_ACTIVE)
	{
		CComPtr<IAudioClient> pAudioClient;
		if (SUCCEEDED(pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&pAudioClient))))
		{
			WAVEFORMATEX* pwfx;
			if (SUCCEEDED(pAudioClient->GetMixFormat(&pwfx)))
			{
				channels_ = pwfx->nChannels;
				samplesPerSec_ = pwfx->nSamplesPerSec;
				bitsPerSample_ = pwfx->wBitsPerSample;
				CoTaskMemFree(pwfx);
			}
		}

		CComPtr<IAudioEndpointVolume> pEndpointVolume;
		if (SUCCEEDED(pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume)))
		{
			float fVolume = 0.0f;
			if (SUCCEEDED(pEndpointVolume->GetMasterVolumeLevelScalar(&fVolume)))
			{
				volumeLevel_ = fVolume * 100;
			}
		}
	}
}

std::vector<AudioDevice> EnumAudioDevices()
{
	HRESULT hr;
	std::vector<AudioDevice> audioDevices;
	CComPtr<IMMDeviceEnumerator> pDeviceEnumerator;

	hr = pDeviceEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
	if (FAILED(hr)) return audioDevices;

	CComPtr<IMMDeviceCollection> pDeviceCollection;
	hr = pDeviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATEMASK_ALL, &pDeviceCollection);
	if (FAILED(hr)) return audioDevices;

	UINT count;
	hr = pDeviceCollection->GetCount(&count);
	for (UINT index = 0; index < count; index++)
	{
		CComPtr<IMMDevice> pDevice;

		hr = pDeviceCollection->Item(index, &pDevice);
		if (SUCCEEDED(hr))
		{
			AudioDevice audioDevice(pDevice);
			if (audioDevice.deviceName_.length() > 0)
			{
				audioDevices.push_back(audioDevice);
			}
		}
	}

	if (audioDevices.size() > 0)
	{
		std::sort(audioDevices.begin(), audioDevices.end(), [](const AudioDevice& d1, const AudioDevice& d2) {
			return d1.deviceState_ < d2.deviceState_;
			});
	}

	return audioDevices;
}

void ReportAudioDevices(const std::vector<AudioDevice>& audioDevices)
{
	std::wcout << "Audio Playback Devices" << std::endl << std::endl;
	int i = 1;
	for (auto audioDevice : audioDevices)
	{
		std::wcout << ColorFormat(audioDevice.deviceState_ == DeviceState::Active ?  51 : 245) << i << L". " << audioDevice.deviceName_ << L" [" << audioDevice.deviceType_ << "]" << std::endl;
		switch (audioDevice.deviceState_)
		{
			case DeviceState::Active:
				std::wcout << ColorFormat(10) << L"[Active]";
				ReportDeviceFeatures(audioDevice);
				std::wcout << RESET_COLOR_FORMAT << std::endl;
				break;

			case DeviceState::NotPresent:
				std::wcout << ColorFormat(88) << L"[Not Present]" << RESET_COLOR_FORMAT << std::endl;
				break;

			case DeviceState::Disabled:
				std::wcout << ColorFormat(88) << L"[Disabled]" << RESET_COLOR_FORMAT << std::endl;
				break;

			case DeviceState::Unplugged:
				std::wcout << ColorFormat(88) << L"[Unplugged]" << RESET_COLOR_FORMAT << std::endl;
		}

		i++;
	}
}

void ReportDeviceFeatures(const AudioDevice& audioDevice)
{
	if (audioDevice.channels_ < 2)
	{
		std::wcout << ColorFormat(196);
	}
	else
	{
		std::wcout << ColorFormat(45);
	}
	std::wcout << L" [" << audioDevice.channels_;
	(audioDevice.channels_ > 1) ? std::wcout << L" - Channels]" : std::wcout << " - Channel])";
	std::wcout << ColorFormat(27) << L" [" << audioDevice.bitsPerSample_ << L" bit, " << audioDevice.samplesPerSec_ << L" Hz]";

	if (audioDevice.volumeLevel_ < 25.0)
	{
		std::wcout << ColorFormat(196);
	}
	else if (audioDevice.volumeLevel_ < 50.0)
	{
		std::wcout << ColorFormat(214);
	} 
	else if (audioDevice.volumeLevel_ < 75.0)
	{
		std::wcout << ColorFormat(184);
	}
	else
	{
		std::wcout << ColorFormat(10);
	}

	std::wcout << L" [Volume: " << std::fixed << std::setprecision(0) << audioDevice.volumeLevel_ << L"%]";
}

std::wstring ColorFormat(int colorCode)
{
	std::wstringstream format;
	format << L"\033[38;5;" << colorCode << L"m";

	return format.str();
}
