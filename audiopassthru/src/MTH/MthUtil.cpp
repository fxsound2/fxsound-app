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
#include <time.h>

#include "codedefs.h"
#include "pt_defs.h"

#include "mth.h"
#include "u_mth.h"

#include "slout.h"

// String related mth functions only built in WIN32 builds.
#if defined( WIN32 )
#include "pstr.h"
#endif //WIN32

/*
 * FUNCTION: mthIntInRange()
 * DESCRIPTION:
 *
 *  Passes back whether or not the passed value is in the range of
 *  the passed two range values.  The range values do not need to
 *  be sorted.  If the value equals a range value, it is considered in
 *  range.
 *
 *  The passed i_tolerance can increase the range by adding or subtracting
 *  to both end points by the passed amount.
 */
int PT_DECLSPEC mthIntInRange(int i_value, int i_range_val1, int i_range_val2, int i_tolerance, 
                  int *ip_in_range)
{
	int min_range;
	int max_range;

	/* Figure out min and max of range */
	if (i_range_val1 < i_range_val2)
	{
		min_range = i_range_val1;
		max_range = i_range_val2;
	}
	else
	{
		min_range = i_range_val2;
		max_range = i_range_val1;
	}

	/* Allow for tolerance */
	min_range = min_range - i_tolerance;
	max_range = max_range + i_tolerance;

	/* Check if value is in range */
	if ((i_value >= min_range) && (i_value <= max_range))
		*ip_in_range = IS_TRUE;
	else
		*ip_in_range = IS_FALSE;

	return(OKAY);
}
   
/*
 * FUNCTION: mthSetIfClose()
 * DESCRIPTION:
 *
 *   Sets the passed rp_value to be the passed r_check_val if it is close to
 *   the passed r_check_val within the passed r_delta range.  Passes back if it is
 *   in the range, into the passed ip_close variable.
 */
int PT_DECLSPEC mthSetIfClose(realtype *rp_value, realtype r_check_val, realtype r_delta, 
                  int *ip_close)
{   
   realtype difference;

   difference = (realtype)fabs(*rp_value - r_check_val);
   
   if (difference < r_delta)
   {
      *rp_value = r_check_val;
      *ip_close = IS_TRUE;
   }
   else
      *ip_close = IS_FALSE;
         
   return(OKAY);
}  

/*
 * FUNCTION: mthRealCheckEqual()
 * DESCRIPTION:
 *
 *   Check if the two real values are equal based on the passed delta. 
 *
 *   For example if r_value1 = 1.0, r_value2 = 1.0000001, r_delta = 0.001, then they will
 *   be considered equal.
 */
int PT_DECLSPEC mthRealCheckEqual(realtype r_value1, realtype r_value2, realtype r_delta, int *ip_equal)
{   
   realtype difference;

	*ip_equal = IS_FALSE;

   difference = (realtype)fabs(r_value1 - r_value2);
   
   if (difference < r_delta)
		*ip_equal = IS_TRUE;
         
   return(OKAY);
}  

/*
 * FUNCTION: mthCalcCoarseFineRange()
 * DESCRIPTION:
 *
 *   Calculates and passes back the coarse and fine max values based on the passed
 *   sum and the ratio that the two values should have.  The i_round_flag specifies
 *   if the values should be rounded to nice looking value, yet maintain the ratio.
 */
int PT_DECLSPEC mthCalcCoarseFineRange(realtype r_total, int i_ratio, int i_number_of_levels,
                           int i_round_flag, realtype *rp_max_coarse, realtype *rp_max_fine)
{
   *rp_max_fine = (r_total/((realtype) i_ratio + (realtype)1.0));
   *rp_max_coarse = r_total - *rp_max_fine;
   
   if( i_round_flag )
   {                 
   	  realtype delta, tmp_val;
      mthCalcQuantDelta(0.0, *rp_max_coarse, i_number_of_levels, &delta);
	  tmp_val = mthCalcRoundedValue(*rp_max_coarse, delta, 
		                            (realtype)-1.0e30, (realtype)1.0e30);
	  if ( tmp_val > *rp_max_coarse )
	  	*rp_max_coarse = tmp_val - delta;
	  else
	  	*rp_max_coarse = tmp_val;
	  	
	  *rp_max_fine = *rp_max_coarse/i_ratio;
   }      
   return(OKAY);
} 

/*
 * FUNCTION: mthCalcFineFromCoarseRange()
 * DESCRIPTION:
 *
 *  Based on the passed max coarse value, and assuming that the coarse and fine
 *  values were origianally calculated using mthCalcCoarseFineRange() this function, 
 *  calculates and passes back the max_fine value corresponding the the passed 
 *  max_coarse value.
 */
int PT_DECLSPEC mthCalcFineFromCoarseRange(realtype r_max_coarse, int i_ratio, int i_number_of_levels,
                               int i_round_flag, realtype *rp_max_fine)
{
	/* NOTE - only valid for the case where i_round_flag is set */
	if( i_round_flag )
		*rp_max_fine = r_max_coarse/i_ratio;
	else
		return(NOT_OKAY);

    return(OKAY);
}                                                                                           

/*
 * FUNCTION: mthCalcQuantDelta()
 * DESCRIPTION:
 *
 *   Calculates and passes back the delta value that divides the specified
 *   input range into a series of values that display with limited fractional digits.
 *   For use in quantizing a number series or in rounding a number to a simpler value.
 */
void PT_DECLSPEC mthCalcQuantDelta(realtype r_range_min, realtype r_range_max, int i_number_of_levels,
                      realtype *rp_delta)
{
    realtype rough_delta = (r_range_max - r_range_min)/(realtype)(i_number_of_levels - 1);
    realtype log_rough_delta = (realtype)log10((double)rough_delta);
    int i_ten_power;
    realtype remainder;
    i_ten_power = (int)log_rough_delta;
    remainder = log_rough_delta - i_ten_power;
    realtype delta_val;
    int index;
    	
    if( log_rough_delta < 0.0) /* Force remainder positive */
    {
		i_ten_power = (int)log_rough_delta - 1;
		remainder += (realtype)1.0;
	}
	
	/* Check for zero remainder- if so, rough_delta is a power of ten. */
	if( remainder == 0.0 )
	{                     
		delta_val = rough_delta;
	}
	else
	{		
   		/* Find smallest delta value greater than the remainder */
   		/* currently rounds to .1, .2, .25, .5 */
   		delta_val = 10.0; 
   		if( (realtype)log10(5.0) > remainder )  	    
    		delta_val = 5.0;
   		if( (realtype)log10(2.5) > remainder )  	    
    		delta_val = 2.5;
   		if( (realtype)log10(2.0) > remainder )  	    
    		delta_val = 2.0;
    		
		/* Put the correct exponent on the delta val */
		if( i_ten_power > 0 )
			for( index=0; index<i_ten_power; index++)
				delta_val *= (realtype)10.0;    		    
		if( i_ten_power < 0 )
			for( index=0; index<(-i_ten_power); index++)
				delta_val *= (realtype)0.1;
	}
			
	*rp_delta = delta_val;
}

/*
 * FUNCTION: mthCalcRoundedValue()
 * DESCRIPTION:
 *
 *   Calculates and returns the rounded value associated with the delta value passed in.
 */
realtype PT_DECLSPEC mthCalcRoundedValue(realtype r_input_value, realtype r_delta, 
							 realtype r_output_min, realtype r_output_max)
{                      
       long l_tmp;
       realtype r_tmp;
       r_tmp = (realtype)(r_input_value / r_delta);
       /* Compensate for truncation offset (different for pos and neg) */
       if( r_tmp >= 0.0 )
         	r_tmp += (realtype)0.5;
       else
           	r_tmp -= (realtype)0.5;
       l_tmp = (long)r_tmp;
       
       r_tmp = (realtype)(l_tmp * r_delta);
       
       if( r_tmp < r_output_min )
           r_tmp = r_output_min;
       if( r_tmp > r_output_max )
           r_tmp = r_output_max;
       
       return( r_tmp );
}

/*
 * FUNCTION: mthCalcClosestNiceValue()
 * DESCRIPTION:
 *
 *   Calculates and returns the closest value which rounds the specified
 *   fractional number of decimal places. For example, r_num_places = 2.5
 *   will round to the series 100, 105, 110, . . . 
 *   
 */
realtype PT_DECLSPEC mthCalcClosestNiceValue(realtype r_input_value, realtype r_num_places)
{
	realtype r_tmp, round_val, remainder, log_round_val;
	int num_decimal_places, i_tmp;

	num_decimal_places = (int)log10(r_input_value) + 1;

	remainder = r_num_places - (int)r_num_places;

	log_round_val = num_decimal_places - (int)r_num_places + (realtype)log10(remainder);

	round_val = (realtype)pow((double)10.0, (double)log_round_val);
	remainder = (realtype)fmod(r_input_value, round_val);
	i_tmp = (int)(r_input_value/round_val);
	r_tmp = (realtype)(i_tmp * round_val);
	if( remainder >= (round_val * (realtype)0.5) )
		r_tmp += round_val;

	return(r_tmp);
}

/*
 * FUNCTION: mthSearchForClosestValue()
 * DESCRIPTION:
 *
 * Search the passed array of floats for the value that is closest to the
 * passed search value.  It passes back the index corresponding to the array element
 * that is closest.
 * NOTE- current implementation does make any assumption about the nature of the
 * data in the array and thus does not use any smarts to improve speed, but is
 * guaranteed to work for any non-linear type curves.
 */
int PT_DECLSPEC mthSearchForClosestValueInRealArray(realtype r_search_value,
																	 realtype *rp_values_array,
																	 int i_array_size,
																	 int *ip_closest_index)
{
	int i;
	int closest_index;
	realtype smallest_difference;

	/* Initialize with first element as current closest */
	smallest_difference = (realtype)fabs(r_search_value - rp_values_array[0]);
	closest_index = 0;

	for(i=1; i<i_array_size; i++)
	{
		realtype trial_difference;

		trial_difference = (realtype)fabs(r_search_value - rp_values_array[i]);

		if( trial_difference < smallest_difference )
		{
			smallest_difference = trial_difference;
			closest_index = i;
		}
	}

	*ip_closest_index = closest_index;

	return(OKAY);
}

/*
 * FUNCTION:  mthRoundRealToInt()
 * DESCRIPTION:
 *
 *   Roucnd the passed real value to the proper integer value. 
 *
 *   For example: Input = 4.6 Output = 5
 *   
 */
int PT_DECLSPEC mthRoundRealToInt(realtype r_input_value, int *ip_output_value)
{
	if (r_input_value >= 0)
	{
		*ip_output_value = (int)(r_input_value + (realtype)0.5);
	}
	else
	{
		*ip_output_value = (int)(r_input_value - (realtype)0.5);
	}

	return(OKAY);
}

/*
 * FUNCTION:  mthCalcIterativeAverage()
 * DESCRIPTION:
 *
 *   Calculate a continually updated average value based on the average value to date, the
 *   newest data value, and the total number of data values used.
 *
 *   If this is the first data value used then pass 1 for the total number of data values.
 *   
 */
int PT_DECLSPEC mthCalcIterativeAverage(realtype r_average_to_date, 
													 realtype r_new_data_value,
													 long l_total_number_of_data_values,
													 realtype *rp_new_average)
{
	double new_val;
	double average;
	double avg_factor;
	double data_factor;

	if (l_total_number_of_data_values <= 0)
		return(NOT_OKAY);

	if (l_total_number_of_data_values == 1)
	{
		*rp_new_average = r_new_data_value;
		return(OKAY);
	}

	/* Calculate the interative average, use doubles internally for best accuracy */
	average = (double)r_average_to_date;
	new_val = (double)r_new_data_value;
	data_factor = (double)(1.0)/(double)l_total_number_of_data_values;
	avg_factor = (double)(l_total_number_of_data_values - 1) * data_factor;

	*rp_new_average = (realtype)( average * avg_factor + new_val * data_factor );

	return(OKAY);
}

/*
 * FUNCTION:  mthFastSqrt()
 * DESCRIPTION:
 *
 *   Calculates an appromixation of the square root of x, faster than sqrt() call.
 *   Note this function will typically have on the order of 5% error.
 *   Testing an inlined version of the function below, the performance increase appeared
 *   to vary depending on input size. For small values less than one, a performance increase
 *   of greater than 10 was seen. Far larger values greater than 1, an increase of around 2 was seen.
 */
realtype PT_DECLSPEC mthFastSqrt(realtype r_x)
{
        union
        {
                int tmp;
                float x;
        } u;
        u.x = (float)r_x;
        u.tmp -= 1<<23; /* Remove last bit so 1.0 gives 1.0 */
        /* tmp is now an approximation to logbase2(r_x) */
        u.tmp >>= 1; /* divide by 2 */
        u.tmp += 1<<29; /* add 64 to exponent: (e+127)/2 =(e/2)+63, */
        /* that represents (e/2)-64 but we want e/2 */
        return u.x;
}

#if defined( WIN32 ) // String functions are only built in WIN32 builds.

/*
 * FUNCTION: mthRotN()
 * DESCRIPTION:
 *
 *  Ansi version of mthRotN_Wide()
 */
int PT_DECLSPEC mthRotN(char *cp_string, int i_rot_n, int i_encode)
{
	wchar_t wcp_string[PT_MAX_GENERIC_STRLEN];
	int i_buffer_size;

	/* Create wide version of string */
	if (pstrConvertToWideCharString(cp_string, wcp_string, PT_MAX_GENERIC_STRLEN) != OKAY)
		return(NOT_OKAY);

	if (mthRotN_Wide(wcp_string, i_rot_n, i_encode) != OKAY)
		return(NOT_OKAY);

	i_buffer_size = strlen(cp_string) + 1;

	/* Convert back to ansi */
	if (pstrConvertWideCharStringToAnsiCharString(wcp_string, 
																 cp_string,
																 i_buffer_size) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: mthRotN_Wide()
 * DESCRIPTION:
 *
 *  Rotate the passed string by the passed amount.  This is used
 *  for encoding a string.
 */
int PT_DECLSPEC mthRotN_Wide(wchar_t *wcp_string, int i_rot_n, int i_encode)
{
	int length;
	int index;

	if (wcp_string == NULL)
		return(NOT_OKAY);

	length = (int)wcslen(wcp_string);

	for (index = 0; index < length; index++)
	{
		if (i_encode)
			wcp_string[index] = wcp_string[index] + i_rot_n;
	   else
			wcp_string[index] = wcp_string[index] - i_rot_n;
	}
       
	return(OKAY);
}

/*
 * FUNCTION: mthIsLong()
 * DESCRIPTION:
 *
 * Ansi version of mthIsLong_Wide()
 */
int PT_DECLSPEC mthIsLong(char *cp_string, int *ip_is_long)
{
	wchar_t wcp_string[PT_MAX_GENERIC_STRLEN];

	/* Create wide version of string */
	if (pstrConvertToWideCharString(cp_string, wcp_string, PT_MAX_GENERIC_STRLEN) != OKAY)
		return(NOT_OKAY);

	if (mthIsLong_Wide(wcp_string, ip_is_long) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: mthIsLong_Wide()
 * DESCRIPTION:
 *
 * Pass back if the passed string is a long int. 
 */
int PT_DECLSPEC mthIsLong_Wide(wchar_t *wcp_string, int *ip_is_long)
{
	int length;
	int index;
	int done;

	*ip_is_long = IS_FALSE; 

	if (wcp_string == NULL)
       return(NOT_OKAY);

	length = (int)wcslen(wcp_string);

	if (length <= 0)
		return(OKAY);

	done = IS_FALSE;
	index = 0;   

	while (!done)
	{
		if ((wcp_string[index] < L'0') || (wcp_string[index] > L'9'))
		{
			if (index != 0)
				done = IS_TRUE;
			else if (wcp_string[index] != '-')
				done = IS_TRUE;
		}

		if (!done)
	   {
			index++;
			if (index == length)
			{
				done = IS_TRUE;
				*ip_is_long = IS_TRUE;
		  }
		}
	}
       
	return(OKAY);
}

/*
 * FUNCTION: mthIsHex()
 * DESCRIPTION:
 *
 * Pass back if the passed string is a legal hex. 
 */
int PT_DECLSPEC mthIsHex(char *cp_string, int *ip_is_hex)
{
	wchar_t wcp_string[PT_MAX_GENERIC_STRLEN];

	/* Create wide version of string */
	if (pstrConvertToWideCharString(cp_string, wcp_string, PT_MAX_GENERIC_STRLEN) != OKAY)
		return(NOT_OKAY);

	if (mthIsHex_Wide(wcp_string, ip_is_hex) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: mthIsHex_Wide()
 * DESCRIPTION:
 *
 * Pass back if the passed string is a legal hex. 
 */
int PT_DECLSPEC mthIsHex_Wide(wchar_t *wcp_string, int *ip_is_hex)
{
	int length;
	int index;
	int done;
	int legal_char;

	*ip_is_hex = IS_FALSE; 

	if (wcp_string == NULL)
       return(NOT_OKAY);

	length = (int)wcslen(wcp_string);

	if (length <= 0)
	   return(OKAY);

	done = IS_FALSE;
   index = 0;

   while (!done)
   {
	   /* Check if it is a legal character */
	   legal_char = IS_FALSE;
	   if ((wcp_string[index] >= L'0') && (wcp_string[index] <= L'9'))
	      legal_char = IS_TRUE;
	   else if ((wcp_string[index] >= L'a') && (wcp_string[index] <= L'f'))
	      legal_char = IS_TRUE;
	   else if ((wcp_string[index] >= L'A') && (wcp_string[index] <= L'F'))
	      legal_char = IS_TRUE;

	   if (!legal_char)
	      done = IS_TRUE;
	   else
	   {
	      index++;
		  if (index == length)
		  {
		     done = IS_TRUE;
		     *ip_is_hex = IS_TRUE;
		  }
	   }
	}
       
	return(OKAY);
}
#endif // String functions are only built in WIN32 builds.