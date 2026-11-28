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

//#include "stdafx.h"
#include "u_AudioPassthru.h"
#include <algorithm>
#include <exception>
#include "reg.h"
#include "sndDevices.h"
#include "DeviceCatalog.h"

#define DFXG_SND_SERVER_KILL_THREAD_TIMEOUT_MSECS		  3000
#define DFXG_TRUNCATED_DRIVER_TEXT_LENGTH			      50

// See https://stackoverflow.com/questions/6472948/converting-member-function-pointer-to-timerproc
//std::map<UINT_PTR, AudioPassthru*> AudioPassthru::m_AudioPassthruClassMap;  //definition

// According to Paul's note, sndDevice handle must be declared statically.
// He didn't explain fully why (something to do with device event callbacks) 
// but without doing this I was getting access violiation excpeiotn in MMDevApi.dll so 
// there is some truth to it.
// A late MMDevice callback can outlive an exceptional shutdown. Keep its
// backing storage (including immutable queue IDs) alive until process exit.
sndDevicesHdlType& AudioPassthruPrivate::s_sndDevices_ = *new sndDevicesHdlType();
std::atomic<AudioPassthruCallback*> AudioPassthruPrivate::s_callback_{nullptr};
std::atomic<HWND> AudioPassthruPrivate::s_notification_window_{nullptr};
std::atomic<bool> AudioPassthruPrivate::s_device_change_pending_{false};
std::atomic<bool> AudioPassthruPrivate::s_device_message_posted_{false};
PTP_TIMER AudioPassthruPrivate::s_device_retry_ = nullptr;
std::atomic<bool> AudioPassthruPrivate::s_retry_armed_{false};

AudioPassthruPrivate::AudioPassthruPrivate()
{
	hp_sndDevices_ = (PT_HANDLE *)&(s_sndDevices_);
	//hp_sndDevices_ = hp_sndDevices;
	hProcessingThread_ = NULL;
	ProcessingThreadID_ = (DWORD)0;
	i_kill_processing_thread_ = IS_FALSE;
	swprintf(wcp_playback_device_guid_, PT_MAX_GENERIC_STRLEN, L"");
	b_no_valid_snd_device_dialog_shown_ = false;
	debug_ = IS_TRUE;
}

AudioPassthruPrivate::~AudioPassthruPrivate()
{
    // The wrapper deletes us only after the coordinator has joined its thread.
    // Audio/COM cleanup has already happened there, never on the UI thread.
    registerCallback(nullptr);
}

int AudioPassthruPrivate::init()
{
	int status_flag;
	
    initialization_started_ = true;
    s_device_retry_ = CreateThreadpoolTimer(retryDeviceNotice, nullptr, nullptr);
    if (s_device_retry_ == nullptr)
        OutputDebugStringW(L"FxSound: unable to create the device-notice retry timer\n");
	/* Initialize the handle */
	if (sndDevicesInit(this->hp_sndDevices_, NULL, SND_DEVICES_INIT_FOR_PROCESSING, debug_, &status_flag) != OKAY)
		return(NOT_OKAY);

	s_sndDevices_.deviceChangeCallback = onDeviceChange;
    refreshRestoreSnapshot();
    publishUsedEndpoints();
    sndDeviceHandleToSoundDevices(false);
    if (s_sndDevices_.deviceChanges.hasPending()) onDeviceChange();

	if (status_flag != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
	{
		// Its possible that the audio system was in transition and returned a not ready status that caused us to arrive here.
		// The timer will force a call to sndDevicesReInit on first play attempt so there is no need to call it here, this section should only deal with fatal errors.
		if (debug_)
		{
			//swprintf(cast_handle->wcp_msg1, 2048, L"dfxg_SndServerInit(): sndDevicesInit failed, status_flag = %d", status_flag);
			//(cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
		}

		// Fatal errors.
		if (status_flag == SND_DEVICES_MP3_DLL_LOAD_FAILED)
		{
			// We were unable to load the MP3 decoding dll (only applies to MR), in addition to putting trace statement here we may want to add message box to user here.
		}
	}

	/* Initialize code for fixing the case of HDMI monitors being plugged in */
	//if (dfxg_SndServerHdmiFixInit(hp_dfxg) != OKAY)
		//return(NOT_OKAY);

	//pDspProcessingModule_->init();

	return(OKAY);
}

void AudioPassthruPrivate::setDspProcessingModule(DfxDsp* p_dfx_dsp)
{
	p_dfx_dsp_ = p_dfx_dsp;
}


std::vector<SoundDevice> AudioPassthruPrivate::getSoundDevices(bool active_devices)
{
    std::vector<SoundDevice> result;
    for (const auto& device : sound_devices_)
        if (!active_devices || device.isActive) result.push_back(device);
    return result;
}

int AudioPassthruPrivate::sndDeviceHandleToSoundDevices(bool active_devices)
{
	int i_resultFlag;
	wchar_t wcp_user_seleted_playback_device_guid[PT_MAX_GENERIC_STRLEN]{};
	wchar_t wcp_targeted_real_playback_device_guid[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_capture_device_guid[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_dfx_device_guid[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_default_device_guid[PT_MAX_GENERIC_STRLEN];

	// Cast the handle so we can access its data.
	struct sndDevicesHdlType *cast_handle;
	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices_;

	// Clear SoundDevices
	sound_devices_.clear();

	// Get the guid of the currently selected real playback device
	if (sndDevicesGetID(hp_sndDevices_, SND_DEVICES_USER_SELECTED_PLAYBACK_DEVICE, wcp_user_seleted_playback_device_guid, &i_resultFlag) != OKAY ||
        sndDevicesGetID(hp_sndDevices_, SND_DEVICES_TARGETED_REAL_PLAYBACK, wcp_targeted_real_playback_device_guid, &i_resultFlag) != OKAY ||
		sndDevicesGetID(hp_sndDevices_, SND_DEVICES_CAPTURE, wcp_capture_device_guid, &i_resultFlag) != OKAY ||
		sndDevicesGetID(hp_sndDevices_, SND_DEVICES_VIRTUAL_PLAYBACK_DFX, wcp_dfx_device_guid, &i_resultFlag) != OKAY ||
		sndDevicesGetID(hp_sndDevices_, SND_DEVICES_DEFAULT, wcp_default_device_guid, &i_resultFlag) != OKAY)
	{
		return(NOT_OKAY);
	}

	for (int index = 0; index < cast_handle->totalNumDevices && index < SND_DEVICES_MAX_NUM_DEVICES; index++)
	{
		if (cast_handle->pwszID[index][0] == L'\0' || cast_handle->deviceFriendlyName[index][0] == L'\0')
		{
			continue;
		}

		if (active_devices && cast_handle->deviceState[index] != DEVICE_STATE_ACTIVE)
		{
			continue;
		}

		SoundDevice sound_device;
		sound_device.pwszID = std::wstring(cast_handle->pwszID[index]);
		sound_device.deviceFriendlyName = std::wstring(cast_handle->deviceFriendlyName[index]);
		sound_device.deviceDescription = std::wstring(cast_handle->deviceDescription[index] != NULL ? cast_handle->deviceDescription[index] : L"");
		sound_device.deviceNumChannel = cast_handle->deviceNumChannel[index];
		sound_device.deviceFormFactor = std::wstring(cast_handle->deviceFormFactor[index]);
		if (cast_handle->deviceState[index] == DEVICE_STATE_ACTIVE)
		{
			sound_device.isActive = true;
		}
		else
		{
			sound_device.isActive = false;
        }

		// Skip mono devices if SND_DEVICES_MONO_BUG_SKIP_MONO_DEVICES is IS_TRUE
		if (SND_DEVICES_MONO_BUG_SKIP_MONO_DEVICES && sound_device.deviceNumChannel == 1)
		{
			continue;
		}

		// Figure out if this is a real device or not
		sound_device.isRealDevice = false;
		for (int index2 = 0; index2 < cast_handle->numRealDevices && index2 < SND_DEVICES_MAX_NUM_DEVICES; index2++)
		{
			if (cast_handle->pwszIDRealDevices[index2] != nullptr && sound_device.pwszID == cast_handle->pwszIDRealDevices[index2])
			{
				sound_device.isRealDevice = true;
			}
		}

		if (sound_device.pwszID == wcp_user_seleted_playback_device_guid)
		{
			sound_device.isUserSelectedPlaybackDevice = true;
		}
		if (sound_device.pwszID == wcp_targeted_real_playback_device_guid &&
			sound_device.pwszID != wcp_dfx_device_guid)
		{
			sound_device.isTargetedRealPlaybackDevice = true;
		}
		if (sound_device.pwszID == wcp_capture_device_guid)
		{
			sound_device.isCaptureDevice = true;
		}
		if (sound_device.pwszID == wcp_dfx_device_guid)
		{
			sound_device.isDFXDevice = true;
		}
		if (sound_device.pwszID == wcp_default_device_guid)
		{
			sound_device.isDefaultDevice = true;
		}

		sound_devices_.push_back(sound_device);
	}	

	return(OKAY);
}

void AudioPassthruPrivate::onDeviceChange() noexcept
{
    // MMDevice copied the ID; only events affecting the used route stopped it.
    // Pending survives a missing HWND; attaching the window retries delivery.
    if (s_sndDevices_.deviceNotificationsClosed.load(std::memory_order_acquire)) return;
    s_device_change_pending_.store(true, std::memory_order_release);
    const HWND window = s_notification_window_.load(std::memory_order_acquire);
    if (window == nullptr) return;
    if (!s_device_message_posted_.exchange(true, std::memory_order_acq_rel)
        && !PostMessageW(window, AudioPassthru::deviceChangeMessage, 0, 0))
    {
        s_device_message_posted_.store(false, std::memory_order_release);
        armDeviceRetry();
    }
}

void AudioPassthruPrivate::armDeviceRetry() noexcept
{
    if (s_device_retry_ == nullptr || s_notification_window_.load(std::memory_order_acquire) == nullptr
        || s_sndDevices_.deviceNotificationsClosed.load(std::memory_order_acquire)) return;
    if (s_retry_armed_.exchange(true, std::memory_order_acq_rel)) return;
    LARGE_INTEGER delay{};
    delay.QuadPart = -50 * 10000; // One pending-transition retry, never an idle poll.
    FILETIME due{delay.LowPart, static_cast<DWORD>(delay.HighPart)};
    SetThreadpoolTimer(s_device_retry_, &due, 0, 0);
}

void CALLBACK AudioPassthruPrivate::retryDeviceNotice(PTP_CALLBACK_INSTANCE, void*, PTP_TIMER) noexcept
{
    s_retry_armed_.store(false, std::memory_order_release);
    if (s_device_change_pending_.load(std::memory_order_acquire)) onDeviceChange();
}

void AudioPassthruPrivate::closeDeviceRetry() noexcept
{
    // Called by the cleanup task after unregistering/draining MMDevice callbacks.
    if (s_device_retry_ == nullptr) return;
    SetThreadpoolTimer(s_device_retry_, nullptr, 0, 0);
    WaitForThreadpoolTimerCallbacks(s_device_retry_, TRUE);
    CloseThreadpoolTimer(s_device_retry_);
    s_device_retry_ = nullptr;
}

void AudioPassthruPrivate::setDeviceNotificationWindow(HWND window) noexcept
{
    s_notification_window_.store(window, std::memory_order_release);
    if (window != nullptr && !stopping_ && (s_device_change_pending_.load(std::memory_order_acquire)
        || s_sndDevices_.deviceChanges.hasPending())) onDeviceChange();
}

void AudioPassthruPrivate::dispatchDeviceChange()
{
    s_device_message_posted_.store(false, std::memory_order_release);
    if (stopping_ || !s_device_change_pending_.load(std::memory_order_acquire)) return;
    s_device_change_pending_.store(false, std::memory_order_release);
    const bool notice = drainDeviceChanges();
    const bool catalogUpdated = refreshPendingCatalogue(notice);
    if (notice && s_sndDevices_.playbackDeviceIsUnavailable)
    {
        s_sndDevices_.playbackDeviceIsUnavailable = FALSE;
        route_change_pending_ = true;
    }
    // Publish copied catalogue values while the old worker can still be alive.
    // The callback may choose a fallback and replace pending_output_id_.
    if (catalogUpdated)
        if (auto* callback = s_callback_.load(std::memory_order_acquire)) callback->onSoundDeviceChange(false);
    if (!route_change_pending_)
    {
        if (catalogue_dirty_)
        {
            s_device_change_pending_.store(true, std::memory_order_release);
            armDeviceRetry();
        }
        if (s_sndDevices_.deviceChanges.hasPending()) onDeviceChange();
        return;
    }
    DWORD error = ERROR_SUCCESS;
    if (pollStopped(error) != WAIT_OBJECT_0)
    {
        s_device_change_pending_.store(true, std::memory_order_release);
        armDeviceRetry();
        return;
    }
    if (!applyPendingOutput())
    {
        s_device_change_pending_.store(true, std::memory_order_release);
        armDeviceRetry();
    }
    if (catalogue_dirty_)
    {
        s_device_change_pending_.store(true, std::memory_order_release);
        armDeviceRetry();
    }
    if (s_sndDevices_.deviceChanges.hasPending()) onDeviceChange();
}

bool AudioPassthruPrivate::drainDeviceChanges()
{
    bool notice = false;
    fxsounddevices::DeviceChangeQueue::Event event{};
    while (s_sndDevices_.deviceChanges.tryPop(event))
    {
        if (s_sndDevices_.deviceChanges.isDuplicate(event)) continue;
        notice = true;
        // Re-evaluate against the latest published route, not the route that
        // existed when this event was queued during a previous ReInit.
        if (event.kind == fxsounddevices::DeviceChangeQueue::Kind::defaultChanged
            || s_sndDevices_.deviceChanges.usesEndpoint(event.missingId ? nullptr : event.id))
            route_change_pending_ = true;
    }
    if (s_sndDevices_.deviceChanges.takeReconcileRequired())
    {
        notice = true;
        route_change_pending_ = true;
        OutputDebugStringW(L"FxSound: reconciling an uncertain endpoint notification\n");
    }
    if (route_change_pending_) signalCaptureStop();
    return notice;
}

void AudioPassthruPrivate::overlayRouteFlags()
{
    for (auto& item : sound_devices_)
    {
        item.isCaptureDevice = item.pwszID == route_capture_id_;
        item.isTargetedRealPlaybackDevice = !route_output_id_.empty() && item.pwszID == route_output_id_;
        if (!route_virtual_id_.empty()) item.isDFXDevice = item.pwszID == route_virtual_id_;
        item.isRealDevice = !item.isDFXDevice;
        item.pAllDevices = nullptr;
    }
}

bool AudioPassthruPrivate::refreshDeviceCatalogue()
{
    std::vector<SoundDevice> next;
    std::wstring currentDefault;
    const HRESULT hr = fxsounddevices::readDeviceCatalog(next, currentDefault);
    if (FAILED(hr))
    {
        OutputDebugStringW(L"FxSound: catalogue refresh failed; retaining its previous values\n");
        return false;
    }
    for (auto& item : next)
        for (const auto& previous : sound_devices_)
            if (item.pwszID == previous.pwszID)
            {
                item.isUserSelectedPlaybackDevice = previous.isUserSelectedPlaybackDevice;
                if (!item.isActive && item.deviceNumChannel == 0) item.deviceNumChannel = previous.deviceNumChannel;
                break;
            }
    sound_devices_ = std::move(next);
    overlayRouteFlags();
    return true;
}

bool AudioPassthruPrivate::refreshPendingCatalogue(bool notice)
{
    const auto now = GetTickCount64();
    if (notice)
    {
        catalogue_dirty_ = catalogue_notice_pending_ = true;
        catalogue_retry_after_ = now;
        catalogue_retry_until_ = now + 3000;
    }
    if (!catalogue_dirty_ || now < catalogue_retry_after_) return false;
    if (refreshDeviceCatalogue())
    {
        catalogue_dirty_ = false;
        const bool publish = catalogue_notice_pending_;
        catalogue_notice_pending_ = false;
        return publish;
    }
    if (now >= catalogue_retry_until_)
    {
        catalogue_dirty_ = catalogue_notice_pending_ = false;
        OutputDebugStringW(L"FxSound: catalogue retry budget exhausted; waiting for the next device event\n");
    }
    else catalogue_retry_after_ = now + 250;
    return false;
}

bool AudioPassthruPrivate::publishUsedEndpoints() noexcept
{
    try
    {
        const auto id = [](int index) -> const wchar_t*
        {
            return index >= 0 && index < s_sndDevices_.totalNumDevices && index < SND_DEVICES_MAX_NUM_DEVICES
                ? s_sndDevices_.pwszID[index] : L"";
        };
        route_virtual_id_ = id(s_sndDevices_.dfxDeviceNum);
        route_capture_id_ = id(s_sndDevices_.captureDeviceNum);
        route_output_id_ = id(s_sndDevices_.playbackDeviceNum);
        if (!s_sndDevices_.deviceChanges.setVirtualDeviceId(route_virtual_id_.c_str()))
        { s_sndDevices_.deviceChanges.invalidateUsedEndpoints(); return false; }
        return s_sndDevices_.deviceChanges.setUsedEndpoints(route_virtual_id_.c_str(),
            route_capture_id_.c_str(), route_output_id_.c_str());
    }
    catch (const std::exception&)
    {
        s_sndDevices_.deviceChanges.invalidateUsedEndpoints();
        return false;
    }
}

bool AudioPassthruPrivate::applyPendingOutput()
{
    if (pending_output_id_.empty()) return true;
    auto id = pending_output_id_;
    const auto generation = pending_output_generation_;
    const auto found = std::find_if(sound_devices_.begin(), sound_devices_.end(), [&](const SoundDevice& item)
    { return item.isActive && item.isRealDevice && item.pwszID == id; });
    if (found == sound_devices_.end()) { pending_output_id_.clear(); return true; }
    const BOOL oldVolume = s_sndDevices_.ignoreVolumeCallbacks.exchange(TRUE);
    if (s_sndDevices_.deviceCallbacksInFlight.load() != 0)
    { s_sndDevices_.ignoreVolumeCallbacks.store(oldVolume); return false; }
    // All engine users have retired; now a newly catalogued ID can enter its
    // mutable table. This path is never used by catalogue-only notifications.
    int status = SND_DEVICES_DEVICE_OPERATION_COMPLETED;
    const int enumerated = sndDevices_GetAll(hp_sndDevices_, &s_sndDevices_.totalNumDevices);
    if (generation != pending_output_generation_)
    { s_sndDevices_.ignoreVolumeCallbacks.store(oldVolume); return false; }
    const int selected = enumerated == OKAY ? sndDevicesSetDeviceType(hp_sndDevices_, SND_DEVICES_DEFAULT,
        &id[0], &status) : NOT_OKAY_NO_BREAK;
    s_sndDevices_.ignoreVolumeCallbacks.store(oldVolume);
    if (generation != pending_output_generation_) return false;
    pending_output_id_.clear();
    if (selected != OKAY || status != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
        OutputDebugStringW(L"FxSound: requested output was not applied; refreshing the route\n");
    refreshRestoreSnapshot();
    return true;
}

void AudioPassthruPrivate::requestStop() noexcept
{
    stopping_ = true;
    s_sndDevices_.deviceNotificationsClosed.store(true);
    s_sndDevices_.deviceChangeCallback.store(nullptr, std::memory_order_release);
    setDeviceNotificationWindow(nullptr);
    registerCallback(nullptr);
    signalCaptureStop();
}

void AudioPassthruPrivate::refreshRestoreSnapshot() noexcept
{
    // Only the owner reads tables/registry. The shutdown task receives immutable
    // strings, never the live sndDevices handle or any of its COM interfaces.
    try
    {
        auto snapshot = std::make_shared<fxaudio::EndpointRestoreSnapshot>();
        const int virtualIndex = s_sndDevices_.dfxDeviceNum;
        if (virtualIndex >= 0 && virtualIndex < s_sndDevices_.totalNumDevices && virtualIndex < SND_DEVICES_MAX_NUM_DEVICES)
            snapshot->virtualId = s_sndDevices_.pwszID[virtualIndex];
        const auto add = [&](const wchar_t* id)
        {
            if (id != nullptr && id[0] != 0 && snapshot->virtualId != id
                && std::find(snapshot->candidates.begin(), snapshot->candidates.end(), id) == snapshot->candidates.end())
                snapshot->candidates.emplace_back(id);
        };
        const wchar_t* keys[] = {SND_DEVICES_REGISTRY_USER_SELECTED_PLAYBACK_WIDE,
            SND_DEVICES_REGISTRY_MOST_RECENT_PLAYBACK_WIDE, SND_DEVICES_REGISTRY_MOST_RECENT_DEFAULT_WIDE,
            SND_DEVICES_REGISTRY_PRIOR_DEFAULT_WIDE, SND_DEVICES_REGISTRY_ORIGINAL_DEFAULT_WIDE};
        for (const auto* key : keys)
        {
            wchar_t id[PT_MAX_GENERIC_STRLEN]{};
            if (sndDeviceReadFromRegistry(hp_sndDevices_, REG_CURRENT_USER, const_cast<wchar_t*>(key), id) == OKAY)
                add(id);
        }
        const int count = (std::max)(0, (std::min)(s_sndDevices_.totalNumDevices, SND_DEVICES_MAX_NUM_DEVICES));
        for (int i = 0; i < count; ++i) add(s_sndDevices_.pwszID[i]);
        snapshot->hasSavedMute = s_sndDevices_.playbackSettingsAreSaved != FALSE;
        snapshot->savedMuteId = s_sndDevices_.savedPlaybackDeviceID;
        snapshot->savedMute = s_sndDevices_.savedPlaybackMute != FALSE;
        restore_snapshot_ = std::move(snapshot);
    }
    catch (const std::exception&)
    {
        OutputDebugStringW(L"FxSound: unable to refresh endpoint snapshot; keeping previous snapshot\n");
    }
}

fxaudio::ShutdownTask::Outcome AudioPassthruPrivate::shutdown(DWORD timeoutMs) noexcept
{
    requestStop();
    try
    {
        if (!shutdown_task_.status().started)
        {
            auto snapshot = restore_snapshot_ ? std::make_shared<fxaudio::EndpointRestoreSnapshot>(*restore_snapshot_) : nullptr;
            fxaudio::ShutdownTask::Work work;
            work.waitWorker = [this, snapshot](DWORD budget)
            {
                const ULONGLONG deadline = GetTickCount64() + budget;
                bool stopped = hProcessingThread_ == nullptr || hProcessingThread_ == INVALID_HANDLE_VALUE;
                if (!stopped && WaitForSingleObject(hProcessingThread_, budget) == WAIT_OBJECT_0)
                {
                    CloseHandle(hProcessingThread_);
                    hProcessingThread_ = nullptr;
                    stopped = true;
                }
                while (s_sndDevices_.deviceCallbacksInFlight.load() != 0 && GetTickCount64() < deadline)
                    Sleep(1);
                const bool quiet = s_sndDevices_.deviceCallbacksInFlight.load() == 0;
                if (snapshot)
                {
                    // The gate is closed. After old volume callbacks drain, take
                    // their last mute observation without I/O or table enumeration.
                    snapshot->savedMute = s_sndDevices_.savedPlaybackMute.load() != FALSE;
                    if (!quiet) snapshot->hasSavedMute = false;
                }
                if (!quiet)
                    OutputDebugStringW(L"FxSound: volume callback still busy; restoring endpoint policy without mute\n");
                // A busy callback also prohibits freeing the runtime. The policy
                // restoration is still attempted independently of this result.
                return stopped && quiet;
            };
            work.restore = [snapshot]() { return snapshot ? fxaudio::restoreEndpoint(*snapshot) : S_FALSE; };
            work.cleanup = [this]()
            {
                const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
                if (FAILED(com)) return false;
                const bool freed = !initialization_started_ || sndDevicesFree(hp_sndDevices_) == OKAY;
                closeDeviceRetry();
                if (freed) hp_sndDevices_ = nullptr;
                CoUninitialize();
                return freed;
            };
            shutdown_task_.start(std::move(work));
        }
        return shutdown_task_.wait(timeoutMs);
    }
    catch (const std::exception&)
    {
        OutputDebugStringW(L"FxSound: unable to start audio teardown; retaining runtime until process exit\n");
        return shutdown_task_.status();
    }
}

void AudioPassthruPrivate::signalCaptureStop() noexcept
{
    i_kill_processing_thread_ = IS_TRUE;
    s_sndDevices_.stopAudioCaptureAndPlaybackLoop = 1;
}

DWORD AudioPassthruPrivate::pollStopped(DWORD& error) noexcept
{
    error = ERROR_SUCCESS;
    if (hProcessingThread_ == NULL || hProcessingThread_ == INVALID_HANDLE_VALUE)
    {
        hProcessingThread_ = NULL;
        return WAIT_OBJECT_0;
    }
    const DWORD result = WaitForSingleObject(hProcessingThread_, 0);
    if (result == WAIT_OBJECT_0)
    {
        CloseHandle(hProcessingThread_);
        hProcessingThread_ = NULL;
    }
    else if (result == WAIT_FAILED)
    {
        error = GetLastError();
    }
    return result;
}

/*
* FUNCTION: killProcessingThread()
* DESCRIPTION:
*
*  If the processing thread is running, kill it.
*  This function does not return until the thread has been killed or it times out
*  trying to kill the thread.
*
*/
int AudioPassthruPrivate::killProcessingThread(int *ip_timed_out)
{
    signalCaptureStop();
    *ip_timed_out = IS_FALSE;
    if (hProcessingThread_ == NULL || hProcessingThread_ == INVALID_HANDLE_VALUE)
    {
        hProcessingThread_ = NULL;
        return OKAY;
    }
    const DWORD result = WaitForSingleObject(hProcessingThread_, DFXG_SND_SERVER_KILL_THREAD_TIMEOUT_MSECS);
    if (result == WAIT_TIMEOUT)
    {
        *ip_timed_out = IS_TRUE;
        return OKAY;
    }
    if (result != WAIT_OBJECT_0) return NOT_OKAY;
    CloseHandle(hProcessingThread_);
    hProcessingThread_ = NULL;
    return OKAY;
}

/*
* FUNCTION: setBufferLength()
* DESCRIPTION:
*
*  Sets the buffer length.
*
*/
int AudioPassthruPrivate::setBufferLength(int i_buffer_length_msecs)
{
	int i_timed_out;

	/*
	* Set the new buffer setting.
	* This just sets registry value which will be picked up during reinit.
	*/
	if (sndDevicesSetBufferSizeMilliSecs(hp_sndDevices_, i_buffer_length_msecs) != OKAY)
		return(NOT_OKAY);

	/* Kill the processing thread, so that the timer will then restart it with the new buffer setting */
	if (killProcessingThread(&i_timed_out) != OKAY)
		return(NOT_OKAY);

	/* If the killing of processing attempt timed out, then we can't do anything else */
	if (i_timed_out)
		return(OKAY);

	return(OKAY);
}

/*
* FUNCTION: processTimer()
* DESCRIPTION:
*
*  Process the snd server timer.
*/
int AudioPassthruPrivate::processTimer()
{
    if (stopping_) return OKAY;
    if (s_device_change_pending_.load(std::memory_order_acquire))
    {
        dispatchDeviceChange();
        if (route_change_pending_ && s_device_change_pending_.load(std::memory_order_acquire)) return OKAY;
    }
	BOOL b_need_to_start_thread;
	int numRealDevices;
	int DfxDeviceEnabledFlag;
	int statusFlag;

	b_need_to_start_thread = FALSE;

	/*
	* Check if we need to start the thread for the first time or restart it.
	* Keep in mind that the main thread will kill itself if anything changes in terms of active soundcards.
	* This is kind of a brute force way of reinitializing and taking care of changes.
	*/
	if (hProcessingThread_ == NULL)
		b_need_to_start_thread = TRUE;
	else if (hProcessingThread_ == INVALID_HANDLE_VALUE)
	{
		// Playback device was unavailable on the last attempt. Wait for onDeviceChange() to
		// clear playbackDeviceIsUnavailable before retrying, to avoid hammering sndDevicesReInit.
		if (!s_sndDevices_.playbackDeviceIsUnavailable)
			hProcessingThread_ = NULL;  // flag cleared — allow retry on next tick
	}
	else
	{
		DWORD error = ERROR_SUCCESS;
		const DWORD status = pollStopped(error);
		if (status == WAIT_FAILED) return NOT_OKAY;
		b_need_to_start_thread = (status == WAIT_OBJECT_0);
	}

	if (b_need_to_start_thread)
	{
        s_sndDevices_.ignoreVolumeCallbacks.store(TRUE);
        if (s_sndDevices_.deviceCallbacksInFlight.load() != 0) return OKAY;

		/* Initialize flag which can be set by the outside telling thread to end */
		i_kill_processing_thread_ = IS_FALSE;
        route_change_pending_ = false;
        const auto reinitGeneration = pending_output_generation_;

		/* Reinit the sndDevices module */
		const int reinit = sndDevicesReInit(hp_sndDevices_, SND_DEVICES_INIT_FOR_PROCESSING,
            &numRealDevices, &DfxDeviceEnabledFlag, &statusFlag);
        // The producer now reads immutable notification data only. Keep notices
        // accepted even after a failed/unavailable reinit, so recovery can wake us.
        s_sndDevices_.ignoreDeviceCallbacks.store(FALSE);
        if (reinit != OKAY)
        {
            OutputDebugStringW(L"FxSound: endpoint reinitialization failed\n");
            return NOT_OKAY_NO_BREAK;
        }
        refreshRestoreSnapshot();
        sndDeviceHandleToSoundDevices(false);
        const bool canStart = numRealDevices > 0 && DfxDeviceEnabledFlag == IS_TRUE;
        if (canStart && !publishUsedEndpoints())
        {
            signalCaptureStop();
            route_change_pending_ = true;
            onDeviceChange();
            return OKAY;
        }
        if (!canStart) s_sndDevices_.deviceChanges.forgetUsedEndpoints();
        if (s_sndDevices_.deviceChanges.hasPending() || s_device_change_pending_.load())
        {
            onDeviceChange();
            dispatchDeviceChange();
            if (route_change_pending_) return OKAY;
        }
        if (reinitGeneration != pending_output_generation_ || !pending_output_id_.empty()) return OKAY;

		if (statusFlag != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
		{
			/*
			if (cast_handle->trace.mode)
			{
				swprintf(cast_handle->wcp_msg1, 2048, L"dfxg_ProcessSndServerTimer(): sndDevicesReInit failed, statusFlag = %d", statusFlag);
				(cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
			}

			if (statusFlag == SND_DEVICES_NO_VALID_PLAYBACK_DEVICE)
			{
				if (dfxg_GetTranslatedString(hp_dfxg, IDS_DFX_MSG_ASK_EXIT_DFX_TO_HEAR_SOUNDS,
					cast_handle->wcp_translated_string) != OKAY)
					return(NOT_OKAY);
			}

			if (statusFlag == SND_DEVICES_ASK_USER_SELECT_PLAYBACK_DEVICE)
			{
				if (dfxg_GetTranslatedString(hp_dfxg, IDS_DFX_MSG_SELECT_AUDIO_PLAYBACK_FROM_MENU,
					cast_handle->wcp_translated_string) != OKAY)
					return(NOT_OKAY);
			}

			if (cast_handle->snd_server.b_no_valid_snd_device_dialog_shown == false)
			{
				cast_handle->snd_server.b_no_valid_snd_device_dialog_shown = true;
				if (dfxg_BringUpMsgModelessDlg(hp_dfxg, cast_handle->wcp_translated_string, IS_FALSE, NULL, IS_FALSE, 0) != OKAY)
					return(NOT_OKAY);
			}
			*/
		}
		else
		{
			b_no_valid_snd_device_dialog_shown_ = false;
		}

		/* PTNOTE - added check on DfxDeviceEnabledFlag status, may need to take additional steps if no DFX device is preset. */
		if ((numRealDevices > 0) && (DfxDeviceEnabledFlag == IS_TRUE))
		{
			hProcessingThread_ = CreateThread(NULL, 0, processingThread, (LPVOID)this, 0L, &ProcessingThreadID_);
		}
		else
		{
			// No usable playback path right now:
			//  - No real (physical) playback device is present (numRealDevices == 0), or
			//  - The DFX virtual device is not enabled/found, or
			//  - The playback device is unavailable (e.g. AUDCLNT_E_UNSUPPORTED_FORMAT /
			//    AUDCLNT_E_DEVICE_IN_USE, signaled by playbackDeviceIsUnavailable).
			// In every case there is nothing to be gained from re-running sndDevicesReInit
			// every 100ms, which would both burn CPU and leak COM objects on every retry
			// (each reinit re-enumerates all devices and re-activates the audio clients).
			// Use INVALID_HANDLE_VALUE as a sentinel to pause retries; onDeviceChange()
			// clears playbackDeviceIsUnavailable when the device situation changes, which
			// then allows a retry.
			if ((numRealDevices <= 0) || (DfxDeviceEnabledFlag != IS_TRUE))
				s_sndDevices_.playbackDeviceIsUnavailable = TRUE;
			hProcessingThread_ = INVALID_HANDLE_VALUE;
		}

		if (auto* callback = s_callback_.load(std::memory_order_acquire))
			callback->onSoundDeviceChange(true);

		/* Check if a new playback device has been selected */
		/*
		if (dfxg_SndServerHdmiFixCheckForNewPlaybackDevice(hp_dfxg) != OKAY)
			return(NOT_OKAY);
		*/
	}

	return(OKAY);
}

/*
* FUNCTION: threadWorker()
* DESCRIPTION:
*/
DWORD AudioPassthruPrivate::threadWorker(void)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    if (sndDevicesStartStopCapture(hp_sndDevices_, SND_DEVICES_START_CAPTURE) != OKAY)
        return NOT_OKAY_NO_BREAK;

    const DWORD result = captureAndPlay();
    // This worker owns the WASAPI stop calls, including capture/playback errors.
    const int stopResult = sndDevicesStartStopCapture(hp_sndDevices_, SND_DEVICES_STOP_CAPTURE);
    return stopResult == OKAY ? result : NOT_OKAY_NO_BREAK;
}

DWORD AudioPassthruPrivate::captureAndPlay()
{
    while (!i_kill_processing_thread_)
    {
        float* pcm = nullptr;
        int frames = 0;
        WAVEFORMATEX* format = nullptr;
        int status = SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS;
        if (sndDevicesDoCapture(hp_sndDevices_, &pcm, &frames, &format, &status) != OKAY)
            return NOT_OKAY_NO_BREAK;
        if (status != SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS || i_kill_processing_thread_)
            break;

        // A mute/unmute cycle invalidates playback of a block captured earlier.
        const auto muteState = mute_state_.load(std::memory_order_acquire);
        const bool muted = (muteState & mutedBit) != 0;
        if (frames > 0) applyDsp(pcm, frames, *format);
        if (i_kill_processing_thread_) break;

        // Playback may still have data queued even when capture returns no frames.
        if (!muted && mute_state_.load(std::memory_order_acquire) == muteState
            && sndDevicesDoPlayback(hp_sndDevices_, &status) != OKAY)
            return NOT_OKAY_NO_BREAK;
        if (status != SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS) break;
    }
    return OKAY;
}

void AudioPassthruPrivate::applyDsp(float* pcm, int frames, const WAVEFORMATEX& format)
{
    // The existing startup path retries format setup on subsequent audio blocks.
    p_dfx_dsp_->setSignalFormat(format.wBitsPerSample, format.nChannels, format.nSamplesPerSec,
                               format.wBitsPerSample);
    if (format.nChannels == 1 && SND_DEVICES_MONO_BUG_DO_NOT_PROCESS) return;

    auto* samples = reinterpret_cast<short int*>(pcm);
    p_dfx_dsp_->processAudio(samples, samples, frames, IS_FALSE);
}

/*
* FUNCTION: processingThread()
* DESCRIPTION:
*/
DWORD WINAPI AudioPassthruPrivate::processingThread(LPVOID lpParam)
{
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized))
    {
        OutputDebugStringW(L"FxSound: COM initialization failed on the audio thread\n");
        return NOT_OKAY_NO_BREAK;
    }

    auto* callerClass = static_cast<AudioPassthruPrivate*>(lpParam);
    DWORD ret = NOT_OKAY_NO_BREAK;
    try { ret = callerClass->threadWorker(); }
    catch (const std::exception& error)
    {
        OutputDebugStringA("FxSound: audio worker exception: ");
        OutputDebugStringA(error.what());
    }
    catch (...) { OutputDebugStringW(L"FxSound: unknown audio worker exception\n"); }

	CoUninitialize();

	return ret;
}

void AudioPassthruPrivate::mute(bool mute)
{
	const auto previous = mute_state_.load(std::memory_order_relaxed);
    const unsigned bit = mute ? mutedBit : 0;
    if ((previous & mutedBit) != bit)
        mute_state_.store((previous + 2) ^ mutedBit, std::memory_order_release);
}

void AudioPassthruPrivate::registerCallback(AudioPassthruCallback* callback)
{
	s_callback_.store(callback, std::memory_order_release);
}

bool AudioPassthruPrivate::isPlaybackDeviceAvailable()
{
    BOOL availability;

    sndDevicesGetPlaybackDeviceAvialblility(hp_sndDevices_, &availability);
    if (availability == TRUE)
        return true;
    else
        return false;
}

bool AudioPassthruPrivate::checkDeviceChanges()
{
    if (stopping_) return false;
    if (!refreshPendingCatalogue(true))
    {
        if (catalogue_dirty_) { onDeviceChange(); armDeviceRetry(); }
        return false;
    }
    if (auto* callback = s_callback_.load(std::memory_order_acquire)) callback->onSoundDeviceChange(false);
    return true;
}

void AudioPassthruPrivate::restoreDefaultPlaybackDevice()
{
	// Power-off supersedes a queued automatic restore, even if the previous
	// worker has not stopped yet or a reentrant control call is still returning.
	pending_output_id_.clear();
	++pending_output_generation_;
	route_change_pending_ = false;
	int i_resultFlag;
	/* Change the default soundcard to not be the DFX virtual one but instead the proper real one */
	if (sndDevicesRestoreDefaultDevice(hp_sndDevices_, &i_resultFlag) != OKAY)
		return;
}

int AudioPassthruPrivate::setTargetedRealPlaybackDevice(const std::wstring sound_device_guid)
{
    if (stopping_ || sound_device_guid.empty()) return NOT_OKAY_NO_BREAK;
    if (pending_output_id_.empty() && !route_change_pending_ && sound_device_guid == route_output_id_
        && hProcessingThread_ != nullptr && hProcessingThread_ != INVALID_HANDLE_VALUE) return OKAY;
    // Always replace an older queued target, including a manual re-selection
    // of the currently playing fallback while a restore is waiting for stop.
    pending_output_id_ = sound_device_guid;
    ++pending_output_generation_;
    route_change_pending_ = true;
    signalCaptureStop();
    onDeviceChange();
    return OKAY;
}
