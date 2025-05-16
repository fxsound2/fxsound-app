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
#include <stdlib.h>
#include <stdio.h>

#include "codedefs.h"
#include "slout.h"
#include "mry.h"
#include "u_realSample.h"
#include "realSample.h"

/*
 * FUNCTION: realSampleForceLegalValues_ArrayOnly()
 * DESCRIPTION:
 *   Forces the passed array of real samples pts to be within the legal range.  
 *
 *   To minimize popping, values out of the legal range are reset to the value 
 *   of the previous sample pt. 
 */
int PT_DECLSPEC realSampleForceLegalValues_ArrayOnly(realtype *rp_samples, 
													              long l_num_sample_pts)
{
   long l_sample_pt_index;
   realtype r_previous_value;

	if (rp_samples == NULL)
		return(NOT_OKAY);

	r_previous_value = (realtype)0.0;

	for (l_sample_pt_index = 0; l_sample_pt_index < l_num_sample_pts; l_sample_pt_index++)
	{
		if ((rp_samples[l_sample_pt_index] < (realtype)REAL_SAMPLE_LEGAL_VALUE_MIN) ||
			 (rp_samples[l_sample_pt_index] > (realtype)REAL_SAMPLE_LEGAL_VALUE_MAX))
		{
			rp_samples[l_sample_pt_index] = r_previous_value;
		}

		r_previous_value = rp_samples[l_sample_pt_index];
	}

	return(OKAY);
}
