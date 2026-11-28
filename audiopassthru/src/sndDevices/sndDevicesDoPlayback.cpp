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

#include "codedefs.h"
#include "slout.h"
#include "mry.h"
#include "u_sndDevices.h"
#include "sndDevices.h"
#include "PlaybackReserve.h"

/*
 * FUNCTION: sndDevicesDoPlayback()
 * DESCRIPTION: Plays back audio buffers to specified playback device.
 * Plays buffer frames that have been queued up by the capture device.
 */
int PT_DECLSPEC sndDevicesDoPlayback(PT_HANDLE *hp_sndDevices, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	HRESULT hr;
	UINT32 numFramesQueuedUpToPlay;
	UINT32 i, j, index, loopsize;

	int numPlaybackChannels;
	int k;

	*ip_resultFlag = SND_DEVICES_PLAYBACK_ERROR;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if( cast_handle->pAudioClientPlayback == NULL )
	{
		*ip_resultFlag = SND_DEVICES_NULL_PLAYBACK_CLIENT;
		SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_PLAYBACK_CLIENT)
	}

	numPlaybackChannels = cast_handle->wfxPlayback.nChannels;

	// Do upsampling if needed.
	if( cast_handle->upsampleRatio > 1 )
	{
		// Copy channel corrected playback buffers back to capture buffer so we can then re-sample.
		loopsize = cast_handle->capturedFramesCount * numPlaybackChannels;
		for(i=0; i<loopsize; i++)
			cast_handle->fCaptureBuf[i] = cast_handle->fPlaybackBuf[i];

		// Fill playback buff with upsampling
		index = 0;
		for(i=0; i<cast_handle->capturedFramesCount; i++) // Index through frames (sample sets)
		{
			for(j=0; j<cast_handle->upsampleRatio; j++)	 // Repeat frame fills for upsampling
			{
				for(k=0; k<numPlaybackChannels; k++)
				{
					cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i * numPlaybackChannels + k];
					index++;
				}
			}
		}
	}

    const UINT32 total = cast_handle->playbackFrameCount;
    const UINT32 rate = cast_handle->wfxPlayback.nSamplesPerSec;
    const UINT32 capacity = cast_handle->bufferFrameSizePlayback;
    auto* client = cast_handle->pAudioClientPlayback;
    auto* renderer = cast_handle->pAudioClientPlaybackRender;
    if (!capacity || !rate || !renderer || !cast_handle->fPlaybackBuf) return NOT_OKAY;
    if (!total) { *ip_resultFlag = SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS; return OKAY; }
    auto& controls = fxsoundtiming::playbackReserveControl();
    const unsigned requestedMs = controls.targetMs.load(std::memory_order_relaxed);
    const unsigned target = fxsoundtiming::targetFrames(rate, capacity, total, requestedMs);
    // One worker/thread owns this state and starts a fresh physical stream after reinitialization.
    thread_local fxsoundtiming::ReserveTransition slew;
    fxsoundtiming::PacketPlan plan{total, false};
    bool planned = false;
    UINT32 sent = 0;
    ULONGLONG progress = GetTickCount64();
    const auto stopRequested = [&]()
    {
        if (!cast_handle->stopAudioCaptureAndPlaybackLoop) return false;
        *ip_resultFlag = SND_DEVICES_CAPTURE_FORCED_EXIT;
        return true;
    };
    while (sent < plan.frames)
    {
        if (stopRequested()) return OKAY;
        hr = client->GetCurrentPadding(&numFramesQueuedUpToPlay);
        if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_GET_PADDING_FAILED)
        UINT32 prime = 0;
        const bool resumeEmpty = !numFramesQueuedUpToPlay && !planned &&
            (cast_handle->playbackStreamIsTemporarilyPaused ||
             (target && slew.ready && slew.requested == requestedMs && !slew.remaining));
        if (resumeEmpty)
        {
            if (!cast_handle->playbackStreamIsTemporarilyPaused)
            {
                hr = client->Stop();
                if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_PLAYBACK_ERROR)
                cast_handle->playbackStreamIsTemporarilyPaused = 1;
            }
            prime = target;
            slew.prime(requestedMs, target);
            if (prime)
            {
                if (stopRequested()) return OKAY;
                BYTE* silence = nullptr;
                hr = renderer->GetBuffer(prime, &silence);
                if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_PLAYBACK_RENDER_FAILED)
                hr = renderer->ReleaseBuffer(prime, AUDCLNT_BUFFERFLAGS_SILENT);
                if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_RELEASE_BUFFER_FAILED)
                hr = client->GetCurrentPadding(&numFramesQueuedUpToPlay);
                if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_GET_PADDING_FAILED)
            }
        }
        // Stop may arrive while COM primes the queue. Do not write new audio
        // after the control thread has requested it to stop.
        if (stopRequested()) return OKAY;
        if (!planned)
        {
            plan = slew.plan(total, requestedMs, target, numFramesQueuedUpToPlay);
            planned = true;
        }
        // Lowering the high-water mark during a transition would move the delay upstream.
        const unsigned maxQueue = plan.transition ? capacity : target + (std::min)(capacity, total);
        const unsigned space = numFramesQueuedUpToPlay < maxQueue ? maxQueue - numFramesQueuedUpToPlay : 0;
        const UINT32 amount = (std::min)(space, plan.frames - sent);
        if (!amount)
        {
            if (GetTickCount64() - progress > 2000)
                SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_PLAYBACK_ERROR)
            Sleep(1);
            continue;
        }
        BYTE* output = nullptr;
        hr = renderer->GetBuffer(amount, &output);
        if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_PLAYBACK_RENDER_FAILED)
        fxsoundtiming::writeFrames(reinterpret_cast<float*>(output), cast_handle->fPlaybackBuf, total,
                               plan.frames, sent, amount, numPlaybackChannels);
        hr = renderer->ReleaseBuffer(amount, 0);
        if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_RELEASE_BUFFER_FAILED)
        // In particular, never restart a client stopped during ReleaseBuffer.
        if (stopRequested()) return OKAY;
        const bool starting = cast_handle->playbackStreamIsTemporarilyPaused != 0;
        if (starting)
        {
            hr = client->Start();
            if (FAILED(hr)) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_PLAYBACK_ERROR)
            // A control-thread Stop can finish while Start is in progress.
            // Check again and compensate rather than leave the client running.
            if (stopRequested())
            {
                hr = client->Stop();
                if (FAILED(hr))
                {
                    wchar_t message[128];
                    swprintf_s(message, L"FxSound: Stop after interrupted render Start failed (0x%08lx)\n",
                        static_cast<unsigned long>(hr));
                    OutputDebugStringW(message);
                    *ip_resultFlag = SND_DEVICES_PLAYBACK_ERROR;
                    SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_PLAYBACK_ERROR)
                }
                return OKAY;
            }
            cast_handle->playbackStreamIsTemporarilyPaused = 0;
        }
        controls.observedMs.store(static_cast<int>((numFramesQueuedUpToPlay * 1000ULL + rate / 2) / rate), std::memory_order_relaxed);
        controls.appliedTargetMs.store(static_cast<int>((target * 1000ULL + rate / 2) / rate), std::memory_order_relaxed);
        controls.transitioning.store(plan.transition, std::memory_order_relaxed);
        controls.lastWriteTick.store(GetTickCount64(), std::memory_order_release);

        sent += amount;
        progress = GetTickCount64();
    }

	*ip_resultFlag = SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS;

	return(OKAY);
}
