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
 * FUNCTION: sosSetSection()
 * DESCRIPTION:
 *   Sets the coefficients of the specified section.
 */
int PT_DECLSPEC sosSetSection(PT_HANDLE *hp_sos, int i_section_num, int i_set_freq, 
                  				struct filt2ndOrderBoostCutShelfFilterType *filt,
										int i_filt_type)
{
	struct sosHdlType *cast_handle;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
	if (cast_handle == NULL)
       return(NOT_OKAY);
	
	/* Make sure parameters are legal */
	if (i_section_num >= cast_handle->num_allocated_sections)
	   return(NOT_OKAY);
	   
	/* Update old values */
 	((cast_handle->sections)[i_section_num]).a1_old = ((cast_handle->sections)[i_section_num]).a1; 
 	((cast_handle->sections)[i_section_num]).a2_old = ((cast_handle->sections)[i_section_num]).a2; 
	
	/* Store the values */
 	((cast_handle->sections)[i_section_num]).b0 = filt->b0; 
 	((cast_handle->sections)[i_section_num]).b1 = filt->b1; 
 	((cast_handle->sections)[i_section_num]).b2 = filt->b2; 
 	((cast_handle->sections)[i_section_num]).a1 = filt->a1; 
 	((cast_handle->sections)[i_section_num]).a2 = filt->a2; 
 	
 	(cast_handle->sos_type)[i_section_num] = i_filt_type; 
	
   /* Set response flag for this section to invalid (needs to be updated) */
   (cast_handle->sos_response_valid)[i_section_num] = 0;
    
   /* If desired, set center freq for this section */
   if( i_set_freq )
    	cast_handle->sos_center_freq[i_section_num] = filt->r_center_freq;
    
   /* Set the targeted center frequency response for this section (db) */
   cast_handle->sos_center_freq_response[i_section_num] = filt->boost;

	cast_handle->section_on_flag[i_section_num] = filt->section_on_flag;
    
	return(OKAY);
}

/*
 * FUNCTION: sosSetSectionUnityGain()
 * DESCRIPTION:
 *   Sets the coefficients of the specified sos section to flat gain of 1.0 .
 */
int PT_DECLSPEC sosSetSectionUnityGain(PT_HANDLE *hp_sos, int i_section_num, int i_init_freq)
{
	struct sosHdlType *cast_handle;
	struct filt2ndOrderBoostCutShelfFilterType filt;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
 	if (cast_handle == NULL)
       return(NOT_OKAY);
	
	filt.b0 = (realtype)1.0;
	filt.b1 = (realtype)0.0;
	filt.b2 = (realtype)0.0;
	filt.a1 = (realtype)0.0;
	filt.a2 = (realtype)0.0;
	filt.boost = (realtype)0.0;
	filt.r_center_freq = (realtype)20.0;
	filt.section_on_flag = IS_FALSE;

	/* Sets the center frequency to 20.0, generic type, response valid will be turned off */
   if( sosSetSection(hp_sos, i_section_num, i_init_freq,
                      &filt, SOS_GENERIC) != OKAY )
		return(NOT_OKAY);
		
	return(OKAY);
}

/*
 * FUNCTION: sosSetAllSectionsUnityGain()
 * DESCRIPTION:
 *   Sets the coefficients of all sos sections to flat gain of 1.0 .
 * 	 Does all allocated sections.
 */
int PT_DECLSPEC sosSetAllSectionsUnityGain(PT_HANDLE *hp_sos, int i_init_freqs)
{
	struct sosHdlType *cast_handle;
	int section_num;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
	if (cast_handle == NULL)
       return(NOT_OKAY);
       
    for(section_num=0; section_num < cast_handle->num_allocated_sections; section_num++)
		sosSetSectionUnityGain(hp_sos, section_num, i_init_freqs);    	       
	
	return(OKAY);
}

/*
 * FUNCTION: sosZeroStateAllSections()
 * DESCRIPTION:
 *   Zeros the state values for all sections.
 */
int PT_DECLSPEC sosZeroStateAllSections(PT_HANDLE *hp_sos)
{
	struct sosHdlType *cast_handle;
	int section_num, i;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
	if (cast_handle == NULL)
       return(NOT_OKAY);

	cast_handle->in1_old = (realtype)0.0;
	cast_handle->in2_old = (realtype)0.0;
	cast_handle->outDC1_old = (realtype)0.0;
	cast_handle->outDC2_old = (realtype)0.0;

	for(i=0; i<8; i++)
	{
		cast_handle->in1_oldSS[i] = (realtype)0.0;
		cast_handle->in2_oldSS[i] = (realtype)0.0;
		cast_handle->outDC1_oldSS[i] = (realtype)0.0;
		cast_handle->outDC2_oldSS[i] = (realtype)0.0;
	}
       
	for(section_num=0; section_num < cast_handle->num_allocated_sections; section_num++)
	{
		/* Zero the internal state values */
		((cast_handle->sections)[section_num]).state1 = (realtype)0.0; 
		((cast_handle->sections)[section_num]).state2 = (realtype)0.0; 
		((cast_handle->sections)[section_num]).state3 = (realtype)0.0; 
		((cast_handle->sections)[section_num]).state4 = (realtype)0.0;

		for(i=0; i<8; i++)
		{
			((cast_handle->sections)[section_num]).state_1[i] = (realtype)0.0; 
 			((cast_handle->sections)[section_num]).state_2[i] = (realtype)0.0; 
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: sosSetNumActiveSections()
 * DESCRIPTION:
 *  Set the number of active sections.  This must be less than or equal to the
 *  number of allocated sections.
 */
int PT_DECLSPEC sosSetNumActiveSections(PT_HANDLE *hp_sos, int i_num_active_sections)
{
	struct sosHdlType *cast_handle;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
	if (cast_handle == NULL)
       return(NOT_OKAY);
	
	/* Make sure parameters are legal */
	if ((i_num_active_sections > cast_handle->num_allocated_sections) ||
	    (i_num_active_sections < 0))
	   return(NOT_OKAY);
	   
    cast_handle->num_active_sections = i_num_active_sections;
    
	return(OKAY);
}    
    
/*
 * FUNCTION: sosSetMasterGain()
 * DESCRIPTION:
 *  Sets an external master gain for the combined filter structure.
 */
int PT_DECLSPEC sosSetMasterGain(PT_HANDLE *hp_sos, realtype r_master_gain)
{
	struct sosHdlType *cast_handle;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
	if (cast_handle == NULL)
       return(NOT_OKAY);
       
 	cast_handle->master_gain = r_master_gain;
 	
 	return(OKAY);   
}

/*
 * FUNCTION: sosSetSectionResponseFlag()
 * DESCRIPTION:
 *   Sets the value of the flag that says if the response for this section is valid.
 */
int PT_DECLSPEC sosSetSectionResponseFlag(PT_HANDLE *hp_sos, int i_section_num, int i_valid_flag)
{
	struct sosHdlType *cast_handle;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
    if (cast_handle == NULL)
       return(NOT_OKAY);

	/* Make sure parameters are legal */
	if (i_section_num >= cast_handle->num_allocated_sections)
	   return(NOT_OKAY);
	   
	/* Set the return value */
 	(cast_handle->sos_response_valid)[i_section_num] = i_valid_flag; 
 	
	return(OKAY);
}

/*
 * FUNCTION: sosSetDisableBand1Flag()
 * DESCRIPTION:
 *  Used to disable band1 in cases such has syncing of DFX Hyperbass and Band1 control.
 *
 */
int PT_DECLSPEC sosSetDisableBand1Flag(PT_HANDLE *hp_sos, bool b_disable_band_1)
{
	struct sosHdlType *cast_handle;

	cast_handle = (struct sosHdlType *)(hp_sos);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->disable_band_1 = b_disable_band_1;

	return(OKAY);
}

int PT_DECLSPEC sosSetVolumeNormalization(PT_HANDLE *hp_sos, realtype r_target_rms)
{
	struct sosHdlType* cast_handle;

	cast_handle = (struct sosHdlType*)(hp_sos);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->target_rms = r_target_rms;
	cast_handle->normalization_gain = 1.0f;

	return(OKAY);
}