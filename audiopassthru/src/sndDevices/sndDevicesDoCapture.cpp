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
#include <Avrt.h>

#include "slout.h"
#include "mry.h"
#include "u_sndDevices.h"
#include "sndDevices.h"

/*
 * FUNCTION: sndDevicesDoCapture()
 * DESCRIPTION: Captures audio buffers from specified capture device.
 * Returns fp_buffer to use to do audio processing in place.
 * *ip_numSampleSets will contain the number of sample sets in the buffer
 * and **pp_wfxDfx contains the format of the buffer.
 */
int PT_DECLSPEC sndDevicesDoCapture(PT_HANDLE *hp_sndDevices, float **fpp_buffer, int *ip_numSampleSets, WAVEFORMATEX **pp_wfxDfx, int *ip_resultFlag)
{
	struct sndDevicesHdlType *cast_handle;
	HRESULT hr;
	UINT32 packetLength = 0;
	UINT32 numFramesQueuedUpToPlay;
	UINT32 numFramesQueuedUpToCapture;
	UINT32 numFramesQueuedUpToPlayReferencedToCapture; // Corrected for samp rate difference.
	DWORD flags;
	float *fptr;
	UINT32 loopsize, index, offset;
	UINT32 i;
	int numCaptureChannels;
	int numPlaybackChannels;
	UINT32 numDesiredCaptureFrames;
	UINT32 halfCaptureBufferSize;
	UINT32 quarterCaptureBufferSize;
	int i_playback_index;
#ifdef _DEBUG
	UINT64 DevicePosition;
	UINT64 QPCPosition;
#endif

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if(cast_handle->pAudioClientCapture == NULL) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_CAPTURE_CLIENT)
	if(cast_handle->pAudioClientPlayback == NULL) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_PLAYBACK_CLIENT)
	if(cast_handle->pAudioCaptureLoopback == NULL) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_LOOPBACK_CLIENT)

	cast_handle->capturedFramesCount = 0; // Probably want to maintain and not zero this value.

	numCaptureChannels = cast_handle->wfxCapture.nChannels;
	numPlaybackChannels = cast_handle->wfxPlayback.nChannels;
	*fpp_buffer = NULL;
	*ip_numSampleSets = 0;
	*pp_wfxDfx = NULL;

	// Repeat this loop until we have enough frames to fill the specified playback buffer space.
	do
	{
		// If the device callbacks or an external call has thrown the stop flag, exit this thread.
		if( cast_handle->stopAudioCaptureAndPlaybackLoop == 1 )
		{
			*ip_numSampleSets = 0;
			*ip_resultFlag = SND_DEVICES_CAPTURE_FORCED_EXIT;
			return(OKAY);
		}

		// With the looping now used to build playback buffer we can sleep for very short time.
		Sleep(1);

		// Check for bad value to upsample ratio, was happening when some Bluetooth devices were added or removed (11/25/15).
		if( (cast_handle->upsampleRatio <= 0) || (cast_handle->upsampleRatio > 16) )
		{/*
			*ip_numSampleSets = 0;
			*ip_resultFlag = SND_DEVICES_CAPTURE_ERROR;
			return(OKAY); */

			SLOUT_FIRST_LINE(L"sndDevicesDoCapture():: upsampleRatio is out of range");
			cast_handle->upsampleRatio = 1;

		}

		hr = cast_handle->pAudioClientCapture->GetCurrentPadding(&numFramesQueuedUpToCapture);
		if (FAILED(hr)) goto Exit;

		// This call returns the number of frames still awaiting playback in the playback buffer
		hr = cast_handle->pAudioClientPlayback->GetCurrentPadding(&numFramesQueuedUpToPlay);
		if (FAILED(hr)) goto Exit;

		numFramesQueuedUpToPlayReferencedToCapture = numFramesQueuedUpToPlay/cast_handle->upsampleRatio;

		// Calculate the number of playback frames to fill, compensated for samp rate differences.
		cast_handle->numPlaybackFramesAvailableToFill = cast_handle->bufferFrameSizeCapture - numFramesQueuedUpToPlayReferencedToCapture;

		halfCaptureBufferSize = cast_handle->bufferFrameSizeCapture/2;
		quarterCaptureBufferSize = cast_handle->bufferFrameSizeCapture/4;

		// If playback buffer is totally empty, don't send more buffers to playback until we fill 1/2 the capture buffer.
		if( numFramesQueuedUpToPlayReferencedToCapture == 0 )
		{
			numDesiredCaptureFrames = halfCaptureBufferSize;
			cast_handle->playbackIsActive = SND_DEVICES_PLAYBACK_IS_STOPPED;
			
			if( cast_handle->playbackStreamIsTemporarilyPaused == 0 )
			{
				cast_handle->playbackStreamIsTemporarilyPaused = 1;
				hr = cast_handle->pAudioClientPlayback->Stop(); // Stop playback to allow PC to sleep if no audio is playing.
				if (FAILED(hr)) goto Exit;
			}
		}
		else
		{
			cast_handle->playbackIsActive = SND_DEVICES_PLAYBACK_IS_ACTIVE;

			if( cast_handle->playbackStreamIsTemporarilyPaused )
			{
				cast_handle->playbackStreamIsTemporarilyPaused = 0;
				hr = cast_handle->pAudioClientPlayback->Start(); // Restart playback.
			}

			// If the playback buffer is already at least 1/2 full, don't grab anymore capture buffers.
			if( cast_handle->numPlaybackFramesAvailableToFill < halfCaptureBufferSize )
				numDesiredCaptureFrames = 0;
			else
				// If the playback buffer is less than 1/2 full, get what we need to make 1/2 full.
				numDesiredCaptureFrames = cast_handle->numPlaybackFramesAvailableToFill - halfCaptureBufferSize;
		}

		if( numDesiredCaptureFrames == 0 )
			goto DoneGrabbingFrames;

		// The audio is streamed in small "packets", a number of frames (sample sets). So far it appears that
		// packets are always 10ms in duration, independent of sampling freq, so at 44.1k we get 441 sample sets/packet.
		hr = cast_handle->pAudioCaptureLoopback->GetNextPacketSize(&packetLength); // The number of frames in the next packet.
		if (FAILED(hr)) goto Exit;

		do // Loop to read the small data packets to fill capture buffer up to available playback size.
		{
			if( packetLength > 0 )
			{
				// Get the data in this packet.
				hr = cast_handle->pAudioCaptureLoopback->GetBuffer(&(cast_handle->pDataPacketCapture), &(cast_handle->numCaptureFramesAvailable), &flags,
#ifdef _DEBUG
					&DevicePosition, &QPCPosition);
#else
					NULL, NULL);
#endif
				if (FAILED(hr)) goto Exit;

				fptr = (float *)(cast_handle->pDataPacketCapture);

				loopsize = cast_handle->numCaptureFramesAvailable * numCaptureChannels;
				offset = cast_handle->capturedFramesCount * numCaptureChannels;

				// Silent flag means to treat packet as containing all zeros, even though it may not.
				if( flags & AUDCLNT_BUFFERFLAGS_SILENT )
				{
					cast_handle->playbackIsActive = SND_DEVICES_PLAYBACK_IS_STOPPED;
					for(i=0; i<loopsize; i++)
						cast_handle->fCaptureBuf[ offset + i ] = (float)0.0;
				}
				else
				{
					for(i=0; i<loopsize; i++)
						cast_handle->fCaptureBuf[ offset + i ] = fptr[i];
				}

// #define DO_BEEPS_ON_SONG_CHANGES
#ifdef DO_BEEPS_ON_SONG_CHANGES
				// The discontinuity flag gets set by WMP when clicking on different song selections but not on start/stop.
				// In youtube the flag doesn't get set by start/stop or playing songs but does get set when capture
				// buffer overflows, as does WMP.
				static int discontinuity_count = 0;
				static int silent_count = 0;
				static int timestamp_error_count = 0;
				int event_flag;

				event_flag = 1;

				for(i=0; i<loopsize; i++ )
					if( fabs(cast_handle->fCaptureBuf[ offset + i ]) > 0.001 )
						event_flag = 0;

				if( flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY )
				{
 					discontinuity_count++;
					event_flag = 1;
				}
				if( flags & AUDCLNT_BUFFERFLAGS_SILENT )
				{
					silent_count++;
					event_flag = 1;
				}
				if( flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR )
				{
					timestamp_error_count++;
					event_flag = 1;
				}

				if( event_flag )
					for(i=0; i<loopsize; i++)
						if( (i % 100) == 0 )
							cast_handle->fCaptureBuf[ offset + i ] = (float)0.95;
						else
							cast_handle->fCaptureBuf[ offset + i ] = (float)0.0;
#endif

				// This release call is to be called as soon as possible following the GetBuffer call.
				hr = cast_handle->pAudioCaptureLoopback->ReleaseBuffer(cast_handle->numCaptureFramesAvailable);
				if (FAILED(hr)) goto Exit;

				cast_handle->capturedFramesCount += cast_handle->numCaptureFramesAvailable;

				hr = cast_handle->pAudioCaptureLoopback->GetNextPacketSize(&packetLength);
				if (FAILED(hr)) goto Exit;
			}

			// The audio is streamed in small "packets", a number of frames (sample sets). So far it appears that
			// packets are always 10ms in duration, independent of sampling freq, so at 44.1k we get 441 sample sets/packet.
			hr = cast_handle->pAudioCaptureLoopback->GetNextPacketSize(&packetLength); // The number of frames in the next packet.
			if (FAILED(hr)) goto Exit;

		} while( (cast_handle->capturedFramesCount < numDesiredCaptureFrames ) // No sleep in this loop.
			      && (packetLength > 0) );

		// Check to see if we are in the case where no more capture frames are coming in and we need to playout the rest of the playback buffer.
		hr = cast_handle->pAudioClientPlayback->GetCurrentPadding(&numFramesQueuedUpToPlay);
		if (FAILED(hr)) goto Exit;

		// If we are not in startup mode (numFramesQueuedUpToPlay !=  0) and  playback buffer has shrunk to 1/4 or less desired size
		// and no more capture buffers are coming in, transfer remaining capture buffers to playback.
		if( (numFramesQueuedUpToPlay != 0) && (cast_handle->capturedFramesCount > 0) && (numFramesQueuedUpToPlay/cast_handle->upsampleRatio) <= quarterCaptureBufferSize )
			goto DoneGrabbingFrames;

	} while( cast_handle->capturedFramesCount < numDesiredCaptureFrames ); // This loop has a sleep to allow capture buffers to build up.

DoneGrabbingFrames :

	// Correct number of playback frame for sample rate difference.
	cast_handle->playbackFrameCount = cast_handle->upsampleRatio * cast_handle->capturedFramesCount;

	// We should now have enough audio data to fill playback buffer
	if( cast_handle->capturedFramesCount > 0 )
	{
		loopsize = cast_handle->capturedFramesCount * numCaptureChannels;

		// If channels don't match, correctly fill playback buffer.
		//Ordering for 5.1 is: Front Left, Front Right, Front Center, Low Frequency, Back Left, Back Right
		//Ordering for 7.1 is: Front Left, Front Right, Front Center, Low Frequency, Back Left, Back Right, Side Left, Side Right
		// With current implementation, the only case we hit here should be 6 channel capture, 8 channel playback.
		// Note, with added 5.1 and 7.1 support we shouldn't ever hit this case.
		if( numCaptureChannels != numPlaybackChannels )
		{
			if( numCaptureChannels == 6 )
			{
				// For now we will always assume capture device has 6 channels.
				index = 0;
				if( numPlaybackChannels == 2 ) // Stereo
				{
					for(i=0; i<loopsize; i += numCaptureChannels) // Index through frames (sample sets)
					{
							cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i];
							index++;
							cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 1];
							index++;
					}
				}
				else if( numPlaybackChannels == 4 )	// Quad
				{
					for(i=0; i<loopsize; i += numCaptureChannels) // Index through frames (sample sets)
					{
						cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i];
						index++;
						cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 1];
						index++;
						cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 4];
						index++;
						cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 5];
						index++;
					}
				}
				else if( numPlaybackChannels == 8 )	// 7.1 (eight channel)
				{
					for(i=0; i<loopsize; i += numCaptureChannels) // Index through frames (sample sets)
					{
						cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i];
						index++;
						cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 1];
						index++;
						cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 2];
						index++;
						cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 3];
						index++;
						cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 4];
						index++;
						cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 5];
						index++;
						cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 4]; // Fill side channels with back channels
						index++;
						cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 5];
						index++;
					}
				}
			} // End of numCaptureChannels == 6
			else if( (numPlaybackChannels == 4) && (numCaptureChannels == 2) )
			{
				index = 0;
				for(i=0; i<loopsize; i += numCaptureChannels) // Index through frames (sample sets)
				{
					cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i];
					index++;
					cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 1];
					index++;
					cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i];
					index++;
					cast_handle->fPlaybackBuf[index] = cast_handle->fCaptureBuf[i + 1];
					index++;
				}
			}
			// Since we are not able to playback to mono devices correctly (bluetooth problem) we simply fill the playback buffer with zeros
			else if (numPlaybackChannels == 1)
			{
				if (SND_DEVICES_MONO_BUG_FORCE_SILENCE)
				{
					for (i_playback_index = 0; i_playback_index < cast_handle->playbackBufAllocSize; i_playback_index++)
					{
						cast_handle->fPlaybackBuf[i_playback_index] = (realtype)0.0;
					}
				} else
				{
					// TODO: Handle mono playback correctly. The code below doesn't work well.
					if (numCaptureChannels == 2)
					{
						index = 0;
						for (i = 0; i<loopsize; i += numCaptureChannels) // Index through frames (sample sets)
						{
							cast_handle->fPlaybackBuf[index] = (cast_handle->fCaptureBuf[i] + cast_handle->fCaptureBuf[i + 1]) / (realtype)2.0;
							index++;
						}
					}
				}
			}
		}
		else	// Channel sizes match, just do a straight copy.
		{
			for(i=0; i<loopsize; i++)
				cast_handle->fPlaybackBuf[i] = cast_handle->fCaptureBuf[i];
		}
	}

	// We now exit the capture loop with the playback buffers ready for processing
	// They are at 44.1 or 48khz but at the playback devices channel format.
	*fpp_buffer = cast_handle->fPlaybackBuf;

	*ip_numSampleSets = cast_handle->capturedFramesCount;

	*pp_wfxDfx = &(cast_handle->wfxDfxProcessing);

Exit:
	if(hr == S_OK)
		*ip_resultFlag = SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS;
	else
		*ip_resultFlag = SND_DEVICES_CAPTURE_ERROR;

	return(OKAY);
}

/*
 * FUNCTION: sndDevicesStartStopCapture()
 * DESCRIPTION: Allows starting or stopping audio capture. Note that after capture stops, existing buffers
 * that remain queued up for the playback buffer will playout and then playback will stop also.
 */
int PT_DECLSPEC sndDevicesStartStopCapture(PT_HANDLE *hp_sndDevices, int i_startStopFlag)
{
	struct sndDevicesHdlType *cast_handle;
	HRESULT hr;

	cast_handle = (struct sndDevicesHdlType *)hp_sndDevices;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if(cast_handle->pAudioClientCapture == NULL) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_CAPTURE_CLIENT)
	if(cast_handle->pAudioClientPlayback == NULL) SND_DEVICES_SET_STATUS_AND_RETURN_OK(SND_DEVICES_NULL_PLAYBACK_CLIENT)

	if( i_startStopFlag == SND_DEVICES_START_CAPTURE )
	{
		cast_handle->stopAudioCaptureAndPlaybackLoop = 0;
		hr = cast_handle->pAudioClientCapture->Start(); // Start capturing, this starts filling of the internal capture buffers.
		hr = cast_handle->pAudioClientPlayback->Start(); // Start playback, this starts filling of the internal playback buffers.
	}
	else
	{
		hr = cast_handle->pAudioClientCapture->Stop();  // Stop capturing.
		hr = cast_handle->pAudioClientPlayback->Stop(); // Stop playback.
		cast_handle->stopAudioCaptureAndPlaybackLoop = 1;
	}

	return(OKAY);
}
