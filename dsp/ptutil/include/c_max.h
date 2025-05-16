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
#ifndef _C_MAX_H_
#define _C_MAX_H_

/* Defines */
#define MAXIMIZE_NUM_QUANTIZE_SETTINGS 5
#define MAXIMIZE_NUM_DITHER_TYPES 4
#define MAXIMIZE_DITHER_NAME_0 "None"
#define MAXIMIZE_DITHER_NAME_1 "Uniform"
#define MAXIMIZE_DITHER_NAME_2 "Triangular"
#define MAXIMIZE_DITHER_NAME_3 "Shaped"

#define MAXIMIZE_DITHER_NAME_ABRIVIATED_0 "None"
#define MAXIMIZE_DITHER_NAME_ABRIVIATED_1 "Uniform"
#define MAXIMIZE_DITHER_NAME_ABRIVIATED_2 "Triang."
#define MAXIMIZE_DITHER_NAME_ABRIVIATED_3 "Shaped"

#define MAXIMIZE_NOISE_SEED 10322234L

/* Constants */
/* Used to set both display values and to set DSP qnt handle. */
#define MAXIMIZE_MIN_TIME_CONST 0.1
#define MAXIMIZE_MAX_TIME_CONST 100.0

#define DSP_MAXIMIZE_GAIN_BOOST_MIN_VALUE 0
#define DSP_MAXIMIZE_GAIN_BOOST_MAX_VALUE 30.0

#define DSP_MAXIMIZE_MAX_OUTPUT_MIN_VALUE -30.0
#define DSP_MAXIMIZE_MAX_OUTPUT_MAX_VALUE 0.0

#define MAXI_MAX_DELAY_LEN 96 /* Max amount of space reserved for look ahead */
#define MAXI_ENVELOPE_BIAS 1.0e-24 /* Bias on envelope to avoid under flow */
#define MAXI_LOOK_AHEAD_DELAY 0.00075 /* Look ahead delay */

/* Targeted boosted output level */
/* #define MAXIMIZE_TARGET_LEVEL_SETTING 0.32 */
#define MAXIMIZE_TARGET_LEVEL_SETTING 0.32

/* Cutoff of the one pole lowpass level filter in hz */
/* #define MAXIMIZE_LEVEL_FILT_CUTOFF 0.06 */
#define MAXIMIZE_LEVEL_FILT_CUTOFF 0.1

/* Special structure used for parameters and state of algorithm */
struct dspMaxiStructType
{
	/* Parameters common to all dsp functions */
	/* Note- must occupy same 32 word locations as defines in Boardrv1.h */
	long pc_to_dsp_flags;
	long dsp_to_pc_flags;
	long dsp_number_of_elements;
	realtype dsp_sampling_freq;
	long stereo_in_flag;
	long dsp_mute_in_flag;
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

	/* Algorithm parameters */
	realtype gain_boost;
	realtype max_output;
	realtype release_time_beta; /* To be replaced by quant funct that sends delay instead */
	long num_quant_bits;
	long dither_type;
	int max_delay;
	int quantize_on_flag;
	/* Added for auto mode, the max desired boosted output level */
	realtype target_level;

	/* Algorithm state variables */
	float dly_start_l[MAXI_MAX_DELAY_LEN];
	float dly_start_r[MAXI_MAX_DELAY_LEN];
	float *ptr_l;
	float *ptr_r;
	float max_abs_l;
	float max_abs_r;
	float delta_l;
	float delta_r;
	float env_l;
	float env_r;
	int ramp_count_l;
	int ramp_count_r;
	float noise1_old;
	float noise2_old;
	/* New variables for auto mode */
	/* Trying double versions to see if normalization problem goes away */
	/*
	float level;
	float a0;
	float filt_gain;
	 */

	double level;
	double a0;
	double filt_gain;

};

/* Algorithm specific parameters */
#define MAXIMIZE_GAIN_BOOST        20L + COMM_MEM_OFFSET
#define MAXIMIZE_MAX_OUTPUT        21L + COMM_MEM_OFFSET
#define MAXIMIZE_RELEASE_TIME      22L + COMM_MEM_OFFSET
#define MAXIMIZE_QUANTIZE_NUM_BITS 23L + COMM_MEM_OFFSET
#define MAXIMIZE_DITHER_TYPE       24L + COMM_MEM_OFFSET
#define MAXIMIZE_MAX_DELAY         25L + COMM_MEM_OFFSET
#define MAXIMIZE_QUANTIZE_ON       26L + COMM_MEM_OFFSET
#define MAXIMIZE_TARGET_LEVEL      27L + COMM_MEM_OFFSET

#endif /* _C_MAX_H_ */
