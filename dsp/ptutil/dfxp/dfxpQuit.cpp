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

/* dfxpQuit.cpp */

#include <windows.h>
#include <stdio.h>

#include "u_dfxp.h" /* Must go before codedefs.h due to mmgr */
#include "codedefs.h"

#include "dfxp.h"
#include "qnt.h"
#include "com.h"
#include "GraphicEq.h"
#include "spectrum.h"
#include "SurroundSyn.h"
#include "BinauralSyn.h"
#include "dfxSharedUtil.h"

/*
 * FUNCTION: dfxpQuit() 
 * DESCRIPTION:
 *   Free the global data.
 */
int dfxpQuit(PT_HANDLE **hpp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(*hpp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   if (!(cast_handle->fully_initialized))
		return(NOT_OKAY);

   /* Free all the global data */
	if (dfxp_FreeAll(*hpp_dfxp) != OKAY)
		return(NOT_OKAY);

	free(cast_handle);

	*hpp_dfxp = NULL;

	return(OKAY);
}

/*
 * FUNCTION: dfxp_FreeAll() 
 * DESCRIPTION:
 *  Frees all the global data.
 */
int dfxp_FreeAll(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   /* Free all the midi_to_dsp qnt handles */
   if (cast_handle->midi_to_dsp.fidelity_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.fidelity_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   }   

   if (cast_handle->midi_to_dsp.spaciousness_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.spaciousness_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.ambience_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.ambience_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.dynamic_boost_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.dynamic_boost_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.bass_boost_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.bass_boost_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

	/* Fixed Aural Activation Specific */
   if (cast_handle->midi_to_dsp.aural_filter_gain_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.aural_filter_gain_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.aural_filter_a1_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.aural_filter_a1_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.aural_filter_a0_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.aural_filter_a0_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 
   
	/* Fixed Reverb Specific */
   if (cast_handle->midi_to_dsp.room_size_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.room_size_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.damping_bandwidth_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.damping_bandwidth_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.rolloff_bandwidth_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.rolloff_bandwidth_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.motion_rate_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.motion_rate_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.motion_depth_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.motion_depth_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.screen_lex_main_knob3_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.screen_lex_main_knob3_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.screen_lex_main_knob4_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.screen_lex_main_knob4_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

	/* Fixed Optimizer Specific */
   if (cast_handle->midi_to_dsp.release_time_beta_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.release_time_beta_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.screen_opt_main_knob3_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.screen_opt_main_knob3_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

	/* Fixed Widener Specific */
   if (cast_handle->midi_to_dsp.dispersion_delay_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.dispersion_delay_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.wid_filter_gain_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.wid_filter_gain_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.wid_filter_a1_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.wid_filter_a1_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   } 

   if (cast_handle->midi_to_dsp.wid_filter_a0_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.wid_filter_a0_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   }

	/* Delay specific */
   if (cast_handle->midi_to_dsp.dly_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_dsp.dly_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   }

	/* Free all the other qnt handles */
   if (cast_handle->real_to_midi_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->real_to_midi_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   }

   if (cast_handle->midi_to_real_qnt_hdl != NULL)
   {
      if (qntFreeUp(&(cast_handle->midi_to_real_qnt_hdl)) != OKAY)
         return(NOT_OKAY);
   }

	/* Free the com handles */
	if (cast_handle->com_hdl_front != NULL)
   {
      if (comFreeUp(&(cast_handle->com_hdl_front)) != OKAY)
         return(NOT_OKAY);
   } 	 

	if (cast_handle->com_hdl_rear != NULL)
   {
      if (comFreeUp(&(cast_handle->com_hdl_rear)) != OKAY)
         return(NOT_OKAY);
   } 	 

	if (cast_handle->com_hdl_side != NULL)
   {
      if (comFreeUp(&(cast_handle->com_hdl_side)) != OKAY)
         return(NOT_OKAY);
   } 	 

	if (cast_handle->com_hdl_center != NULL)
   {
      if (comFreeUp(&(cast_handle->com_hdl_center)) != OKAY)
         return(NOT_OKAY);
   } 	 

	if (cast_handle->com_hdl_subwoofer != NULL)
   {
      if (comFreeUp(&(cast_handle->com_hdl_subwoofer)) != OKAY)
         return(NOT_OKAY);
	}

	// Free EQ handle
	if (cast_handle->eq.graphicEq_hdl != NULL)
   {
      if (GraphicEqFreeUp(&(cast_handle->eq.graphicEq_hdl)) != OKAY)
         return(NOT_OKAY);
	}

	// Free SurroundSyn handle
	if (cast_handle->SurroundSyn_hdl != NULL)
   {
      if (SurroundSynFreeUp(&(cast_handle->SurroundSyn_hdl)) != OKAY)
         return(NOT_OKAY);
	}

	/* Free the GraphicEq handle */
   if (cast_handle->eq.graphicEq_hdl != NULL)
	{
		if (GraphicEqFreeUp(&cast_handle->eq.graphicEq_hdl) != OKAY)
			return(NOT_OKAY);
	}

	/* Free the spectrum handle */
	if (cast_handle->spectrum.spectrum_hdl != NULL)
	{
		if (spectrumFreeUp(&(cast_handle->spectrum.spectrum_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Free the binauralSyn handle */
	if (cast_handle->BinauralSyn_hdl != NULL)
	{
		if (BinauralSynFreeUp(&(cast_handle->BinauralSyn_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Free shared memory library */
	if (cast_handle->hp_sharedUtil != NULL)
	{
		if (dfxSharedUtilFreeUp(&(cast_handle->hp_sharedUtil)) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}