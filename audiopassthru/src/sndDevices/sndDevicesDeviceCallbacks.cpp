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
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <string>


#include "slout.h"
#include "mry.h"
#include "u_sndDevices.h"
#include "sndDevices.h"

// This is for registering the device callbacks. For some reason the callbacks would crash
// when this was an object in the handle, perhaps because it was allocated?
//CsndDevicesMMNotificationClient g_DeviceEvents;

CsndDevicesMMNotificationClient::CsndDevicesMMNotificationClient() :_cRef(1),_pEnumerator(NULL)
{
}

CsndDevicesMMNotificationClient::~CsndDevicesMMNotificationClient()
{
	// For some reason even though _pEnumerator comes in here non-null, this crashes when called in windevcon.
  //SAFE_RELEASE(_pEnumerator)
}

// IUnknown methods -- AddRef, Release, and QueryInterface

ULONG STDMETHODCALLTYPE CsndDevicesMMNotificationClient::AddRef()
{
  return InterlockedIncrement(&_cRef);
}

ULONG STDMETHODCALLTYPE CsndDevicesMMNotificationClient::Release()
{
  ULONG ulRef = InterlockedDecrement(&_cRef);
  if (0 == ulRef)
  {
      delete this;
  }
  return ulRef;
}

HRESULT STDMETHODCALLTYPE CsndDevicesMMNotificationClient::QueryInterface(
                          REFIID riid, VOID **ppvInterface)
{
  if (IID_IUnknown == riid)
  {
      AddRef();
      *ppvInterface = (IUnknown*)this;
  }
  else if (__uuidof(IMMNotificationClient) == riid)
  {
      AddRef();
      *ppvInterface = (IMMNotificationClient*)this;
  }
  else
  {
      *ppvInterface = NULL;
      return E_NOINTERFACE;
  }
  return S_OK;
}

// Callback methods for device-event notifications.

HRESULT STDMETHODCALLTYPE CsndDevicesMMNotificationClient::OnDefaultDeviceChanged(
                          EDataFlow flow, ERole role,
                          LPCWSTR pwstrDeviceId)
{
  char  *pszFlow = "?????";
  char  *pszRole = "?????";
  int i_resultFlag;
  
  struct sndDevicesHdlType *cast_handle;
    
  cast_handle = (struct sndDevicesHdlType *)g_sndDevicesCallbacks_hdl;

  if (cast_handle == NULL)
    return(S_OK);

  if (cast_handle->dfxDeviceNum == SND_DEVICES_DEVICE_NOT_PRESENT)
	return(S_OK);

  SLOUT_FIRST_LINE(L"CsndDevicesMMNotificationClient::OnDefaultDeviceChanged() enters");

  if( (flow == eRender) && (role == eMultimedia) && (cast_handle->ignoreDeviceCallbacks == FALSE) )
  {
	  // If one of the DFX capture devices was set as the default and the default device has been changed, force a re-initialization.
	  // Note- setting the flag only if a DFX device was the default is filtering out callbacks we want made.
	  //if( (cast_handle->dfxDeviceNum == cast_handle->defaultDeviceNum) || (cast_handle->dfx48DeviceNum == cast_handle->defaultDeviceNum) )
	  SLOUT_FIRST_LINE(L"CsndDevicesMMNotificationClient::OnDefaultDeviceChanged() setting processing thread kill flag");

	  // Don't think we need to kill process thread if DFX itself is selected as default from within our code
	  if ( cast_handle->pwszID[cast_handle->dfxDeviceNum] != NULL && wcscmp(pwstrDeviceId, cast_handle->pwszID[cast_handle->dfxDeviceNum]) == 0)
		  return(S_OK);

	  //std::wstring temp_string(pwstrDeviceId);
	  /* Set the newly targeted playback device as the default.  It will then automatically become the targeted device */
	  //if (sndDevicesSetDeviceType(g_sndDevicesCallbacks_hdl, SND_DEVICES_DEFAULT, &temp_string[0], &i_resultFlag) != OKAY)
		//  return(NOT_OKAY);
  	
  	
	  cast_handle->stopAudioCaptureAndPlaybackLoop = 1;
  }

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CsndDevicesMMNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
  struct sndDevicesHdlType *cast_handle;
    
  cast_handle = (struct sndDevicesHdlType *)g_sndDevicesCallbacks_hdl;

  if (cast_handle == NULL)
    return(S_OK);
	
  SLOUT_FIRST_LINE(L"CsndDevicesMMNotificationClient::OnDeviceAdded() enters");

  // These callbacks only appear to get hit when a device is added that didn't already have a driver installed.
  if( cast_handle->ignoreDeviceCallbacks == FALSE )
  {
	  SLOUT_FIRST_LINE(L"CsndDevicesMMNotificationClient::OnDeviceAdded() setting processing thread kill flag");
	  cast_handle->stopAudioCaptureAndPlaybackLoop = 1;
  }

  return S_OK;
};

HRESULT STDMETHODCALLTYPE CsndDevicesMMNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
  struct sndDevicesHdlType *cast_handle;
    
  cast_handle = (struct sndDevicesHdlType *)g_sndDevicesCallbacks_hdl;

  if (cast_handle == NULL)
    return(S_OK);

  SLOUT_FIRST_LINE(L"CsndDevicesMMNotificationClient::OnDeviceRemoved() enters");

  // These callbacks only appear to get hit when a device is added that didn't already have a driver installed.
  if( cast_handle->ignoreDeviceCallbacks == FALSE )
  {
	  SLOUT_FIRST_LINE(L"CsndDevicesMMNotificationClient::OnDeviceRemoved() setting processing thread kill flag");

	  cast_handle->stopAudioCaptureAndPlaybackLoop = 1;
  }

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CsndDevicesMMNotificationClient::OnDeviceStateChanged(
                          LPCWSTR pwstrDeviceId,
                          DWORD dwNewState)
{
  struct sndDevicesHdlType *cast_handle;
  int type;
    
  cast_handle = (struct sndDevicesHdlType *)g_sndDevicesCallbacks_hdl;

  if (cast_handle == NULL)
    return(S_OK);

  //SLOUT_FIRST_LINE(L"CsndDevicesMMNotificationClient::OnDeviceStateChanged() enters");

  switch (dwNewState)
  {
  case DEVICE_STATE_ACTIVE:
	   type = dwNewState;	// When connecting device, this gets hit around 10 times.
      break;
  case DEVICE_STATE_DISABLED:
	   type = dwNewState;
      break;
  case DEVICE_STATE_NOTPRESENT:
	   type = dwNewState;		// When disconnecting device, this gets hit around 12 times.
      break;
  case DEVICE_STATE_UNPLUGGED:
	   type = dwNewState;
      break;
  }

  // Ignore identical callbacks from the same guid. Since device must be disconnected after being connected,
  // we should never miss a connect or disconnect event.
  if( pwstrDeviceId != NULL )
		if( (wcscmp(pwstrDeviceId, cast_handle->lastDeviceAddCallbackGuid) == 0) && ( dwNewState == cast_handle->lastDeviceAddCallbackGuidtype ) )
		{
			SLOUT_FIRST_LINE(L"CsndDevicesMMNotificationClient::OnDeviceStateChanged() ignoring identical callback");
		   return(S_OK);
		}

  if( pwstrDeviceId == NULL )
		wcscpy(cast_handle->lastDeviceAddCallbackGuid, L"");
  else
	   wcscpy(cast_handle->lastDeviceAddCallbackGuid, pwstrDeviceId );

  cast_handle->lastDeviceAddCallbackGuidtype = dwNewState;

  if( cast_handle->ignoreDeviceCallbacks == FALSE )
  {
	  SLOUT_FIRST_LINE(L"CsndDevicesMMNotificationClient::OnDeviceStateChanged() setting processing thread kill flag");
	  cast_handle->stopAudioCaptureAndPlaybackLoop = 1;
  }

  return S_OK;
}

// NOTE - On Vista/Win7 this callback would only get hit when a legitmate "property" of a device changed,
// such as samp freq, num channels. On Win8 this callback is getting hit when the mute setting is changed.
// The thread stop set has been disabled to avoid Win8 mute problems as any change of these properties should
// cause the capture thread to exit with an error condition.
HRESULT STDMETHODCALLTYPE CsndDevicesMMNotificationClient::OnPropertyValueChanged(
                          LPCWSTR pwstrDeviceId,
                          const PROPERTYKEY key)
{
  struct sndDevicesHdlType *cast_handle;
    
  cast_handle = (struct sndDevicesHdlType *)g_sndDevicesCallbacks_hdl;

  if (cast_handle == NULL)
    return(S_OK);

	/*
	struct sndDevicesHdlType *cast_handle;
	wchar_t guidID[PT_MAX_GENERIC_STRLEN];
	int deviceNum;
	 
	cast_handle = (struct sndDevicesHdlType *)g_sndDevicesCallbacks_hdl;

	if (cast_handle == NULL)
	 return(S_OK);

	// Only force a reset if a property value changed on the playback or capture devices.
	if( cast_handle->ignoreDeviceCallbacks == FALSE )
	{
	  if( pwstrDeviceId != NULL )	// Sometimes this comes in with a null value.
	  {
		  wcscpy(guidID, pwstrDeviceId);

		  if( sndDevices_UtilsGetIndexFromID(g_sndDevicesCallbacks_hdl, guidID, &deviceNum) == OKAY )
			  if( (deviceNum == cast_handle->captureDeviceNum) || (deviceNum == cast_handle->playbackDeviceNum) )
				 cast_handle->stopAudioCaptureAndPlaybackLoop = 1;
	  }
	}
	*/

	//SLOUT_FIRST_LINE(L"CsndDevicesMMNotificationClient::OnPropertyValueChanged() enters");

	return S_OK;
}

// Function to be called from sndDevicesInit to pass in allocated handle for by methods in this class.
void CsndDevicesMMNotificationClient::SetPtHandle(PT_HANDLE *sndDevices_hdl)
{
	g_sndDevicesCallbacks_hdl = sndDevices_hdl;
}
