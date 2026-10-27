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
#include "AudioRoutingPolicy.h"
#include "ptime.h"

// Choose routing independently of Windows defaults. Custom inputs never fall back to another source.
int PT_DECLSPEC sndDevicesImplementDeviceRules(PT_HANDLE *hp_sndDevices, int *ip_resultFlag)
{
    auto* state = (sndDevicesHdlType*)hp_sndDevices;
    if (state == NULL)
        return NOT_OKAY;
    *ip_resultFlag = SND_DEVICES_RULES_NOT_POSSIBLE;

    AudioRouting::Preferences preferences;
    const auto read = [&](wchar_t* key, std::wstring& destination) {
        wchar_t value[PT_MAX_GENERIC_STRLEN] = {};
        if (sndDeviceReadFromRegistry(hp_sndDevices, REG_CURRENT_USER, key, value) != OKAY)
            return false;
        destination = value;
        return true;
    };
    if (!read(SND_DEVICES_REGISTRY_USER_SELECTED_INPUT_WIDE, preferences.input) ||
        !read(SND_DEVICES_REGISTRY_USER_SELECTED_PLAYBACK_WIDE, preferences.pinnedOutput) ||
        !read(SND_DEVICES_REGISTRY_MOST_RECENT_PLAYBACK_WIDE, preferences.recentPlayback) ||
        !read(SND_DEVICES_REGISTRY_MOST_RECENT_DEFAULT_WIDE, preferences.recentDefault) ||
        !read(SND_DEVICES_REGISTRY_PRIOR_DEFAULT_WIDE, preferences.priorDefault) ||
        !read(SND_DEVICES_REGISTRY_ORIGINAL_DEFAULT_WIDE, preferences.originalDefault))
        return NOT_OKAY;
    int automaticMode;
    if (sndDevicesGetDefaultDeviceSelectionMode(hp_sndDevices, &automaticMode) != OKAY)
        return NOT_OKAY;
    preferences.requestedOutput = state->requestedPlaybackDeviceID;
    if (state->defaultDeviceNum >= 0)
        preferences.systemDefault = state->pwszID[state->defaultDeviceNum];

    if ((automaticMode == SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_OFF || !preferences.input.empty()) &&
        !preferences.recentPlayback.empty())
        preferences.systemDefault.clear();

    std::vector<AudioRouting::Endpoint> endpoints;
    for (int i = 0; i < state->totalNumDevices; ++i)
    {
        AudioRouting::Endpoint endpoint;
        endpoint.id = state->pwszID[i];
        endpoint.recording = state->deviceFlow[i] == eCapture;
        endpoint.virtualDevice = i == state->dfxDeviceNum;
        endpoint.active = state->deviceState[i] == DEVICE_STATE_ACTIVE;
        endpoint.channels = state->deviceNumChannel[i];
        endpoints.push_back(endpoint);
    }
    if (state->numPreviousRealDevices > 0 && state->numRealDevices > state->numPreviousRealDevices)
        for (int i = 0; i < state->numRealDevices; ++i)
        {
            bool previouslyPresent = false;
            for (int j = 0; j < state->numPreviousRealDevices; ++j)
                previouslyPresent |= wcscmp(state->pwszIDRealDevices[i], state->pwszIDPreviousRealDevices[j]) == 0;
            if (!previouslyPresent)
            {
                preferences.newOutput = state->pwszIDRealDevices[i];
                break;
            }
        }
    const auto route = AudioRouting::select(endpoints, preferences);
    if (route.error != AudioRouting::Error::none)
    {
        *ip_resultFlag = route.error == AudioRouting::Error::inputUnavailable ? SND_DEVICES_INPUT_NOT_AVAILABLE :
            route.error == AudioRouting::Error::feedback ? SND_DEVICES_INPUT_OUTPUT_CONFLICT : SND_DEVICES_NO_VALID_PLAYBACK_DEVICE;
        return OKAY;
    }
    state->captureDeviceNum = route.input;
    state->playbackDeviceNum = route.output;
    state->pCaptureDevice = state->pAllDevices[route.input];
    state->pPlaybackDevice = state->pAllDevices[route.output];

    // Save the original physical default once, before FxSound claims it.
    if (preferences.originalDefault.empty() && state->defaultDeviceNum >= 0 &&
        state->defaultDeviceNum != state->dfxDeviceNum)
        if (sndDevicesWriteToRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_ORIGINAL_DEFAULT_WIDE,
            state->pwszID[state->defaultDeviceNum]) != OKAY)
            return NOT_OKAY;

    WAVEFORMATEX playbackFormat = {};
    int result;
    if (sndDevicesGetFormatFromID(hp_sndDevices, state->pwszID[route.output], &playbackFormat, &result) != OKAY)
        return NOT_OKAY;
    if (result != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
    {
        *ip_resultFlag = result;
        return OKAY;
    }
    const DWORD captureRate = playbackFormat.nSamplesPerSec % 48000 == 0 ? 48000 : 44100;
    if (playbackFormat.nSamplesPerSec == 0 || playbackFormat.nSamplesPerSec % captureRate != 0 ||
        playbackFormat.nSamplesPerSec / captureRate > 16)
    {
        *ip_resultFlag = SND_DEVICES_SAMP_FREQ_NOT_VALID;
        return OKAY;
    }
    state->wfxDfxProcessing = playbackFormat;
    state->wfxDfxProcessing.nSamplesPerSec = captureRate;
    state->wfxDfxProcessing.wBitsPerSample = 32;
    state->wfxDfxProcessing.nBlockAlign = playbackFormat.nChannels * sizeof(float);
    state->wfxDfxProcessing.nAvgBytesPerSec = captureRate * state->wfxDfxProcessing.nBlockAlign;
    state->upsampleRatio = playbackFormat.nSamplesPerSec / captureRate;

    if (route.input == state->dfxDeviceNum)
    {
        const int channels = playbackFormat.nChannels == 4 ? 6 : playbackFormat.nChannels;
        if (sndDevicesSetDfxDeviceSampleRateAndChannels(hp_sndDevices,
            captureRate == 48000 ? SND_DEVICES_DFX_SAMP_FREQ_48 : SND_DEVICES_DFX_SAMP_FREQ_44_1, channels, &result) != OKAY)
            return NOT_OKAY;
        if (result != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
        {
            *ip_resultFlag = result;
            return OKAY;
        }
        int mode;
        if (sndDevicesGetDefaultDeviceSelectionMode(hp_sndDevices, &mode) != OKAY)
            return NOT_OKAY;
        if (mode == SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_ON)
        {
            if (sndDevicesSetDeviceType(hp_sndDevices, SND_DEVICES_DEFAULT, state->pwszID[route.input], &result) != OKAY)
                return NOT_OKAY;
            if (result != SND_DEVICES_DEVICE_OPERATION_COMPLETED)
            {
                *ip_resultFlag = result;
                return OKAY;
            }
        }
    }
    // Only the FxSound virtual endpoint may be reconfigured or made the Windows default.
    if (sndDevicesWriteToRegistry(hp_sndDevices, REG_CURRENT_USER, SND_DEVICES_REGISTRY_MOST_RECENT_PLAYBACK_WIDE,
        state->pwszID[route.output]) != OKAY)
        return NOT_OKAY;
    state->requestedPlaybackDeviceID[0] = L'\0';
    *ip_resultFlag = SND_DEVICES_DEVICE_OPERATION_COMPLETED;
    return OKAY;
}
