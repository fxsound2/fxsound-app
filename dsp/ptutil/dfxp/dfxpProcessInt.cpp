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
 * FUNCTION: dfxpModifyShortIntSamples() 
 * DESCRIPTION:
 *   Process the passed buffer of short int values. While short int pointer type is used to pass data,
 *   the data can be integers from 8 bits to 32 bits in size. For 8 bit and 16 bit sizes no bits are
 *   wasted (all bits are valid). For 24 and 32 bit sample sizes, 16, 20, 24 or 32 bits can be valid.
 */
int dfxpModifyShortIntSamples(PT_HANDLE *hp_dfxp, short int *sip_input_samples, 
													short int *sip_output_samples,
								   	         int i_num_sample_sets)
{
	struct dfxpHdlType *cast_handle;
	int total_num_samples;
	int i_reorder;
	int i;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_int_samples_done))
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyShortIntSamples(): Entered");

   if (!(cast_handle->fully_initialized))
		return(NOT_OKAY);

	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_int_samples_done))
	{
		swprintf(cast_handle->wcp_msg1, L"dfxpModifyShortIntSamples(): i_num_sample_sets = %d", i_num_sample_sets);
	   (cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
   }

	total_num_samples = cast_handle->num_channels_out * i_num_sample_sets;

	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_int_samples_done))
	{
		swprintf(cast_handle->wcp_msg1, L"dfxpModifyShortIntSamples(): Total Num Samples = %d", total_num_samples);
	   (cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
   }

	//If buffer is bigger than max size or format is unsupported, just copy input to output
	//Note buffer is sized to account for multiple channels, so test below is correct
	if( (i_num_sample_sets > DAW_MAX_BUFFER_SIZE) || (cast_handle->unsupported_format_flag == IS_TRUE) )
	{
		for(i=0;i<total_num_samples;i++)
			sip_output_samples[i] = sip_input_samples[i];
		return(OKAY);
	}

	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_int_samples_done))
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyShortIntSamples(): Calling mthConvertIntBufToRealtype()");

	/* Convert incoming 8 bit, 16 bit, 20 bit, 24 bit or 32 bit int buffer to 32 bit floats */
	if (mthConvertIntBufToRealtype(total_num_samples, cast_handle->bits_per_sample, 
		                            cast_handle->valid_bits, sip_input_samples, 
											 cast_handle->r_samples) != OKAY)
	   return(NOT_OKAY);

	// If surround sound, set sample reorder flag
	if ( (cast_handle->num_channels_out == 6) || (cast_handle->num_channels_out == 8) )
		i_reorder = IS_TRUE;
	else
		i_reorder = IS_FALSE;

	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_int_samples_done))
	{
		swprintf(cast_handle->wcp_msg1, L"dfxpModifyShortIntSamples(): Calling dfxpModifyRealtypeSamples(), i_reorder = %d", i_reorder);
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	}

	if (dfxpModifyRealtypeSamples(hp_dfxp, cast_handle->r_samples, i_num_sample_sets, i_reorder) != OKAY)
		return(NOT_OKAY);

	/* Convert processed buffer back to int buffer format. Note that this
	 * version of the function doesn't clip the real data and thus assumes that
	 * the realtype buffer is +/- 1.0
	 */
	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_int_samples_done))
	{
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyShortIntSamples(): Calling mthConvertRealtypeBufToIntBuf()");
		swprintf(cast_handle->wcp_msg1, L"   bps = %d, valid_bits = %d", cast_handle->bits_per_sample, cast_handle->valid_bits);
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	}

	if (mthConvertRealtypeBufToIntBuf(total_num_samples, cast_handle->bits_per_sample, 
		                               cast_handle->valid_bits, cast_handle->r_samples, 
												 sip_output_samples) != OKAY)
		return(NOT_OKAY);

	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_int_samples_done))
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyShortIntSamples(): Success");

	cast_handle->trace.i_process_int_samples_done = IS_TRUE;

	return(OKAY);
}
