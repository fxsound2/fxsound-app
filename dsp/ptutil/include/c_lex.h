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
#ifndef _C_LEX_H_
#define _C_LEX_H_

#include "dspfxp_studioverb.h"

/* Value defines */
/* High frequency decay and rolloffs in kHz, used on screen
 * and to initialize DSP parameter qnt handles.
 */
#define LEX_HIGH_FREQ_DECAY_MIN_VAL DSPFXP_STUDIOVERB_SCREEN_HIGH_FREQ_DECAY_MIN
#define LEX_HIGH_FREQ_DECAY_MAX_VAL DSPFXP_STUDIOVERB_SCREEN_HIGH_FREQ_DECAY_MAX 
#define LEX_HIGH_FREQ_ROLLOFF_MIN_VAL DSPFXP_STUDIOVERB_SCREEN_HIGH_FREQ_ROLLOFF_MIN
#define LEX_HIGH_FREQ_ROLLOFF_MAX_VAL DSPFXP_STUDIOVERB_SCREEN_HIGH_FREQ_ROLLOFF_MAX 

#define DSP_DECAY_MIN_VALUE 0.0
#define DSP_DECAY_MAX_VALUE 1.0

/* First input diffuser coeff, depends on density setting */
#define DSP_LEX_LAT1_MIN_VALUE (0.75 * 0.4)
#define DSP_LEX_LAT1_MAX_VALUE 0.75

/* Second input diffuser coeff, depends on density setting */
#define DSP_LEX_LAT3_MIN_VALUE (0.625 * 0.4)
#define DSP_LEX_LAT3_MAX_VALUE 0.625

#define DSP_LEX_LAT5_MIN_VALUE 0.0
#define DSP_LEX_LAT5_MAX_VALUE 1.0

#define DSP_LEX_LAT6_MIN_VALUE 0.0
#define DSP_LEX_LAT6_MAX_VALUE 1.0

/* Nominal algorithm value is 1.0, above 1.5 gets too surgy */
#define DSP_LEX_ROOM_SIZE_MIN_VALUE 0.5
#define DSP_LEX_ROOM_SIZE_MAX_VALUE 1.5

#define DSP_LEX_MOTION_RATE_MIN_VALUE 0.2
#define DSP_LEX_MOTION_RATE_MAX_VALUE 2.0

/* Note extra point to handle roundoff problem */
#define LEX_NUM_OSC_PTS (8192+1)

#define LEX_OSC_PHASE 0.0
#define LEX_PREDELAY_MAX_MS	100.0
#define LEX_MODULATION_DELAY_MAX_MS	2.0

#define LEX_MAX_MODULATION_DELAY (LEX_MODULATION_DELAY_MAX_MS/1000.0)

#define LEX_PREDELAY_LEN		(LEX_PREDELAY_MAX_MS/1000.0)
/* Total nominal delay, including predelay and modulation (default room size) 
 * 100 + 4.77 + 3.595 + 12.73 + 9.31 + (22.6 + 2.0) + 149.6 + 60.5 + 125.0
 * + (30.5 + 2.0) + 141.7 + 89.2 + 106.3 = 859.805 ms.
 */
#define LAT1_LEFT_DELAY_LEN   (4.77/1000.0)
#define LAT2_LEFT_DELAY_LEN   (3.595/1000.0)
#define LAT3_LEFT_DELAY_LEN   (12.73/1000.0)
#define LAT4_LEFT_DELAY_LEN   (9.31/1000.0)
#define LAT5_LEFT_DELAY_LEN_NOMINAL (22.6/1000.0)
/* Next is first right early reflection. Modified for better balance */
/* #define D1_LEFT_TAP1_DELAY    (11.9/1000.0) */
#define D1_LEFT_TAP1_DELAY    (10.1/1000.0)
#define D1_LEFT_TAP2_DELAY    (66.9/1000.0)
#define D1_LEFT_TAP3_DELAY    (121.9/1000.0)
#define D1_LEFT_TAP4_DELAY    (149.6/1000.0)
#define LAT6_LEFT_TAP1_DELAY  (6.28/1000.0)
#define LAT6_LEFT_TAP2_DELAY  (41.26/1000.0)
#define LAT6_LEFT_DELAY_LEN   (60.5/1000.0)
#define D2_LEFT_TAP1_DELAY    (35.8/1000.0)
#define D2_LEFT_TAP2_DELAY    (89.8/1000.0)
#define D2_LEFT_TAP3_DELAY    (125.0/1000.0)
#define LAT7_LEFT_DELAY_LEN_NOMINAL (30.5/1000.0)
/* Next is first left early reflection. Modified for better balance */
/* #define D3_LEFT_TAP1_DELAY    (8.93/1000.0) */
#define D3_LEFT_TAP1_DELAY    (10.1/1000.0)
#define D3_LEFT_TAP2_DELAY    (70.9/1000.0)
#define D3_LEFT_TAP3_DELAY    (99.9/1000.0)
#define D3_LEFT_TAP4_DELAY    (141.7/1000.0)
#define LAT8_LEFT_TAP1_DELAY  (11.25/1000.0)
#define LAT8_LEFT_TAP2_DELAY  (64.3/1000.0)
#define LAT8_LEFT_DELAY_LEN   (89.2/1000.0)
#define D4_LEFT_TAP1_DELAY    (4.065/1000.0)
#define D4_LEFT_TAP2_DELAY    (67.1/1000.0)
#define D4_LEFT_TAP3_DELAY    (106.3/1000.0)

/* Special structure used for parameters and state of algorithm */
struct dspLexStructType
{
	/* Parameters common to all dsp functions */
	/* Note- must occupy same 32 word locations as defines in Boardrv1.h */
	long pc_to_dsp_flags;
	long dsp_to_pc_flags;
	long dsp_number_of_elements;
	realtype dsp_sampling_freq;
	long stereo_in_flag;
	float dsp_mute_in_flag;
	long unassigned6;
	long unassigned7;
	long unassigned8;
	long unassigned9;
	realtype dry_gain;
	realtype wet_gain;
	realtype master_gain;
	long dsp_dma_in_transfer;
	long unassigned14;
	long unassigned15;
	long unassigned16;
	long unassigned17;
	long unassigned18;
	long unassigned19;

	/* Note- algorithm specific parameters must occupy same 32 word locations
	 * as defines below.
	 */
	/* Algorithm parameters, mapped to memory locations below */
	float roomsize;
	float decay;
	float lat1_coeff;
	float lat3_coeff;
	float lat5_coeff;
	float lat6_coeff;
	float bandwidth;
	float one_minus_bandwidth;
	float damping;
	float one_minus_damping;
	unsigned long pre_delay;
	float modulation_depth;
	float modulation_freq;

	/* Algorithm state variables */
	float *osc_mult_table;
	float *osc_plus_table;
	float osc_table_p;                    
	float f_num_pts;

	float *ptr;
	unsigned long MasterLen;
	float *MasterStart;
	float *MasterEnd;

	unsigned long pre_dly_len_l;

	unsigned long lat1_dly_len_l;

	unsigned long lat2_dly_len_l;

	unsigned long lat3_dly_len_l;

	unsigned long lat4_dly_len_l;

	float lat5_dly_len_l;
	unsigned long lat5_dly_maxlen_l;

	unsigned long D1_tap1;
	unsigned long D1_tap2;
	unsigned long D1_tap3;
	unsigned long D1_tap4;

	unsigned long lat6_tap1;
	unsigned long lat6_tap2;
	unsigned long lat6_dly_len_l;
	float lat6_out_old_l;

	unsigned long D2_tap1;
	unsigned long D2_tap2;
	unsigned long D2_tap3;

	float lat7_dly_len_l;
	unsigned long lat7_dly_maxlen_l;

	unsigned long D3_tap1;
	unsigned long D3_tap2;
	unsigned long D3_tap3;
	unsigned long D3_tap4;

	unsigned long lat8_tap1;
	unsigned long lat8_tap2;
	unsigned long lat8_dly_len_l;
	float lat8_out_old_l;

	unsigned long D4_tap1;
	unsigned long D4_tap2;
	unsigned long D4_tap3;
	float D4_out;

	float old_damp_val1_l;
	float old_damp_val2_l;
	float old_bandwidth_val_l;
};

/* Algorithm specific parameters */
#define LEX_ROOM_SIZE                   20L + COMM_MEM_OFFSET
#define LEX_DECAY                       21L + COMM_MEM_OFFSET
#define LEX_LAT1_COEFF                  22L + COMM_MEM_OFFSET
#define LEX_LAT3_COEFF                  23L + COMM_MEM_OFFSET
#define LEX_LAT5_COEFF                  24L + COMM_MEM_OFFSET
#define LEX_LAT6_COEFF                  25L + COMM_MEM_OFFSET
#define LEX_ROLLOFF                     26L + COMM_MEM_OFFSET
#define LEX_ONE_MINUS_ROLLOFF           27L + COMM_MEM_OFFSET
#define LEX_DAMPING                     28L + COMM_MEM_OFFSET
#define LEX_ONE_MINUS_DAMPING           29L + COMM_MEM_OFFSET
#define LEX_PRE_DELAY                   30L + COMM_MEM_OFFSET
#define LEX_MOTION_DEPTH                31L + COMM_MEM_OFFSET
#define LEX_MOTION_RATE                 32L + COMM_MEM_OFFSET

#endif /* _C_LEX_H_ */
