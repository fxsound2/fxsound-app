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
#include <direct.h>

#include "slout.h"
#include "vals.h"
#include "mry.h"
#include "file.h"
#include "prelst.h"
#include "pstr.h"
#include "pt_defs.h"
#include "u_prelst.h"

/*
 * FUNCTION: prelstCreate()
 * DESCRIPTION:
 *  Allocates and initializes the passed prelst handle.  It will look on the
 *  disk for the effects in the passed directory and add them to the handle.
 *  
 *  The passed user min, and user max values are the values that the user sees.
 *  (i.e. offset by one)
 */
int PT_DECLSPEC prelstCreate(PT_HANDLE **hpp_prelst, CSlout *hp_slout, wchar_t *wcp_factory_dir, 
				 wchar_t *wcp_user_dir, int i_user_min_index, int i_user_max_index)
{
	struct prelstHdlType *cast_handle;
	int proper_type;
	wchar_t wcp_fullpath[PT_MAX_PATH_STRLEN];
	int index;
    
	if ((wcp_factory_dir == NULL) || (wcp_user_dir == NULL))
	  return(NOT_OKAY);

	/* Allocate the handle */
	cast_handle = (struct prelstHdlType *)calloc(1,
						sizeof(struct prelstHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);  
		
	/* Store the factory directory */
	cast_handle->wcp_factory_dir = (wchar_t *)calloc(1,
						(sizeof(wchar_t) * (wcslen(wcp_factory_dir) + 1)));
	if (cast_handle->wcp_factory_dir == NULL)
		return(NOT_OKAY);
	swprintf(cast_handle->wcp_factory_dir, L"%s", wcp_factory_dir);

	/* Store the user directory */
    cast_handle->wcp_user_dir = (wchar_t *)calloc(1,
						(sizeof(wchar_t) * (wcslen(wcp_user_dir) + 1)));
	if (cast_handle->wcp_user_dir == NULL)
		return(NOT_OKAY);
	swprintf(cast_handle->wcp_user_dir, L"%s", wcp_user_dir);

	/* Store the slout */
	cast_handle->slout_hdl = hp_slout;
    
    /* Allocate the array of exist flags */
    cast_handle->exist_flags =
	   (int *)calloc((i_user_max_index + 1), sizeof(int));   
    
    /* Store the array limits */
    cast_handle->user_min_index = i_user_min_index;
    cast_handle->user_max_index = i_user_max_index;
    
    /* Loop through the files looking for presets */
    for (index = 0; index <= i_user_max_index; index++)
    {
       /* Construct path to preset */
       if (prelstConstructFullpath((PT_HANDLE *)cast_handle, index, wcp_fullpath) != OKAY)
          return(NOT_OKAY);

       /* Check if it exists and is a valid preset */
       if (valsCheckFileType(wcp_fullpath, &proper_type, cast_handle->slout_hdl) != OKAY)
          return(NOT_OKAY);
          
       if (proper_type)
          (cast_handle->exist_flags)[index] = IS_TRUE;
       else
          (cast_handle->exist_flags)[index] = IS_FALSE;           
    }

	*hpp_prelst = (PT_HANDLE *)cast_handle;

	return(OKAY);
} 

/*
 * FUNCTION: prelstGetExists()
 * DESCRIPTION:
 *   Passes back the exist flag of the passed index.
 */
int PT_DECLSPEC prelstGetExists(PT_HANDLE *hp_prelst, int i_index, int *ip_exist)
{
   struct prelstHdlType *cast_handle;

   cast_handle = (struct prelstHdlType *)(hp_prelst);
   
   if (cast_handle == NULL)
      return(NOT_OKAY);

   if ((i_index < 0) || (i_index > cast_handle->user_max_index))
      return(NOT_OKAY);
      
   *ip_exist = (cast_handle->exist_flags)[i_index];
   
   return(OKAY);
} 

/*
 * FUNCTION: prelstConstructFullpath()
 * DESCRIPTION:
 *   Fills in the passed already allocated string the fullpath to the passed preset.
 */
int PT_DECLSPEC prelstConstructFullpath(PT_HANDLE *hp_prelst, int i_index, wchar_t *wcp_fullpath)
{
   struct prelstHdlType *cast_handle;
   wchar_t wcp_filename[PT_MAX_PATH_STRLEN];
   int is_factory;

   cast_handle = (struct prelstHdlType *)(hp_prelst);
   
   if (cast_handle == NULL)
      return(NOT_OKAY);

   /* Construct the filename */
   if ((i_index < 0) || (i_index > cast_handle->user_max_index))
      return(NOT_OKAY);
   
   if (prelstConstructFilename(hp_prelst, i_index, wcp_filename) != OKAY)
      return(NOT_OKAY);

   /* Figure out if it is a factory preset */
   if (prelstAskIsFactory(hp_prelst, i_index, &is_factory) != OKAY)
	   return(NOT_OKAY);
   if (is_factory)
      swprintf(wcp_fullpath, L"%s\\%s", cast_handle->wcp_factory_dir, wcp_filename);
   else
      swprintf(wcp_fullpath, L"%s\\%s", cast_handle->wcp_user_dir, wcp_filename);
   
   return(OKAY);
} 

/*
 * FUNCTION: prelstConstructFilename()
 * DESCRIPTION:
 *   Fills in the passed already allocated string with the filename of the passed preset
 */
int PT_DECLSPEC prelstConstructFilename(PT_HANDLE *hp_prelst, int i_index, wchar_t *wcp_filename)
{
   struct prelstHdlType *cast_handle;

   cast_handle = (struct prelstHdlType *)(hp_prelst);
   
   if (cast_handle == NULL)
      return(NOT_OKAY);

   if ((i_index < 0) || (i_index > cast_handle->user_max_index))
      return(NOT_OKAY);

   if (i_index < cast_handle->user_min_index)
	  swprintf(wcp_filename, L"%d.%s", i_index + 1, PRELST_FACTORY_EXTENSION_WIDE);
   else
      swprintf(wcp_filename, L"%d.%s", i_index + 1, PRELST_USER_EXTENSION_WIDE);
   
   return(OKAY);
} 

/*
 * FUNCTION: prelstAskIsFactory()
 * DESCRIPTION:
 *   Passes back if the passed preset is a factory preset.
 */
int PT_DECLSPEC prelstAskIsFactory(PT_HANDLE *hp_prelst, int i_index, int *ip_factory)
{
   struct prelstHdlType *cast_handle;

   cast_handle = (struct prelstHdlType *)(hp_prelst);
   
   if (cast_handle == NULL)
      return(NOT_OKAY);

   if ((i_index < 0) || (i_index > cast_handle->user_max_index))
      return(NOT_OKAY);

   if (i_index < cast_handle->user_min_index)
      *ip_factory = IS_TRUE;
   else
      *ip_factory = IS_FALSE;
   
   return(OKAY);
} 

/*
 * FUNCTION: prelstCalcListToRealNum()
 * DESCRIPTION:
 *   Calculate and passes back the preset number, based on the number of the preset
 *   in the selection list.  
 *   
 *   Note: The selection list starts at 0 and does not have any non-existant presets. 
 */
int PT_DECLSPEC prelstCalcListToRealNum(PT_HANDLE *hp_prelst, int i_list_index, int *ip_real_index)
{
   struct prelstHdlType *cast_handle;
   int count;
   int passed_end;
   int found;
   
   cast_handle = (struct prelstHdlType *)(hp_prelst);
   
   if (cast_handle == NULL)
      return(NOT_OKAY);

   if (i_list_index < 0)
      return(NOT_OKAY);      
   
   *ip_real_index = 0;
   count = 0;
   passed_end = IS_FALSE;
   found = IS_FALSE;
   while ((!passed_end) && (!found))
   {
      if ((cast_handle->exist_flags)[*ip_real_index] == IS_TRUE)
         count++;
      
      if ((count - 1) == i_list_index)
         found = IS_TRUE;
      else
      {
         *ip_real_index = *ip_real_index + 1;
      
         if (*ip_real_index > cast_handle->user_max_index)
            passed_end = IS_TRUE;
      }
   }
   
   if (!found)
      return(NOT_OKAY);

   return(OKAY);
}

/*
 * FUNCTION: prelstCalcRealToListNum()
 * DESCRIPTION:
 *   Calculate and passes back the list number, based on the number of the preset number.
 *   
 *   Note: The selection list starts at 0 and does not have any non-existant presets. 
 */
int PT_DECLSPEC prelstCalcRealToListNum(PT_HANDLE *hp_prelst, int i_real_index, int *ip_list_index)
{
   struct prelstHdlType *cast_handle;
   int index;
   int count;
   
   cast_handle = (struct prelstHdlType *)(hp_prelst);
   
   if (cast_handle == NULL)
      return(NOT_OKAY);

   if ((i_real_index < 0) || (i_real_index > cast_handle->user_max_index))
      return(NOT_OKAY);  
      
   /* Make sure the passed real preset exists */
   if (((cast_handle->exist_flags)[i_real_index]) == IS_FALSE)
      return(NOT_OKAY);
   
   count = 0;
   for (index = 0; index <= i_real_index; index++)
   {
      if (((cast_handle->exist_flags)[index]) == IS_TRUE)
         count++;
   }    
   
   *ip_list_index = count - 1;

   return(OKAY);
}  

/*
 * FUNCTION: prelstAddFile()
 * DESCRIPTION:
 *   Tell the module that the passed index now exists.
 */
int PT_DECLSPEC prelstAddFile(PT_HANDLE *hp_prelst, int i_index)
{
   struct prelstHdlType *cast_handle;

   cast_handle = (struct prelstHdlType *)(hp_prelst);

   if (cast_handle == NULL)
      return(NOT_OKAY);

   if ((i_index < 0) || (i_index > cast_handle->user_max_index))
      return(NOT_OKAY);
      
   (cast_handle->exist_flags)[i_index] = IS_TRUE;
   
   return(OKAY);
} 

/*
 * FUNCTION: prelstRemoveFile()
 * DESCRIPTION:
 *   Tell the module that the passed index does not exists.
 */
int PT_DECLSPEC prelstRemoveFile(PT_HANDLE *hp_prelst, int i_index)
{
   struct prelstHdlType *cast_handle;

   cast_handle = (struct prelstHdlType *)(hp_prelst);

   if (cast_handle == NULL)
      return(NOT_OKAY);

   if ((i_index < 0) || (i_index > cast_handle->user_max_index))
      return(NOT_OKAY);
      
   (cast_handle->exist_flags)[i_index] = IS_FALSE;
   
   return(OKAY);
} 

/*
 * FUNCTION: prelstNextAvailableUserPreset()
 * DESCRIPTION:
 *   Passes back the next available user preset number.  If there are not any available,
 *   it issues an error message and returns NOT_OKAY.
 */
int PT_DECLSPEC prelstNextAvailableUserPreset(PT_HANDLE *hp_prelst, int *ip_avail_index)
{
   struct prelstHdlType *cast_handle;
   int found;
   
   cast_handle = (struct prelstHdlType *)(hp_prelst);
   
   if (cast_handle == NULL)
      return(NOT_OKAY);

   *ip_avail_index = cast_handle->user_min_index;
   
   found = IS_FALSE;
   while ((!found) && ((*ip_avail_index) <= cast_handle->user_max_index))
   {
      if ((cast_handle->exist_flags)[(*ip_avail_index)] == IS_FALSE)
         found = IS_TRUE;
      else
         *ip_avail_index = *ip_avail_index + 1;
   }
   
   if (!found)
   {
	   sprintf(cast_handle->msg1, "No available preset locations.");
	   (cast_handle->slout_hdl)->Error(FIRST_LINE, cast_handle->msg1);
	   sprintf(cast_handle->msg1, "You must delete a preset.");
	   (cast_handle->slout_hdl)->Error(NEXT_LINE, cast_handle->msg1);  
   }
   
   return(OKAY);
}

/*
 * FUNCTION: prelstFreeUp()
 * DESCRIPTION:
 *   Frees the passed prelst handle and sets to NULL.
 */
int PT_DECLSPEC prelstFreeUp(PT_HANDLE **hpp_prelst)
{
	struct prelstHdlType *cast_handle;

	cast_handle = (struct prelstHdlType *)(*hpp_prelst);

	if (cast_handle == NULL)
		return(OKAY);
		
	/* Free factory directory name */
	if (cast_handle->wcp_factory_dir != NULL)
		free(cast_handle->wcp_factory_dir);

	/* Free user directory name */
	if (cast_handle->wcp_user_dir != NULL)
		free(cast_handle->wcp_user_dir);
	
	/* Free the array of flags */
	if (cast_handle->exist_flags != NULL)		
	   free(cast_handle->exist_flags);

	free(cast_handle);

	*hpp_prelst = NULL;

	return(OKAY);
}

/*
 * FUNCTION: prelstDump()
 * DESCRIPTION:
 *   Dump the passed eflst handle to the screen.
 */
int PT_DECLSPEC prelstDump(PT_HANDLE *hp_prelst)
{
	struct prelstHdlType *cast_handle;
	int index;

	cast_handle = (struct prelstHdlType *)(hp_prelst);

    /* Dump the factory flags */
    for (index = 0; index < cast_handle->user_min_index; index++)
	{
	   sprintf(cast_handle->msg1, "FACTORY[%d] = %d",
	           index, (cast_handle->exist_flags)[index]);
	   (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
	}
	
	/* Dump the user flags */
    for (index = cast_handle->user_min_index; index <= cast_handle->user_max_index; index++)
	{
	   sprintf(cast_handle->msg1, "USER[%d] = %d",
	           index, (cast_handle->exist_flags)[index]);
	   (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
	}  

	return(OKAY);
}