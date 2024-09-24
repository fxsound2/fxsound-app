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
