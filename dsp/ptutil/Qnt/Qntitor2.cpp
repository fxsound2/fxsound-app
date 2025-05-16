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
#include "mth.h"
#include "qnt.h"
#include "u_qnt.h"

/*
 * FUNCTION: qntIToRInitReverbFeedback()
 * DESCRIPTION:
 * Allocates and initializes the passed qnt handle.
 * NOTES: This is a special version for reverb algorithms feedback
 * gains.  It normalizes the gain settings based on the relative
 * delay amounts of each element so that decay times are equal.
 * l_reference delay is the element that is matched in decay time.
 * Note that the actual output min and maxes will be different than
 * the reference values since they are compensated.
 */
int PT_DECLSPEC qntIToRInitReverbFeedback(PT_HANDLE **hpp_qnt, CSlout *hp_slout,
					 int i_input_min, int i_input_max,
					 realtype r_reference_output_min, 
					 realtype r_reference_output_max,
					 realtype r_reference_delay, realtype r_element_delay)
{
   struct qntHdlType *cast_handle;
   int index;
   int array_size;
 
   array_size = i_input_max - i_input_min + 1;
    
   /* Allocate the handle */
   cast_handle = (struct qntHdlType *)calloc(1,
						sizeof(struct qntHdlType));
   if (cast_handle == NULL)
	  return(NOT_OKAY);

   cast_handle->slout_hdl = hp_slout;
   cast_handle->in_out_mode = QNT_INT_TO_REAL;
   cast_handle->min_int_input = i_input_min;
   cast_handle->int_array = NULL;
   cast_handle->long_array = NULL;
   cast_handle->i_force_value_index = 0; /* Init index to legal value */
    
   /* Allocate the array of reals */
   cast_handle->array_size = array_size;
   cast_handle->real_array = (realtype *)calloc(cast_handle->array_size,
				               sizeof(realtype));
   if (cast_handle->real_array == NULL)
	  return(NOT_OKAY);   
		
   realtype scale = (r_reference_output_max - r_reference_output_min)
  					/(realtype)(i_input_max - i_input_min);
  					
   for (index=0; index < array_size; index++)
   {                               
  	   realtype reference_val = r_reference_output_min + scale * index;
  	
  	   if( reference_val >= 0.0 )
 		cast_handle->real_array[index] = 
 				(realtype)pow(reference_val, (r_element_delay)/(r_reference_delay));
 	   else
 		   cast_handle->real_array[index] = 
 				-(realtype)pow(-reference_val, (r_element_delay)/(r_reference_delay));
   } 

   *hpp_qnt = (PT_HANDLE *)cast_handle;

   return(OKAY);
}

/*
 * FUNCTION: qntIToRInitTrackPitchIToR()
 * DESCRIPTION:
 *  Allocates and initializes the passed qnt handle.
 * NOTES: This is a special version for pitch shifters.
 * It assumes the input handle contains pitch shift
 * amounts in cents.  The output handle contains time increment/decrement
 * values that yield the requested shifts.
 */
int PT_DECLSPEC qntIToRInitTrackPitchIToR(PT_HANDLE **hpp_qnt, PT_HANDLE *hp_qnt_IToR,
					 CSlout *hp_slout)
{
	struct qntHdlType *cast_handle;
	struct qntHdlType *cast_handle_ref;
	
   int index;

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
	cast_handle->in_out_mode = QNT_INT_TO_REAL;
   cast_handle->min_int_input = cast_handle_ref->min_int_input;
   cast_handle->int_array = NULL;
   cast_handle->long_array = NULL;
   cast_handle->i_force_value_index = 0; /* Init index to legal value */
     
   /* Allocate the array of reals */
	cast_handle->array_size = cast_handle_ref->array_size;
	cast_handle->real_array = (realtype *)calloc(cast_handle->array_size,
				               sizeof(realtype));
	if (cast_handle->real_array == NULL)
		return(NOT_OKAY);                       
		
   /* For each cent value, calculate the required timebase increment.
    * First calculate required a value - f(at)
    * Then map to incremental value f(at) = f(t(1+k))
    */
   {
     	realtype a, k, cents;
    	for (index=0; index < cast_handle->array_size; index++)
    	{
    		cents = cast_handle_ref->real_array[index];
    		/* Map cents value to time base multiplier */
    		a = (realtype)pow((double)2.0, (double)(cents/(realtype)1200.0));
    		/* Map timebase multiplier to timebase increment/decrement */
    		k = a - (realtype)1.0;
 			cast_handle->real_array[index] = -k;
 		}
 	} 

	*hpp_qnt = (PT_HANDLE *)cast_handle;

	return(OKAY);
}

/*
 * FUNCTION: qntIToRInitPitchCompIToR()
 * DESCRIPTION:
 *  Allocates and initializes the passed qnt handle.
 * NOTES: This is a special compensation function for pitch shifters.
 * It assumes the input handle contains time incremental (dsp val) coarse
 * pitch shift amounts.  The output handle contains a compensation
 * factor that is used on the fine shift value for non-zero coarse shifts.
 */
int PT_DECLSPEC qntIToRInitPitchCompIToR(PT_HANDLE **hpp_qnt, PT_HANDLE *hp_qnt_IToR,
					 CSlout *hp_slout)
{
	struct qntHdlType *cast_handle;
	struct qntHdlType *cast_handle_ref;
	
   int index;

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
	cast_handle->in_out_mode = QNT_INT_TO_REAL;
   cast_handle->min_int_input = cast_handle_ref->min_int_input;
   cast_handle->int_array = NULL;
   cast_handle->long_array = NULL;
   cast_handle->i_force_value_index = 0; /* Init index to legal value */
     
   /* Allocate the array of reals */
	cast_handle->array_size = cast_handle_ref->array_size;
	cast_handle->real_array = (realtype *)calloc(cast_handle->array_size,
				               sizeof(realtype));
	if (cast_handle->real_array == NULL)
		return(NOT_OKAY);                       
		
   /* For each coarse time increment value, calculate the required compensation.
    * NOTE- THIS IS AN EMPIRICAL FIRST SHOT- NEEDS TO BE BANGED OUT ANALYTICALLY.
    */
   {
     	realtype val;
    	for (index=0; index < cast_handle->array_size; index++)
    	{
    		val = cast_handle_ref->real_array[index];
    		cast_handle->real_array[index] = (realtype)1.0 - val;
    	}
 	} 

	*hpp_qnt = (PT_HANDLE *)cast_handle;

	return(OKAY);
}

/*
 * FUNCTION: qntIToRInitPitchSpliceDelay()
 * DESCRIPTION:
 * Allocates and initializes the passed qnt handles.
 * NOTES: This is a special compensation function for pitch shifters.
 * It initializes handles for the splice and pitch delay values based
 * on the coarse pitch screen handle values. 
 */
int PT_DECLSPEC qntIToRInitPitchSpliceDelay(PT_HANDLE **hpp_delay_qnt, PT_HANDLE **hpp_splice_qnt, PT_HANDLE *hp_coarse_qnt,
					 CSlout *hp_slout)
{
	struct qntHdlType *cast_handle_splice, *cast_handle_delay;
	struct qntHdlType *cast_handle_coarse;
	
   int index;

	/* Allocate the handles */
	cast_handle_splice = (struct qntHdlType *)calloc(1,
						sizeof(struct qntHdlType));
	if (cast_handle_splice == NULL)
		return(NOT_OKAY);
	cast_handle_delay = (struct qntHdlType *)calloc(1,
						sizeof(struct qntHdlType));
	if (cast_handle_delay == NULL)
		return(NOT_OKAY);
		
	/* Assign the reference handle */
	cast_handle_coarse = (struct qntHdlType *)hp_coarse_qnt;
	if (cast_handle_coarse == NULL)
		return(NOT_OKAY);

	if (cast_handle_coarse->in_out_mode != QNT_INT_TO_REAL)
		return(NOT_OKAY);

	cast_handle_splice->slout_hdl = hp_slout;
	cast_handle_splice->in_out_mode = QNT_INT_TO_REAL;
   cast_handle_splice->min_int_input = cast_handle_coarse->min_int_input;
   cast_handle_splice->int_array = NULL;
   cast_handle_splice->long_array = NULL;
   cast_handle_splice->i_force_value_index = 0; /* Init index to legal value */
	cast_handle_delay->slout_hdl = hp_slout;
	cast_handle_delay->in_out_mode = QNT_INT_TO_REAL;
   cast_handle_delay->min_int_input = cast_handle_coarse->min_int_input;
   cast_handle_delay->int_array = NULL;
   cast_handle_delay->long_array = NULL;
   cast_handle_delay->i_force_value_index = 0; /* Init index to legal value */
     
   /* Allocate the arrays of reals */
	cast_handle_splice->array_size = cast_handle_coarse->array_size;
	cast_handle_splice->real_array = (realtype *)calloc(cast_handle_splice->array_size,
				               sizeof(realtype));
	if (cast_handle_splice->real_array == NULL)
		return(NOT_OKAY);                       
	cast_handle_delay->array_size = cast_handle_coarse->array_size;
	cast_handle_delay->real_array = (realtype *)calloc(cast_handle_delay->array_size,
				               sizeof(realtype));
	if (cast_handle_delay->real_array == NULL)
		return(NOT_OKAY);                       
		
    /* For each coarse shift value in cents, assign delay and splice values. */
    {
    	for (index=0; index < cast_handle_coarse->array_size; index++)
    	{            
    		realtype shift = cast_handle_coarse->real_array[index]; 
    		
    		if( shift == (realtype)-1200.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)3440.0;
    		}
    		else if( shift == (realtype)-1100.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)3440.0;
    		}
    		else if( shift == (realtype)-1000.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)3440.0;
    		}
    		else if( shift == (realtype)-900.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)3440.0;
    		}
    		else if( shift == (realtype)-800.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)3440.0;
    		}
    		else if( shift == (realtype)-700.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)2300.0;
    		}
    		else if( shift == (realtype)-600.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)2300.0;
    		}
    		else if( shift == (realtype)-500.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)2300.0;
    		}
    		else if( shift == (realtype)-400.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)2300.0;
    		}
    		else if( shift == (realtype)-300.0 )
    		{
    			/* Other possible values - (1900, 1540) */
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)2300.0;
    		}
    		else if( shift == (realtype)-200.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)1540.0;
    		}
    		else if( shift == (realtype)-100.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)1200.0;
    		}
    		else if( shift == (realtype)0.0 )
    		{
    			/* Other possible values (1200, .76) */
    			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)3000.0;
    		}
    		else if( shift == (realtype)100.0 )
    		{
    			/* Other possible values (1540, .76), (2300, .76) */
    			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)3000.0;
    		}
    		else if( shift == (realtype)200.0 )
    		{
     			/* Other possible values (1540, .76), (2300, .76) */
   			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)3000.0;
    		}
    		else if( shift == (realtype)300.0 )
    		{
   				/* Other possible value 1540 */
     			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)3000.0;
    		}
    		else if( shift == (realtype)400.0 )
    		{
   				/* Other possible values- 1720, 2300, 3600 */
    			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)3000.0;
    		}
    		else if( shift == (realtype)500.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)3950.0;
    		}
    		else if( shift == (realtype)600.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)4500.0;
    		}
    		else if( shift == (realtype)700.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)4500.0;
    		}
    		else if( shift == (realtype)800.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)4500.0;
    		}
    		else if( shift == (realtype)900.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)4500.0;
    		}
    		else if( shift == (realtype)1000.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)4500.0;
    		}
    		else if( shift == (realtype)1100.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)4500.0;
    		}
    		else if( shift == (realtype)1200.0 )
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.775;
    			cast_handle_delay->real_array[index] = (realtype)4500.0;
    		}
    		else
    		/* Fall through case */
    		{
    			cast_handle_splice->real_array[index] = (realtype)0.76;
    			cast_handle_delay->real_array[index] = (realtype)2300.0;
    		}
    	}
 	} 

	*hpp_splice_qnt = (PT_HANDLE *)cast_handle_splice;
	*hpp_delay_qnt =  (PT_HANDLE *)cast_handle_delay;

	return(OKAY);
}

/*
 * FUNCTION: qntIToRdBCalcInit()
 * DESCRIPTION:
 *  Initializes a qnt handle to give the linear gains corresponding to the
 *  dB gains in the passed in qnt handle. dB setting at and beneath r_zero_threshold
 *  get set to a linear gain value of 0.0 .
 */
int PT_DECLSPEC qntIToRdBCalcInit(PT_HANDLE **hpp_linear_qnt, PT_HANDLE *hp_db_qnt, CSlout *hp_slout,
                      realtype r_zero_threshold)
{
	struct qntHdlType *cast_handle_db;
	struct qntHdlType *cast_handle_linear;
	
   int index;

	/* Allocate the linear handle */
	cast_handle_linear = (struct qntHdlType *)calloc(1,
						sizeof(struct qntHdlType));
	if (cast_handle_linear == NULL)
		return(NOT_OKAY);
		
	/* Assign the db handle */
	cast_handle_db = (struct qntHdlType *)hp_db_qnt;
	if (cast_handle_db == NULL)
		return(NOT_OKAY);

	if (cast_handle_db->in_out_mode != QNT_INT_TO_REAL)
		return(NOT_OKAY);

	cast_handle_linear->slout_hdl = hp_slout;
	cast_handle_linear->in_out_mode = QNT_INT_TO_REAL;
   cast_handle_linear->min_int_input = cast_handle_db->min_int_input;
   cast_handle_linear->int_array = NULL;
   cast_handle_linear->long_array = NULL;
   cast_handle_linear->i_force_value_index = 0; /* Init index to legal value */
     
   /* Allocate the arrays of reals */
	cast_handle_linear->array_size = cast_handle_db->array_size;
	cast_handle_linear->real_array = (realtype *)calloc(cast_handle_db->array_size,
				               sizeof(realtype));
	if (cast_handle_linear->real_array == NULL)
		return(NOT_OKAY);                       
		
   /* Calc linear values, set to 0 gain at and below threshold */
   for (index=0; index < cast_handle_db->array_size; index++)
   {            
    	realtype db = cast_handle_db->real_array[index]; 
    		
    	if( db > r_zero_threshold )
    	{
    		cast_handle_linear->real_array[index] = (realtype)pow(10.0, ((double)db * 1.0/20.0));
    	}
    	else
    		cast_handle_linear->real_array[index] = (realtype)0.0;
   }

	*hpp_linear_qnt = (PT_HANDLE *)cast_handle_linear;

	return(OKAY);
}

/*
 * FUNCTION: qntIToRTimeConstantToBeta()
 * DESCRIPTION:
 *  Initializes a qnt handle to give the linear gains corresponding to the
 *  dB gains in the passed in qnt handle. dB setting at and beneath r_zero_threshold
 *  get set to a linear gain value of 0.0 .
 */
int PT_DECLSPEC qntIToRTimeConstantBeta(PT_HANDLE **hpp_beta_qnt, PT_HANDLE *hp_time_constant_qnt, CSlout *hp_slout,
                      realtype r_sampling_freq)
{
	struct qntHdlType *cast_handle_time_constant;
	struct qntHdlType *cast_handle_beta;
	
   int index;

	/* Allocate the linear handle */
	cast_handle_beta = (struct qntHdlType *)calloc(1,
						sizeof(struct qntHdlType));
	if (cast_handle_beta == NULL)
		return(NOT_OKAY);
		
	/* Assign the db handle */
	cast_handle_time_constant = (struct qntHdlType *)hp_time_constant_qnt;
	if (cast_handle_time_constant == NULL)
		return(NOT_OKAY);

	if (cast_handle_time_constant->in_out_mode != QNT_INT_TO_REAL)
		return(NOT_OKAY);

	cast_handle_beta->slout_hdl = hp_slout;
	cast_handle_beta->in_out_mode = QNT_INT_TO_REAL;
   cast_handle_beta->min_int_input = cast_handle_time_constant->min_int_input;
   cast_handle_beta->int_array = NULL;
   cast_handle_beta->long_array = NULL;
   cast_handle_beta->i_force_value_index = 0; /* Init index to legal value */
     
   /* Allocate the arrays of reals */
	cast_handle_beta->array_size = cast_handle_time_constant->array_size;
	cast_handle_beta->real_array = (realtype *)calloc(cast_handle_time_constant->array_size,
				               sizeof(realtype));
	if (cast_handle_beta->real_array == NULL)
		return(NOT_OKAY);                       
		
   /* Calc linear values, set to 0 gain at and below threshold */
   for (index=0; index < cast_handle_time_constant->array_size; index++)
   {            
		realtype exp_arg;
    	realtype time_constant = cast_handle_time_constant->real_array[index]; 

		exp_arg = (realtype)1.0/(time_constant * (realtype)0.001 * r_sampling_freq);

		cast_handle_beta->real_array[index] = (realtype)exp(-exp_arg);
	}

  	*hpp_beta_qnt = (PT_HANDLE *)cast_handle_beta;

	return(OKAY);
}

/*
 * FUNCTION: qntIToRCalc()
 * DESCRIPTION:
 *  Calculates the real output based on the passed integer input.
 */
int PT_DECLSPEC qntIToRCalc(PT_HANDLE *hp_qnt, int i_input, realtype *rp_output)
{
	struct qntHdlType *cast_handle;
   int index;

	cast_handle = (struct qntHdlType *)hp_qnt;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->in_out_mode != QNT_INT_TO_REAL)
		return(NOT_OKAY);
                                              
   /* Figure out array index */
   index = i_input - cast_handle->min_int_input;
    
   /* Make sure legal index */
   if ((index < 0) || (index >= cast_handle->array_size))
       return(NOT_OKAY);
        
   *rp_output = cast_handle->real_array[index];
	return(OKAY);
}
   
/*
 * FUNCTION: qntIToRGetHalfDelta()
 * DESCRIPTION:
 *  Pass back the half_delta value of the passed handle.
 */
int PT_DECLSPEC qntIToRGetHalfDelta(PT_HANDLE *hp_qnt, realtype *rp_half_delta)
{
	struct qntHdlType *cast_handle; 
	
	cast_handle = (struct qntHdlType *)hp_qnt;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->in_out_mode != QNT_INT_TO_REAL)
		return(NOT_OKAY);
	
	*rp_half_delta = cast_handle->half_delta;
		
	return(OKAY);
}  

/*
 * FUNCTION: qntIToRCalcFromOut()
 * DESCRIPTION:
 *   Based on the passed output value figure out and pass back what input value
 *   would be the closest to generating this output (as long as it is less than 
 *   or equal).
 *
 *   Note: This function assumes that the output values are in increasing order.
 */
int PT_DECLSPEC qntIToRCalcFromOut(PT_HANDLE *hp_qnt, realtype r_output, int *ip_input, 
                       realtype *rp_output)
{
   struct qntHdlType *cast_handle;
   int index;
   int done;
	
   cast_handle = (struct qntHdlType *)hp_qnt;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   if (cast_handle->in_out_mode != QNT_INT_TO_REAL)
      return(NOT_OKAY);  
  
   /* Initalize return values */
   index = cast_handle->min_int_input;
   *ip_input = index;
   *rp_output = (cast_handle->real_array)[index];
   done = IS_FALSE;
  
   /* Find the closest output value (less than or equal) */
   while (!done)
   {
      index++;
      if (index >= cast_handle->array_size)
         done = IS_TRUE;
      else if ((cast_handle->real_array)[index] > r_output)
         done = IS_TRUE;
      else
      {
         if ((cast_handle->real_array)[index] > *rp_output)
         {
            *ip_input = index;
            *rp_output = (cast_handle->real_array)[index];            
         }
      }
   }
	
   return(OKAY);
}
