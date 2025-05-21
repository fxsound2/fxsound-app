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

/*
 * FUNCTION: valsGetFileVersion()
 * DESCRIPTION:
 *   Get the version of the file.
 */
int PT_DECLSPEC valsGetFileVersion(PT_HANDLE *hp_vals, realtype *rp_file_version)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*rp_file_version = cast_handle->file_version;

	return(OKAY);
}

/*
 * FUNCTION: valsGetTotalNumElements()
 * DESCRIPTION:
 *   Get total number of elements possible.
 */
int PT_DECLSPEC valsGetTotalNumElements(PT_HANDLE *hp_vals, int *ip_num_elements)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_num_elements = cast_handle->total_num_elements;

	return(OKAY);
}

/*
 * FUNCTION: valsGetMaxElement()
 * DESCRIPTION:
 *   Get the element number of the element which has the highest value for the 
 *   passed main parameter value.
 */
int PT_DECLSPEC valsGetMaxElement(PT_HANDLE *hp_vals, int i_param_num, int *ip_max_element)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

    if ((i_param_num < 0) || (i_param_num >= VALS_NUM_ELEMENT_PARAMS))
       return(NOT_OKAY);

	/* Make sure it is being kept track of */
	if ((cast_handle->max[i_param_num]).calc_flag != IS_TRUE)
	   return(NOT_OKAY);

	*ip_max_element = (cast_handle->max[i_param_num]).index;

	return(OKAY);
}

/*
 * FUNCTION: valsGetMaxValue()
 * DESCRIPTION:
 *   Get the max value of the passed parameter.
 */
int PT_DECLSPEC valsGetMaxValue(PT_HANDLE *hp_vals, int i_param_num, int *ip_max)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);
  
    if ((i_param_num < 0) || (i_param_num >= VALS_NUM_ELEMENT_PARAMS))
       return(NOT_OKAY);

	/* Make sure it is being kept track of */
	if ((cast_handle->max[i_param_num]).calc_flag != IS_TRUE)
	   return(NOT_OKAY);

	*ip_max = (cast_handle->max[i_param_num]).value;

	return(OKAY);
}

/*
 * FUNCTION: valsGetMax1and4Element()
 * DESCRIPTION:
 *   Get the element number of the element which has the greatest sum of main parameters
 *   1 and 4.
 */
int PT_DECLSPEC valsGetMax1and4Element(PT_HANDLE *hp_vals, int *ip_max_element)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Make sure it is being kept track of */
	if (cast_handle->maxSum1_4.calc_flag != IS_TRUE)
	   return(NOT_OKAY);

	*ip_max_element = cast_handle->maxSum1_4.index;

	return(OKAY);
}

/*
 * FUNCTION: valsGetComment()
 * DESCRIPTION:
 *   Get the comment that the file was saved.
 */
int PT_DECLSPEC valsGetComment(PT_HANDLE *hp_vals, wchar_t **wcpp_comment)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*wcpp_comment = cast_handle->wcp_comment;

	return(OKAY);
}

/*
 * FUNCTION: valsGetMainParamValue()
 * DESCRIPTION:
 *   Get the value of the passed main parameter.
 */
int PT_DECLSPEC valsGetMainParamValue(PT_HANDLE *hp_vals, int i_param_num, int *ip_value)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if ((i_param_num < 0) || (i_param_num >= VALS_NUM_MAIN_PARAMS))
	   return(NOT_OKAY);
	   
	if ((cast_handle->double_params) && 
	    (cast_handle->param_set == VALS_PARAM_SET_2))
	   *ip_value = (cast_handle->main_params_2)[i_param_num];
	else
	   *ip_value = (cast_handle->main_params)[i_param_num];

	return(OKAY);
} 

/*
 * FUNCTION: valsGetElementParamValue()
 * DESCRIPTION:
 *   Get the value of the passed element parameter.
 */
int PT_DECLSPEC valsGetElementParamValue(PT_HANDLE *hp_vals, int i_element_num, 
                             int i_param_num, int *ip_value)
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
	   *ip_value = (cast_handle->element_params_2)[i_element_num][i_param_num];
	else	    
	   *ip_value = (cast_handle->element_params)[i_element_num][i_param_num];

	return(OKAY);
} 


/*
 * FUNCTION: valsGetNumAppDependentVars()
 * DESCRIPTION:
 *   Get the number of application dependent variables.
 */
int PT_DECLSPEC valsGetNumAppDependentVars(PT_HANDLE *hp_vals, int *ip_num_ints, 
                               int *ip_num_reals, int *ip_num_strings)
{
   struct valsHdlType *cast_handle;

   cast_handle = (struct valsHdlType *)hp_vals;

   if (cast_handle == NULL)
      return(NOT_OKAY);
      
   *ip_num_ints = cast_handle->app_depend.num_ints;
   *ip_num_reals = cast_handle->app_depend.num_reals;  
   *ip_num_strings = cast_handle->app_depend.num_reals;
		
   return(OKAY);
}

/*
 * FUNCTION: valsGetAppDependentInt()
 * DESCRIPTION:
 *   Get the passed application dependent int value
 */
int PT_DECLSPEC valsGetAppDependentInt(PT_HANDLE *hp_vals, int i_index, int *ip_value)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (i_index >= cast_handle->app_depend.num_ints)
	   return(NOT_OKAY);
	   
	*ip_value = cast_handle->app_depend.int_vals[i_index];

	return(OKAY);
}  

/*
 * FUNCTION: valsGetAppDependentReal()
 * DESCRIPTION:
 *   Get the passed application dependent real value
 */
int PT_DECLSPEC valsGetAppDependentReal(PT_HANDLE *hp_vals, int i_index, realtype *rp_value)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (i_index >= cast_handle->app_depend.num_reals)
	   return(NOT_OKAY);
	   
	*rp_value = cast_handle->app_depend.real_vals[i_index];

	return(OKAY);
}  

/*
 * FUNCTION: valsGetAppDependentString()
 * DESCRIPTION:
 *   Get the passed application dependent string
 *   Note: This function only sets the passed pointer to point to the string.
 */
int PT_DECLSPEC valsGetAppDependentString(PT_HANDLE *hp_vals, int i_index, 
														wchar_t **wcpp_string)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (i_index >= cast_handle->app_depend.num_strings)
	   return(NOT_OKAY);
	   
	*wcpp_string = cast_handle->app_depend.wcpp_strings[i_index];

	return(OKAY);
}

/*
 * FUNCTION: valsGetDoubleSetType()
 * DESCRIPTION:
 *   Passes back if the passed vals handle is a Double Set one.
 */
int PT_DECLSPEC valsGetDoubleSetType(PT_HANDLE *hp_vals, int *ip_double_set)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);
    
    *ip_double_set = cast_handle->double_params;
   
    return(OKAY);
}    

/*
 * FUNCTION: valsGetParamSet()
 * DESCRIPTION:
 *   Gets which parameter set will be operated on.  This only makes 
 *   sense if the vals is a "double_params" type.  The returned value
 *   can be either VALS_PARAM_SET_1 or VALS_PARAM_SET_2.
 */
int PT_DECLSPEC valsGetParamSet(PT_HANDLE *hp_vals, int *ip_param_set)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);
    
    *ip_param_set = cast_handle->param_set;
   
    return(OKAY);
}  

/*
 * FUNCTION: valsGetGraphicEq()
 * DESCRIPTION:
 *
 */
int PT_DECLSPEC valsGetGraphicEq(PT_HANDLE *hp_vals, PT_HANDLE **hpp_graphicEq, int *ip_eq_on)
{
	struct valsHdlType *cast_handle;

	cast_handle = (struct valsHdlType *)hp_vals;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*hpp_graphicEq = cast_handle->hp_graphicEq;

	*ip_eq_on = cast_handle->i_eq_on;

	return(OKAY);
}