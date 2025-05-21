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

/* dfxpSet.cpp */

#include "codedefs.h"

#include <windows.h>
#include <stdio.h>

#include "u_dfxp.h" 

#include "dfxp.h"
#include "DfxSdk.h"
#include "qnt.h"
//#include "dfxForWmpDefs.h"
#include "reg.h"
#include "midi.h"

/*
 * FUNCTION: dfxpSetKnobValue() 
 * DESCRIPTION:
 *   Sets the passed knob to the specified value (in float representation).
 */
int dfxpSetKnobValue(PT_HANDLE *hp_dfxp, int i_knob_type, float f_value, bool b_from_eq_change)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int midi_val;

   if (!(cast_handle->fully_initialized))
		return(NOT_OKAY);

	/* Make sure it is in the proper range */
	if ((f_value < DFX_UI_MIN_VALUE) || (f_value > DFX_UI_MAX_VALUE))
		return(NOT_OKAY);
 
	/* Convert the passed real value to the proper MIDI value */
	if (qntRToICalc(cast_handle->real_to_midi_qnt_hdl, (realtype)f_value, &midi_val) != OKAY)
		return(NOT_OKAY);

	/* Set the midi representation of the value */
	if (dfxp_SetKnobValue_MIDI(hp_dfxp, i_knob_type, midi_val, b_from_eq_change) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_SetKnobValue_MIDI() 
 * DESCRIPTION:
 *   Sets the passed knob to the specified value (in MIDI representation).
 */
int dfxp_SetKnobValue_MIDI(PT_HANDLE *hp_dfxp, int i_knob_type, int i_midi_value, bool b_from_eq_change)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	realtype r_eq_setting_normalized;
	realtype r_eq_setting_db;

   if (!(cast_handle->fully_initialized))
		return(NOT_OKAY);

	/* Make sure it is in the proper range */
	if ((i_midi_value < MIDI_MIN_VALUE) || (i_midi_value > MIDI_MAX_VALUE))
		return(NOT_OKAY);

	/* Take care of the specific knobs */
   if (i_knob_type == DFX_UI_KNOB_FIDELITY)
	{
		if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_VALUE_FIDELITY_WIDE, i_midi_value) != OKAY)
			return(NOT_OKAY);
      if (dfxp_CommunicateFidelity(hp_dfxp) != OKAY)
         return(NOT_OKAY);
	}
	else if (i_knob_type == DFX_UI_KNOB_AMBIENCE)
	{   
		if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_VALUE_AMBIENCE_WIDE, i_midi_value) != OKAY)
			return(NOT_OKAY);
      if (dfxp_CommunicateAmbience(hp_dfxp) != OKAY)
         return(NOT_OKAY);
	}
	else if (i_knob_type == DFX_UI_KNOB_DYNAMIC_BOOST)
	{
		if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_VALUE_DYNAMIC_BOOST_WIDE, i_midi_value) != OKAY)
			return(NOT_OKAY);
      if (dfxp_CommunicateDynamicBoost(hp_dfxp) != OKAY)
         return(NOT_OKAY);
	}
	else if (i_knob_type == DFX_UI_KNOB_SURROUND)
	{
		if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_VALUE_SURROUND_WIDE, i_midi_value) != OKAY)
			return(NOT_OKAY);
      if (dfxp_CommunicateSpaciousness(hp_dfxp) != OKAY)
         return(NOT_OKAY);
	}
	else if (i_knob_type == DFX_UI_KNOB_BASS_BOOST)
	{
		if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_VALUE_BASS_BOOST_WIDE, i_midi_value) != OKAY)
			return(NOT_OKAY);
      if (dfxp_CommunicateBassBoost(hp_dfxp) != OKAY)
         return(NOT_OKAY);

		/* 
		 * Move the EQ setting of band 1 to match the new HyperBass setting. 
		 * NOTE: Make sure not to do this if this call came as a result of a change to EQ band1 trying to move the
		 *       the HyperBass slider to match the EQ.  Otherwise, we will end up in an endless loop.
		 */
		if (!b_from_eq_change)
		{
			/* Convert the midi setting to a real value between 0.0 and 1.0 */
			if (qntIToRCalc(cast_handle->midi_to_real_qnt_hdl, i_midi_value, &r_eq_setting_normalized) != OKAY)
				return(NOT_OKAY);

			/* Convert normalized value to dB (maxed out at 10) */
			r_eq_setting_db = r_eq_setting_normalized * (realtype)10.0;

			if (dfxpEqSetBandBoostCut(hp_dfxp, DFXP_STORAGE_TYPE_ALL, 1, r_eq_setting_db) != OKAY)
				return(NOT_OKAY);
		}
	}
	
	return(OKAY);
}

/*
 * FUNCTION: dfxpSetButtonValue()
 * DESCRIPTION:
 *   Sets the passed button to the specified value on/off value.
 */
int dfxpSetButtonValue(PT_HANDLE *hp_dfxp, int i_button_type, int i_value)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   if (!(cast_handle->fully_initialized))
		return(NOT_OKAY);

	/* Take care of the non-bypass type buttons */
	if (i_button_type == DFX_UI_BUTTON_MUSIC_MODE)
	{
		if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_MODE_MUSIC_MODE_WIDE, i_value) != OKAY)
			return(NOT_OKAY);
      if (dfxp_CommunicateMusicMode(hp_dfxp) != OKAY)
         return(NOT_OKAY);
	}
	else if (i_button_type == DFX_UI_BUTTON_BYPASS)
	{
      if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_BYPASS_ALL_WIDE, i_value) != OKAY)
	      return(NOT_OKAY);
		if (dfxp_CommunicateBypassSettings(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}
	else if (i_button_type == DFX_UI_BUTTON_FIDELITY)
	{
	   if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_BYPASS_FIDELITY_WIDE, !(i_value)) != OKAY)
	      return(NOT_OKAY);
		if (dfxp_CommunicateBypassSettings(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}
	else if (i_button_type == DFX_UI_BUTTON_AMBIENCE)
	{   
	   if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_BYPASS_AMBIENCE_WIDE, !(i_value)) != OKAY)
	      return(NOT_OKAY);
		if (dfxp_CommunicateBypassSettings(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}
	else if (i_button_type == DFX_UI_BUTTON_DYNAMIC_BOOST)
	{
	   if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_BYPASS_DYNAMIC_BOOST_WIDE, !(i_value)) != OKAY)
	      return(NOT_OKAY);
		if (dfxp_CommunicateBypassSettings(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}
	else if (i_button_type == DFX_UI_BUTTON_SURROUND)
	{
	   if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_BYPASS_SURROUND_WIDE, !(i_value)) != OKAY)
	      return(NOT_OKAY);
		if (dfxp_CommunicateBypassSettings(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}
	else if (i_button_type == DFX_UI_BUTTON_BASS_BOOST)
	{
	   if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_BYPASS_BASS_BOOST_WIDE, !(i_value)) != OKAY)
	      return(NOT_OKAY);
		if (dfxp_CommunicateBypassSettings(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}
	else if (i_button_type == DFX_UI_BUTTON_HEADPHONE)
	{
      if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_BYPASS_HEADPHONE_WIDE, !(i_value)) != OKAY)
	      return(NOT_OKAY);
		if (dfxp_CommunicateBypassSettings(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}
	else if (i_button_type == DFX_UI_BUTTON_REMIX_BYPASS)
	{
      if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_REMIX_BYPASS_ALL_WIDE, i_value) != OKAY)
	      return(NOT_OKAY);
		if (dfxp_CommunicateBypassSettings(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: dfxpConvertFaderRealValueToInt()
 * DESCRIPTION:
 *
 * Takes the passed fader real value and converts it to the proper number to display.
 *
 */
int dfxpConvertFaderRealValueToInt(PT_HANDLE *hp_dfxp, realtype r_value, int *ip_value, int i_quantized)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if (i_quantized)
	{
	   if (r_value < (realtype)0.05)
	      *ip_value = 0;
	   else if (r_value < (realtype)0.15)
	      *ip_value = 1;
	   else if (r_value < (realtype)0.25)
	      *ip_value = 2;
	   else if (r_value < (realtype)0.35)
	      *ip_value = 3;
	   else if (r_value < (realtype)0.45)
	      *ip_value = 4;
	   else if (r_value < (realtype)0.55)
	      *ip_value = 5;
	   else if (r_value < (realtype)0.65)
	      *ip_value = 6;
	   else if (r_value < (realtype)0.75)
	      *ip_value = 7;
	   else if (r_value < (realtype)0.85)
	      *ip_value = 8;
	   else if (r_value < (realtype)0.95)
	      *ip_value = 9;
	   else 
		   *ip_value = 10;
	}
	else
	{
	   if (r_value < (realtype)0.1)
	      *ip_value = 0;
	   else if (r_value < (realtype)0.2)
	      *ip_value = 1;
	   else if (r_value < (realtype)0.3)
	      *ip_value = 2;
	   else if (r_value < (realtype)0.4)
	      *ip_value = 3;
	   else if (r_value < (realtype)0.5)
	      *ip_value = 4;
	   else if (r_value < (realtype)0.6)
	      *ip_value = 5;
	   else if (r_value < (realtype)0.7)
	      *ip_value = 6;
	   else if (r_value < (realtype)0.8)
	      *ip_value = 7;
	   else if (r_value < (realtype)0.9)
	      *ip_value = 8;
	   else if (r_value < (realtype)1.0)
	      *ip_value = 9;
	   else 
		   *ip_value = 10;
	}

   return(OKAY);
}

/*
 * FUNCTION: dfxpConvertIntToFaderRealValue()
 * DESCRIPTION:
 *
 * Takes the passed fader int value (0 - 10) and passes back the corresponding real value
 * of the fader (0.0 - 1.0).
 *
 */
int dfxpConvertIntToFaderRealValue(PT_HANDLE *hp_dfxp, int i_value, realtype *rp_value)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   if (i_value == 0)
	   *rp_value = (realtype)0.0;
	else if (i_value == 1)
		*rp_value = (realtype)0.1;
	else if (i_value == 2)
		*rp_value = (realtype)0.2;
	else if (i_value == 3)
		*rp_value = (realtype)0.3;
	else if (i_value == 4)
		*rp_value = (realtype)0.4;
	else if (i_value == 5)
		*rp_value = (realtype)0.5;
	else if (i_value == 6)
		*rp_value = (realtype)0.6;
	else if (i_value == 7)
		*rp_value = (realtype)0.7;
	else if (i_value == 8)
		*rp_value = (realtype)0.8;
	else if (i_value == 9)
		*rp_value = (realtype)0.9;
	else 
		*rp_value = (realtype)1.0;

   return(OKAY);
}


/*
 * FUNCTION: dfxpSetProcessingOverride() 
 * DESCRIPTION:
 *
 *  Sets the special DFX processing override setting.  When Remix is running we may want
 *  override the DFX processing components based on certain conditions.  For example a copy Remix
 *  may want to completely override DFX processing if it detects that a DFX plugin is also concurrently
 *  running.
 *
 */
int dfxpSetProcessingOverride(PT_HANDLE *hp_dfxp, int i_dfx_processing_override)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);
	
	if (cast_handle->trace.mode)
	   (cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpSetProcessingOverride: Entered");

	/* If the override setting has not changed, there is nothing to do */
   if (i_dfx_processing_override == cast_handle->processing_override)
		return(OKAY);

	cast_handle->processing_override = i_dfx_processing_override;

   if (dfxp_CommunicateBypassSettings(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	if (cast_handle->trace.mode)
	   (cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpSetProcessingOverride: Done");

	return(OKAY);
}

/*
 * FUNCTION: dfxpSetTemporaryBypassAll()
 * DESCRIPTION:
 *
 *  Sets whether or not to temporarily bypass all processing.
 */
int dfxpSetTemporaryBypassAll(PT_HANDLE *hp_dfxp, int i_temporary_bypass_all)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   if (!(cast_handle->fully_initialized))
		return(NOT_OKAY);

	/* THIS MIGHT FAIL FOR INTERNET EXPLORER */
	if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_TEMPORARY_BYPASS_ALL_WIDE, i_temporary_bypass_all) != OKAY)
		return(OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxpSetDfxTunedTrackPlaying()
 * DESCRIPTION:
 *
 *  Sets whether or not to temporarily bypass all processing because the current track
 *  has already been DFX Tuned.
 */
int dfxpSetDfxTunedTrackPlaying(PT_HANDLE *hp_dfxp, int i_dfx_tuned_track_playing)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if (cast_handle->trace.mode)
	   (cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpSetDfxTunedTrackPlaying: Entered");

   if (!(cast_handle->fully_initialized))
		return(NOT_OKAY);

	if (cast_handle->trace.mode)
	{
		swprintf(cast_handle->wcp_msg1, L"dfxpSetDfxTunedTrackPlaying: i_dfx_tuned_track_playing = %d", i_dfx_tuned_track_playing);
	   (cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
   }

	if (dfxp_SessionWriteIntegerValue(hp_dfxp, DFXP_REGISTRY_DFX_TUNED_TRACK_PLAYING_WIDE, i_dfx_tuned_track_playing) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxpSetTotalAudioProcessedTime()
 * DESCRIPTION:
 *   Set the total audio processed time to the given value
 */
int dfxpSetTotalAudioProcessedTime(PT_HANDLE *hp_dfxp, unsigned long ul_msec_audio_processed_time)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if (cast_handle->trace.mode)
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpSetTotalAudioProcessedTime: Entered");

	if (!(cast_handle->fully_initialized))
		return(NOT_OKAY);

	cast_handle->ul_total_msecs_audio_processed_time = ul_msec_audio_processed_time;

	return(OKAY);
}