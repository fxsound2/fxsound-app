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

#include <stdlib.h>
#include <stdio.h>

#include "slout.h"
#include "sos.h"
#include "GraphicEq.h"
#include "u_GraphicEq.h"

/*
 * FUNCTION: GraphicEqNew()
 * DESCRIPTION:
 *  Allocates and initializes the passed GraphicEq handle.
 */
int PT_DECLSPEC GraphicEqNew( PT_HANDLE **hpp_GraphicEq, int i_num_bands, int i_trace_mode, CSlout *hp_slout)
{
	struct GraphicEqHdlType *cast_handle;

	/* Allocate the handle */
	cast_handle = (struct GraphicEqHdlType *)calloc(1, sizeof(struct GraphicEqHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);

	*hpp_GraphicEq = (PT_HANDLE *)cast_handle;

	cast_handle->slout_hdl = hp_slout;
	cast_handle->i_trace_mode = i_trace_mode;

	if (i_trace_mode)
	{
		swprintf(cast_handle->wcp_msg1, L"GraphicEqNew(): Entered, i_num_bands = %d", i_num_bands);
		(cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);	
	}

	if ( (i_num_bands > GRAPHIC_EQ_MAX_NUM_BANDS) || (i_num_bands < 1) )
		return(NOT_OKAY);

	cast_handle->num_bands = i_num_bands;
	cast_handle->app_has_hyperbass = false;

	if ( sosNew( (PT_HANDLE **)&(cast_handle->sos_hdl), NULL, i_num_bands) != OKAY )
		return(NOT_OKAY);

	if (GraphicEq_InitSections(*hpp_GraphicEq) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: GraphicEqFreeUp()
 * DESCRIPTION:
 *   Frees the passed GraphicEq handle and sets to NULL.
 */
int PT_DECLSPEC GraphicEqFreeUp(PT_HANDLE **hpp_GraphicEq)
{
	struct GraphicEqHdlType *cast_handle;

	cast_handle = (struct GraphicEqHdlType *)(*hpp_GraphicEq);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Free sos handle */
	if( cast_handle->sos_hdl != NULL )
		if( sosFreeUp( &cast_handle->sos_hdl ) != OKAY )
			return(NOT_OKAY);

	/* Free main handle */
	free(cast_handle);

	*hpp_GraphicEq = NULL;

	return(OKAY);
}

