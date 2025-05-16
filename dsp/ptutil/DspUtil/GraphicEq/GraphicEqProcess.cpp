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
#include <stdlib.h>
#include <stdio.h>

#include "codedefs.h"
#include "sos.h"
#include "GraphicEq.h"
#include "u_GraphicEq.h"

/*
 * FUNCTION: GraphicEqProcess()
 * DESCRIPTION:
 *  Process the passed input signal and set the output signal to have the processed data.
 */
int PT_DECLSPEC GraphicEqProcess(PT_HANDLE *hp_GraphicEq,
							realtype *rp_signal_in,	 /* Input signal, points interleaved */
							realtype *rp_signal_out, /* Array to store the processed signal */
							int i_num_sample_sets,   /* Number of mono sample points or stereo sample pairs */
							int i_num_channels,      /* 1 for mono, 2 for stereo, 6 or 8 for surround */
                     realtype r_samp_freq     /* Sampling frequency in hz. */
							)
{
	struct GraphicEqHdlType *cast_handle;

	cast_handle = (struct GraphicEqHdlType *)(hp_GraphicEq);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* If the sampling frequency has changed recalc all filter coeffs */
	if( r_samp_freq != cast_handle->sampling_freq )
	{
		cast_handle->sampling_freq = r_samp_freq;
		if( GraphicEqReCalcAllBandCoeffs( hp_GraphicEq ) != OKAY )
			return(NOT_OKAY);
	}

	/* Call processing function */
	if( i_num_channels <= 2 )
	{
		if( sosProcessBuffer( (PT_HANDLE *)(cast_handle->sos_hdl), rp_signal_in, rp_signal_out, i_num_sample_sets, i_num_channels) != OKAY)
			return(NOT_OKAY);
	}
	else if( (i_num_channels == 6) || (i_num_channels == 8) )
	{
		if( sosProcessSurroundBuffer( (PT_HANDLE *)(cast_handle->sos_hdl), rp_signal_in, rp_signal_out, i_num_sample_sets, i_num_channels) != OKAY)
			return(NOT_OKAY);
	}
	else
		return(NOT_OKAY);

	return(OKAY);
}
