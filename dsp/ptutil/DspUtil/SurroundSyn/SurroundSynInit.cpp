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
#ifndef __ANDROID__
#include <windows.h>
#endif //WIN32

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "codedefs.h"
#include "slout.h"
#include "SurroundSyn.h"
#include "u_SurroundSyn.h"

/*
 * FUNCTION: SurroundSynNew()
 * DESCRIPTION:
 *  Allocates and initializes the passed SurroundSyn handle.
 */
int PT_DECLSPEC SurroundSynNew(PT_HANDLE **hpp_SurroundSyn)
{
	struct SurroundSynHdlType *cast_handle;
	int i;

	/* Allocate the handle */
	cast_handle = (struct SurroundSynHdlType *)calloc( 1, sizeof(struct SurroundSynHdlType) );
	if( cast_handle == NULL)
		return(NOT_OKAY);

	// Zero filter states
	for(i=0; i<(SURROUND_SYN_NLPZEROS + 1); i++)
	{
		cast_handle->xv[i] = 0.0;
		cast_handle->yv[i] = 0.0;
	}
	
	/* Call Reset Function with default settings */
	//if( SurroundSynReset( (PT_HANDLE *)cast_handle ) != OKAY)
	//	return(NOT_OKAY);

   *hpp_SurroundSyn = (PT_HANDLE *)cast_handle;

	return(OKAY);
}

/*
 * FUNCTION: SurroundSynFreeUp()
 * DESCRIPTION:
 *   Frees the passed SurroundSyn handle and sets to NULL.
 */
int PT_DECLSPEC SurroundSynFreeUp(PT_HANDLE **hpp_SurroundSyn)
{
	struct SurroundSynHdlType *cast_handle;

	cast_handle = (struct SurroundSynHdlType *)(*hpp_SurroundSyn);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Now free main handle */
	free(cast_handle);

	*hpp_SurroundSyn = NULL;

	return(OKAY);
}