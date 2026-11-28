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

namespace
{
void recordDeviceChange(PT_HANDLE* opaque, fxsounddevices::DeviceChangeQueue::Kind kind,
                        LPCWSTR id, DWORD state = 0) noexcept
{
    auto* handle = reinterpret_cast<sndDevicesHdlType*>(opaque);
    if (handle == nullptr) return;
    // Sequentially consistent entry/close makes a late callback harmless even
    // when unregister races with a callback that has not entered its body yet.
    fxsounddevices::DeviceCallbackScope scope(handle->deviceCallbacksInFlight, handle->deviceNotificationsClosed);
    if (!scope) return;
    if (handle->ignoreDeviceCallbacks.load()) return;
    if (kind == fxsounddevices::DeviceChangeQueue::Kind::defaultChanged
        && handle->deviceChanges.isVirtualDevice(id)) return;

    if (kind == fxsounddevices::DeviceChangeQueue::Kind::stateChanged)
    {
        if (handle->deviceChanges.isRepeatedState(id, state)) return;
    }
    else handle->deviceChanges.invalidateStateCache();
    // Stop immediately on MMDevice's thread; dispatching the UI may be delayed.
    handle->stopAudioCaptureAndPlaybackLoop.store(1, std::memory_order_release);
    handle->deviceChanges.enqueue(kind, id, state);
    if (auto wake = handle->deviceChangeCallback.load(std::memory_order_acquire)) wake();
}
}

HRESULT STDMETHODCALLTYPE CsndDevicesMMNotificationClient::OnDefaultDeviceChanged(
    EDataFlow flow, ERole role, LPCWSTR id)
{
    if (flow == eRender && role == eMultimedia)
        recordDeviceChange(g_sndDevicesCallbacks_hdl, fxsounddevices::DeviceChangeQueue::Kind::defaultChanged, id);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CsndDevicesMMNotificationClient::OnDeviceAdded(LPCWSTR id)
{
    recordDeviceChange(g_sndDevicesCallbacks_hdl, fxsounddevices::DeviceChangeQueue::Kind::added, id);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CsndDevicesMMNotificationClient::OnDeviceRemoved(LPCWSTR id)
{
    recordDeviceChange(g_sndDevicesCallbacks_hdl, fxsounddevices::DeviceChangeQueue::Kind::removed, id);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CsndDevicesMMNotificationClient::OnDeviceStateChanged(LPCWSTR id, DWORD state)
{
    recordDeviceChange(g_sndDevicesCallbacks_hdl, fxsounddevices::DeviceChangeQueue::Kind::stateChanged, id, state);
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
