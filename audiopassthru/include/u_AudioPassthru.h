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
#include <atomic>
#include <cstdint>
#include <memory>
#include "AudioShutdown.h"
#include "EndpointRestore.h"

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
	std::vector<SoundDevice> getSoundDevices(bool active_devices = true);
	int killProcessingThread(int *ip_timed_out);
	int setBufferLength(int i_buffer_length_msecs);
	int processTimer();
	void setDspProcessingModule(DfxDsp* p_dfx_dsp);
	static DWORD WINAPI processingThread(LPVOID lpParam);
	DWORD threadWorker(void); // Needs to be public to be called from static thread starter function
	int setTargetedRealPlaybackDevice(const std::wstring sound_device_guid);
	void registerCallback(AudioPassthruCallback *callback);
    void setDeviceNotificationWindow(HWND window) noexcept;
    void dispatchDeviceChange();
    void requestStop() noexcept;
    fxaudio::ShutdownTask::Outcome shutdown(DWORD timeoutMs) noexcept;
    DWORD pollStopped(DWORD& error) noexcept;
    bool isPlaybackDeviceAvailable();
	bool checkDeviceChanges();
	void restoreDefaultPlaybackDevice();

private:
    friend struct AudioPassthruTestPeer;
	int sndDeviceHandleToSoundDevices(bool active_devices = true);
	static void onDeviceChange() noexcept;
    void signalCaptureStop() noexcept;
    DWORD captureAndPlay();
    void applyDsp(float* pcm, int frames, const WAVEFORMATEX& format);
    void refreshRestoreSnapshot() noexcept;
    bool publishUsedEndpoints() noexcept;
    bool refreshDeviceCatalogue();
    bool refreshPendingCatalogue(bool notice);
    bool drainDeviceChanges();
    bool applyPendingOutput();
    void overlayRouteFlags();
    bool route_change_pending_ = false; // Owner only; catalogue events never set this implicitly.
    std::wstring pending_output_id_, route_virtual_id_, route_capture_id_, route_output_id_;
    std::uint64_t pending_output_generation_ = 0;
    bool catalogue_dirty_ = false, catalogue_notice_pending_ = false;
    ULONGLONG catalogue_retry_after_ = 0, catalogue_retry_until_ = 0;
    static void CALLBACK retryDeviceNotice(PTP_CALLBACK_INSTANCE, void*, PTP_TIMER) noexcept;
    static void armDeviceRetry() noexcept;
    void closeDeviceRetry() noexcept;
    std::shared_ptr<const fxaudio::EndpointRestoreSnapshot> restore_snapshot_;
    fxaudio::ShutdownTask shutdown_task_;
    bool initialization_started_ = false;
    bool stopping_ = false; // Owner thread; prevents restart once shutdown begins.

	PT_HANDLE *hp_sndDevices_;
	static sndDevicesHdlType& s_sndDevices_;

	/* Processing Thread Info */
	HANDLE hProcessingThread_;
	DWORD ProcessingThreadID_;
	std::atomic<int> i_kill_processing_thread_; /* Flag set from the outside telling processing thread to end */
	wchar_t wcp_playback_device_guid_[PT_MAX_GENERIC_STRLEN];
	bool b_no_valid_snd_device_dialog_shown_; /* Flag stating whether we have shown the user a message to select a valid snd device.  We only want it shown once per session. */
	int debug_;
	std::vector<SoundDevice> sound_devices_;
	static constexpr unsigned mutedBit = 1;
    // Low bit is mute; the remaining bits distinguish successive mute cycles.
    std::atomic<unsigned> mute_state_{0};
	DfxDsp *p_dfx_dsp_;
	static std::atomic<AudioPassthruCallback*> s_callback_;
    static std::atomic<HWND> s_notification_window_;
    static std::atomic<bool> s_device_change_pending_, s_device_message_posted_;
    static PTP_TIMER s_device_retry_;
    static std::atomic<bool> s_retry_armed_;
};

