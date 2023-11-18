/* THIS VERSION DOES THE ON/OFF DEMO MODE */
/* (C) COPYRIGHT 1994-1999 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */

/*
 * FILE: Play32.c
 * DATE: 4/24/99
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: MP3 Player plug-in
 *
 */

#ifndef DSPSOFT_TARGET
#define DSP_TARGET
#endif
#include "Play_num.h"  /* Sets number of elements */

/* Standard includes */
#include <math.h>
#include <stdio.h>
#include "codedefs.h" /* Company common defines */
#include "platform.h" /* Targets a specific DSP card       */
#include "pt_defs.h"  /* Defines common to both PC and DSP */	
#include "product_type.h" /* Sets DSPFX or DFX */
#include "boardrv1.h" /* DSP hardware specific defines, for both PC and DSP */

/* For setting demo bypass mode */
#include "demodef.h"

/* #define DSP_PROGRAM_SIZE 0x400 */ /* Overide default program size */
#ifdef DSP_TARGET
#include "dma32.h"
#endif

#include "dsp_mem1.h" /* For DSP memory configuration (typical delay version) */
#include "c_dsps.h"
#include "c_Play.h"   /* For specific parameter mappings */
#include "c_aural.h"
#include "c_lex.h"
#include "c_max.h"
#include "c_Wid.h"
#include "dutio.h"	  /* A/D I/O macros */
#include "dutcom.h"	  /* PC to DSP and DSP to PC com routines */
#include "dutinit.h"  /* DSP and AD initialization */
#include "kerdma.h"
#include "kerdelay.h"  /* Has wet-dry macro */
#include "KerNoise.h"

static void DSPS_PLAY_RUN(void);

#ifdef DSP_TARGET
void main(int argc, char *argv[])
{
	DSPS_PLAY_INIT();
	while(1)
		DSPS_PLAY_RUN();
}
#endif

/* Only build init function in 32 bit files (same for both) */
#if defined(DSPSOFT_32_BIT)
DSP_FUNC_DEF int DSPS_PLAY_INIT(float *fp_params, float *fp_memory, long l_memsize, float *fp_state, int i_init_flag, float r_samp_freq)
{
	float *params = fp_params;
	float *memory = fp_memory;
	float *state  = fp_state;
	long stereo_mode;

	/* Initialize play specific parameters. These share the parameter space with
	 * the activator params, and are located above the last activator param.
	 */
	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		struct dspPlayStructType *s = (struct dspPlayStructType *)params;

		/* Save stereo mode for transfer to other parameter sets. */
		stereo_mode = ((long *)(fp_params))[DSP_PLAY_STEREO_MODE_INDEX];

		s->bypass_on = 0L;
		s->activator_on = 1L;
		s->ambience_on = 1L;
		s->widener_on = 1L;
		s->reset_demo_count = 0L;

		/* Initialize internal states */
		s->bypass_mode = 0;
		s->sample_count = 0;
		s->max_sample_count_process = (unsigned long)(r_samp_freq * (realtype)DSP_PLAY_PROCESS_TIME);
		s->max_sample_count_demo = (unsigned long)(r_samp_freq * (realtype)DSP_PLAY_DEMO_TIME);
	}

	if( dspsAuralInit(params, memory, l_memsize, state, i_init_flag, r_samp_freq) != OKAY)
		return(NOT_OKAY);

	/* After generic initialization, set desired values for fixed parameters */
	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		struct dspAuralStructType *s = (struct dspAuralStructType *)params;

		/* Need to transfer the stereo mode from the first set to each set */
		((long *)(params))[DSP_PLAY_STEREO_MODE_INDEX] = stereo_mode;

		/* Initialization values from Quick preset 1, 44khz */
		s->dry_gain = (realtype)0.622047;	  
		s->wet_gain = (realtype)0.377953;
		s->aural_drive = (realtype)1.76993;
		s->aural_odd = (realtype)1.5;
		s->aural_even = (realtype)0.0;
		/* Next 3 are samp_freq dependent, need to be reset for other than 44.1kHz */
		s->gain = (realtype)0.788950;
		s->a1 = (realtype)1.53285;
		s->a0 = (realtype)-0.622949;
	}

	params += 2 * DSPS_MAX_NUM_PARAMS;
	memory += DSPS_SOFT_MEM_AURAL_ENHANCER_LENGTH;
	state  += DSPS_NUM_STATE_VARS;

	if( dspsLexReverbInit(params, memory, l_memsize, state, i_init_flag, r_samp_freq) != OKAY)
		return(NOT_OKAY);

	/* After generic initialization, set desired values for fixed parameters */
	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		struct dspLexStructType *s = (struct dspLexStructType *)params;

		/* Need to transfer the stereo mode from the first set to each set */
		((long *)(params))[DSP_PLAY_STEREO_MODE_INDEX] = stereo_mode;

		/* Initial values from quick pick one but with dry/wet of 0.21, 44.1kHz.
		 * Wet-Dry are boosted for better bypass balance
		 */
		s->wet_gain = (realtype)(0.21 * 1.3);	  
		s->dry_gain = (realtype)(0.69 * 1.3);
		s->dsp_mute_in_flag = (realtype)1.0;

		/* Next values are sampling frequency dependent, need to be recalculated
		 * for other than 44.1kHz.
		 */
		s->lat1_coeff = (realtype)0.75;
		s->lat3_coeff = (realtype)0.625;
		s->lat5_coeff = (realtype)0.70;
		s->lat6_coeff = (realtype)0.5;
		s->damping = (realtype)0.408290;
		s->one_minus_damping = (realtype)0.591710;
		s->bandwidth = (realtype)0.350110;
		s->one_minus_bandwidth = (realtype)0.649890;
		s->roomsize = (realtype)1.0;
		s->modulation_freq = (realtype)0.110871;
		s->modulation_depth = (realtype)27.7795;
		s->decay = (realtype)0.565664;
		s->pre_delay = 1;
	}

	params += 2 * DSPS_MAX_NUM_PARAMS;
	memory += DSPS_SOFT_MEM_LEX_LENGTH;
	state  += DSPS_NUM_STATE_VARS;

	if( dspsWideInit(params, memory, l_memsize, state, i_init_flag, r_samp_freq) != OKAY)
		return(NOT_OKAY);

	/* After generic initialization, set desired values for fixed parameters */
	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		struct dspWideStructType *s = (struct dspWideStructType *)params;
 
		/* Need to transfer the stereo mode from the first set to each set */
		((long *)(params))[DSP_PLAY_STEREO_MODE_INDEX] = stereo_mode;

		/* Starting Presets - at 44.1 hHz.
		 * Intensity - 35
		 * Stereo Width - 100
		 * Dispersion - 5.0
		 * Center Level - 100
		 * Center Depth - 0
		 * Freq. Threshold - 100
		 */
		s->bypass_flag = 0L;
		s->master_gain = (realtype)1.0;
		s->intensity = (realtype)0.354331;
		s->width = (realtype)1.0;
		s->reverse_width = (realtype)0.0;
		s->center_gain = (realtype)1.0;
		s->center_depth = 1L;
		s->dispersion_l = 169L; /* Left-right factor is 0.793651 */
		s->dispersion_r = 218L;
		s->gain = (realtype)0.989976;
		s->a1 = (realtype)1.97985;
		s->a0 = (realtype)-0.980053;
 	}

	params += 2 * DSPS_MAX_NUM_PARAMS;
	memory += DSPS_SOFT_MEM_WIDE_LENGTH;
	state  += DSPS_NUM_STATE_VARS;

	if( dspsMaximizerInit(params, memory, l_memsize, state, i_init_flag, r_samp_freq) != OKAY)
		return(NOT_OKAY);

	/* After generic initialization, set desired values for fixed parameters */
	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		struct dspMaxiStructType *s = (struct dspMaxiStructType *)params;
 
		/* Need to transfer the stereo mode from the first set to each set */
		((long *)(params))[DSP_PLAY_STEREO_MODE_INDEX] = stereo_mode;

		/* Initializations from quick pick 1, 44.1khz */
		s->wet_gain = (realtype)1.0;	  
		s->dry_gain = (realtype)0.0;

		s->gain_boost = (realtype)1.99526;
		s->max_output = (realtype)0.966051;
		s->max_delay = 33;
		s->release_time_beta = (realtype)0.997776;
		s->num_quant_bits = KERNOISE_QUANTIZE_16;   /* Hardcoded for 16 bit quantization */
		s->dither_type = KERNOISE_DITHER_SHAPED; /* Noise shaped dither type */
	}

	return(OKAY);
}
#endif /* DSPSOFT_32_BIT */

#ifdef DSPSOFT_TARGET
DSP_FUNC_DEF void DSPS_PLAY_PROCESS(long *lp_data, int l_length,
								   float *fp_params, float *fp_memory, float *fp_state,
								   struct hardwareMeterValType *sp_meters, int DSP_data_type)
{
	float *params = fp_params;
	float *memory = fp_memory;
	float *state  = fp_state;
	struct hardwareMeterValType dummy_meters;

	struct dspPlayStructType *s = (struct dspPlayStructType *)params;

	/* If in demo mode, implement the bypassing. Note that we are using the dummy_meters
	 * var in the processing calls so the aux_vals doesn't get overwritten
	 */
	#if (defined(DSPFX_DEMO_VERSION) & (PT_PRODUCT_TYPE == PT_PRODUCT_DFX))
		s->sample_count += l_length;

		if( s->bypass_mode )
		{
			/* Check to see if its time to come out of bypassed mode, or if a knob has moved. */
			if( (s->sample_count > s->max_sample_count_demo) || (s->reset_demo_count != IS_FALSE) )
			{
				s->bypass_mode = 0;
				s->sample_count = 0;
				sp_meters->values_are_new = IS_TRUE;
				/* Copy long bypass mode value into aux_vals array */
				*(long *)(&(sp_meters->aux_vals[0])) = IS_FALSE;
				s->reset_demo_count = 0L;
			}
		}
		else
		{
			if( s->reset_demo_count != IS_FALSE )
			{
				s->sample_count = 0;
				s->reset_demo_count = 0;
			}
			else if( s->sample_count > s->max_sample_count_process )
			{
				s->bypass_mode = 1;
				s->sample_count = 0;
				sp_meters->values_are_new = IS_TRUE;
				/* Copy long bypass mode value into aux_vals array */
				*(long *)(&(sp_meters->aux_vals[0])) = IS_TRUE;
			}
		}
	#endif

	#ifdef DSPFX_DEMO_VERSION
	if( !(s->bypass_on) && !(s->bypass_mode) )
	#else
	if( !(s->bypass_on) )
	#endif
	{
		if( s->activator_on )
		{

			#if defined(DSPSOFT_32_BIT)
			dspsAuralProcess32(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
			#else
			dspsAuralProcess(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
			#endif
		}

		params += 2 * DSPS_MAX_NUM_PARAMS;
		memory += DSPS_SOFT_MEM_AURAL_ENHANCER_LENGTH;
		state  += DSPS_NUM_STATE_VARS;

		if( s->ambience_on )
		{
			#if defined(DSPSOFT_32_BIT)
			dspsLexReverbProcess32(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
			#else
			dspsLexReverbProcess(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
			#endif
		}

		params += 2 * DSPS_MAX_NUM_PARAMS;
		memory += DSPS_SOFT_MEM_LEX_LENGTH;
		state  += DSPS_NUM_STATE_VARS;

		if( s->widener_on )
		{
			#if defined(DSPSOFT_32_BIT)
			dspsWideProcess32(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
			#else
			dspsWideProcess(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
			#endif
		}

		params += 2 * DSPS_MAX_NUM_PARAMS;
		memory += DSPS_SOFT_MEM_WIDE_LENGTH;
		state  += DSPS_NUM_STATE_VARS;

		/* Note that optimizer is never bypassed, the output gain is set to
		 * unity when the process switch on the UI is not selected.
		 */
		#if defined(DSPSOFT_32_BIT)
		dspsMaximizerProcess32(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
		#else
		dspsMaximizerProcess(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
		#endif
	}
} 
#endif
