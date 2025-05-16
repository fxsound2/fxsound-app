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
 * FUNCTION: qntIToLInit()
 * DESCRIPTION:
 *  Allocates and initializes the passed qnt handle.
 *  NOTE- currently only handles asymetric case, and
 *  doesn't handle quantized output case.
 */
int PT_DECLSPEC qntIToLInit(PT_HANDLE **hpp_qnt, CSlout *hp_slout,
					 int i_input_min, int i_input_max,
					 long l_output_min, long l_output_max,
					 int i_output_quantized, int i_num_output_levels,
					 int i_symetric_flag)
{
	struct qntHdlType *cast_handle;
   int index;
   realtype scale;
	realtype mod_val;

	/* Allocate the handle */
	cast_handle = (struct qntHdlType *)calloc(1,
						sizeof(struct qntHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->slout_hdl = hp_slout;
	cast_handle->in_out_mode = QNT_INT_TO_LONG; 

   cast_handle->min_int_input = i_input_min;
   cast_handle->int_array = NULL;
   cast_handle->real_array = NULL;
    
   /* Allocate the array of longs */
	cast_handle->array_size = i_input_max - i_input_min + 1;
	cast_handle->long_array = (long *)calloc(cast_handle->array_size,
				               sizeof(long));
	if (cast_handle->long_array == NULL)
		return(NOT_OKAY);   
                   
	if (i_output_quantized)
	{
		mod_val = (realtype)(l_output_max - l_output_min)/(realtype)(i_num_output_levels - 1);

		scale = (realtype)(l_output_max - l_output_min)/(realtype)(i_input_max - i_input_min);
    		
		for (index=0; index < (cast_handle->array_size - 1); index++)
		{
			realtype tmp_val;

			tmp_val = l_output_min + scale * index;
			tmp_val -= (realtype)fmod(tmp_val, mod_val);
 			cast_handle->long_array[index] = (long)(tmp_val + 0.5);
 		} 

		/* Hard set endpoint to override any roundoff inaccuracies */
		cast_handle->long_array[(cast_handle->array_size - 1)] = l_output_max;
	}
	else
	{
		/* Not quantized case */
		scale = (realtype)(l_output_max - l_output_min)/(realtype)(i_input_max - i_input_min);
    		
		for (index=0; index < (cast_handle->array_size - 1); index++)
		{
 			cast_handle->long_array[index] = (long)(l_output_min + scale * index + 0.5);
 		} 

		/* Hard set endpoint to override any roundoff inaccuracies */
		cast_handle->long_array[(cast_handle->array_size - 1)] = l_output_max;
	}
	                   
	*hpp_qnt = (PT_HANDLE *)cast_handle;

	return(OKAY);
}

/*
 * FUNCTION: qntIToLInitTrackIToR()
 * DESCRIPTION:
 *  Allocates and initializes the passed qnt handle.
 *  NOTE- This is a special version that causes a long
 *  control output value, such as delay, to exactly track
 *  the values of a warped or quantized qntIToR handle.
 *  The function assumes a linear relationship between the
 *  reference values and the long output values such that if
 *  the real output is zero, the long output is the min val.
 */
int PT_DECLSPEC qntIToLInitTrackIToR(PT_HANDLE **hpp_qnt, PT_HANDLE *hp_qnt_IToR,
					 CSlout *hp_slout, long l_output_min, long l_output_max)
{
	struct qntHdlType *cast_handle;
	struct qntHdlType *cast_handle_ref;
	
    int index;
    realtype r_output_max_ref;
    realtype scale;

	/* Allocate the handle */
	cast_handle = (struct qntHdlType *)calloc(1,
						sizeof(struct qntHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);
		
	/* Assign the reference handle */
	cast_handle_ref = (struct qntHdlType *)hp_qnt_IToR;
	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle_ref->in_out_mode != QNT_INT_TO_REAL)
		return(NOT_OKAY);

	cast_handle->slout_hdl = hp_slout;
	cast_handle->in_out_mode = QNT_INT_TO_LONG; 

    cast_handle->min_int_input = cast_handle_ref->min_int_input;
    cast_handle->int_array = NULL;
    cast_handle->real_array = NULL;
    
    /* Allocate the array of longs */
	cast_handle->array_size = cast_handle_ref->array_size;
	cast_handle->long_array = (long *)calloc(cast_handle->array_size,
				               sizeof(long));
	if (cast_handle->long_array == NULL)
		return(NOT_OKAY);                       
		
	r_output_max_ref = cast_handle_ref->real_array[cast_handle_ref->array_size - 1];
                   
    scale = (realtype)(l_output_max - l_output_min)/r_output_max_ref;
    	
    for (index=0; index < (cast_handle->array_size - 1); index++)
    {
 		cast_handle->long_array[index] = 
 				(long)(l_output_min + scale * cast_handle_ref->real_array[index] + 0.5);
 		if( cast_handle->long_array[index] > l_output_max )
 			  cast_handle->long_array[index] = l_output_max;
 		if( cast_handle->long_array[index] < l_output_min )
 			  cast_handle->long_array[index] = l_output_min;

 	} 

    /* Hard set endpoint to override any roundoff inaccuracies */
    cast_handle->long_array[(cast_handle->array_size - 1)] = l_output_max;               
	                   
	*hpp_qnt = (PT_HANDLE *)cast_handle;

	return(OKAY);
}

/*
 * FUNCTION: qntIToLCalc()
 * DESCRIPTION:
 *  Calculates the long output based on the passed integer input.
 */
int PT_DECLSPEC qntIToLCalc(PT_HANDLE *hp_qnt, int i_input, long *lp_output)
{
	struct qntHdlType *cast_handle;
   int index;

	cast_handle = (struct qntHdlType *)hp_qnt;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->in_out_mode != QNT_INT_TO_LONG)
		return(NOT_OKAY);

   /* Figure out array index */
   index = i_input - cast_handle->min_int_input;
    
   /* Make sure legal index */
   if ((index < 0) || (index >= cast_handle->array_size))
      return(NOT_OKAY);
        
   *lp_output = cast_handle->long_array[index];
    
	return(OKAY);
}

