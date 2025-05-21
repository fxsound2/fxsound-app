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
#include "AudioSession.h"

extern const wchar_t* RESET_COLOR_FORMAT;

AudioSession::AudioSession(IAudioSessionControl* pSessionControl) : sessionName_(L""), sessionState_(AudioSessionState::AudioSessionStateInactive), systemSession_(false), volumeLevel_(0.0), muted_(false)
{
	if (pSessionControl == NULL)
	{
		return;
	}

	initProperties(pSessionControl);
}

void AudioSession::initProperties(IAudioSessionControl* pSessionControl)
{
	LPWSTR displayName = NULL;
	HRESULT hr = pSessionControl->GetDisplayName(&displayName);
	if (SUCCEEDED(hr) && displayName != NULL && lstrlen(displayName) != 0) {
		sessionName_ = displayName;
		CoTaskMemFree(displayName);
	}

	CComPtr<ISimpleAudioVolume> pSimpleVolume;
	hr = pSessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pSimpleVolume);
	if (SUCCEEDED(hr))
	{
		float volume = 0.0f;
		BOOL mute = FALSE;
		hr = pSimpleVolume->GetMute(&mute);
		muted_ = mute == TRUE;
		hr = pSimpleVolume->GetMasterVolume(&volume);
		if (SUCCEEDED(hr))
		{
			volumeLevel_ = volume * 100.0f;
		}
	}

	AudioSessionState state;
	hr = pSessionControl->GetState(&state);
	if (SUCCEEDED(hr))
	{
		sessionState_ = state;
	}

	CComPtr<IAudioSessionControl2> pSessionControl2;
	hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
	if (SUCCEEDED(hr))
	{
		if (pSessionControl2->IsSystemSoundsSession() == S_OK)
		{
			systemSession_ = true;
		}
		else
		{
			systemSession_ = false;
		}

		if (sessionName_.empty())
		{
			DWORD processId = 0;
			hr = pSessionControl2->GetProcessId(&processId);
			if (SUCCEEDED(hr) && processId != 0) {
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
				if (hProcess) {
					WCHAR processName[MAX_PATH] = L"";
					if (GetModuleBaseName(hProcess, NULL, processName, MAX_PATH)) {
						sessionName_ = processName;
					}
					CloseHandle(hProcess);
				}
			}
		}
	}
}

std::vector<AudioSession> EnumAudioSessions()
{
	HRESULT hr;
	std::vector<AudioSession> audioSessions;
	CComPtr<IMMDeviceEnumerator> pDeviceEnumerator;

	hr = pDeviceEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
	if (FAILED(hr)) return audioSessions;

	CComPtr<IMMDevice> pConsoleDevice;
	hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pConsoleDevice);
	if (FAILED(hr)) return audioSessions;

	EnumDeviceSessions(pConsoleDevice, audioSessions);

	CComPtr<IMMDevice> pMultimediaDevice;
	hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pMultimediaDevice);
	if (FAILED(hr)) return audioSessions;

	EnumDeviceSessions(pMultimediaDevice, audioSessions);

	CComPtr<IMMDevice> pCommDevice;
	hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eCommunications, &pCommDevice);
	if (FAILED(hr)) return audioSessions;

	EnumDeviceSessions(pCommDevice, audioSessions);

	std::sort(audioSessions.begin(), audioSessions.end(), [](const AudioSession& s1, const AudioSession& s2) {
		return s1.sessionName_ < s2.sessionName_; });

	return audioSessions;
}

void EnumDeviceSessions(IMMDevice* pDevice, std::vector<AudioSession>& audioSessions)
{
	HRESULT hr;

	CComPtr<IAudioSessionManager2> pSessionManager;
	hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&pSessionManager));
	if (FAILED(hr)) return;

	CComPtr<IAudioSessionEnumerator> pSessionEnumerator;
	hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
	if (FAILED(hr)) return;

	int sessionCount;
	hr = pSessionEnumerator->GetCount(&sessionCount);
	if (FAILED(hr)) return;

	auto isSystemSession = [](const AudioSession& session) { return session.systemSession_; };
	for (auto i = 0; i < sessionCount; i++)
	{
		CComPtr<IAudioSessionControl> pSessionControl;

		hr = pSessionEnumerator->GetSession(i, &pSessionControl);
		if (SUCCEEDED(hr))
		{
			AudioSession audioSession(pSessionControl);
			if (audioSession.sessionName_.empty() && !audioSession.systemSession_) continue;

			if (audioSession.systemSession_)
			{
				auto it = std::find_if(audioSessions.begin(), audioSessions.end(), isSystemSession);
				if (it == audioSessions.end())
				{
					audioSession.sessionName_ = L"System sounds";
					audioSessions.push_back(audioSession);
				}
			}
			else
			{
				std::wstring sessionName = audioSession.sessionName_;
				auto it = std::find_if(audioSessions.begin(), audioSessions.end(), [sessionName](const AudioSession& session) { return session.sessionName_ == sessionName; });
				if (it == audioSessions.end())
				{
					audioSessions.push_back(audioSession);
				}
			}
		}
	}
}

void ReportAudioSessions(const std::vector<AudioSession>& audioSessions)
{
	std::wcout << std::endl << "Audio Sessions" << std::endl << std::endl;
	int i = 1;
	for (auto audioSession : audioSessions)
	{
		std::wcout << ColorFormat(audioSession.sessionState_ == AudioSessionState::AudioSessionStateActive ? 51 : 245) << i << L". " << audioSession.sessionName_ << std::endl;
		switch (audioSession.sessionState_)
		{
		case AudioSessionState::AudioSessionStateActive:
			std::wcout << ColorFormat(10) << L"[Active] ";
			ReportSessionVolume(audioSession);
			std::wcout << RESET_COLOR_FORMAT << std::endl;
			break;

		case AudioSessionState::AudioSessionStateInactive:
			std::wcout << ColorFormat(172) << L"[Inactive] ";
			ReportSessionVolume(audioSession);
			std::wcout << RESET_COLOR_FORMAT << std::endl;
			break;

		case AudioSessionState::AudioSessionStateExpired:
			std::wcout << ColorFormat(172) << L"[Expired] ";
			ReportSessionVolume(audioSession);
			std::wcout << RESET_COLOR_FORMAT << std::endl;
		}

		i++;
	}
}

void ReportSessionVolume(const AudioSession& audioSession)
{
	if (audioSession.muted_)
	{
		std::wcout << ColorFormat(196);
		std::wcout << L"[Muted] ";
	}
	else
	{
		if (audioSession.volumeLevel_ < 25.0)
		{
			std::wcout << ColorFormat(196);
		}
		else if (audioSession.volumeLevel_ < 50.0)
		{
			std::wcout << ColorFormat(214);
		}
		else if (audioSession.volumeLevel_ < 75.0)
		{
			std::wcout << ColorFormat(184);
		}
		else
		{
			std::wcout << ColorFormat(10);
		}
	}

	std::wcout << L"[Volume: " << std::fixed << std::setprecision(0) << audioSession.volumeLevel_ << L"%]";
}