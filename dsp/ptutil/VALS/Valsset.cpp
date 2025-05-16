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

/* Standard includes */
#include <dos.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "slout.h"
#include "mry.h"
#include "vals.h"
#include "u_vals.h"  
#include "GraphicEq.h"
 
/*
 * FUNCTION: valsSetTotalNumElements()
 * DESCRIPTION:
 *  Set total number of elements possible.
 */
int PT_DECLSPEC valsSetTotalNumElements(PT_HANDLE *hp_vals, int i_num_elements)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->total_num_elements = i_num_elements;

	return(OKAY);
} 
 
/*
 * FUNCTION: valsSetComment()
 * DESCRIPTION:
 *   Set the comment associated with the effect.
 */
int PT_DECLSPEC valsSetComment(PT_HANDLE *hp_vals, wchar_t *wcp_comment)
{
	struct valsHdlType *cast_handle;
	int length;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Free the old comment */
	if (cast_handle->wcp_comment != NULL)
		free(cast_handle->wcp_comment);

	if (wcp_comment == NULL)
	{
		cast_handle->wcp_comment = NULL;
		return(OKAY);
	}

	/* Allocate the string */
	length = (int)wcslen(wcp_comment);
	cast_handle->wcp_comment = (wchar_t *)calloc(1, ((length+1)*sizeof(wchar_t)));
	if (cast_handle->wcp_comment == NULL)
		return(NOT_OKAY);

	/* Set the string */
	swprintf(cast_handle->wcp_comment, L"%s", wcp_comment);

	return(OKAY);
}

/*
 * FUNCTION: valsSetParamSet()
 * DESCRIPTION:
 *   Sets which parameter set will be operated on.  This only makes 
 *   sense if the vals is a "double_params" type.  The passed value
 *   can be either VALS_PARAM_SET_1 or VALS_PARAM_SET_2.
 */
int PT_DECLSPEC valsSetParamSet(PT_HANDLE *hp_vals, int i_param_set)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);
    
    cast_handle->param_set = i_param_set;
   
    return(OKAY);
}  

/*
 * FUNCTION: valsSetMainParamValue()
 * DESCRIPTION:
 *   Set the passed main parameter value.
 */
int PT_DECLSPEC valsSetMainParamValue(PT_HANDLE *hp_vals, int i_param_num, int i_value)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);  

	if ((i_param_num < 0) || (i_param_num >= VALS_NUM_MAIN_PARAMS))
	   return(NOT_OKAY);

	if ((cast_handle->double_params) && 
	    (cast_handle->param_set == VALS_PARAM_SET_2))
	   (cast_handle->main_params_2)[i_param_num] = i_value;	
	else
	   (cast_handle->main_params)[i_param_num] = i_value;
    
    return(OKAY);
}  

/*
 * FUNCTION: valsSetElementParamValue()
 * DESCRIPTION:
 *   Set the passed element parameter value.
 */
int PT_DECLSPEC valsSetElementParamValue(PT_HANDLE *hp_vals, int i_element_num, 
                             int i_param_num, int i_value)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);  

	if ((i_param_num < 0) || (i_param_num >= VALS_NUM_ELEMENT_PARAMS))
	   return(NOT_OKAY);

	if ((i_element_num < 0) || (i_element_num >= cast_handle->total_num_elements))
	   return(NOT_OKAY);
	   
	if ((cast_handle->double_params) && 
	    (cast_handle->param_set == VALS_PARAM_SET_2))
       (cast_handle->element_params_2)[i_element_num][i_param_num] = i_value; 
	else
	   (cast_handle->element_params)[i_element_num][i_param_num] = i_value;	

	/* Calculate the max value */
    if ((cast_handle->max[i_param_num]).calc_flag == IS_TRUE)	
	{
	   if (vals_CalcNewMax(hp_vals, i_param_num, i_element_num) != OKAY)
	      return(NOT_OKAY);
	}
	
	/* Calculate the new max sum of element params 1 and 4 */
	if (((i_param_num == 1) || (i_param_num == 4)) && 
	    (cast_handle->maxSum1_4.calc_flag))
	{
	   if (vals_CalcNewMaxSum1and4(hp_vals, i_element_num) != OKAY)
		return(NOT_OKAY);
    }
    
    return(OKAY);
}  

/*
 * FUNCTION: vals_CalcNewMax()
 * DESCRIPTION:
 *   Calculate and stores the new max value of the passed parameter, based
 *   on the fact that the passed element has been changed.
 *
 *   Pass in -1 for the element num to specify that nothing has changed, and you 
 *   want to force a brand new calculation.
 */
int vals_CalcNewMax(PT_HANDLE *hp_vals, int i_param_num, int i_element_num)
{
	struct valsHdlType *cast_handle;
	int new_value;
	int new_abs_value;
	int max_value;
	int max_abs_value;
	int index;
    int store_new_max;
    
	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);
		
    if ((i_param_num < 0) || (i_param_num >= VALS_NUM_ELEMENT_PARAMS))
       return(NOT_OKAY);

	if (i_element_num != -1)
	{
	   if ((i_element_num < 0) || 
	       (i_element_num >= cast_handle->total_num_elements))
	      return(NOT_OKAY);
	}
	
	/*
	 * Check if max value can be changed quickly without looping through
	 * all the elements.  We can do this if the new sum is greater than the
	 * old greatest value.
	 */
	if (i_element_num != -1)
	{
	   new_value = (cast_handle->element_params)[i_element_num][i_param_num];
       max_value = (cast_handle->max[i_param_num]).value;
 
	   store_new_max = IS_FALSE;
	   if ((cast_handle->max[i_param_num]).abs_flag)
	   {
          new_abs_value = abs(new_value);
          max_abs_value = abs(max_value);
          
	      if (new_abs_value > max_abs_value)
	         store_new_max = IS_TRUE;
	   }
	   else
	   {
	      if (new_value > max_value)
	         store_new_max = IS_TRUE; 
	   }
	   
	   if (store_new_max)
	   { 
			(cast_handle->max[i_param_num]).value = new_value;
			(cast_handle->max[i_param_num]).index = i_element_num;
	        return(OKAY);
	   }
	}

	/* Otherwise, need to loop through the elements to find max */
	max_value =  0;
	(cast_handle->max[i_param_num]).value = max_value;
	(cast_handle->max[i_param_num]).index = 0;
	
	for (index = 0; index < cast_handle->total_num_elements; index++)
	{
	   new_value = (cast_handle->element_params)[index][i_param_num];          
	   store_new_max = IS_FALSE;
	   
	   if ((cast_handle->max[i_param_num]).abs_flag)
	   {
	      new_abs_value = abs(new_value);
          max_abs_value = abs(max_value);
          
	      if (new_abs_value > max_abs_value)
	         store_new_max = IS_TRUE;
	   }
	   else
	   {
	      if (new_value > max_value)
	         store_new_max = IS_TRUE; 
	   }
	   
	   if (store_new_max)
	   {
	      max_value = new_value;
	      (cast_handle->max[i_param_num]).value = new_value;
		  (cast_handle->max[i_param_num]).index = index;
	   }
    }

	return(OKAY);
}

/*
 * FUNCTION: vals_CalcNewMaxSum1and4()
 * DESCRIPTION:
 *   Calculate and stores the new global max sum of 1 and 4, based
 *   on the fact that the passed element has been changed.
 *
 *   Pass in -1 for the element num to specify that nothing has changed, and you want
 *   to force a brand new calculation.
 */
int vals_CalcNewMaxSum1and4(PT_HANDLE *hp_vals, int i_element_num)
{
	struct valsHdlType *cast_handle;
	long new_sum;
    long new_abs_sum;
    long max_sum;
    long max_abs_sum;
	int index;
    int store_new_max;
    
	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);
		
	if (i_element_num != -1)
	{
	   if ((i_element_num < 0) || 
	       (i_element_num >= cast_handle->total_num_elements))
	      return(NOT_OKAY);
	}
	
	/*
	 * Check if max value can be changed quickly without looping through
	 * all the elements.  We can do this if the new sum is greater than the
	 * old greatest value.
	 */
	if (i_element_num != -1)
	{
	   new_sum = (long)(((cast_handle->factor_1_to_4) * 
	                     (cast_handle->element_params)[i_element_num][1]) +
	                     (cast_handle->element_params)[i_element_num][4]);
	   max_sum = cast_handle->maxSum1_4.sum_scale_val;
	             
	   store_new_max = IS_FALSE;
	   if (cast_handle->maxSum1_4.abs_flag)
	   {
          new_abs_sum = labs(new_sum);
          max_abs_sum = labs(max_sum);
          
	      if (new_abs_sum > max_abs_sum)
	         store_new_max = IS_TRUE;
	   }
	   else
	   {
	      if (new_sum > max_sum)
	         store_new_max = IS_TRUE; 
	   }	             
	          
	   if (store_new_max)
	   { 
			cast_handle->maxSum1_4.sum_scale_val = new_sum;
			cast_handle->maxSum1_4.index = i_element_num;
	        return(OKAY);
	   }
	}

	/* Otherwise, need to loop through the elements to find max sum */
	max_sum = 0L;
	cast_handle->maxSum1_4.sum_scale_val = max_sum;
	cast_handle->maxSum1_4.index = 0;
	
	for (index = 0; index < cast_handle->total_num_elements; index++)
	{
	   new_sum = (long)(((cast_handle->factor_1_to_4) *
	                     (cast_handle->element_params)[index][1]) +
	                     (cast_handle->element_params)[index][4]);

	   store_new_max = IS_FALSE;
	   
	   if (cast_handle->maxSum1_4.abs_flag)
	   {
	      new_abs_sum = labs(new_sum);
          max_abs_sum = labs(max_sum);
          
	      if (new_abs_sum > max_abs_sum)
	         store_new_max = IS_TRUE;
	   }
	   else
	   {
	      if (new_sum > max_sum)
	         store_new_max = IS_TRUE; 
	   }

	   if (store_new_max)
	   {
	      max_sum = new_sum;
	      cast_handle->maxSum1_4.sum_scale_val = new_sum;
		  cast_handle->maxSum1_4.index = index;
	      
	   }
    }

	return(OKAY);
} 

/*
 * FUNCTION: valsSetAppDependentInt()
 * DESCRIPTION:
 *   Store the passed application dependent int into the passed handle.
 */
int PT_DECLSPEC valsSetAppDependentInt(PT_HANDLE *hp_vals, int i_index, int i_value)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Make sure the array is allocated */
	if (i_index >= cast_handle->app_depend.num_ints)
	   return(NOT_OKAY);

	/* Set the value */
	cast_handle->app_depend.int_vals[i_index] = i_value;

	return(OKAY);
}  

/*
 * FUNCTION: valsSetAppDependentReal()
 * DESCRIPTION:
 *   Store the passed application dependent real into the passed handle.
 */
int PT_DECLSPEC valsSetAppDependentReal(PT_HANDLE *hp_vals, int i_index, realtype r_value)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Make sure the array is allocated */
	if (i_index >= cast_handle->app_depend.num_reals)
	   return(NOT_OKAY);

	/* Set the value */
	cast_handle->app_depend.real_vals[i_index] = r_value;

	return(OKAY);
}

/*
 * FUNCTION: valsSetAppDependentString()
 * DESCRIPTION:
 *   Store the passed application dependent string into the passed handle.
 */
int PT_DECLSPEC valsSetAppDependentString(PT_HANDLE *hp_vals, int i_index, wchar_t *wcp_string)
{
	struct valsHdlType *cast_handle;
	int length;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Make sure the array is allocated */
	if (i_index >= cast_handle->app_depend.num_strings)
	   return(NOT_OKAY);
	   
	/* Free the old string if it has been previously allocated */
	if (cast_handle->app_depend.wcpp_strings[i_index] != NULL)
		free(cast_handle->app_depend.wcpp_strings[i_index]);

	if (wcp_string == NULL)
	{
		cast_handle->app_depend.wcpp_strings[i_index] = NULL;
		return(OKAY);
	}

	/* Allocate the string */
	length = (int)wcslen(wcp_string);
	cast_handle->app_depend.wcpp_strings[i_index] = (wchar_t *)calloc(1, ((length+1)*sizeof(wchar_t)));
	if (cast_handle->app_depend.wcpp_strings[i_index] == NULL)
		return(NOT_OKAY);

	/* Set the string */
	swprintf(cast_handle->app_depend.wcpp_strings[i_index], L"%s", wcp_string);

	return(OKAY);
}

/*
 * FUNCTION: valsSetGraphicEq()
 * DESCRIPTION:
 */
int PT_DECLSPEC valsSetGraphicEq(PT_HANDLE *hp_vals, PT_HANDLE *hp_graphicEq, int i_eq_on)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	int i_num_bands;
	realtype r_boost_cut;
	realtype r_center_freq;
	int i_band_num;

	/* Free up the old graphicEq in handle if it exists */
	if (cast_handle->hp_graphicEq != NULL)
	{
		if (cast_handle->i_trace_mode)
		{
			swprintf(cast_handle->wcp_msg1, L"valsSetGraphicEq(): Calling GraphicEqFreeUp()");
			(cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
		}

		if (GraphicEqFreeUp(&cast_handle->hp_graphicEq) != OKAY)
			return(NOT_OKAY);
	}

	/* Get the number of bands */
	if (GraphicEqGetNumBands(hp_graphicEq, &i_num_bands) != OKAY)
		return(NOT_OKAY);

	/* Create the new copy of the of Graphic Eq handle in the vals handle */
	if (GraphicEqNew(&cast_handle->hp_graphicEq, i_num_bands, cast_handle->i_trace_mode, cast_handle->slout_hdl) != OKAY)
		return(NOT_OKAY);

	/* Copy all the band info */
	for (i_band_num = 1; i_band_num <= i_num_bands; i_band_num++)
	{
		if (GraphicEqGetBandBoostCut(hp_graphicEq, i_band_num, &r_boost_cut) != OKAY)
			return(NOT_OKAY);
		if (GraphicEqGetBandCenterFrequency(hp_graphicEq, i_band_num, &r_center_freq) != OKAY)
			return(NOT_OKAY);

		if (GraphicEqSetBandBoostCut(cast_handle->hp_graphicEq, i_band_num, r_boost_cut) != OKAY)
			return(NOT_OKAY);
		if (GraphicEqSetBandFreq(cast_handle->hp_graphicEq, i_band_num, r_center_freq) != OKAY)
			return(NOT_OKAY);
	}

	/* Store the on/off flag */
	cast_handle->i_eq_on = i_eq_on;

	return(OKAY);
}

/*
 * FUNCTION: valsSetGraphicEqOn()
 * DESCRIPTION:
 */
int PT_DECLSPEC valsSetGraphicEqOn(PT_HANDLE *hp_vals, int i_eq_on)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->i_eq_on = i_eq_on;

	return(OKAY);
}