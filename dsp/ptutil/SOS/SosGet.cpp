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
#include "slout.h"
#include "vals.h"
#include "mry.h"
#include "filt.h"
#include "sos.h"
#include "u_sos.h"

/*
 * FUNCTION: sosGetMasterGain()
 * DESCRIPTION:
 *  Gets the external master gain for the combined filter structure.
 */
int PT_DECLSPEC sosGetMasterGain(PT_HANDLE *hp_sos, realtype *rp_master_gain)
{
	struct sosHdlType *cast_handle;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
	if (cast_handle == NULL)
       return(NOT_OKAY);
       
 	*rp_master_gain = cast_handle->master_gain;
 	
 	return(OKAY);   
}

/*
 * FUNCTION: sosGetNumAllocatedSections()
 * DESCRIPTION:
 *   Get the number of allocated sections in the passed sos handle.
 */
int PT_DECLSPEC sosGetNumAllocatedSections(PT_HANDLE *hp_sos, int *ip_num_sections)
{
	struct sosHdlType *cast_handle;

	cast_handle = (struct sosHdlType *)(hp_sos);  

    if (cast_handle == NULL)
       return(NOT_OKAY);
 
    *ip_num_sections = cast_handle->num_allocated_sections;
    
    return(OKAY);
} 

/*
 * FUNCTION: sosGetNumActiveSections()
 * DESCRIPTION:
 *   Get the number of active sections in the passed sos handle.
 */
int PT_DECLSPEC sosGetNumActiveSections(PT_HANDLE *hp_sos, int *ip_num_sections)
{
	struct sosHdlType *cast_handle;

	cast_handle = (struct sosHdlType *)(hp_sos);  

    if (cast_handle == NULL)
       return(NOT_OKAY);
 
    *ip_num_sections = cast_handle->num_active_sections;
    
    return(OKAY);
}

/*
 * FUNCTION: sosGetSection()
 * DESCRIPTION:
 *   Get the coeff values of the specified section.
 */
int PT_DECLSPEC sosGetSection(PT_HANDLE *hp_sos, int i_section_num, realtype *rp_b0, realtype *rp_b1, realtype *rp_b2,
                  realtype *rp_a1, realtype *rp_a2, realtype *rp_a1_old, realtype *rp_a2_old, int *ip_sos_type,
				  int *ip_section_on_flag)
{
	struct sosHdlType *cast_handle;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
    if (cast_handle == NULL)
       return(NOT_OKAY);

	/* Make sure parameters are legal */
	if (i_section_num >= cast_handle->num_allocated_sections)
	   return(NOT_OKAY);
	   
	/* Set the return values */
 	 *rp_b0 = ((cast_handle->sections)[i_section_num]).b0; 
 	 *rp_b1 = ((cast_handle->sections)[i_section_num]).b1; 
 	 *rp_b2 = ((cast_handle->sections)[i_section_num]).b2; 
 	 *rp_a1 = ((cast_handle->sections)[i_section_num]).a1; 
 	 *rp_a2 = ((cast_handle->sections)[i_section_num]).a2; 
 	 *rp_a1_old = ((cast_handle->sections)[i_section_num]).a1_old; 
 	 *rp_a2_old = ((cast_handle->sections)[i_section_num]).a2_old; 
 	 
 	 *ip_sos_type  = (cast_handle->sos_type)[i_section_num]; 
	
	 *ip_section_on_flag = cast_handle->section_on_flag[i_section_num];
    
	return(OKAY);
} 

/*
 * FUNCTION: sosGetSectionResponseFlag()
 * DESCRIPTION:
 *   Get the value of the flag that says if the response for this section is valid.
 */
int PT_DECLSPEC sosGetSectionResponseFlag(PT_HANDLE *hp_sos, int i_section_num, int *ip_valid_flag)
{
	struct sosHdlType *cast_handle;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
    if (cast_handle == NULL)
       return(NOT_OKAY);

	/* Make sure parameters are legal */
	if (i_section_num >= cast_handle->num_allocated_sections)
	   return(NOT_OKAY);
	   
	/* Set the return value */
 	*ip_valid_flag = (cast_handle->sos_response_valid)[i_section_num]; 
 	
	return(OKAY);
} 

/*
 * FUNCTION: sosGetSectionResponseFlagArray()
 * DESCRIPTION:
 *   Gets array of flags that say if the responses for the sections are valid.
 */
int PT_DECLSPEC sosGetSectionResponseFlagArray(PT_HANDLE *hp_sos, int **ip_valid_flag_array)
{
	struct sosHdlType *cast_handle;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
    if (cast_handle == NULL)
       return(NOT_OKAY);

	/* Set the return value */
 	*ip_valid_flag_array = cast_handle->sos_response_valid; 
 	
	return(OKAY);
} 

/*
 * FUNCTION: sosGetCenterFreqArray()
 * DESCRIPTION:
 *   Gets array of the center frequency settings for the sections.
 */
int PT_DECLSPEC sosGetCenterFreqArray(PT_HANDLE *hp_sos, realtype **rpp_center_freqs)
{
	struct sosHdlType *cast_handle;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
    if (cast_handle == NULL)
       return(NOT_OKAY);

	/* Set the return value */
 	*rpp_center_freqs = cast_handle->sos_center_freq; 
 	
	return(OKAY);
} 

/*
 * FUNCTION: sosGetCenterFreqIndexArray()
 * DESCRIPTION:
 *   Gets array of the center frequency settings for the sections.
 */
int PT_DECLSPEC sosGetCenterFreqIndexArray(PT_HANDLE *hp_sos, int **ipp_center_freq_indexes)
{
	struct sosHdlType *cast_handle;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
    if (cast_handle == NULL)
       return(NOT_OKAY);

	/* Set the return value */
 	*ipp_center_freq_indexes = cast_handle->sos_center_freq_indexes; 
 	
	return(OKAY);
} 

/*
 * FUNCTION: sosGetCenterFreqResponseArray()
 * DESCRIPTION:
 *   Gets array of dB responses at the center frequency settings for the sections.
 */
int PT_DECLSPEC sosGetCenterFreqResponseArray(PT_HANDLE *hp_sos, realtype **rpp_center_freq_responses)
{
	struct sosHdlType *cast_handle;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
    if (cast_handle == NULL)
       return(NOT_OKAY);

	/* Set the return value */
 	*rpp_center_freq_responses = cast_handle->sos_center_freq_response; 
 	
	return(OKAY);
} 

