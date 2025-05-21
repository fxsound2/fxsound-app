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
#include "codedefs.h"
#include "slout.h" 

/* For functions that need number of midi steps */
#define QNT_NUM_MIDI_STEPS 128

/* Response type defines */     
/* Used for most knob to real mappings */
/* See also QNT_RESPONSE_LINEAR_NO_ROUND */
#define QNT_RESPONSE_LINEAR 		     0

/* Used for volume knobs and sliders where control range is MIDI oriented (0 - 127).
 * Note that this is a special mapping coded into the qntInitIToR function.
 * This flag yields the linear output (warped to a db oriented response) that
 * is used for transfer to the DSP and to DISPLAY linear output values (not db).
 */ 
#define QNT_RESPONSE_MIDI_VOLUME         1

/* Used for displaying volume knobs and sliders in db. Note that this is 
 * a special mapping coded into the qntInitIToR function
 */
#define QNT_RESPONSE_MIDI_VOLUME_DISPLAY 2 

/* The smallest usable db level (one tick to the right of off) that is used with
 * this MIDI oriented volume control mapping.  Note that with the 128 output
 * control, for a knob with unity gain full on, -31.5 gives 0.25 db steps.
 */
#define QNT_RESPONSE_MIDI_VOLUME_DB_MIN -31.5

/* The value in db that represents a linear gain of 0.0 (should be - infinity)
 * an alternative value would be a minimum float value from FLOAT.H 
 */ 
#define QNT_DB_VOLUME_OFF -1500.0

/* This define is used for controls such as Low Frequency Oscillators.
 * It forces an "equal ratio" response to the input so that each control
 * increment implies the same multiplicative factor on the output.
 */
#define QNT_RESPONSE_EXP				3

/* Used for control functions such as VCO's that start at a small value (ie. 0.05 - 15 hz).
 * This one generates a two part linear response that adds resolution on the low end. 
 * Low end breakpoint depends on ratio of start val to end val.
 * a special mapping coded into the qntInitIToR function
 */
#define QNT_RESPONSE_TWO_PART_LINEAR	4
 
/* Used for audio range frequency functions such as filter center points.
 * This puts points at 20 and 20khz, and right on musical tones with 440 tuning.
 */
#define QNT_RESPONSE_EXP_FREQ	5

/* Used for filter Q settings.
 */
#define QNT_RESPONSE_Q_TYPE		6

/* This is a special function for knobs such as frequency fine controls.
 * It has an exponential response up and down around a unity center point.
 */
#define QNT_RESPONSE_EXP_FACTOR	7

/* Applies a sqrt shaped curve that intersects the endpoint values.
 */
#define QNT_RESPONSE_SQRT	8

/* Same as QNT_RESPONSE_LINEAR but doesn't do decimal rounding */
#define QNT_RESPONSE_LINEAR_NO_ROUND 9

/* Special qnt shapes for maximizer boost and output. Display 
 * versions are in db, DSP versions are linear equivalent.
 */
#define QNT_RESPONSE_MAXI_BOOST 10
#define QNT_RESPONSE_MAXI_BOOST_DSP 11
#define QNT_RESPONSE_MAXI_MAX_OUTPUT 12
#define QNT_RESPONSE_MAXI_MAX_OUTPUT_DSP 13

/* qnt.c */
int PT_DECLSPEC qntFreeUp(PT_HANDLE **);
int PT_DECLSPEC qntDump(PT_HANDLE *);

/* qntIToR.cpp */
int PT_DECLSPEC qntIToRInit(PT_HANDLE **, CSlout *, int, int, realtype, realtype,
					 int, int, int, realtype, int, int);

/* qntIToR2.cpp */
int PT_DECLSPEC qntIToRInitReverbFeedback(PT_HANDLE **, CSlout *, int, int, 
								realtype, realtype, realtype, realtype);
int PT_DECLSPEC qntIToRCalc(PT_HANDLE *hp_qnt, int, realtype *);  
int PT_DECLSPEC qntIToRCalcFromOut(PT_HANDLE *hp_qnt, realtype, int *, realtype *); 
int PT_DECLSPEC qntIToRGetHalfDelta(PT_HANDLE *hp_qnt, realtype *);
int PT_DECLSPEC qntIToRInitTrackPitchIToR(PT_HANDLE **, PT_HANDLE *, CSlout *);
int PT_DECLSPEC qntIToRInitPitchCompIToR(PT_HANDLE **, PT_HANDLE *, CSlout *);
int PT_DECLSPEC qntIToRInitPitchSpliceDelay(PT_HANDLE **, PT_HANDLE **, PT_HANDLE *, CSlout *);
int PT_DECLSPEC qntIToRdBCalcInit(PT_HANDLE **, PT_HANDLE *, CSlout *, realtype);
int PT_DECLSPEC qntIToRTimeConstantBeta(PT_HANDLE **hpp_beta_qnt, PT_HANDLE *hp_time_constant_qnt, CSlout *hp_slout,
													 realtype r_sampling_freq);
/* qntIToL.c */
int PT_DECLSPEC qntIToLInit(PT_HANDLE **, CSlout *, int, int, long, long,
					 int, int, int);
int PT_DECLSPEC qntIToLInitTrackIToR(PT_HANDLE **, PT_HANDLE *, CSlout *, long, long);
int PT_DECLSPEC qntIToLCalc(PT_HANDLE *hp_qnt, int, long *);

/* qntRToI.c */
int PT_DECLSPEC qntRToIInit(PT_HANDLE **, CSlout *, realtype, realtype, int, int, int, int, int);
int PT_DECLSPEC qntRToICalc(PT_HANDLE *hp_qnt, realtype, int *);
int PT_DECLSPEC qntRToICalcFromOut(PT_HANDLE *hp_qnt, int, realtype *);

/* qntRToL.c */
int PT_DECLSPEC qntRToLInit(PT_HANDLE **, CSlout *, realtype, realtype, long, long, int, int);
int PT_DECLSPEC qntRToLCalc(PT_HANDLE *hp_qnt, realtype, long *);
int PT_DECLSPEC qntRToLCalcFromOut(PT_HANDLE *hp_qnt, long, realtype *);

/* qntRToR.c */
int PT_DECLSPEC qntRToRInit(PT_HANDLE **, CSlout *, realtype, realtype, realtype, realtype,
					 int, int, int);
int PT_DECLSPEC qntRToRCalc(PT_HANDLE *hp_qnt, realtype, realtype *);
int PT_DECLSPEC qntRToRCalcFromOut(PT_HANDLE *hp_qnt, realtype, realtype *);

/* qnt2But.cpp */
int PT_DECLSPEC qnt2ndOrderButterworthInit(PT_HANDLE **hpp_filter_gain_qnt,
					 PT_HANDLE **hpp_filter_a1_qnt,
					 PT_HANDLE **hpp_filter_a0_qnt,
					 CSlout *hp_slout,
					 int i_input_min, int i_input_max,
					 realtype r_omega_min, realtype r_omega_max,
					 int i_response_type);
int PT_DECLSPEC qntIToRSimpleLowpassInit(PT_HANDLE **hpp_coeff_qnt, PT_HANDLE *hp_display_freq_qnt, CSlout *hp_slout,
													  realtype r_sampling_period, realtype r_scale_freq);

/* qntIToBoostCut.cpp */
int PT_DECLSPEC qntIToBoostCutInit(PT_HANDLE **hpp_qnt, CSlout *hp_slout,
					 int i_input_min, int i_input_max,
					 realtype r_boost_min,
					 realtype r_boost_max,
					 realtype r_center_freq,
					 realtype r_samp_freq,
					 realtype r_Q,
					 int i_filter_type);
int PT_DECLSPEC qntIToBoostCutCalc(PT_HANDLE *hp_qnt, int i_input,
											  struct filt2ndOrderBoostCutShelfFilterType *sp_filt);


