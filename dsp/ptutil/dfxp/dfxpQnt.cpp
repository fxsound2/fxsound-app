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

/* dfxpQnt.cpp */

#include <windows.h>
#include <stdio.h>

#include "u_dfxp.h" /* Must go before codedefs.h due to mmgr */
#include "codedefs.h"

#include "dfxp.h"
#include "qnt.h"
#include "filt.h"
#include "c_aural.h"
#include "c_Play.h"
#include "c_lex.h"
#include "c_max.h"
#include "c_aural.h"
#include "c_wid.h"
#include "c_dly1.h"
#include "Pt_defs.h"
#include "DfxSdk.h"
#include "midi.h"

/*
 * FUNCTION: dfxp_InitAllQnts() 
 * DESCRIPTION:
 *   Called one time to initialize all the qnt handles.
 */
int dfxp_InitAllQnts(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* Set all qnts to NULL */
 	cast_handle->real_to_midi_qnt_hdl = NULL;
 	cast_handle->midi_to_real_qnt_hdl = NULL;

   cast_handle->midi_to_dsp.fidelity_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.spaciousness_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.ambience_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.dynamic_boost_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.bass_boost_qnt_hdl = NULL;

	cast_handle->midi_to_dsp.aural_filter_gain_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.aural_filter_a1_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.aural_filter_a0_qnt_hdl = NULL;

	cast_handle->midi_to_dsp.room_size_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.damping_bandwidth_qnt_hdl = NULL;
   cast_handle->midi_to_dsp.rolloff_bandwidth_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.motion_rate_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.motion_depth_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.screen_lex_main_knob3_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.screen_lex_main_knob4_qnt_hdl = NULL;

	cast_handle->midi_to_dsp.release_time_beta_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.screen_opt_main_knob3_qnt_hdl = NULL;

	cast_handle->midi_to_dsp.dispersion_delay_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.wid_filter_gain_qnt_hdl = NULL;
   cast_handle->midi_to_dsp.wid_filter_a1_qnt_hdl = NULL;
	cast_handle->midi_to_dsp.wid_filter_a0_qnt_hdl = NULL;

	/* Delay Specific */
	cast_handle->midi_to_dsp.dly_qnt_hdl = NULL;

	/* Initialize the static qnt handle */
   if (dfxp_InitStaticQnts(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	/* Initialize the dynamic qnt handles */
   if (dfxp_InitDynamicQnts(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_InitStaticQnts() 
 * DESCRIPTION:
 *   Called one time to initialize all the static qnt handles.
 */
int dfxp_InitStaticQnts(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   /* Real to MIDI qnt */
	if (qntRToIInit(&(cast_handle->real_to_midi_qnt_hdl), cast_handle->slout1,
                   DFX_UI_MIN_VALUE, DFX_UI_MAX_VALUE, 
                   MIDI_MIN_VALUE, MIDI_MAX_VALUE,
	                IS_TRUE, (MIDI_MAX_VALUE - MIDI_MIN_VALUE + 1),
	                IS_FALSE) != OKAY)
      return(NOT_OKAY);

	/* Midi to Real qnt */
	if (qntIToRInit(&(cast_handle->midi_to_real_qnt_hdl), cast_handle->slout1,
		             MIDI_MIN_VALUE, MIDI_MAX_VALUE,
						 (realtype)DFX_UI_MIN_VALUE,
						 (realtype)DFX_UI_MAX_VALUE,
					    IS_FALSE, 0,
					    IS_FALSE, 0,
					    IS_FALSE, QNT_RESPONSE_LINEAR) != OKAY)
		return(NOT_OKAY);

	/* Fidelity */
	if (qntIToRInit(&(cast_handle->midi_to_dsp.fidelity_qnt_hdl), cast_handle->slout1,
					  MIDI_MIN_VALUE, MIDI_MAX_VALUE,
					  (realtype)DSP_AURAL_DRIVE_MIN_VALUE, 
					  (realtype)DSP_AURAL_DRIVE_MAX_VALUE * (realtype)PLY_FIDELITY_INTENSITY_MAX_SCALE,
					  IS_FALSE, 0,
					  IS_FALSE, 0,
					  IS_FALSE, QNT_RESPONSE_LINEAR) != OKAY)
		return(NOT_OKAY);

	/* Ambience - Note that an exponential curve is used instead of the linear
	 * from the Studioverb, to push heavy decays more to the end.
	 */
	if (qntIToRInit(&(cast_handle->midi_to_dsp.ambience_qnt_hdl), cast_handle->slout1,
					  MIDI_MIN_VALUE, MIDI_MAX_VALUE,
					  (realtype)PLY_DECAY_MIN_VALUE, 
					  (realtype)PLY_DECAY_MAX_VALUE,
					  IS_FALSE, 0,
					  IS_FALSE, 0,
					  IS_FALSE, QNT_RESPONSE_EXP) != OKAY)
		return(NOT_OKAY);

	/* Dynamic Boost */
	if (qntIToRInit(&(cast_handle->midi_to_dsp.dynamic_boost_qnt_hdl), cast_handle->slout1,
					  MIDI_MIN_VALUE, MIDI_MAX_VALUE,
					  (realtype)DSP_MAXIMIZE_GAIN_BOOST_MIN_VALUE, 
					  (realtype)DSP_MAXIMIZE_GAIN_BOOST_MAX_VALUE,
					  IS_FALSE, 0,
					  IS_FALSE, 0,
					  IS_FALSE, QNT_RESPONSE_MAXI_BOOST_DSP) != OKAY)
		return(NOT_OKAY);

	/* Spaciousness */
	if (qntIToRInit(&(cast_handle->midi_to_dsp.spaciousness_qnt_hdl), cast_handle->slout1,
					  MIDI_MIN_VALUE, MIDI_MAX_VALUE,
					  (realtype)DSP_WID_INTENSITY_MIN_VALUE, 
					  (realtype)(DSP_WID_INTENSITY_MAX_VALUE * PLY_WIDENER_BOOST_MAX_SCALE),
					  IS_FALSE, 0,
					  IS_FALSE, 0,
					  IS_FALSE, QNT_RESPONSE_LINEAR) != OKAY)
		return(NOT_OKAY);

	/* 
	 * Room Size (Special Case: This one is fixed and does not change
	 *            with the sampling freq.)
	 */
	if (qntIToRInit(&(cast_handle->midi_to_dsp.room_size_qnt_hdl), cast_handle->slout1,
					  MIDI_MIN_VALUE, MIDI_MAX_VALUE,
					  (realtype)DSP_LEX_ROOM_SIZE_MIN_VALUE, 
					  (realtype)DSP_LEX_ROOM_SIZE_MAX_VALUE,
					  IS_FALSE, 0,
					  IS_FALSE, 0,
					  IS_FALSE, QNT_RESPONSE_LINEAR) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_InitDynamicQnts() 
 * DESCRIPTION:
 *   Initialize the qnt handles which change depending on other
 *   settings such as sampling_freq.  This function can be called
 *   repeatedly.
 */
int dfxp_InitDynamicQnts(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* Initialize Dynamic qnts */
	if (dfxp_InitDynamicQnts_Aural(hp_dfxp) != OKAY)
		return(NOT_OKAY);
	if (dfxp_InitDynamicQnts_Lex(hp_dfxp) != OKAY)
		return(NOT_OKAY);
	if (dfxp_InitDynamicQnts_Opt(hp_dfxp) != OKAY)
		return(NOT_OKAY);
	if (dfxp_InitDynamicQnts_Wid(hp_dfxp) != OKAY)
		return(NOT_OKAY);
	if (dfxp_InitDynamicQnts_Play(hp_dfxp) != OKAY)
		return(NOT_OKAY);
	if (dfxp_InitDynamicQnts_Delay(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_InitDynamicQnts_Aural() 
 * DESCRIPTION:
 */
int dfxp_InitDynamicQnts_Aural(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* This feeds the highpass filter with the -3db point in normalized radian frequency */
	realtype omega_min, omega_max;

	omega_min = (realtype)TWO_PI * (realtype) DFXP_AURAL_CONTROL_HERTZ_MIN_VAL * cast_handle->internal_sampling_period;
	omega_max = (realtype)TWO_PI * (realtype) DFXP_AURAL_CONTROL_HERTZ_MAX_VAL * cast_handle->internal_sampling_period;

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

	if (qnt2ndOrderButterworthInit(&(cast_handle->midi_to_dsp.aural_filter_gain_qnt_hdl),
												&(cast_handle->midi_to_dsp.aural_filter_a1_qnt_hdl),
												&(cast_handle->midi_to_dsp.aural_filter_a0_qnt_hdl),
												cast_handle->slout1, MIDI_MIN_VALUE, MIDI_MAX_VALUE,
												omega_min, omega_max, QNT_RESPONSE_LINEAR) != OKAY)
		return(NOT_OKAY);

   return(OKAY);
}

/*
 * FUNCTION: dfxp_InitDynamicQnts_Lex() 
 * DESCRIPTION:
 */
int dfxp_InitDynamicQnts_Lex(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   realtype dsp_motion_rate_min_value;
   realtype dsp_motion_rate_max_value;
   realtype dsp_motion_depth_max_value;

   if (cast_handle->midi_to_dsp.screen_lex_main_knob3_qnt_hdl != NULL)
	{
	   if (qntFreeUp(&(cast_handle->midi_to_dsp.screen_lex_main_knob3_qnt_hdl)) != OKAY)
	      return(NOT_OKAY);
	}

	/* Note - this qnt handle is set up like the Lexicon plug-in knob3 screen
	 * display qnt handle, to allow proper initialization of the
	 * damping_bandwidth_qnt_hdl qnt handle.
	 */
	if (qntIToRInit(&(cast_handle->midi_to_dsp.screen_lex_main_knob3_qnt_hdl), cast_handle->slout1,
					    MIDI_MIN_VALUE, MIDI_MAX_VALUE,
					    (realtype)LEX_HIGH_FREQ_ROLLOFF_MIN_VAL, 
					    (realtype)LEX_HIGH_FREQ_ROLLOFF_MAX_VAL,
					    IS_FALSE, 0,
					    IS_FALSE, (realtype)0.0,
					    IS_TRUE, QNT_RESPONSE_EXP) != OKAY)
      return(NOT_OKAY);

	if (cast_handle->midi_to_dsp.damping_bandwidth_qnt_hdl != NULL)
	{
	   if (qntFreeUp(&(cast_handle->midi_to_dsp.damping_bandwidth_qnt_hdl)) != OKAY)
	      return(NOT_OKAY);
	}

	if (qntIToRSimpleLowpassInit(&(cast_handle->midi_to_dsp.damping_bandwidth_qnt_hdl),
						cast_handle->midi_to_dsp.screen_lex_main_knob3_qnt_hdl, cast_handle->slout1,
						cast_handle->internal_sampling_period, 1.0e3) != OKAY)
		return(NOT_OKAY);

   if (cast_handle->midi_to_dsp.screen_lex_main_knob4_qnt_hdl != NULL)
	{
	   if (qntFreeUp(&(cast_handle->midi_to_dsp.screen_lex_main_knob4_qnt_hdl)) != OKAY)
	      return(NOT_OKAY);
	}

	/* Note - this qnt handle is set up like the Lexicon plug-in knob4 screen
	 * display qnt handle, to allow proper initialization of the
	 * damping_bandwidth_qnt_hdl qnt handle.
	 */
	if (qntIToRInit(&(cast_handle->midi_to_dsp.screen_lex_main_knob4_qnt_hdl), cast_handle->slout1,
					  MIDI_MIN_VALUE, MIDI_MAX_VALUE,
					  (realtype)LEX_HIGH_FREQ_DECAY_MIN_VAL, 
					  (realtype)LEX_HIGH_FREQ_DECAY_MAX_VAL,
					  IS_FALSE, 0,
					  IS_FALSE, (realtype)0.0,
					  IS_TRUE, QNT_RESPONSE_EXP) != OKAY)
		return(NOT_OKAY);

	if (cast_handle->midi_to_dsp.rolloff_bandwidth_qnt_hdl != NULL)
	{
	   if (qntFreeUp(&(cast_handle->midi_to_dsp.rolloff_bandwidth_qnt_hdl)) != OKAY)
	      return(NOT_OKAY);
	}

	if (qntIToRSimpleLowpassInit(&(cast_handle->midi_to_dsp.rolloff_bandwidth_qnt_hdl),
						cast_handle->midi_to_dsp.screen_lex_main_knob4_qnt_hdl, cast_handle->slout1,
						cast_handle->internal_sampling_period, 1.0e3) != OKAY)
		return(NOT_OKAY);
  
	/* Calculate the min and max dsp values for motion rate.
	 * Note the compensation for the repeated point on the oscillator.
	 */
	{
		realtype tmp_r = (realtype)(LEX_NUM_OSC_PTS - 1) * cast_handle->internal_sampling_period;
		dsp_motion_rate_min_value = (realtype)(DSP_LEX_MOTION_RATE_MIN_VALUE) * tmp_r;
		dsp_motion_rate_max_value = (realtype)(DSP_LEX_MOTION_RATE_MAX_VALUE) * tmp_r;
	}

	if (cast_handle->midi_to_dsp.motion_rate_qnt_hdl != NULL)
	{
	   if (qntFreeUp(&(cast_handle->midi_to_dsp.motion_rate_qnt_hdl)) != OKAY)
	      return(NOT_OKAY);
	}
	if (qntIToRInit(&(cast_handle->midi_to_dsp.motion_rate_qnt_hdl), cast_handle->slout1,
					  MIDI_MIN_VALUE, MIDI_MAX_VALUE,
					  (realtype)dsp_motion_rate_min_value, 
					  (realtype)dsp_motion_rate_max_value,
					  IS_FALSE, 0,
					  IS_FALSE, 0,
					  IS_FALSE, QNT_RESPONSE_LINEAR) != OKAY)
		return(NOT_OKAY);

	dsp_motion_depth_max_value = (realtype)(LEX_MODULATION_DELAY_MAX_MS/1000.0) * cast_handle->internal_sampling_freq;

	if (cast_handle->midi_to_dsp.motion_depth_qnt_hdl != NULL)
	{
	   if (qntFreeUp(&(cast_handle->midi_to_dsp.motion_depth_qnt_hdl)) != OKAY)
	      return(NOT_OKAY);
	}
	if (qntIToRInit(&(cast_handle->midi_to_dsp.motion_depth_qnt_hdl), cast_handle->slout1,
					  MIDI_MIN_VALUE, MIDI_MAX_VALUE,
					  (realtype)0.0, 
					  dsp_motion_depth_max_value,
					  IS_FALSE, 0,
					  IS_FALSE, 0,
					  IS_FALSE, QNT_RESPONSE_LINEAR) != OKAY)
		return(NOT_OKAY);

   return(OKAY);
}

/*
 * FUNCTION: dfxp_InitDynamicQnts_Opt() 
 * DESCRIPTION:
 */
int dfxp_InitDynamicQnts_Opt(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   if (cast_handle->midi_to_dsp.screen_opt_main_knob3_qnt_hdl != NULL)
	{
	   if (qntFreeUp(&(cast_handle->midi_to_dsp.screen_opt_main_knob3_qnt_hdl)) != OKAY)
	      return(NOT_OKAY);
   }

	/* Note - this qnt handle is set up like the Optimizer plug-in knob3 screen
	 * display qnt handle, to allow proper initialization of the
	 * release_time_beta_qnt_hdl qnt handle.
	 */
	if (qntIToRInit(&(cast_handle->midi_to_dsp.screen_opt_main_knob3_qnt_hdl), cast_handle->slout1,
					    MIDI_MIN_VALUE, MIDI_MAX_VALUE,
					    (realtype)MAXIMIZE_MIN_TIME_CONST, 
					    (realtype)MAXIMIZE_MAX_TIME_CONST,
					    IS_FALSE, 0,
					    IS_FALSE, (realtype)0.0,
					    IS_TRUE, QNT_RESPONSE_EXP) != OKAY)
      return(NOT_OKAY);

	if (cast_handle->midi_to_dsp.release_time_beta_qnt_hdl != NULL)
	{
	   if (qntFreeUp(&(cast_handle->midi_to_dsp.release_time_beta_qnt_hdl)) != OKAY)
	      return(NOT_OKAY);
	}
	/* Uses a qnt handle with desired settings in millisecs to create a qnt handle
	 * with the corresponding exponential beta values.
	 */
	if (qntIToRTimeConstantBeta(&(cast_handle->midi_to_dsp.release_time_beta_qnt_hdl), 
		                         cast_handle->midi_to_dsp.screen_opt_main_knob3_qnt_hdl, cast_handle->slout1,
                               cast_handle->internal_sampling_freq) != OKAY)
		return(NOT_OKAY);

   return(OKAY);
}

/*
 * FUNCTION: dfxp_InitDynamicQnts_Wid() 
 * DESCRIPTION:
 */
int dfxp_InitDynamicQnts_Wid(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	long dsp_dispersion_min_value;
	long dsp_dispersion_max_value;
   realtype dsp_threshold_min_value;
   realtype dsp_threshold_max_value;
	realtype omega_min;
	realtype omega_max;

   /* Calculate the min and max dsp values of dispersion.
	 * Note delay line method requires minimum delay of 1.
	 */
	dsp_dispersion_min_value = 
	   1 + (long)((float)(WID_DISPERSION_MIN_MS * 0.001) * cast_handle->internal_sampling_freq);
	dsp_dispersion_max_value = 
	   (long)((float)(WID_DISPERSION_MAX_MS * 0.001) * cast_handle->internal_sampling_freq);

	if (cast_handle->midi_to_dsp.dispersion_delay_qnt_hdl != NULL)
	{
	   if (qntFreeUp(&(cast_handle->midi_to_dsp.dispersion_delay_qnt_hdl)) != OKAY)
	      return(NOT_OKAY);
	}
	if (qntIToLInit(&(cast_handle->midi_to_dsp.dispersion_delay_qnt_hdl), cast_handle->slout1,
					  MIDI_MIN_VALUE, MIDI_MAX_VALUE,
					  dsp_dispersion_min_value, 
					  dsp_dispersion_max_value,
					  IS_FALSE, IS_FALSE, IS_FALSE) != OKAY)
		return(NOT_OKAY);

   /* Calculate the min and max dsp values of threshold */
   dsp_threshold_min_value = 
		(realtype)(WID_FREQ_THRESHOLD_MIN) * cast_handle->internal_sampling_period;
   dsp_threshold_max_value = 
	   (realtype)(WID_FREQ_THRESHOLD_MAX) * cast_handle->internal_sampling_period;

	/* This feeds the highpass filter with the -3db point in normalized radian frequency */
	omega_min = (realtype)TWO_PI * (realtype) WID_FREQ_THRESHOLD_MIN * cast_handle->internal_sampling_period;
	omega_max = (realtype)TWO_PI * (realtype) WID_FREQ_THRESHOLD_MAX * cast_handle->internal_sampling_period;

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

	if (qnt2ndOrderButterworthInit(&(cast_handle->midi_to_dsp.wid_filter_gain_qnt_hdl),
												&(cast_handle->midi_to_dsp.wid_filter_a1_qnt_hdl),
												&(cast_handle->midi_to_dsp.wid_filter_a0_qnt_hdl),
												cast_handle->slout1, MIDI_MIN_VALUE, MIDI_MAX_VALUE,
												omega_min, omega_max, QNT_RESPONSE_LINEAR) != OKAY)
		return(NOT_OKAY);

   return(OKAY);
}

/*
 * FUNCTION: dfxp_InitDynamicQnts_Play() 
 * DESCRIPTION:
 */
int dfxp_InitDynamicQnts_Play(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if (cast_handle->midi_to_dsp.bass_boost_qnt_hdl != NULL)
	{
	   if (qntFreeUp(&(cast_handle->midi_to_dsp.bass_boost_qnt_hdl)) != OKAY)
	      return(NOT_OKAY);
	}

	/* Bass Boost component of Play dsp function */
	if (qntIToBoostCutInit(&(cast_handle->midi_to_dsp.bass_boost_qnt_hdl), cast_handle->slout1,
					  MIDI_MIN_VALUE, MIDI_MAX_VALUE,
					  (realtype)DSP_PLY_BASSBOOST_MIN_VALUE, 
					  (realtype)DSP_PLY_BASSBOOST_MAX_VALUE,
					  (realtype)DSP_PLY_BASSBOOST_CENTER_FREQ,
					  cast_handle->internal_sampling_freq,
					  (realtype)DSP_PLY_BASSBOOST_Q,
					  FILT_BOOST_CUT) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_InitDynamicQnts_Delay() 
 * DESCRIPTION:
 */
int dfxp_InitDynamicQnts_Delay(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* Delay qnt */
   long dsp_delay_max_value;
   realtype max_total_dly_msecs;
   realtype max_total_dly_secs;

   max_total_dly_msecs = DSP_PLY_MAX_ELEMENT_DELAY;
   max_total_dly_secs = (max_total_dly_msecs) / (realtype) 1000.0;
   dsp_delay_max_value = (long)((cast_handle->internal_sampling_freq)*(max_total_dly_secs));
   
	if (cast_handle->midi_to_dsp.dly_qnt_hdl != NULL)
	{
	   if (qntFreeUp(&(cast_handle->midi_to_dsp.dly_qnt_hdl)) != OKAY)
	      return(NOT_OKAY);
	}

   if (qntRToLInit(&(cast_handle->midi_to_dsp.dly_qnt_hdl), cast_handle->slout1,
					  0, max_total_dly_msecs,
					  DSP_DELAY_MIN_VALUE, dsp_delay_max_value,
					  IS_FALSE, 0) != OKAY)
      return(NOT_OKAY);

	return(OKAY);
}