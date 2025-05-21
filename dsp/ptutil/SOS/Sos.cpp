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
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "slout.h"
#include "vals.h"
#include "mry.h"
#include "filt.h"
#include "sos.h"
#include "u_sos.h"

/*
 * FUNCTION: sosNew()
 * DESCRIPTION:
 *  Allocates and initializes the passed sos handle to have the passed number of sections.
 */
int PT_DECLSPEC sosNew(PT_HANDLE **hpp_sos, CSlout *hp_slout, int i_num_sections)
{
	struct sosHdlType *cast_handle;   

	/* Allocate the handle */
	cast_handle = (struct sosHdlType *)calloc(1,
						sizeof(struct sosHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);  

	/* Store the slout */
	cast_handle->slout_hdl = hp_slout;
    
    /* Store number of sections */
    cast_handle->num_allocated_sections = i_num_sections;       
    cast_handle->num_active_sections = i_num_sections;    
    
    /* Initialize master gain */
    cast_handle->master_gain = (realtype)1.0;
	cast_handle->normalization_gain = (realtype)1.0;
	cast_handle->target_rms = 0.0f;

	cast_handle->disable_band_1 = false;

    /* Allocate the sections */
    cast_handle->sections =
	   (struct sosSectionType *)calloc(i_num_sections, sizeof(struct sosSectionType));   
    if (cast_handle->sections == NULL)
       return(NOT_OKAY);
    
	*hpp_sos = (PT_HANDLE *)cast_handle;

    /* Set sections to unity gain, and initialize freq setting */
	if( sosSetAllSectionsUnityGain(*hpp_sos, IS_TRUE) != OKAY)
		return(NOT_OKAY);

	// Zero state values
	if( sosZeroStateAllSections(*hpp_sos) != OKAY)
		return(NOT_OKAY);
		
	return(OKAY);
}

/*
 * FUNCTION: sosFreeUp()
 * DESCRIPTION:
 *   Frees the passed sos handle and sets to NULL.
 */
int PT_DECLSPEC sosFreeUp(PT_HANDLE **hpp_sos)
{
	struct sosHdlType *cast_handle;

	cast_handle = (struct sosHdlType *)(*hpp_sos);

	if (cast_handle == NULL)
		return(OKAY);

	/* Free the array of flags */
	if (cast_handle->sections != NULL)		
	   free(cast_handle->sections);

	free(cast_handle);

	*hpp_sos = NULL;

	return(OKAY);
}

/*
 * FUNCTION: sosDump()
 * DESCRIPTION:
 *   Dump the passed sos handle to the screen.
 */
int PT_DECLSPEC sosDump(PT_HANDLE *hp_sos)
{
   int num_sections;
   int section_num;
   int sos_type;
   realtype b0, b1, b2, a1, a2, a1_old, a2_old;
   struct sosHdlType *cast_handle;
   int section_on_flag;

   cast_handle = (struct sosHdlType *)(hp_sos);

   if (cast_handle == NULL)
      return(OKAY);

	// Don't try to print if slout handle is NULL
	if (cast_handle->slout_hdl == NULL)
		return(OKAY);
 
   if (sosGetNumAllocatedSections(hp_sos, &num_sections) != OKAY)
      return(NOT_OKAY);
    
   sprintf(cast_handle->msg1, "Number of sections = %d\n", num_sections);
   (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
    
   /* Dump the sections */
   for (section_num = 0; section_num < cast_handle->num_allocated_sections; section_num++)
   {   	     
         if (sosGetSection(hp_sos, section_num, &b0, &b1, &b2, &a1, &a2, &a1_old, 
			               &a2_old, &sos_type, &section_on_flag) != OKAY)
            return(NOT_OKAY); 
            
         sprintf(cast_handle->msg1, "b1[%d] = %g\n", section_num, b1);
	     (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);     
         sprintf(cast_handle->msg1, "b2[%d] = %g\n", section_num, b2);
	     (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);     
         sprintf(cast_handle->msg1, "a1[%d] = %g\n", section_num, a1);
	     (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);     
         sprintf(cast_handle->msg1, "a2[%d] = %g\n", section_num, a2);
	     (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);     
         sprintf(cast_handle->msg1, "sos_type[%d] = %d\n", section_num, sos_type);
	     (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
   }    

   return(OKAY);
}
