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
#include "spectrum.h"
#include "u_spectrum.h"

/*
 * FUNCTION: spectrumSetTimeConstant()
 * DESCRIPTION:
 *  Sets the response time constant of the spectrum displays
 */
int PT_DECLSPEC spectrumSetTimeConstant(PT_HANDLE *hp_spectrum, realtype r_time_constant)
{
	struct spectrumHdlType *cast_handle;
	double alpha, one_minus_alpha;

	cast_handle = (struct spectrumHdlType *)(hp_spectrum);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	if( (r_time_constant > (realtype)SPECTRUM_MAX_TIME_CONSTANT)
		  || (r_time_constant < (realtype)SPECTRUM_MIN_TIME_CONSTANT) )
			return(NOT_OKAY);

	cast_handle->time_constant = r_time_constant;

	alpha = exp( -(double)r_time_constant / cast_handle->internal_samp_freq );

	one_minus_alpha = 1.0 - alpha;

	cast_handle->alpha = (realtype)alpha;
	cast_handle->one_minus_alpha = (realtype)one_minus_alpha;

	return(OKAY);
}

/*
 * FUNCTION: spectrumSetSensitivity()
 * DESCRIPTION:
 *  Sets the overall sensitivity of the spectrum displays
 */
int PT_DECLSPEC spectrumSetSensitivity(PT_HANDLE *hp_spectrum, realtype r_sensitivity)
{
	struct spectrumHdlType *cast_handle;

	cast_handle = (struct spectrumHdlType *)(hp_spectrum);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	if( (r_sensitivity > (realtype)SPECTRUM_MAX_SENSITIVITY)
		  || (r_sensitivity < (realtype)SPECTRUM_MIN_SENSITIVITY) )
			return(NOT_OKAY);

	cast_handle->sensitivity = r_sensitivity * (realtype)SPECTRUM_SENSITIVITY_FACTOR;

	// Places sensitivity in to each filters gain structure
	if( spectrumReset( hp_spectrum ) != OKAY )
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: spectrumSetDelay()
 * DESCRIPTION:
 *  Sets the response time constant of the spectrum displays
 */
int PT_DECLSPEC spectrumSetDelay(PT_HANDLE *hp_spectrum, realtype r_delay_secs)
{
	struct spectrumHdlType *cast_handle;

	cast_handle = (struct spectrumHdlType *)(hp_spectrum);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	if( (r_delay_secs > (realtype)SPECTRUM_MAX_DELAY_SECS)
		  || (r_delay_secs < (realtype)SPECTRUM_MIN_DELAY_SECS) )
			return(NOT_OKAY);

	cast_handle->delay_secs = r_delay_secs;

	cast_handle->delay_count = (int)( r_delay_secs/cast_handle->refresh_rate_secs + (float)0.5 );

	return(OKAY);
}
