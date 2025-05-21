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

#include "codedefs.h"
#include "mth.h"
#include "filt.h"

/*
 * FUNCTION: filtRun1rstLowPass()
 * DESCRIPTION:
 *   Implements a first order lowpass.
 *   NOTE- this implementation assumes negated denominator coeffs.
 */
void PT_DECLSPEC filtRun1rstLowPass(realtype r_in, realtype *rp_in_old,
								realtype *rp_out, realtype *rp_out_old,
								realtype r_gain, realtype r_a0)
{
	*rp_out_old *= r_a0;
	*rp_out_old += (r_in + *rp_in_old) * r_gain;
	*rp_in_old = r_in;
	*rp_out = *rp_out_old;
}

/*
 * FUNCTION: filtRun2ndLowPass()
 * DESCRIPTION:
 *   Implements a second order lowpass.
 *   NOTE- this implementation assumes negated denominator coeffs.
 */
void PT_DECLSPEC filtRun2ndLowPass(realtype r_in, realtype *rp_in_minus1, realtype *rp_in_minus2,
							  realtype *rp_out, realtype *rp_out_minus1, realtype *rp_out_minus2,
							  realtype r_gain, realtype r_a1, realtype r_a0)
{
	*rp_out = *rp_out_minus1 * r_a1 + *rp_out_minus2 * r_a0;
	*rp_out_minus2 = *rp_out_minus1;
	*rp_out += (r_in + (realtype)2.0 * *rp_in_minus1 + *rp_in_minus2) * r_gain;
	*rp_out_minus1 = *rp_out;
	*rp_in_minus2 = *rp_in_minus1;
	*rp_in_minus1 = r_in;
}

/*
 * FUNCTION: filtRun1rstHighPass()
 * DESCRIPTION:
 *   Implements a first order lowpass.
 *   NOTE- this implementation assumes negated denominator coeffs.
 *   NOTE- same denominator as lowpass, similar numerator.
 */
void PT_DECLSPEC filtRun1rstHighPass(realtype r_in, realtype *rp_in_old,
								realtype *rp_out, realtype *rp_out_old,
								realtype r_gain, realtype r_a0)
{
	*rp_out_old *= r_a0;
	*rp_out_old += (r_in - *rp_in_old) * r_gain;
	*rp_in_old = r_in;
	*rp_out = *rp_out_old;
}

/*
 * FUNCTION: filtRun2ndHighPass()
 * DESCRIPTION:
 *   Implements a second order highpass.
 *   NOTE- this implementation assumes negated denominator coeffs.
 *   NOTE- same denominator as lowpass, similar numerator.
 */
void PT_DECLSPEC filtRun2ndHighPass(realtype r_in, realtype *rp_in_minus1, realtype *rp_in_minus2,
							  realtype *rp_out, realtype *rp_out_minus1, realtype *rp_out_minus2,
							  realtype r_gain, realtype r_a1, realtype r_a0)
{
	/* Note HP and LP have same denominator, similar numerator */
	*rp_out = *rp_out_minus1 * r_a1 + *rp_out_minus2 * r_a0;
	*rp_out_minus2 = *rp_out_minus1;
	*rp_out += (r_in - (realtype)2.0 * *rp_in_minus1 + *rp_in_minus2) * r_gain;
	*rp_out_minus1 = *rp_out;
	*rp_in_minus2 = *rp_in_minus1;
	*rp_in_minus1 = r_in;
}
