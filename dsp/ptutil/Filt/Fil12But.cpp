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
 * FUNCTION: filtDesignSimple1rstLowPass()
 * DESCRIPTION:
 *   Designs the coefficients for a simple first order lowpass with unity gain.
 *   -3db frequency omega is normalized radian frequency, 0 -> 2PI .
 *   Note filter structure is:
 *
 *				(1 - a0)
 *		   ---------------
 *			  1 - a0z**-1
 *
 *	  This version matches the -3dB point (see MathCad calculations).
 */
void PT_DECLSPEC filtDesignSimple1rstLowPass(realtype r_omega, realtype *rp_a0)
{   
	realtype cos_om;
	realtype root_calc;

	cos_om= (realtype)cos(r_omega);
	root_calc = (realtype)sqrt(cos_om * cos_om - (realtype)4.0 * cos_om + (realtype)3.0);

	*rp_a0 = (realtype)2.0 - cos_om - root_calc;
}

/*
 * FUNCTION: filtDesign1rstLowpass()
 * DESCRIPTION:
 *   Designs the coefficients for a first order lowpass with unity gain.
 *   -3db frequency omega is normalized radian frequency, 0 -> 2PI .
 *   This version using the Binlinear Transformation.
 *   Note filter structure is:
 *
 *				g*(z+1)
 *		   --------------
 *				z - a0
 */
void PT_DECLSPEC filtDesign1rstLowPass(realtype r_omega, realtype *rp_gain, realtype *rp_a0)
{   
	realtype tmp = (realtype)1.0/((realtype)2.0 + r_omega);

	*rp_gain = r_omega * tmp;
	/* Note to simplify implementation, a0 is calculated already negated */
	*rp_a0 = ((realtype)2.0 - r_omega) * tmp;
}

/*
 * FUNCTION: filtDesign1rstHighPass()
 * DESCRIPTION:
 *   Designs the coefficients for a first order highpass with unity gain.
 *   -3db frequency omega is normalized radian frequency, 0 -> 2PI .
 *   Note filter structure is:
 *
 *				g*(z-1)
 *		   --------------
 *				z - a0
 *
 * Note denominator is same as lowpass, numerator is similar.
 */
void PT_DECLSPEC filtDesign1rstHighPass(realtype r_omega, realtype *rp_gain, realtype *rp_a0)
{   
	realtype tmp = (realtype)1.0/((realtype)2.0 + r_omega);

	*rp_gain = (realtype)2.0 * tmp;
	/* Note to simplify implementation, a0 is calculated already negated */
	*rp_a0 = ((realtype)2.0 - r_omega) * tmp;
}

/*
 * FUNCTION: filtDesign2ndButLowPass()
 * DESCRIPTION:
 *   Designs the coefficients for a second order lowpass with unity gain.
 *   -3db frequency omega is normalized radian frequency, 0 -> 2PI .
 *   Note filter structure is:
 *
 *				g(z**2 + 2z + 1)
 *		   ----------------------
 *				z**2 - a1z - a0
 *
 *	  Note denominator coeffs are designed negated to simplify implementation.
 */
void PT_DECLSPEC filtDesign2ndButLowPass(realtype r_omega, realtype *rp_gain, realtype *rp_a1, realtype *rp_a0)
{   
	realtype omega2 = r_omega * r_omega;
	realtype twoRoot2omega = (realtype)(2.0 * sqrt(2.0)) * r_omega;
	realtype tmp = (realtype)1.0/((realtype)4.0 + omega2 + twoRoot2omega);

	*rp_gain = omega2 * tmp;
	/* Note to simplify implementation, a1, a0 are calculated already negated */
	*rp_a1 = ((realtype)8.0 - (realtype)2.0 * omega2) * tmp;
	*rp_a0 = (twoRoot2omega - (realtype)4.0 - omega2) * tmp;
}

/*
 * FUNCTION: filtDesign2ndButHighPass()
 * DESCRIPTION:
 *   Designs the coefficients for a second order lowpass with unity gain.
 *   -3db frequency omega is normalized radian frequency, 0 -> 2PI .
 *   Note filter structure is:
 *
 *				g(z**2 - 2z + 1)
 *		   ----------------------
 *				z**2 - a1z - a0
 *
 *	  Note denominator coeffs are designed negated to simplify implementation.
 *	  Note denominator is same as lowpass, numerator is similar.
 */
void PT_DECLSPEC filtDesign2ndButHighPass(realtype r_omega, realtype *rp_gain, realtype *rp_a1, realtype *rp_a0)
{   
	realtype omega2 = r_omega * r_omega;
	realtype twoRoot2omega = (realtype)(2.0 * sqrt(2.0)) * r_omega;
	realtype tmp = (realtype)1.0/((realtype)4.0 + omega2 + twoRoot2omega);

	/* Note HP and LP have same denominator, similar numerator */
	*rp_gain = (realtype)4.0 * tmp;
	/* Note to simplify implementation, a1, a0 are calculated already negated */
	*rp_a1 = ((realtype)8.0 - (realtype)2.0 * omega2) * tmp;
	*rp_a0 = (twoRoot2omega - (realtype)4.0 - omega2) * tmp;
}
