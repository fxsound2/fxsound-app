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

#include "codedefs.h"
#include "mth.h"
#include "u_mth.h"

/*
 * FUNCTION: mthWarp()
 * DESCRIPTION:
 *
 *   Change the passed array of integers to be warped by the passed warp factor.  
 *   The warped values will be limited by the passed max and min values.
 *
 *   The i_symetric_around_zero flag says if the inferred output range is symetric around 0.
 *	  If not on, then the range is assummed to start at 0.  Note that the cases that are not
 *   currently handled are asymmetric about zero and a non-zero value to a general value.
 *   Note - this function assumes that the warp factor (r_factor) is always >= 0.0 .
 *
 */
int PT_DECLSPEC mthWarp(int *ip_value_array, int i_array_size, realtype r_factor,
			               int i_min_value, int i_max_value,
			               int i_symetric_around_zero)
{
	int index;
	int new_value;
	int old_value;

	/* Loop through all the values */
	for (index = 0; index < i_array_size; index++)
	{
		old_value = (ip_value_array)[index];
		   
      if (mth_WarpCalcMult(old_value, r_factor, i_min_value, i_max_value,
                        i_symetric_around_zero, &new_value) != OKAY)
		   return(NOT_OKAY);

		/* Set the warped value */
		(ip_value_array)[index] = new_value;
	}

   return(OKAY);
}

/*
 * FUNCTION: mth_WarpCalcMult()
 * DESCRIPTION:
 *   Multiply the passed i_old_value by the passed in value of r_mult.
 *   Used for general purpose control value warping.  i_symetric_about_zero on implies
 *   that the control value represents a number that goes from -x to +x.
 *   If that flag is off, then the number goes from 0 to x. 
 *   is from the middle of the passed range.  
 */
int mth_WarpCalcMult(int i_old_value, realtype r_mult, int i_min_value, 
                 int i_max_value, int i_symetric_around_zero,
                 int *ip_new_value)
{		   
   int new_value = i_old_value;                    
	realtype r_tmp;
   int mid_point;	 

   if (i_symetric_around_zero)
   {
      /* For 0->127 input, "midpoint" = 63 */
		mid_point = (i_max_value - i_min_value)/2;

    	/* Offset for two center values */	
		if (i_old_value > mid_point)
		   new_value -= (mid_point + 1); 
		else
			new_value -= mid_point;
		
		r_tmp = (new_value * r_mult);
		
      /* Compensate for truncation offset (different for pos and neg) */
      if (r_tmp >= 0.0)
         r_tmp += (realtype)0.5;
      else
         r_tmp -= (realtype)0.5;
           	
		new_value = (int)r_tmp;           	
			
    	/* Reapply correct offset */	
		if (i_old_value > mid_point)
	      new_value += (mid_point + 1); 
		else
			new_value += mid_point;		
	}                         
   else	                       
   {
      /* Handle asymmetric case */
	   r_tmp = (new_value * r_mult);
		
      /* Compensate for truncation offset (different for pos and neg) */
      if (r_tmp >= 0.0)
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

	 return(OKAY);
}

/*
 * FUNCTION: mthPanStyleWarp()
 * DESCRIPTION:
 *   Change the array values to be warped by the percentage of how far the value
 *   is from the middle of the passed range.
 */
int PT_DECLSPEC mthPanStyleWarp(int *ip_value_array, int i_array_size, int i_warp,
			                       int i_min_value, int i_max_value)
{
   int index;
	int new_value;
	int old_value;

	/* Loop through all the values */
	for (index = 0; index < i_array_size; index++)
	{
		old_value = (ip_value_array)[index];

      if (mth_CalcPanStyleWarp(old_value, i_warp, i_min_value, i_max_value, 
                                &new_value) != OKAY)
         return(NOT_OKAY);
        
		/* Take care of max and min values */
		if (new_value > i_max_value)
		   new_value = i_max_value;
		else if (new_value < i_min_value)
		   new_value = i_min_value;

		/* Set the warped value */
		(ip_value_array)[index] = new_value;
	}
	
	return(OKAY);
}  

/*
 * FUNCTION: mth_CalcPanStyleWarp()
 * DESCRIPTION:
 *   Warp the passed i_old_value by the percentage that the passed i_warp value
 *   is from the middle of the passed range.  
 *   NOTES- The function is currently repeating some operations during each
 * 	 call that could be done once during an initialization phase.  Also,
 *   some warp value based operations are being done on each element that could
 *   be done once per warp value change.
 */
int mth_CalcPanStyleWarp(int i_old_value, int i_warp, int i_min_value, 
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
