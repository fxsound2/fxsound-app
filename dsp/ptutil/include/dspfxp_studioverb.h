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
#ifndef _DSPFXP_STUDIOVERB_H_
#define _DSPFXP_STUDIOVERB_H_

#include "dspfxp.h"

/* Studioverb specific display settings */
#define DSPFXP_STUDIOVERB_SCREEN_ROOM_SIZE_MIN              0.33
#define DSPFXP_STUDIOVERB_SCREEN_ROOM_SIZE_MAX              1.0

#define DSPFXP_STUDIOVERB_SCREEN_DECAY_TIME_MIN             0.25
#define DSPFXP_STUDIOVERB_SCREEN_DECAY_TIME_MAX             20.0

#define DSPFXP_STUDIOVERB_SCREEN_HIGH_FREQ_ROLLOFF_MIN      1.0
#define DSPFXP_STUDIOVERB_SCREEN_HIGH_FREQ_ROLLOFF_MAX      20.0

#define DSPFXP_STUDIOVERB_SCREEN_HIGH_FREQ_DECAY_MIN        1.0
#define DSPFXP_STUDIOVERB_SCREEN_HIGH_FREQ_DECAY_MAX        20.0 

#define DSPFXP_STUDIOVERB_SCREEN_MIX_LEVEL_MIN              0.0
#define DSPFXP_STUDIOVERB_SCREEN_MIX_LEVEL_MAX              1.0

#define DSPFXP_STUDIOVERB_SCREEN_OUTPUT_LEVEL_MIN           0.0
#define DSPFXP_STUDIOVERB_SCREEN_OUTPUT_LEVEL_MAX           1.0

#define DSPFXP_STUDIOVERB_SCREEN_DENSITY_MIN                0.0
#define DSPFXP_STUDIOVERB_SCREEN_DENSITY_MAX                1.0

#define DSPFXP_STUDIOVERB_SCREEN_PRE_DELAY_MIN              0.0
#define DSPFXP_STUDIOVERB_SCREEN_PRE_DELAY_MAX              100.0

#define DSPFXP_STUDIOVERB_SCREEN_MOTION_DEPTH_MIN           0.0
#define DSPFXP_STUDIOVERB_SCREEN_MOTION_DEPTH_MAX           100.0

#define DSPFXP_STUDIOVERB_SCREEN_MOTION_RATE_MIN            0.2
#define DSPFXP_STUDIOVERB_SCREEN_MOTION_RATE_MAX            2.0

/******************************************
 * dspfxp - Studioverb specific functions *
 ******************************************/

/* dspfxpStudioverbInit.cpp */
int dspfxpStudioverbInit(DSPFXP_HANDLE **hpp_dspfxp);
int dspfxpStudioverbFreeUp(DSPFXP_HANDLE **hpp_dspfxp);

/* dspfxpStudioverbProcess.cpp */
int dspfxpStudioverbBeginProcess(DSPFXP_HANDLE *hp_dspfxp, int i_bits_per_sample, int i_num_channels, int i_sample_rate);
int dspfxpStudioverbModifyShortIntSamples(DSPFXP_HANDLE *hp_dspfxp, short int *si_input_samples, short int *si_output_samples, int i_numsamples);
int dspfxpStudioverbModifyFloatSamples(DSPFXP_HANDLE *hp_dspfxp, float *fp_input_samples, float *fp_output_samples, int i_numsamples);

/* dspfxpStudioverbSet.cpp */
int dspfxpStudioverbSetBypassAll(DSPFXP_HANDLE *hp_dspfxp, int i_bypass_all);
int dspfxpStudioverbSetMuteIn(DSPFXP_HANDLE *hp_dspfxp, int i_mute_in);
int dspfxpStudioverbSetMuteOut(DSPFXP_HANDLE *hp_dspfxp, int i_mute_out);

/* dspfxpStudioverbCtrlToScreen.cpp */
int dspfxpStudioverbCtrlToScreenRoomSize(DSPFXP_HANDLE *hp_dspfxp, float f_ctrl_val, 
											int i_set_dsp_flag, float *fp_screen_val);
int dspfxpStudioverbCtrlToScreenDecayTime(DSPFXP_HANDLE *hp_dspfxp, float f_ctrl_val, 
											int i_set_dsp_flag, float *fp_screen_val);
int dspfxpStudioverbCtrlToScreenHighFreqRolloff(DSPFXP_HANDLE *hp_dspfxp, float f_ctrl_val, 
											int i_set_dsp_flag, float *fp_screen_val);
int dspfxpStudioverbCtrlToScreenHighFreqDecay(DSPFXP_HANDLE *hp_dspfxp, float f_ctrl_val, 
											int i_set_dsp_flag, float *fp_screen_val);
int dspfxpStudioverbCtrlToScreenMixLevel(DSPFXP_HANDLE *hp_dspfxp, float f_ctrl_val, 
											int i_set_dsp_flag, float *fp_screen_val);
int dspfxpStudioverbCtrlToScreenOutputLevel(DSPFXP_HANDLE *hp_dspfxp, float f_ctrl_val, 
											int i_set_dsp_flag, float *fp_screen_val);
int dspfxpStudioverbCtrlToScreenDensity(DSPFXP_HANDLE *hp_dspfxp, float f_ctrl_val, 
											int i_set_dsp_flag, float *fp_screen_val);
int dspfxpStudioverbCtrlToScreenPreDelay(DSPFXP_HANDLE *hp_dspfxp, float f_ctrl_val, 
											int i_set_dsp_flag, float *fp_screen_val);
int dspfxpStudioverbCtrlToScreenMotionDepth(DSPFXP_HANDLE *hp_dspfxp, float f_ctrl_val, 
											int i_set_dsp_flag, float *fp_screen_val);
int dspfxpStudioverbCtrlToScreenMotionRate(DSPFXP_HANDLE *hp_dspfxp, float f_ctrl_val, 
											int i_set_dsp_flag, float *fp_screen_val);

/* dspfxpStudioverbScreenToCtrl.cpp */
int dspfxpStudioverbScreenToCtrlRoomSize(DSPFXP_HANDLE *hp_dspfxp, float f_screen_val, 
															int i_set_dsp_flag, float *fp_ctrl_val, 
															float *fp_quantized_screen_val); 
int dspfxpStudioverbScreenToCtrlDecayTime(DSPFXP_HANDLE *hp_dspfxp, float f_screen_val, 
															int i_set_dsp_flag, float *fp_ctrl_val, 
															float *fp_quantized_screen_val);
int dspfxpStudioverbScreenToCtrlHighFreqRolloff(DSPFXP_HANDLE *hp_dspfxp, float f_screen_val, 
															int i_set_dsp_flag, float *fp_ctrl_val, 
															float *fp_quantized_screen_val);
int dspfxpStudioverbScreenToCtrlHighFreqDecay(DSPFXP_HANDLE *hp_dspfxp, float f_screen_val, 
															int i_set_dsp_flag, float *fp_ctrl_val, 
															float *fp_quantized_screen_val);
int dspfxpStudioverbScreenToCtrlMixLevel(DSPFXP_HANDLE *hp_dspfxp, float f_screen_val, 
															int i_set_dsp_flag, float *fp_ctrl_val, 
															float *fp_quantized_screen_val);
int dspfxpStudioverbScreenToCtrlOutputLevel(DSPFXP_HANDLE *hp_dspfxp, float f_screen_val, 
															int i_set_dsp_flag, float *fp_ctrl_val, 
															float *fp_quantized_screen_val);
int dspfxpStudioverbScreenToCtrlDensity(DSPFXP_HANDLE *hp_dspfxp, float f_screen_val, 
															int i_set_dsp_flag, float *fp_ctrl_val, 
															float *fp_quantized_screen_val);
int dspfxpStudioverbScreenToCtrlPreDelay(DSPFXP_HANDLE *hp_dspfxp, float f_screen_val, 
															int i_set_dsp_flag, float *fp_ctrl_val, 
															float *fp_quantized_screen_val);
int dspfxpStudioverbScreenToCtrlMotionDepth(DSPFXP_HANDLE *hp_dspfxp, float f_screen_val, 
															int i_set_dsp_flag, float *fp_ctrl_val, 
															float *fp_quantized_screen_val);
int dspfxpStudioverbScreenToCtrlMotionRate(DSPFXP_HANDLE *hp_dspfxp, float f_screen_val, 
															int i_set_dsp_flag, float *fp_ctrl_val, 
															float *fp_quantized_screen_val);

#endif //_DSPFXP_STUDIOVERB_H_
