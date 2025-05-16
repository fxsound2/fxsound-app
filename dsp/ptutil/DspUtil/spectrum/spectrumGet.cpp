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
#include <windows.h>

#include "codedefs.h"
#include "u_spectrum.h"

/*
 * FUNCTION: spectrumGetBandValues()
 * DESCRIPTION:
 *  Fills in the passed array with the previously calculated band values.
 */
int PT_DECLSPEC spectrumGetBandValues(PT_HANDLE *hp_spectrum,
							                 realtype *rp_band_values,
                                      int i_array_size)
{
	struct spectrumHdlType *cast_handle;
	int band_index;

	cast_handle = (struct spectrumHdlType *)(hp_spectrum);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (rp_band_values == NULL)
		return(NOT_OKAY);

	/* Make sure the array size is correct */
	if (i_array_size != cast_handle->num_bands)
		return(NOT_OKAY);

	for (band_index = 0; band_index < cast_handle->num_bands; band_index++)
	{
		rp_band_values[band_index] = cast_handle->band_values[band_index];
	}

	return(OKAY);
}