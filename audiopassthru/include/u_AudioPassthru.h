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

#ifndef __ANDROID__
#include <windows.h>
#endif //WIN32
#include "AudioPassthru.h"
#include "sndDevices.h"
#include <Mmdeviceapi.h>
#include <vector> 
#include <map>

class DspProcessingModule
{
public:
	virtual int setSignalFormat(int i_bps, int i_nch, int i_srate, int i_valid_bits) = 0;
	virtual int processAudio(short int *si_input_samples, short int *si_output_samples, int i_num_sample_sets, int i_check_for_duplicate_buffers) = 0;
};

class AudioPassthruPrivate
{
public:
	AudioPassthruPrivate();
	~AudioPassthruPrivate();
	int init();
	void mute(bool mute);
	std::vector<SoundDevice> getSoundDevices();
	int killProcessingThread(int *ip_timed_out);
	int setBufferLength(int i_buffer_length_msecs);
	int processTimer();
	void setDspProcessingModule(DfxDsp* p_dfx_dsp);
	static DWORD WINAPI processingThread(LPVOID lpParam);
	DWORD threadWorker(void); // Needs to be public to be called from static thread starter function
	int setTargetedRealPlaybackDevice(const std::wstring sound_device_guid);
	void registerCallback(AudioPassthruCallback *callback);
    bool isPlaybackDeviceAvailable();

private:
	int sndDeviceHandleToSoundDevices();

	PT_HANDLE *hp_sndDevices_;
	static sndDevicesHdlType s_sndDevices_;

	/* Processing Thread Info */
	HANDLE hProcessingThread_;
	DWORD ProcessingThreadID_;
	int i_kill_processing_thread_; /* Flag set from the outside telling processing thread to end */
	wchar_t wcp_playback_device_guid_[PT_MAX_GENERIC_STRLEN];
	bool b_no_valid_snd_device_dialog_shown_; /* Flag stating whether we have shown the user a message to select a valid snd device.  We only want it shown once per session. */
	int debug_;
	std::vector<SoundDevice> sound_devices_;
	bool mute_;
	DfxDsp *p_dfx_dsp_;
	AudioPassthruCallback *callback_;
};

