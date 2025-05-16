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
#ifndef _C_PLAY_H_
#define _C_PLAY_H_

/* Local Functions */
void play32_but_bs_350_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bs_416_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bs_416_4757(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bs_495_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bs_495_5656(realtype *xv, realtype *yv, realtype in, realtype *in_bs);

void play32_but_bp_350_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bp_416_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bp_416_4757(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bp_495_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bp_495_5656(realtype *xv, realtype *yv, realtype in, realtype *in_bs);

void play32_cheb_2_bs_350_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_cheb_2_bs_416_4757(realtype *xv, realtype *yv, realtype in, realtype *in_bs);

void play32_but_bs_283_5030(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bs_400_5030(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bs_229_6325(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bs_324_6325(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bs_185_7953(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bs_262_7953(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bs_150_10000(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_but_bs_100_12000(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
void play32_no_filt(realtype *xv, realtype *yv, realtype in, realtype *in_bs);

/* For the demo version, sets the time in prcess and time in demo mode, secs */
/* Was 60 and 5 */
#define DSP_PLAY_PROCESS_TIME 20.0
#define DSP_PLAY_DEMO_TIME 8.0

/* Gain boost for Type II vocal reduction */
#define DSP_PLAY_VOCAL_TYPEII_BOOST 1.1

/* 
 * For the dsp qnt handles which do not change when a knob is changed,
 * however they do change when the samping frequencies change, we set
 * a hard coded MIDI value taken from a chosen preset.
 */
#define DSP_PLAY_AURAL_TUNE_MIDI            53
#define DSP_PLAY_LEX_ROOM_SIZE_MIDI         64
#define DSP_PLAY_LEX_ROLLOFF_MIDI           89
#define DSP_PLAY_LEX_DAMPING_MIDI           81
#define DSP_PLAY_LEX_DEPTH_MIDI             40
#define DSP_PLAY_LEX_RATE_MIDI              28
#define DSP_PLAY_MAX_RELEASE_TIME_BETA_MIDI 85
#define DSP_PLAY_WID_DISPERSION_MIDI        23
#define DSP_PLAY_WID_FREQ_THRESHOLD_MIDI     0

/* Offsets that allow writing to correct DSP parameter spaces */
#define DSP_PLAY_AURAL_PARAM_OFFSET			(0 * DSPS_MAX_NUM_PARAMS * 2)
#define DSP_PLAY_LEX_PARAM_OFFSET			(1 * DSPS_MAX_NUM_PARAMS * 2)
#define DSP_PLAY_WIDENER_PARAM_OFFSET		(2 * DSPS_MAX_NUM_PARAMS * 2)
#define DSP_PLAY_DELAY_PARAM_OFFSET			(3 * DSPS_MAX_NUM_PARAMS * 2)
#define DSP_PLAY_OPTIMIZER_PARAM_OFFSET	(4 * DSPS_MAX_NUM_PARAMS * 2)

/* Used to transfer stereo mode to all dsp parameter sets */
#define DSP_PLAY_STEREO_MODE_INDEX 4

/* The scale values against the dsp max and mins for main effects */
#define PLY_FIDELITY_INTENSITY_MAX_SCALE (realtype)0.8

/* NOTE - added back when redoing MDLY build, not sure what
 * correct value should be, but appears to only affect old
 * MDly based DFX build
 */
#define PLY_AMBIENCE_DECAY_MAX_SCALE (realtype)1.0

/* Scales the maximum boost of the Optimzer component */
#define PLY_OPTIMIZER_BOOST_MAX_SCALE    (realtype)0.7

/* Scales the maximum wideness of the Widener component */
#define PLY_WIDENER_BOOST_MAX_SCALE    (realtype)0.7

/* Factor to add extra overall gain boost to wet and dry when doing ambience */
#define PLY_AMBIENCE_BOOST_FACTOR        (realtype)1.3

/* Ambience Decay endpoints, used with EXP curve shape */
/* The standard ambience setting at midi 51 closely matches the original linear version */
#define PLY_DECAY_MIN_VALUE 0.095
#define PLY_DECAY_MAX_VALUE 0.95

/* Min and max values for the dsp */
#define DSP_PLY_DRY_MIN_VALUE            (realtype)0.7 * PLY_AMBIENCE_BOOST_FACTOR
#define DSP_PLY_DRY_MAX_VALUE            (realtype)0.5 * PLY_AMBIENCE_BOOST_FACTOR
#define DSP_PLY_WET_MIN_VALUE            (realtype)0.3 * PLY_AMBIENCE_BOOST_FACTOR
#define DSP_PLY_WET_MAX_VALUE            (realtype)0.5 * PLY_AMBIENCE_BOOST_FACTOR

/* For bass boost component of play function */
/* Initial settings
#define DSP_PLY_BASSBOOST_MIN_VALUE  0.0
#define DSP_PLY_BASSBOOST_MAX_VALUE 12.0
#define DSP_PLY_BASSBOOST_CENTER_FREQ 100.0
#define DSP_PLY_BASSBOOST_Q 1.5
*/
/* Second set of settings, with initial non-smart optimizer,
 * decided it required too much dynamic boost lowering to
 * allow bass boost
#define DSP_PLY_BASSBOOST_MIN_VALUE  0.0
#define DSP_PLY_BASSBOOST_MAX_VALUE 15.0
#define DSP_PLY_BASSBOOST_CENTER_FREQ 73.4
#define DSP_PLY_BASSBOOST_Q 2.5
*/
#define DSP_PLY_BASSBOOST_MIN_VALUE  0.0
#define DSP_PLY_BASSBOOST_MAX_VALUE  15.0
#define DSP_PLY_BASSBOOST_CENTER_FREQ 90.0
#define DSP_PLY_BASSBOOST_Q 2.5

/* Vocal Reduction component of play function */
#define DSP_PLY_VOCAL_REDUCTION_MIN_VALUE 0.0
#define DSP_PLY_VOCAL_REDUCTION_MAX_VALUE 1.0

/* Maximum delay used in headphone multi-tap delay */
#define DSP_PLY_MAX_ELEMENT_DELAY 36.0

/* Gain and wet dry mix */
#define PLY_DELAY_GAIN 1.0
#define PLY_HEADPHONE_DRY (0.5 * PLY_DELAY_GAIN)
#define PLY_HEADPHONE_WET (0.5 * PLY_DELAY_GAIN)

/* Cross feed gain of small cross delays */
#define PLY_HEADPHONE_CROSSGAIN 0.707

/* Overall delay factor */
#define PLY_DELAY_FACTOR 0.75

/* Amounts of headphone delay elements in millsecs */
#define PLY_HEADPHONE_DELAY0 29.0
#define PLY_HEADPHONE_DELAY1 32.7
#define PLY_HEADPHONE_DELAY2 13.0
#define PLY_HEADPHONE_DELAY3 21.8
#define PLY_HEADPHONE_DELAY4 20.6
#define PLY_HEADPHONE_DELAY5 16.7
#define PLY_HEADPHONE_DELAY6 36.0
#define PLY_HEADPHONE_DELAY7 27.2

/* Headphone element feedback settings */
#define PLY_HEADPHONE_FEEDBACK0 -0.66
#define PLY_HEADPHONE_FEEDBACK1 0.58
#define PLY_HEADPHONE_FEEDBACK2 0.8
#define PLY_HEADPHONE_FEEDBACK3 -0.66
#define PLY_HEADPHONE_FEEDBACK4 0.66
#define PLY_HEADPHONE_FEEDBACK5 -0.78
#define PLY_HEADPHONE_FEEDBACK6 -0.46
#define PLY_HEADPHONE_FEEDBACK7 -0.4

#define PLY_MASTER_FEEDBACK 0.5

/* Headphone pan gain settings */
#define PLY_HEADPHONE_PAN_SETTING0 0.12
#define PLY_HEADPHONE_PAN_SETTING1 1.0
#define PLY_HEADPHONE_PAN_SETTING2 -0.08
#define PLY_HEADPHONE_PAN_SETTING3 -0.74
#define PLY_HEADPHONE_PAN_SETTING4 0.76
#define PLY_HEADPHONE_PAN_SETTING5 0.42
#define PLY_HEADPHONE_PAN_SETTING6 -1.0
#define PLY_HEADPHONE_PAN_SETTING7 -0.4

/* Play specific parameters.
 * Note that since this memory space is shared with the first
 * effect, the aural activator, we must use the parameter space
 * above the activators last parameter.
 */
#define DSP_PLAY_BYPASS_ON				36L + COMM_MEM_OFFSET
#define DSP_PLAY_ACTIVATOR_ON			37L + COMM_MEM_OFFSET
#define DSP_PLAY_AMBIENCE_ON			38L + COMM_MEM_OFFSET
#define DSP_PLAY_WIDENER_ON			39L + COMM_MEM_OFFSET
#define DSP_PLAY_BASS_BOOST_ON		40L + COMM_MEM_OFFSET
#define DSP_PLAY_HEADPHONE_ON			41L + COMM_MEM_OFFSET
#define DSP_PLAY_RESET_DEMO_COUNT   42L + COMM_MEM_OFFSET

#define DSP_PLAY_B0						43L + COMM_MEM_OFFSET
#define DSP_PLAY_B1						44L + COMM_MEM_OFFSET
#define DSP_PLAY_B2						45L + COMM_MEM_OFFSET
#define DSP_PLAY_A1						46L + COMM_MEM_OFFSET
#define DSP_PLAY_A2						47L + COMM_MEM_OFFSET

#define DSP_PLAY_B0_LP					48L + COMM_MEM_OFFSET
#define DSP_PLAY_B1_LP					49L + COMM_MEM_OFFSET
#define DSP_PLAY_B2_LP					50L + COMM_MEM_OFFSET
#define DSP_PLAY_B3_LP					51L + COMM_MEM_OFFSET
#define DSP_PLAY_B4_LP					52L + COMM_MEM_OFFSET
#define DSP_PLAY_A1_LP					53L + COMM_MEM_OFFSET
#define DSP_PLAY_A2_LP					54L + COMM_MEM_OFFSET
#define DSP_PLAY_A3_LP					55L + COMM_MEM_OFFSET
#define DSP_PLAY_A4_LP					56L + COMM_MEM_OFFSET

#define DSP_VOCAL_REDUCTION_VAL		57L + COMM_MEM_OFFSET
#define DSP_PLAY_VOCAL_REDUCTION_ON	58L + COMM_MEM_OFFSET
#define DSP_PLAY_VOCAL_REDUCTION_MODE 59L + COMM_MEM_OFFSET

/* Special structure used for parameters and state of algorithm */
struct dspPlayStructType
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
	long unassigned20; 
	long unassigned21;
	long unassigned22;
	long unassigned23;
	long unassigned24;
	long unassigned25;
	long unassigned26;
	long unassigned27;
	long unassigned28;
	long unassigned29;
	long unassigned30;
	long unassigned31;
	long unassigned32;
	long unassigned33; /* End of currently assigned Aural Activator parameters/states */
	long unassigned34; /* Room for 2 more if needed */
	long unassigned35;

	/* Start play specific parameters, above activator parameters */
	/* Note- algorithm specific parameters must occupy same 32 word locations
	 * as defines below.
	 */
	long bypass_on;
	long activator_on;
	long ambience_on;
	long widener_on;
	long bassboost_on;
	long headphone_on;
	long reset_demo_count;

	/* Bass boost coeffs */
	realtype b0;
	realtype b1;
	realtype b2;
	realtype a1;
	realtype a2;

	/* Vocal Elminator Lowpass Filter Coeffs */
	realtype b0_lp;
	realtype b1_lp;
	realtype b2_lp;
	realtype b3_lp;
	realtype b4_lp;
	realtype a1_lp;
	realtype a2_lp;
	realtype a3_lp;
	realtype a4_lp;

	/* Vocal Eliminator Hipass Filter Coeffs */
	/*
	realtype b0_hp;
	realtype b1_hp;
	realtype b2_hp;
	realtype a1_hp;
	realtype a2_hp;
    */

	unsigned long vocal_elim_val;
	long vocal_elim_on;
	long vocal_mode;

	/* Play internal state parameters */
	long bypass_mode;
	unsigned long sample_count;
	unsigned long max_sample_count_process;
	unsigned long max_sample_count_demo;
	unsigned long last_vocal_val;
	long last_mode;

	/* Play algorithm state variables */
	/* The filter coeffs will be set during initialization, not from parameter changes */
	/* Bass boost states */
	realtype in1_w1;
	realtype in1_w2;
	realtype in2_w1;
	realtype in2_w2;

	/* For Vocal Eliminator lowpass */
	realtype in1_w1_lp;
	realtype in1_w2_lp;
	realtype in1_w3_lp;
	realtype in1_w4_lp;
	realtype in2_w1_lp;
	realtype in2_w2_lp;
	realtype in2_w3_lp;
	realtype in2_w4_lp;

	realtype out1_w1_lp;
	realtype out1_w2_lp;
	realtype out1_w3_lp;
	realtype out1_w4_lp;
	realtype out1_w1_hp;
	realtype out1_w2_hp;
	realtype out1_w3_hp;
	realtype out1_w4_hp;
	realtype out2_w1_lp;
	realtype out2_w2_lp;
	realtype out2_w3_lp;
	realtype out2_w4_lp;
	realtype out2_w1_hp;
	realtype out2_w2_hp;
	realtype out2_w3_hp;
	realtype out2_w4_hp;

	/* For now, use parameter space for delay lines.
	 * Note that the delay_lines param must be the last parameter since
	 * there will be an array of data written at that address space.
	 */
	unsigned long head_delay;
	unsigned long delay_line_index;
	realtype delay_lines;
};

#endif /* _C_PLAY_H_ */
