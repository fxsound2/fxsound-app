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
#include "codedefs.h"

/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <mmreg.h>
#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <endpointvolume.h>

#include "slout.h"
#include "reg.h"
#include "u_sndDevices.h"
#include "sndDevices.h"

/*
 * FUNCTION: sndDevicesReInit()
 * DESCRIPTION:
 *  This function will be called when any event that changes the properties of capture/playback devices occurs,
 *  such as a change in the playback devices sampling frequency that causes the processing loop to exit or
 *  a change in the currently selected default device.
 */
int PT_DECLSPEC sndDevicesReInit(PT_HANDLE *hp_sndDevices, int i_initType, int *ipNumRealDevices, int *ipDfxDeviceEnabledFlag, int *ip_status)
{
	struct sndDevicesHdlType *cast_handle;
	float playbackVolSetting;
	wchar_t name[PT_MAX_GENERIC_STRLEN];
	wchar_t regVal[PT_MAX_GENERIC_STRLEN];
	HRESULT hr;
	int resultFlag;
	int loopCount;
	int captureAllocSize, playbackAllocSize;
	int captureBufferChannelsForAllocation;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	SLOUT_FIRST_LINE(L"sndDevicesReInit():: sndDevicesReInit() enters");

	*ip_status = SND_DEVICES_DEVICE_OPERATION_COMPLETED;
	*ipNumRealDevices = 0;
	*ipDfxDeviceEnabledFlag = IS_FALSE;
	cast_handle->function_status = SND_DEVICES_DEVICE_OPERATION_COMPLETED;	// Clear function error status code.

	//cast_handle->ignoreDeviceCallbacks = TRUE;

	cast_handle->totalNumDevices = SND_DEVICES_DEVICE_NOT_PRESENT;
	cast_handle->dfxDeviceNum = SND_DEVICES_DEVICE_NOT_PRESENT;
	cast_handle->captureDeviceNum = SND_DEVICES_DEVICE_NOT_PRESENT;
	cast_handle->playbackDeviceNum = SND_DEVICES_DEVICE_NOT_PRESENT;
	cast_handle->defaultDeviceNum = SND_DEVICES_DEVICE_NOT_PRESENT;
	cast_handle->priorDefaultDeviceNum = SND_DEVICES_DEVICE_NOT_PRESENT;

	cast_handle->upsampleRatio = 1;
	cast_handle->numPlaybackFramesAvailableToFill = 0;
	cast_handle->playbackIsActive = SND_DEVICES_PLAYBACK_IS_STOPPED;
	cast_handle->playbackStreamIsTemporarilyPaused = 0;

	cast_handle->stopAudioCaptureAndPlaybackLoop = 0;

	wcscpy(cast_handle->lastDeviceAddCallbackGuid, L"");
	cast_handle->lastDeviceAddCallbackGuidtype = 0;
	
	cast_handle->noBufferCount = 0;
	cast_handle->MeasuredVirtualSilentBufferCount = 0;
	cast_handle->MeasuredTrueSilentBufferCount = 0;
	cast_handle->WindowsSilentBufferCount = 0;

	{	// The SND_DEVICES_INIT_FOR_PROCESSING and SND_DEVICES_INIT_NO_PROCESSING cases.

		// Free any reuseable objects that have been allocated.
		if( sndDevices_FreeReuseableObjects(hp_sndDevices) != OKAY )
			return(NOT_OKAY);

		*ipDfxDeviceEnabledFlag = IS_TRUE; //Will get set to false in loop below if DFX device is not found.
		loopCount = 0;
		do
		{
			Sleep(50);

			// Gets the properties of all current devices, finds DFX device if present.
			if( sndDevices_GetAll( hp_sndDevices, &(cast_handle->totalNumDevices) ) != OKAY )
				return(NOT_OKAY);

			*ipNumRealDevices = cast_handle->numRealDevices;

			// When we can read the DFX name, its enabled.
			if( sndDevicesGetFriendlyName(hp_sndDevices, SND_DEVICES_VIRTUAL_PLAYBACK_DFX, name, &resultFlag) != OKAY )
				return(NOT_OKAY);

			loopCount++;

			if( loopCount > 20 )	// 20 loops of 50 millisecs is 1 second.
			{
				if( i_initType == SND_DEVICES_INIT_FOR_PROCESSING )
				{
					*ipDfxDeviceEnabledFlag = IS_FALSE;
					return(OKAY);
				}
				else
				{
					break;
				}
			}
		}
		while( resultFlag != SND_DEVICES_DEVICE_OPERATION_COMPLETED );
	}

	// If audio processing will be performed, do additional initialization.
	if( (i_initType == SND_DEVICES_INIT_FOR_PROCESSING))
	{
		if (cast_handle->numRealDevices > 0) 
		{
			// Based on current and past default device settings and user selections,
			// assign the current playback device, the DFX capture device and the default device.
			if (sndDevicesImplementDeviceRules(hp_sndDevices, &resultFlag) != OKAY)
				return(NOT_OKAY);

			if (resultFlag != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
			{
				*ipDfxDeviceEnabledFlag = IS_FALSE;
				*ip_status = resultFlag;
				SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_RULES_NOT_POSSIBLE);
			}

			// Do the initial non-format dependent setup of capture and playback devices.
			if (sndDevicesInitialSetupCaptureDevice(hp_sndDevices, &resultFlag) != OKAY)
				return(NOT_OKAY);

			if (resultFlag != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
			{
				*ipDfxDeviceEnabledFlag = IS_FALSE;
				*ip_status = resultFlag;
				SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_RULES_NOT_POSSIBLE);
			}

			if (sndDevicesInitialSetupPlaybackDevice(hp_sndDevices, &resultFlag) != OKAY)
				return(NOT_OKAY);

			if (resultFlag != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
			{
				*ipDfxDeviceEnabledFlag = IS_FALSE;
				*ip_status = resultFlag;
				SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_RULES_NOT_POSSIBLE);
			}

			// NOTE - currently we're initializing DFX volume to playback device's volume, we may want to do the opposite.
			// Initialize the capture device (DFX) volume to the same as the playback device volume.
			// Recovers playback volume control setting in a nomalized range of 0.0 to 1.0
			hr = cast_handle->pEndptVolPlayback->GetMasterVolumeLevelScalar(&playbackVolSetting);
			if (hr != S_OK)
			{
				*ipDfxDeviceEnabledFlag = IS_FALSE;
				*ip_status = SND_DEVICES_RULES_NOT_POSSIBLE;
				SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_GET_MASTER_VOLUME_FAILED);
			}

			// Note, second arg is a GUID used in callbacks generated by this change to identify who made the change.
			// This is done to ID this change as coming from this program to avoid an infinite loop.
			hr = cast_handle->pEndptVolCapture->SetMasterVolumeLevelScalar(playbackVolSetting, &(cast_handle->guidThisApplication));
			if (hr != S_OK)
			{
				*ipDfxDeviceEnabledFlag = IS_FALSE;
				*ip_status = SND_DEVICES_RULES_NOT_POSSIBLE;
				SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_SET_MASTER_VOLUME_FAILED);
			}

			// Turn off mute on DFX device
			hr = cast_handle->pEndptVolCapture->SetMute(FALSE, &(cast_handle->guidThisApplication));
			if ((hr != S_OK) && (hr != S_FALSE))	// Note, will return S_FALSE if the mute was already off, check for other errors.
			{
				*ipDfxDeviceEnabledFlag = IS_FALSE;
				*ip_status = SND_DEVICES_RULES_NOT_POSSIBLE;
				SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_SET_MUTE_FAILED);
			}

			// Turn off mute on playback device
			hr = cast_handle->pEndptVolPlayback->SetMute(FALSE, &(cast_handle->guidThisApplication));
			if ((hr != S_OK) && (hr != S_FALSE))	// Note, will return S_FALSE if the mute was already off, check for other errors.
			{
				*ipDfxDeviceEnabledFlag = IS_FALSE;
				*ip_status = SND_DEVICES_RULES_NOT_POSSIBLE;
				SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_SET_MUTE_FAILED);
			}

			// Setup capture and playback buffers
			cast_handle->bufferSizeMilliSecs = SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS;

			// Read the buffer_size from the user setting location, if present set the internal value.
			if (sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_USER_BUFFER_SIZE, regVal) != OKAY)
				return(NOT_OKAY);

			if (wcscmp(regVal, L"") != 0)
			{
				swscanf(regVal, L"%d", &(cast_handle->bufferSizeMilliSecs));
			}
			else
			{
				// If no user setting, read the default buffer size setting.
				if (sndDeviceReadFromRegistry(hp_sndDevices, REG_LOCAL_MACHINE, SND_DEVICES_REGISTRY_DEFAULT_BUFFER_SIZE, regVal) != OKAY)
					return(NOT_OKAY);

				// If a value was read, set it, otherwise use the default.
				if (wcscmp(regVal, L"") != 0)
				{
					swscanf(regVal, L"%d", &(cast_handle->bufferSizeMilliSecs));
				}
			}

			// Check the ranges on the buffer size setting.
			if ((cast_handle->bufferSizeMilliSecs < SND_DEVICES_CAPTURE_BUFFER_MIN_SIZE_MILLI_SECS) || (cast_handle->bufferSizeMilliSecs >(SND_DEVICES_CAPTURE_BUFFER_MAX_SIZE_MILLI_SECS)))
				cast_handle->bufferSizeMilliSecs = SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS;

			// NOTE bufferSizeMilliSecs is the average bulk delay, actual buffer length is twice this, so use 500 in denoms to correct.
			cast_handle->hnsRequestedDurationCapture = (REFERENCE_TIME)((double)cast_handle->bufferSizeMilliSecs * (double)SND_DEVICES_REFTIMES_PER_SEC / 500.0);
			cast_handle->hnsRequestedDurationPlayback = (REFERENCE_TIME)((double)cast_handle->bufferSizeMilliSecs * (double)SND_DEVICES_REFTIMES_PER_SEC / 500.0);

			// PT-NOTE - in this non-dynamic version lock the allocation size to the max that would be needed
			//captureAllocSize  = (int)(48000  * 8 * (double)SND_DEVICES_CAPTURE_BUFFER_MAX_SIZE_MILLI_SECS * 1.001/500.0);
			//playbackAllocSize = (int)(192000 * 8 * (double)SND_DEVICES_CAPTURE_BUFFER_MAX_SIZE_MILLI_SECS * 1.001/500.0);

			// Note - with non-matched sample rates, in DoPlayback playback channels are temporarily copied back
			// to the capture buffer, but while still at capture sampling rate. So capture buffer must have size
			// for the same number of channels in the playback buffer.
			if ((cast_handle->wfxPlayback.nChannels > cast_handle->wfxCapture.nChannels) && (cast_handle->upsampleRatio > 1))
				captureBufferChannelsForAllocation = cast_handle->wfxPlayback.nChannels;
			else
				captureBufferChannelsForAllocation = cast_handle->wfxCapture.nChannels;

			captureAllocSize = (int)(cast_handle->wfxCapture.nSamplesPerSec  * captureBufferChannelsForAllocation  * (double)cast_handle->bufferSizeMilliSecs * 1.001 / 500.0);
			playbackAllocSize = (int)(cast_handle->wfxPlayback.nSamplesPerSec * cast_handle->wfxPlayback.nChannels * (double)cast_handle->bufferSizeMilliSecs * 1.001 / 500.0);

			if (cast_handle->fCaptureBuf != NULL)
			{
				if (captureAllocSize != cast_handle->captureBufAllocSize)
				{
					free(cast_handle->fCaptureBuf);
					cast_handle->fCaptureBuf = (float *)calloc(captureAllocSize, sizeof(float));
				}
			}
			else
				cast_handle->fCaptureBuf = (float *)calloc(captureAllocSize, sizeof(float));

			cast_handle->captureBufAllocSize = captureAllocSize;

			if (cast_handle->fPlaybackBuf != NULL)
			{
				if (playbackAllocSize != cast_handle->playbackBufAllocSize)
				{
					free(cast_handle->fPlaybackBuf);
					free(cast_handle->fFilePlaybackBuf);
					cast_handle->fPlaybackBuf = (float *)calloc(playbackAllocSize, sizeof(float));
					cast_handle->fFilePlaybackBuf = (float *)calloc(playbackAllocSize, sizeof(float));
				}
			}
			else
			{
				cast_handle->fPlaybackBuf = (float *)calloc(playbackAllocSize, sizeof(float));
				cast_handle->fFilePlaybackBuf = (float *)calloc(playbackAllocSize, sizeof(float));
			}

			cast_handle->playbackBufAllocSize = playbackAllocSize;

			if ((cast_handle->fCaptureBuf == NULL) || (cast_handle->fPlaybackBuf == NULL) || (cast_handle->fFilePlaybackBuf == NULL))
				return(NOT_OKAY);

			// Do the final format dependent setup of capture and playback devices.
			if (sndDevicesFinalSetupCaptureDevice(hp_sndDevices, &resultFlag) != OKAY)
				return(NOT_OKAY);

			if (resultFlag != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
			{
				*ipDfxDeviceEnabledFlag = IS_FALSE;
				*ip_status = resultFlag;
				SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_SET_MUTE_FAILED);
			}

			if (sndDevicesFinalSetupPlaybackDevice(hp_sndDevices, &resultFlag) != OKAY)
				return(NOT_OKAY);

			if (resultFlag != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
			{
				*ipDfxDeviceEnabledFlag = IS_FALSE;
				*ip_status = resultFlag;
				SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_SET_MUTE_FAILED);
			}

			cast_handle->ignoreDeviceCallbacks = FALSE;
		}
	}

	SLOUT_FIRST_LINE(L"sndDevicesReInit():: Returns OKAY");

	return(OKAY);
} 
