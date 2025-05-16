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
 * FUNCTION: filtCalcFirResponse()
 * DESCRIPTION:
 *   Calculates the frequency response of an 2nd FIR filter.
 *   Output returns in linear (not db) form. r_freq is normalized frequency.
 */
double PT_DECLSPEC filtCalcFirResponse(double *dp_coeffs, int i_num_coeffs, double d_freq)
{   
	double response, omega, real, imag;
	int i;
	
	omega = (double)MTH_TWO_PI * d_freq;
	real = 0.0;
	imag = 0.0;

	for(i=0; i<i_num_coeffs; i++)
	{
		double omega_i = (double)i * omega;

		real += dp_coeffs[i] * cos(omega_i);
		imag -= dp_coeffs[i] * sin(omega_i);
	}

	response = sqrt( real*real + imag*imag );	

	return(response);
}
