/*
FxSound
Copyright (C) 2023  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
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

#include "slout.h"
#include "pstr.h"
#include "reg.h"
#include "mry.h"
#include "operatingSystem.h"
#include "u_sndDevices.h"
#include "sndDevices.h"

/*
 * FUNCTION: sndDevicesGetID()
 * DESCRIPTION:  Gets the device guid ID based on the passed in device type.
 * If the device is active passed in string pointer will be filled with ID GUID chars, otherwise a null string is passed back.
 */
int PT_DECLSPEC sndDevicesGetID(PT_HANDLE *hp_sndDevices, int i_deviceType, wchar_t *wcp_ID, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	wchar_t guidID[PT_MAX_GENERIC_STRLEN];
	int device_num;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if ((cast_handle->i_trace_on) && (cast_handle->slout_hdl))
	{
		swprintf(cast_handle->wcp_msg1, L"sndDevicesGetID():: Enter");
		cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	}

	*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;
	wcscpy(wcp_ID, L"");

	switch(i_deviceType)
	{
	case SND_DEVICES_USER_SELECTED_PLAYBACK_DEVICE :
		//Check to see if the user has selected a playback device.
		if( sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_USER_SELECTED_PLAYBACK_WIDE, guidID) != OKAY )
			return(NOT_OKAY);

		// Get this devices internal index if the device is active.
		if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, guidID, &device_num) != OKAY )
			return(NOT_OKAY);

		if( device_num != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_ID, guidID);
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	case SND_DEVICES_TARGETED_REAL_PLAYBACK :
		if( cast_handle->playbackDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_ID, cast_handle->pwszID[ cast_handle->playbackDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	case SND_DEVICES_CAPTURE :
		if( cast_handle->captureDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_ID, cast_handle->pwszID[ cast_handle->captureDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	case SND_DEVICES_VIRTUAL_PLAYBACK_DFX :
		if( cast_handle->dfxDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_ID, cast_handle->pwszID[ cast_handle->dfxDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	/*case SND_DEVICES_VIRTUAL_PLAYBACK_48 :
		if( cast_handle->dfx48DeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_ID, cast_handle->pwszID[ cast_handle->dfx48DeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;*/

	case SND_DEVICES_DEFAULT :
		if( cast_handle->defaultDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_ID, cast_handle->pwszID[ cast_handle->defaultDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	case SND_DEVICES_PRIOR_DEFAULT : // The device that was just default prior to setting DFX as the default.
		if( cast_handle->priorDefaultDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_ID, cast_handle->pwszID[ cast_handle->priorDefaultDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	case SND_DEVICES_PRIOR_PLAYBACK : //Get the ID for the device used for playback prior to the current playback device.
		if( sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_PRIOR_DEFAULT_WIDE, wcp_ID) != OKAY )
			return(NOT_OKAY);
		// Get this devices internal index if the device is active.
		if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, wcp_ID, &device_num) != OKAY )
			return(NOT_OKAY);
		if( device_num == SND_DEVICES_DEVICE_NOT_PRESENT )
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	default:
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT; // Reaching this point means a programming error so NOT_OKAY is returned.
		return(NOT_OKAY);
	}

	if ((cast_handle->i_trace_on) && (cast_handle->slout_hdl))
	{
		swprintf(cast_handle->wcp_msg1, L"sndDevicesGetID():: Result: %i ID %s", *ip_resultFlag, wcp_ID);
		cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
		swprintf(cast_handle->wcp_msg1, L"sndDevicesGetID():: Returns OKAY");
		cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	}

	return(OKAY);
} 

/*
 * FUNCTION: sndDevicesGetFriendlyName()
 * DESCRIPTION: Passed in string pointer will be used to fill with friendly name.
 */
int PT_DECLSPEC sndDevicesGetFriendlyName(PT_HANDLE *hp_sndDevices, int i_deviceType, wchar_t *wcp_Name, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;
	wcscpy(wcp_Name, L"");

	switch(i_deviceType)
	{
	case SND_DEVICES_TARGETED_REAL_PLAYBACK :
		if( cast_handle->playbackDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_Name, cast_handle->deviceFriendlyName[ cast_handle->playbackDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	case SND_DEVICES_CAPTURE :
		if( cast_handle->captureDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_Name, cast_handle->deviceFriendlyName[ cast_handle->captureDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	case SND_DEVICES_VIRTUAL_PLAYBACK_DFX :
		if( cast_handle->dfxDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_Name, cast_handle->deviceFriendlyName[ cast_handle->dfxDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	/*case SND_DEVICES_VIRTUAL_PLAYBACK_48 :
		if( cast_handle->dfx48DeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_Name, cast_handle->deviceFriendlyName[ cast_handle->dfx48DeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;*/

	case SND_DEVICES_DEFAULT :
		if( cast_handle->defaultDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_Name, cast_handle->deviceFriendlyName[ cast_handle->defaultDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	default:
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT; // Reaching this point means a programming error so NOT_OKAY is returned.
		return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesGetDeviceName()
 * DESCRIPTION: Passed in string pointer will be used to fill with description name, for example "Speakers".
 */
int PT_DECLSPEC sndDevicesGetDeviceName(PT_HANDLE *hp_sndDevices, int i_deviceType, wchar_t *wcp_Description, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if ((cast_handle->i_trace_on) && (cast_handle->slout_hdl))
	{
			cast_handle->slout_hdl->Message_Wide(FIRST_LINE, L"sndDevicesGetDeviceName():: Enters");
	}

	*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;
	wcscpy(wcp_Description, L"");

	switch(i_deviceType)
	{
	case SND_DEVICES_TARGETED_REAL_PLAYBACK :
		if( cast_handle->playbackDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_Description, cast_handle->deviceDescription[ cast_handle->playbackDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	case SND_DEVICES_CAPTURE :
		if( cast_handle->captureDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_Description, cast_handle->deviceDescription[ cast_handle->captureDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	case SND_DEVICES_VIRTUAL_PLAYBACK_DFX :
		if( cast_handle->dfxDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_Description, cast_handle->deviceDescription[ cast_handle->dfxDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	case SND_DEVICES_DEFAULT :
		if( cast_handle->defaultDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			wcscpy(wcp_Description, cast_handle->deviceDescription[ cast_handle->defaultDeviceNum ] );
		else
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	default:
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT; // Reaching this point means a programming error so NOT_OKAY is returned.
		
		if ((cast_handle->i_trace_on) && (cast_handle->slout_hdl))
		{
			swprintf(cast_handle->wcp_msg1, PT_MAX_GENERIC_STRLEN, L"sndDevicesGetDeviceName() returns NOT_OKAY with result flag %i", *ip_resultFlag);
			cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
		}

		return(NOT_OKAY);
	}

	if ((cast_handle->i_trace_on) && (cast_handle->slout_hdl))
	{
		swprintf(cast_handle->wcp_msg1, L"sndDevicesGetDeviceName() returns OKAY with result flag %i", *ip_resultFlag);
		cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	}

	return(OKAY);
} 

/*
 * FUNCTION: sndDevicesGetFriendlyNameFromID()
 * DESCRIPTION: Passed in string pointer will be used to fill with friendly name.
 */
int PT_DECLSPEC sndDevicesGetFriendlyNameFromID(PT_HANDLE *hp_sndDevices, wchar_t *wcp_ID, wchar_t *wcp_Name, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	int i;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
	wcscpy(wcp_Name, L"");

	for(i=0; i<cast_handle->totalNumDevices; i++)
	{
		// Check ID strings to see if they match
		if( cast_handle->pwszID[i] != NULL && wcscmp(cast_handle->pwszID[i], wcp_ID) == 0 )
		{
			wcscpy(wcp_Name, cast_handle->deviceFriendlyName[i] );
			*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;
			break;
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesGetDeviceNameFromID()
 * DESCRIPTION: Passed in string pointer will be used to fill with description, ie "Speakers".
 */
int PT_DECLSPEC sndDevicesGetDeviceNameFromID(PT_HANDLE *hp_sndDevices, wchar_t *wcp_ID, wchar_t *wcp_Description, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	int i;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
	wcscpy(wcp_Description, L"");

	for(i=0; i<cast_handle->totalNumDevices; i++)
	{
		// Check ID strings to see if they match
		if( cast_handle->pwszID[i] != NULL && wcscmp(cast_handle->pwszID[i], wcp_ID) == 0 )
		{
			wcscpy(wcp_Description, cast_handle->deviceDescription[i] );
			*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;
			break;
		}
	}

	return(OKAY);
}

/*
* FUNCTION: sndDevicesGetNumberOfChannelsFromID()
* DESCRIPTION: Passed in string pointer will be used to fill with friendly name.
*/
int PT_DECLSPEC sndDevicesGetNumberOfChannelsFromID(PT_HANDLE *hp_sndDevices, wchar_t *wcp_ID, int *ip_numChannels, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;

	for (auto index = 0; index < cast_handle->totalNumDevices; index++)
	{
		// Check ID strings to see if they match
		if (cast_handle->pwszID[index] != NULL && wcscmp(cast_handle->pwszID[index], wcp_ID) == 0)
		{
			*ip_numChannels = cast_handle->deviceNumChannel[index];
			*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;
			break;
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesGetAllRealDeviceIDs()
 * DESCRIPTION: 
 *
 *  Passes back an array of strings of all the guids of all the real devices.
 *
 */
int PT_DECLSPEC sndDevicesGetAllRealDeviceIDs(PT_HANDLE *hp_sndDevices, wchar_t ***wcppp_IDs, int *ip_numDevices)
{
	struct sndDevicesHdlType *cast_handle;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_numDevices = cast_handle->numRealDevices;

	*wcppp_IDs = (wchar_t **)cast_handle->pwszIDRealDevices;

	return(OKAY);
} 

/*
 * FUNCTION: sndDevicesGetFormatFromID()
 * DESCRIPTION: Fills in the passed in WAVEFORMATEX structure with the format of the specified device, by its guid ID.
 */
int PT_DECLSPEC sndDevicesGetFormatFromID(PT_HANDLE *hp_sndDevices, wchar_t *wcp_ID, WAVEFORMATEX *p_wfx, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	IAudioClient *pAudioClient;
	WAVEFORMATEX *pwfx;
	HRESULT hr;
	int i;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;

	if(cast_handle->totalNumDevices <= 0)
		return(OKAY);

	for(i=0; i<cast_handle->totalNumDevices; i++)
	{
		// Check ID string to see if this is the requested device,
		if( cast_handle->pwszID[i] != NULL && wcscmp(cast_handle->pwszID[i], wcp_ID) == 0 )
		{
			// Activate capture device for audio so we can get its format
			hr = cast_handle->pAllDevices[i]->Activate(cast_handle->IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
			if( hr != S_OK )
			{
				*ip_resultFlag = SND_DEVICES_DEVICE_ACTIVATION_FAILED;

				SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_ACTIVATION_FAILED);
			}

			// Get the format.
			hr = pAudioClient->GetMixFormat(&(pwfx));
			if( hr != S_OK )
			{
				*ip_resultFlag = SND_DEVICES_DEVICE_GET_FORMAT_FAILED;

				SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_GET_FORMAT_FAILED);
			}

			// Copy the format structure.
			*p_wfx = *pwfx;

			*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

			if(pAudioClient != NULL)
				pAudioClient->Release();

			break;  // Jump out of loop since we found the requested ID.
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesGetIconFileName()
 * DESCRIPTION: Gets the icon file name of the specified device.
 */
int PT_DECLSPEC sndDevicesGetIconFileName(PT_HANDLE *hp_sndDevices, int i_deviceType, wchar_t *wcp_iconFileName, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	HRESULT hr;
	IPropertyStore *pProps = NULL;
	PROPVARIANT varName;
	int i_device_num;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if ((cast_handle->i_trace_on) && (cast_handle->slout_hdl))
	{
		cast_handle->slout_hdl->Message_Wide(FIRST_LINE, L"sndDevicesGetIconFileName():: Enter");
	}

	*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	// Init string to '\0' to avoid crashes from any attempted string manipulation after unexpected return.
	wcp_iconFileName[0] = '\0';

	if ((cast_handle->i_trace_on) && (cast_handle->slout_hdl))
	{
		cast_handle->slout_hdl->Message_Wide(FIRST_LINE, L"sndDevices_UtilsGetIndexFromType() called");
	}

	if( sndDevices_UtilsGetIndexFromType(hp_sndDevices, i_deviceType, &i_device_num) != OKAY)
		return(NOT_OKAY);

	if( i_device_num == SND_DEVICES_DEVICE_NOT_PRESENT )
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		return(OKAY);
	}

	if( cast_handle->pAllDevices[i_device_num] == NULL )
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		return(OKAY);
	}

	if ((cast_handle->i_trace_on) && (cast_handle->slout_hdl))
	{
		cast_handle->slout_hdl->Message_Wide(FIRST_LINE, L"OpenPropertyStore() called");
	}

	hr = cast_handle->pAllDevices[i_device_num]->OpenPropertyStore(STGM_READ, &pProps);
	if (FAILED(hr))
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_OPEN_PROPS_FAILED);
	}

	// Initialize container for property value.
	PropVariantInit(&varName);

	if ((cast_handle->i_trace_on) && (cast_handle->slout_hdl))
	{
		cast_handle->slout_hdl->Message_Wide(FIRST_LINE, L"GetValue() called");
	}

	hr = pProps->GetValue(PKEY_DeviceClass_IconPath, &varName);
	if (FAILED(hr))
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_ICON_PROP_GET_FAILED);
	}

	if( varName.pwszVal != NULL )
		wcscpy(wcp_iconFileName, varName.pwszVal);
	else
		wcp_iconFileName[0] = '\0';

	PropVariantClear(&varName);
	SAFE_RELEASE(pProps)

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesGetLoopStopFlag()
 * DESCRIPTION: Gets the value of the
 */
int PT_DECLSPEC sndDevicesGetLoopStopFlag(PT_HANDLE *hp_sndDevices,  int *ip_loopStopFlag)
{
	struct sndDevicesHdlType *cast_handle;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_loopStopFlag = cast_handle->stopAudioCaptureAndPlaybackLoop;

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesGetBuffersForProcessing()
 * DESCRIPTION: Provides the buffers so audio processing can be applied, with processing to be done in place.
 * This function must be only placed directly after sndDevicesDoCapture and thus right before sndDevicesDoPlayback.
 * Buffers are always provided in 32 bit floating point format, with sampling freq and channels in p_wfxDfx.
 */
int PT_DECLSPEC sndDevicesGetBuffersForProcessing(PT_HANDLE *hp_sndDevices, float **fpp_buffer, int *ip_numSampleSets, WAVEFORMATEX **pp_wfxDfx)
{
	struct sndDevicesHdlType *cast_handle;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*fpp_buffer = cast_handle->fPlaybackBuf;

	*ip_numSampleSets = cast_handle->capturedFramesCount;

	*pp_wfxDfx = &(cast_handle->wfxDfxProcessing);

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesGetTotalNumDevices()
 * DESCRIPTION: Gets the total number of active playback devices, including the DFX devices.
 */
int PT_DECLSPEC sndDevicesGetTotalNumDevices(PT_HANDLE *hp_sndDevices,  int *ip_totalNumDevices)
{
	struct sndDevicesHdlType *cast_handle;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_totalNumDevices = cast_handle->totalNumDevices;

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesGetIDFromIndex()
 * DESCRIPTION:  Gets the guid ID for the passed in internal index number.
 * If the passed index number is not a valid device, returns "" (NULL).
 */
int PT_DECLSPEC sndDevicesGetIDFromIndex(PT_HANDLE *hp_sndDevices, int i_index, wchar_t *wcp_ID)
{
	struct sndDevicesHdlType *cast_handle;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	wcscpy(wcp_ID, L"");

	if( (cast_handle->totalNumDevices <= 0) || ( i_index >= cast_handle->totalNumDevices) || ( i_index < 0) )
		return(OKAY);

	wcscpy(wcp_ID, cast_handle->pwszID[i_index]);

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesGetDefaultDeviceSelectionMode()
 * DESCRIPTION: Sets the mode for default device selection, use
 * SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_OFF or SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_ON
 */
int PT_DECLSPEC sndDevicesGetDefaultDeviceSelectionMode(PT_HANDLE *hp_sndDevices, int *ip_mode)
{
	struct sndDevicesHdlType *cast_handle;
	wchar_t modeSetting[PT_MAX_GENERIC_STRLEN];

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if( sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_DEFAULT_DEVICE_SELECTION_MODE, modeSetting) != OKAY )
		return(NOT_OKAY);

	if( wcscmp(modeSetting, L"off") == 0 )
		*ip_mode = SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_OFF;
	else
		*ip_mode = SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_ON;

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesGetBufferSizeMilliSecs()
 * DESCRIPTION: Gets the average audio buffer length and delay in milliSecs.
 * Min and Max values are SND_DEVICES_CAPTURE_BUFFER_MIN_SIZE_MILLI_SECS and SND_DEVICES_CAPTURE_BUFFER_MAX_SIZE_MILLI_SECS
 * Set iTypeFlag to SND_DEVICES_USER_SETTING	or SND_DEVICES_DEFAULT_SETTING
 */
int PT_DECLSPEC sndDevicesGetBufferSizeMilliSecs(PT_HANDLE *hp_sndDevices, int iTypeFlag, int *ipBufferSize)
{
	struct sndDevicesHdlType *cast_handle;
	wchar_t regVal[PT_MAX_GENERIC_STRLEN];

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if( iTypeFlag == SND_DEVICES_USER_SETTING )
	{
		// Read the user buffer_size from the registry
		if( sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_USER_BUFFER_SIZE, regVal) != OKAY)
			return(NOT_OKAY);

		if( wcscmp(regVal, L"") != 0)
			swscanf(regVal, L"%d", ipBufferSize);
		else
			*ipBufferSize = cast_handle->bufferSizeMilliSecs;
	}
	else
	{
		// Read the default buffer_size from the registry
		if( sndDeviceReadFromRegistry(hp_sndDevices, REG_LOCAL_MACHINE, SND_DEVICES_REGISTRY_DEFAULT_BUFFER_SIZE, regVal) != OKAY)
			return(NOT_OKAY);

		if( wcscmp(regVal, L"") != 0)
			swscanf(regVal, L"%d", ipBufferSize);
		else
			*ipBufferSize = SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS;
	}

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesGetRecommendedBufferSizeMilliSecs()
 * DESCRIPTION: Gets the recommended buffer size in millisecs, based on CPU and OS characteristics.
 */
int PT_DECLSPEC sndDevicesGetRecommendedBufferSizeMilliSecs(PT_HANDLE *hp_sndDevices, int *ipBufferSize)
{
	struct sndDevicesHdlType *cast_handle;
	unsigned int procInfo;
	int numCores;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ipBufferSize = SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS;

	// Get the current OS
	if( operatingSystemGetSystemProperties(&procInfo, &numCores) != OKAY )
		return(NOT_OKAY);

	if( procInfo & OPERATING_SYSTEM_CPU_32 )
	{
		// 32 bit processor and OS case
		if( procInfo & OPERATING_SYSTEM_VISTA_32 )
			*ipBufferSize = SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS_32BIT_VISTA_32BIT_CPU;
		else
			*ipBufferSize = SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS_32BIT_OS_32BIT_CPU;
	}
	else
	{
		// 64 bit processor case
		if( procInfo & OPERATING_SYSTEM_32_BIT_OS )
		{
			// 32 bit OS on 64 bit processor
			if( procInfo & OPERATING_SYSTEM_VISTA_32 )
				*ipBufferSize = SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS_32BIT_VISTA_64BIT_CPU;
			else
				*ipBufferSize = SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS_32BIT_OS_64BIT_CPU;
		}
		else
		{
			// 64 bit OS on 64 bit processor
			*ipBufferSize = SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS_64BIT_OS;
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesGetPlayBackStatus()
 * NOTE: this function is for use with DFX processing mode, not Max Recorder file playback.
 * DESCRIPTION: Gets a flag on whether DFX processing playback is active or has stopped, value is either:
 * SND_DEVICES_PLAYBACK_IS_STOPPED or SND_DEVICES_PLAYBACK_IS_ACTIVE.
 */
int PT_DECLSPEC sndDevicesGetPlayBackStatus(PT_HANDLE *hp_sndDevices,  int *ip_playbackIsActiveFlag)
{
	struct sndDevicesHdlType *cast_handle;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_playbackIsActiveFlag = cast_handle->playbackIsActive;

	return(OKAY);
}


int PT_DECLSPEC sndDevicesGetNumMonoDevices(PT_HANDLE *hp_sndDevices, int *ip_numMonoDevices)
{
	int i_deviceIndex;
	int i_numChannels;
	int i_resultFlag;

	struct sndDevicesHdlType *cast_handle;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_numMonoDevices = 0;

	for (i_deviceIndex = 0; i_deviceIndex < cast_handle->numRealDevices; i_deviceIndex++)
	{
		if (wcslen(cast_handle->pwszIDRealDevices[i_deviceIndex]) > 0)
		{
			if (sndDevicesGetNumberOfChannelsFromID(hp_sndDevices, cast_handle->pwszIDRealDevices[i_deviceIndex], &i_numChannels, &i_resultFlag) != OKAY)
				return(NOT_OKAY);

			if (i_numChannels == 1)
			{
				(*ip_numMonoDevices)++;
			}
		}
	}

	return(OKAY);
}

int PT_DECLSPEC sndDevicesGetPlaybackDeviceAvialblility(PT_HANDLE* hp_sndDevices, BOOL* bp_avilablility)
{
    struct sndDevicesHdlType *cast_handle;

    *bp_avilablility = TRUE;

    cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

    if (cast_handle == NULL)
        return(NOT_OKAY);

    if (cast_handle->playbackDeviceIsUnavailable)
    {
        *bp_avilablility = FALSE;
    }

    return(OKAY);
}
