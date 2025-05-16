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
#include "mry.h"
#include "qnt.h"
#include "u_qnt.h"

/*
 * FUNCTION: qntRToRInit()
 * DESCRIPTION:
 *  Allocates and initializes the passed qnt handle, for
 *  real to real calcutlations.
 */
int PT_DECLSPEC qntRToRInit(PT_HANDLE **hpp_qnt, CSlout *hp_slout,
					 realtype r_input_min, realtype r_input_max,
					 realtype r_output_min, realtype r_output_max,
					 int i_output_quantized, int i_num_output_levels,
					 int i_value_symetric)
{
	struct qntHdlType *cast_handle;

	/* Allocate the handle */
	cast_handle = (struct qntHdlType *)calloc(1,
						sizeof(struct qntHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->slout_hdl = hp_slout;
	cast_handle->in_out_mode = QNT_REAL_TO_REAL;

	cast_handle->r_input_min = r_input_min;
	cast_handle->r_input_max = r_input_max;

	cast_handle->r_output_min = r_output_min;
	cast_handle->r_output_max = r_output_max;

/*
	cast_handle->input_quantized_flag = i_input_quantized;
	cast_handle->num_input_levels = i_num_input_levels;
*/
	cast_handle->output_quantized_flag = i_output_quantized;
	cast_handle->num_output_levels = i_num_output_levels;

/*
	cast_handle->value_symetric_flag = i_zero_symetric;
*/

	cast_handle->r_scale = (r_output_max - r_output_min)/
						(realtype)(r_input_max - r_input_min);

	cast_handle->r_scale_inv = (realtype) 1.0/cast_handle->r_scale;

/*
	if( i_num_output_levels != i_num_input_levels )
		cast_handle->levels_unequal_flag = IS_TRUE;
	else
		cast_handle->levels_unequal_flag = IS_FALSE;
*/

	*hpp_qnt = (PT_HANDLE *)cast_handle;

	return(OKAY);
}

/*
 * FUNCTION: qntRToRCalc()
 * DESCRIPTION:
 *  Calculates the real output based on the passed real input.
 */
int PT_DECLSPEC qntRToRCalc(PT_HANDLE *hp_qnt, realtype r_input, realtype *rp_output)
{
	struct qntHdlType *cast_handle;

	cast_handle = (struct qntHdlType *)hp_qnt;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->in_out_mode != QNT_REAL_TO_REAL)
		return(NOT_OKAY);

	*rp_output = (r_input - cast_handle->r_input_min) *
					  cast_handle->r_scale + cast_handle->r_output_min;

	return(OKAY);
}

/*
 * FUNCTION: qntRToRCalcFromOut()
 * DESCRIPTION:
 *  Calculates the real input based on the passed real output.
 */
int PT_DECLSPEC qntRToRCalcFromOut(PT_HANDLE *hp_qnt, realtype r_output, realtype *rp_input)
{
	struct qntHdlType *cast_handle;

	cast_handle = (struct qntHdlType *)hp_qnt;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->in_out_mode != QNT_REAL_TO_REAL)
		return(NOT_OKAY);

	*rp_input = (r_output - cast_handle->r_output_min) *
					  cast_handle->r_scale_inv + cast_handle->r_input_min;

	return(OKAY);
}

