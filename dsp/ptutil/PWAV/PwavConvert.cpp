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
#include <stdio.h>
#include <stdlib.h>

#ifndef __ANDROID__
#include <windows.h>
#else
#ifndef DWORD
#define DWORD unsigned int
#endif
#endif //WIN32

#include "codedefs.h"
#include "pwav.h"

// Handle and functions declared in u_pwav.h are only used in WIN32 builds.
// The function pwave24BitToFloat should be moved out of the pwav module to eliminate these problems.
#ifndef __ANDROID__
#include "u_pwav.h"
#endif //WIN32

/*
 * FUNCTION: pwav24BitToFloat()
 * DESCRIPTION:
 *  Converts from packed 24 bit format to MS float format (-1.0 to 1.0)
 *  l_length is the length in samples.
 */
int PT_DECLSPEC pwav24BitToFloat(char *cp_24_bit, realtype *rp_float,
							int i_length, int i_stereo_flag)
{
	int i;
	long itmp;
   
	if (i_stereo_flag)
		i_length *= 2;

	for( i=0; i<i_length; i++)
	{
		memcpy( &itmp, (cp_24_bit + i * 3), 3 );
		/* Note that this copy files the first 3 lsb's, so shift left */
		itmp <<= 8;
		rp_float[i] = (float)itmp * ((realtype)1.0/(realtype)2147483648.0);
	}
   
   return(OKAY);
}

/*
 * FUNCTION: pwavFloatTo24Bit()
 * DESCRIPTION:
 *  Converts from packed 24 bit format to MS float format (-1.0 to 1.0)
 *  l_length is the length in samples.
 */
int PT_DECLSPEC pwavFloatTo24Bit(realtype *rp_float, char *cp_24_bit,
							int i_length, int i_stereo_flag)
{
	int i;
	long itmp;
   
	if (i_stereo_flag)
		i_length *= 2;

	for( i=0; i<i_length; i++)
	{
		realtype ftmp = rp_float[i];

		if (ftmp >= (realtype)1.0 )
			itmp = 2147483647L;
		else if (ftmp <= (realtype)-1.0)
			/* If you try to assign to -2147483647, compiler gives warning */
			itmp = -2147483647L;
		else
			itmp = (long)(ftmp * (realtype)2147483648.0);

		/* Note that this copys the first 3 lsb's, so shift left */
		itmp >>= 8;
		memcpy( (cp_24_bit + i * 3), &itmp, 3 );
	}
   
   return(OKAY);
}
