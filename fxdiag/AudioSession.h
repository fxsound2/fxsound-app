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
struct AudioSession
{
	std::wstring sessionName_;
	AudioSessionState sessionState_;
	bool systemSession_;
	float volumeLevel_;
	bool muted_;

	AudioSession(IAudioSessionControl* pSessionControl);
	AudioSession() = delete;
	~AudioSession() = default;

private:
	void initProperties(IAudioSessionControl* pSessionControl);
};

std::vector<AudioSession> EnumAudioSessions();
void EnumDeviceSessions(IMMDevice* pDevice, std::vector<AudioSession>& audioSessions);
void ReportAudioSessions(const std::vector<AudioSession>& audioSessions);
void ReportSessionVolume(const AudioSession& audioSession);

std::wstring ColorFormat(int colorCode);
