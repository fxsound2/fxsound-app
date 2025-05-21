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
 * FUNCTION: valsInit()
 * DESCRIPTION:
 *  Allocates and initializes the passed vals handle to have the
 *  passed total number of possible elements.
 *  The passed double_params flag says if there are 2 pairs of parameters.
 */
int PT_DECLSPEC valsInit(PT_HANDLE **hpp_vals, CSlout *hp_slout, int i_num_elements, 
             realtype r_version, int i_double_params)
{
	struct valsHdlType *cast_handle;
	int element_index;
	int param_index;

	/* Allocate the handle */
	cast_handle = (struct valsHdlType *)calloc(1,
						sizeof(struct valsHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->slout_hdl = hp_slout;
	cast_handle->total_num_elements = i_num_elements;

	/* Initialize file info */
   cast_handle->file_version = r_version;
	cast_handle->wcp_comment = NULL; 
	
    /* Initialize the double params mode */
    cast_handle->double_params = i_double_params;  
    cast_handle->param_set = VALS_PARAM_SET_1;
    
	/* Initialize the main values */
	for (param_index = 0; param_index < VALS_NUM_MAIN_PARAMS; param_index++)
	   cast_handle->main_params[param_index] = 0;
    if (cast_handle->double_params)
    {
	   for (param_index = 0; param_index < VALS_NUM_MAIN_PARAMS; param_index++)
	      cast_handle->main_params_2[param_index] = 0;  
    }  
      
	/* Initialize the individual elements */
	for (element_index = 0; element_index < i_num_elements; element_index++)
	{
		/* Initalize each element paramter */
		for (param_index = 0; param_index < VALS_NUM_ELEMENT_PARAMS; 
		     param_index++)
		{
		   cast_handle->element_params[element_index][param_index] = 0; 
		   if (cast_handle->double_params)
		      cast_handle->element_params_2[element_index][param_index] = 0;     
		}
	}
    
    /* Initialize the max info */
    for (param_index = 0; param_index < VALS_NUM_ELEMENT_PARAMS; param_index++)
    {
       (cast_handle->max[param_index]).calc_flag = IS_FALSE;
       (cast_handle->max[param_index]).abs_flag = IS_FALSE;
       (cast_handle->max[param_index]).value = 0;
       (cast_handle->max[param_index]).index = 0;
    }
    
	/* Initalize max sum of 1 and 4 info */
	(cast_handle->maxSum1_4).calc_flag = IS_FALSE;
	(cast_handle->maxSum1_4).abs_flag = IS_FALSE;
	(cast_handle->maxSum1_4).sum_scale_val = 0L;
	(cast_handle->maxSum1_4).index = 0;  
	cast_handle->factor_1_to_4 = 1;
	 
	/* Initialize the app dependent info */
	if (valsInitAppDependentInfo((PT_HANDLE *)cast_handle, 0, 0, 0) != OKAY)
	   return(NOT_OKAY);
	
	cast_handle->hp_graphicEq = NULL;

	*hpp_vals = (PT_HANDLE *)cast_handle;

	return(OKAY);
} 

/*
 * FUNCTION: valsInitAppDependentInfo()
 * DESCRIPTION:
 *   Allocates and initializes the application dependent part of the passed handle.    
 */
int PT_DECLSPEC valsInitAppDependentInfo(PT_HANDLE *hp_vals, int i_num_ints, int i_num_reals, 
													  int i_num_strings)
{
   struct valsHdlType *cast_handle;
    
   cast_handle = (struct valsHdlType *)(hp_vals);

   /* Allocate the ints */
   cast_handle->app_depend.num_ints = i_num_ints;  
   
   if (i_num_ints > 0)
   {
      cast_handle->app_depend.int_vals = (int *)calloc(i_num_ints, sizeof(int));
      if (cast_handle->app_depend.int_vals == NULL)
         return(NOT_OKAY);
   }
   else
      cast_handle->app_depend.int_vals = NULL; 
   
   /* Allocate the reals */
   cast_handle->app_depend.num_reals = i_num_reals;
   if (i_num_reals > 0)
   {
      cast_handle->app_depend.real_vals = (realtype *)calloc(i_num_reals, sizeof(realtype));
      if (cast_handle->app_depend.real_vals == NULL)
         return(NOT_OKAY);   
   }
   else
      cast_handle->app_depend.real_vals = NULL;    
   
   /* Allocate the strings */
   cast_handle->app_depend.num_strings = i_num_strings;
   if (i_num_strings > 0)
   {
      cast_handle->app_depend.wcpp_strings = (wchar_t **)calloc(i_num_strings, sizeof(wchar_t *));
      if (cast_handle->app_depend.wcpp_strings == NULL)
         return(NOT_OKAY);
   }  
   else
      cast_handle->app_depend.wcpp_strings = NULL;      
   
   return(OKAY);
}

/*
 * FUNCTION: valsResetNumAppDependentVars()
 * DESCRIPTION:
 *   Reallocates the application dependent part of the passed handle.  If an
 *   array gets bigger, it maintains the old portion of the array. 
 */
int PT_DECLSPEC valsResetNumAppDependentVars(PT_HANDLE *hp_vals, int i_num_ints, int i_num_reals, 
                                 int i_num_strings)
{
   struct valsHdlType *cast_handle;
   int index;
    
   cast_handle = (struct valsHdlType *)(hp_vals);  
   
   if (cast_handle == NULL)
      return(NOT_OKAY);   

   /* Check if need to reallocate the ints */
   if (cast_handle->app_depend.num_ints != i_num_ints)
   {
      if (i_num_ints == 0)
      {
         if (cast_handle->app_depend.int_vals != NULL)
         { 
            free(cast_handle->app_depend.int_vals);
            cast_handle->app_depend.int_vals = NULL;
         }
      } 
      else
      {
         cast_handle->app_depend.int_vals = 
            (int *)realloc(cast_handle->app_depend.int_vals, 
                              (i_num_ints * sizeof(int)));
         if (cast_handle->app_depend.int_vals == NULL)
            return(NOT_OKAY);
      }
      cast_handle->app_depend.num_ints = i_num_ints;
   }
    
   /* Check if need to reallocate the reals */
   if (cast_handle->app_depend.num_reals != i_num_reals)
   {
      if (i_num_reals == 0)
      {
         if (cast_handle->app_depend.real_vals != NULL)
         {   
            free(cast_handle->app_depend.real_vals);
            cast_handle->app_depend.real_vals = NULL;
         }
      } 
      else
      {
         cast_handle->app_depend.real_vals = 
            (realtype *)realloc(cast_handle->app_depend.real_vals, 
                              (i_num_reals * sizeof(realtype)));      
         if (cast_handle->app_depend.real_vals == NULL)
            return(NOT_OKAY);
      }
      cast_handle->app_depend.num_reals = i_num_reals;
   }
   
   /* Check if need to reallocate the strings */
   if (cast_handle->app_depend.num_strings != i_num_strings)
   {
      if (i_num_strings == 0)
      {
         if (cast_handle->app_depend.wcpp_strings != NULL)
         {
            if (cast_handle->app_depend.num_strings > 0)
            {
               for (index = 0; index < cast_handle->app_depend.num_strings; index++)       
                  free(cast_handle->app_depend.wcpp_strings[index]); 
            }
            
            free(cast_handle->app_depend.wcpp_strings);
            cast_handle->app_depend.wcpp_strings = NULL;
         }
      } 
      else
      {
         cast_handle->app_depend.wcpp_strings = 
            (wchar_t **)realloc(cast_handle->app_depend.wcpp_strings, 
                              (i_num_strings * sizeof(wchar_t *)));            
         if (cast_handle->app_depend.wcpp_strings == NULL)
            return(NOT_OKAY);
      }
      cast_handle->app_depend.num_strings = i_num_strings;
   }           

   return(OKAY);
}

/*
 * FUNCTION: valsSetMaintainMax()
 * DESCRIPTION:
 *   Sets that the handle should keep track of which element has the max for the passed
 *   param.  The abs_flag says if max should be based on absolute value.
 */
int PT_DECLSPEC valsSetMaintainMax(PT_HANDLE *hp_vals, int i_param_num, int i_abs_flag)
{
   struct valsHdlType *cast_handle;

   cast_handle = (struct valsHdlType *)(hp_vals);

   if (cast_handle == NULL)
      return(NOT_OKAY); 
   
   if ((i_param_num < 0) || (i_param_num >= VALS_NUM_ELEMENT_PARAMS))
      return(NOT_OKAY);
      
   (cast_handle->max[i_param_num]).calc_flag = IS_TRUE;
   (cast_handle->max[i_param_num]).abs_flag = i_abs_flag;

   /* Calculate which element has the max */
   if (vals_CalcNewMax((PT_HANDLE *)cast_handle, i_param_num, -1) != OKAY)
     return(NOT_OKAY);
   
   return(OKAY);
}  

/*
 * FUNCTION: valsSetMaintainMax1and4()
 * DESCRIPTION:
 *   Sets that the handle should keep track of which element has the max sum of element
 *   parameters 1 and 4.  
 *   
 *   The abs_flag says if max should be based on absolute value.
 *   
 *   Note: This is a special case for the delay application which needs to keep track of
 *         which element has the biggest total delay (coarse to fine).
 */
int PT_DECLSPEC valsSetMaintainMax1and4(PT_HANDLE *hp_vals, int i_abs_flag, int i_factor_1_to_4)
{
   struct valsHdlType *cast_handle;

   cast_handle = (struct valsHdlType *)(hp_vals);

   if (cast_handle == NULL)
      return(NOT_OKAY); 

   cast_handle->maxSum1_4.calc_flag = IS_TRUE;
   cast_handle->maxSum1_4.abs_flag = i_abs_flag;
   cast_handle->maxSum1_4.sum_scale_val = 0L;
   cast_handle->maxSum1_4.index = 0;
   cast_handle->factor_1_to_4 = i_factor_1_to_4;
   
   	/* Calculate the max sum of elements params 1 and 4 */
	if (vals_CalcNewMaxSum1and4((PT_HANDLE *)cast_handle, -1) != OKAY)
	   return(NOT_OKAY);
   
   return(OKAY);
}
  
/*
 * FUNCTION: valsCopy()
 * DESCRIPTION:
 *  Allocates and initializes the passed hpp_to_vals handle to have be
 *  a copy of the passed hp_from_vals handle.
 */
int PT_DECLSPEC valsCopy(PT_HANDLE *hp_from_vals, PT_HANDLE **hpp_to_vals)
{
   struct valsHdlType *cast_to_hdl;
   struct valsHdlType *cast_from_hdl;
   int param_index;
   int element_index;
   int length;
   int index;

   if (hp_from_vals == NULL)
   {
      *hpp_to_vals = NULL;
	  return(OKAY);
   }

   /* Cast the from delay */
   cast_from_hdl = (struct valsHdlType *)(hp_from_vals);

   /* Allocate the new handle */
   cast_to_hdl = (struct valsHdlType *)calloc(1,
						sizeof(struct valsHdlType));
   if (cast_to_hdl == NULL)
      return(NOT_OKAY);

   /* Copy the global info */
   cast_to_hdl->slout_hdl = cast_from_hdl->slout_hdl;
   cast_to_hdl->total_num_elements = cast_from_hdl->total_num_elements; 
   cast_to_hdl->file_version = cast_from_hdl->file_version; 
   cast_to_hdl->double_params = cast_from_hdl->double_params;
   cast_to_hdl->param_set = cast_from_hdl->param_set;            

   /* Allocate and copy the comment */
   if (cast_from_hdl->wcp_comment == NULL)
      cast_to_hdl->wcp_comment = NULL;
   else
   {
      length = (int)wcslen(cast_from_hdl->wcp_comment);
	  cast_to_hdl->wcp_comment = (wchar_t *)calloc(1,
						((length+1)*sizeof(wchar_t)));
	  if (cast_to_hdl->wcp_comment == NULL)
	     return(NOT_OKAY);

      /* Set the string */
	  swprintf(cast_to_hdl->wcp_comment, L"%s", cast_from_hdl->wcp_comment);
   }

   /* Copy the main params */
   for (param_index = 0; param_index < VALS_NUM_MAIN_PARAMS; param_index++)
   {
      cast_to_hdl->main_params[param_index] = 
	     cast_from_hdl->main_params[param_index];
      
      if (cast_from_hdl->double_params)
      {
         cast_to_hdl->main_params_2[param_index] = 
	        cast_from_hdl->main_params_2[param_index];      
      }
   }
	
   /* Copy the element params */
   for (element_index = 0; element_index < cast_from_hdl->total_num_elements; 
	    element_index++)
   {
      for (param_index = 0; param_index < VALS_NUM_ELEMENT_PARAMS; 
		   param_index++)
      {
	     cast_to_hdl->element_params[element_index][param_index] = 
		      cast_from_hdl->element_params[element_index][param_index];
		      
		 if (cast_from_hdl->double_params)
		 {
		    cast_to_hdl->element_params_2[element_index][param_index] = 
		       cast_from_hdl->element_params_2[element_index][param_index];
		 }
	  }
   } 
  
   /* Copy the max info */
   for (param_index = 0; param_index < VALS_NUM_ELEMENT_PARAMS; param_index++)
   {
      (cast_to_hdl->max[param_index]).calc_flag = 
         (cast_from_hdl->max[param_index]).calc_flag;      
      (cast_to_hdl->max[param_index]).abs_flag = 
         (cast_from_hdl->max[param_index]).abs_flag;      
      (cast_to_hdl->max[param_index]).value = (cast_from_hdl->max[param_index]).value;           
      (cast_to_hdl->max[param_index]).sum_scale_val = (cast_from_hdl->max[param_index]).sum_scale_val;            
      (cast_to_hdl->max[param_index]).index = (cast_from_hdl->max[param_index]).index;      
   } 

   /* Copy the max sum of 1 and 4 info */
   cast_to_hdl->maxSum1_4.calc_flag = cast_from_hdl->maxSum1_4.calc_flag;
   cast_to_hdl->maxSum1_4.abs_flag = cast_from_hdl->maxSum1_4.abs_flag; 
   cast_to_hdl->maxSum1_4.value = cast_from_hdl->maxSum1_4.value;
   cast_to_hdl->maxSum1_4.sum_scale_val = cast_from_hdl->maxSum1_4.sum_scale_val;   
   cast_to_hdl->maxSum1_4.index = cast_from_hdl->maxSum1_4.index; 
   cast_to_hdl->factor_1_to_4 = cast_from_hdl->factor_1_to_4;
   
   /* Allocate the app dependent info */ 
   if (valsInitAppDependentInfo((PT_HANDLE *)cast_to_hdl, 
                                    cast_from_hdl->app_depend.num_ints,
                                    cast_from_hdl->app_depend.num_reals,
                                    cast_from_hdl->app_depend.num_strings) != OKAY)
      return(NOT_OKAY);
   
   /* Copy the app dependent info */
   for (index = 0; index < cast_to_hdl->app_depend.num_ints; index++)
      cast_to_hdl->app_depend.int_vals[index] = 
         cast_from_hdl->app_depend.int_vals[index];
 
   for (index = 0; index < cast_to_hdl->app_depend.num_reals; index++)
      cast_to_hdl->app_depend.real_vals[index] = 
         cast_from_hdl->app_depend.real_vals[index];

   for (index = 0; index < cast_to_hdl->app_depend.num_strings; index++)
   {   
      if (cast_from_hdl->app_depend.wcpp_strings[index] != NULL)
      {
         if (valsSetAppDependentString((PT_HANDLE *)cast_to_hdl, index, 
                                       cast_from_hdl->app_depend.wcpp_strings[index]) != OKAY)
            return(NOT_OKAY);
      }
   }
   
   *hpp_to_vals = (PT_HANDLE *)cast_to_hdl;

   return(OKAY);
} 


/*
* FUNCTION: valsCompare()
* DESCRIPTION:
*  Compare the two passed vals handles to see if they have the same settings.
*  Passes back IS_TRUE if they are different, and IS_FALSE if they are the same.
*/
int PT_DECLSPEC valsCompare(PT_HANDLE *hp_vals1, PT_HANDLE *hp_vals2, int *ip_different)
{
	struct valsHdlType *cast_handle_1;
	struct valsHdlType *cast_handle_2;
	int param_index;
	int element_index;
	int index;

	*ip_different = IS_TRUE;

	if (hp_vals1 == NULL)
		return(OKAY);

	if (hp_vals2 == NULL)
		return(OKAY);

	/* Cast the handles */
	cast_handle_1 = (struct valsHdlType *)(hp_vals1);
	cast_handle_2 = (struct valsHdlType *)(hp_vals2);

	/* Compare the handles */
	if ((cast_handle_1->total_num_elements) != (cast_handle_2->total_num_elements))
		return(OKAY);

	if ((cast_handle_1->double_params) != (cast_handle_2->double_params))
		return(OKAY);

	if ((cast_handle_1->param_set) != (cast_handle_2->param_set))
		return(OKAY);

	/* Compare the main params */
	for (param_index = 0; param_index < VALS_NUM_MAIN_PARAMS; param_index++)
	{
		if ((cast_handle_1->main_params[param_index]) != (cast_handle_2->main_params[param_index]))
			return(OKAY);

		if (cast_handle_1->double_params)
		{
			if (cast_handle_1->main_params_2[param_index] != cast_handle_2->main_params_2[param_index])
			return(OKAY);
		}
	}

	/* Compare the element params */
	for (element_index = 0; element_index < cast_handle_1->total_num_elements; element_index++)
	{
		for (param_index = 0; param_index < VALS_NUM_ELEMENT_PARAMS; param_index++)
		{
			if ((cast_handle_1->element_params[element_index][param_index]) != (cast_handle_2->element_params[element_index][param_index]))
				return(OKAY);

			if (cast_handle_1->double_params)
			{
				if ((cast_handle_1->element_params_2[element_index][param_index]) != (cast_handle_2->element_params_2[element_index][param_index]))
					return(OKAY);
			}
		}
	}

	/* Compare the app dependent info */
	if ((cast_handle_1->app_depend.num_ints) != (cast_handle_2->app_depend.num_ints))
		return(OKAY);

	for (index = 0; index < cast_handle_1->app_depend.num_ints; index++)
	{
		if ((cast_handle_1->app_depend.int_vals[index]) != (cast_handle_2->app_depend.int_vals[index]))
			return(OKAY);
	}

	if ((cast_handle_1->app_depend.num_reals) && (cast_handle_2->app_depend.num_reals))
		return(OKAY);

	for (index = 0; index < cast_handle_1->app_depend.num_reals; index++)
	{
		if ((cast_handle_1->app_depend.real_vals[index]) != (cast_handle_2->app_depend.real_vals[index]))
			return(OKAY);
	}

	*ip_different = IS_FALSE;

	return(OKAY);
}

/*
 * FUNCTION: valsFreeUp()
 * DESCRIPTION:
 *   Frees the passed vals handle and sets to NULL.
 */
int PT_DECLSPEC valsFreeUp(PT_HANDLE **hpp_vals)
{
	struct valsHdlType *cast_handle;
    int index;
    
	cast_handle = (struct valsHdlType *)(*hpp_vals);

	if (cast_handle == NULL)
		return(OKAY);

	if (cast_handle->wcp_comment != NULL)
		free(cast_handle->wcp_comment);
		
    if (cast_handle->app_depend.num_ints > 0)
       free(cast_handle->app_depend.int_vals);

    if (cast_handle->app_depend.num_reals > 0)
       free(cast_handle->app_depend.real_vals);
 
    if (cast_handle->app_depend.num_strings > 0)
    {
       for (index = 0; index < cast_handle->app_depend.num_strings; index++)       
          free(cast_handle->app_depend.wcpp_strings[index]); 
       free(cast_handle->app_depend.wcpp_strings);
    }
    
	/* Free up the graphicEq in handle if it exists */
	if (cast_handle->hp_graphicEq != NULL)
	{
		if (cast_handle->i_trace_mode)
		{
			swprintf(cast_handle->wcp_msg1, L"valsFreeUp(): Calling GraphicEqFreeUp()");
			(cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
		}

		if (GraphicEqFreeUp(&cast_handle->hp_graphicEq) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle != NULL)
		free(cast_handle);

	*hpp_vals = NULL;

	return(OKAY);
}

/*
 * FUNCTION: valsDump()
 * DESCRIPTION:
 *   Dump the passed vals handle to the screen.
 */
int PT_DECLSPEC valsDump(PT_HANDLE *hp_vals)
{
	struct valsHdlType *cast_handle;
	int param_index;
    int element_index;
    int index;
    int int_val;
    realtype real_val;
    wchar_t *wcp_string_val;
    
	cast_handle = (struct valsHdlType *)(hp_vals);

	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, L"VALS HDL:\n");

	if (cast_handle == NULL)
	{
		cast_handle->slout_hdl->Message_Wide(FIRST_LINE, L"Vals Handle is NULL\n");
		return(OKAY);
	}

	if (cast_handle->slout_hdl == NULL)
       return(NOT_OKAY);

	swprintf(cast_handle->wcp_msg1, L"Num Elements = %d\n", cast_handle->total_num_elements);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	
	swprintf(cast_handle->wcp_msg1, L"File Version = %g\n", cast_handle->file_version);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);	

	if (cast_handle->wcp_comment == NULL)
       swprintf(cast_handle->wcp_msg1, L"Comment is NULL\n");
	else
	   swprintf(cast_handle->wcp_msg1, L"Comment: %s\n", cast_handle->wcp_comment);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
 
	swprintf(cast_handle->wcp_msg1, L"Double Params = %d\n", cast_handle->double_params);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);	 
	
	swprintf(cast_handle->wcp_msg1, L"Param Set = %d\n", cast_handle->param_set);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);	 	
 
	/* Display the main values */
	for (param_index = 0; param_index < VALS_NUM_MAIN_PARAMS; param_index++)
	{
	   swprintf(cast_handle->wcp_msg1, L"MainParam[%d] = %d\n", param_index, 
	           cast_handle->main_params[param_index]);
       cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);  
       
       if (cast_handle->double_params)
       {
          swprintf(cast_handle->wcp_msg1, L"MainParam_2[%d] = %d\n", param_index, 
	              cast_handle->main_params_2[param_index]);
          cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);  
       }
    }
    
	/* Display the individual elements */
	for (element_index = 0; element_index < cast_handle->total_num_elements;
	    element_index++)
	{
		for (param_index = 0; param_index < VALS_NUM_ELEMENT_PARAMS; 
		     param_index++)
		{
		   swprintf(cast_handle->wcp_msg1, L"ElementParam[%d][%d] = %d\n", element_index, 
		           param_index, cast_handle->element_params[element_index][param_index]);
		   cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	       
	       if (cast_handle->double_params)
	       {
		      swprintf(cast_handle->wcp_msg1, L"ElementParam_2[%d][%d] = %d\n", element_index, 
		              param_index, cast_handle->element_params_2[element_index][param_index]);
		      cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);	      
	       }
		}
	}
	
    /* Display the max info */
	for (param_index = 0; param_index < VALS_NUM_ELEMENT_PARAMS; param_index++)
	{
	   swprintf(cast_handle->wcp_msg1, L"Max[%d].calc_flag = %d\n", param_index, 
	      (cast_handle->max[param_index]).calc_flag);
	   cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);

	   swprintf(cast_handle->wcp_msg1, L"Max[%d].abs_flag = %d\n", param_index, 
	      (cast_handle->max[param_index]).abs_flag);
	   cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);	
	  
	   swprintf(cast_handle->wcp_msg1, L"Max[%d].value = %d\n", param_index, 
	      (cast_handle->max[param_index]).value);
	   cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	  	
	   swprintf(cast_handle->wcp_msg1, L"Max[%d].index = %d\n", param_index, 
	      (cast_handle->max[param_index]).index);
       cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
    }
    
    /* Display max sum of 1 and 4 info */
    swprintf(cast_handle->wcp_msg1, L"maxSum1_4.calc_flag = %d\n", 
            cast_handle->maxSum1_4.calc_flag);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);

    swprintf(cast_handle->wcp_msg1, L"maxSum1_4.abs_flag = %d\n", 
            cast_handle->maxSum1_4.abs_flag);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
    
    swprintf(cast_handle->wcp_msg1, L"maxSum1_4.sum_scale_val = %ld\n", 
            cast_handle->maxSum1_4.sum_scale_val);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);

    swprintf(cast_handle->wcp_msg1, L"maxSum1_4.index = %d\n", 
            cast_handle->maxSum1_4.index);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
    swprintf(cast_handle->wcp_msg1, L"factor_1_to_4 = %d\n", 
            cast_handle->factor_1_to_4);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);

    /* Display the app dependent info */
    swprintf(cast_handle->wcp_msg1, L"Number of App. Dependent ints: %d\n",
            cast_handle->app_depend.num_ints);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
    for (index = 0; index < cast_handle->app_depend.num_ints; index++)
    {
       if (valsGetAppDependentInt((PT_HANDLE *)cast_handle, index, &int_val) != OKAY)
          return(NOT_OKAY);     
       swprintf(cast_handle->wcp_msg1, L"   int_vals[%d] = %d\n", index, int_val);
       cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
    }
    
    swprintf(cast_handle->wcp_msg1, L"Number of App. Dependent reals: %d\n",
            cast_handle->app_depend.num_reals);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
    for (index = 0; index < cast_handle->app_depend.num_reals; index++)
    {
       if (valsGetAppDependentReal((PT_HANDLE *)cast_handle, index, &real_val) != OKAY)
          return(NOT_OKAY);       
       swprintf(cast_handle->wcp_msg1, L"   real_vals[%d] = %g\n", index, real_val);
       cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);   
   }
    
    swprintf(cast_handle->wcp_msg1, L"Number of App. Dependent strings: %d\n",
            cast_handle->app_depend.num_strings);
	cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
    for (index = 0; index < cast_handle->app_depend.num_strings; index++)
    {
       if (valsGetAppDependentString((PT_HANDLE *)cast_handle, index, &wcp_string_val) != OKAY)
          return(NOT_OKAY);       
       swprintf(cast_handle->wcp_msg1, L"   strings[%d] = %s\n", index, wcp_string_val);
       cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
    }    
    
	return(OKAY);
}