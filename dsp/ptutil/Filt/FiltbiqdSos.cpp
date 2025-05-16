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
#include <math.h>
#include <float.h>

#include "codedefs.h"
#include "filt.h"
#include "sos.h"

/* ------------------------------------------------------------------------
	filtSosParametric - Design straightforward 2nd-order presence filter, and
	load into specified second order section.
	Must be given (normalized) center frequency, Q, and
	boost/cut in db.  Frequencies are normalized to s_freq = 1.
	
	Note that internal coeffs mean:

                         -1      -2
	         A0 + A1 Z  + A2 Z
	T(Z) = ---------------------
                         -1      -2
	         1 + B1 Z  + B2 Z

	Modified from original version to be smart at 0 boost and to send
	back normalized sections of the form:

	Note that internal coeffs mean:

                         -1      -2
	              1 + A1 Z  + A2 Z
	T(Z) = g * ---------------------
                         -1      -2
	             1 + B1 Z  + B2 Z
------------------------------------------------------------------------ */
int PT_DECLSPEC filtSosParametric(PT_HANDLE *hp_sos, int section_num, realtype r_samp_freq, 
                      realtype r_center_freq, realtype boost, realtype Q)
{
	struct filt2ndOrderBoostCutShelfFilterType filt;

	filt.r_samp_freq = r_samp_freq;
	filt.r_center_freq = r_center_freq;
	filt.boost = boost;
	filt.Q = Q;

	if( filtCalcParametric(&filt) != OKAY)
		return(NOT_OKAY);

	/* Now store coeffs into sos- note a versus b notation is unconventional */
	if( sosSetSection(hp_sos, section_num, IS_TRUE, &filt, 
	                  SOS_PARA) != OKAY ) 
		return(NOT_OKAY); 
		
	return(OKAY);
}

/* ------------------------------------------------------------------------
	filtSosShelf - Design straightforward 2nd-order shelving filter, and
	load into specified second order section.
	Must be given (normalized) center (corner) frequency normalized to s_freq = 1.

	Note that internal coeffs mean:

                         -1      -2
	         A0 + A1 Z  + A2 Z
	T(Z) = ---------------------
                         -1      -2
	         1 + B1 Z  + B2 Z

	Modified from original version to be smart at 0 boost and to send
	back normalized sections of the form:

	Note that internal coeffs mean:

                         -1      -2
	              1 + A1 Z  + A2 Z
	T(Z) = g * ---------------------
                         -1      -2
	             1 + B1 Z  + B2 Z
------------------------------------------------------------------------ */
int PT_DECLSPEC filtSosShelf(PT_HANDLE *hp_sos, int section_num, int high_or_low, 
				 realtype r_samp_freq, realtype r_center_freq, realtype boost)
{	
	struct filt2ndOrderBoostCutShelfFilterType filt;

	filt.r_samp_freq = r_samp_freq;
	filt.r_center_freq = r_center_freq;
	filt.boost = boost;

	if( filtCalcShelf(&filt, high_or_low) != OKAY)
		return(NOT_OKAY);

	/* Now store coeffs into sos- note a versus b notation is unconventional */
	if( sosSetSection(hp_sos, section_num, IS_TRUE, &filt,
	                  SOS_SHELF) != OKAY ) 
		return(NOT_OKAY); 
		
	return(OKAY);
}
