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
 * FUNCTION: qntRToLInit()
 * DESCRIPTION:
 *  Allocates and initializes the passed qnt handle, for
 *  real to long calcutlations.
 */
int PT_DECLSPEC qntRToLInit(PT_HANDLE **hpp_qnt, CSlout *hp_slout,
					 realtype r_input_min, realtype r_input_max,
					 long l_output_min, long l_output_max,
					 int i_output_quantized, int i_num_output_levels)
{
	struct qntHdlType *cast_handle;

	/* Allocate the handle */
	cast_handle = (struct qntHdlType *)calloc(1,
						sizeof(struct qntHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->slout_hdl = hp_slout;
	cast_handle->in_out_mode = QNT_REAL_TO_LONG;

	cast_handle->r_input_min = r_input_min;
	cast_handle->r_input_max = r_input_max;

	cast_handle->l_output_min = l_output_min;
	cast_handle->l_output_max = l_output_max;

	cast_handle->output_quantized_flag = i_output_quantized;
	cast_handle->num_output_levels = i_num_output_levels;

	cast_handle->r_scale = (realtype)(l_output_max - l_output_min)/
												(r_input_max - r_input_min);

	cast_handle->r_scale_inv = (realtype) 1.0/cast_handle->r_scale;


	*hpp_qnt = (PT_HANDLE *)cast_handle;

	return(OKAY);
}

/*
 * FUNCTION: qntRToLCalc()
 * DESCRIPTION:
 *  Calculates the long output based on the passed real input.
 */
int PT_DECLSPEC qntRToLCalc(PT_HANDLE *hp_qnt, realtype r_input, long *lp_output)
{
	struct qntHdlType *cast_handle;

	cast_handle = (struct qntHdlType *)hp_qnt;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->in_out_mode != QNT_REAL_TO_LONG)
		return(NOT_OKAY);

	*lp_output = (long) ((r_input - cast_handle->r_input_min) *
       cast_handle->r_scale + cast_handle->l_output_min + (realtype) 0.5);

	return(OKAY);
}

/*
 * FUNCTION: qntRToLCalcFromOut()
 * DESCRIPTION:
 *  Calculates the real input based on the passed long output.
 */
int PT_DECLSPEC qntRToLCalcFromOut(PT_HANDLE *hp_qnt, long l_output, realtype *rp_input)
{
	struct qntHdlType *cast_handle;

	cast_handle = (struct qntHdlType *)hp_qnt;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->in_out_mode != QNT_REAL_TO_LONG)
		return(NOT_OKAY);

	*rp_input = (l_output - cast_handle->l_output_min) *
					  cast_handle->r_scale_inv + cast_handle->r_input_min;

	return(OKAY);
}

