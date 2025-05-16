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

/* dfxpGet.cpp */

#include "codedefs.h"

#include <windows.h>
#include <stdio.h>

#include "u_dfxp.h" 

#include "dfxp.h" 
#include "DfxSdk.h"
#include "qnt.h"
#include "spectrum.h"

/*
 * FUNCTION: dfxpGetKnobValue() 
 * DESCRIPTION:
 *  Passes back the value of the specified knob as a float.
 */
int dfxpGetKnobValue(PT_HANDLE *hp_dfxp, int i_knob_type, float *fp_value)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	realtype r_value;
	int i_midi_value;

	*fp_value = (float)0.0;

	if (dfxp_GetKnobValue_MIDI(hp_dfxp, i_knob_type, &i_midi_value) != OKAY)
		return(NOT_OKAY);

	/* Calulate the real value */
   if (qntIToRCalc(cast_handle->midi_to_real_qnt_hdl, i_midi_value, &r_value) != OKAY)
		return(NOT_OKAY);

	*fp_value = (float)r_value;

	return(OKAY);
}

/*
 * FUNCTION: dfxp_GetKnobValue_MIDI() 
 * DESCRIPTION:
 *  Passes back the value of the specified knob in MIDI representation.
 */
int dfxp_GetKnobValue_MIDI(PT_HANDLE *hp_dfxp, int i_knob_type, int *ip_midi_value)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	wchar_t wcp_key_name[128];
	int default_value;

   if (ip_midi_value == NULL)
		return(NOT_OKAY);

	*ip_midi_value = 0;


	/***************************
	 * TAKE CARE OF DFX CASE   *
	 ***************************/

   /* Hard code the vocal reduction value */
	if (i_knob_type == DFX_UI_KNOB_VOCAL_REDUCTION)
	{
		*ip_midi_value = DFXP_INIT_VOCAL_REDUCTION_MIDI_VAL;

		return(OKAY);
	}

   /* Determine the registry keyname and default value associated with the knob_type */
	if (i_knob_type == DFX_UI_KNOB_FIDELITY)
	{
		default_value = DFXP_INIT_FIDELITY_MIDI_VAL;
		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_VALUE_FIDELITY_WIDE);
	}
	else if (i_knob_type == DFX_UI_KNOB_AMBIENCE)
	{
		default_value = DFXP_INIT_AMBIENCE_MIDI_VAL;
		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_VALUE_AMBIENCE_WIDE);
	}
	else if (i_knob_type == DFX_UI_KNOB_DYNAMIC_BOOST)
	{
		default_value = DFXP_INIT_DYNAMIC_BOOST_MIDI_VAL;
		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_VALUE_DYNAMIC_BOOST_WIDE);
	}
	else if (i_knob_type == DFX_UI_KNOB_SURROUND)
	{
		default_value = DFXP_INIT_SURROUND_MIDI_VAL;
		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_VALUE_SURROUND_WIDE);
	}
	else if (i_knob_type == DFX_UI_KNOB_BASS_BOOST)
	{
		default_value = DFXP_INIT_BASS_BOOST_MIDI_VAL;
		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_VALUE_BASS_BOOST_WIDE);
	}
	else 
		return(OKAY);

	/* Get the midi value */
   if (dfxp_SessionReadIntegerValue(hp_dfxp, wcp_key_name, default_value, ip_midi_value) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxpGetButtonValue()
 * DESCRIPTION:
 *   Passes back the on/off value of the specified button.
 */
int dfxpGetButtonValue(PT_HANDLE *hp_dfxp, int i_button_type, int *ip_value)
{
	struct dfxpHdlType *cast_handle;
	int default_bypass_value;
	int default_music_mode;
	int registry_value;
	wchar_t wcp_key_name[256];
	int use_opposite_of_registy_value;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* The vocal reduction values are special cases because they are always bypassed */
	if (i_button_type == DFX_UI_BUTTON_VOCAL_REDUCTION_ON)
	{
		*ip_value = IS_FALSE;
		return(OKAY);
	}
	if (i_button_type == DFX_UI_BUTTON_VOCAL_REDUCTION_MODE)
	{
		*ip_value = DFX_UI_VOCAL_REDUCTION_MODE_1;
		return(OKAY);
	}
	/* The music mode is a special case because it can have several different values */
	if (i_button_type == DFX_UI_BUTTON_MUSIC_MODE)
	{
		default_music_mode = DFX_UI_MUSIC_MODE_MUSIC2;

		if (dfxp_SessionReadIntegerValue(hp_dfxp, DFXP_REGISTRY_MODE_MUSIC_MODE_WIDE, default_music_mode, ip_value) != OKAY)
			return(NOT_OKAY);

		return(OKAY);
	}

   /* All of the remaining buttons are on/off buttons */
	if (i_button_type == DFX_UI_BUTTON_BYPASS)
	{
		default_bypass_value = IS_FALSE;
		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_BYPASS_ALL_WIDE);
		use_opposite_of_registy_value = IS_FALSE;
	}
	else if (i_button_type == DFX_UI_BUTTON_FIDELITY)
	{
		default_bypass_value = IS_FALSE;
		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_BYPASS_FIDELITY_WIDE);
		use_opposite_of_registy_value = IS_TRUE;
	}
	else if (i_button_type == DFX_UI_BUTTON_AMBIENCE)
	{
		default_bypass_value = IS_FALSE;
		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_BYPASS_AMBIENCE_WIDE);
		use_opposite_of_registy_value = IS_TRUE;
	}
	else if (i_button_type == DFX_UI_BUTTON_DYNAMIC_BOOST)
	{
		default_bypass_value = IS_FALSE;

		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_BYPASS_DYNAMIC_BOOST_WIDE);
		use_opposite_of_registy_value = IS_TRUE;
	}
	else if (i_button_type == DFX_UI_BUTTON_SURROUND)
	{
		default_bypass_value = IS_FALSE;

		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_BYPASS_SURROUND_WIDE);
		use_opposite_of_registy_value = IS_TRUE;
	}
	else if (i_button_type == DFX_UI_BUTTON_BASS_BOOST)
	{
		default_bypass_value = IS_FALSE;
		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_BYPASS_BASS_BOOST_WIDE);
		use_opposite_of_registy_value = IS_TRUE;
	}
	else if (i_button_type == DFX_UI_BUTTON_HEADPHONE)
	{
		default_bypass_value = IS_TRUE;
		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_BYPASS_HEADPHONE_WIDE);
		use_opposite_of_registy_value = IS_TRUE;
	}
	else if (i_button_type == DFX_UI_BUTTON_REMIX_BYPASS)
	{
		default_bypass_value = IS_FALSE;
		swprintf(wcp_key_name, L"%s", DFXP_REGISTRY_REMIX_BYPASS_ALL_WIDE);
		use_opposite_of_registy_value = IS_FALSE;;
	}
	else
		return(NOT_OKAY);

	/* Read the value from the registry */
	if (dfxp_SessionReadIntegerValue(hp_dfxp, wcp_key_name, default_bypass_value, &registry_value) != OKAY)
		return(NOT_OKAY);

	if (use_opposite_of_registy_value)
		*ip_value = !registry_value;
	else
		*ip_value = registry_value;

	return(OKAY);
}

/*
 * FUNCTION: dfxpGetSamplingFreq()
 * DESCRIPTION:
 *
 *  Passes back the current sampling frequency used for processing.
 *
 */
int dfxpGetSamplingFreq(PT_HANDLE *hp_dfxp, realtype *rp_sampling_freq)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   *rp_sampling_freq = cast_handle->sampling_freq;

	return(OKAY);
}

/*
 * FUNCTION: dfxpGetProcessingOverride() 
 * DESCRIPTION:
 *
 *  Gets the special DFX processing override setting.  
 *  See dfxpSetProcessingOverride() for description.
 *
 */
int dfxpGetProcessingOverride(PT_HANDLE *hp_dfxp, int *ip_dfx_processing_override)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	*ip_dfx_processing_override = cast_handle->processing_override;

	return(OKAY);
}

/*
 * FUNCTION: dfxpGetFirstTimeRunFlag()
 * DESCRIPTION:
 *   Passes back a flag stating whether or not this is the first time the DFX has been run since
 *   installation.
 */
int dfxpGetFirstTimeRunFlag(PT_HANDLE *hp_dfxp, int *ip_first_time_run_flag)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	*ip_first_time_run_flag = cast_handle->first_time_run_flag;

	return(OKAY);
}

/*
 * FUNCTION: dfxpGetTemporaryBypassAll()
 * DESCRIPTION:
 *
 *  Gets whether or not to temporarily bypass all processing.
 */
int dfxpGetTemporaryBypassAll(PT_HANDLE *hp_dfxp, int *ip_temporary_bypass_all)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	int i_default_temporary_bypass_all;

	if (cast_handle == NULL)
		return(OKAY);

	i_default_temporary_bypass_all = IS_FALSE;

	if (dfxp_SessionReadIntegerValue(hp_dfxp, DFXP_REGISTRY_TEMPORARY_BYPASS_ALL_WIDE, 
											   i_default_temporary_bypass_all, ip_temporary_bypass_all) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxpGetDfxTunedTrackPlaying()
 * DESCRIPTION:
 *
 *  Gets whether or not a DFX printed track is currently playing (i.e. a song already processed by iDFX).
 */
int dfxpGetDfxTunedTrackPlaying(PT_HANDLE *hp_dfxp, int *ip_dfx_tuned_track_playing)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	int i_default_dfx_tuned_track_playing;

	if (cast_handle == NULL)
		return(OKAY);

	i_default_dfx_tuned_track_playing = IS_FALSE;

	if (dfxp_SessionReadIntegerValue(hp_dfxp, DFXP_REGISTRY_DFX_TUNED_TRACK_PLAYING_WIDE, 
												i_default_dfx_tuned_track_playing, ip_dfx_tuned_track_playing) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxpGetTotalAudioProcessedTime()
 * DESCRIPTION:
 *   Get the total audio processed time
 */
int dfxpGetTotalAudioProcessedTime(PT_HANDLE *hp_dfxp, unsigned long *ul_msec_audio_processed_time)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	*ul_msec_audio_processed_time = 0;

	if (cast_handle == NULL)
		return(OKAY);

	if (cast_handle->trace.mode)
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpGetTotalAudioProcessedTime: Entered");

	if (!(cast_handle->fully_initialized))
		return(NOT_OKAY);

	*ul_msec_audio_processed_time = cast_handle->ul_total_msecs_audio_processed_time;

	return(OKAY);
}