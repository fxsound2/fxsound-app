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
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "codedefs.h"

#include "mth.h"

#include "u_mth.h"

/*
 * FUNCTION: mthCalcCosHarmonics()
 * DESCRIPTION:
 *	Calculates the harmonics of a passed in cosine wave. Output
 * array must be dimensioned large enough to hold result, which
 * will include fundamental.
 *
 * PTNOTE- needs additional testing to make sure indexs work correctly.
 */
void PT_DECLSPEC mthCalcCosHarmonics(realtype r_cos_in, int i_num_harmonics, realtype *rp_harmonics)
{
	int i;

	/* Need to do initial calculation of first harmonic before we can start the recursion */
	rp_harmonics[0] = r_cos_in;
	rp_harmonics[1] = (realtype)2.0 * r_cos_in * r_cos_in - (realtype)1.0;

	for(i=1; i<i_num_harmonics; i++)
		rp_harmonics[i+1] = (realtype)2.0 * rp_harmonics[i] * r_cos_in - rp_harmonics[i-1];
}

