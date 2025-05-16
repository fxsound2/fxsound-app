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
#ifndef _SND_DEVICES_H_
#define _SND_DEVICES_H_

#include <Winbase.h>
#include <comdef.h>
#include <mmreg.h>
#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <endpointvolume.h>
#include "slout.h"

#define PT_MAX_GENERIC_STRLEN          512
/*
 * THERE IS CURRENTLY A PROBLEM WITH MONO SOUND DEVICES.
 * We can temporarily turn off mono soundcard functionality
 * with these defines.
 */

#define SND_DEVICES_MONO_BUG_DO_NOT_PROCESS		IS_TRUE
#define SND_DEVICES_MONO_BUG_FORCE_SILENCE		IS_TRUE
#define SND_DEVICES_MONO_BUG_SKIP_MONO_DEVICES	IS_TRUE

/*
#define SND_DEVICES_MONO_BUG_DO_NOT_PROCESS		IS_TRUE
#define SND_DEVICES_MONO_BUG_FORCE_SILENCE		IS_FALSE
#define SND_DEVICES_MONO_BUG_SKIP_MONO_DEVICES	IS_FALSE
*/

/* Limit settings */
#define SND_DEVICES_MAX_NUM_DEVICES 16

/* Device "friendly name" string, used to identify DFX device. */
#define SND_DEVICES_DFX_DEVICE_STRING L"FxSound Audio Enhancer"
#define SND_DEVICES_DFX_PREVIOUS_DEVICE_STRING L"DFX Audio Enhancer"

/* Device "description name" (default is "Speakers") */
#define SND_DEVICES_DFX_DEVICE_DESCRIPTION_STRING L"FxSound Speakers"


/* Initialization and control type specifiers */
#define SND_DEVICES_INIT_FOR_PROCESSING	1	/* Use when audio processing will be performed */
#define SND_DEVICES_INIT_NO_PROCESSING		2	/* Use when only device operations will be performed but no audio processing*/

#define SND_DEVICES_START_CAPTURE			6
#define SND_DEVICES_STOP_CAPTURE				7

/* Mode Flags */
#define SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_OFF	0
#define SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_ON	1

/* DFX processed buffer playback Status */
#define SND_DEVICES_PLAYBACK_IS_STOPPED 0
#define SND_DEVICES_PLAYBACK_IS_ACTIVE	1

/* Get and Set result flags */
#define SND_DEVICES_DEVICE_OPERATION_COMPLETED	 0
#define SND_DEVICES_GENERIC_FAILURE					-1
#define SND_DEVICES_DEVICE_NOT_PRESENT				-2
#define SND_DEVICES_PROPERTY_SET_FAILED			-3
#define SND_DEVICES_RULES_NOT_POSSIBLE				-4
#define SND_DEVICES_DLL_LOAD_ERROR					-13
#define SND_DEVICES_THREAD_MADE_REQUESTED_EXIT	-14
#define SND_DEVICES_THREAD_TERMINATE_FAILED		-15
#define SND_DEVICES_INSTANCE_CREATE_FAILED		-16
#define SND_DEVICES_ENUMERATE_FAILED				-17
#define SND_DEVICES_REGISTER_FAILED					-18
#define SND_DEVICES_GETCOUNT_FAILED					-19
#define SND_DEVICES_GETID_FAILED						-20
#define SND_DEVICES_GET_DEVICES_FAILED				-21
#define SND_DEVICES_GET_AUDIOENDPOINT_FAILED		-22
#define SND_DEVICES_GET_INDEXED_DEVICE_FAILED	-23
#define SND_DEVICES_GET_PADDING_FAILED				-24
#define SND_DEVICES_GET_REG_PROPS_FAILED			-25
#define SND_DEVICES_GET_BUFFER_FAILED				-26
#define SND_DEVICES_RELEASE_BUFFER_FAILED			-27
#define SND_DEVICES_NULL_CAPTURE_CLIENT			-28
#define SND_DEVICES_NULL_PLAYBACK_CLIENT			-29
#define SND_DEVICES_NULL_LOOPBACK_CLIENT			-30
#define SND_DEVICES_WAIT_FOR_MUTEX_FAILED			-31
#define SND_DEVICES_PLAYBACK_RENDER_FAILED		-32
#define SND_DEVICES_THREAD_PLAYBACK_FAILED		-33
#define SND_DEVICES_DEVICE_ACTIVATION_FAILED		-34
#define SND_DEVICES_DEVICE_GET_FORMAT_FAILED		-35
#define SND_DEVICES_DEVICE_SET_FORMAT_FAILED		-36
#define SND_DEVICES_DEVICE_OPEN_PROPS_FAILED		-37
#define SND_DEVICES_DEVICE_ICON_PROP_GET_FAILED -38
#define SND_DEVICES_DEVICE_INIT_PROP_FAILED		-39
#define SND_DEVICES_CAPTURE_DEVICE_IS_NULL		-40
#define SND_DEVICES_SAMP_FREQ_NOT_VALID			-41
#define SND_DEVICES_NUM_CHANS_NOT_VALID			-42
#define SND_DEVICES_MP3_DLL_LOAD_FAILED			-43
#define SND_DEVICES_SET_MASTER_VOLUME_FAILED		-44
#define SND_DEVICES_GET_MASTER_VOLUME_FAILED		-45
#define SND_DEVICES_NULL_VOLUME_ENDPOINT			-46
#define SND_DEVICES_SET_MUTE_FAILED					-47
#define SND_DEVICES_NULL_CAPTURE_DEVICE			-48
#define SND_DEVICES_NULL_PLAYBACK_DEVICE			-49
#define SND_DEVICES_GET_SERVICE_FAILED				-50
#define SND_DEVICES_AUDIO_CLIENT_INIT_FAILED		-54
#define SND_DEVICES_NO_VALID_PLAYBACK_DEVICE 	-57
#define SND_DEVICES_ASK_USER_SELECT_PLAYBACK_DEVICE	-58

/* Device Status Change Events */
#define SND_DEVICES_NO_CHANGE						0
#define SND_DEVICES_DEFAULT_DEVICE_CHANGED	1
#define SND_DEVICES_DEVICE_ADDED					2
#define SND_DEVICES_DEVICE_REMOVED				3
#define SND_DEVICES_DEVICE_ACTIVE				4
#define SND_DEVICES_DEVICE_DISABLED				5
#define SND_DEVICES_DEVICE_NOTPRESENT			6
#define SND_DEVICES_DEVICE_UNPLUGGED			7
#define SND_DEVICES_DEVICE_PROPERTY_CHANGE	8

/* Capture and playback return flags */
#define SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS		0
#define SND_DEVICES_FILE_PLAYBACK_SUCCESS			0
#define SND_DEVICES_CAPTURE_ERROR					201
#define SND_DEVICES_CAPTURE_FILE_WRITE_ERROR		202
#define SND_DEVICES_PLAYBACK_ERROR					203
#define SND_DEVICES_CAPTURE_FORCED_EXIT			204
#define SND_DEVICES_FILE_PLAYBACK_FORCED_EXIT	205
#define SND_DEVICES_FILE_PLAYBACK_REACHED_EOF	206
#define SND_DEVICES_CAPTURE_NOT_POSSIBLE			207
#define SND_DEVICES_PLAYBACK_NOT_POSSIBLE			208
#define SND_DEVICES_NO_REAL_DEVICES_FOUND			209

/* Device specifiers and related storage locations */
#define SND_DEVICES_TARGETED_REAL_PLAYBACK			100
#define SND_DEVICES_VIRTUAL_PLAYBACK_DFX				101
#define SND_DEVICES_USER_SELECTED_PLAYBACK_DEVICE	103
#define SND_DEVICES_CAPTURE								104
#define SND_DEVICES_DEFAULT								105
#define SND_DEVICES_PRIOR_DEFAULT						106
#define SND_DEVICES_PRIOR_PLAYBACK						107

/* General control defines */
#define SND_DEVICES_USER_SETTING				0
#define SND_DEVICES_DEFAULT_SETTING			1

/* Registry related defines, current user */
#define SND_DEVICES_REGISTRY_DEVICES_WIDE							L"devices"
#define SND_DEVICES_REGISTRY_ORIGINAL_DEFAULT_WIDE				L"original_default"
#define SND_DEVICES_REGISTRY_MOST_RECENT_DEFAULT_WIDE			L"most_recent_default"
#define SND_DEVICES_REGISTRY_PRIOR_DEFAULT_WIDE					L"prior_default"
#define SND_DEVICES_REGISTRY_MOST_RECENT_PLAYBACK_WIDE		L"most_recent_playback"
#define SND_DEVICES_REGISTRY_USER_SELECTED_PLAYBACK_WIDE		L"user_selected_playback"
#define SND_DEVICES_REGISTRY_DEFAULT_DEVICE_SELECTION_MODE	L"default_device_mode"
#define SND_DEVICES_REGISTRY_USER_BUFFER_SIZE					L"user_buffer_size"

/* Registry related defines, local machine */
#define SND_DEVICES_REGISTRY_DFX_GUID								L"dfx_guid"
#define SND_DEVICES_REGISTRY_DEFAULT_BUFFER_SIZE				L"default_buffer_size"

/* Dfx Device sample freq flags */
#define SND_DEVICES_DFX_SAMP_FREQ_44_1			0
#define SND_DEVICES_DFX_SAMP_FREQ_48			1

/* Controls the period in MS where following device callbacks are ignored */
#define SND_DEVICES_CALLBACK_TIME_WINDOW		0
/* Controls the time in MS that the callbacks wait before setting reset flag */
#define SND_DEVICES_CALLBACK_WAIT_TIME			0
/* Controls the time in MS that RuleChange code sleeps after changing the default device */
#define SND_DEVICES_DEFAULT_CHANGE_WAIT_TIME	1500

/* Max number of allowed simultaneous encoding threads. */
#define SND_DEVICES_MAX_ENCODING_THREAD_COUNT 4

/* Time to wait in millisecs for the encoding thread termination to complete */
#define SND_DEVICES_MAX_WAIT_FOR_ENCODING_THREAD_TERMINATION 1000

#define SND_DEVICES_MAX_SAMP_FREQ 192000
#define SND_DEVICES_MIN_NUM_CHANS 2
#define SND_DEVICES_MAX_NUM_CHANS 8

// These are the average delay buffer sizes. The actual internal buffers will be twice this length.
#define SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS_32BIT_OS_32BIT_CPU 80
#define SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS_32BIT_VISTA_32BIT_CPU 100
#define SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS_32BIT_VISTA_64BIT_CPU 100
#define SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS_32BIT_OS_64BIT_CPU 60
#define SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS_64BIT_OS 40
#define SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS SND_DEVICES_CAPTURE_BUFFER_DEFAULT_SIZE_MILLI_SECS_32BIT_OS_32BIT_CPU
#define SND_DEVICES_CAPTURE_BUFFER_MIN_SIZE_MILLI_SECS 10
#define SND_DEVICES_CAPTURE_BUFFER_MAX_SIZE_MILLI_SECS 100
#define SND_DEVICES_REFTIMES_PER_SEC 1.0e7		// A fixed reference number, used in for internal calculations

// Function Status Error Macro, uses Slout module.
// First make a w_char version of __FILE__
#define SND_DEVICES_WIDE2(x) L##x
#define SND_DEVICES_WIDE1(x) SND_DEVICES_WIDE2(x)
#define SND_DEVICES_WFILE SND_DEVICES_WIDE1(__FILE__)

#define SND_DEVICES_SET_STATUS_AND_RETURN_OK(status)  { \
cast_handle->function_status = status; \
if((cast_handle->i_trace_on) && (cast_handle->slout_hdl)) \
{ \
	swprintf(cast_handle->wcp_msg1, PT_MAX_GENERIC_STRLEN, L"File %s, Line %d :: sndDevices Error Code: %d ", SND_DEVICES_WFILE, __LINE__, cast_handle->function_status); \
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1); \
 } \
return(OKAY);}

// Device callback class declarations.
class CsndDevicesMMNotificationClient : public IMMNotificationClient
{
    LONG _cRef;
    IMMDeviceEnumerator *_pEnumerator;

public:
    CsndDevicesMMNotificationClient();

    ~CsndDevicesMMNotificationClient();

	 // This is a sndDevices handle that is set during sndDevicesInit and then used in this class for all handle based ops.
	 PT_HANDLE *g_sndDevicesCallbacks_hdl;

    // IUnknown methods -- AddRef, Release, and QueryInterface

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE QueryInterface(
                                REFIID riid, VOID **ppvInterface);
    
    // Callback methods for device-event notifications.

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(
                                EDataFlow flow, ERole role,
                                LPCWSTR pwstrDeviceId);

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId);

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId);

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(
                                LPCWSTR pwstrDeviceId,
                                DWORD dwNewState);

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(
                                LPCWSTR pwstrDeviceId,
                                const PROPERTYKEY key);

	 // Added to allow handle to be set during sndDevicesInit call
	 void SetPtHandle(PT_HANDLE *sndDevices_hdl);
};

/* Capture and Playback device Volume Callback Declarations */
class CsndDevicesAudioEndpointVolumeCallbackCapture : public IAudioEndpointVolumeCallback
{
	LONG _cRef;

public:
    CsndDevicesAudioEndpointVolumeCallbackCapture();

    ~CsndDevicesAudioEndpointVolumeCallbackCapture();

	 // This is a sndDevices handle that is set during sndDevicesInit and then used in this class for all handle based ops.
	 PT_HANDLE *g_sndDevicesVolumeCallbackCapture_hdl;

    // IUnknown methods -- AddRef, Release, and QueryInterface

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface);

	 // PTNOTE - this appears to be the callback hit when the real control is moved.
    // Callback method for endpoint-volume-change notifications.
    HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify);

	 // Added to allow handle to be set during sndDevicesInit call
	 void SetPtHandle(PT_HANDLE *sndDevices_hdl);
};

class CsndDevicesAudioEndpointVolumeCallbackPlayback : public IAudioEndpointVolumeCallback
{
	LONG _cRef;

public:
    CsndDevicesAudioEndpointVolumeCallbackPlayback();

    ~CsndDevicesAudioEndpointVolumeCallbackPlayback();

	 // This is a sndDevices handle that is set during sndDevicesInit and then used in this class for all handle based ops.
	 PT_HANDLE *g_sndDevicesVolumeCallbackPlayback_hdl;

    // IUnknown methods -- AddRef, Release, and QueryInterface

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface);

	 // PTNOTE - this appears to be the callback hit when the real control is moved.
    // Callback method for endpoint-volume-change notifications.
    HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify);

	 // Added to allow handle to be set during sndDevicesInit call
	 void SetPtHandle(PT_HANDLE *sndDevices_hdl);
};


struct sndDevicesHdlType {
   
	/* Message info */
   CSlout *slout_hdl;
   wchar_t wcp_msg1[PT_MAX_GENERIC_STRLEN]; /* String for messages */
   int i_trace_on;

	// Note, originally declared as const but had to take that off to move them into global struct.
	CLSID CLSID_MMDeviceEnumerator;
	IID IID_IMMDeviceEnumerator;
	IID IID_IAudioClient;
	IID IID_IAudioCaptureClient;
	IID IID_IAudioRenderClient;
	IID IID_IAudioEndpointVolume;

	GUID guidThisApplication; // For identifying in callbacks device volume control changes made by this app

	IMMDevice *pAllDevices[SND_DEVICES_MAX_NUM_DEVICES]; // Object pointers for each device.
	IMMDevice *pCaptureDevice;
	IMMDevice *pPlaybackDevice;

	LPWSTR pwszID[SND_DEVICES_MAX_NUM_DEVICES]; // For the GUID ID strings for each device, all devices combined.
	LPWSTR pwszIDRealDevices[SND_DEVICES_MAX_NUM_DEVICES]; // For the GUID ID strings for each real playback device.
	WCHAR pwszIDPreviousRealDevices[SND_DEVICES_MAX_NUM_DEVICES][PT_MAX_GENERIC_STRLEN]; // To detect when a new devices is added.

	wchar_t deviceFriendlyName[SND_DEVICES_MAX_NUM_DEVICES][PT_MAX_GENERIC_STRLEN];  // Friendly name, ie "DFX Audio Enhancer 10.5", all devices.
	wchar_t deviceDescription[SND_DEVICES_MAX_NUM_DEVICES][PT_MAX_GENERIC_STRLEN];  // Descriptive name, ie "Speakers", all devices.
	wchar_t *deviceFriendlyNameRealDevices[SND_DEVICES_MAX_NUM_DEVICES];  // Friendly names, just the real devices.
	wchar_t *deviceDescriptionRealDevices[SND_DEVICES_MAX_NUM_DEVICES];  // Descriptive names, just the real devices.
	int deviceNumChannel[SND_DEVICES_MAX_NUM_DEVICES]; // Number of channels for all devices.

   IAudioClient *pAudioClientCapture;
   IAudioCaptureClient *pAudioCaptureLoopback;
	IAudioEndpointVolume *pEndptVolCapture;
   IAudioClient *pAudioClientPlayback;
   IAudioRenderClient *pAudioClientPlaybackRender;
	IAudioEndpointVolume *pEndptVolPlayback;

	CsndDevicesMMNotificationClient DeviceEvents; // For device callbacks.
	CsndDevicesAudioEndpointVolumeCallbackCapture EPVolEventsCapture;	  // For capture device volume callbacks.
	CsndDevicesAudioEndpointVolumeCallbackPlayback EPVolEventsPlayback; // For playback device volume callbacks.

   REFERENCE_TIME hnsRequestedDurationCapture;
   REFERENCE_TIME hnsActualDurationCapture;
   REFERENCE_TIME hnsRequestedDurationPlayback;
   REFERENCE_TIME hnsActualDurationPlayback;

	WAVEFORMATEX wfxCapture;
	WAVEFORMATEX wfxPlayback;
	WAVEFORMATEX wfxDfxProcessing;
	WAVEFORMATEX wfxRecording;

   UINT32 numCaptureFramesAvailable;

   UINT32 bufferFrameSizeCapture;  // Contain the actual full size of the internal shared buffers in frames.
   UINT32 bufferFrameSizePlayback;

	// For small packet exchanges, these point to temp memory in the record and playback methods
   BYTE *pDataPacketCapture;
   BYTE *pDataPacketPlayback;

	// Full length buffers, scale up a little for rounding slop
	// Now allocated in init call.
	float *fCaptureBuf;
	float *fPlaybackBuf;
	float *fFilePlaybackBuf;
	int captureBufAllocSize;
	int playbackBufAllocSize;

	UINT32 capturedFramesCount;		// The number of frames we have read from the system capture device.
	UINT32 playbackFrameCount;			// The number of frames we have ready to pass to the system playback device.
    UINT32 numPlaybackFramesAvailableToFill;	// The number of open frames available to fill in the system playback buffer.

	UINT32 upsampleRatio;
	float sigPower;
	int bufferSizeMilliSecs;		// This is the average delay, actual internal buffers are twice this length.
	//int playback_has_started;
	int stopAudioCaptureAndPlaybackLoop;
	int dfxDeviceNum;	// The combo 44.1k and 48k hz. DFX device
	//int dfx48DeviceNum;	// The 48k hz. DFX device
	int defaultDeviceNum;
	int playbackDeviceNum;
	int captureDeviceNum;
	int priorDefaultDeviceNum;
	int totalNumDevices;
	int numPreviousRealDevices;
	int numRealDevices;
	int deviceCallBackType;
	DWORD lastDeviceCallbackTime;
	DWORD lastDeviceAddCallbackGuidtype;
	wchar_t lastDeviceAddCallbackGuid[PT_MAX_GENERIC_STRLEN]; // Contains the guid of the last device added or removed that generated a callback.
	int totalCallbackCount;			// For debugging.
	int totalReInitCalls;			// For debugging.
	BOOL ignoreDeviceCallbacks;
	int initializationMode;		// Will be either SND_DEVICES_INIT_FOR_PROCESSING or SND_DEVICES_INIT_NO_PROCESSING
	int playbackIsActive;		// Will be either SND_DEVICES_PLAYBACK_IS_STOPPED or SND_DEVICES_PLAYBACK_IS_ACTIVE.
	int playbackStreamIsTemporarilyPaused;	// Used to turn off playback stream when no capture or playback is occuring.

	int noBufferCount;			// This is a count of how many times no buffers were available in a DoRecording call.
	int WindowsSilentBufferCount;				// This is a buffer specified by the Windows flag to be all zeros.
	int MeasuredVirtualSilentBufferCount;	// This is buffer of measured all very low values, essentially silence.
	int MeasuredTrueSilentBufferCount;		// This is buffer of measured all true zero values.
    BOOL playbackDeviceIsUnavailable;

	// Module common status flag, set by functions that can't complete their requestion operation
	int function_status;
};

_COM_SMARTPTR_TYPEDEF(IMMDevice, __uuidof(IMMDevice));
_COM_SMARTPTR_TYPEDEF(IMMDeviceEnumerator, __uuidof(IMMDeviceEnumerator));
_COM_SMARTPTR_TYPEDEF(IMMDeviceCollection, __uuidof(IMMDeviceCollection));
_COM_SMARTPTR_TYPEDEF(IPropertyStore, __uuidof(IPropertyStore));

int PT_DECLSPEC sndDevicesInit(PT_HANDLE *, CSlout *, int, int, int *);
int PT_DECLSPEC sndDevicesFree(PT_HANDLE *);

/* sndDevicesReInit.cpp */
int PT_DECLSPEC sndDevicesReInit(PT_HANDLE *, int, int *, int *, int *);

/* sndDevicesGet.cpp */
int PT_DECLSPEC sndDevicesGetID(PT_HANDLE *, int, wchar_t *, int *);
int PT_DECLSPEC sndDevicesGetFormatFromID(PT_HANDLE *, wchar_t *, WAVEFORMATEX *, int *);
int PT_DECLSPEC sndDevicesGetFriendlyName(PT_HANDLE *, int , wchar_t *, int *);
int PT_DECLSPEC sndDevicesGetFriendlyNameFromID(PT_HANDLE *, wchar_t *, wchar_t *, int *);
int PT_DECLSPEC sndDevicesGetDeviceName(PT_HANDLE *, int, wchar_t *, int *);
int PT_DECLSPEC sndDevicesGetDeviceNameFromID(PT_HANDLE *, wchar_t *, wchar_t *, int *);
int PT_DECLSPEC sndDevicesGetNumberOfChannelsFromID(PT_HANDLE *, wchar_t *, int *, int *);
int PT_DECLSPEC sndDevicesGetIconFileName(PT_HANDLE *, int, wchar_t *, int *);
int PT_DECLSPEC sndDevicesGetLoopStopFlag(PT_HANDLE *, int *);
int PT_DECLSPEC sndDevicesGetBuffersForProcessing(PT_HANDLE *, float **, int *, WAVEFORMATEX **);
int PT_DECLSPEC sndDevicesGetTotalNumDevices(PT_HANDLE *,  int *);
int PT_DECLSPEC sndDevicesGetIDFromIndex(PT_HANDLE *, int, wchar_t *);
int PT_DECLSPEC sndDevicesGetDefaultDeviceSelectionMode(PT_HANDLE *, int *);
int PT_DECLSPEC sndDevicesGetAllRealDeviceIDs(PT_HANDLE *, wchar_t ***, int *);
int PT_DECLSPEC sndDevicesGetBufferSizeMilliSecs(PT_HANDLE *, int, int *);
int PT_DECLSPEC sndDevicesGetRecommendedBufferSizeMilliSecs(PT_HANDLE *, int *);
int PT_DECLSPEC sndDevicesGetPlayBackStatus(PT_HANDLE *, int *);
int PT_DECLSPEC sndDevicesGetNumMonoDevices(PT_HANDLE *, int *);
int PT_DECLSPEC sndDevicesGetPlaybackDeviceAvialblility(PT_HANDLE*, BOOL*);

/* sndDevicesSet.cpp */
int PT_DECLSPEC sndDevicesSetDeviceType(PT_HANDLE *, int, wchar_t *, int *);
int PT_DECLSPEC sndDevicesSetIconFileName(PT_HANDLE *, int, wchar_t *, int *);
int PT_DECLSPEC sndDevicesSetDeviceName(PT_HANDLE *, int, wchar_t *, int *);
int PT_DECLSPEC sndDevicesSetDefaultDeviceSelectionMode(PT_HANDLE *, int);
int PT_DECLSPEC sndDevicesSetDeviceEnabledStatus(PT_HANDLE *, int, BOOL, int *);
int PT_DECLSPEC sndDevicesSetDeviceEnabledStatusFromGuid(PT_HANDLE *, wchar_t *, BOOL, int *);
int PT_DECLSPEC sndDevicesSetDfxDeviceSampleRateAndChannels(PT_HANDLE *, int, int, int *);
int PT_DECLSPEC sndDevicesSetBufferSizeMilliSecs(PT_HANDLE *, int);

/* sndDevicesImplementDeviceRules.cpp */
int PT_DECLSPEC sndDevicesImplementDeviceRules(PT_HANDLE *, int *);

/* sndDevicesSetupDevices.cpp */
int PT_DECLSPEC sndDevicesInitialSetupCaptureDevice(PT_HANDLE *, int *);
int PT_DECLSPEC sndDevicesInitialSetupPlaybackDevice(PT_HANDLE *, int *);
int PT_DECLSPEC sndDevicesFinalSetupCaptureDevice(PT_HANDLE *, int *);
int PT_DECLSPEC sndDevicesFinalSetupPlaybackDevice(PT_HANDLE *, int *);
int PT_DECLSPEC sndDevicesRestoreDefaultDevice(PT_HANDLE *, int *);

/* sndDevicesDoCapture.cpp */
int PT_DECLSPEC sndDevicesDoCapture(PT_HANDLE *, float **, int *, WAVEFORMATEX **, int *);
int PT_DECLSPEC sndDevicesStartStopCapture(PT_HANDLE *, int);

/* sndDevicesDoPlayback.cpp */
int PT_DECLSPEC sndDevicesDoPlayback(PT_HANDLE *, int *);

/* sndDevicesReg.cpp */
int sndDevicesWriteToRegistry(PT_HANDLE *, int, wchar_t *, wchar_t *);
int sndDeviceReadFromRegistry(PT_HANDLE *, int, wchar_t *, wchar_t *);

#endif //_SND_DEVICES_H_