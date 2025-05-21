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
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "codedefs.h"
#include "slout.h"
#include "spectrum.h"
#include "u_spectrum.h"

/*
 * FUNCTION: spectrumNew()
 * DESCRIPTION:
 *  Allocates and initializes the passed spectrum handle.
 */
int PT_DECLSPEC spectrumNew( PT_HANDLE **hpp_spectrum, int i_num_bands, realtype r_delay_secs, realtype r_refresh_rate_secs, CSlout *hp_slout, int i_trace_mode)
{
	struct spectrumHdlType *cast_handle;

	/* Allocate the handle */
	cast_handle = (struct spectrumHdlType *)calloc( 1, sizeof(struct spectrumHdlType) );
	if( cast_handle == NULL)
		return(NOT_OKAY);

	/* Store the slout */
	cast_handle->slout_hdl = hp_slout;
	cast_handle->trace_mode = i_trace_mode;
    
 	if (i_num_bands <= 0)
		return(NOT_OKAY);

	cast_handle->num_bands = i_num_bands;

	cast_handle->buffer_index = 0;
	cast_handle->refresh_rate_secs = r_refresh_rate_secs;

	if( spectrumSetDelay((PT_HANDLE *)cast_handle, r_delay_secs) != OKAY )
		return(NOT_OKAY);
	
	/* Set default values */
	cast_handle->num_channels = 2;
	cast_handle->samp_freq = (realtype)SPECTRUM_NORMALIZED_SAMP_FREQ;

	// This function also calls spectrumReset(), initializing internal settings to default values
	if( spectrumSetSensitivity((PT_HANDLE *)cast_handle, (realtype)(SPECTRUM_DEFAULT_SENSITIVITY)) != OKAY)
		return(NOT_OKAY);

   *hpp_spectrum = (PT_HANDLE *)cast_handle;

	return(OKAY);
}

/*
 * FUNCTION: spectrumFreeUp()
 * DESCRIPTION:
 *   Frees the passed spectrum handle and sets to NULL.
 */
int PT_DECLSPEC spectrumFreeUp(PT_HANDLE **hpp_spectrum)
{
	struct spectrumHdlType *cast_handle;

	cast_handle = (struct spectrumHdlType *)(*hpp_spectrum);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Now free main handle */
	free(cast_handle);

	*hpp_spectrum = NULL;

	return(OKAY);
}