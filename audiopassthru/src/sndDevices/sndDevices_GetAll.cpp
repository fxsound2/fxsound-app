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
#include "pstr.h"

//extern CsndDevicesMMNotificationClient g_DeviceEvents;	// This is for registering the device callbacks.

// Gets an array of objects for all audio devices and stores indexes to the default device and DFX device if installed.
int PT_DECLSPEC sndDevices_GetAll( PT_HANDLE *hp_sndDevices, int *ip_num_devices )
{
	HRESULT hr;
	IMMDevicePtr pDefaultDevice = NULL;
	IMMDeviceEnumeratorPtr pEnumerator = NULL;
	IMMDeviceCollectionPtr pCollectionAllDevices;
	UINT  deviceCount;
	IPropertyStorePtr pProps = NULL;
	LPWSTR pwszIDdefault = NULL;
	PROPVARIANT FriendlyName;
	PROPVARIANT DeviceDesc;
	ULONG i;
	WAVEFORMATEX wfx;
	int k;
	int i_found_start_location;
	int i_found_dfx_string;
	int resultFlag;

	EDataFlow flow = eRender;
	ERole role = eMultimedia;

	struct sndDevicesHdlType *cast_handle;
    
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	// Errors should be cleared at higher level.
	//cast_handle->function_status = SND_DEVICES_DEVICE_OPERATION_COMPLETED;

	// Copy current devices to previous devices to allow new device detection
	if( (cast_handle->numRealDevices > 0) && (cast_handle->numRealDevices != cast_handle->numPreviousRealDevices) )
	{
		for(k=0; k< cast_handle->numRealDevices; k++)
			wcscpy(cast_handle->pwszIDPreviousRealDevices[k], cast_handle->pwszIDRealDevices[k]);

		cast_handle->numPreviousRealDevices = cast_handle->numRealDevices;
	}
	cast_handle->numRealDevices = 0;

	hr = CoCreateInstance(cast_handle->CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, cast_handle->IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_INSTANCE_CREATE_FAILED)

	// Enumerate all playback audio devices, "eRender" means look only for playback devices.
	hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollectionAllDevices);   
	if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_ENUMERATE_FAILED)

	// Now get total number of playback devices
	hr = pCollectionAllDevices->GetCount(&deviceCount);
	if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_GETCOUNT_FAILED)

	cast_handle->totalNumDevices = deviceCount;
	*ip_num_devices = deviceCount;

	// There are no active sound devices and thus also no default device.
	if( cast_handle->totalNumDevices <= 0 )
	{
		cast_handle->numRealDevices = 0;
		cast_handle->dfxDeviceNum = SND_DEVICES_DEVICE_NOT_PRESENT;
		cast_handle->defaultDeviceNum = SND_DEVICES_DEVICE_NOT_PRESENT;
		CoTaskMemFree(pwszIDdefault);
		PropVariantClear(&FriendlyName);
		PropVariantClear(&DeviceDesc);

		return(OKAY);
	}

	// First get the ID string for the current default playback device.
	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDefaultDevice);
	if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_GET_AUDIOENDPOINT_FAILED);

	if(pDefaultDevice != NULL)
	{
		// Get the default device ID strincast_handle->
		pDefaultDevice->GetId(&pwszIDdefault);
		if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_GETID_FAILED);
	}

	// Loop to set up objects for each device and to find and store DFX and default device nums.
	for (i = 0; i < deviceCount; i++)
	{
		// Get indexed device.
		hr = pCollectionAllDevices->Item(i, &(cast_handle->pAllDevices[i]));
		if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_GET_INDEXED_DEVICE_FAILED);

		// Get and store the endpoint ID string for each device (this is the GUID ID for the device).
		hr = cast_handle->pAllDevices[i]->GetId( &(cast_handle->pwszID[i]) );
		if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_GETID_FAILED);

		// Access the registry properties for this device.
		hr = cast_handle->pAllDevices[i]->OpenPropertyStore(STGM_READ, &pProps);
		if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_INSTANCE_CREATE_FAILED);

		// Initialize containers for property values.
		PropVariantInit(&FriendlyName);
		PropVariantInit(&DeviceDesc);

		// Get the devices's friendly-name property. This key doesn't include the "Speakers" prefix.
		hr = pProps->GetValue(PKEY_Device_FriendlyName, &FriendlyName);
		if (FAILED(hr)) continue;

		// Get the devices's descriptive name property, ie "Speakers".
		hr = pProps->GetValue(PKEY_Device_DeviceDesc, &DeviceDesc);
		if (FAILED(hr)) continue;

		// Copy friendly name and description
        if (FriendlyName.pwszVal != NULL && DeviceDesc.pwszVal != NULL) 
        {
            wcscpy(cast_handle->deviceFriendlyName[i], FriendlyName.pwszVal);
            wcscpy(cast_handle->deviceDescription[i], DeviceDesc.pwszVal);

            // Get device number of channels
            if (sndDevicesGetFormatFromID(hp_sndDevices, cast_handle->pwszID[i], &wfx, &resultFlag) != OKAY)
                return(NOT_OKAY);
            cast_handle->deviceNumChannel[i] = wfx.nChannels;

            // Check ID strings to see if this is the default device
            if (cast_handle->pwszID[i] != NULL && pwszIDdefault != NULL && wcscmp(cast_handle->pwszID[i], pwszIDdefault) == 0)
                cast_handle->defaultDeviceNum = i;

            // Check friendly name to see if this is one of the DFX devices.
            // NOTE: We need to check if the DFX name is part of the device name. Normally it will be an exact
            //			match, but for some reason, sometimes windows prepends the device name with "2-" which
            //			causes the device name to appear as ""2- DFX Audio Enhancer 11.1"
            //
            //			The old way we did the exact match check is as follows:
            //			if( wcscmp(cast_handle->deviceFriendlyName[i], SND_DEVICES_DFX_DEVICE_STRING) == 0 )

            if (pstrCalcLocationOfStrInStr_Wide(cast_handle->deviceFriendlyName[i], SND_DEVICES_DFX_DEVICE_STRING,
                0, &i_found_start_location, &i_found_dfx_string) != OKAY)
            {
                hr = S_FALSE;
                if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_INSTANCE_CREATE_FAILED);
            }
            if (i_found_dfx_string)
                cast_handle->dfxDeviceNum = i;

            // If its not one of the DFX devices, store it as a real playback device
            if ((cast_handle->dfxDeviceNum != i))
            {
                cast_handle->pwszIDRealDevices[cast_handle->numRealDevices] = cast_handle->pwszID[i];
                cast_handle->deviceFriendlyNameRealDevices[cast_handle->numRealDevices] = cast_handle->deviceFriendlyName[i];
                cast_handle->deviceDescriptionRealDevices[cast_handle->numRealDevices] = cast_handle->deviceDescription[i];
                cast_handle->numRealDevices += 1; // Update total number of real playback devices.
            }
        }
        else
        {
            wcscpy(cast_handle->deviceFriendlyName[i], L"Unknown");
            wcscpy(cast_handle->deviceDescription[i], L"Unknown");

            cast_handle->deviceNumChannel[i] = 1;
        }
	}

	return(OKAY);
}
