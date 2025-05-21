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

#include "slout.h"
#include "reg.h"
#include "mry.h"
#include "u_sndDevices.h"
#include "sndDevices.h"
#include "ptime.h"

/*
 * FUNCTION: sndDevicesImplementDeviceRules()
 */
int PT_DECLSPEC sndDevicesImplementDeviceRules(PT_HANDLE *hp_sndDevices, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;

	// An array of multiple w_char strings.
	wchar_t **wcpp_RealDeviceIDs;
	wchar_t mostRecentDefaultID[PT_MAX_GENERIC_STRLEN];
	wchar_t priorDefaultID[PT_MAX_GENERIC_STRLEN];
	wchar_t originalDefaultID[PT_MAX_GENERIC_STRLEN];
	wchar_t mostRecentPlaybackID[PT_MAX_GENERIC_STRLEN];
	wchar_t userSelectedPlaybackID[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_trace_msg[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_formatted_time[PT_MAX_GENERIC_STRLEN];

	WAVEFORMATEX wfx;
	int numRealDevices;
	int playbackSamplingFrequency;
	int captureSamplingFrequency;
	int numCaptureChannels;
	int deviceIndex;
	int resultFlag;
	int i, j;
	int NoMatchFlag;
	int i_numChannels;
	int i_resultFlag;
	int iNumMonoDevices;
	int iNumNonMonoDevices;

	int WritePreviousDefault;

	int current_defaultNum = SND_DEVICES_DEVICE_NOT_PRESENT;
	int dfxNum = SND_DEVICES_DEVICE_NOT_PRESENT;
	int userSelectedPlaybackNum = SND_DEVICES_DEVICE_NOT_PRESENT;	// Currently never set with current implemenation, user selection is done by forcing that device to be the default.
	int mostRecentDefaultNum = SND_DEVICES_DEVICE_NOT_PRESENT;	// Prior to coming into this function, the device that had most recently been selected as default device.
	int priorDefaultNum = SND_DEVICES_DEVICE_NOT_PRESENT;			// Prior to coming into this function, the device that was default before the mostRecentDefaultNum one.
	int originalDefaultNum = SND_DEVICES_DEVICE_NOT_PRESENT;		// The device that was default on the first startup of DFX after installation.
	int mostRecentPlaybackNum = SND_DEVICES_DEVICE_NOT_PRESENT;	// The device that was used for playback prior do coming into this function.
	int defaultSelectionAutoMode = SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_ON; // In current implementation this is always on.
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	SLOUT_FIRST_LINE(L"sndDevicesImplementDeviceRules() enters");

	*ip_resultFlag = SND_DEVICES_RULES_NOT_POSSIBLE;
	WritePreviousDefault = 0;

	//  Get the ID's for all the real playback devices present prior to entering this function and finds DFX device and current default device.
	if( sndDevicesGetAllRealDeviceIDs(hp_sndDevices, &(wcpp_RealDeviceIDs), &numRealDevices) != OKAY )
		return(NOT_OKAY);

	if( numRealDevices <= 0 )
	{
		SLOUT_FIRST_LINE(L"sndDevicesImplementDeviceRules() :: no real devices, returning OKAY");
		return(OKAY);
	}

	current_defaultNum = cast_handle->defaultDeviceNum; // The device the system is currently using as the Default device.
	dfxNum = cast_handle->dfxDeviceNum;

	// Get the default device selection mode (in currently implementation always on).
	if( sndDevicesGetDefaultDeviceSelectionMode(hp_sndDevices, &defaultSelectionAutoMode) != OKAY )
		return(NOT_OKAY);

	//Check to see if the user has selected the playback device (in currently implementation never set).
	if( sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_USER_SELECTED_PLAYBACK_WIDE, userSelectedPlaybackID) != OKAY )
		return(NOT_OKAY);
	// Get this devices internal index if the device is active.
	if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, userSelectedPlaybackID, &userSelectedPlaybackNum) != OKAY )
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

	//Get the ID for the most recent playback device.
	if( sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_MOST_RECENT_PLAYBACK_WIDE, mostRecentPlaybackID) != OKAY )
		return(NOT_OKAY);
	// Get this devices internal index if the device is active.
	if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, mostRecentPlaybackID, &mostRecentPlaybackNum) != OKAY )
		return(NOT_OKAY);

	// Test for first run since installation by checking for non-null most recent default device ID value.
	if( wcscmp(mostRecentDefaultID, L"") == 0 )
	{
		// NOTE - ELIMINATE THE REG WRITE BELOW WHEN THE INSTALLER IS PROPERLY IMPLEMENTED, IT WILL DO THIS WRITE.
		// Store the DFX device guids so we can easily change their enabled state.
		//if( sndDevicesWriteToRegistry(hp_sndDevices, REG_LOCAL_MACHINE, SND_DEVICES_REGISTRY_DFX_GUID, cast_handle->pwszID[dfxNum]) != OKAY)
		//	return(NOT_OKAY);

		// This is the first run after installation, store the current default device as both the original default and the most recent default.
		if( current_defaultNum != dfxNum )
		{
			if( sndDevicesWriteToRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_ORIGINAL_DEFAULT_WIDE, cast_handle->pwszID[current_defaultNum]) != OKAY)
				return(NOT_OKAY);
			if( sndDevicesWriteToRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_MOST_RECENT_DEFAULT_WIDE, cast_handle->pwszID[current_defaultNum]) != OKAY)
				return(NOT_OKAY);

			// Now set the playback device to be the original default device.
			cast_handle->playbackDeviceNum = current_defaultNum;

			goto PlaybackDeviceIsSelected;
		}
		// If we fall through here, its a strange case of DFX drivers being installed with one selected as default
		// but the original default device never being written. Continuing should repair the case.
	}

	// This is not the first run after installation.
	// If there is only one real playback device, it must be assigned as the selected one for playback.
	if( numRealDevices == 1 )
	{
		// Get this playback devices internal index, wcpp_RealDeviceIDs[0] references the first device. 
		if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, wcpp_RealDeviceIDs[0], &deviceIndex) != OKAY )
			return(NOT_OKAY);

		// Use the single real playback device.
		cast_handle->playbackDeviceNum = deviceIndex;
		// NOTE: Do not skip to PlaybackDeviceIsSelected anymore so it can go through the check
		// below to make sure the selected playback device is not a mono device.
		//goto PlaybackDeviceIsSelected;
	}

	// We have more than 1 real playback device.
	// Check to see if the user has manually selected the playback device and also if it is active.
	// Note, in current implementation when user selects a playback device in DFX we force it to be the default device, userSelectedPlaybackNum is not set.
	if( userSelectedPlaybackNum != SND_DEVICES_DEVICE_NOT_PRESENT)
	{
		// The user selected a playback device and its active so we'll use it.
		cast_handle->playbackDeviceNum = userSelectedPlaybackNum;
		goto PlaybackDeviceIsSelected;
	}

	// Check to see if a device has just been added, if so, find it and set it as the playback device.
	// Note that if more than one device was added this will set playback to the first device found.
	if( (cast_handle->numRealDevices > cast_handle->numPreviousRealDevices) && (cast_handle->numPreviousRealDevices > 0) )
	{
		// Now we need to find the new device GUID that was not in the previous list.
		for(i=0; i<cast_handle->numRealDevices; i++)
		{
			if (SND_DEVICES_MONO_BUG_SKIP_MONO_DEVICES)
			{
				if (sndDevicesGetNumberOfChannelsFromID(hp_sndDevices, cast_handle->pwszIDRealDevices[i], &i_numChannels, &i_resultFlag) != OKAY)
					return(NOT_OKAY);

				if (i_numChannels == 1)
				{
					continue;
				}
			}
			
			NoMatchFlag = 1;
			for(j=0; j<cast_handle->numPreviousRealDevices; j++)
			{
				if( wcscmp(cast_handle->pwszIDRealDevices[i], cast_handle->pwszIDPreviousRealDevices[j]) == 0 )
					NoMatchFlag = 0;
			}
			if(NoMatchFlag)
				break; // This new device didn't match any of the previous devices.
		}
		// Get this devices internal index.
		if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, cast_handle->pwszIDRealDevices[i], &deviceIndex) != OKAY )
			return(NOT_OKAY);

		if (deviceIndex != SND_DEVICES_DEVICE_NOT_PRESENT)
		{
			cast_handle->playbackDeviceNum = deviceIndex;
			WritePreviousDefault = 1;
			goto PlaybackDeviceIsSelected;
		}
	}

	// If we got here we have more than one real playback device and either the user hasn't manually selected one or
	// the selected device is not present. Use rules to select the playback device based on the default device setting and history.
	// First check to see if the current default device is not one of the DFX devices.
	if( current_defaultNum != dfxNum )
	{
		// A non-DFX device is currently the default device.
		// Check to see if the user has manually turned off automatice default device selection mode.
		//if( defaultSelectionAutoMode == SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_OFF )
		// For now, in both auto and non-auto default selection mode, for both these cases we will use the current default device for playback.
		{
			// Neither DFX device is the default and we are in automatic default device selection mode.
			// This is likely a case where a new device has been plugged in or installed so we will use it as the playback device.

			cast_handle->playbackDeviceNum = current_defaultNum;
			WritePreviousDefault = 1;
		}
	}
	else
	{
		// If we got here then there are multiple playback devices, one of the DFX devices is already selected as the default
		// device and the user hasn't manually selected the real playback device.
		// In this case we will use the first active device from the following list: 
		//		mostRecentPlayback, mostRecentDefault, priorDefault, originalDefault.
		if( mostRecentPlaybackNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			cast_handle->playbackDeviceNum = mostRecentPlaybackNum;

		else if( mostRecentDefaultNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			cast_handle->playbackDeviceNum = mostRecentDefaultNum;

		else if( priorDefaultNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			cast_handle->playbackDeviceNum = priorDefaultNum;

		else if( originalDefaultNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			cast_handle->playbackDeviceNum = originalDefaultNum;

		else
		{
			// If we got here, there are multiple playback devices but none of our prior used and saved device ID's are currently active.
			// For now we will set the playback device to the first active real device.
			if( numRealDevices >= 1 )
			{
				// Get this playback devices internal index, wcpp_RealDeviceIDs[0] references the first device. 
				if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, wcpp_RealDeviceIDs[0], &deviceIndex) != OKAY )
					return(NOT_OKAY);

				// Use the single real playback device.
				cast_handle->playbackDeviceNum = deviceIndex;
				// NOTE: Do not skip to PlaybackDeviceIsSelected anymore so it can go through the check
				// below to make sure the selected playback device is not a mono device.
				//goto PlaybackDeviceIsSelected;
			}
			else
				cast_handle->playbackDeviceNum = SND_DEVICES_DEVICE_NOT_PRESENT;
		}
	}

PlaybackDeviceIsSelected:  // Label to jump to when the playback device num has been determined.
	if (SND_DEVICES_MONO_BUG_SKIP_MONO_DEVICES)
	{
		// Check if selected playback device is a mono device. If so, try to revert back to the most recent
		// playback device but also check to make sure it is not a mono device. If it is still a mono device,
		// we pass either SND_DEVICES_NO_VALID_PLAYBACK_DEVICE or SND_DEVICES_ASK_USER_SELECT_PLAYBACK_DEVICE
		// back out to the DFXG level to pop up a messagebox telling users what to do.
		if (sndDevicesGetNumberOfChannelsFromID(hp_sndDevices, cast_handle->pwszID[cast_handle->playbackDeviceNum], &i_numChannels, &i_resultFlag) != OKAY)
			return(NOT_OKAY);

		if (i_numChannels == 1) {
			// Try reverting bak to the most recent playback device
			if (sndDevicesGetNumberOfChannelsFromID(hp_sndDevices, mostRecentPlaybackID, &i_numChannels, &i_resultFlag) != OKAY)
				return(NOT_OKAY);

			if (i_numChannels == 1) {
				// Find out how many non-mono playback devices there are. If there is more than one, we 
				// ask the user to manually select one from the menyu. If there is none, we tell the user
				// that DFX is incompatible.
				if (sndDevicesGetNumMonoDevices(hp_sndDevices, &iNumMonoDevices) != OKAY)
					return(NOT_OKAY);

				iNumNonMonoDevices = numRealDevices - iNumMonoDevices;

				if (iNumNonMonoDevices >= 1)
				{
					// There are more than one playback devices. Ask the users to manually select one from the DFX menu
					// (which only lists non-mono playback devices).
					*ip_resultFlag = SND_DEVICES_ASK_USER_SELECT_PLAYBACK_DEVICE;
				}
				else
				{
					// If there is only one playback device present on the system and it is mono, that means the user
					// must exit DFX in order to get sounds.
					*ip_resultFlag = SND_DEVICES_NO_VALID_PLAYBACK_DEVICE;
				}

				return(OKAY);
			}
			else {
				// Get this playback devices internal index, wcpp_RealDeviceIDs[0] references the first device.
				// We will use the most recent playback device recorded in the registry.
				if (sndDevices_UtilsGetIndexFromID(hp_sndDevices, mostRecentPlaybackID, &deviceIndex) != OKAY)
					return(NOT_OKAY);
				cast_handle->playbackDeviceNum = deviceIndex;
			}
		}
	}

	// If we were unable to assign a playback device, return SND_DEVICES_DEVICE_NOT_PRESENT
	if( cast_handle->playbackDeviceNum == SND_DEVICES_DEVICE_NOT_PRESENT )
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_NOT_PRESENT);
	}

	cast_handle->pPlaybackDevice = cast_handle->pAllDevices[cast_handle->playbackDeviceNum];

	if( cast_handle->pPlaybackDevice == NULL )
		return(NOT_OKAY);

	if ((cast_handle->i_trace_on) && (cast_handle->slout_hdl))
	{
		if (ptimeGetDateAndTimeStr(wcp_formatted_time) != OKAY)
			return(NOT_OKAY);

		cast_handle->slout_hdl->Message_Wide(FIRST_LINE, L"sndDevicesImplementDeviceRules(): Writing Playback device to registry");
		swprintf(wcp_trace_msg, L"   Device: %s", cast_handle->pwszID[cast_handle->playbackDeviceNum]);
		cast_handle->slout_hdl->Message_Wide(FIRST_LINE, wcp_trace_msg);
		swprintf(wcp_trace_msg, L"   Timestamp: %s", wcp_formatted_time);
		cast_handle->slout_hdl->Message_Wide(FIRST_LINE, wcp_trace_msg);
	}

	// Write the selected playback device ID to the registry.
	if( sndDevicesWriteToRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_MOST_RECENT_PLAYBACK_WIDE, cast_handle->pwszID[cast_handle->playbackDeviceNum]) != OKAY)
		return(NOT_OKAY);

	if( WritePreviousDefault )
	{

		// Update the most recently used default ID with the current playback ID.
		if( sndDevicesWriteToRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_MOST_RECENT_DEFAULT_WIDE, cast_handle->pwszID[cast_handle->playbackDeviceNum]) != OKAY )
			return(NOT_OKAY);

		// Update the prior default device ID with the most recently used default ID.
		if( sndDevicesWriteToRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_PRIOR_DEFAULT_WIDE, mostRecentPlaybackID) != OKAY )
			return(NOT_OKAY);

		// Get the prior default ID number if its active
		if( sndDevices_UtilsGetIndexFromID(hp_sndDevices, mostRecentPlaybackID, &priorDefaultNum) != OKAY )
				return(NOT_OKAY);
	}

	// Get the playback devices format and sampling frequency so we can select the correct DFX device for capture.
	if( sndDevicesGetFormatFromID(hp_sndDevices, cast_handle->pwszID[cast_handle->playbackDeviceNum], &wfx, &resultFlag) != OKAY )
		return(NOT_OKAY);

	cast_handle->wfxDfxProcessing = wfx;	// Initialize DFX processing format to match playback device format, sampling frequency will be corrected below.

	playbackSamplingFrequency = wfx.nSamplesPerSec;

	// Make sure the DFX device is valid.
	if( cast_handle->dfxDeviceNum == SND_DEVICES_DEVICE_NOT_PRESENT )
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_DEVICE_NOT_PRESENT);
	}

	cast_handle->captureDeviceNum = cast_handle->dfxDeviceNum;

	numCaptureChannels = wfx.nChannels;

	// Restrict numCaptureChannels to supported values
	if( numCaptureChannels < SND_DEVICES_MIN_NUM_CHANS )
		numCaptureChannels = SND_DEVICES_MIN_NUM_CHANS;

	// In quad output case we will capture stereo and fill back channels with fronts
	if( numCaptureChannels == 4 )
		numCaptureChannels = 6;

	if( numCaptureChannels > SND_DEVICES_MAX_NUM_CHANS )
		numCaptureChannels = SND_DEVICES_MAX_NUM_CHANS;

	if( (playbackSamplingFrequency % 48000) == 0 )
	{
		captureSamplingFrequency = 48000;
		if( sndDevicesSetDfxDeviceSampleRateAndChannels(hp_sndDevices, SND_DEVICES_DFX_SAMP_FREQ_48, numCaptureChannels, &resultFlag) != OKAY )
			return(NOT_OKAY);
	}
	else
	{
		captureSamplingFrequency = 44100;
		if( sndDevicesSetDfxDeviceSampleRateAndChannels(hp_sndDevices, SND_DEVICES_DFX_SAMP_FREQ_44_1, numCaptureChannels, &resultFlag) != OKAY )
			return(NOT_OKAY);
	}

	if( resultFlag != SND_DEVICES_DEVICE_OPERATION_COMPLETED )
	{
		*ip_resultFlag = SND_DEVICES_PROPERTY_SET_FAILED;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_PROPERTY_SET_FAILED);
	}

	cast_handle->wfxDfxProcessing.nSamplesPerSec = captureSamplingFrequency; // Correct DFX processing sampling freq.

	// Unless the user has turned off automatic default device selection, set the Windows default device to the current DFX capture device.
	if( defaultSelectionAutoMode == SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_ON )
	{
		//cast_handle->ignoreDeviceCallbacks = TRUE;

		if( sndDevicesSetDeviceType(hp_sndDevices, SND_DEVICES_DEFAULT, cast_handle->pwszID[cast_handle->captureDeviceNum], &resultFlag) != OKAY)
			return(NOT_OKAY);

		// Sleep for 1 second so the device callbacks caused by the change above don't force us to do addition undesired re-inits.
		//Sleep(SND_DEVICES_DEFAULT_CHANGE_WAIT_TIME);

		//cast_handle->ignoreDeviceCallbacks = FALSE;
	}

	// Setup selected capture device.
	cast_handle->pCaptureDevice = cast_handle->pAllDevices[cast_handle->captureDeviceNum];

	if( cast_handle->pCaptureDevice == NULL )
	{
		*ip_resultFlag = SND_DEVICES_DEVICE_NOT_PRESENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_CAPTURE_DEVICE_IS_NULL);
	}

	if( (captureSamplingFrequency == 0) || (playbackSamplingFrequency == 0) )
	{
		cast_handle->upsampleRatio = 1;
	}
	else
		cast_handle->upsampleRatio = playbackSamplingFrequency/captureSamplingFrequency;

	*ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	SLOUT_FIRST_LINE(L"sndDevicesImplementDeviceRules() completes and returns OKAY");

	return(OKAY);
}