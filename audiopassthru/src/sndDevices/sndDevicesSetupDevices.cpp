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
#include <stdio.h>
#include <math.h>

#include <mmreg.h>
#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <endpointvolume.h>
#include <Propvarutil.h>

// This is the include file for the "hidden" IPolicyConfig MS functions.
#include "PolicyConfig.h"

#include "slout.h"
#include "reg.h"
#include "mry.h"
#include "operatingSystem.h"
#include "u_sndDevices.h"
#include "sndDevices.h"

/*
 * FUNCTION: sndDevicesInitialSetupCaptureDevice()
 * Does non-format dependent setup of the capture device.
 */
int PT_DECLSPEC sndDevicesInitialSetupCaptureDevice(PT_HANDLE *hp_sndDevices, int *ip_status)
{
	struct sndDevicesHdlType *cast_handle;
	WAVEFORMATEX *pwfx;
	HRESULT hr;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if(cast_handle == NULL)
		return(NOT_OKAY);

	*ip_status = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	if( cast_handle->numRealDevices == 0 )
	{
		*ip_status = SND_DEVICES_DEVICE_NOT_PRESENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_NOT_PRESENT);
	}

	// Setup capture device if it was opened.
	if( cast_handle->pCaptureDevice == NULL )
	{
		*ip_status = SND_DEVICES_CAPTURE_DEVICE_IS_NULL;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_CAPTURE_DEVICE_IS_NULL);
	}

	// Activate capture device for audio.
	hr = cast_handle->pCaptureDevice->Activate(cast_handle->IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&cast_handle->pAudioClientCapture);
	if (FAILED(hr))
	{
		*ip_status = SND_DEVICES_DEVICE_ACTIVATION_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_ACTIVATION_FAILED);
	}

	if( cast_handle->pAudioClientCapture == NULL )
	{
		*ip_status = SND_DEVICES_NULL_CAPTURE_CLIENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_CAPTURE_CLIENT);
	}

	if( cast_handle->initializationMode == SND_DEVICES_INIT_FOR_PROCESSING )
	{
		// Activate capture device for volume control.
		hr = cast_handle->pCaptureDevice->Activate(cast_handle->IID_IAudioEndpointVolume, CLSCTX_ALL, NULL, (void**)&(cast_handle->pEndptVolCapture) );
		if (FAILED(hr))
		{
			*ip_status = SND_DEVICES_DEVICE_ACTIVATION_FAILED;
			SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_ACTIVATION_FAILED);
		}

		if( cast_handle->pEndptVolCapture == NULL )
		{
			*ip_status = SND_DEVICES_NULL_VOLUME_ENDPOINT;
			SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_VOLUME_ENDPOINT);
		}

		// Setup capture device volume control callback
		hr = cast_handle->pEndptVolCapture->RegisterControlChangeNotify( (IAudioEndpointVolumeCallback*)&(cast_handle->EPVolEventsCapture) );
		if (FAILED(hr))
		{
			*ip_status = SND_DEVICES_REGISTER_FAILED;
			SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_REGISTER_FAILED);
		}
	}

	// PTNOTE - Retrieves format of captured stream, will always be the current setting of the sound card, with
	// the exception that the signal format appears to always be 32 bit float.
	// This apparently allocates space and modified the pointer to point to that new space.
	hr = cast_handle->pAudioClientCapture->GetMixFormat(&pwfx);
	if (FAILED(hr))
	{
		*ip_status = SND_DEVICES_DEVICE_GET_FORMAT_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_GET_FORMAT_FAILED);
	}

	cast_handle->wfxCapture = *pwfx;

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesFinalSetupCaptureDevice()
 * Does final format dependent setup of the capture device.
 */
int PT_DECLSPEC sndDevicesFinalSetupCaptureDevice(PT_HANDLE *hp_sndDevices, int *ip_status)
{
	struct sndDevicesHdlType *cast_handle;
	WAVEFORMATEX *pwfx;
	DWORD StreamFlags;
	unsigned int procInfo;
	int numCores;
	HRESULT hr;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_status = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	if( cast_handle->numRealDevices == 0 )
	{
		*ip_status = SND_DEVICES_DEVICE_NOT_PRESENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_NOT_PRESENT);
	}

	if( cast_handle->pAudioClientCapture == NULL )
	{
		*ip_status = SND_DEVICES_CAPTURE_DEVICE_IS_NULL;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_CAPTURE_DEVICE_IS_NULL);
	}

	// Get the current OS
	if( operatingSystemGetSystemProperties(&procInfo, &numCores) != OKAY )
		return(NOT_OKAY);

	// The volume control hide flag can only be used on Win7.
	if( procInfo & OPERATING_SYSTEM_VISTA )
		StreamFlags = AUDCLNT_STREAMFLAGS_LOOPBACK;
	else
		// The AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE flag hides extra occurences of our volume control in Mixer dialog.
		StreamFlags = AUDCLNT_STREAMFLAGS_LOOPBACK|AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE;

	// For some reason next call won't take our copied version of this format, likely its expecting WAVEFORMATEXTENSIBLE
	hr = cast_handle->pAudioClientCapture->GetMixFormat(&pwfx);
	if (FAILED(hr))
	{
		*ip_status = SND_DEVICES_DEVICE_GET_FORMAT_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_GET_FORMAT_FAILED);
	}

	// Set capture device up to perform loopback capture. Shared mode allows multiple apps to do capture.
	hr = cast_handle->pAudioClientCapture->Initialize(AUDCLNT_SHAREMODE_SHARED, StreamFlags,
																	  cast_handle->hnsRequestedDurationCapture,0, pwfx, NULL);
	if (FAILED(hr))
	{
		*ip_status = SND_DEVICES_DEVICE_INIT_PROP_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_INIT_PROP_FAILED);
	}

	// Get the size of the allocated buffer, this is in sample sets, which they call frames.
	hr = cast_handle->pAudioClientCapture->GetBufferSize(&(cast_handle->bufferFrameSizeCapture));
	if (FAILED(hr))
	{
		*ip_status = SND_DEVICES_GET_BUFFER_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_GET_BUFFER_FAILED);
	}

	// Request and setup the capture service.
	hr = cast_handle->pAudioClientCapture->GetService(cast_handle->IID_IAudioCaptureClient, (void**)&cast_handle->pAudioCaptureLoopback);
	if (FAILED(hr))
	{
		*ip_status = SND_DEVICES_GET_SERVICE_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_GET_SERVICE_FAILED);
	}

	if( cast_handle->pAudioCaptureLoopback == NULL )
	{
		*ip_status = SND_DEVICES_NULL_LOOPBACK_CLIENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_LOOPBACK_CLIENT);
	}

	// Calculate the actual duration of the allocated capture buffer, in REF TIME tics.
	cast_handle->hnsActualDurationCapture = (REFERENCE_TIME)((double)SND_DEVICES_REFTIMES_PER_SEC * (double)cast_handle->bufferFrameSizeCapture / (double)cast_handle->wfxCapture.nSamplesPerSec);

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesInitialSetupPlaybackDevice()
 * Does non-format dependent setup of the playback device.
 */
int PT_DECLSPEC sndDevicesInitialSetupPlaybackDevice(PT_HANDLE *hp_sndDevices, int *ip_status)
{
	struct sndDevicesHdlType *cast_handle;
	WAVEFORMATEX *pwfx;
	HRESULT hr;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_status = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	// Setup playback device if it was opened.
	if( cast_handle->pPlaybackDevice == NULL )
	{
		*ip_status = SND_DEVICES_NULL_PLAYBACK_DEVICE;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_PLAYBACK_DEVICE);
	}

	if (cast_handle->pAudioClientPlayback != NULL)
	{
		SAFE_RELEASE(cast_handle->pAudioClientPlayback);
	}

	// Activate playback device for audio playback.
	hr = cast_handle->pPlaybackDevice->Activate( cast_handle->IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&cast_handle->pAudioClientPlayback);
	if (FAILED(hr))
	{
		*ip_status = SND_DEVICES_DEVICE_ACTIVATION_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_ACTIVATION_FAILED);
	}

	if( cast_handle->pAudioClientPlayback == NULL )
	if (FAILED(hr))
	{
		*ip_status = SND_DEVICES_NULL_PLAYBACK_CLIENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_PLAYBACK_CLIENT);
	}

	if( cast_handle->initializationMode == SND_DEVICES_INIT_FOR_PROCESSING )
	{
		// Activate playback device for volume control.
		hr = cast_handle->pPlaybackDevice->Activate(cast_handle->IID_IAudioEndpointVolume, CLSCTX_ALL, NULL, (void**)&cast_handle->pEndptVolPlayback);
		if (FAILED(hr))
		{
			*ip_status = SND_DEVICES_DEVICE_ACTIVATION_FAILED;
			SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_ACTIVATION_FAILED);
		}
		
		if( cast_handle->pEndptVolPlayback == NULL )
		{
			*ip_status = SND_DEVICES_NULL_VOLUME_ENDPOINT;
			SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_VOLUME_ENDPOINT);
		}

		// Setup playback device volume control callback
		hr = cast_handle->pEndptVolPlayback->RegisterControlChangeNotify( (IAudioEndpointVolumeCallback*)&(cast_handle->EPVolEventsPlayback) );
		if (FAILED(hr))
		{
			*ip_status = SND_DEVICES_REGISTER_FAILED;
			SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_REGISTER_FAILED);
		}
	}

	// Get playback device format, this apparently allocates space and modified the pointer to point to that new space.
	hr = cast_handle->pAudioClientPlayback->GetMixFormat(&pwfx);
	if (FAILED(hr))
	{
		*ip_status = SND_DEVICES_DEVICE_GET_FORMAT_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_GET_FORMAT_FAILED);
	}

	cast_handle->wfxPlayback = *pwfx;

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesFinalSetupPlaybackDevice()
 * Does final format dependent setup of the playback device.
 */
int PT_DECLSPEC sndDevicesFinalSetupPlaybackDevice(PT_HANDLE *hp_sndDevices, int *ip_status)
{
	struct sndDevicesHdlType *cast_handle;
	WAVEFORMATEX *pwfx;
	unsigned int procInfo;
	int numCores;
	DWORD StreamFlags;
	HRESULT hr;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_status = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	if( cast_handle->pAudioClientPlayback == NULL )
	{
		*ip_status = SND_DEVICES_NULL_PLAYBACK_CLIENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_PLAYBACK_CLIENT);
	}

	// Get the current OS
	if( operatingSystemGetSystemProperties(&procInfo, &numCores) != OKAY )
		return(NOT_OKAY);

	// The volume control hide flag can only be used on Win7.
	if( procInfo & OPERATING_SYSTEM_VISTA )
		StreamFlags = 0;
	else
		// The AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE flag hides extra occurences of our volume control in Mixer dialog.
		StreamFlags = AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE;

	// For some reason next call won't take our copied version of this format, likely its expecting WAVEFORMATEXTENSIBLE
	hr = cast_handle->pAudioClientPlayback->GetMixFormat(&pwfx);
	if (FAILED(hr))
	{
		*ip_status = SND_DEVICES_DEVICE_GET_FORMAT_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_GET_FORMAT_FAILED);
	}

    cast_handle->playbackDeviceIsUnavailable = FALSE;
	// Set up playback device for playback in shared mode (multiples apps can play audio).
	hr = cast_handle->pAudioClientPlayback->Initialize( AUDCLNT_SHAREMODE_SHARED, StreamFlags, cast_handle->hnsRequestedDurationPlayback, 0, pwfx, NULL);
	if (FAILED(hr))
	{
        if (hr == AUDCLNT_E_DEVICE_IN_USE)
        {
            cast_handle->playbackDeviceIsUnavailable = TRUE;
        }
		*ip_status = SND_DEVICES_AUDIO_CLIENT_INIT_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_AUDIO_CLIENT_INIT_FAILED);
	}

	// Get the actual size of the allocated buffer in sample sets, which are called frames.
	hr = cast_handle->pAudioClientPlayback->GetBufferSize(&cast_handle->bufferFrameSizePlayback);
	if (FAILED(hr))
	{
		*ip_status = SND_DEVICES_GET_BUFFER_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_GET_BUFFER_FAILED);
	}

	// Initialize variable for available playback buffer space
	cast_handle->numPlaybackFramesAvailableToFill = cast_handle->bufferFrameSizePlayback;

	// Additional step to setup playback.
	hr = cast_handle->pAudioClientPlayback->GetService( cast_handle->IID_IAudioRenderClient, (void**)&(cast_handle->pAudioClientPlaybackRender));
	if (FAILED(hr))
	{
		*ip_status = SND_DEVICES_GET_SERVICE_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_GET_SERVICE_FAILED);
	}

	if( cast_handle->pAudioClientPlaybackRender == NULL )
	{
		*ip_status = SND_DEVICES_PLAYBACK_RENDER_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_PLAYBACK_RENDER_FAILED);
	}

	// Calculate the actual duration of the allocated capture buffer, in REF TIME tics.
	cast_handle->hnsActualDurationPlayback = (REFERENCE_TIME)((double)SND_DEVICES_REFTIMES_PER_SEC * (double)cast_handle->bufferFrameSizePlayback / (double)cast_handle->wfxPlayback.nSamplesPerSec);

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesRestoreDefaultDevice()
 * This function restores as the Windows default playback device one of the real
 * non-DFX playback devices on the machine.
 * It searches the real devices in the following order, using the first active device in this list.
 * userSelectedPlaybackDevice, mostRecentPlaybackDevice, mostRecentDefault device, priorDefaultDevice, originalDefault device.
 * non-DFX device that had been the default playback device.
 */
int PT_DECLSPEC sndDevicesRestoreDefaultDevice(PT_HANDLE *hp_sndDevices, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	int device_num = SND_DEVICES_DEVICE_NOT_PRESENT;
	int result_flag = SND_DEVICES_DEVICE_NOT_PRESENT;

	wchar_t userSelectedPlaybackID[PT_MAX_GENERIC_STRLEN];
	wchar_t mostRecentPlaybackID[PT_MAX_GENERIC_STRLEN];
	wchar_t mostRecentDefaultID[PT_MAX_GENERIC_STRLEN];
	wchar_t priorDefaultID[PT_MAX_GENERIC_STRLEN];
	wchar_t originalDefaultID[PT_MAX_GENERIC_STRLEN];

	int userSelectedPlaybackNum = SND_DEVICES_DEVICE_NOT_PRESENT;
	int mostRecentPlaybackNum = SND_DEVICES_DEVICE_NOT_PRESENT;
	int mostRecentDefaultNum = SND_DEVICES_DEVICE_NOT_PRESENT;
	int priorDefaultNum = SND_DEVICES_DEVICE_NOT_PRESENT;
	int originalDefaultNum = SND_DEVICES_DEVICE_NOT_PRESENT;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	//Check to see if the user has selected the playback device.
	if( sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_USER_SELECTED_PLAYBACK_WIDE, userSelectedPlaybackID) != OKAY )
		return(NOT_OKAY);
	// Get this devices internal index if the device is active.
	if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, userSelectedPlaybackID, &userSelectedPlaybackNum) != OKAY )
		return(NOT_OKAY);

	//Get the ID for the most recent playback device.
	if( sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_MOST_RECENT_PLAYBACK_WIDE, mostRecentPlaybackID) != OKAY )
		return(NOT_OKAY);
	// Get this devices internal index if the device is active.
	if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, mostRecentPlaybackID, &mostRecentPlaybackNum) != OKAY )
		return(NOT_OKAY);

	//Get the ID and device num for the most recent default device.
	if( sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_MOST_RECENT_DEFAULT_WIDE, mostRecentDefaultID) != OKAY )
		return(NOT_OKAY);
	// Get this devices internal index if the device is active.
	if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, mostRecentDefaultID, &mostRecentDefaultNum) != OKAY )
		return(NOT_OKAY);

	//Get the ID and device num for the prior default device.
	if( sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_PRIOR_DEFAULT_WIDE, priorDefaultID) != OKAY )
		return(NOT_OKAY);
	// Get this devices internal index if the device is active.
	if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, priorDefaultID, &priorDefaultNum) != OKAY )
		return(NOT_OKAY);

	//Get the ID for the original default device.
	if( sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_ORIGINAL_DEFAULT_WIDE, originalDefaultID) != OKAY )
		return(NOT_OKAY);
	// Get this devices internal index if the device is active.
	if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, originalDefaultID, &originalDefaultNum) != OKAY )
		return(NOT_OKAY);

	if( userSelectedPlaybackNum != SND_DEVICES_DEVICE_NOT_PRESENT )
		device_num = userSelectedPlaybackNum;

	else if( mostRecentPlaybackNum != SND_DEVICES_DEVICE_NOT_PRESENT )
		device_num = mostRecentPlaybackNum;

	else if( mostRecentDefaultNum != SND_DEVICES_DEVICE_NOT_PRESENT )
		device_num = mostRecentDefaultNum;

	else if( priorDefaultNum != SND_DEVICES_DEVICE_NOT_PRESENT )
		device_num = priorDefaultNum;

	else if( originalDefaultNum != SND_DEVICES_DEVICE_NOT_PRESENT )
		device_num = originalDefaultNum;

	// If an active device was found, set it as the default.
	if( device_num != SND_DEVICES_DEVICE_NOT_PRESENT )
	{
		if( sndDevicesSetDeviceType(hp_sndDevices, SND_DEVICES_DEFAULT, cast_handle->pwszID[device_num], &result_flag) != OKAY )
			return(NOT_OKAY);

		*ip_resultFlag = result_flag;
	}
	else
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;

	return(OKAY);
}
