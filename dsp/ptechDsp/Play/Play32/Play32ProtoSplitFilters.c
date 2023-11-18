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
#include "filt.h"

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
#include "c_dly1.h"
#include "dutio.h"	  /* A/D I/O macros */
#include "dutcom.h"	  /* PC to DSP and DSP to PC com routines */
#include "dutinit.h"  /* DSP and AD initialization */
#include "kerdma.h"
#include "kerdelay.h"  /* Has wet-dry macro */
#include "KerNoise.h"

#define NZEROS 8
#define NPOLES 8

static float xv_lp1[NZEROS+1], yv_lp1[NPOLES+1];
static float xv_lp2[NZEROS+1], yv_lp2[NPOLES+1];
static float xv_hp1[NZEROS+1], yv_hp1[NPOLES+1];
static float xv_hp2[NZEROS+1], yv_hp2[NPOLES+1];

/* PTHACK for prototyping. Declare structure here so if file open
 * fails values from last read will be present.
 */
#ifdef DSP_READ_VALS
#include "c_ReadVals.h"
static struct ReadValsType ReadVals;
#endif

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

	if( dspsDly8Init(params, memory, DSPS_SOFT_MEM_DELAY_LENGTH, state, i_init_flag, r_samp_freq) != OKAY)
		return(NOT_OKAY);

	/* After generic initialization, set desired values for fixed parameters */
	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		/* Dly uses older non structure based parameter references */
		float *COMM_MEM_OFFSET = params;

 		/* Need to transfer the stereo mode from the first set to each set */
		((long *)(params))[DSP_PLAY_STEREO_MODE_INDEX] = stereo_mode;

		/* Set input muting value to 1.0 */
		*(volatile float *)(DSP_MUTE_IN_FLAG) = 1.0;

		/* Set feedback values */
		*(volatile float *)(ELEM0_FEEDBACK) = (realtype)PLY_MASTER_FEEDBACK;
		*(volatile float *)(ELEM1_FEEDBACK) = (realtype)PLY_MASTER_FEEDBACK;
		*(volatile float *)(ELEM2_FEEDBACK) = (realtype)PLY_MASTER_FEEDBACK;
		*(volatile float *)(ELEM3_FEEDBACK) = (realtype)PLY_MASTER_FEEDBACK;
		*(volatile float *)(ELEM4_FEEDBACK) = (realtype)PLY_MASTER_FEEDBACK;
		*(volatile float *)(ELEM5_FEEDBACK) = (realtype)PLY_MASTER_FEEDBACK;
		*(volatile float *)(ELEM6_FEEDBACK) = (realtype)PLY_MASTER_FEEDBACK;
		*(volatile float *)(ELEM7_FEEDBACK) = (realtype)PLY_MASTER_FEEDBACK;
	}

	params += 2 * DSPS_MAX_NUM_PARAMS;
	memory += DSPS_SOFT_MEM_DELAY_LENGTH;
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
	float *COMM_MEM_OFFSET = fp_params;
	struct dspPlayStructType *s = (struct dspPlayStructType *)fp_params;

	static float l_del[4096];
	static float r_del[4096];

	static float d_gain = 0.25;
	static int l_delay = 1000;
	static int d_index = 0;

	#ifdef DSP_READ_VALS
	/* PTHACK for prototyping */
	ReadProtoVals(8, &ReadVals);
	#endif
		
	if( !(s->bypass_on) )
	{
#if(PT_DSP_BUILD == PT_DSP_DSPFX)
			/* Below only used in DSP-FX builds */
			long transfer_state = 0; /* For sending out meter values */
			long status = 0;         /* For sending run time status to PC */

			float in_meter1_dma = 0.0;			
			float in_meter2_dma = 0.0;
			float out_meter1_dma = 0.0;
			float out_meter2_dma = 0.0;
#endif

			/* Needed for dutil io macros */
			unsigned data_index = 0;
			long *read_in_buf;
			long *read_out_buf;

			int i;

			read_in_buf = lp_data;
			read_out_buf = lp_data;

			for(i=0; i<l_length; i++)
			{
				float in1, in2;
				float out1, out2;
				float mono;
				volatile long in_count = 0;
				volatile long out_count = 0;

				dutilGetInputsAndMeter( in1, in2, status);

				mono = (in1 + in2) * (realtype)0.5;

				if( ReadVals.switch_vals[0] )
				{	
					/* 200 hz Butterworth, apparently a +3db peak at crossover */
					/*
					#define LPGAIN 5.025847783e+03
					#define HPGAIN 1.020353514e+00
					 */
					#define LPGAIN 3.140749294e+03
					#define HPGAIN 1.019527071e+00

					float *xv, *yv;
					float lp_out, hp_out;

					/*
					d_gain = ReadVals.f_vals[1];
					l_delay = ReadVals.l_vals[0];
					*/

					xv = &(xv_lp1[0]);
					yv = &(yv_lp2[0]);

					xv[0] = xv[1]; xv[1] = xv[2]; 
					xv[2] = mono * (1.0 / LPGAIN);
					yv[0] = yv[1]; yv[1] = yv[2];

						/* 200 hz Butterworth, apparently has a +3 db peak at crossover */
						/* yv[2] =   (xv[0] + xv[2]) + 2 * xv[1]
                     + ( -0.9605029194 * yv[0]) + (  1.9597070338 * yv[1]); */

					/* 200 hz Bessel - still has some coloration */
					yv[2] =   (xv[0] + xv[2]) + 2 * xv[1]
                     + ( -0.9391454766 * yv[0]) + (  1.9378718952 * yv[1]);
        
					lp_out = yv[2];

					xv = &(xv_hp1[0]);
					yv = &(yv_hp2[0]);

					xv[0] = xv[1]; xv[1] = xv[2]; 
					xv[2] = mono * (1.0 / HPGAIN);
					yv[0] = yv[1]; yv[1] = yv[2];
			 
					/* 200 hz Butterworth, apparently has a +3 db peak at crossover */
					/* yv[2] =   (xv[0] + xv[2]) - 2 * xv[1]
                     + ( -0.9605029194 * yv[0]) + (  1.9597070338 * yv[1]); */

					/* 200 hz Bessel */
					yv[2] =   (xv[0] + xv[2]) - 2 * xv[1]
                     + ( -0.9619400067 * yv[0]) + (  1.9614477236 * yv[1]);        

					hp_out = yv[2];

					out1 = lp_out - hp_out;
					out2 = out1;

				}
				else
				{
					out1 = mono;
					out2 = mono;
				}

				dutilPutOutputsAndMeter(out1, out2, status);
			}

		params += 2 * DSPS_MAX_NUM_PARAMS;
		memory += DSPS_SOFT_MEM_AURAL_ENHANCER_LENGTH;
		state  += DSPS_NUM_STATE_VARS;

		params += 2 * DSPS_MAX_NUM_PARAMS;
		memory += DSPS_SOFT_MEM_LEX_LENGTH;
		state  += DSPS_NUM_STATE_VARS;

		params += 2 * DSPS_MAX_NUM_PARAMS;
		memory += DSPS_SOFT_MEM_WIDE_LENGTH;
		state  += DSPS_NUM_STATE_VARS;

		params += 2 * DSPS_MAX_NUM_PARAMS;
		memory += DSPS_SOFT_MEM_DELAY_LENGTH;
		state  += DSPS_NUM_STATE_VARS;

		/* Note that optimizer is never bypassed, the output gain is set to
		 * unity when the process switch on the UI is not selected.
		 */
		/* */
		#if defined(DSPSOFT_32_BIT)
		dspsMaximizerProcess32(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
		#else
		dspsMaximizerProcess(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
		#endif
		/* */
	}
}


#endif /* DSPSOFT_32_BIT */