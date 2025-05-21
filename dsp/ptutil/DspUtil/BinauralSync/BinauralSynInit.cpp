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

#include "codedefs.h"
#include "BinauralSyn.h"
#include "u_BinauralSyn.h"

/*
 * FUNCTION: BinauralSynNew()
 * DESCRIPTION:
 *  Allocates and initializes the passed BinauralSyn handle.
 */
int PT_DECLSPEC BinauralSynNew( PT_HANDLE **hpp_BinauralSyn, int i_num_coeffs )
{
	int i;
	struct BinauralSynHdlType *cast_handle;

	/* Allocate the handle */
	cast_handle = (struct BinauralSynHdlType *)calloc(1, sizeof(struct BinauralSynHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);

	*hpp_BinauralSyn = (PT_HANDLE *)cast_handle;

	if ( i_num_coeffs > BINAURAL_SYN_MAX_NUM_COEFFS )
		return(NOT_OKAY);

	cast_handle->num_coeffs = i_num_coeffs;

	// Initialize sample memory, zero index
	for(i=0; i<BINAURAL_SYN_MAX_NUM_COEFFS; i++)
	{
		cast_handle->LFsamples[i] = (realtype)0.0;
		cast_handle->RFsamples[i] = (realtype)0.0;
		cast_handle->LRsamples[i] = (realtype)0.0;
		cast_handle->RRsamples[i] = (realtype)0.0;
		cast_handle->LSsamples[i] = (realtype)0.0;
		cast_handle->RSsamples[i] = (realtype)0.0;
	}

	cast_handle->last_left = (realtype)0.0;
	cast_handle->last_right = (realtype)0.0;
	cast_handle->sample_index = 0;
	cast_handle->s_index = 0;
	cast_handle->s_ratio = 1;
	cast_handle->last_samp_freq = (int)BINAURAL_SYN_DEFAULT_SAMP_FREQ;
	cast_handle->internal_samp_rate_flag = (int)BINAURAL_SYN_COEFF_SAMP_RATE_44_1;

	return(OKAY);
}

/*
 * FUNCTION: BinauralSynFreeUp()
 * DESCRIPTION:
 *   Frees the passed BinauralSyn handle and sets to NULL.
 */
int PT_DECLSPEC BinauralSynFreeUp(PT_HANDLE **hpp_BinauralSyn)
{
	struct BinauralSynHdlType *cast_handle;

	cast_handle = (struct BinauralSynHdlType *)(*hpp_BinauralSyn);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	// Free main handle
	free(cast_handle);

	*hpp_BinauralSyn = NULL;

	return(OKAY);
}
