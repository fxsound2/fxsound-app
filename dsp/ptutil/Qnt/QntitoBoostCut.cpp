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
#include <math.h>

#include "codedefs.h"
#include "mry.h"
#include "qnt.h"
#include "filt.h"
#include "u_qnt.h"

/*
 * FUNCTION: qntIToBoostCutInit()
 * DESCRIPTION:
 *  Allocates and initializes the passed qnt handle.
 */
int PT_DECLSPEC qntIToBoostCutInit(PT_HANDLE **hpp_qnt, CSlout *hp_slout,
					 int i_input_min, int i_input_max,
					 realtype r_boost_min,
					 realtype r_boost_max,
					 realtype r_center_freq,
					 realtype r_samp_freq,
					 realtype r_Q,
					 int i_filter_type)
{
	struct qntHdlType *cast_handle;
	realtype boost_factor;
   int i;

	/* Allocate the handle */
	cast_handle = (struct qntHdlType *)calloc(1,
						sizeof(struct qntHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->slout_hdl = hp_slout;
	cast_handle->in_out_mode = QNT_INT_TO_BOOST_CUT; 

   cast_handle->min_int_input = i_input_min;
   cast_handle->int_array = NULL;
   cast_handle->real_array = NULL;
	cast_handle->long_array = NULL;
    
   /* Allocate the array of longs */
	cast_handle->array_size = i_input_max - i_input_min + 1;
	cast_handle->filt_array =
		        (struct filt2ndOrderBoostCutShelfFilterType *)calloc(
				                    cast_handle->array_size,
				                    sizeof(struct filt2ndOrderBoostCutShelfFilterType));

	if (cast_handle->filt_array == NULL)
		return(NOT_OKAY);

	/* Precalculate boost index factor */
	boost_factor = (r_boost_max - r_boost_min)/(realtype)(i_input_max - i_input_min);

#if 1
	/* NOTE - ORIGINAL LINEAR db BOOST */
   for (i=0; i<cast_handle->array_size; i++)
	{
		struct filt2ndOrderBoostCutShelfFilterType *filt;

		filt = &(cast_handle->filt_array[i]);

		filt->r_center_freq = r_center_freq;
		filt->r_samp_freq = r_samp_freq;
		filt->Q = r_Q;

		filt->boost = r_boost_min + (realtype)i * boost_factor;

		if ( i_filter_type == FILT_BOOST_CUT ) /* Parametric boost/cut */
		{
			if (filtCalcParametric(filt) != OKAY)
				return(NOT_OKAY);
		}
		else /* High or low shelf */
		{
			if (filtCalcShelf(filt, i_filter_type) != OKAY)
				return(NOT_OKAY);
		}
	}
#endif

#if 0
	/* NOTE - ORIGINAL LINEAR BOOST WAS TEMPORARILY MODIFIED TO A TWO
	 * PIECE LINEAR CURVE, ONE LINE FROM 0 to 3 db at i=51 and the next
	 * line from 3 at 51 to 15 at 127.
	 * USING THIS BOOST UNTIL WE GET THE AUTO OPTIMIZER WORKING
	 */
	boost_factor = (realtype)(3.0 - 0.0)/(realtype)(51 - 0);
   for (i=0; i<51; i++)
	{
		struct filt2ndOrderBoostCutShelfFilterType *filt;

		filt = &(cast_handle->filt_array[i]);

		filt->r_center_freq = r_center_freq;
		filt->r_samp_freq = r_samp_freq;
		filt->Q = r_Q;

		filt->boost = (realtype)i * boost_factor;

		if ( i_filter_type == FILT_BOOST_CUT ) /* Parametric boost/cut */
		{
			if (filtCalcParametric(filt) != OKAY)
				return(NOT_OKAY);
		}
		else /* High or low shelf */
		{
			if (filtCalcShelf(filt, i_filter_type) != OKAY)
				return(NOT_OKAY);
		}
	}

	boost_factor = (realtype)(15.0 - 3.0)/(realtype)(127 - 51);
   for (i=51; i<cast_handle->array_size; i++)
	{
		struct filt2ndOrderBoostCutShelfFilterType *filt;

		filt = &(cast_handle->filt_array[i]);

		filt->r_center_freq = r_center_freq;
		filt->r_samp_freq = r_samp_freq;
		filt->Q = r_Q;

		filt->boost = (realtype)3.0 + (realtype)(i - 51) * boost_factor;

		if ( i_filter_type == FILT_BOOST_CUT ) /* Parametric boost/cut */
		{
			if (filtCalcParametric(filt) =!= OKAY)
				return(NOT_OKAY);
		}
		else /* High or low shelf */
		{
			if (filtCalcShelf(filt, i_filter_type) != OKAY)
				return(NOT_OKAY);
		}
	}
#endif

	*hpp_qnt = (PT_HANDLE *)cast_handle;

	return(OKAY);
}


/*
 * FUNCTION: qntIToBoostCutCalc()
 * DESCRIPTION:
 *  Calculates the long output based on the passed integer input.
 */
int PT_DECLSPEC qntIToBoostCutCalc(PT_HANDLE *hp_qnt, int i_input,
											  struct filt2ndOrderBoostCutShelfFilterType *sp_filt)
{
	struct qntHdlType *cast_handle;
   int index;

	cast_handle = (struct qntHdlType *)hp_qnt;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->in_out_mode != QNT_INT_TO_BOOST_CUT)
		return(NOT_OKAY);

   /* Figure out array index */
   index = i_input - cast_handle->min_int_input;
    
   /* Make sure legal index */
   if ((index < 0) || (index >= cast_handle->array_size))
      return(NOT_OKAY);
        
   *sp_filt = cast_handle->filt_array[index];
    
	return(OKAY);
}

