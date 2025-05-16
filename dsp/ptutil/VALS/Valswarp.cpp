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
#include "slout.h"
#include "vals.h"
#include "u_vals.h"
#include "qnt.h"                 

/* Local definitions */
int vals_CalcPanStyleWarp(int, int, int, int, int *);

/*
 * FUNCTION: valsWarp()
 * DESCRIPTION:
 *   Change the hp_vals_target element param i_param_num of each element to 
 *   be warped by the coresponding value of the hp_dly_source multiplied by
 *   the passed warp factor.  The warped value will be limited by the
 *   passed max and min values.
 *
 *   The i_symetric_around_zero flag says if the inferred output range is symetric around 0.
 *	 If not on, then the range is assummed to start at 0.  Note that the cases that are not
 *   currently handled are asymmetric about zero and a non-zero value to a general value.
 *   Note - this function assumes that the warp factor (r_factor) is always >= 0.0 .
 */
int PT_DECLSPEC valsWarp(PT_HANDLE *hp_vals_source, PT_HANDLE *hp_vals_target,
			 int i_param_num, realtype r_factor,
			 int i_min_value, int i_max_value,
			 int i_symetric_around_zero)
{
	struct valsHdlType *cast_source_hdl;
	struct valsHdlType *cast_target_hdl;
	int element_num;
	int new_value;
	int old_value;

	cast_source_hdl = (struct valsHdlType *)hp_vals_source;
	cast_target_hdl = (struct valsHdlType *)hp_vals_target;

	if ((cast_source_hdl == NULL) || (cast_target_hdl == NULL))
		return(NOT_OKAY);

	/* Make sure they have the same number of elements */
	if ((cast_source_hdl->total_num_elements) !=
		 (cast_target_hdl->total_num_elements))
		return(NOT_OKAY);

	/* Loop through all the elements */
	for (element_num = 0; element_num < cast_source_hdl->total_num_elements;
		  element_num++)
	{
		old_value = 
		   (cast_source_hdl->element_params)[element_num][i_param_num];
		   
        vals_CalcMult(old_value, r_factor, i_min_value, i_max_value,
                            i_symetric_around_zero, &new_value);

		/* Set the warped value */
		(cast_target_hdl->element_params)[element_num][i_param_num] = 
		   new_value;
	}
	
    /* Recalculate the max value */
    if ((cast_target_hdl->max[i_param_num]).calc_flag == IS_TRUE)	
	{
	   if (vals_CalcNewMax(hp_vals_target, i_param_num, -1) != OKAY)
	      return(NOT_OKAY);
	}	

	return(OKAY);
}

/*
 * FUNCTION: valsWarp1and4()
 * DESCRIPTION:
 *
 *   Change the hp_vals_target elements 1 and 4 params of each element to 
 *   be warped by the coresponding value of the hp_dly_source multiplied by
 *   the passed warp factor.  The warped value will be limited by the
 *   passed max and min values.
 *
 *   The i_symetric_around_zero flag says if the inferred output range is symetric around 0.
 *	 NOTES - Currently, this function directly warps the control values.  Note that this
 *   will only give an approximation of true warping, since due to quantitization and 
 *	 forced values, the relationship between the control vals and the final vals is non-linear.
 *	 An enhanced method would be to calculate the actual total value implied by the fine and coarse
 *	 knobs, warp that value and then project it back onto the closest possible fine and control
 *   settings. 
 *
 *   The passed hp_coarse_qnt and and hp_fine_qnt are the qnt handles which are
 *   used to translate the passed vals to their inferred values.  These must be
 *   Integer to Real qnt handles. 
 */
int PT_DECLSPEC valsWarp1and4(PT_HANDLE *hp_vals_source, PT_HANDLE *hp_vals_target,
			 realtype r_factor,
			 int i_min_1_value, int i_max_1_value,
			 int i_min_4_value, int i_max_4_value,
			 int i_symetric_around_zero,
			 PT_HANDLE *hp_coarse_qnt,
			 PT_HANDLE *hp_fine_qnt)
{
	struct valsHdlType *cast_source_hdl;
	struct valsHdlType *cast_target_hdl;
	int element_num;
	int new_1_value;
	int new_4_value;
	int old_1_value;
    int old_4_value;
    
	cast_source_hdl = (struct valsHdlType *)hp_vals_source;
	cast_target_hdl = (struct valsHdlType *)hp_vals_target;
	
	if ((cast_source_hdl == NULL) || (cast_target_hdl == NULL))
		return(NOT_OKAY);

	if ((hp_coarse_qnt == NULL) || (hp_fine_qnt == NULL))
	   return(NOT_OKAY);
	
	/* Make sure they have the same number of elements */
	if ((cast_source_hdl->total_num_elements) !=
		 (cast_target_hdl->total_num_elements))
		return(NOT_OKAY);		
		
	/* Loop through all the elements */
	for (element_num = 0; element_num < cast_source_hdl->total_num_elements;
		  element_num++)
	{
		old_1_value = 
		   (cast_source_hdl->element_params)[element_num][1];
		old_4_value = 
		   (cast_source_hdl->element_params)[element_num][4];
		   
        {
    		realtype target_warped_val, warped_coarse, warped_fine;

			/* Calculate the targeted warped value */
			{
				realtype old_coarse_val, old_fine_val, tmp, half_delta;
				/* Array values and num bits is set for MIDI (0 -> 127) */		
				int bit_array[] = {64, 32, 16, 8, 4, 2, 1};
				int num_control_bits = 7;
				int i;
				
				if (qntIToRCalc(hp_coarse_qnt, old_1_value, &old_coarse_val) != OKAY)
					return(NOT_OKAY);
				target_warped_val = old_coarse_val * r_factor;
				if (qntIToRCalc(hp_fine_qnt, old_4_value, &old_fine_val) != OKAY)
					return(NOT_OKAY);
					
				target_warped_val += old_fine_val * r_factor;
				
            	/* Find coarse knob warping.  For asym, this is the largest value
             	 * that is less that the target warped value.  For sym case, this
             	 * is the value closest to the targeted warped value
             	 */
				new_1_value = 0;				                         
				warped_coarse = 0.0;
				for ( i=0; i<num_control_bits; i++)
				{
					if (qntIToRCalc(hp_coarse_qnt, (new_1_value + bit_array[i]), &tmp) != OKAY)
						return(NOT_OKAY);
					if ( tmp <= target_warped_val )
					{
						warped_coarse = tmp;
						new_1_value += bit_array[i];
					}
				}
				
				if (qntIToRGetHalfDelta(hp_coarse_qnt, &half_delta) != OKAY)
					return(NOT_OKAY);				
				/* For symetric case, minimize |distance| */
				if ( (i_symetric_around_zero) && 
								(( target_warped_val - warped_coarse ) > half_delta ) )
				{           
					int trial_value = new_1_value + 1;
					
					while( trial_value <= i_max_1_value )
					{
						if (qntIToRCalc(hp_coarse_qnt, trial_value, &tmp) != OKAY)
							return(NOT_OKAY);
						if( fabs(target_warped_val - tmp) < half_delta )
						{
							warped_coarse = tmp;
							new_1_value = trial_value;
							break;
						}
						trial_value++;
					}
				}						
						
            	/* Find fine value warping.  For both cases, this is the value that
             	 * minimizes the distance between the target and the final output
             	 */                
				new_4_value = 0;
				warped_fine = 0.0;
				realtype target_fine = target_warped_val - warped_coarse;
				realtype tmp_fine;
				for ( i=0; i<num_control_bits; i++)
				{
					if (qntIToRCalc(hp_fine_qnt, (new_4_value + bit_array[i]), &tmp_fine) != OKAY)
						return(NOT_OKAY);
					if ( tmp_fine <= target_fine )
					{
						warped_fine = tmp_fine;
						new_4_value += bit_array[i];
					}
				}
				/* For both fine cases, minimize |distance| */
				if (qntIToRGetHalfDelta(hp_fine_qnt, &half_delta) != OKAY)
					return(NOT_OKAY);				
				if ( (target_fine - warped_fine) > half_delta )
				{           
					int trial_value = new_4_value + 1;
					while( trial_value <= i_max_4_value )
					{
						if (qntIToRCalc(hp_fine_qnt, trial_value, &tmp) != OKAY)
							return(NOT_OKAY);
						if( fabs(target_fine - tmp) < half_delta )
						{
							warped_fine = tmp;
							new_4_value = trial_value;
							break;
						}
						trial_value++;
					}
				}						
			}			
        }                            

		if ( new_1_value < i_min_1_value )
			new_1_value = i_min_1_value;
		if ( new_1_value > i_max_1_value )
			new_1_value = i_max_1_value;
			
		if ( new_4_value < i_min_4_value )
			new_4_value = i_min_4_value;
		if ( new_4_value > i_max_4_value )
			new_4_value = i_max_4_value;

		/* Set the warped values */
		(cast_target_hdl->element_params)[element_num][1] = new_1_value; 
		(cast_target_hdl->element_params)[element_num][4] = new_4_value;
	}
	
	/* Recalculate the max sum of element params 1 and 4 */
	if (vals_CalcNewMaxSum1and4(hp_vals_target, -1) != OKAY)
	   return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: valsPanStyleWarp()
 * DESCRIPTION:
 *   Change the hp_vals_target element param i_param_num of each element to 
 *   be warped by the coresponding value of the hp_dly_source multiplied by a
 *   percentage of how far the input value is from the middle of the passed
 *   range.
 */
int PT_DECLSPEC valsPanStyleWarp(PT_HANDLE *hp_vals_source, PT_HANDLE *hp_vals_target,
			         int i_param_num, int i_warp,
			         int i_min_value, int i_max_value)
{
	struct valsHdlType *cast_source_hdl;
	struct valsHdlType *cast_target_hdl;
	int element_num;
	int new_value;
	int old_value;

	cast_source_hdl = (struct valsHdlType *)hp_vals_source;
	cast_target_hdl = (struct valsHdlType *)hp_vals_target;

	if ((cast_source_hdl == NULL) || (cast_target_hdl == NULL))
		return(NOT_OKAY);

	/* Make sure they have the same number of elements */
	if ((cast_source_hdl->total_num_elements) !=
		 (cast_target_hdl->total_num_elements))
		return(NOT_OKAY);

	/* Loop through all the elements */
	for (element_num = 0; element_num < cast_source_hdl->total_num_elements;
		  element_num++)
	{
		old_value = 
		   (cast_source_hdl->element_params)[element_num][i_param_num];

        if (vals_CalcPanStyleWarp(old_value, i_warp, i_min_value, i_max_value, 
                                 &new_value) != OKAY)
           return(NOT_OKAY);
        
		/* Take care of max and min values */
		if (new_value > i_max_value)
		   new_value = i_max_value;
		else if (new_value < i_min_value)
		   new_value = i_min_value;

		/* Set the warped value */
		(cast_target_hdl->element_params)[element_num][i_param_num] = 
		   new_value;
	}
	
    /* Recalculate the max value */
    if ((cast_target_hdl->max[i_param_num]).calc_flag == IS_TRUE)	
	{
	   if (vals_CalcNewMax(hp_vals_target, i_param_num, -1) != OKAY)
	      return(NOT_OKAY);
	}	

	return(OKAY);
}  

/*
 * FUNCTION: vals_CalcPanStyleWarp()
 * DESCRIPTION:
 *   Warp the passed i_old_value by the percentage that the passed i_warp value
 *   is from the middle of the passed range.  
 *   NOTES- The function is currently repeating some operations during each
 * 	 call that could be done once during an initialization phase.  Also,
 *   some warp value based operations are being done on each element that could
 *   be done once per warp value change.
 */
int vals_CalcPanStyleWarp(int i_old_value, int i_warp, int i_min_value, 
                          int i_max_value, int *ip_new_value)
{
	/* The following statement could be done in an initialization phase
	 * instead of being repeated on each call. */

	/* For 0->127 input, midpoint = 63 */
	int mid_point = (i_max_value - i_min_value)/2;
	
	realtype tmp_new_value = (realtype)i_old_value;
	int tmp_warp = i_warp;

    /* Offset for two center values */	
	if( i_old_value > mid_point )
		tmp_new_value -= (mid_point + 1); 
	else
		tmp_new_value -= mid_point;
		
	if( tmp_warp > mid_point )
		tmp_warp -= (mid_point + 1);
	else
		tmp_warp -= mid_point;
		
	tmp_new_value = (tmp_new_value * (realtype)tmp_warp)/(realtype)mid_point;
	
	if (tmp_new_value >= 0.0 )
		tmp_new_value += (realtype)0.5;
	else                
		tmp_new_value -= (realtype)0.5;
	
	if(tmp_new_value > 0 )
		*ip_new_value = (int)tmp_new_value + mid_point + 1;
	else
		*ip_new_value = (int)tmp_new_value + mid_point;
		
   return(OKAY);
}

/*
 * FUNCTION: vals_CalcMult()
 * DESCRIPTION:
 *   Multiply the passed i_old_value by the passed in value of r_mult.
 *   Used for general purpose control value warping.  i_symetric_about_zero on implies
 *   that the control value represents a number that goes from -x to +x.
 *   If that flag is off, then the number goes from 0 to x. 
 *   is from the middle of the passed range.  
 */
inline void vals_CalcMult(int i_old_value, realtype r_mult, int i_min_value, 
                          int i_max_value, int i_symetric_around_zero,
                          int *ip_new_value)
{		   
	 int new_value = i_old_value;                    
	 realtype r_tmp;
	 
     if ( i_symetric_around_zero )
     {
        /* For 0->127 input, "midpoint" = 63 */
		int mid_point = (i_max_value - i_min_value)/2;

    	/* Offset for two center values */	
		if( i_old_value > mid_point )
			new_value -= (mid_point + 1); 
		else
			new_value -= mid_point;
		
		r_tmp = (new_value * r_mult);
		
       /* Compensate for truncation offset (different for pos and neg) */
        if( r_tmp >= 0.0 )
         	r_tmp += (realtype)0.5;
        else
           	r_tmp -= (realtype)0.5;
           	
		new_value = (int)r_tmp;           	
			
    	/* Reapply correct offset */	
		if( i_old_value > mid_point )
			new_value += (mid_point + 1); 
		else
			new_value += mid_point;
		/* End of symmetric case */		
     }                         
     /* Handle asymmetric case */
     else	                       
     {
		r_tmp = (new_value * r_mult);
		
       /* Compensate for truncation offset (different for pos and neg) */
        if( r_tmp >= 0.0 )
         	r_tmp += (realtype)0.5;
        else
           	r_tmp -= (realtype)0.5;
           	
		new_value = (int)r_tmp;           	
	 }

	 /* Clip warped value to allowable output range */
	 if (new_value > i_max_value)
		 new_value = i_max_value;
	 else if (new_value < i_min_value)
		 new_value = i_min_value;
	
	 *ip_new_value = new_value;
}

/*
 * FUNCTION: vals_CalcMultRealOut()
 * DESCRIPTION:
 *   Multiply the passed i_old_value by the passed in value of r_mult, with real output.
 *   Used for general purpose control value warping.  i_symetric_about_zero on implies
 *   that the control value represents a number that goes from -x to +x.
 *   If that flag is off, then the number goes from 0 to x. 
 *   is from the middle of the passed range.  
 */
inline void vals_CalcMultRealOut(int i_old_value, realtype r_mult, int i_min_value, 
                          int i_max_value, int i_symetric_around_zero,
                          realtype *rp_new_value)
{		   
	 realtype r_new_value;
	 
     if ( i_symetric_around_zero )
     {
        /* For 0->127 input, "midpoint" = 63 */
		int mid_point = (i_max_value - i_min_value)/2;
		r_new_value = (realtype)i_old_value;

    	/* Offset for two center values */	
		if( i_old_value > mid_point )
			r_new_value -= (mid_point + 1); 
		else
			r_new_value -= mid_point;
		
		r_new_value = (r_new_value * r_mult);
			
    	/* Reapply correct offset */	
		if( i_old_value > mid_point )
			r_new_value += (mid_point + 1); 
		else
			r_new_value += mid_point;
		/* End of symmetric case */		
     }                         
     /* Handle asymmetric case */
     else	
		r_new_value = (i_old_value * r_mult);

	 /* Clip warped value to allowable output range */
	 if (r_new_value > (realtype)i_max_value)
		 r_new_value = (realtype)i_max_value;
	 else if (r_new_value < (realtype)i_min_value)
		 r_new_value = (realtype)i_min_value;
	
	 *rp_new_value = r_new_value;
}

/*
 * FUNCTION: vals_CalcMultWrapAround()
 * DESCRIPTION:
 *   Multiplies the passed i_old_value by the passed in value of r_mult.
 *   Used for general purpose control warping.  i_symetric_about_zero on implies
 *   that the control value represents a number that goes from -x to +x.
 *   If that flag is off, then the number goes from 0 to x. 
 *	 This special version allows the knob value to "wrap around", keeping track of
 *   the number of times the knob wraps.  Note that number of wraps can be negative
 *   for knobs that are symetric (-x to +x).
 */
inline void vals_CalcMultWrapAround(int i_old_value, realtype r_mult, int i_min_value, 
                          int i_max_value, int i_symetric_around_zero, int *ip_number_wraps, 
                          int *ip_new_value)
{		   
	 int new_value;
	 realtype r_tmp;
	 int wrap_count = 0;
	 
     if ( i_symetric_around_zero )
     {
        /* For 0 the MIDI oriented 0 to 127 input range, "midpoint" = 63 */
		int mid_point = (i_max_value - i_min_value)/2;
		new_value = i_old_value;

		/* First handle case of positive knob setting */
		if ( i_old_value > mid_point )
		{                               
			/* Note different offset for positive and negative settings */
			new_value -= mid_point + 1;;

		    r_tmp = (new_value * r_mult);
		
       	    /* Compensate for truncation offset (different for pos and neg) */
            if( r_tmp >= 0.0 )
         	    r_tmp += (realtype)0.5;
            else
           	    r_tmp -= (realtype)0.5;
           	
		    new_value = (int)r_tmp;           	
		    
		    while ( new_value > mid_point )
		    {
		    	new_value -= mid_point;
		    	wrap_count++;
		    }                   
		    /* Reapply offset */
		    new_value += mid_point + 1;
		}   
		/* Negative knob setting case */
		else
		{
			/* Note different offset for positive and negative settings */
			/* Count wrap arounds with negative total */
			new_value -= mid_point;
			
		    r_tmp = (new_value * r_mult);
		
       	    /* Compensate for truncation offset (different for pos and neg) */
            if( r_tmp >= 0.0 )
         	    r_tmp += (realtype)0.5;
            else
           	    r_tmp -= (realtype)0.5;
           	
		    new_value = (int)r_tmp;           	
		    
		    while ( new_value < (-mid_point) )
		    {
		    	new_value += mid_point;
		    	wrap_count--;
		    }                      
		    
		    /* Reapply offset */
		    new_value += mid_point;                                           
		}
	 /* End of symmetric case */		
     }                         
     /* Handle asymmetric case */
     else
     {	
		new_value = i_old_value;
		r_tmp = (new_value * r_mult);
		
        /* Compensate for truncation offset (different for pos and neg) */
        if( r_tmp >= 0.0 )
            r_tmp += (realtype)0.5;
        else
          	r_tmp -= (realtype)0.5;
           	
		new_value = (int)r_tmp;           	
		
		while ( new_value > i_max_value )
		{
		   	new_value -= i_max_value;
		   	wrap_count++;
		}
		if ( new_value < i_min_value )
			new_value = i_min_value;
	 }
	 
	 /* Now do a final clip of output values */
	 /* Note that this code should only get executed if r_warp factor was bad (negative) */
	 if ( new_value < i_min_value )
		new_value = i_min_value;

	 else if ( new_value > i_max_value )
	 	new_value = i_max_value;
	 
	 *ip_new_value = new_value;
	 *ip_number_wraps = wrap_count;
} 

/*
 * FUNCTION: vals_CalcAdd()
 * DESCRIPTION:
 *   Adds the passed i_old_value with the passed in value of i_add_value.
 *   Used for general purpose control value warping.  i_symetric_about_zero on implies
 *   that the control value represents a number that goes from -x to +x.
 *   If that flag is off, then the number goes from 0 to x. 
 *   is from the middle of the passed range.  
 */
inline void vals_CalcAdd(int i_old_value, int i_add_value, int i_min_value, 
                          int i_max_value, int i_symetric_around_zero,
                          int *ip_new_value)
{		   
	 int new_value;                    
	 
     if ( i_symetric_around_zero )
     {
        /* For 0->127 input, "midpoint" = 63 */
		int mid_point = (i_max_value - i_min_value)/2;
		new_value = i_old_value;

    	/* Offset for two center values */	
		if( i_old_value > mid_point )
			new_value -= (mid_point + 1); 
		else
			new_value -= mid_point;
		
		new_value += i_add_value;
		
    	/* Reapply correct offset */	
		if( i_old_value > mid_point )
			new_value += (mid_point + 1); 
		else
			new_value += mid_point;
		/* End of symmetric case */		
     }                         
     /* Handle asymmetric case */
     else	
	  {
	 	   new_value = i_old_value + i_add_value;
	  }

	 /* Clip warped value to allowable output range */
	 if (new_value > i_max_value)
		 new_value = i_max_value;
	 else if (new_value < i_min_value)
		 new_value = i_min_value;
	
	 *ip_new_value = new_value;
}

/*
 * FUNCTION: vals_CalcAddReal()
 * DESCRIPTION:
 *   Adds the passed i_old_value with the passed in real value of r_add_value.
 *   Used for general purpose control value warping.  i_symetric_about_zero on implies
 *   that the control value represents a number that goes from -x to +x.
 *   If that flag is off, then the number goes from 0 to x. 
 *   is from the middle of the passed range.  
 */
inline void vals_CalcAddReal(int i_old_value, realtype r_add_value, int i_min_value, 
                          int i_max_value, int i_symetric_around_zero,
                          int *ip_new_value)
{		   
	 realtype r_new_value;                    
	 int new_value;
	 
     if ( i_symetric_around_zero )
     {
        /* For 0->127 input, "midpoint" = 63 */
		int mid_point = (i_max_value - i_min_value)/2;
		r_new_value = (realtype)i_old_value;

    	/* Offset for two center values */	
		if( i_old_value > mid_point )
			r_new_value -= (mid_point + 1); 
		else
			r_new_value -= mid_point;
		
		r_new_value += r_add_value;
		
		
    	/* Reapply correct offset */	
		if( i_old_value > mid_point )
			r_new_value += (mid_point + 1); 
		else
			r_new_value += mid_point;
		/* End of symmetric case */		
     }                         
     /* Handle asymmetric case */
     else	
	  {
		   r_new_value = (realtype)i_old_value + r_add_value;
     }

     /* Compensate for truncation offset (different for pos and neg) */
     if( r_new_value >= 0.0 )
         r_new_value += (realtype)0.5;
     else
         r_new_value -= (realtype)0.5;           	
         
     new_value = (int)r_new_value;

	 /* Clip warped value to allowable output range */
	 if (new_value > i_max_value)
		 new_value = i_max_value;
	 else if (new_value < i_min_value)
		 new_value = i_min_value;
	
	 *ip_new_value = new_value;
}


/*
 * FUNCTION: vals_CalcAddWrapAround()
 * DESCRIPTION:
 *   Adds the passed i_old_value with the passed in value of i_add_value.
 *   Used for general purpose control warping.  i_symetric_about_zero on implies
 *   that the control value represents a number that goes from -x to +x.
 *   If that flag is off, then the number goes from 0 to x. 
 *	 This special version allows the knob value to "wrap around", keeping track of
 *   the number of times the knob wraps.  Note that number of wraps can be negative
 *   for knobs that are symetric (-x to +x).
 */
inline void vals_CalcAddWrapAround(int i_old_value, int i_add_value, int i_min_value, 
                          int i_max_value, int i_symetric_around_zero, int *ip_number_wraps, 
                          int *ip_new_value)
{		   
	 int new_value;
	 int wrap_count = 0;
	 
     if ( i_symetric_around_zero )
     {
        /* For 0 the MIDI oriented 0 to 127 input range, "midpoint" = 63 */
		int mid_point = (i_max_value - i_min_value)/2;
		new_value = i_old_value;

		/* First handle case of positive knob setting */
		if ( i_old_value > mid_point )
		{                               
			/* Note different offset for positive and negative settings */
			new_value -= mid_point + 1;;

		    new_value += i_add_value;
		    
		    while ( new_value > mid_point )
		    {
		    	new_value -= mid_point;
		    	wrap_count++;
		    }                   
		    /* Reapply offset */
		    new_value += mid_point + 1;
		}   
		/* Negative knob setting case */
		else
		{
			/* Note different offset for positive and negative settings */
			/* Count wrap arounds with negative total */
			new_value -= mid_point;
			
		    new_value += i_add_value;
		    
		    while ( new_value < (-mid_point) )
		    {
		    	new_value += mid_point;
		    	wrap_count--;
		    }                      
		    
		    /* Reapply offset */
		    new_value += mid_point;                                           
		}
	 /* End of symmetric case */		
     }                         
     /* Handle asymmetric case */
     else
     {	
		new_value = i_old_value;
		new_value += i_add_value;
		
		while ( new_value > i_max_value )
		{
		   	new_value -= i_max_value;
		   	wrap_count++;
		}
		if ( new_value < i_min_value )
			new_value = i_min_value;
	 }
	 
	 /* Now do a final clip of output values */
	 /* Note that this code should only get executed if r_warp factor was bad (negative) */
	 if ( new_value < i_min_value )
		new_value = i_min_value;

	 else if ( new_value > i_max_value )
	 	new_value = i_max_value;
	 
	 *ip_new_value = new_value;
	 *ip_number_wraps = wrap_count;
}