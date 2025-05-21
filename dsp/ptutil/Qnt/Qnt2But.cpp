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
#include "pt_defs.h"
#include "mry.h"
#include "mth.h"
#include "filt.h"
#include "qnt.h"
#include "u_qnt.h"

/*
 * FUNCTION: qnt2ndOrderButterworthInit()
 * DESCRIPTION:
 *  A special quantization function that prepares the 3 ItoR quant handles
 *  that are needed for the common 2nd order Lowpass and Highpass filtering
 *  operations. Uses QNT_RESPONSE_EXP (exponential) response type. Currently
 *  requires that input value be less than output value.
 */
int PT_DECLSPEC qnt2ndOrderButterworthInit(PT_HANDLE **hpp_filter_gain_qnt,
					 PT_HANDLE **hpp_filter_a1_qnt,
					 PT_HANDLE **hpp_filter_a0_qnt,
					 CSlout *hp_slout,
					 int i_input_min, int i_input_max,
					 realtype r_omega_min, realtype r_omega_max,
					 int i_response_type)
{
	int i;
	int num_levels;
	realtype factor, omega;
	realtype gain, a1, a0;
   struct qntHdlType *cast_handle_gain, *cast_handle_a1, *cast_handle_a0;

	if( r_omega_min <= (realtype)0.0 )
		return(NOT_OKAY);

  	factor = (realtype)pow((double)(r_omega_max/r_omega_min), (double)(1.0/i_input_max));

	/* First do a standard initialization of the 3 ItoR qnt handles */
	if (qntIToRInit(hpp_filter_gain_qnt, hp_slout,
					    i_input_min, i_input_max,
					    (realtype)1.0, (realtype)10.0,
						 IS_FALSE, 0,
						 IS_FALSE, 0,
						 IS_FALSE, QNT_RESPONSE_EXP) != OKAY)
		return(NOT_OKAY);
  
	if (qntIToRInit(hpp_filter_a1_qnt, hp_slout,
					    i_input_min, i_input_max,
					    (realtype)1.0, (realtype)10.0,
						 IS_FALSE, 0,
						 IS_FALSE, 0,
						 IS_FALSE, QNT_RESPONSE_EXP) != OKAY)
		return(NOT_OKAY);

	if (qntIToRInit(hpp_filter_a0_qnt, hp_slout,
					    i_input_min, i_input_max,
					    (realtype)1.0, (realtype)10.0,
						 IS_FALSE, 0,
						 IS_FALSE, 0,
						 IS_FALSE, QNT_RESPONSE_EXP) != OKAY)
		return(NOT_OKAY);

	cast_handle_gain = (struct qntHdlType *)*hpp_filter_gain_qnt;
	cast_handle_a1 = (struct qntHdlType *)*hpp_filter_a1_qnt;
	cast_handle_a0 = (struct qntHdlType *)*hpp_filter_a0_qnt;

	/* Now reset quant arrays with correct filter coeff vals.
	 * Note this routine uses negated denominator coeffs to simplify
	 * implementation.
	 */
	num_levels = i_input_max - i_input_min + 1;
	omega = r_omega_min; 

	/* First hard set end points */
	filtDesign2ndButHighPass(r_omega_min, &gain, &a1, &a0);
	cast_handle_gain->real_array[0] = gain;
	cast_handle_a1->real_array[0] = a1;
	cast_handle_a0->real_array[0] = a0;

	filtDesign2ndButHighPass(r_omega_max, &gain, &a1, &a0);
	cast_handle_gain->real_array[num_levels - 1] = gain;
	cast_handle_a1->real_array[num_levels - 1] = a1;
	cast_handle_a0->real_array[num_levels - 1] = a0;

	for(i=1; i<(num_levels - 1); i++)
	{
		omega *= factor;
		filtDesign2ndButHighPass(omega, &gain, &a1, &a0);
		cast_handle_gain->real_array[i] = gain;
		cast_handle_a1->real_array[i] = a1;
		cast_handle_a0->real_array[i] = a0;
	}

	return(OKAY);
}

/*
 * FUNCTION: qntIToRSimpleLowpassInit()
 * DESCRIPTION:
 *  Initializes a real qnt handle to give the denominator coefficient for a 
 *  unity gain simple lowpass filter of the form (1 - a0)/(1 - a0z**-1).
 *  Achieves a -3dB point at the selected frequency.
 *  Input frequencies are in the hp_display_freq_qnt handle, in hz.
 *  the r_scale_freq value is used to compensate for any display factor that
 *  is present in the hp_display_freq_qnt handle. For example, if that handle
 *  is in kHz, then set r_scale_freq to 1.0e3 .
 */
int PT_DECLSPEC qntIToRSimpleLowpassInit(PT_HANDLE **hpp_coeff_qnt, PT_HANDLE *hp_display_freq_qnt, CSlout *hp_slout,
													  realtype r_sampling_period, realtype r_scale_freq)
{
	struct qntHdlType *cast_handle_coeff;
	struct qntHdlType *cast_handle_freq;
	int index;
	int array_size;

	/* Allocate the handle */
	cast_handle_coeff = (struct qntHdlType *)calloc(1,
					sizeof(struct qntHdlType));
	if (cast_handle_coeff == NULL)
		return(NOT_OKAY);

	/* Assign the freq handle */
	cast_handle_freq = (struct qntHdlType *)hp_display_freq_qnt;
	if (cast_handle_freq == NULL)
		return(NOT_OKAY);

	array_size = cast_handle_freq->array_size;

	if (cast_handle_freq->in_out_mode != QNT_INT_TO_REAL)
		return(NOT_OKAY);

	cast_handle_coeff->slout_hdl = hp_slout;
	cast_handle_coeff->in_out_mode = QNT_INT_TO_REAL;
	cast_handle_coeff->min_int_input = cast_handle_freq->min_int_input;
	cast_handle_coeff->int_array = NULL;
	cast_handle_coeff->long_array = NULL;
	cast_handle_coeff->i_force_value_index = 0; /* Init index to legal value */

	/* Allocate the array of reals */
	cast_handle_coeff->array_size = array_size;
	cast_handle_coeff->real_array = (realtype *)calloc(cast_handle_coeff->array_size,
								sizeof(realtype));
	if (cast_handle_coeff->real_array == NULL)
		return(NOT_OKAY);   

  	/* Now fill out array values. */
	for(index=0; index < array_size; index++)
	{
		realtype omega, coeff, freq;

		freq = cast_handle_freq->real_array[index] * r_scale_freq;

		omega = (realtype)(TWO_PI) * freq * r_sampling_period;
		filtDesignSimple1rstLowPass(omega, &coeff);

		cast_handle_coeff->real_array[index] = coeff;
	}

	*hpp_coeff_qnt = (PT_HANDLE *)cast_handle_coeff;

	return(OKAY);
}
