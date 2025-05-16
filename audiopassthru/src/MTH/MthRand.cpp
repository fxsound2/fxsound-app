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
#ifndef __ANDROID__
#include <windows.h>
#include <winbase.h>
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "codedefs.h"

#include "mth.h"

#include "u_mth.h"

/*
	Function: mthGenRandomLongFromSeed
	a portable and reasonably fast random number generator
		multiplicative random number generator
	see
	   L'Ecuyer - Comm. of the ACM, Oct. 1990, vol. 33.
	   Numerical Recipes in C, 2nd edition, pp.  278-86
	   L'Ecuyer and Cote, ACM Transactions on Mathematical Software,
		 March 1991
	   Russian peasant algorithm -- Knuth, vol. II, pp.  442-43
*/	   
#define  MOD    2147483647L     /* modulus for generator */
#define  MULT        41358L     /* multiplier            */
						  /* modulus = mult*quotient + remainder */
#define  Q           51924L     /* int(modulus / multiplier) */
#define  R           10855L     /* remainder                 */
#define  MAX_VALUE    (MOD-1)

/* generate the next value in sequence from generator
	uses approximate factoring
	residue = (a * x) mod modulus
			= a*x - [(a*x)/modulus]*modulus
	where
		[(a*x)/modulus] = integer less than or equal to (a*x)/modulus
	approximate factoring avoids overflow associated with a*x and
		uses equivalence of above with
	residue = a * (x - q * k) - r * k + (k-k1) * modulus
	where
		modulus = a * q + r
		q  = [modulus/a]
		k  = [x/q]		(= [ax/aq])
		k1 = [a*x/modulus]
	assumes
		a, m > 0
		0 < init_rand < modulus
		a * a <= modulus
		[a*x/a*q]-[a*x/modulus] <= 1
			(for only one addition of modulus below) */
int PT_DECLSPEC mthGenRandomLongFromSeed(long init_rand, long *value)
{
	long k, residue ;

	k = init_rand / Q ;
	residue = MULT * (init_rand - Q * k) - R * k ;
	if (residue < 0)
		residue += MOD ;

	if( !(residue >= 1 && residue <= MAX_VALUE) )
		return(NOT_OKAY);

	*value = residue;

	return(OKAY);
}

/*
 * FUNCTION: mthGenerateRandomInt()
 * DESCRIPTION:
 *
 *   Generates and returns a random integer between 0 and the specified max val.
 *   This version is "seedless", but is based on current time,
 *   so it must not be called very often or many times in a row.
 */
int PT_DECLSPEC mthGenerateRandomInt(int i_max_val, int *ip_random_val)
{
	clock_t current_time;

	current_time = clock();

	*ip_random_val = current_time % (i_max_val + 1);
       
	return(OKAY);
}

/*
 * FUNCTION: mthGenerateRandomUnsignedLong()
 * DESCRIPTION:
 *
 *   Generates and returns a random unsigned long between the specified min and max val.
 *   This version is "seedless", but is based on current time,
 *   so it must not be called very often or many times in a row.
 */
int PT_DECLSPEC mthGenerateRandomUnsignedLong(unsigned long ul_min_val,
								  unsigned long ul_max_val,
								  unsigned long *ulp_random_val)
{
#ifdef WIN32
	__int64 perf_count;
	__int64 diff;

	diff = ul_max_val - ul_min_val;

	QueryPerformanceCounter( (LARGE_INTEGER *)&perf_count );

	/* Note that on some rare machines, the performance counter
	 * is not available. If this is the case assign an arbitrary
	 * value
	 */
	if( perf_count == 0 )
		perf_count = (__int64)54321;

	*ulp_random_val = (unsigned long)((__int64)ul_min_val + (perf_count % diff));

#endif /* WIN32 */
	
	return(OKAY);
}


