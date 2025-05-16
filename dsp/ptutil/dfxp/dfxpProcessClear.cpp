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

/* dfxpProcessClear.cpp */

#include "codedefs.h"

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <share.h>

#include "u_dfxp.h" /* Must go before codedefs.h due to mmgr */

#include "dfxp.h"
#include "mth.h"
#include "realSample.h"
#include "spectrum.h"
#include "com.h"
#include "DfxSdk.h"
#include "file.h"

extern "C" {
#include "comSftwr.h"
}

/*
 * FUNCTION: dfxp_ClearBuffersIfSongStart() 
 * DESCRIPTION:
 *
 *  If the song is starting we need to clear the buffers.  It would be best if
 *  we could know the user hit "play" and then we would know to clear the buffers however since
 *  we don't know when the user hit "play" we must assume it is the start of a new song if no
 *  buffer has been processed for a certain period of time (like one second).
 *
 *  The data that is to be cleared is the slip buffer data and the reverberations.  Also the
 *  BeatDsp buffers are cleared.
 *
 */
int dfxp_ClearBuffersIfSongStart(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int i_msecs_since_last_buffer_processed;


	/* HACK */
	i_msecs_since_last_buffer_processed = 0;

/*
	if (dfxp_CalcMsecsSinceLastBufferProcessed(hp_dfxp, &i_msecs_since_last_buffer_processed) != OKAY)
		return(NOT_OKAY);
*/

	/* If it is the start of a new song, clear the previous buffered audio data */
	if (i_msecs_since_last_buffer_processed >= DFXP_MSECS_SINCE_LAST_BUFFER_TO_ASSUME_STOPPED)
   {
		/* Clear previously buffered audio */
		if (dfxpClearPreviousBufferedAudio(hp_dfxp) != OKAY)
			return(NOT_OKAY);

		/* Clear the spectrum */
		if (cast_handle->spectrum.spectrum_hdl != NULL)
		{
			if (spectrumReset(cast_handle->spectrum.spectrum_hdl) != OKAY)
				return(NOT_OKAY);
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: dfxpClearPreviousBufferedAudio() 
 * DESCRIPTION:
 *   Clears all the data associated with previous audio data which has been processed and buffered.
 *   This function is usually called at the start of a new song so that tails of the previous song
 *   are not heard at the beginning of the next song.
 */
int dfxpClearPreviousBufferedAudio(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* 
	 * Clear the DFX buffered data (reverb tails) 
	 * NOTE: For now we are calling BeginProcess() but we should call a simpler com function
	 *       to clear the buffers.
	 */
	if (dfxpBeginProcess(hp_dfxp, cast_handle->bits_per_sample, cast_handle->num_channels_out, (int)cast_handle->sampling_freq) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}