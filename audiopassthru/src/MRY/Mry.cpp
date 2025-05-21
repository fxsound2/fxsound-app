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
#include <dos.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "codedefs.h"
#include "errno.h"
#include "slout.h"
#include "mry.h"

#define DEF_GLOBAL
#include "u_mry.h"

/*
 * FUNCTION: mryFreeUpStringArray()
 * DESCRIPTION:
 *
 *  Frees up the passed array of strings.
 */
int PT_DECLSPEC mryFreeUpStringArray(char ***cppp_array, int i_num_elements) 
{
	int index;

	if (i_num_elements <= 0)
		return(OKAY);

   /* Free the array strings */
	for (index = 0; index < i_num_elements; index++)
	{
		free((*cppp_array)[index]);
	}
	free(*cppp_array);

	cppp_array = NULL;

	return(OKAY);
}

/*
 * FUNCTION: mryFreeUpStringArray_Wide()
 * DESCRIPTION:
 *
 *  Frees up the passed array of strings.
 */
int PT_DECLSPEC mryFreeUpStringArray_Wide(wchar_t ***wcppp_array, int i_num_elements) 
{
	int index;

	if (i_num_elements <= 0)
		return(OKAY);

   /* Free the array strings */
	for (index = 0; index < i_num_elements; index++)
	{
		free((*wcppp_array)[index]);
	}
	free(*wcppp_array);

	wcppp_array = NULL;

	return(OKAY);
}