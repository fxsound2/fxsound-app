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

#include "slout.h"
#include "mry.h"
#include "u_sndDevices.h"
#include "sndDevices.h"

/*
 * FUNCTION: sndDevices_UtilsGetIndexFromType()
 * DESCRIPTION:  Gets the internal index number for the passed in device type.
 */
int PT_DECLSPEC sndDevices_UtilsGetIndexFromType(PT_HANDLE *hp_sndDevices, int i_deviceType, int *ip_index)
{
	struct sndDevicesHdlType *cast_handle;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	*ip_index = SND_DEVICES_DEVICE_NOT_PRESENT;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if( cast_handle->totalNumDevices <= 0 )
		return(OKAY);

	*ip_index = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	switch(i_deviceType)
	{
	case SND_DEVICES_TARGETED_REAL_PLAYBACK :
		if( cast_handle->playbackDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			*ip_index = cast_handle->playbackDeviceNum;
		else
			*ip_index = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	case SND_DEVICES_CAPTURE :
		if( cast_handle->captureDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			*ip_index = cast_handle->captureDeviceNum;
		else
			*ip_index = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	case SND_DEVICES_VIRTUAL_PLAYBACK_DFX :
		if( cast_handle->dfxDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			*ip_index = cast_handle->dfxDeviceNum;
		else
			*ip_index = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	/*case SND_DEVICES_VIRTUAL_PLAYBACK_48 :
		if( cast_handle->dfx48DeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			*ip_index = cast_handle->dfx48DeviceNum;
		else
			*ip_index = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;*/

	case SND_DEVICES_DEFAULT :
		if( cast_handle->defaultDeviceNum != SND_DEVICES_DEVICE_NOT_PRESENT )
			*ip_index = cast_handle->defaultDeviceNum;
		else
			*ip_index = SND_DEVICES_DEVICE_NOT_PRESENT;
		break;

	default: // Its a programming error to ask for a device type that doesn't exist.
		*ip_index = SND_DEVICES_DEVICE_NOT_PRESENT;
		return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: sndDevices_UtilsGetIndexFromID()
 * DESCRIPTION:  Gets the internal index number for the passed in guid ID.
 */
int PT_DECLSPEC sndDevices_UtilsGetIndexFromID(PT_HANDLE *hp_sndDevices, wchar_t *wcp_ID, int *ip_index)
{
	struct sndDevicesHdlType *cast_handle;
	int i;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	*ip_index = SND_DEVICES_DEVICE_NOT_PRESENT;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (wcp_ID == NULL || wcslen(wcp_ID) == 0)
	{
		return(OKAY);
	}

	if( cast_handle->totalNumDevices <= 0 )
		return(OKAY);

	for(i=0; i<cast_handle->totalNumDevices; i++)
	{
		// Check ID strings to see if they match
		if( cast_handle->pwszID[i] != NULL && wcscmp(cast_handle->pwszID[i], wcp_ID) == 0 )
		{
			*ip_index = i;
			break;
		}
	}

	return(OKAY);
}