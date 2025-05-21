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

/* dfxpComm.cpp */

#include "codedefs.h"

#include <windows.h>
#include <stdio.h>
#include <math.h>

#include "u_dfxp.h" 

#include "dfxp.h"
#include "DfxSdk.h"
#include "com.h"
#include "qnt.h"
#include "math.h"
#include "midi.h"
#include "filt.h"

#include "C_Play.h"
#include "C_aural.h"
#include "C_dsps.h"
#include "c_lex.h"
#include "c_max.h"
#include "c_aural.h"
#include "c_wid.h"
#include "c_dly1.h"

/* 
 * Any midi setting for Ambience at this value or below will be
 * treated as 0.
 */
#define DFXP_MIN_EFFECTIVE_MIDI_AMBIENCE 12

/*
 * FUNCTION: dfxp_CommunicateInit() 
 * DESCRIPTION:
 *   Initialize the Comm handle.  This function should only be called once
 *   at the beginning.
 */
int dfxp_CommunicateInit(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	struct {
	   int softdsp_mode;
	   int board_address;
	   int processor_num;
	   long base_address;
	   char cp_dsp_dirpath[64];
	   int num_cards_configured;
	} obsolete;

	/* 
	 * Initialize the variables which are passed to comInit but really don't
	 * have any meaning except are in the function call for historical 
	 * purposes.
	 */
	obsolete.softdsp_mode = IS_TRUE;
	obsolete.board_address = 0;
	obsolete.processor_num = 1;
	obsolete.base_address = 0L;
	obsolete.num_cards_configured = 0;
	sprintf(obsolete.cp_dsp_dirpath, "");

   if (comInit(&(cast_handle->com_hdl_front), obsolete.softdsp_mode, obsolete.board_address, 
		         obsolete.processor_num, obsolete.base_address, obsolete.cp_dsp_dirpath, 
	            obsolete.num_cards_configured, cast_handle->slout1) != OKAY)
      return(NOT_OKAY);

   if (comInit(&(cast_handle->com_hdl_rear), obsolete.softdsp_mode, obsolete.board_address, 
		         obsolete.processor_num, obsolete.base_address, obsolete.cp_dsp_dirpath, 
	            obsolete.num_cards_configured, cast_handle->slout1) != OKAY)
      return(NOT_OKAY);

   if (comInit(&(cast_handle->com_hdl_side), obsolete.softdsp_mode, obsolete.board_address, 
		         obsolete.processor_num, obsolete.base_address, obsolete.cp_dsp_dirpath, 
	            obsolete.num_cards_configured, cast_handle->slout1) != OKAY)
      return(NOT_OKAY);

   if (comInit(&(cast_handle->com_hdl_center), obsolete.softdsp_mode, obsolete.board_address, 
		         obsolete.processor_num, obsolete.base_address, obsolete.cp_dsp_dirpath, 
	            obsolete.num_cards_configured, cast_handle->slout1) != OKAY)
      return(NOT_OKAY);

   if (comInit(&(cast_handle->com_hdl_subwoofer), obsolete.softdsp_mode, obsolete.board_address, 
		         obsolete.processor_num, obsolete.base_address, obsolete.cp_dsp_dirpath, 
	            obsolete.num_cards_configured, cast_handle->slout1) != OKAY)
      return(NOT_OKAY);

	if (dfxp_ComLoadAndRun(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	if (dfxpCommunicateAll(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_ComLoadAndRun() 
 * DESCRIPTION:
 *   Load and run the dsp program.  This needs to be called whenever the
 *   sampling freq, or bit width changes.
 */
int dfxp_ComLoadAndRun(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int stereo_flag;

	if (cast_handle->num_channels_out == 1)
		stereo_flag = IS_FALSE;
	else
		stereo_flag = IS_TRUE;

	if( cast_handle->num_channels_out == 1 )
	{
		if (comSoftDspLoadAndRunNonShared(cast_handle->com_hdl_front, DFXP_DSP_FUNCTION_NAME, cast_handle->internal_sampling_freq, 
										stereo_flag, (short)DFXP_DSP_INTERNAL_BIT_WIDTH) != OKAY)
			return(NOT_OKAY);
	}

	if( cast_handle->num_channels_out >= 2 )
	{
		// Front and rear channels as stereo
		if (comSoftDspLoadAndRunNonShared(cast_handle->com_hdl_front, DFXP_DSP_FUNCTION_NAME, cast_handle->internal_sampling_freq, 
										stereo_flag, (short)DFXP_DSP_INTERNAL_BIT_WIDTH) != OKAY)
			return(NOT_OKAY);

		if (comSoftDspLoadAndRunNonShared(cast_handle->com_hdl_rear, DFXP_DSP_FUNCTION_NAME, cast_handle->internal_sampling_freq, 
										stereo_flag, (short)DFXP_DSP_INTERNAL_BIT_WIDTH) != OKAY)
			return(NOT_OKAY);

		if (comSoftDspLoadAndRunNonShared(cast_handle->com_hdl_side, DFXP_DSP_FUNCTION_NAME, cast_handle->internal_sampling_freq, 
										stereo_flag, (short)DFXP_DSP_INTERNAL_BIT_WIDTH) != OKAY)
			return(NOT_OKAY);

		// Center and Subwoofer as mono
		if (comSoftDspLoadAndRunNonShared(cast_handle->com_hdl_center, DFXP_DSP_FUNCTION_NAME, cast_handle->internal_sampling_freq, 
										IS_FALSE, (short)DFXP_DSP_INTERNAL_BIT_WIDTH) != OKAY)
			return(NOT_OKAY);

		if (comSoftDspLoadAndRunNonShared(cast_handle->com_hdl_subwoofer, DFXP_DSP_FUNCTION_NAME, cast_handle->internal_sampling_freq, 
										IS_FALSE, (short)DFXP_DSP_INTERNAL_BIT_WIDTH) != OKAY)
			return(NOT_OKAY);
	}

   return(OKAY);
}

/*
 * FUNCTION: dfxpCommunicateAll() 
 * DESCRIPTION:
 *   Communicate to the DSP all the information needed for processing.
 */
int dfxpCommunicateAll(PT_HANDLE *hp_dfxp)
{
	/* Communicate all the parameters that can change based on user controls */
	if (dfxpCommunicateAllNonFixed(hp_dfxp, IS_FALSE) != OKAY)
		return(NOT_OKAY);

	/* Communicate all the parameters that don't change based on user controls */
	if (dfxp_CommunicateAllFixed(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxpCommunicateAllNonFixed() 
 * DESCRIPTION:
 *   Communicate to the DSP all the information needed for processing which can change
 *   based on user interaction.
 *  
 *   The i_communicate_slowly flag is used to specify that we want to save CPU power in cases in which we have
 *   no choice but to call this function for every buffer since the UI is running in a seperate process, such as the XP winmm and dsound DLL cases. 
 */
int dfxpCommunicateAllNonFixed(PT_HANDLE *hp_dfxp, int i_communicate_slowly_flag)
{
	struct dfxpHdlType *cast_handle;
	int i, eqOnReg;
	realtype boostCut;
	wchar_t wcp_boost_cut[PT_MAX_GENERIC_STRLEN];

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if ((!i_communicate_slowly_flag) || 
		 (i_communicate_slowly_flag && cast_handle->i_communicate_slowly_count == 0))
	{
		if (dfxp_CommunicateFidelity(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}

	if ((!i_communicate_slowly_flag) || 
		 (i_communicate_slowly_flag && cast_handle->i_communicate_slowly_count == 5))
	{
		if (dfxp_CommunicateSpaciousness(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}

	if ((!i_communicate_slowly_flag) || 
		 (i_communicate_slowly_flag && cast_handle->i_communicate_slowly_count == 10))
	{
		if (dfxp_CommunicateAmbience(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}

	if ((!i_communicate_slowly_flag) || 
		 (i_communicate_slowly_flag && cast_handle->i_communicate_slowly_count == 15))
	{
		if (dfxp_CommunicateBassBoost(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}

	if ((!i_communicate_slowly_flag) || 
		 (i_communicate_slowly_flag && cast_handle->i_communicate_slowly_count == 20))
	{
		if (dfxp_CommunicateVocalReduction(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}

	if ((!i_communicate_slowly_flag) || 
		 (i_communicate_slowly_flag && cast_handle->i_communicate_slowly_count == 25))
	{
		if (dfxp_CommunicateBypassSettings(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}

	if ((!i_communicate_slowly_flag) || 
		 (i_communicate_slowly_flag && cast_handle->i_communicate_slowly_count == 30))
	{
		if (dfxp_CommunicateMusicMode(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}

	// This code section only executes in the XP case when running from the winmm or dsound dll's, in this case cast_handle->processing_only is TRUE.
	// It implements the key dfxp calls required to update the EQ params that are performed in the sound card case by dfxg_UpdateFromRegistryEQ,
	// so if the content of that function changes the code here must be updated.
	if (cast_handle->processing_only)
	{
		if ((!i_communicate_slowly_flag) || 
			 (i_communicate_slowly_flag && cast_handle->i_communicate_slowly_count == 35))
		{
			// NOTE, the init and processing calls in the winmm and dsound dll's are called asyncronously so the processing call is made before the dfxp init call has fully completed.
			// The check below prevents trying to make use of the dfxp and EQ handles before initialization of them has been completed.
			if (cast_handle->fully_initialized == TRUE)
			{
				if (dfxpEqGetProcessingOn(hp_dfxp, DFXP_STORAGE_TYPE_REGISTRY, &eqOnReg) != OKAY)
					return(NOT_OKAY);

				if (dfxpEqSetProcessingOn(hp_dfxp, DFXP_STORAGE_TYPE_MEMORY, eqOnReg) != OKAY)
					return(NOT_OKAY);

				for(i=2;i<=DFXP_GRAPHIC_EQ_NUM_BANDS;i++)
				{
					if( dfxpEqGetBandBoostCut_FromRegistry(hp_dfxp, i, &boostCut, wcp_boost_cut) != OKAY)
						return(NOT_OKAY);

					if( dfxpEqSetBandBoostCut(hp_dfxp, DFXP_STORAGE_TYPE_MEMORY, i, boostCut) != OKAY)
						return(NOT_OKAY);
				}
			}
		}
	}

   /* Dynamic Boost is communicated in dfxp_CommunicateBypassSettings() */

	if (i_communicate_slowly_flag)
	{
		(cast_handle->i_communicate_slowly_count)++;
		if (cast_handle->i_communicate_slowly_count == 40)
			cast_handle->i_communicate_slowly_count = 0;
	}

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateAllFixed() 
 * DESCRIPTION:
 *   Communicate to the DSP all the information needed for processing which does not change
 *   based on user interaction.
 */
int dfxp_CommunicateAllFixed(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if (dfxp_CommunicateFixedQnts_Aural(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	if (dfxp_CommunicateFixedQnts_Lex(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	if (dfxp_CommunicateFixedQnts_Opt(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	if (dfxp_CommunicateFixedQnts_Wid(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	if (dfxp_CommunicateFixedQnts_Play(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	if (dfxp_CommunicateFixedQnts_Delay(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateBypassSettings()
 * DESCRIPTION:
 *   Communicates the bypass settings.
 */ 
int dfxp_CommunicateBypassSettings(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int bypass_fidelity;
	int bypass_surround;
	int bypass_bass_boost;
	int bypass_vocal_reduction;
	int fidelity_on;
	int surround_on;
	int bass_boost_on;
	int headphone_on;
	int vocal_reduction_on;
	int stereo_or_mono_flag;
	int surround_sound_flag;
	int bypass_all;

	/* Get the current settings */
	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_BYPASS, &bypass_all) != OKAY)
		return(NOT_OKAY);

	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_FIDELITY, &fidelity_on) != OKAY)
		return(NOT_OKAY);
	bypass_fidelity = !fidelity_on;
	
	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_SURROUND, &surround_on) != OKAY)
		return(NOT_OKAY);
	bypass_surround = !surround_on;

	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_BASS_BOOST, &bass_boost_on) != OKAY)
		return(NOT_OKAY);
	bypass_bass_boost = !bass_boost_on;

	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_HEADPHONE, &headphone_on) != OKAY)
		return(NOT_OKAY);
	cast_handle->binaural_headphone_on_flag = headphone_on;

	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_VOCAL_REDUCTION_ON, &vocal_reduction_on) != OKAY)
		return(NOT_OKAY);
	bypass_vocal_reduction = !vocal_reduction_on;

	/* Take care of the override processing setting */
	if (cast_handle->processing_override == DFXP_PROCESSING_OVERRIDE_DFX_ALL_EXCEPT_OPTIMIZER)
	{
	   bypass_fidelity = IS_TRUE;
	   bypass_surround = IS_TRUE;
	   bypass_bass_boost = IS_TRUE;
	   bypass_vocal_reduction = IS_TRUE;
		cast_handle->binaural_headphone_on_flag = IS_FALSE;
	}

	if(cast_handle->num_channels_out <= 2)
	{
		stereo_or_mono_flag = 1;
		surround_sound_flag = 0;
	}
	else
	{
		stereo_or_mono_flag = 0;
		surround_sound_flag = 1;
	}

	/* Send the master bypass */
	// Force master bypass on SurroundSound channels when in stereo or mono mode.
	if (comLongIntWrite(cast_handle->com_hdl_front, DSP_PLAY_BYPASS_ON, bypass_all) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_rear, DSP_PLAY_BYPASS_ON, (bypass_all | stereo_or_mono_flag) ) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_side, DSP_PLAY_BYPASS_ON, (bypass_all | stereo_or_mono_flag) ) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_center, DSP_PLAY_BYPASS_ON, (bypass_all | stereo_or_mono_flag) ) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_subwoofer, DSP_PLAY_BYPASS_ON, (bypass_all | stereo_or_mono_flag) ) != OKAY)
		return(NOT_OKAY);

	/* Send the individual knob bypass settings */
	// Activator is always off on subwoofer channel
	if (comLongIntWrite(cast_handle->com_hdl_front, DSP_PLAY_ACTIVATOR_ON, !(bypass_fidelity)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_rear, DSP_PLAY_ACTIVATOR_ON, !(bypass_fidelity)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_side, DSP_PLAY_ACTIVATOR_ON, !(bypass_fidelity)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_center, DSP_PLAY_ACTIVATOR_ON, !(bypass_fidelity)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_subwoofer, DSP_PLAY_ACTIVATOR_ON, 0) != OKAY)
		return(NOT_OKAY);

	// Widener is always off in center and subwoofer channels
	if (comLongIntWrite(cast_handle->com_hdl_front, DSP_PLAY_WIDENER_ON, !(bypass_surround)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_rear, DSP_PLAY_WIDENER_ON, !(bypass_surround)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_side, DSP_PLAY_WIDENER_ON, !(bypass_surround)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_center, DSP_PLAY_WIDENER_ON, 0) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_subwoofer, DSP_PLAY_WIDENER_ON, 0) != OKAY)
		return(NOT_OKAY);

	// For Surround Sound mode, bass boost is only on in subwoofer channel
	// PTNOTE - on 10/29/10, modified to always allow bass boost on front channels
	//if (comLongIntWrite(cast_handle->com_hdl_front, DSP_PLAY_BASS_BOOST_ON, (!(bypass_bass_boost) & stereo_or_mono_flag) ) != OKAY)
	if (comLongIntWrite(cast_handle->com_hdl_front, DSP_PLAY_BASS_BOOST_ON, !(bypass_bass_boost)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_rear, DSP_PLAY_BASS_BOOST_ON, 0) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_side, DSP_PLAY_BASS_BOOST_ON, 0) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_center, DSP_PLAY_BASS_BOOST_ON, 0) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_subwoofer, DSP_PLAY_BASS_BOOST_ON, (!(bypass_bass_boost) & surround_sound_flag)) != OKAY)
		return(NOT_OKAY);

	// Vocal reduction only in in front channels
	if (comLongIntWrite(cast_handle->com_hdl_front, DSP_PLAY_VOCAL_REDUCTION_ON, !(bypass_vocal_reduction)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_rear, DSP_PLAY_VOCAL_REDUCTION_ON, 0) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_side, DSP_PLAY_VOCAL_REDUCTION_ON, 0) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_center, DSP_PLAY_VOCAL_REDUCTION_ON, 0) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_subwoofer, DSP_PLAY_VOCAL_REDUCTION_ON, 0) != OKAY)
		return(NOT_OKAY);

	/* Do special calculation for Ambience bypass */
	if (dfxp_CommAmbienceBypass(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	/* 
	 * Send the optimizer bypass (Since in this case we really want to just send
	 * a zero knob setting when in bypass we will just call the normal comm for
	 * the optimizer
	 */
	if (dfxp_CommunicateDynamicBoost(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	/* PTNOTE - this older headphone mode has been replaced by the BinauralSyn technology. Turn off this older version */
	/*
	// Headphone mode only on in front channels when in stereo mode
	if (comLongIntWrite(cast_handle->com_hdl_front, DSP_PLAY_HEADPHONE_ON, (!(bypass_headphone) & stereo_or_mono_flag) ) != OKAY)
		return(NOT_OKAY); */
	if (comLongIntWrite(cast_handle->com_hdl_front, DSP_PLAY_HEADPHONE_ON, 0 ) != OKAY)
		return(NOT_OKAY);

	// These channels always had the older headphone technology off
	if (comLongIntWrite(cast_handle->com_hdl_rear, DSP_PLAY_HEADPHONE_ON, 0) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_side, DSP_PLAY_HEADPHONE_ON, 0) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_center, DSP_PLAY_HEADPHONE_ON, 0) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_subwoofer, DSP_PLAY_HEADPHONE_ON, 0) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateFidelity()
 * DESCRIPTION:
 */
int dfxp_CommunicateFidelity(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_fidelity;
	realtype dsp_fidelity;
	int music_mode;

   if (dfxp_GetKnobValue_MIDI(hp_dfxp, DFX_UI_KNOB_FIDELITY, &pc_fidelity) != OKAY)
		return(NOT_OKAY);

	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_MUSIC_MODE, &music_mode) != OKAY)
		return(NOT_OKAY);

	if (music_mode == DFX_UI_MUSIC_MODE_SPEECH)
	{
		pc_fidelity = (int)(pc_fidelity * DFXP_SPEECH_MODE_FIDELITY_FACTOR);
		if (pc_fidelity > MIDI_MAX_VALUE)
			pc_fidelity = MIDI_MAX_VALUE;
	}
	
	/* Calculate the value that should be sent to the dsp card */
	if (qntIToRCalc(cast_handle->midi_to_dsp.fidelity_qnt_hdl, 
		             pc_fidelity, 
		             &dsp_fidelity) != OKAY)
		return(NOT_OKAY);

	// Note every channel gets normal fidelity except for subwoofer channel, which will always be bypassed
	if (comRealWrite(cast_handle->com_hdl_front, AURAL_DRIVE + DSP_PLAY_AURAL_PARAM_OFFSET, 
		              dsp_fidelity) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, AURAL_DRIVE + DSP_PLAY_AURAL_PARAM_OFFSET, 
		              dsp_fidelity) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, AURAL_DRIVE + DSP_PLAY_AURAL_PARAM_OFFSET, 
		              dsp_fidelity) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, AURAL_DRIVE + DSP_PLAY_AURAL_PARAM_OFFSET, 
		              dsp_fidelity) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateAmbience()
 * DESCRIPTION:
 */ 
int dfxp_CommunicateAmbience(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_liveliness;
	realtype dsp_decay;
	realtype dsp_lat6_coeff;
	int pc_size;
	realtype roomsize;
	realtype wet_gain;
	realtype dry_gain;
	int music_mode;

   if (dfxp_GetKnobValue_MIDI(hp_dfxp, DFX_UI_KNOB_AMBIENCE, &pc_liveliness) != OKAY)
		return(NOT_OKAY);

	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_MUSIC_MODE, &music_mode) != OKAY)
		return(NOT_OKAY);

	/* For MUSIC2 and SPEECH modes warp ambience to very low level */
	if (music_mode != DFX_UI_MUSIC_MODE_MUSIC1)
		pc_liveliness = (int)((realtype)pc_liveliness * DFXP_MUSIC_MODE2_AMBIENCE_FACTOR);

	/* Hardcode the room size */
	pc_size = DSP_PLAY_LEX_ROOM_SIZE_MIDI;

	/* Get the roomsize value so we can compensate the liveliness */
	if (qntIToRCalc(cast_handle->midi_to_dsp.room_size_qnt_hdl, pc_size, &roomsize) != OKAY)
		return(NOT_OKAY);

	/* Calculate the values that should be sent to the dsp card */
	if (qntIToRCalc(cast_handle->midi_to_dsp.ambience_qnt_hdl, pc_liveliness, &dsp_decay) != OKAY)
		return(NOT_OKAY);

	/* Now compensate decay for roomsize */
	dsp_decay = (realtype)pow(dsp_decay, roomsize);

	dsp_lat6_coeff = dsp_decay + (realtype)0.15;
	if( dsp_lat6_coeff < (realtype)0.25 )
		dsp_lat6_coeff = (realtype)0.25;
	if( dsp_lat6_coeff > (realtype)0.5 )
		dsp_lat6_coeff = (realtype)0.5;

	// For lower setting of ambience, warp wet/dry settings
	// Note that 0 setting covers value from 0 to 12 and ambience is bypassed for 12 and below
	if( pc_liveliness > 40 )
	{
		// Note that these are the fixed values used in version 6 and earlier
		wet_gain = (realtype)(0.21 * 1.3);
		dry_gain = (realtype)(0.69 * 1.3);
	}
	else
	{
		// Do warping so that at low pc_liveness wet is tending to zero.
		wet_gain = (realtype)((pc_liveliness - 12) * (1.0/(40 - 12)) * (0.21 * 1.3));
		dry_gain = (realtype)0.897 + (realtype)((40 - pc_liveliness) * (1.0/(40 - 12))) * (realtype)(1.0 - 0.897);
	}
		
	// Ambience will always be bypassed on subwoofer channel, so don't send to that one
	if (comRealWrite(cast_handle->com_hdl_front, DRY_GAIN + DSP_PLAY_LEX_PARAM_OFFSET, dry_gain) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, DRY_GAIN + DSP_PLAY_LEX_PARAM_OFFSET, dry_gain) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, DRY_GAIN + DSP_PLAY_LEX_PARAM_OFFSET, dry_gain) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, DRY_GAIN + DSP_PLAY_LEX_PARAM_OFFSET, dry_gain) != OKAY)
		return(NOT_OKAY);

	if (comRealWrite(cast_handle->com_hdl_front, WET_GAIN + DSP_PLAY_LEX_PARAM_OFFSET, wet_gain) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, WET_GAIN + DSP_PLAY_LEX_PARAM_OFFSET, wet_gain) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, WET_GAIN + DSP_PLAY_LEX_PARAM_OFFSET, wet_gain) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, WET_GAIN + DSP_PLAY_LEX_PARAM_OFFSET, wet_gain) != OKAY)
		return(NOT_OKAY);

	if (comRealWrite(cast_handle->com_hdl_front, LEX_DECAY + DSP_PLAY_LEX_PARAM_OFFSET, dsp_decay) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, LEX_DECAY + DSP_PLAY_LEX_PARAM_OFFSET, dsp_decay) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, LEX_DECAY + DSP_PLAY_LEX_PARAM_OFFSET, dsp_decay) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, LEX_DECAY + DSP_PLAY_LEX_PARAM_OFFSET, dsp_decay) != OKAY)
		return(NOT_OKAY);

	if (comRealWrite(cast_handle->com_hdl_front, LEX_LAT6_COEFF + DSP_PLAY_LEX_PARAM_OFFSET, dsp_lat6_coeff) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, LEX_LAT6_COEFF + DSP_PLAY_LEX_PARAM_OFFSET, dsp_lat6_coeff) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, LEX_LAT6_COEFF + DSP_PLAY_LEX_PARAM_OFFSET, dsp_lat6_coeff) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, LEX_LAT6_COEFF + DSP_PLAY_LEX_PARAM_OFFSET, dsp_lat6_coeff) != OKAY)
		return(NOT_OKAY);
	
	/* 
	 * Due to the fact that the bypass for Ambience also depends on the
	 * knob setting, we must send the correct bypass setting.
	 */
   if (dfxp_CommAmbienceBypass(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
} 

/*
 * FUNCTION: dfxp_CommunicateDynamicBoost()
 * DESCRIPTION:
 *   Communicates to the dsp card the current gain boost value
 */
int dfxp_CommunicateDynamicBoost(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_gain_boost;
	int max_delay;
	realtype dsp_gain_boost;
	int bypass_all;
	int music_mode;
	int dynamic_boost_on;
	int bypass_dynamic_boost;

   if (dfxp_GetKnobValue_MIDI(hp_dfxp, DFX_UI_KNOB_DYNAMIC_BOOST, &pc_gain_boost) != OKAY)
		return(NOT_OKAY);

	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_MUSIC_MODE, &music_mode) != OKAY)
		return(NOT_OKAY);

	switch(music_mode)
	{
	case  DFX_UI_MUSIC_MODE_MUSIC2 :
		pc_gain_boost = (int)((realtype)(DFXP_MUSIC_MODE2_DYNAMIC_BOOST_FACTOR) * pc_gain_boost);
		break;
	case DFX_UI_MUSIC_MODE_SPEECH :
		pc_gain_boost = (int)((realtype)(DFXP_SPEECH_MODE_DYNAMIC_BOOST_FACTOR) * pc_gain_boost);
		break;
	}

	/* Limit warped gain boost */
	if( pc_gain_boost > MIDI_MAX_VALUE )
			pc_gain_boost = MIDI_MAX_VALUE;

	/* Get the bypass settings */;
	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_BYPASS, &bypass_all) != OKAY)
		return(NOT_OKAY);

	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_DYNAMIC_BOOST, &dynamic_boost_on) != OKAY)
		return(NOT_OKAY);
	bypass_dynamic_boost = !dynamic_boost_on;

	/* Check if we should bypass */
   if ((bypass_dynamic_boost) || bypass_all)
		pc_gain_boost = MIDI_MIN_VALUE;

	/* Take care of overriding the processing */
	if ((cast_handle->processing_override == DFXP_PROCESSING_OVERRIDE_DFX_ALL_EXCEPT_OPTIMIZER) ||
		 (cast_handle->processing_override == DFXP_PROCESSING_OVERRIDE_DYNAMIC_BOOST))
      pc_gain_boost = MIDI_MIN_VALUE;


	/* Scale control value directly for now */
	pc_gain_boost = (int)((realtype)pc_gain_boost * (realtype)PLY_OPTIMIZER_BOOST_MAX_SCALE);

	/* Calculate the value that should be sent to the dsp card */
	if (qntIToRCalc(cast_handle->midi_to_dsp.dynamic_boost_qnt_hdl, pc_gain_boost, &dsp_gain_boost) != OKAY)
		return(NOT_OKAY);

	if (comRealWrite(cast_handle->com_hdl_front, MAXIMIZE_GAIN_BOOST + DSP_PLAY_OPTIMIZER_PARAM_OFFSET, 
		              dsp_gain_boost) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, MAXIMIZE_GAIN_BOOST + DSP_PLAY_OPTIMIZER_PARAM_OFFSET, 
		              dsp_gain_boost) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, MAXIMIZE_GAIN_BOOST + DSP_PLAY_OPTIMIZER_PARAM_OFFSET, 
		              dsp_gain_boost) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, MAXIMIZE_GAIN_BOOST + DSP_PLAY_OPTIMIZER_PARAM_OFFSET, 
		              dsp_gain_boost) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_subwoofer, MAXIMIZE_GAIN_BOOST + DSP_PLAY_OPTIMIZER_PARAM_OFFSET, 
		              dsp_gain_boost) != OKAY)
		return(NOT_OKAY);

	/* Set delay to desired lookahead time. Needs special init in PC code. */
	max_delay = (int)(cast_handle->internal_sampling_freq * (realtype)MAXI_LOOK_AHEAD_DELAY);

	if (comLongIntWrite(cast_handle->com_hdl_front, MAXIMIZE_MAX_DELAY + DSP_PLAY_OPTIMIZER_PARAM_OFFSET, max_delay) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_rear, MAXIMIZE_MAX_DELAY + DSP_PLAY_OPTIMIZER_PARAM_OFFSET, max_delay) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_side, MAXIMIZE_MAX_DELAY + DSP_PLAY_OPTIMIZER_PARAM_OFFSET, max_delay) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_center, MAXIMIZE_MAX_DELAY + DSP_PLAY_OPTIMIZER_PARAM_OFFSET, max_delay) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_subwoofer, MAXIMIZE_MAX_DELAY + DSP_PLAY_OPTIMIZER_PARAM_OFFSET, max_delay) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateSpaciousness()
 * DESCRIPTION:
 *   Communicates to the dsp card the current spaciousness value
 */
int dfxp_CommunicateSpaciousness(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_intensity;
	realtype dsp_intensity;

   if (dfxp_GetKnobValue_MIDI(hp_dfxp, DFX_UI_KNOB_SURROUND, &pc_intensity) != OKAY)
		return(NOT_OKAY);

	/* Calculate the values that should be sent to the dsp card */
	if (qntIToRCalc(cast_handle->midi_to_dsp.spaciousness_qnt_hdl, pc_intensity, 
		             &dsp_intensity) != OKAY)
	 return(NOT_OKAY);

	// Only front, rear and side will have spacialization active, others bypassed
	if (comRealWrite(cast_handle->com_hdl_front, DSP_WID_INTENSITY + DSP_PLAY_WIDENER_PARAM_OFFSET, 
		              dsp_intensity) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, DSP_WID_INTENSITY + DSP_PLAY_WIDENER_PARAM_OFFSET, 
		              dsp_intensity) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, DSP_WID_INTENSITY + DSP_PLAY_WIDENER_PARAM_OFFSET, 
		              dsp_intensity) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateVocalReduction()
 * DESCRIPTION:
 *   Communicates to the dsp card the current vocal reduction
 */
int dfxp_CommunicateVocalReduction(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int i_dsp_vocal_reduction_val;
	int vocal_reduction_mode;

	/* 
	 * This is a special case in which an int value is passed to the dsp based on the
	 * midi value because this is the one knob that is quantized.
	 */
   if (dfxp_GetKnobValue_MIDI(hp_dfxp, DFX_UI_KNOB_VOCAL_REDUCTION, &i_dsp_vocal_reduction_val) != OKAY)
		return(NOT_OKAY);

	/* Send vocal reduction parameter */
	// Only sent to the front DSP processor
	if (comLongIntWrite(cast_handle->com_hdl_front, DSP_VOCAL_REDUCTION_VAL, 
		                 i_dsp_vocal_reduction_val) != OKAY)
		return(NOT_OKAY);

	/* 
	 * Send whether it is in Male or Female mode.
	 *
	 * NOTE: If DFX_UI_VOCAL_REDUCTION_MODE_1 is Male and DFX_UI_VOCAL_REDUCTION_MODE_2 is Female.
	 * Note DSP uses mode values starting at zero.
	 */
   if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_VOCAL_REDUCTION_MODE, &vocal_reduction_mode) != OKAY)
		return(NOT_OKAY);

	if (comLongIntWrite(cast_handle->com_hdl_front, DSP_PLAY_VOCAL_REDUCTION_MODE, 
		                  (vocal_reduction_mode - 1) ) != OKAY)
	   return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateBassBoost()
 * DESCRIPTION:
 *   Communicates to the dsp card the current bass boost value
 */
int dfxp_CommunicateBassBoost(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;
	PT_HANDLE *com_hdl;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_bass_boost;
	struct filt2ndOrderBoostCutShelfFilterType filt;

	int music_mode;

   if (dfxp_GetKnobValue_MIDI(hp_dfxp, DFX_UI_KNOB_BASS_BOOST, &pc_bass_boost) != OKAY)
		return(NOT_OKAY);

	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_MUSIC_MODE, &music_mode) != OKAY)
		return(NOT_OKAY);

	switch (music_mode)
	{
		case DFX_UI_MUSIC_MODE_SPEECH :
			pc_bass_boost = (int)((realtype)(DFXP_SPEECH_MODE_BASS_BOOST_FACTOR) * pc_bass_boost);
			break;
	}

	if ( qntIToBoostCutCalc(cast_handle->midi_to_dsp.bass_boost_qnt_hdl, pc_bass_boost,
											  &filt) != OKAY)
		return(NOT_OKAY);

	/* Write filter coeffs to front channels. */
	com_hdl = cast_handle->com_hdl_front;

	if (comRealWrite(com_hdl, DSP_PLAY_B0, 
		              filt.b0) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(com_hdl, DSP_PLAY_B1, 
		              filt.b1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(com_hdl, DSP_PLAY_B2, 
		              filt.b2) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(com_hdl, DSP_PLAY_A1, 
		              filt.a1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(com_hdl, DSP_PLAY_A2, 
		              filt.a2) != OKAY)
		return(NOT_OKAY);

	/* Write filter coeffs to subwoofer also if in surround sound mode */
	if( cast_handle->num_channels_out > 2 )
		com_hdl = cast_handle->com_hdl_subwoofer;

	if (comRealWrite(com_hdl, DSP_PLAY_B0, 
		              filt.b0) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(com_hdl, DSP_PLAY_B1, 
		              filt.b1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(com_hdl, DSP_PLAY_B2, 
		              filt.b2) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(com_hdl, DSP_PLAY_A1, 
		              filt.a1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(com_hdl, DSP_PLAY_A2, 
		              filt.a2) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateFixedQnts_Aural()
 * DESCRIPTION:
 *   Communicates the qnt values of the Aural Activation which
 *   do not change when the knob is changed.
 */
int dfxp_CommunicateFixedQnts_Aural(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if (dfxp_AuralCommunicateTune(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateFixedQnts_Lex()
 * DESCRIPTION:
 *   Communicates the qnt values of the Reverb which
 *   do not change when the knob is changed.
 */
int dfxp_CommunicateFixedQnts_Lex(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   if (dfxp_LexCommunicateSize(hp_dfxp) != OKAY)
      return(NOT_OKAY);
   if (dfxp_LexCommunicateRolloff(hp_dfxp) != OKAY)
      return(NOT_OKAY);   
   if (dfxp_LexCommunicateDamping(hp_dfxp) != OKAY)
      return(NOT_OKAY);	
   if (dfxp_LexCommunicateRate(hp_dfxp) != OKAY)
      return(NOT_OKAY);
   if (dfxp_LexCommunicateDepth(hp_dfxp) != OKAY)
      return(NOT_OKAY);

   return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateFixedQnts_Opt()
 * DESCRIPTION:
 *   Communicates the qnt values of the Optimizer which
 *   do not change when the knob is changed.
 */
int dfxp_CommunicateFixedQnts_Opt(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* Send the release time */
   if (dfxp_MaxCommunicateReleaseTime(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	/* Write the setting for the target boosted output level. Note that this
	 * doesn't use a quant function now, but likely will be enhanced to use
	 * a quant function in the DSP-FX version of the new optimizer.
	 */
	if (comRealWrite(cast_handle->com_hdl_front, (MAXIMIZE_TARGET_LEVEL + DSP_PLAY_OPTIMIZER_PARAM_OFFSET), (realtype)MAXIMIZE_TARGET_LEVEL_SETTING) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, (MAXIMIZE_TARGET_LEVEL + DSP_PLAY_OPTIMIZER_PARAM_OFFSET), (realtype)MAXIMIZE_TARGET_LEVEL_SETTING) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, (MAXIMIZE_TARGET_LEVEL + DSP_PLAY_OPTIMIZER_PARAM_OFFSET), (realtype)MAXIMIZE_TARGET_LEVEL_SETTING) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, (MAXIMIZE_TARGET_LEVEL + DSP_PLAY_OPTIMIZER_PARAM_OFFSET), (realtype)MAXIMIZE_TARGET_LEVEL_SETTING) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_subwoofer, (MAXIMIZE_TARGET_LEVEL + DSP_PLAY_OPTIMIZER_PARAM_OFFSET), (realtype)MAXIMIZE_TARGET_LEVEL_SETTING) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateFixedQnts_Wid()
 * DESCRIPTION:
 *   Communicates the qnt values of the Widener which
 *   do not change when the knob is changed.
 */
int dfxp_CommunicateFixedQnts_Wid(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   if (dfxp_WidCommunicateFreqThreshold(hp_dfxp) != OKAY)
      return(NOT_OKAY);

   if (dfxp_WidCommunicateDispersion(hp_dfxp) != OKAY)
      return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateFixedQnts_Play()
 * DESCRIPTION:
 *   Communicates the qnt values of the Player which
 *   do not change when the knob is changed.
 */
int dfxp_CommunicateFixedQnts_Play(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

/* These filters are no longer used in new processing, keep in case we
 * need them later.
 */
#ifdef DSP_ORIGINAL_HEADPHONE

	/* Currently calculates and sets filter coeffs for 5k peak and 10k notch
	 * for headphone processing.
	 */
	struct filt2ndOrderBoostCutShelfFilterType filt;
	/* Boost - 5000 hz, 8.5 db boost, Q = 5

	Cut - 10000 hz, -12 db cut, Q = 8.8
	 */

	filt.r_samp_freq = cast_handle->internal_sampling_freq;
	filt.r_center_freq = (realtype)5000.0;
/*
	filt.boost = (realtype)(8.5);
	filt.Q = (realtype)5.0;
 */

	filt.boost = (realtype)(3.00);
	filt.Q = (realtype)2.5;

   /* Calculate the filter section */
	if (filtCalcParametric(&filt) != OKAY)
		return(NOT_OKAY);

	/* Write filter coeffs */
	if (comRealWrite(cast_handle->com_hdl, DSP_PLAY_B0_5K, 
		              filt.b0) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl, DSP_PLAY_B1_5K, 
		              filt.b1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl, DSP_PLAY_B2_5K, 
		              filt.b2) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl, DSP_PLAY_A1_5K, 
		              filt.a1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl, DSP_PLAY_A2_5K, 
		              filt.a2) != OKAY)
		return(NOT_OKAY);

	filt.r_samp_freq = cast_handle->internal_sampling_freq;
	filt.r_center_freq = (realtype)10000.0;
/*
	filt.boost = (realtype)(-12.0);
	filt.Q = (realtype)8.8;
 */

	filt.boost = (realtype)(-6.0);
	filt.Q = (realtype)8.8;

   /* Calculate the filter section */
	if (filtCalcParametric(&filt) != OKAY)
		return(NOT_OKAY);

	/* Write filter coeffs */
	if (comRealWrite(cast_handle->com_hdl, DSP_PLAY_B0_10K, 
		              filt.b0) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl, DSP_PLAY_B1_10K, 
		              filt.b1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl, DSP_PLAY_B2_10K, 
		              filt.b2) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl, DSP_PLAY_A1_10K, 
		              filt.a1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl, DSP_PLAY_A2_10K, 
		              filt.a2) != OKAY)
		return(NOT_OKAY);

#endif /* DSP_ORIGINAL_HEADPHONE */

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateFixedQnts_Delay()
 * DESCRIPTION:
 *   Communicates the qnt values of the multi-tap delay which
 *   do not change when any knobs are changed.
 */
int dfxp_CommunicateFixedQnts_Delay(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	long dsp_total_dly; 
	int i;
	realtype r_total_delay[DSP_MAX_NUM_OF_DELAY_ELEMENTS];
	realtype r_pan_setting[DSP_MAX_NUM_OF_DELAY_ELEMENTS];

	/* Set all the element gains to 1.0 */
	for(i=0; i<DSP_MAX_NUM_OF_DELAY_ELEMENTS; i++)
	{
		if (comRealWrite(cast_handle->com_hdl_front, DSP_PLAY_DELAY_PARAM_OFFSET + ELEM0_GAIN + i, (realtype)1.0) != OKAY)
			return(NOT_OKAY);
	}

	/* Set delay values */
	r_total_delay[0] = (realtype) (PLY_HEADPHONE_DELAY0 * PLY_DELAY_FACTOR);
	r_total_delay[1] = (realtype) (PLY_HEADPHONE_DELAY1 * PLY_DELAY_FACTOR);
	r_total_delay[2] = (realtype) (PLY_HEADPHONE_DELAY2 * PLY_DELAY_FACTOR);
	r_total_delay[3] = (realtype) (PLY_HEADPHONE_DELAY3 * PLY_DELAY_FACTOR);
	r_total_delay[4] = (realtype) (PLY_HEADPHONE_DELAY4 * PLY_DELAY_FACTOR);
	r_total_delay[5] = (realtype) (PLY_HEADPHONE_DELAY5 * PLY_DELAY_FACTOR);
	r_total_delay[6] = (realtype) (PLY_HEADPHONE_DELAY6 * PLY_DELAY_FACTOR);
	r_total_delay[7] = (realtype) (PLY_HEADPHONE_DELAY7 * PLY_DELAY_FACTOR);

	for(i=0; i<DSP_MAX_NUM_OF_DELAY_ELEMENTS; i++)
	{
		/* Calculate the value that should be sent to the dsp card */
		if (qntRToLCalc(cast_handle->midi_to_dsp.dly_qnt_hdl, r_total_delay[i], &dsp_total_dly) != OKAY)
			return(NOT_OKAY);
    
		 /* Adding one to calculated delay because using NoPop delay algorithm */
		if (comLongIntWrite(cast_handle->com_hdl_front, DSP_PLAY_DELAY_PARAM_OFFSET + ELEM0_DELAY + i,
							  dsp_total_dly + 1) != OKAY)
			return(NOT_OKAY);
	}

	/* Set pan values */
	r_pan_setting[0] = (realtype) PLY_HEADPHONE_PAN_SETTING0;
	r_pan_setting[1] = (realtype) PLY_HEADPHONE_PAN_SETTING1;
	r_pan_setting[2] = (realtype) PLY_HEADPHONE_PAN_SETTING2;
	r_pan_setting[3] = (realtype) PLY_HEADPHONE_PAN_SETTING3;
	r_pan_setting[4] = (realtype) PLY_HEADPHONE_PAN_SETTING4;
	r_pan_setting[5] = (realtype) PLY_HEADPHONE_PAN_SETTING5;
	r_pan_setting[6] = (realtype) PLY_HEADPHONE_PAN_SETTING6;
	r_pan_setting[7] = (realtype) PLY_HEADPHONE_PAN_SETTING7;
	for(i=0; i<DSP_MAX_NUM_OF_DELAY_ELEMENTS; i++)
	{
		realtype left_gain, right_gain;

		/* Note that display pan setting go from -1 -> left to 1 -> right */
		left_gain = (r_pan_setting[i] - (realtype)1.0) * (realtype)-0.5;
		right_gain = (r_pan_setting[i] + (realtype)1.0) * (realtype)0.5;

		if (comRealWrite(cast_handle->com_hdl_front, DSP_PLAY_DELAY_PARAM_OFFSET + ELEM0_PAN_GAIN_LEFT + i,
						  left_gain) != OKAY)
			return(NOT_OKAY);

		if (comRealWrite(cast_handle->com_hdl_front, DSP_PLAY_DELAY_PARAM_OFFSET + ELEM0_PAN_GAIN_RIGHT + i,
						  right_gain) != OKAY)
			return(NOT_OKAY);
	}

	/*
	 * Do the wet, dry and master gains.
	 */
	{
		realtype dsp_dry, dsp_wet;

		dsp_dry = (realtype) PLY_HEADPHONE_DRY;
		dsp_wet = (realtype) PLY_HEADPHONE_WET;

		if (comRealWrite(cast_handle->com_hdl_front, DSP_PLAY_DELAY_PARAM_OFFSET + DRY_GAIN, dsp_dry) != OKAY)
			return(NOT_OKAY);
		if (comRealWrite(cast_handle->com_hdl_front, DSP_PLAY_DELAY_PARAM_OFFSET + WET_GAIN, dsp_wet) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: dfxp_AuralCommunicateTune()
 * DESCRIPTION:
 *   Communicates to the dsp card the current tune value.
 */
int dfxp_AuralCommunicateTune(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_tune;
	realtype filter_gain, filter_a1, filter_a0;

	pc_tune = DSP_PLAY_AURAL_TUNE_MIDI;

	/* Calculate the values that should be sent to the dsp card */
	if (qntIToRCalc(cast_handle->midi_to_dsp.aural_filter_gain_qnt_hdl, pc_tune, &filter_gain) != OKAY)
		return(NOT_OKAY);

	// Not active in subwoofer channel
	if (comRealWrite(cast_handle->com_hdl_front, (AURAL_FILTER_GAIN + DSP_PLAY_AURAL_PARAM_OFFSET), filter_gain) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, (AURAL_FILTER_GAIN + DSP_PLAY_AURAL_PARAM_OFFSET), filter_gain) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, (AURAL_FILTER_GAIN + DSP_PLAY_AURAL_PARAM_OFFSET), filter_gain) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, (AURAL_FILTER_GAIN + DSP_PLAY_AURAL_PARAM_OFFSET), filter_gain) != OKAY)
		return(NOT_OKAY);

	if (qntIToRCalc(cast_handle->midi_to_dsp.aural_filter_a1_qnt_hdl, pc_tune, &filter_a1) != OKAY)
		return(NOT_OKAY);

	if (comRealWrite(cast_handle->com_hdl_front, (AURAL_FILTER_A1 + DSP_PLAY_AURAL_PARAM_OFFSET), 
		              filter_a1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, (AURAL_FILTER_A1 + DSP_PLAY_AURAL_PARAM_OFFSET), 
		              filter_a1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, (AURAL_FILTER_A1 + DSP_PLAY_AURAL_PARAM_OFFSET), 
		              filter_a1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, (AURAL_FILTER_A1 + DSP_PLAY_AURAL_PARAM_OFFSET), 
		              filter_a1) != OKAY)
		return(NOT_OKAY);

	if (qntIToRCalc(cast_handle->midi_to_dsp.aural_filter_a0_qnt_hdl, pc_tune, &filter_a0) != OKAY)
		return(NOT_OKAY);

	if (comRealWrite(cast_handle->com_hdl_front, (AURAL_FILTER_A0 + DSP_PLAY_AURAL_PARAM_OFFSET), filter_a0) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, (AURAL_FILTER_A0 + DSP_PLAY_AURAL_PARAM_OFFSET), filter_a0) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, (AURAL_FILTER_A0 + DSP_PLAY_AURAL_PARAM_OFFSET), filter_a0) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, (AURAL_FILTER_A0 + DSP_PLAY_AURAL_PARAM_OFFSET), filter_a0) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_LexCommunicateSize()
 * DESCRIPTION:
 *   Communicates the Size
 */ 
int dfxp_LexCommunicateSize(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_size;
	realtype roomsize;

   pc_size = DSP_PLAY_LEX_ROOM_SIZE_MIDI;

	/* Calculate the value that should be sent to the dsp card */
	if (qntIToRCalc(cast_handle->midi_to_dsp.room_size_qnt_hdl, pc_size, &roomsize) != OKAY)
		return(NOT_OKAY);

	// Ambience is only active in front, rear and center
	if (comRealWrite(cast_handle->com_hdl_front, LEX_ROOM_SIZE + DSP_PLAY_LEX_PARAM_OFFSET, roomsize) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, LEX_ROOM_SIZE + DSP_PLAY_LEX_PARAM_OFFSET, roomsize) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, LEX_ROOM_SIZE + DSP_PLAY_LEX_PARAM_OFFSET, roomsize) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, LEX_ROOM_SIZE + DSP_PLAY_LEX_PARAM_OFFSET, roomsize) != OKAY)
		return(NOT_OKAY);

	/* After roomsize has been sent, we need to make the DSP reinitialize all the memory pointers. */
	if (comMemReInitialize(cast_handle->com_hdl_front, cast_handle->internal_sampling_freq) != OKAY)
		return(NOT_OKAY);
	if (comMemReInitialize(cast_handle->com_hdl_rear, cast_handle->internal_sampling_freq) != OKAY)
		return(NOT_OKAY);
	if (comMemReInitialize(cast_handle->com_hdl_side, cast_handle->internal_sampling_freq) != OKAY)
		return(NOT_OKAY);
	if (comMemReInitialize(cast_handle->com_hdl_center, cast_handle->internal_sampling_freq) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
} 

/*
 * FUNCTION: dfxp_LexCommunicateRolloff()
 * DESCRIPTION:
 *   Communicates the high frequency rolloff
 */ 
int dfxp_LexCommunicateRolloff(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_rolloff;
	realtype dsp_rolloff;

   pc_rolloff = DSP_PLAY_LEX_ROLLOFF_MIDI;

	/* Calculate the values that should be sent to the dsp card */	
	if (qntIToRCalc(cast_handle->midi_to_dsp.rolloff_bandwidth_qnt_hdl, pc_rolloff, &dsp_rolloff) != OKAY)
		return(NOT_OKAY);

	if (comRealWrite(cast_handle->com_hdl_front, (LEX_ROLLOFF + DSP_PLAY_LEX_PARAM_OFFSET), dsp_rolloff) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, (LEX_ROLLOFF + DSP_PLAY_LEX_PARAM_OFFSET), dsp_rolloff) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, (LEX_ROLLOFF + DSP_PLAY_LEX_PARAM_OFFSET), dsp_rolloff) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, (LEX_ROLLOFF + DSP_PLAY_LEX_PARAM_OFFSET), dsp_rolloff) != OKAY)
		return(NOT_OKAY);

	if (comRealWrite(cast_handle->com_hdl_front, (LEX_ONE_MINUS_ROLLOFF + DSP_PLAY_LEX_PARAM_OFFSET), ((realtype)1.0 - dsp_rolloff)) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, (LEX_ONE_MINUS_ROLLOFF + DSP_PLAY_LEX_PARAM_OFFSET), ((realtype)1.0 - dsp_rolloff)) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, (LEX_ONE_MINUS_ROLLOFF + DSP_PLAY_LEX_PARAM_OFFSET), ((realtype)1.0 - dsp_rolloff)) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, (LEX_ONE_MINUS_ROLLOFF + DSP_PLAY_LEX_PARAM_OFFSET), ((realtype)1.0 - dsp_rolloff)) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
} 

/*
 * FUNCTION: dfxp_LexCommunicateDamping()
 * DESCRIPTION:
 *   Communicates the high frequency damping.
 */ 
int dfxp_LexCommunicateDamping(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_damping;
	realtype dsp_damping;

   pc_damping = DSP_PLAY_LEX_DAMPING_MIDI;

	/* Calculate the values that should be sent to the dsp card */
	if (qntIToRCalc(cast_handle->midi_to_dsp.damping_bandwidth_qnt_hdl, pc_damping, &dsp_damping) != OKAY)
		return(NOT_OKAY);

	// Ambience is only active in front, rear and center
	if (comRealWrite(cast_handle->com_hdl_front, (LEX_DAMPING + DSP_PLAY_LEX_PARAM_OFFSET), dsp_damping) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, (LEX_DAMPING + DSP_PLAY_LEX_PARAM_OFFSET), dsp_damping) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, (LEX_DAMPING + DSP_PLAY_LEX_PARAM_OFFSET), dsp_damping) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, (LEX_DAMPING + DSP_PLAY_LEX_PARAM_OFFSET), dsp_damping) != OKAY)
		return(NOT_OKAY);

	if (comRealWrite(cast_handle->com_hdl_front, (LEX_ONE_MINUS_DAMPING + DSP_PLAY_LEX_PARAM_OFFSET), ((realtype)1.0 - dsp_damping)) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, (LEX_ONE_MINUS_DAMPING + DSP_PLAY_LEX_PARAM_OFFSET), ((realtype)1.0 - dsp_damping)) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, (LEX_ONE_MINUS_DAMPING + DSP_PLAY_LEX_PARAM_OFFSET), ((realtype)1.0 - dsp_damping)) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, (LEX_ONE_MINUS_DAMPING + DSP_PLAY_LEX_PARAM_OFFSET), ((realtype)1.0 - dsp_damping)) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
} 

/*
 * FUNCTION:dfxp_LexCommunicateDepth()
 * DESCRIPTION:
 *   Communicates the motion depth.
 */ 
int dfxp_LexCommunicateDepth(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_depth;
	realtype dsp_depth;

   pc_depth = DSP_PLAY_LEX_DEPTH_MIDI;	

	/* Calculate the value that should be sent to the dsp card */
	if (qntIToRCalc(cast_handle->midi_to_dsp.motion_depth_qnt_hdl, pc_depth, &dsp_depth) != OKAY)
		return(NOT_OKAY);

	// Ambience is only active in front, rear and center
	if (comRealWrite(cast_handle->com_hdl_front, (LEX_MOTION_DEPTH + DSP_PLAY_LEX_PARAM_OFFSET), dsp_depth) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, (LEX_MOTION_DEPTH + DSP_PLAY_LEX_PARAM_OFFSET), dsp_depth) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, (LEX_MOTION_DEPTH + DSP_PLAY_LEX_PARAM_OFFSET), dsp_depth) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, (LEX_MOTION_DEPTH + DSP_PLAY_LEX_PARAM_OFFSET), dsp_depth) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
} 

/*
 * FUNCTION: dfxp_LexCommunicateRate()
 * DESCRIPTION:
 *   Communicates the motion rate.
 */ 
int dfxp_LexCommunicateRate(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_rate;
	realtype dsp_rate;

   pc_rate = DSP_PLAY_LEX_RATE_MIDI;	

	/* Calculate the value that should be sent to the dsp card */
	if (qntIToRCalc(cast_handle->midi_to_dsp.motion_rate_qnt_hdl, pc_rate, &dsp_rate) != OKAY)
		return(NOT_OKAY);

	// Ambience is only active in front, rear and center
	if (comRealWrite(cast_handle->com_hdl_front, (LEX_MOTION_RATE + DSP_PLAY_LEX_PARAM_OFFSET), dsp_rate) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, (LEX_MOTION_RATE + DSP_PLAY_LEX_PARAM_OFFSET), dsp_rate) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, (LEX_MOTION_RATE + DSP_PLAY_LEX_PARAM_OFFSET), dsp_rate) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, (LEX_MOTION_RATE + DSP_PLAY_LEX_PARAM_OFFSET), dsp_rate) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_MaxCommunicateReleaseTime()
 * DESCRIPTION:
 *   Communicates to the dsp card the current release time value
 *   This value is converted from a time constant to an exponential beta val.
 */
int dfxp_MaxCommunicateReleaseTime(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_release_time;
	realtype dsp_release_time_beta;

   pc_release_time = DSP_PLAY_MAX_RELEASE_TIME_BETA_MIDI;	

	/* Calculate the value that should be sent to the dsp card */
	if (qntIToRCalc(cast_handle->midi_to_dsp.release_time_beta_qnt_hdl, pc_release_time, &dsp_release_time_beta) != OKAY)
		return(NOT_OKAY);

	if (comRealWrite(cast_handle->com_hdl_front, (MAXIMIZE_RELEASE_TIME + DSP_PLAY_OPTIMIZER_PARAM_OFFSET), dsp_release_time_beta) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, (MAXIMIZE_RELEASE_TIME + DSP_PLAY_OPTIMIZER_PARAM_OFFSET), dsp_release_time_beta) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, (MAXIMIZE_RELEASE_TIME + DSP_PLAY_OPTIMIZER_PARAM_OFFSET), dsp_release_time_beta) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_center, (MAXIMIZE_RELEASE_TIME + DSP_PLAY_OPTIMIZER_PARAM_OFFSET), dsp_release_time_beta) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_subwoofer, (MAXIMIZE_RELEASE_TIME + DSP_PLAY_OPTIMIZER_PARAM_OFFSET), dsp_release_time_beta) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_WidCommunicateDispersion()
 * DESCRIPTION:
 *   Communicate Dispersion
 */ 
int dfxp_WidCommunicateDispersion(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_dispersion;
	long dsp_dispersion_left;
	long dsp_dispersion_right;

   pc_dispersion = DSP_PLAY_WID_DISPERSION_MIDI;

	/* Calculate the right dispersion value that should be sent to the dsp card.
	 * This is directly from the display value.
	 */
	if (qntIToLCalc(cast_handle->midi_to_dsp.dispersion_delay_qnt_hdl, pc_dispersion, &dsp_dispersion_right) != OKAY)
		return(NOT_OKAY);
	/* Calculate the left dispersion value that should be sent to the dsp card.
	 * This is factored down from the right value.
	 */
	dsp_dispersion_left = (long)(dsp_dispersion_right * (realtype)DSP_WID_LEFT_RIGHT_DISPERSION_FACTOR);

	/* Write both values */
	// Widener is only active in front and rear dsp processors.
	if (comLongIntWrite(cast_handle->com_hdl_front, DSP_WID_DISPERSION_LEFT + DSP_PLAY_WIDENER_PARAM_OFFSET, dsp_dispersion_left) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_rear, DSP_WID_DISPERSION_LEFT + DSP_PLAY_WIDENER_PARAM_OFFSET, dsp_dispersion_left) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_side, DSP_WID_DISPERSION_LEFT + DSP_PLAY_WIDENER_PARAM_OFFSET, dsp_dispersion_left) != OKAY)
		return(NOT_OKAY);

	if (comLongIntWrite(cast_handle->com_hdl_front, DSP_WID_DISPERSION_RIGHT + DSP_PLAY_WIDENER_PARAM_OFFSET, dsp_dispersion_right) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_rear, DSP_WID_DISPERSION_RIGHT + DSP_PLAY_WIDENER_PARAM_OFFSET, dsp_dispersion_right) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_side, DSP_WID_DISPERSION_RIGHT + DSP_PLAY_WIDENER_PARAM_OFFSET, dsp_dispersion_right) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_WidCommunicateFreqThreshold()
 * DESCRIPTION:
 *   Communicate FreqThreshold
 */ 
int dfxp_WidCommunicateFreqThreshold(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int pc_freq_threshold;
	realtype filter_gain, filter_a1, filter_a0;

	pc_freq_threshold = DSP_PLAY_WID_FREQ_THRESHOLD_MIDI;

	/* Calculate the values that should be sent to the dsp card */
	if (qntIToRCalc(cast_handle->midi_to_dsp.wid_filter_gain_qnt_hdl, pc_freq_threshold, &filter_gain) != OKAY)
		return(NOT_OKAY);

	// Widener is only active in front and rear DSP processors
	if (comRealWrite(cast_handle->com_hdl_front, DSP_WID_FILTER_GAIN + DSP_PLAY_WIDENER_PARAM_OFFSET, filter_gain) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, DSP_WID_FILTER_GAIN + DSP_PLAY_WIDENER_PARAM_OFFSET, filter_gain) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, DSP_WID_FILTER_GAIN + DSP_PLAY_WIDENER_PARAM_OFFSET, filter_gain) != OKAY)
		return(NOT_OKAY);

	if (qntIToRCalc(cast_handle->midi_to_dsp.wid_filter_a1_qnt_hdl, pc_freq_threshold, &filter_a1) != OKAY)
		return(NOT_OKAY);

	if (comRealWrite(cast_handle->com_hdl_front, DSP_WID_FILTER_A1 + DSP_PLAY_WIDENER_PARAM_OFFSET, filter_a1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, DSP_WID_FILTER_A1 + DSP_PLAY_WIDENER_PARAM_OFFSET, filter_a1) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, DSP_WID_FILTER_A1 + DSP_PLAY_WIDENER_PARAM_OFFSET, filter_a1) != OKAY)
		return(NOT_OKAY);

	if (qntIToRCalc(cast_handle->midi_to_dsp.wid_filter_a0_qnt_hdl, pc_freq_threshold, &filter_a0) != OKAY)
		return(NOT_OKAY);

	if (comRealWrite(cast_handle->com_hdl_front, DSP_WID_FILTER_A0 + DSP_PLAY_WIDENER_PARAM_OFFSET, filter_a0) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_rear, DSP_WID_FILTER_A0 + DSP_PLAY_WIDENER_PARAM_OFFSET, filter_a0) != OKAY)
		return(NOT_OKAY);
	if (comRealWrite(cast_handle->com_hdl_side, DSP_WID_FILTER_A0 + DSP_PLAY_WIDENER_PARAM_OFFSET, filter_a0) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommAmbienceBypass()
 * DESCRIPTION:
 *   Communicate the ambience bypass.
 */
int dfxp_CommAmbienceBypass(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   int pc_liveliness;
	int ambience_bypass;
	int ambience_on;

   if (dfxp_GetKnobValue_MIDI(hp_dfxp, DFX_UI_KNOB_AMBIENCE, &pc_liveliness) != OKAY)
		return(NOT_OKAY);

	/* Get the bypass settings */;
	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_AMBIENCE, &ambience_on) != OKAY)
		return(NOT_OKAY);
	ambience_bypass = !ambience_on;

	/* 
	 * Perform special warp so that anything displayed as zero actually has a
	 * a zero sound.  (This includes settings up to 0.99 which correspond to a
	 * midi setting of 12.)
	 */
	if ((ambience_bypass) || (pc_liveliness <= DFXP_MIN_EFFECTIVE_MIDI_AMBIENCE))
      ambience_bypass = IS_TRUE;	
   else
		ambience_bypass = IS_FALSE;

	if (cast_handle->processing_override == DFXP_PROCESSING_OVERRIDE_DFX_ALL_EXCEPT_OPTIMIZER)
		ambience_bypass = IS_TRUE;

	// Ambience for subwoofer is always bypassed
	if (comLongIntWrite(cast_handle->com_hdl_front, DSP_PLAY_AMBIENCE_ON, (!ambience_bypass)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_rear, DSP_PLAY_AMBIENCE_ON, (!ambience_bypass)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_side, DSP_PLAY_AMBIENCE_ON, (!ambience_bypass)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_center, DSP_PLAY_AMBIENCE_ON, (!ambience_bypass)) != OKAY)
		return(NOT_OKAY);
	if (comLongIntWrite(cast_handle->com_hdl_subwoofer, DSP_PLAY_AMBIENCE_ON, 0) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_CommunicateMusicMode()
 * DESCRIPTION:
 *   Updates parameters based on current music mode.
 */
int dfxp_CommunicateMusicMode(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* cast_handle->mode_vals.music_mode will be one of the following:
    * DFX_UI_MUSIC_MODE_MUSIC1, DFX_UI_MUSIC_MODE_MUSIC2, DFX_UI_MUSIC_MODE_SPEECH
	 */
	if (dfxp_CommunicateFidelity(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	if (dfxp_CommunicateAmbience(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	if (dfxp_CommunicateDynamicBoost(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	if (dfxp_CommunicateBassBoost(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}