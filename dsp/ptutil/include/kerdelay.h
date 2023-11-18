/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: kerdelay.h
 * DATE: 3/21/95
 * AUTOHOR: Paul F. Titchener
 * DESCRIPTION: Delay Line and Reverberation Macros
 *
 */

/*
 * MACRO: kerRunDelayLineFdbk
 * DESCRIPTION: Runs a tapped delay line, with changing delay and feedback.
 * r_out comes out with oldest data; rp_data comes out pointing at next oldest
 * piece of data, which is now the oldest available in the delay line.
 * Note that this routine gives a delay to r_out of ip_delay, with a minimum
 * delay of 1. (Requesting 0 or 1 gives a delay of one).
 * Available memory space should be ip_delay.
 */
#define kerRunDelayLineFdBk(r_in, r_out, rp_start, rp_data, ip_delay, rp_feedback_gain) \
	  r_out = *rp_data; \
	  *rp_data = r_in + *(volatile float *)rp_feedback_gain * (*rp_data); \
	  rp_data++; \
	  if(rp_data >= ((float *)rp_start + *(volatile long *)ip_delay) ) \
	  { \
		 rp_data = (float *)rp_start; \
	  }

/*
 * MACRO: kerRunDelayLineFdBkRev
 * DESCRIPTION: Runs a tapped delay line, with changing delay and feedback.
 * r_out comes out with oldest data; rp_data comes out pointing at next oldest
 * piece of data, which is now the oldest available in the delay line.
 * Note that this routine gives a delay to r_out of ip_delay, with a minimum
 * delay of 1. (Requesting 0 or 1 gives a delay of one).
 * Available memory space should be ip_delay.
 */
#define kerRunDelayLineFdBkRev(r_in, r_out, rp_start, rp_data, i_delay, r_feedback_gain) \
	  r_out = *rp_data; \
	  *rp_data = r_in + r_feedback_gain * (*rp_data); \
	  rp_data++; \
	  if(rp_data >= ((float *)rp_start + i_delay) ) \
	  { \
		 rp_data = (float *)rp_start; \
	  }

/*
 * MACRO: kerRunDelayLineNoPop
 * DESCRIPTION: Runs a tapped delay line, with changing delay.
 * This version uses additional processing to always use the full delay
 * line for signal storage, causing less pops when delay is changing.
 * r_out comes out with the selected delay data; rp_data comes out pointing
 * at the oldest available data in the delay line.
 * Note that this routine gives a delay to r_out of ip_delay, with a minimum
 * delay of 1. (Requesting 0 or 1 gives a delay of one).
 * Available memory space should be i_line_len.
 * NOTE-BE CAREFUL WHEN SUBTRACTING FROM POINTERS, THEY ARE UNSIGNED
 */
#define kerRunDelayLineNoPop(r_in, r_out, i_delay, rp_start, rp_end, rp_data, i_line_len) \
{ \
	float *rp_MACRO; \
	rp_MACRO = (float *)(rp_data - i_delay ); \
	if((long)rp_MACRO < (long)rp_start) \
		rp_MACRO += (i_line_len); \
	r_out = *rp_MACRO; \
	*rp_data = r_in; \
	rp_data++; \
	if(rp_data >= ( (float *)(rp_end)) ) \
	    rp_data = (float *)rp_start; \
}                           

/*
 * MACRO: kerRunDelayLineFdbkNoPop
 * DESCRIPTION: Runs a tapped delay line, with changing delay and feedback.
 * This version uses additional processing to always use the full delay
 * line for signal storage, causing less pops when delay is changing.
 * r_out comes out with the selected delay data; rp_data comes out pointing
 * at the oldest available data in the delay line.
 * Note that this routine gives a delay to r_out of ip_delay, with a minimum
 * delay of 1. (Requesting 0 or 1 gives a delay of one).
 * Available memory space should be i_line_len.
 * NOTE-BE CAREFUL WHEN SUBTRACTING FROM POINTERS, THEY ARE UNSIGNED
 */
#define kerRunDelayLineFdBkNoPop(r_in, r_out, rp_start, rp_data, ip_del, rp_fdbk, i_line_len) \
{ \
	float *rp_MACRO; \
	rp_MACRO = (float *)(rp_data - *(volatile long *)ip_del ); \
	if((long)rp_MACRO < (long)rp_start) \
		rp_MACRO += (i_line_len); \
	r_out = *rp_MACRO; \
	*rp_data = r_in + *(volatile float *)rp_fdbk * r_out; \
	rp_data++; \
	if(rp_data >= ( (float *)(rp_start + i_line_len)) ) \
	    rp_data = (float *)rp_start; \
}                           

/*
 * MACRO: kerRunDelayLineFdbkNoPopRev
 * DESCRIPTION: Runs a tapped delay line, with changing delay and feedback.
 * This version uses additional processing to always use the full delay
 * line for signal storage, causing less pops when delay is changing.
 * r_out comes out with the selected delay data; rp_data comes out pointing
 * at the oldest available data in the delay line.
 * Note that this routine gives a delay to r_out of ip_delay, with a minimum
 * delay of 1. (Requesting 0 or 1 gives a delay of one).
 * Available memory space should be i_line_len.
 * NOTE-BE CAREFUL WHEN SUBTRACTING FROM POINTERS, THEY ARE UNSIGNED
 * THIS VERSION USES DIRECT VALS FOR FEEDBACK AND DELAY (for REVERBS)
 */
#define kerRunDelayLineFdBkNoPopRev(r_in, r_out, rp_start, rp_data, i_del, r_fdbk, i_line_len) \
{ \
	float *rp_MACRO; \
	rp_MACRO = (float *)(rp_data - (long)i_del ); \
	if((long)rp_MACRO < (long)rp_start) \
		rp_MACRO += (i_line_len); \
	r_out = *rp_MACRO; \
	*rp_data = r_in + (float)r_fdbk * r_out; \
	rp_data++; \
	if(rp_data >= ( (float *)(rp_start + i_line_len)) ) \
	    rp_data = (float *)rp_start; \
}

/*
 * MACRO: kerRunDelayLine
 * DESCRIPTION: Runs a tapped delay line, with changing delay.
 * See description above for details on usage.
 */
#define kerRunDelayLine(r_in, r_out, rp_start, rp_data, ip_delay) \
	  r_out = *rp_data; \
	  *rp_data = r_in; \
	  rp_data++; \
	  if(rp_data >= ((float *)rp_start + *(volatile long *)ip_delay) ) \
	  { \
		 rp_data = (float *)rp_start; \
	  }

/*
 * MACRO: kerRunFixedDelayLine
 * DESCRIPTION: Runs a tapped delay line, with fixed delay
 * See description above for details on usage
 */
#define kerRunFixedDelayLine(r_in, r_out, rp_start, rp_data, i_delay) \
	  r_out = *rp_data; \
	  *rp_data = r_in; \
	  rp_data++; \
	  if(rp_data >= ((float *)rp_start + i_delay) ) \
	  { \
		 rp_data = (float *)rp_start; \
	  }

/* MACRO: kerGetDelayData to get a delayed data point from a delay line.
 * IT MUST BE RUN BEFORE THE MASTER DELAY LINE UPDATE FOR RESULTS DESCRIBED.
 * It assumes rp_current_point is pointing at oldest data. ip_delay is
 * the desired delay amount, with 1 being the minimum delay.  Requesting
 * zero delay will result in a delay of i_line_len. i_line_len is the amount
 * of delay being run in the delay line (see delay macros).
 *
 * IF RUN AFTER THE MASTER DELAY LINE UPDATE- you must request the amount of
 * delay you require + 1; for example, ip_delay = 1 yields a true delay of zero.
 * Maximum delay acheivable is ip_delay - 1.
 * NOTE-BE CAREFUL WHEN SUBTRACTING FROM POINTERS, THEY ARE UNSIGNED
 */
#define kerGetDelayData(rp_start, i_line_len, rp_oldest, ip_delay, rp_out) \
	rp_out = (float *)(rp_oldest - *(volatile long *)ip_delay ); \
	if((long)rp_out < (long)rp_start) \
		rp_out += i_line_len;
		
/* This is similar to above, but gets value directly instead of via a delay pointers */
#define kerGetDelayDataDirect(rp_start, i_line_len, rp_oldest, i_delay, rp_out) \
	rp_out = (float *)(rp_oldest - (long )i_delay ); \
	if((long)rp_out < (long)rp_start) \
		rp_out += i_line_len;

/*
 * MACRO: kerSumGainandPan
 * DESCRIPTION: Calculates gain and panned vals, sums onto output values
 *
 */
#define kerSumGainandPan(r_signal, rp_gain, rp_pan_left, rp_pan_right, \
								 r_left_out, r_right_out) \
	  r_signal *= *(volatile float *)(rp_gain); \
	  r_left_out += r_signal *  *(volatile float *)(rp_pan_left); \
	  r_right_out += r_signal * *(volatile float *)(rp_pan_right);

/*
 * MACRO: kerWetDryMaster
 * DESCRIPTION: Calculates final wet, dry and master volume levels
 * Assumes outputs have been summed with proper individual gains and input is
 * unaltered.
 *
 */
/* Old version that used separate master level */ 
#define kerWetDryMaster(r_left_dry, r_right_dry, rp_wet, rp_dry, rp_master, \
								r_left_out, r_right_out) \
	  r_left_out  *= *(volatile float *)(rp_wet); \
	  r_right_out *= *(volatile float *)(rp_wet); \
	  r_left_out  += *(volatile float *)(rp_dry) * r_left_dry; \
	  r_right_out += *(volatile float *)(rp_dry) * r_right_dry; \
	  r_left_out  *= *(volatile float *)(rp_master); \
	  r_right_out *= *(volatile float *)(rp_master);
/* New version with master assumed to be part of wet, dry levels */	  
#define kerWetDry(r_left_dry, r_right_dry, rp_wet, rp_dry, \
								r_left_out, r_right_out) \
	  r_left_out  *= *(volatile float *)(rp_wet); \
	  r_right_out *= *(volatile float *)(rp_wet); \
	  r_left_out  += *(volatile float *)(rp_dry) * r_left_dry; \
	  r_right_out += *(volatile float *)(rp_dry) * r_right_dry;

/*
 * MACRO: kerFilteredFdbkDelay
 * DESCRIPTION: Runs a tapped delay line, with a one pole low pass filter in the
 * feedback loop.  "Direct" output path is delayed.
 * This version allows changing delay and feedback gain, but fixed filter freq.
 * Requires rp_start to be pointing to sufficient memory, and r_tmp real location.
 * Note that this routine gives a delay of ip_delay + 1, with a minimum
 * delay of 1.  Available memory space should be ip_delay + 1.
 * Note that statics get put in .bss
 */
#define kerFilteredFdBkDelay(r_in, r_out, rp_start, rp_data, ip_delay, \
									  rp_feedback_gain, r_filt_coef, r_tmp) \
	  r_tmp = (r_out = *rp_data) + r_filt_coef * r_tmp; \
	  *rp_data = r_in + *(volatile float *)rp_feedback_gain * r_tmp; \
	  rp_data++; \
	  if(rp_data >= ((float *)rp_start + *(volatile long *)ip_delay) ) \
		 rp_data = (float *)rp_start; \

/* This version references the filter coeff via a pointer */ 
/*
#define kerFilteredFdBkDelayCoefp(r_in, r_out, rp_start, rp_data, ip_delay, \
									  rp_feedback_gain, rp_filt_coef, r_tmp) \
	  r_out = *rp_data; \
	  r_tmp = r_out + *(volatile float *)rp_filt_coef * r_tmp; \
	  *rp_data = r_in + *(volatile float *)rp_feedback_gain * r_tmp; \
	  rp_data++; \
	  if(rp_data >= ((float *)rp_start + *(volatile long *)ip_delay) ) \
		 rp_data = (float *)rp_start; \
*/		
/* Version that requires endpoint of circular buffer be supplied, not just the length */		 
#define kerFilteredFdBkDelayCoefp(r_in, r_out, rp_start, rp_data, ip_delay, \
									  rp_feedback_gain, rp_filt_coef, r_tmp) \
	  r_out = *rp_data; \
	  r_tmp = r_out + *(volatile float *)rp_filt_coef * r_tmp; \
	  *rp_data = r_in + *(volatile float *)rp_feedback_gain * r_tmp; \
	  rp_data++; \
	  if(rp_data >= (float *)*(volatile long *)ip_delay ) \
		 rp_data = (float *)rp_start; \

/*
 * MACRO: kerFilteredFdbkDelayRev
 * DESCRIPTION: Runs a tapped delay line, with a one pole low pass filter in the
 * feedback loop.  "Direct" output path is delayed.
 * This version allows changing delay and feedback gain, but fixed filter freq.
 * Requires rp_start to be pointing to sufficient memory, and r_tmp real location.
 * Note that this routine gives a delay of ip_delay + 1, with a minimum
 * delay of 1.  Available memory space should be ip_delay + 1.
 * Note that statics get put in .bss   
 * This version for reverbs uses direct delay and feedback.
 */
#define kerFilteredFdBkDelayRev(r_in, r_out, rp_start, rp_data, i_delay, \
									  r_feedback_gain, r_filt_coef, r_tmp) \
	  r_tmp = (r_out = *rp_data) + r_filt_coef * r_tmp; \
	  *rp_data = r_in + r_feedback_gain * r_tmp; \
	  rp_data++; \
	  if(rp_data >= ((float *)rp_start + i_delay) ) \
		 rp_data = (float *)rp_start; \
		 
/*
 * MACRO: kerAllPassAdjustable
 * DESCRIPTION: Runs an allpass filter, adjustable feedback and gain.
 * This is the "canonical" version that doesn't correct forward DC gain.
 * Requires rp_start to be pointing to sufficient memory, and r_tmp real location.
 * Note that this routine gives a delay of ip_delay + 1, with a minimum
 * delay of 1.  Available memory space should be ip_delay + 1.
 */
#define kerAllPassAdjustable(r_in, r_out, rp_start, rp_data, ip_delay, \
									  rp_feedback_gain) \
{ \
	  float r_tmp_MACRO; \
	  r_tmp_MACRO = *(volatile float *)rp_feedback_gain * (r_in - *rp_data); \
	  r_out = *rp_data + r_tmp_MACRO; \
	  *rp_data = r_in + r_tmp_MACRO; \
	  rp_data++; \
	  if(rp_data >= ((float *)rp_start + *(volatile long *)ip_delay) ) \
		 rp_data = (float *)rp_start; \
}
 
/*
 * MACRO: kerAllPassAdjustableRev
 * DESCRIPTION: Runs an allpass filter, adjustable feedback and gain.
 * This is the "canonical" version that doesn't correct forward DC gain.
 * Requires rp_start to be pointing to sufficient memory, and r_tmp real location.
 * Note that this routine gives a delay of ip_delay + 1, with a minimum
 * delay of 1.  Available memory space should be ip_delay + 1.
 * Special version for reverbs that uses direct vals for delay and feedback.
 */
#define kerAllPassAdjustableRev(r_in, r_out, rp_start, rp_data, i_delay, \
									  r_feedback_gain) \
{ \
	  float r_tmp_MACRO; \
	  r_tmp_MACRO = r_feedback_gain * (r_in - *rp_data); \
	  r_out = *rp_data + r_tmp_MACRO; \
	  *rp_data = r_in + r_tmp_MACRO; \
	  rp_data++; \
	  if(rp_data >= ((float *)rp_start + i_delay) ) \
		 rp_data = (float *)rp_start; \
}

/*
 * MACRO: kerDelay2Taps
 * DESCRIPTION: A special 2 tap delay line used in the Lexicon style reverb.
 * The delay of tap 2 is set by the difference between mem_start and mem_end.
 */
#define kerDelay2Taps(in, tap1, tap1_out, tap2, tap2_out, ptr, mem_start, mem_end)\
{ \
	float *tmp_ptr;\
	tap2_out = *(ptr);\
	*(ptr) = in;\
	tmp_ptr = ptr - tap1;\
	if( tmp_ptr < mem_start)\
		tmp_ptr += tap2;\
	tap1_out = *tmp_ptr;\
	(ptr)++;\
	if( ptr >= mem_end )\
		ptr = mem_start;\
}

/*
 * MACRO: kerDelay3Taps
 * DESCRIPTION: A special 3 tap delay line used in the Lexicon style reverb.
 * The delay of tap 3 is set by the difference between mem_start and mem_end.
 */
#define kerDelay3Taps(in, tap1, tap1_out, tap2, tap2_out, tap3, tap3_out, ptr, mem_start, mem_end)\
{ \
	float *tmp_ptr;\
	tap3_out = *(ptr);\
	*(ptr) = in;\
	tmp_ptr = ptr - tap1;\
	if( tmp_ptr < mem_start)\
		tmp_ptr += tap3;\
	tap1_out = *tmp_ptr;\
	tmp_ptr = ptr - tap2;\
	if( tmp_ptr < mem_start)\
		tmp_ptr += tap3;\
	tap2_out = *tmp_ptr;\
	(ptr)++;\
	if( ptr >= mem_end )\
		ptr = mem_start;\
}

/*
 * MACRO: kerDelay4Taps
 * DESCRIPTION: A special 3 tap delay line used in the Lexicon style reverb.
 * The delay of tap 3 is set by the difference between mem_start and mem_end.
 */
#define kerDelay4Taps(in, tap1, tap1_out, tap2, tap2_out, tap3, tap3_out, tap4, tap4_out, ptr, mem_start, mem_end)\
{ \
	float *tmp_ptr;\
	tap4_out = *(ptr);\
	*(ptr) = in;\
	tmp_ptr = ptr - tap1;\
	if( tmp_ptr < mem_start)\
		tmp_ptr += tap4;\
	tap1_out = *tmp_ptr;\
	tmp_ptr = ptr - tap2;\
	if( tmp_ptr < mem_start)\
		tmp_ptr += tap4;\
	tap2_out = *tmp_ptr;\
	tmp_ptr = ptr - tap3;\
	if( tmp_ptr < mem_start)\
		tmp_ptr += tap4;\
	tap3_out = *tmp_ptr;\
	(ptr)++;\
	if( ptr >= mem_end )\
		ptr = mem_start;\
}

