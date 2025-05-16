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

//#include "stdafx.h"
#include "u_AudioPassthru.h"
#include "sndDevices.h"

#define DFXG_SND_SERVER_KILL_THREAD_TIMEOUT_MSECS		  3000
#define DFXG_SND_SERVER_KILL_THREAD_WAIT_PER_LOOP_MSECS   50
#define DFXG_TRUNCATED_DRIVER_TEXT_LENGTH			      50

// See https://stackoverflow.com/questions/6472948/converting-member-function-pointer-to-timerproc
//std::map<UINT_PTR, AudioPassthru*> AudioPassthru::m_AudioPassthruClassMap;  //definition

// According to Paul's note, sndDevice handle must be declared statically.
// He didn't explain fully why (something to do with device event callbacks) 
// but without doing this I was getting access violiation excpeiotn in MMDevApi.dll so 
// there is some truth to it.
sndDevicesHdlType AudioPassthruPrivate::s_sndDevices_;

AudioPassthruPrivate::AudioPassthruPrivate()
{
	hp_sndDevices_ = (PT_HANDLE *)&(s_sndDevices_);
	//hp_sndDevices_ = hp_sndDevices;
	hProcessingThread_ = NULL;
	ProcessingThreadID_ = (DWORD)0;
	i_kill_processing_thread_ = IS_FALSE;
	swprintf(wcp_playback_device_guid_, PT_MAX_GENERIC_STRLEN, L"");
	b_no_valid_snd_device_dialog_shown_ = false;
	debug_ = IS_TRUE;
}

AudioPassthruPrivate::~AudioPassthruPrivate()
{
	int i_result_flag;
	int i_timed_out;

	if (hp_sndDevices_ == NULL)
		return;

	/* If the processing thread is running, kill it */
	if (killProcessingThread(&i_timed_out) != OKAY)
		return;

	/* If the killing of processing attempt timed out, then we can't do anything else */
	if (i_timed_out)
		return;

	/*
	* Disable the virtual soundcard
	* NOTE: FOR NOW WE DON'T DO THE DISABLE BECAUSE THIS CAN CAUSE PROBLEMS
	*/
	/*
	if (sndDevicesSetDeviceEnabledStatus(cast_handle->snd_server.hp_sndDevices,
	SND_DEVICES_VIRTUAL_PLAYBACK_DFX, FALSE,  &i_result_flag) != OKAY)
	return(NOT_OKAY);
	*/

	/* Change the default soundcard to not be the DFX virtual one but instead the proper real one */
	if (sndDevicesRestoreDefaultDevice(hp_sndDevices_, &i_result_flag) != OKAY)
		return;

	/* Free the data allocated inside the sndDevices_hdl (NOTE: Does not free up structure) */
	if (sndDevicesFree(hp_sndDevices_) != OKAY)
		return;
}

int AudioPassthruPrivate::init()
{
	int status_flag;
	
	/* Initialize the handle */
	if (sndDevicesInit(this->hp_sndDevices_, NULL, SND_DEVICES_INIT_FOR_PROCESSING, debug_, &status_flag) != OKAY)
		return(NOT_OKAY);

	if (status_flag != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
	{
		// Its possible that the audio system was in transition and returned a not ready status that caused us to arrive here.
		// The timer will force a call to sndDevicesReInit on first play attempt so there is no need to call it here, this section should only deal with fatal errors.
		if (debug_)
		{
			//swprintf(cast_handle->wcp_msg1, 2048, L"dfxg_SndServerInit(): sndDevicesInit failed, status_flag = %d", status_flag);
			//(cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
		}

		// Fatal errors.
		if (status_flag == SND_DEVICES_MP3_DLL_LOAD_FAILED)
		{
			// We were unable to load the MP3 decoding dll (only applies to MR), in addition to putting trace statement here we may want to add message box to user here.
		}
	}

	/* Initialize code for fixing the case of HDMI monitors being plugged in */
	//if (dfxg_SndServerHdmiFixInit(hp_dfxg) != OKAY)
		//return(NOT_OKAY);

	//pDspProcessingModule_->init();

	return(OKAY);
}

void AudioPassthruPrivate::setDspProcessingModule(DfxDsp* p_dfx_dsp)
{
	p_dfx_dsp_ = p_dfx_dsp;
}


std::vector<SoundDevice> AudioPassthruPrivate::getSoundDevices()
{
	// Convert hp_sndDevices from C handle to C++ vector of SoundDevice.
	sndDeviceHandleToSoundDevices();
	return sound_devices_;
}

int AudioPassthruPrivate::sndDeviceHandleToSoundDevices()
{
	int i_resultFlag;
	wchar_t wcp_user_seleted_playback_device_guid[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_targeted_real_playback_device_guid[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_capture_device_guid[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_dfx_device_guid[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_default_device_guid[PT_MAX_GENERIC_STRLEN];

	// Cast the handle so we can access its data.
	struct sndDevicesHdlType *cast_handle;
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices_;

	// Clear SoundDevices
	sound_devices_.clear();

	// Get the guid of the currently selected real playback device
	if (sndDevicesGetID(hp_sndDevices_, SND_DEVICES_TARGETED_REAL_PLAYBACK, wcp_targeted_real_playback_device_guid, &i_resultFlag) != OKAY ||
		sndDevicesGetID(hp_sndDevices_, SND_DEVICES_CAPTURE, wcp_capture_device_guid, &i_resultFlag) != OKAY ||
		sndDevicesGetID(hp_sndDevices_, SND_DEVICES_VIRTUAL_PLAYBACK_DFX, wcp_dfx_device_guid, &i_resultFlag) != OKAY ||
		sndDevicesGetID(hp_sndDevices_, SND_DEVICES_DEFAULT, wcp_default_device_guid, &i_resultFlag) != OKAY)
	{
		return(NOT_OKAY);
	}

	for (int index = 0; index < cast_handle->totalNumDevices; index++) 
	{
		if (cast_handle->pwszID[index] == NULL || cast_handle->deviceFriendlyName[index] == NULL)
		{
			continue;
		}

		SoundDevice sound_device;
		sound_device.pwszID = std::wstring(cast_handle->pwszID[index]);
		sound_device.deviceFriendlyName = std::wstring(cast_handle->deviceFriendlyName[index]);
		sound_device.deviceDescription = std::wstring(cast_handle->deviceDescription[index] != NULL ? cast_handle->deviceDescription[index] : L"");
		sound_device.deviceNumChannel = cast_handle->deviceNumChannel[index];

		// Skip mono devices if SND_DEVICES_MONO_BUG_SKIP_MONO_DEVICES is IS_TRUE
		if (SND_DEVICES_MONO_BUG_SKIP_MONO_DEVICES && sound_device.deviceNumChannel == 1)
		{
			continue;
		}

		// Figure out if this is a real device or not
		sound_device.isRealDevice = false;
		for (int index2 = 0; index2 < cast_handle->numRealDevices; index2++) 
		{
			if (sound_device.pwszID == cast_handle->pwszIDRealDevices[index2]) 
			{
				sound_device.isRealDevice = true;
			}
		}

		if (sound_device.pwszID == wcp_user_seleted_playback_device_guid)
		{
			sound_device.isUserSelectedPlaybackDevice = true;
		}
		if (sound_device.pwszID == wcp_targeted_real_playback_device_guid)
		{
			sound_device.isTargetedRealPlaybackDevice = true;
		}
		if (sound_device.pwszID == wcp_capture_device_guid)
		{
			sound_device.isCaptureDevice = true;
		}
		if (sound_device.pwszID == wcp_dfx_device_guid)
		{
			sound_device.isDFXDevice = true;
		}
		if (sound_device.pwszID == wcp_default_device_guid)
		{
			sound_device.isDefaultDevice = true;
		}

		sound_devices_.push_back(sound_device);
	}	

	return(OKAY);
}


/*
* FUNCTION: killProcessingThread()
* DESCRIPTION:
*
*  If the processing thread is running, kill it.
*  This function does not return until the thread has been killed or it times out
*  trying to kill the thread.
*
*/
int AudioPassthruPrivate::killProcessingThread(int *ip_timed_out)
{
	BOOL b_need_to_kill_thread;
	BOOL bReturn;
	DWORD d_ExitCode;
	int i_thread_has_died;
	long l_total_msecs_waited;

	*ip_timed_out = IS_FALSE;

	/* Check if we should kill the processing thread */
	b_need_to_kill_thread = FALSE;

	if (hProcessingThread_ != NULL)
	{
		bReturn = GetExitCodeThread(hProcessingThread_, &d_ExitCode);
		if (d_ExitCode == STILL_ACTIVE)
		{
			b_need_to_kill_thread = TRUE;
		}
	}

	/* Kill the thread */
	if (b_need_to_kill_thread)
	{
		// Stop capture.  This will cause sndDevicesDoCapture() to exit and then the thread will die.
		if (sndDevicesStartStopCapture(hp_sndDevices_, SND_DEVICES_STOP_CAPTURE) != OKAY)
			return(NOT_OKAY);

		i_kill_processing_thread_ = IS_TRUE;

		/* Wait until thread has died or we have timed out */
		i_thread_has_died = IS_FALSE;
		l_total_msecs_waited = 0L;

		while ((!i_thread_has_died) && (!(*ip_timed_out)))
		{
			bReturn = GetExitCodeThread(hProcessingThread_, &d_ExitCode);
			if (d_ExitCode != STILL_ACTIVE)
			{
				i_thread_has_died = IS_TRUE;
				hProcessingThread_ = NULL;
			}
			else
			{
				Sleep(DFXG_SND_SERVER_KILL_THREAD_WAIT_PER_LOOP_MSECS);
				l_total_msecs_waited = l_total_msecs_waited + DFXG_SND_SERVER_KILL_THREAD_WAIT_PER_LOOP_MSECS;
				if (l_total_msecs_waited >= DFXG_SND_SERVER_KILL_THREAD_TIMEOUT_MSECS)
					*ip_timed_out = IS_TRUE;
			}
		}
	}

	return(OKAY);
}

/*
* FUNCTION: setBufferLength()
* DESCRIPTION:
*
*  Sets the buffer length.
*
*/
int AudioPassthruPrivate::setBufferLength(int i_buffer_length_msecs)
{
	int i_timed_out;

	/*
	* Set the new buffer setting.
	* This just sets registry value which will be picked up during reinit.
	*/
	if (sndDevicesSetBufferSizeMilliSecs(hp_sndDevices_, i_buffer_length_msecs) != OKAY)
		return(NOT_OKAY);

	/* Kill the processing thread, so that the timer will then restart it with the new buffer setting */
	if (killProcessingThread(&i_timed_out) != OKAY)
		return(NOT_OKAY);

	/* If the killing of processing attempt timed out, then we can't do anything else */
	if (i_timed_out)
		return(OKAY);

	return(OKAY);
}

/*
* FUNCTION: processTimer()
* DESCRIPTION:
*
*  Process the snd server timer.
*/
int AudioPassthruPrivate::processTimer()
{
	BOOL bReturn;
	BOOL b_need_to_start_thread;
	DWORD d_ExitCode;
	int numRealDevices;
	int DfxDeviceEnabledFlag;
	int statusFlag;

	b_need_to_start_thread = FALSE;

	/*
	* Check if we need to start the thread for the first time or restart it.
	* Keep in mind that the main thread will kill itself if anything changes in terms of active soundcards.
	* This is kind of a brute force way of reinitializing and taking care of changes.
	*/
	if (hProcessingThread_ == NULL)
		b_need_to_start_thread = TRUE;
	else
	{
		bReturn = GetExitCodeThread(hProcessingThread_, &d_ExitCode);
		if (d_ExitCode != STILL_ACTIVE)
		{
			b_need_to_start_thread = TRUE;
		}
	}

	if (b_need_to_start_thread)
	{
		/* Initialize flag which can be set by the outside telling thread to end */
		i_kill_processing_thread_ = IS_FALSE;

		/* Reinit the sndDevices module */
		if (sndDevicesReInit(hp_sndDevices_, SND_DEVICES_INIT_FOR_PROCESSING, &numRealDevices, &DfxDeviceEnabledFlag, &statusFlag) != OKAY)
			return(NOT_OKAY);

		if (statusFlag != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
		{
			/*
			if (cast_handle->trace.mode)
			{
				swprintf(cast_handle->wcp_msg1, 2048, L"dfxg_ProcessSndServerTimer(): sndDevicesReInit failed, statusFlag = %d", statusFlag);
				(cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
			}

			if (statusFlag == SND_DEVICES_NO_VALID_PLAYBACK_DEVICE)
			{
				if (dfxg_GetTranslatedString(hp_dfxg, IDS_DFX_MSG_ASK_EXIT_DFX_TO_HEAR_SOUNDS,
					cast_handle->wcp_translated_string) != OKAY)
					return(NOT_OKAY);
			}

			if (statusFlag == SND_DEVICES_ASK_USER_SELECT_PLAYBACK_DEVICE)
			{
				if (dfxg_GetTranslatedString(hp_dfxg, IDS_DFX_MSG_SELECT_AUDIO_PLAYBACK_FROM_MENU,
					cast_handle->wcp_translated_string) != OKAY)
					return(NOT_OKAY);
			}

			if (cast_handle->snd_server.b_no_valid_snd_device_dialog_shown == false)
			{
				cast_handle->snd_server.b_no_valid_snd_device_dialog_shown = true;
				if (dfxg_BringUpMsgModelessDlg(hp_dfxg, cast_handle->wcp_translated_string, IS_FALSE, NULL, IS_FALSE, 0) != OKAY)
					return(NOT_OKAY);
			}
			*/
		}
		else
		{
			b_no_valid_snd_device_dialog_shown_ = false;
		}

		/* PTNOTE - added check on DfxDeviceEnabledFlag status, may need to take additional steps if no DFX device is preset. */
		if ((numRealDevices > 0) && (DfxDeviceEnabledFlag == IS_TRUE))
			hProcessingThread_ = CreateThread(NULL, 0, processingThread, (LPVOID)this, 0L, &ProcessingThreadID_);

		/* Check if a new playback device has been selected */
		/*
		if (dfxg_SndServerHdmiFixCheckForNewPlaybackDevice(hp_dfxg) != OKAY)
			return(NOT_OKAY);
		*/

		callback_->onSoundDeviceChange(getSoundDevices());
	}

	return(OKAY);
}

/*
* FUNCTION: threadWorker()
* DESCRIPTION:
*/
DWORD AudioPassthruPrivate::threadWorker(void)
{
	float *fp_buffer;
	int numSampleSets;
	WAVEFORMATEX *pwfx;
	int i_check_for_duplicate_buffers;
	int i_valid_bits;
	int resultFlag;
	DWORD setReturn;

	// Raise the priority of this tread to improve performance. GetCurrentThread() is a call that
	// returns the current thread ID from within the thread itself.
	// A return of 0 means set failed. Not sure what option to use, MS doc is confusing, THREAD_PRIORITY_HIGHEST is another option.
	setReturn = SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	// Start capture.
	if (sndDevicesStartStopCapture(hp_sndDevices_, SND_DEVICES_START_CAPTURE) != OKAY)
		return(NOT_OKAY);

	/*
	* Keep looping until a goto statement is reached to kill the thread
	* Also, constantly check to see the kill_thread flag has been set from the outside.
	*/
	while (1)
	{
		/* Check if thread has been signaled to end */
		if (i_kill_processing_thread_)
			goto KillProcessingThread;

		// Does the data capture and returns a pointer to the data and signal info to be used for in place audio processing.
		// NOT_OKAY is only returned for catastrophic errors but if resultFlag != SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS
		// then typically a change in a device property has caused the capture or playback operation to fail, in this
		// case we need to exit this thread so a reinitialization can be done.
		if (sndDevicesDoCapture(hp_sndDevices_, &fp_buffer, &numSampleSets, &pwfx, &resultFlag) != OKAY)
			return(NOT_OKAY);

		// A non-successful flag will typically be due to a change in the playback devices properties.
		if (resultFlag != SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS)
			goto KillProcessingThread;

		/* Check if thread has been signaled to end */
		if (i_kill_processing_thread_)
			goto KillProcessingThread;

		if (numSampleSets > 0)
		{
			/* Set additional processing settings */
			i_valid_bits = pwfx->wBitsPerSample;
			i_check_for_duplicate_buffers = IS_FALSE;

			/* Make sure processing module has the current format settings */
			//if (dfxpUniversalSetSignalFormat(cast_handle->dfxp_hdl, pwfx->wBitsPerSample, pwfx->nChannels, pwfx->nSamplesPerSec, i_valid_bits) != OKAY)
				//return(NOT_OKAY);
			if (p_dfx_dsp_->setSignalFormat(pwfx->wBitsPerSample, pwfx->nChannels, pwfx->nSamplesPerSec, i_valid_bits) != OKAY)
			{
				// In Release build, initially setSignalFormat() will return NOT OKAY so execution will hit here and throws up message box due to return NOT_OAKY.
				// However, it doesn't happen in Debug build. Workaround is just comment out return(NOT_OKAY) so it won't throw up message boxes in Release build
				// and eventually it will start to return OKAY.
				//return(NOT_OKAY);
			}
				
			

			// 2016-08-25: Workaround for crashing when playback device has only one channel (mono). For example, a Bleutooth headset is mono. When such
			// device is selected as the playback device, we do not apply DFX/DSP processing to the buffer for now. Otherwise it will crash.
			// NOTE: There will also be no sound with a mono playback device until we add code to fill mono playback buffer in sndDevicesDoCapture.cpp, line 279.
			if (pwfx->nChannels != 1 || (pwfx->nChannels == 1 && !SND_DEVICES_MONO_BUG_DO_NOT_PROCESS))
			{
				// Apply DFX processing here using data and format vars above. Format will always be 32 bit floating point.
			//	if (dfxpUniversalModifySamples(cast_handle->dfxp_hdl, (short int *)fp_buffer, (short int *)fp_buffer, numSampleSets, i_check_for_duplicate_buffers) != OKAY)
				//	return(NOT_OKAY);
				p_dfx_dsp_->processAudio((short int *)fp_buffer, (short int *)fp_buffer, numSampleSets, i_check_for_duplicate_buffers);
			}

			/* Check if thread has been signaled to end */
			if (i_kill_processing_thread_)
				goto KillProcessingThread;

			/* A non-successful flag will typically be due to a change in the playback devices properties. */
			if (resultFlag != SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS)
				goto KillProcessingThread;
		}

		/*
		* Plays processed buffer.
		* NOTE: This needs to outside of the if block so that unplayed buffers can still be played.
		*       It is okay to call this even when all buffers have already been played.
		*/
		if (!mute_)
		{
			if (sndDevicesDoPlayback(hp_sndDevices_, &resultFlag) != OKAY)
				return(NOT_OKAY);
		}
		

		/* Make sure the playback succeeded */
		if (resultFlag != SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS)
			goto KillProcessingThread;
	}

KillProcessingThread:

	// Stop capture.
	if (sndDevicesStartStopCapture(hp_sndDevices_, SND_DEVICES_STOP_CAPTURE) != OKAY)
		return(NOT_OKAY);

	/* Simply returning from this function stops the thread */
	return(OKAY);
}

/*
* FUNCTION: processingThread()
* DESCRIPTION:
*/
DWORD WINAPI AudioPassthruPrivate::processingThread(LPVOID lpParam)
{
	// Make sure COM is initialized
	HRESULT hr;
	hr = CoInitialize(NULL);

	AudioPassthruPrivate * callerClass = (AudioPassthruPrivate*)lpParam;
	auto ret = callerClass->threadWorker();

	CoUninitialize();

	return ret;
}

void AudioPassthruPrivate::mute(bool mute)
{
	mute_ = mute;
}

void AudioPassthruPrivate::registerCallback(AudioPassthruCallback* callback)
{
	callback_ = callback;
}

bool AudioPassthruPrivate::isPlaybackDeviceAvailable()
{
    BOOL availability;
    sndDevicesGetPlaybackDeviceAvialblility(hp_sndDevices_, &availability);
    if (availability == TRUE)
        return true;
    else
        return false;
}

int AudioPassthruPrivate::setTargetedRealPlaybackDevice(const std::wstring sound_device_guid)
{
	int i_resultFlag;
	wchar_t wcp_old_targeted_real_playback_guid[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_new_targeted_real_playback_guid[PT_MAX_GENERIC_STRLEN];
	
	/* Set the guid of the newly selected playback device */
	swprintf(wcp_new_targeted_real_playback_guid, PT_MAX_GENERIC_STRLEN, L"%s", sound_device_guid.c_str());

	/* Get the guid of the previously targeted playback device to see if it has changed */
	if (sndDevicesGetID(hp_sndDevices_,
		SND_DEVICES_TARGETED_REAL_PLAYBACK, wcp_old_targeted_real_playback_guid,
		&i_resultFlag) != OKAY)
		return(NOT_OKAY);

	if (i_resultFlag != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
		swprintf(wcp_old_targeted_real_playback_guid, PT_MAX_GENERIC_STRLEN, L"");

	/* If the targeted playback device has not changed, then do nothing */
	if (wcscmp(wcp_old_targeted_real_playback_guid, wcp_new_targeted_real_playback_guid) == 0)
		return(OKAY);

	/* Set the newly targeted playback device as the default.  It will then automatically become the targeted device */
	if (sndDevicesSetDeviceType(hp_sndDevices_, SND_DEVICES_DEFAULT, wcp_new_targeted_real_playback_guid, &i_resultFlag) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}