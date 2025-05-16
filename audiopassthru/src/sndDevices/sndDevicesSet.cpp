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
#include "pstr.h"
//#include "File.h"
#include "u_sndDevices.h"
#include "sndDevices.h"

/*
 * FUNCTION: sndDevicesSetDeviceType()
 * DESCRIPTION: Sets the type (ie default, playback, capture) of the device thas has the specified in guid ID.
 * NOTE - currently these calls can be safely made prior to the system being in run mode but for the calls
 * to be made in run mode in general a sndDevicesResetSystem() call should be made after this call is made.
 * However, the default playback device (SND_DEVICES_DEFAULT) can be changed in run mode without requiring a reset call.
 */
int PT_DECLSPEC sndDevicesSetDeviceType(PT_HANDLE *hp_sndDevices, int i_deviceType, wchar_t *wcp_ID, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	int device_index_num;
	// NOTE - using vista version, works on both vista and win7.
	IPolicyConfigVista *pPolicyConfig;
	//IPolicyConfig *pPolicyConfig;
	ERole reserved = eConsole;
	HRESULT hr;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	// Find the internal index number for the passed in guid ID.
	if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, wcp_ID, &device_index_num) != OKAY )
		return(NOT_OKAY);

	if( device_index_num == SND_DEVICES_DEVICE_NOT_PRESENT )
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		return(OKAY);
	}

	switch(i_deviceType)
	{
	case SND_DEVICES_TARGETED_REAL_PLAYBACK :
		cast_handle->playbackDeviceNum = device_index_num;
		cast_handle->pPlaybackDevice = cast_handle->pAllDevices[device_index_num];
		if( cast_handle->pPlaybackDevice == NULL )
		{
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
			SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_PLAYBACK_DEVICE);
		}
		break;

	case SND_DEVICES_CAPTURE :
		cast_handle->captureDeviceNum = device_index_num;
		cast_handle->pCaptureDevice = cast_handle->pAllDevices[device_index_num];
		if( cast_handle->pCaptureDevice == NULL )
		{
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
			SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_CAPTURE_DEVICE);
		}
		break;

	// We may not want to allow these DFX calls, they probably won't ever be required.
	case SND_DEVICES_VIRTUAL_PLAYBACK_DFX :
		cast_handle->dfxDeviceNum = device_index_num;
		break;

	case SND_DEVICES_DEFAULT :
		hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), NULL, CLSCTX_ALL,
			                   __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfig);
		if (SUCCEEDED(hr))
		{
			hr = pPolicyConfig->SetDefaultEndpoint(cast_handle->pwszID[device_index_num], reserved);
			pPolicyConfig->Release();

			// Store the current default device number and set the new one.
			cast_handle->priorDefaultDeviceNum = cast_handle->defaultDeviceNum;
			cast_handle->defaultDeviceNum = device_index_num;

			return OKAY;
		}
		else
		{
			*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
			SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_INSTANCE_CREATE_FAILED);
		}
		break;

	default:
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT; // Should only get here if there is a programming error.
		return(NOT_OKAY);
	}

	return(OKAY);
} 

/*
 * FUNCTION: sndDevicesSetIconFileName()
 * DESCRIPTION: Sets the icon file name of the specified device.
 */
int PT_DECLSPEC sndDevicesSetIconFileName(PT_HANDLE *hp_sndDevices, int i_deviceType, wchar_t *wcp_iconFileName, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	HRESULT hr;
	IPropertyStore *pProps = NULL;
	PROPVARIANT varName;
	int i_device_num;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	if( sndDevices_UtilsGetIndexFromType(hp_sndDevices, i_deviceType, &i_device_num) != OKAY)
		return(NOT_OKAY);

	if( i_device_num == SND_DEVICES_DEVICE_NOT_PRESENT )
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		return(OKAY);
	}

	if( cast_handle->pAllDevices[i_device_num] == NULL )
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_NOT_PRESENT);

	hr = cast_handle->pAllDevices[i_device_num]->OpenPropertyStore(STGM_READWRITE, &pProps);
	if (FAILED(hr))
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_OPEN_PROPS_FAILED);
	}

	hr = InitPropVariantFromString(wcp_iconFileName, &varName);
	if (FAILED(hr))
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_INIT_PROP_FAILED;
		SAFE_RELEASE(pProps)
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_INIT_PROP_FAILED);
	}

	hr = pProps->SetValue(PKEY_DeviceClass_IconPath, varName);
	if( hr != S_OK)
	{
		*ip_resultFlag = SND_DEVICES_PROPERTY_SET_FAILED; // Will fail if .exe does not have admin privilege.
		PropVariantClear(&varName);
		SAFE_RELEASE(pProps)
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_PROPERTY_SET_FAILED);
	}

	PropVariantClear(&varName);
	SAFE_RELEASE(pProps)

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesSetDeviceName()
 * DESCRIPTION: Sets the device "description name" of the specified device, for example "Speakers".
 */
int PT_DECLSPEC sndDevicesSetDeviceName(PT_HANDLE *hp_sndDevices, int i_deviceType, wchar_t *wcp_deviceName, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	HRESULT hr;
	IPropertyStore *pProps = NULL;
	PROPVARIANT varName;
	int i_device_num;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	if( sndDevices_UtilsGetIndexFromType(hp_sndDevices, i_deviceType, &i_device_num) != OKAY)
		return(NOT_OKAY);

	if( i_device_num == SND_DEVICES_DEVICE_NOT_PRESENT )
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		return(OKAY);
	}

	if( cast_handle->pAllDevices[i_device_num] == NULL )
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_NOT_PRESENT);

	hr = cast_handle->pAllDevices[i_device_num]->OpenPropertyStore(STGM_READWRITE, &pProps);
	if (FAILED(hr))
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_OPEN_PROPS_FAILED);
	}

	hr = InitPropVariantFromString(wcp_deviceName, &varName);
	if (FAILED(hr))
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_INIT_PROP_FAILED;
		SAFE_RELEASE(pProps)
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_INIT_PROP_FAILED);
	}

	hr = pProps->SetValue(PKEY_Device_DeviceDesc, varName);
	if( hr != S_OK)
	{
		*ip_resultFlag = SND_DEVICES_PROPERTY_SET_FAILED; // Will fail if .exe does not have admin privilege.
		PropVariantClear(&varName);
		SAFE_RELEASE(pProps)
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_PROPERTY_SET_FAILED);
	}

	PropVariantClear(&varName);
	SAFE_RELEASE(pProps)

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesSetDefaultDeviceSelectionMode()
 * DESCRIPTION: Sets the mode for default device selection, use
 * SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_OFF or SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_ON
 */
int PT_DECLSPEC sndDevicesSetDefaultDeviceSelectionMode(PT_HANDLE *hp_sndDevices, int i_mode)
{
	struct sndDevicesHdlType *cast_handle;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if( i_mode == SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_ON )
	{
		if( sndDevicesWriteToRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_DEFAULT_DEVICE_SELECTION_MODE, L"on") != OKAY )
			return(NOT_OKAY);
	}
	else
	{
		if( sndDevicesWriteToRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_DEFAULT_DEVICE_SELECTION_MODE, L"off") != OKAY )
			return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesSetDeviceEnabledStatus()
 * DESCRIPTION: Sets specified devices enabled status, TRUE for enabled, FALSE for disabled.
 * Does not disable the device if the DFX guid can not be read.
 * NOTE - this function requires different implementation for Win7 versus Vista.
 */
int PT_DECLSPEC sndDevicesSetDeviceEnabledStatus(PT_HANDLE *hp_sndDevices, int i_deviceType, BOOL bEnabledFlag, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	IPolicyConfigVista *pPolicyConfigVista;
	IPolicyConfig *pPolicyConfigWin7;
	unsigned int procInfo;
	int numCores;
	wchar_t guidID[PT_MAX_GENERIC_STRLEN];
	HRESULT hr;
   int i_device_num;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	if( bEnabledFlag == FALSE )
	{
		// Don't disable the device if the DFX guid has not been written.
		if( sndDeviceReadFromRegistry(hp_sndDevices, REG_LOCAL_MACHINE, SND_DEVICES_REGISTRY_DFX_GUID, guidID) != OKAY)
			return(NOT_OKAY);

		if( wcscmp(guidID, L"") == 0)
			return(OKAY);
	}

	if( sndDevices_UtilsGetIndexFromType(hp_sndDevices, i_deviceType, &i_device_num) != OKAY)
		return(NOT_OKAY);

	if( i_device_num == SND_DEVICES_DEVICE_NOT_PRESENT )
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		return(OKAY);
	}

	// Get the current OS
	if( operatingSystemGetSystemProperties(&procInfo, &numCores) != OKAY )
		return(NOT_OKAY);

	// Exit if on XP or earlier
	if( procInfo & OPERATING_SYSTEM_XP )
	{
		*ip_resultFlag = SND_DEVICES_PROPERTY_SET_FAILED;
		return(OKAY);
	}

	if( procInfo & OPERATING_SYSTEM_VISTA )
	{
		/* Vista version */
		hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), NULL, CLSCTX_ALL,
									 __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfigVista);
		if (SUCCEEDED(hr))
		{
			hr = pPolicyConfigVista->SetEndpointVisibility(cast_handle->pwszID[i_device_num], bEnabledFlag);
			pPolicyConfigVista->Release();
		}
	}
	else
	{
		/* Win7 version */ 
		hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL,
									 __uuidof(IPolicyConfig), (LPVOID *)&pPolicyConfigWin7);
		if (SUCCEEDED(hr))
		{
			hr = pPolicyConfigWin7->SetEndpointVisibility(cast_handle->pwszID[i_device_num], bEnabledFlag);
			pPolicyConfigWin7->Release();
		}
	}

	if( hr != S_OK )
		*ip_resultFlag = SND_DEVICES_PROPERTY_SET_FAILED;

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesSetDeviceEnabledStatusFromGuid()
 * DESCRIPTION: Sets specified devices enabled status, TRUE for enabled, FALSE for disabled.
 * Does not disable the device if the DFX guid can not be read.
 * NOTE - this function requires different implementation for Win7 versus Vista.
 */
int PT_DECLSPEC sndDevicesSetDeviceEnabledStatusFromGuid(PT_HANDLE *hp_sndDevices, wchar_t *wcp_ID, BOOL bEnabledFlag, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	IPolicyConfigVista *pPolicyConfigVista;
	IPolicyConfig *pPolicyConfigWin7;
	HRESULT hr;
	wchar_t guidID[PT_MAX_GENERIC_STRLEN];
	unsigned int procInfo;
	int numCores;
	
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	if( bEnabledFlag == FALSE )
	{
		// Don't disable the device if the DFX guid has not been written.
		if( sndDeviceReadFromRegistry(hp_sndDevices, REG_LOCAL_MACHINE, SND_DEVICES_REGISTRY_DFX_GUID, guidID) != OKAY)
			return(NOT_OKAY);

		if( wcscmp(guidID, L"") == 0)
			return(OKAY);
	}

	// Get the current OS
	if( operatingSystemGetSystemProperties(&procInfo, &numCores) != OKAY )
		return(NOT_OKAY);

	// Exit if on XP or earlier
	if( procInfo & OPERATING_SYSTEM_XP )
	{
		*ip_resultFlag = SND_DEVICES_PROPERTY_SET_FAILED;
		return(OKAY);
	}

	if( procInfo & OPERATING_SYSTEM_VISTA )
	{
		/* Vista version */
		hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), NULL, CLSCTX_ALL,
									 __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfigVista);
		if (SUCCEEDED(hr))
		{
			hr = pPolicyConfigVista->SetEndpointVisibility(wcp_ID, bEnabledFlag);
			pPolicyConfigVista->Release();
		}
	}
	else
	{
		/* Win7 and Win8 version */ 
		hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL,
									 __uuidof(IPolicyConfig), (LPVOID *)&pPolicyConfigWin7);
		if (SUCCEEDED(hr))
		{
			hr = pPolicyConfigWin7->SetEndpointVisibility(wcp_ID, bEnabledFlag);
			pPolicyConfigWin7->Release();
		}
	}

	if( hr != S_OK )
	{
		*ip_resultFlag = SND_DEVICES_PROPERTY_SET_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_PROPERTY_SET_FAILED);
	}

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesSetDfxDeviceSampleRateAndChannels()
 * DESCRIPTION: Sets the sample rate of the DFX device to either SND_DEVICES_DFX_SAMP_FREQ_44_1
 * or SND_DEVICES_DFX_SAMP_FREQ_48 and sets the num channels to specified setting.
 */
int PT_DECLSPEC sndDevicesSetDfxDeviceSampleRateAndChannels(PT_HANDLE *hp_sndDevices, int i_sampRateFlag, int iNumChannels, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	IPolicyConfigVista *pPolicyConfigVista;
	WAVEFORMATEXTENSIBLE *pwfx;
	HRESULT hr;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	if( cast_handle->dfxDeviceNum == SND_DEVICES_DEVICE_NOT_PRESENT )
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		 return(OKAY);
	}

	if( (iNumChannels < SND_DEVICES_MIN_NUM_CHANS) || (iNumChannels > SND_DEVICES_MAX_NUM_CHANS) )
		return(NOT_OKAY);

	/* Uses Vista version of IPolicyConfig, works on Vista and Win7 */
	hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), 
		NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfigVista);
	if (FAILED(hr))
	{
		*ip_resultFlag = SND_DEVICES_INSTANCE_CREATE_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_INSTANCE_CREATE_FAILED);
	}

	// TRUE -> OEM default format, FALSE-> current format.
	hr = pPolicyConfigVista->GetDeviceFormat(cast_handle->pwszID[cast_handle->dfxDeviceNum], FALSE, &pwfx );
	if (FAILED(hr))
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_GET_FORMAT_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_GET_FORMAT_FAILED);
	}

	if( i_sampRateFlag == SND_DEVICES_DFX_SAMP_FREQ_44_1 )
		pwfx->Format.nSamplesPerSec = 44100;
	else
		pwfx->Format.nSamplesPerSec = 48000;

	pwfx->Format.nChannels = iNumChannels;

	pwfx->Format.nBlockAlign = iNumChannels * pwfx->Format.wBitsPerSample/8;

	pwfx->Format.nAvgBytesPerSec = pwfx->Format.nSamplesPerSec * pwfx->Format.nBlockAlign;

	if( iNumChannels == 2 )
		pwfx->dwChannelMask = 3;		// Set both stereo channels on
	else if( iNumChannels == 6 )
		pwfx->dwChannelMask = 1551;	// Seems like for 5.1 channel mask should be 63. But card originally in
												// 5.1 would come in with mask of 1551. Setting to 63 causes error.
	else
		pwfx->dwChannelMask = 1599;	// As in 5.1 case, channel mask has to be different than you would expect.

	hr = pPolicyConfigVista->SetDeviceFormat(cast_handle->pwszID[cast_handle->dfxDeviceNum], pwfx, NULL);
	if (FAILED(hr))
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_SET_FORMAT_FAILED;
		pPolicyConfigVista->Release();
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_SET_FORMAT_FAILED);
	}

	pPolicyConfigVista->Release();

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesSetBufferSizeMilliSecs()
 * DESCRIPTION: Sets the user setting for average audio buffer length and delay in milliSecs.
 * Min and Max values are SND_DEVICES_CAPTURE_BUFFER_MIN_SIZE_MILLI_SECS and SND_DEVICES_CAPTURE_BUFFER_MAX_SIZE_MILLI_SECS
 */
int PT_DECLSPEC sndDevicesSetBufferSizeMilliSecs(PT_HANDLE *hp_sndDevices, int iBufferSize)
{
	struct sndDevicesHdlType *cast_handle;
	wchar_t bufLenStr[PT_MAX_GENERIC_STRLEN];

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);
	
	if( iBufferSize < SND_DEVICES_CAPTURE_BUFFER_MIN_SIZE_MILLI_SECS )
		iBufferSize = SND_DEVICES_CAPTURE_BUFFER_MIN_SIZE_MILLI_SECS;
		
	if( iBufferSize > SND_DEVICES_CAPTURE_BUFFER_MAX_SIZE_MILLI_SECS )
		iBufferSize = SND_DEVICES_CAPTURE_BUFFER_MAX_SIZE_MILLI_SECS;

	cast_handle->bufferSizeMilliSecs = iBufferSize;

	swprintf(bufLenStr, L"%d", iBufferSize);

	// Write the buffer size to the registry.
	if( sndDevicesWriteToRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_USER_BUFFER_SIZE, bufLenStr) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesRecordingSetTmpFolder()
 * DESCRIPTION: Sets the file path for temporary files used in the recording process.
 * It also clears the folder of any remaining partial pre-encoded files with either the
 * SND_DEVICES_RECORD_CACHE_PRE_ENCODED_PARTIAL_FILES_ENCRYPTED_EXTENSION or
 * SND_DEVICES_RECORD_CACHE_PRE_ENCODED_PARTIAL_FILES_UNENCRYPTED_EXTENSION extensions.
 */
/*
int PT_DECLSPEC sndDevicesRecordingSetTmpFolder(PT_HANDLE *hp_sndDevices, wchar_t *wcp_filePath)
{
	struct sndDevicesHdlType *cast_handle;
	wchar_t partialEncrypted[PT_MAX_PATH_STRLEN];
	wchar_t partialUnEncrypted[PT_MAX_PATH_STRLEN];

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if(wcp_filePath == NULL)
		return(NOT_OKAY);

	wcscpy(cast_handle->recordedFilePath, wcp_filePath); 

	// Strip the training / if its there
	if( pstrRemoveTrailingBackslash_Wide(cast_handle->recordedFilePath) != OKAY )
		return(NOT_OKAY);

	// Delete any existing "partial" pre-encrypted files from threads that went dead for some reason.
	swprintf(partialEncrypted,   L"%s\\*.%s", cast_handle->recordedFilePath, SND_DEVICES_RECORD_CACHE_PRE_ENCODED_PARTIAL_FILES_ENCRYPTED_EXTENSION);
	swprintf(partialUnEncrypted, L"%s\\*.%s", cast_handle->recordedFilePath, SND_DEVICES_RECORD_CACHE_PRE_ENCODED_PARTIAL_FILES_UNENCRYPTED_EXTENSION);

	if( fileRemoveWithWildcard_Wide(partialEncrypted,   cast_handle->slout_hdl) != OKAY )
		return(NOT_OKAY);

	if( fileRemoveWithWildcard_Wide(partialUnEncrypted, cast_handle->slout_hdl) != OKAY )
		return(NOT_OKAY);

	return(OKAY);
}
*/

/*
 * FUNCTION: sndDevicesSetRecordPrecision()
 * DESCRIPTION: Sets MP3 encoding bit rate and the number channels in the final encoded version.
 * iBitRate is the MP3 encoding bit rate in kbs and must be 64, 96, 128, 192, 256 or 320
 * i_ForceMonoFlag forces a mono MP3 file to be created even if the input is stereo.
 * This is useful for making long podcasts to limit file size.
 * b_PrecisionChangedFlag signifies that the user has changed these values and any cache pre-encrypted files should be deleted.
 */
/*
int PT_DECLSPEC sndDevicesSetRecordPrecision(PT_HANDLE *hp_sndDevices, int i_BitRate, int i_ForceMonoFlag, bool b_PrecisionChangedFlag)
{
	struct sndDevicesHdlType *cast_handle;
	wchar_t preEncodedEncryptedFiles[PT_MAX_PATH_STRLEN];
	wchar_t preEncodedUnEncryptedFiles[PT_MAX_PATH_STRLEN];

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if( (i_BitRate != 64) && (i_BitRate != 96) && (i_BitRate != 128) && (i_BitRate != 192) && (i_BitRate != 256) && (i_BitRate != 320) )
		return(NOT_OKAY);

	cast_handle->i_BitRate = i_BitRate;
	cast_handle->i_ForceMonoFlag = i_ForceMonoFlag;

	// If either precision setting is changed, delete all existing pre-encoded files.
	if(b_PrecisionChangedFlag)
	{
		swprintf(preEncodedEncryptedFiles,   L"%s\\*.%s", cast_handle->recordedFilePath, SND_DEVICES_RECORD_CACHE_PRE_ENCODED_FINAL_FILES_ENCRYPTED_EXTENSION);
		swprintf(preEncodedUnEncryptedFiles, L"%s\\*.%s", cast_handle->recordedFilePath, SND_DEVICES_RECORD_CACHE_PRE_ENCODED_FINAL_FILES_UNENCRYPTED_EXTENSION);

		if( fileRemoveWithWildcard_Wide(preEncodedEncryptedFiles,   cast_handle->slout_hdl) != OKAY )
			return(NOT_OKAY);

		if( fileRemoveWithWildcard_Wide(preEncodedUnEncryptedFiles, cast_handle->slout_hdl) != OKAY )
			return(NOT_OKAY);
	}

	return(OKAY);
}
*/