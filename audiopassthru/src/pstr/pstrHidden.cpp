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
#include "codedefs.h"

/* Standard includes */
#include <windows.h>
#include <winbase.h>
#include <dos.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "slout.h"
#include "pstr.h"
#include "pt_defs.h"
//#include "encryption.h"

/*
 * FUNCTION: pstrMapStringToInt()
 * DESCRIPTION:
 *
 *  Map the passed string seed to a random (but repeatable) value within the
 *  specfied range.
 *   
 */
int PT_DECLSPEC pstrMapStringToInt(wchar_t *wcp_seed, 
											  int i_min_mapped_val,
											  int i_max_mapped_val,
											  int *ip_mapped_val)
{
	int i_sum_of_char_vals;
	int i_range;
	int i_add_to_min;
	int i_index;
	int i_seed_length;

	if (wcp_seed == NULL)
		return(NOT_OKAY);

	i_seed_length = (int)wcslen(wcp_seed);

	i_sum_of_char_vals = 0;
	for (i_index = 0; i_index < i_seed_length; i_index++)
	{
		i_sum_of_char_vals = i_sum_of_char_vals + (int)wcp_seed[i_index];
	}

	i_range = i_max_mapped_val - i_min_mapped_val + 1;

	i_add_to_min = i_sum_of_char_vals % i_range; 

	*ip_mapped_val = i_min_mapped_val + i_add_to_min;

	return(OKAY);
}