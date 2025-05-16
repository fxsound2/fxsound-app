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
 * FUNCTION: mthGenerateStatistics()
 * DESCRIPTION:
 *
 *  Uses a passed in key to both encrypt and decrypt the passed in
 *  long value. The same key is used both for encryption and decryption.
 *  The first call encrypts the value, and to decrypt the value,
 *  call the function again with the same key and the encrypted value.
 *  The key is a fixed unsigned long value.
 */
int PT_DECLSPEC mthGenerateStatistics( realtype *rp_values, int i_num_vals, realtype *rp_average,
								   realtype *rp_variance, realtype *rp_normalized_variance) 
{
	int i;
	realtype avg = (realtype)0.0;
	realtype var = 0.0;
	realtype norm;

	for(i=0; i<i_num_vals; i++)
		avg += rp_values[i];

	avg /= (realtype)i_num_vals;

	for(i=0; i<i_num_vals; i++)
	{
		realtype tmp;
		tmp = rp_values[i] - avg;
		tmp *= tmp;
		var += tmp;
	}
	var = (realtype)sqrt(var);
	norm = var / (realtype) avg;

	*rp_average = avg;
	*rp_variance = var;
	*rp_normalized_variance = norm;
    
	return(OKAY);
}


