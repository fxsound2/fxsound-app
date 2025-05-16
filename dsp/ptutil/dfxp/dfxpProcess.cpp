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

/* dfxpProcess.cpp */

#include "codedefs.h"

#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "u_dfxp.h" 
#include "dfxp.h"
#include "mth.h"
#include "realSample.h"
#include "spectrum.h"
#include "com.h"
#include "DfxSdk.h"

extern "C" {
#include "comSftwr.h"
}

/*
 * FUNCTION: dfxpBeginProcess() 
 * DESCRIPTION:
 *   Begin the processing of a signal.  This function is called
 *   to inform the processing module that either the samp_freq, 
 *   bit_per_sample, or num_channels has changed.
 */
int dfxpBeginProcess(PT_HANDLE *hp_dfxp, int i_bits_per_sample, int i_num_channels, int i_sample_rate)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);
 
	realtype r_sample_rate;
	int sampling_rate_changed;
	int bits_per_sample_changed;
	int num_channels_changed;
   
	if (!(cast_handle->fully_initialized))
		return(NOT_OKAY);

	if (cast_handle->trace.mode)
	{
		swprintf(cast_handle->wcp_msg1, L"dfxpBeginProcess: Entered: bps = %d, nch = %d, samp_rate = %d",
			     i_bits_per_sample, i_num_channels, i_sample_rate);
	   (cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
   }

	/* Convert the sampling frequency to a real */
	r_sample_rate = (realtype)i_sample_rate;

	/* Check if the sampling rate has changed */
	if (r_sample_rate != cast_handle->sampling_freq)
	   sampling_rate_changed = IS_TRUE;
	else
	   sampling_rate_changed = IS_FALSE;

	/* Check if num channels changed */
	if (i_num_channels != cast_handle->num_channels_in)
		num_channels_changed = IS_TRUE;
	else
		num_channels_changed = IS_FALSE;

	/* Check if bits per sample changed */
	if (i_bits_per_sample != cast_handle->bits_per_sample)
      bits_per_sample_changed = IS_TRUE;
	else
      bits_per_sample_changed = IS_FALSE;

	/* Reset the stored signal info */
	cast_handle->bits_per_sample = i_bits_per_sample;
	cast_handle->valid_bits = i_bits_per_sample;
   cast_handle->num_channels_in = i_num_channels;
   cast_handle->num_channels_out = i_num_channels;
	cast_handle->sampling_freq = r_sample_rate; //Actual sampling rate of the data
	cast_handle->internal_sampling_freq = r_sample_rate; //Sampling rate used internally for processing
	cast_handle->sampling_period = (realtype)1.0/(cast_handle->sampling_freq);
	cast_handle->internal_sampling_period = cast_handle->sampling_period;
	cast_handle->internal_rate_ratio = 1;

	// If the actual signal sampling rate is greater than the max internal rate, the
	// internal rates will be reduced.
	if( r_sample_rate > (realtype)DFXP_MAX_INTERNAL_SAMPLING_FREQ )
	{
		if(r_sample_rate < (realtype)DFXP_MAX_SAMPLING_FREQ)
		{
			cast_handle->internal_sampling_freq = r_sample_rate/(realtype)2.0;
			cast_handle->internal_sampling_period = (realtype)1.0/(cast_handle->internal_sampling_freq);
			cast_handle->internal_rate_ratio = 2;
		}
		else // 192khz case
		{
			cast_handle->internal_sampling_freq = r_sample_rate/(realtype)4.0;
			cast_handle->internal_sampling_period = (realtype)1.0/(cast_handle->internal_sampling_freq);
			cast_handle->internal_rate_ratio = 4;
		}
	}

	/* Check to see if this format is supported */
	if( (r_sample_rate  > (realtype)DFXP_MAX_SAMPLING_FREQ) ||
		 (r_sample_rate  < (realtype)DFXP_MIN_SAMPLING_FREQ) ||
		 (i_num_channels > DFXP_MAX_NUM_CHANNELS) ||
		 (i_bits_per_sample > DFXP_MAX_BITS_PER_SAMPLE) ||
		 (i_bits_per_sample < DFXP_MIN_BITS_PER_SAMPLE) )
	{
		/* Set unsupported flag and skip remainining initialization functions if format isn't supported */
		cast_handle->unsupported_format_flag = IS_TRUE;
		return(OKAY);
	}
	else
		cast_handle->unsupported_format_flag = IS_FALSE;

	/* Reinitialize the dynamic qnt handles */
   if (dfxp_InitDynamicQnts(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	/* Communicate to the DSP all the new settings */
   if (dfxp_ComLoadAndRun(hp_dfxp) != OKAY)
		return(NOT_OKAY);
   if (dfxpCommunicateAll(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	if (cast_handle->trace.mode)
	{
		swprintf(cast_handle->wcp_msg1, L"dfxpBeginProcess: Successful Finish");
	   (cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
   }

	return(OKAY);
}

/*
 * FUNCTION: dfxpSetValidBits() 
 * DESCRIPTION:
 *   Initially set or change the valid bits for processing.  For example the bps may be set to 24
 *   however valid bits may only be 20.
 *
 *   NOTE: THIS FUNCTION MUST BE CALLED AFTER dfxpBeginProcess BECAUSE THAT FUNCTION RESETS THE 
 *         VALID_BITS TO THE SAME AS THE BPS.
 */
int dfxpSetValidBits(PT_HANDLE *hp_dfxp, int i_valid_bits)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if (cast_handle->valid_bits != i_valid_bits)
	{
		cast_handle->valid_bits = i_valid_bits;
	}

	return(OKAY);
}

/*
 * FUNCTION: dfxp_UpdateBufferLengthInfo() 
 * DESCRIPTION:
 *   
 *  Record into the registry info about the length of the buffer.
 *
 *  Check if this is the longest buffer processed so far.  If so it recoreds the length in the
 *  registry so that the UI can utilize it for determining things like whether or not processing 
 *  has stopped.
 *
 */
int dfxp_UpdateBufferLengthInfo(PT_HANDLE *hp_dfxp, int i_num_sample_sets, int *ip_current_buffer_msecs)
{
	struct dfxpHdlType *cast_handle;
   
	int i_longest_buffer_msecs;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);
	
	/* Calculate the number of msecs in this buffer */
	*ip_current_buffer_msecs = (int)((i_num_sample_sets * cast_handle->sampling_period) * (realtype)1000.0);

	/* Get the length of longest buffer in msecs so far */
	if (dfxp_SessionReadIntegerValue(hp_dfxp, DFXP_REGISTRY_LONGEST_BUFFER_MSECS_WIDE, 0, 
												&i_longest_buffer_msecs) != OKAY)
		return(NOT_OKAY);

	/* Check if this is the longest buffer so far.  If so, record it's length */
	if (*ip_current_buffer_msecs > i_longest_buffer_msecs)
	{
		if (dfxp_StoreLongestBufferSize(hp_dfxp, *ip_current_buffer_msecs) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}


/*
 * FUNCTION: dfxp_StoreLongestBufferSize() 
 * DESCRIPTION:
 *   
 *  Record into the registry the passed size as the longest buffer processed in msecs.
 *
 */
int dfxp_StoreLongestBufferSize(PT_HANDLE *hp_dfxp, int i_longest_buffer_msecs)
{
	struct dfxpHdlType *cast_handle;
   
	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);
	
	/* 
	 * NOTE: THIS SHOULD BE MOVED TO SHARED MEMORY.  RIGHT NOW IT MAY FAIL IF RUNNING IN EXE LIKE
	 *		   INTERNET EXPLORER WHICH DOES NOT ALLOW REGISTRY WRITES.
	 */
	if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_LONGEST_BUFFER_MSECS_WIDE, i_longest_buffer_msecs) != OKAY)
		return(OKAY);

	return(OKAY);
}