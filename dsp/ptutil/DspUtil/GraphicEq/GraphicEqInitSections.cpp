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
#include <math.h>

#include "codedefs.h"
#include "filt.h"
#include "sos.h"
#include "u_GraphicEq.h"

/*
 * FUNCTION: GraphicEq_InitSections()
 * DESCRIPTION:
 *  Initialize the sos sections.
 */
int GraphicEq_InitSections(PT_HANDLE *hp_GraphicEq)
{
	struct GraphicEqHdlType *cast_handle;
	realtype min_band_freq, max_band_freq;

	cast_handle = (struct GraphicEqHdlType *)(hp_GraphicEq);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Set default sampling frequency, can be changed by buffer processing calls */
	cast_handle->sampling_freq = (realtype)GRAPHIC_EQ_DEFAULT_SAMPLING_FREQ;

	/* Set default first band and last band freqs., can be changed if desired by
	 * calling function below with different frequencies.
	 */
	min_band_freq = (realtype)GRAPHIC_EQ_DEFAULT_FIRST_BAND_FREQ;
	max_band_freq = (realtype)GRAPHIC_EQ_DEFAULT_LAST_BAND_FREQ;

	/* Note that his function determines and sets filter's Q value */
	if( GraphicEqReSetAllBandFreqs(hp_GraphicEq, min_band_freq, max_band_freq ) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}