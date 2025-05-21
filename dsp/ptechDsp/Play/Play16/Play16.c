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

/*
 * FILE: Play32.c
 * DATE: 4/24/99
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: MP3 Player plug-in
 * MODIFICATION NOTE - modified on 7/28/09 to only set the out1 (left) output when
 * a the input signal is mono. Previously, to support the general effects plug-in case
 * both out1 and out2 were being set to the mono signal, now out2 will be set to 0.0
 *
 */

#ifndef DSPSOFT_TARGET
#define DSP_TARGET
#endif
#include "play_num.h"  /* Sets number of elements */

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
#include "c_play.h"   /* For specific parameter mappings */
#include "c_aural.h"
#include "c_lex.h"
#include "c_max.h"
#include "c_wid.h"
#include "c_dly1.h"
#include "dutio.h"	  /* A/D I/O macros */
#include "dutcom.h"	  /* PC to DSP and DSP to PC com routines */
#include "dutinit.h"  /* DSP and AD initialization */
#include "kerdma.h"
#include "kerdelay.h"  /* Has wet-dry macro */
#include "kernoise.h"

#define NZEROS 8
#define NPOLES 8

static float xv_bs1[NZEROS+1], yv_bs1[NPOLES+1];
static float xv_bs2[NZEROS+1], yv_bs2[NPOLES+1];
static float xv_bp1[NZEROS+1], yv_bp1[NPOLES+1];
static float xv_bp2[NZEROS+1], yv_bp2[NPOLES+1];

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

		/* Note that the head_delay variable is initialized to twice the desired delay
		 * in samples due to the stereo delay. Note that with a delay of 0.227 mS at 44.1kHz
		 * there is a delay of 10 samples in each channel.
		 */
		s->head_delay = 2 * (long)( (float)(0.227e-3) * r_samp_freq );

		/* Next 3 are samp_freq dependent, need to be reset for other than 44.1kHz */
		s->b0 = (realtype)1.0;
		s->b1 = (realtype)0.0;
		s->b2 = (realtype)0.0;
		s->a1 = (realtype)0.0;
		s->a2 = (realtype)0.0;

		/* Set lp filter coeffs */
		{
			realtype r_omega = (realtype)(6.283185 * 150.0/44100.0);

			/* filtDesign2ndButLowPass(r_omega, &(s->b0_lp), &(s->a1_lp), &(s->a2_lp));
			   Not linking for some reason. */
			/*
			{   
				realtype omega2 = r_omega * r_omega;
				realtype twoRoot2omega = (realtype)(2.0 * sqrt(2.0)) * r_omega;
				realtype tmp = (realtype)1.0/((realtype)4.0 + omega2 + twoRoot2omega);

				s->b0_lp = omega2 * tmp;
				s->a1_lp = ((realtype)8.0 - (realtype)2.0 * omega2) * tmp;
				s->a2_lp = (twoRoot2omega - (realtype)4.0 - omega2) * tmp;
			}
			*/

			/* Hard set to 4th order lowpass, 150 hz sampling freq */
			/*
			s->a1_lp = 3.9441544121;
			s->a2_lp = -5.8340173191;
			s->a3_lp = 3.8355461887;
			s->a4_lp = -0.9456834846;
			 */

			/* Hard set to 4th order lowpass, 250 hz sampling freq */
			/*
			s->a1_lp = 3.9069255472;
			s->a2_lp = -5.7250836195;
			s->a3_lp = 3.7292747700;
			s->a4_lp = -0.9111182347;
			 */

			/* Hard set to 4th order lowpass, 350 hz sampling freq */
			/* SO FAR, LOOKS BEST FOR BOTTOM END CUTTOFF */
			s->a1_lp = (float)3.8696989703 ;
			s->a2_lp = (float)-5.6175190223 ;
			s->a3_lp = (float)3.6256257574 ;
			s->a4_lp = (float)-0.8778115036 ;

			/* Hard set to 4th order lowpass, 5000 hz sampling freq */
			s->a1_lp = (float)2.1544844308  ;
			s->a2_lp = (float)-1.9894244796  ;
			s->a3_lp = (float)0.8650973862  ;
			s->a4_lp = (float)-0.1481421788  ;

			/* Hard set to 4th order lowpass, 4000 hz sampling freq */
			s->a1_lp = (float)2.5194645027   ;
			s->a2_lp = (float)-2.5608371103   ;
			s->a3_lp = (float)1.2062353665   ;
			s->a4_lp = (float)-0.2201292677   ;

			/* Hard set to 4th order lowpass, Chebyshev, 2db ripple, 4000 hz sampling freq */
			s->a1_lp = (float)2.1626046286    ;
			s->a2_lp = (float)-2.1682529894    ;
			s->a3_lp = (float)1.0744968552    ;
			s->a4_lp = (float)-0.3090930061    ;
		}

	} /* End of parameter initialization */

	/* Zero internal signal memory spaces to eliminates glitches on restarts */
	if( (i_init_flag & DSPS_INIT_PARAMS) || (i_init_flag & DSPS_ZERO_MEMORY) )
	{
		struct dspPlayStructType *s = (struct dspPlayStructType *)params;

		/* Zero delay line memory */
		{
			unsigned int i;
			realtype *rp;
			
			rp = &(s->delay_lines);

			for(i=0; i < s->head_delay; i++)
			{
				*rp = (float)0.0;
				rp++;
			}
		}
		s->delay_line_index = 0;

		/* Zero internal filter states */
		s->in1_w1 = 0.0;
		s->in1_w2 = 0.0;
		s->in2_w1 = 0.0;
		s->in2_w2 = 0.0;

		s->in1_w1_lp = 0.0;
		s->in1_w2_lp = 0.0;
		s->in1_w3_lp = 0.0;
		s->in1_w4_lp = 0.0;
		s->in2_w1_lp = 0.0;
		s->in2_w2_lp = 0.0;
		s->in2_w3_lp = 0.0;
		s->in2_w4_lp = 0.0;

		s->out1_w1_lp = 0.0;
		s->out1_w2_lp = 0.0;
		s->out1_w3_lp = 0.0;
		s->out1_w4_lp = 0.0;
		s->out2_w1_lp = 0.0;
		s->out2_w2_lp = 0.0;
		s->out2_w3_lp = 0.0;
		s->out2_w4_lp = 0.0;
		s->out1_w1_hp = 0.0;
		s->out1_w2_hp = 0.0;
		s->out1_w3_hp = 0.0;
		s->out1_w4_hp = 0.0;
		s->out2_w1_hp = 0.0;
		s->out2_w2_hp = 0.0;
		s->out2_w3_hp = 0.0;
		s->out2_w4_hp = 0.0;
	} /* End of signal memory zeroing */

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

	/* Note DMX builds to not use memory space, but need to increment state and params */
	params += 2 * DSPS_MAX_NUM_PARAMS;
	memory += DSPS_SOFT_MEM_AURAL_ENHANCER_LENGTH;
	state  += DSPS_NUM_STATE_VARS;

#if !defined( PT_DMX_BUILD )
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
#endif /* !defined PT_DMX_BUILD */

	params += 2 * DSPS_MAX_NUM_PARAMS;
	memory += DSPS_SOFT_MEM_LEX_LENGTH;
	state  += DSPS_NUM_STATE_VARS;

#if !defined( PT_DMX_BUILD )
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
#endif /* !defined PT_DMX_BUILD */

	params += 2 * DSPS_MAX_NUM_PARAMS;
	memory += DSPS_SOFT_MEM_WIDE_LENGTH;
	state  += DSPS_NUM_STATE_VARS;

#if !defined( PT_DMX_BUILD )
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
#endif /* !defined PT_DMX_BUILD */

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

	#ifdef DSP_READ_VALS
	/* PTHACK for prototyping */
	ReadProtoVals(8, &ReadVals);
	#endif
		
	if( !(s->bypass_on) )
	{
#if !defined( PT_DMX_BUILD )
		if( (s->vocal_elim_on) && (s->vocal_elim_val != 0) )
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

			float in1_bs, in1_bp, in2_bs, in2_bp;
			float diff, diff_gain;

			/* Declare bandpass and bandstop function pointers */
			void (*bs_ptr)(realtype *xv, realtype *yv, realtype in, realtype *in_bs);
			void (*bp_ptr)(realtype *xv, realtype *yv, realtype in, realtype *in_bs);

			int i;

			read_in_buf = lp_data;
			read_out_buf = lp_data;

			if( (s->vocal_elim_val != s->last_vocal_val)
				 || (s->vocal_mode != s->last_mode) )
			{
				s->last_vocal_val = s->vocal_elim_val;
				s->last_mode = s->vocal_mode;
				for(i=0; i < NPOLES + 1; i++)
				{
					xv_bs1[i] = (realtype)0.0;
					yv_bs1[i] = (realtype)0.0;
					xv_bs2[i] = (realtype)0.0;
					yv_bs2[i] = (realtype)0.0;
					xv_bp1[i] = (realtype)0.0;
					yv_bp1[i] = (realtype)0.0;
					xv_bp2[i] = (realtype)0.0;
					yv_bp2[i] = (realtype)0.0;
				}
			}

			switch( s->vocal_elim_val )
			{
			case 0:
				diff_gain = (float)1.0;
				bs_ptr = play32_but_bs_350_4000;
				bp_ptr = play32_but_bp_350_4000;
				break;
			case 1:
				diff_gain = (float)1.0;
				if( s->vocal_mode ) /* Female */
				{
					bs_ptr = play32_but_bs_416_4757;
					bp_ptr = play32_but_bp_416_4757;
				}
				else  /* Male */
				{
					bs_ptr = play32_but_bs_350_4000;
					bp_ptr = play32_but_bp_350_4000;
				}
				break;
			case 2:
				diff_gain = (float)0.63;
				if( s->vocal_mode ) /* Female */
				{
					bs_ptr = play32_but_bs_416_4757;
					bp_ptr = play32_but_bp_416_4757;
				}
				else  /* Male */
				{
					bs_ptr = play32_but_bs_350_4000;
					bp_ptr = play32_but_bp_350_4000;
				}
				break;
			case 3:
				diff_gain = (float)0.4;
				if( s->vocal_mode ) /* Female */
				{
					bs_ptr = play32_but_bs_416_4757;
					bp_ptr = play32_but_bp_416_4757;
				}
				else  /* Male */
				{
					bs_ptr = play32_but_bs_350_4000;
					bp_ptr = play32_but_bp_350_4000;
				}
				break;
			case 4:
				diff_gain = (float)0.2;
				if( s->vocal_mode ) /* Female */
				{
					bs_ptr = play32_but_bs_416_4757;
					bp_ptr = play32_but_bp_416_4757;
				}
				else  /* Male */
				{
					bs_ptr = play32_but_bs_350_4000;
					bp_ptr = play32_but_bp_350_4000;
				}
				break;
			case 5:
				diff_gain = (float)0.1;
				if( s->vocal_mode ) /* Female */
				{
					bs_ptr = play32_but_bs_416_4757;
					bp_ptr = play32_but_bp_416_4757;
				}
				else  /* Male */
				{
					bs_ptr = play32_but_bs_350_4000;
					bp_ptr = play32_but_bp_350_4000;
				}
				break;
			case 6:
				diff_gain = (float)0.0;
				if( s->vocal_mode ) /* Female */
					bs_ptr = play32_but_bs_416_4757;
				else  /* Male */
					bs_ptr = play32_but_bs_350_4000;
				bp_ptr = play32_no_filt;
				break;
			case 7:
				diff_gain = (float)0.0;
				if( s->vocal_mode ) /* Female */
					bs_ptr = play32_cheb_2_bs_416_4757;
				else
					bs_ptr = play32_cheb_2_bs_350_4000;
				bp_ptr = play32_no_filt;
				break;
			case 8:
				diff_gain = (float)0.0;
				if( s->vocal_mode ) /* Female */
					bs_ptr = play32_but_bs_400_5030;
				else
					bs_ptr = play32_but_bs_283_5030;
				bp_ptr = play32_no_filt;
				break;
			case 9:
				diff_gain = (float)0.0;
				if( s->vocal_mode ) /* Female */
					bs_ptr = play32_but_bs_324_6325;
				else
					bs_ptr = play32_but_bs_229_6325;
				bp_ptr = play32_no_filt;
				break;
			case 10:
				diff_gain = (float)0.0;
				if( s->vocal_mode ) /* Female */
					bs_ptr = play32_but_bs_262_7953;
				else
					bs_ptr = play32_but_bs_185_7953;
				bp_ptr = play32_no_filt;
				break;
			}

			for(i=0; i<l_length; i++)
			{
				float in1, in2;
				float out1, out2;
				volatile long in_count = 0;
				volatile long out_count = 0;

				dutilGetInputsAndMeter( in1, in2, status);

				/* Note - correct digital butterworth highpass works better than
				 * simply trying to subtract the lowpass from the unfiltered.
				 * Note - band stop, 4th order Butterworth, 350 to 4000 works
				   pretty well.
				 */

				bs_ptr(xv_bs1, yv_bs1, in1, &in1_bs);
				bs_ptr(xv_bs2, yv_bs2, in2, &in2_bs);

				bp_ptr(xv_bp1, yv_bp1, in1, &in1_bp);
				bp_ptr(xv_bp2, yv_bp2, in2, &in2_bp);

				diff = (in1_bp - in2_bp) * diff_gain;

				out1 = in1_bs + diff;
				out2 = in2_bs + diff;

				/*
				out1 = in1_bs;
				out2 = in2_bs;
				*/

				dutilPutOutputsAndMeter(out1, out2, status);
			}
		}
#endif /* !defined PT_DMX_BUILD */

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

		#ifdef DSP_READ_VALS
		/* PTHACK for prototyping */
		if( ReadVals.switch_vals[0] )
		#endif

#if !defined( PT_DMX_BUILD )
		if( s->ambience_on )
		/* Turn off Lex ambience if in headphone mode (NOT CURRENTLY) */
		/* if( s->ambience_on && !(s->headphone_on ) ) */

		{
			#if defined(DSPSOFT_32_BIT)
			dspsLexReverbProcess32(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
			#else
			dspsLexReverbProcess(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
			#endif
		}
#endif /* !defined PT_DMX_BUILD */

		/* Note DMX builds to not use memory space, but need to increment state and params */
		params += 2 * DSPS_MAX_NUM_PARAMS;
		memory += DSPS_SOFT_MEM_LEX_LENGTH;
		state  += DSPS_NUM_STATE_VARS;

#if !defined( PT_DMX_BUILD )
		if( s->widener_on )
		{
			#if defined(DSPSOFT_32_BIT)
			dspsWideProcess32(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
			#else
			dspsWideProcess(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
			#endif
		}
#endif /* !defined PT_DMX_BUILD */

		params += 2 * DSPS_MAX_NUM_PARAMS;
		memory += DSPS_SOFT_MEM_WIDE_LENGTH;
		state  += DSPS_NUM_STATE_VARS;

		/* Now do bass boost locally in the play function. Note that
		 * we will use the original unshifted param, memory and state pointers.
		 */
		if( s->bassboost_on )
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
				volatile long in_count = 0;
				volatile long out_count = 0;

				dutilGetInputsAndMeter( in1, in2, status);

				/* Adapted from macro kerSosFiltDirectForm2TransPara */
				/* This special purpose Transformed Direct2 version allows non-unity b0, but
				 * assumes b1=a1, as in parametric boost/cut filters.
				 * coeffs must be ordered b0, b1, b2, a2.
				 */
				out1 = s->in1_w1 + s->b0 * (in1 + (realtype)1.0e-30);
				s->in1_w1 = (in1 - out1) * s->b1 + s->in1_w2;
				s->in1_w2 = s->b2 * in1;
				s->in1_w2 -= s->a2 * out1;				

				/* Do right channel if stereo in */
				if(s->stereo_in_flag)
				{
					out2 = s->in2_w1 + s->b0 * (in2 + (realtype)1.0e-30);
					s->in2_w1 = (in2 - out2) * s->b1 + s->in2_w2;
					s->in2_w2 = s->b2 * in2;
					s->in2_w2 -= s->a2 * out2;				
				}
				else
				{
					/* Note that assigning out2 to out1 in the mono case doubles output level in bass boost.
					 * This works well with DFX and mono signals, but causes inconsistency with stereo case.
					 */
					/* out2 = out1; */
					 /* MODIFICATION NOTE - on 7/28/09, code was modified so with mono signals out2 is set to 0.0
					  * instead of being set to out1
					  */
					out2 = 0.0;
				}

				dutilPutOutputsAndMeter(out1, out2, status);
			}
		}

#if !defined( PT_DMX_BUILD )
		/* Do headphone processing locally in play function */
		if( s->headphone_on )
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

			realtype *delay_p = &(s->delay_lines);


			read_in_buf = lp_data;
			read_out_buf = lp_data;

#ifdef PLY_DO_CROSS_FEED
			if(s->stereo_in_flag)
			{
				int i;
				#ifdef DSP_READ_VALS
				/* PTHACK for prototyping */
				if( ReadVals.switch_vals[1] )
				#endif

				for(i=0; i<l_length; i++)
				{
					float in1, in2;
					float out1, out2;
					float left_delayed, right_delayed;
					float cross_gain = (float)PLY_HEADPHONE_CROSSGAIN;
					volatile long in_count = 0;
					volatile long out_count = 0;

					dutilGetInputsAndMeter( in1, in2, status);

					{
						realtype *rp;

						rp = ( delay_p + s->delay_line_index );
						left_delayed = *rp;
						*rp = in1;
						s->delay_line_index++;
						rp++;

						right_delayed = *rp;
						*rp = in2;
						s->delay_line_index++;
					}

					/* Note that the head_delay variable is initialized to twice the desired delay
					 * in samples due to the stereo delay.
					 */
					#ifdef DSP_READ_VALS_X
					/* PTHACK for prototyping */
					s->head_delay = ReadVals.l_vals[0];
					#endif

					if( s->delay_line_index >= s->head_delay )
						s->delay_line_index = 0;

					#ifdef DSP_READ_VALS_X
					/* PTHACK for prototyping */
					if( ReadVals.switch_vals[1] )
						cross_gain = ReadVals.f_vals[1];
					else
						cross_gain = (float)0.707;
					#endif

					out1 = in1 + right_delayed * cross_gain;
					out2 = in2 + left_delayed * cross_gain;

					dutilPutOutputsAndMeter(out1, out2, status);
				}
			} /* End of cross feeding part of headphone processing */
#endif /* #ifdef PLY_DO_CROSS_FEED */

			/* Add headphone ambience */
			#ifdef DSP_READ_VALS
			/* PTHACK for prototyping */
			if( ReadVals.switch_vals[2] )
			#endif
			if( s->headphone_on )
			{
				#if defined(DSPSOFT_32_BIT)
				dspsDly8Process32(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
				#else
				dspsDly8Process(lp_data, l_length, params, memory, state, &dummy_meters, DSP_data_type);
				#endif
			}
		}
#endif /* !defined PT_DMX_BUILD */

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

#if !defined( PT_DMX_BUILD )
#if defined(DSPSOFT_32_BIT)
/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 350 and 4000 hz.
 */
void play32_but_bs_350_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ 
	xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
	xv[8] = in * (float)(1.0/1.991064787e+00);
	yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
	yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8839668614 * (xv[1] + xv[7]) +  27.3088500520 * (xv[2] + xv[6])
					-  54.2796008160 * (xv[3] + xv[5]) +  67.7094359580 * xv[4]
					+ ( -0.2522551910 * yv[0]) + (  2.3248226511 * yv[1])
					+ ( -9.4589799069 * yv[2]) + ( 22.1912299590 * yv[3])
					+ (-32.8360255980 * yv[4]) + ( 31.3750993500 * yv[5])
					+ (-18.8952757960 * yv[6]) + (  6.5513841772 * yv[7]));
	*in_bs = yv[8];
}

/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 416 and 4000 hz.
 */
void play32_but_bs_416_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(1.0/1.965771570e+00);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8622629891 * (xv[1] + xv[7]) +  27.1806922410 * (xv[2] + xv[6])
               -  53.9622387450 * (xv[3] + xv[5]) +  67.2876203930 * xv[4]
               + ( -0.2587874782 * yv[0]) + (  2.3723101917 * yv[1])
               + ( -9.6065481757 * yv[2]) + ( 22.4443568230 * yv[3])
               + (-33.0928401140 * yv[4]) + ( 31.5271697300 * yv[5])
               + (-18.9428281440 * yv[6]) + (  6.5571664529 * yv[7]));
  *in_bs = yv[8];
}

/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 416 and 4757 hz.
 */
void play32_but_bs_416_4757(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(1.0/2.279029503e+00);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8345499343 * (xv[1] + xv[7]) +  27.0175647530 * (xv[2] + xv[6])
               -  53.5590265400 * (xv[3] + xv[5]) +  66.7520263700 * xv[4]
               + ( -0.1925516538 * yv[0]) + (  1.8109349941 * yv[1])
               + ( -7.5493194974 * yv[2]) + ( 18.2164183760 * yv[3])
               + (-27.8379755140 * yv[4]) + ( 27.5871261380 * yv[5])
               + (-17.2971021450 * yv[6]) + (  6.2624680178 * yv[7]));
  *in_bs = yv[8];
}  

/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 495 and 4000 hz.
 */
void play32_but_bs_495_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(1.0/1.935966116e+00);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8363539694 * (xv[1] + xv[7]) +  27.0281663250 * (xv[2] + xv[6])
               -  53.5852056730 * (xv[3] + xv[5]) +  66.7867894370 * xv[4]
               + ( -0.2668162421 * yv[0]) + (  2.4302799692 * yv[1])
               + ( -9.7856172436 * yv[2]) + ( 22.7498779640 * yv[3])
               + (-33.4013132940 * yv[4]) + ( 31.7089902870 * yv[5])
               + (-18.9993896500 * yv[6]) + (  6.5639867631 * yv[7]));
	*in_bs = yv[8];
}

/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 495 and 5656 hz.
 */
void play32_but_bs_495_5656(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(1.0/2.684361630e+00);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.7629769950 * (xv[1] + xv[7]) +  26.5989294340 * (xv[2] + xv[6])
               -  52.5280925360 * (xv[3] + xv[5]) +  65.3842925230 * xv[4]
               + ( -0.1388443764 * yv[0]) + (  1.3333341746 * yv[1])
               + ( -5.7113736171 * yv[2]) + ( 14.2357794520 * yv[3])
               + (-22.6078560300 * yv[4]) + ( 23.4357628700 * yv[5])
               + (-15.4621595390 * yv[6]) + (  5.9153524729 * yv[7]));
	*in_bs = yv[8];
}

/* 4th order Butterworth bandpass filter, 44100 sampling freq.,
 * corners at 350 and 4000 hz.
 */
void play32_but_bp_350_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bp)
{
	xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
	xv[8] = in * (float)(1.0/3.973026762e+02);
	yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
	yv[8] =   (xv[0] + xv[8]) - (realtype)(4 * (xv[2] + xv[6]) + 6 * xv[4]
					+ ( -0.2522551910 * yv[0]) + (  2.3248226511 * yv[1])
					+ ( -9.4589799069 * yv[2]) + ( 22.1912299590 * yv[3])
					+ (-32.8360255980 * yv[4]) + ( 31.3750993500 * yv[5])
					+ (-18.8952757960 * yv[6]) + (  6.5513841772 * yv[7]));
	*in_bp = yv[8];
}

/* 4th order Butterworth bandpass filter, 44100 sampling freq.,
 * corners at 416 and 4000 hz.
 */
void play32_but_bp_416_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bp)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(1.0/4.234058370e+02);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) - (realtype)(4 * (xv[2] + xv[6]) + 6 * xv[4]
               + ( -0.2587874782 * yv[0]) + (  2.3723101917 * yv[1])
               + ( -9.6065481757 * yv[2]) + ( 22.4443568230 * yv[3])
               + (-33.0928401140 * yv[4]) + ( 31.5271697300 * yv[5])
               + (-18.9428281440 * yv[6]) + (  6.5571664529 * yv[7]));
  *in_bp = yv[8];
}

/* 4th order Butterworth bandpass filter, 44100 sampling freq.,
 * corners at 416 and 4757 hz.
 */
void play32_but_bp_416_4757(realtype *xv, realtype *yv, realtype in, realtype *in_bp)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(1.0/2.186484287e+02);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) - (realtype)(4 * (xv[2] + xv[6]) + 6 * xv[4]
               + ( -0.1925516538 * yv[0]) + (  1.8109349941 * yv[1])
               + ( -7.5493194974 * yv[2]) + ( 18.2164183760 * yv[3])
               + (-27.8379755140 * yv[4]) + ( 27.5871261380 * yv[5])
               + (-17.2971021450 * yv[6]) + (  6.2624680178 * yv[7]));
  *in_bp = yv[8];
}
  
/* 4th order Butterworth bandpass filter, 44100 sampling freq.,
 * corners at 495 and 4000 hz.
 */
void play32_but_bp_495_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bp)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(1.0/4.576921490e+02);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) - (realtype)(4 * (xv[2] + xv[6]) + 6 * xv[4]
            + ( -0.2668162421 * yv[0]) + (  2.4302799692 * yv[1])
            + ( -9.7856172436 * yv[2]) + ( 22.7498779640 * yv[3])
            + (-33.4013132940 * yv[4]) + ( 31.7089902870 * yv[5])
            + (-18.9993896500 * yv[6]) + (  6.5639867631 * yv[7]));
  *in_bp = yv[8];
}

/* 4th order Butterworth bandpass filter, 44100 sampling freq.,
 * corners at 495 and 5656 hz.
 */
void play32_but_bp_495_5656(realtype *xv, realtype *yv, realtype in, realtype *in_bp)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(1.0/1.219234118e+02);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) - (realtype)(4 * (xv[2] + xv[6]) + 6 * xv[4]
               + ( -0.1388443764 * yv[0]) + (  1.3333341746 * yv[1])
               + ( -5.7113736171 * yv[2]) + ( 14.2357794520 * yv[3])
               + (-22.6078560300 * yv[4]) + ( 23.4357628700 * yv[5])
               + (-15.4621595390 * yv[6]) + (  5.9153524729 * yv[7]));
  *in_bp = yv[8];
}

/* 4th order Chebyshev bandstop filter, 44100 sampling freq.,
 * corners at 350 and 4000 hz, 2 db ripple.
 */
void play32_cheb_2_bs_350_4000(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{
	xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
	xv[8] = in * (float)(1.0 / 2.190272828e+00);
	yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
	yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8839668614 * (xv[1] + xv[7]) +  27.3088500520 * (xv[2] + xv[6])
					-  54.2796008160 * (xv[3] + xv[5]) +  67.7094359580 * xv[4]
					+ ( -0.3300111703 * yv[0]) + (  2.5145388923 * yv[1])
					+ ( -9.0846412205 * yv[2]) + ( 20.0414285500 * yv[3])
					+ (-29.0525923120 * yv[4]) + ( 27.9670965330 * yv[5])
					+ (-17.2960654660 * yv[6]) + (  6.2402458703 * yv[7]));
	*in_bs = yv[8];
}

/* 4th order Chebyshev bandstop filter, 44100 sampling freq.,
 * corners at 416 and 4757 hz, 2 db ripple.
 */
void play32_cheb_2_bs_416_4757(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(1.0 / 2.589911852e+00);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8345499343 * (xv[1] + xv[7]) +  27.0175647530 * (xv[2] + xv[6])
               -  53.5590265400 * (xv[3] + xv[5]) +  66.7520263700 * xv[4]
               + ( -0.2926745170 * yv[0]) + (  2.0614066907 * yv[1])
               + ( -7.1791197006 * yv[2]) + ( 15.8460502400 * yv[3])
               + (-23.5632717950 * yv[4]) + ( 23.6281253700 * yv[5])
               + (-15.3747158770 * yv[6]) + (  5.8741984588 * yv[7]));
  *in_bs = yv[8];
}
  
/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 283 and 5030 hz.
 */
void play32_but_bs_283_5030(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(DSP_PLAY_VOCAL_TYPEII_BOOST * 1.0 / 2.470175994e+00);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8800903508 * (xv[1] + xv[7]) +  27.2859339760 * (xv[2] + xv[6])
               -  54.2228149920 * (xv[3] + xv[5]) +  67.6339435400 * xv[4]
               + ( -0.1639252313 * yv[0]) + (  1.5742626436 * yv[1])
               + ( -6.7012735207 * yv[2]) + ( 16.5063179530 * yv[3])
               + (-25.7486426410 * yv[4]) + ( 26.0486366610 * yv[5])
               + (-16.6683297870 * yv[6]) + (  6.1529535950 * yv[7]));
	*in_bs = yv[8];
}

/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 400 and 5030 hz.
 */
void play32_but_bs_400_5030(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(DSP_PLAY_VOCAL_TYPEII_BOOST * 1.0 / 2.413268878e+00);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8310172916 * (xv[1] + xv[7]) +  26.9968119330 * (xv[2] + xv[6])
               -  53.5077905250 * (xv[3] + xv[5]) +  66.6839949520 * xv[4]
               + ( -0.1717396179 * yv[0]) + (  1.6319898936 * yv[1])
               + ( -6.8827400491 * yv[2]) + ( 16.8196061380 * yv[3])
               + (-26.0637905660 * yv[4]) + ( 26.2266461170 * yv[5])
               + (-16.7163549410 * yv[6]) + (  6.1563817055 * yv[7]));
  *in_bs = yv[8];
}  

/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 229 and 6325 hz.
 */
void play32_but_bs_229_6325(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(DSP_PLAY_VOCAL_TYPEII_BOOST * 1.0 / 3.253340559e+00);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8747059787 * (xv[1] + xv[7]) +  27.2541228440 * (xv[2] + xv[6])
               -  54.1440146340 * (xv[3] + xv[5]) +  67.5291965010 * xv[4]
               + ( -0.0946825079 * yv[0]) + (  0.9514203236 * yv[1])
               + ( -4.2791179552 * yv[2]) + ( 11.1845117200 * yv[3])
               + (-18.6409147100 * yv[4]) + ( 20.3230137830 * yv[5])
               + (-14.1114629180 * yv[6]) + (  5.6672319678 * yv[7]));
  *in_bs = yv[8];
}

/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 324 and 6325 hz.
 */
void play32_but_bs_324_6325(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(DSP_PLAY_VOCAL_TYPEII_BOOST * 1.0 / 3.189419487e+00);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8232866002 * (xv[1] + xv[7]) +  26.9514299610 * (xv[2] + xv[6])
               -  53.3957955450 * (xv[3] + xv[5]) +  66.5353081780 * xv[4]
               + ( -0.0984877343 * yv[0]) + (  0.9803285388 * yv[1])
               + ( -4.3709929483 * yv[2]) + ( 11.3441540620 * yv[3])
               + (-18.7979900930 * yv[4]) + ( 20.4006042510 * yv[5])
               + (-14.1213860340 * yv[6]) + (  5.6637687635 * yv[7]));
  *in_bs = yv[8];
}
  
/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 185 and 7953 hz.
 */
void play32_but_bs_185_7953(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(DSP_PLAY_VOCAL_TYPEII_BOOST * 1.0 / 4.678939280e+00);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8669727040 * (xv[1] + xv[7]) +  27.2084723220 * (xv[2] + xv[6])
               -  54.0309878220 * (xv[3] + xv[5]) +  67.3789776310 * xv[4]
               + ( -0.0466466687 * yv[0]) + (  0.4821316297 * yv[1])
               + ( -2.3374643150 * yv[2]) + (  6.5688678330 * yv[3])
               + (-11.8759839310 * yv[4]) + ( 14.3357927810 * yv[5])
               + (-11.1980220270 * yv[6]) + (  5.0713244367 * yv[7]));
  *in_bs = yv[8];
}

/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 262 and 7953 hz.
 */
void play32_but_bs_262_7953(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(DSP_PLAY_VOCAL_TYPEII_BOOST * 1.0 / 4.598220046e+00);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8122434771 * (xv[1] + xv[7]) +  26.8866805540 * (xv[2] + xv[6])
               -  53.2361172440 * (xv[3] + xv[5]) +  66.3233651870 * xv[4]
               + ( -0.0482047296 * yv[0]) + (  0.4949125022 * yv[1])
               + ( -2.3776127448 * yv[2]) + (  6.6389188911 * yv[3])
               + (-11.9424718900 * yv[4]) + ( 14.3562911380 * yv[5])
               + (-11.1847517920 * yv[6]) + (  5.0629175696 * yv[7]));
  *in_bs = yv[8];
}

/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 150 and 10,000 hz.
 */
void play32_but_bs_150_10000(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(DSP_PLAY_VOCAL_TYPEII_BOOST * 1.0 / 7.735466094e+00);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8536815259 * (xv[1] + xv[7]) +  27.1301175660 * (xv[2] + xv[6])
               -  53.8371407480 * (xv[3] + xv[5]) +  67.1214112060 * xv[4]
               + ( -0.0210635777 * yv[0]) + (  0.1711778749 * yv[1])
               + ( -0.9937184261 * yv[2]) + (  3.0802199599 * yv[3])
               + ( -6.0114210074 * yv[4]) + (  8.3631508823 * yv[5])
               + ( -7.9239214019 * yv[6]) + (  4.3355754645 * yv[7]));
  *in_bs = yv[8];
}

/* 4th order Butterworth bandstop filter, 44100 sampling freq.,
 * corners at 100 and 12,000 hz.
 * NOTE - THIS SEEMED TOO EXTREME
 */
void play32_but_bs_100_12000(realtype *xv, realtype *yv, realtype in, realtype *in_bs)
{ xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
  xv[8] = in * (float)(DSP_PLAY_VOCAL_TYPEII_BOOST / 1.379551295e+01);
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
  yv[8] =   (xv[0] + xv[8]) -   (realtype)(7.8700365903 * (xv[1] + xv[7]) +  27.2265534750 * (xv[2] + xv[6])
					-  54.0757473900 * (xv[3] + xv[5]) +  67.4384621250 * xv[4]
					+ ( -0.0195363135 * yv[0]) + (  0.0117868426 * yv[1])
					+ ( -0.3549570419 * yv[2]) + (  1.3943970185 * yv[3])
					+ ( -2.5096638255 * yv[4]) + (  3.9443245116 * yv[5])
					+ ( -5.0964127909 * yv[6]) + (  3.6300615183 * yv[7]));
  *in_bs = yv[8];
}

void play32_no_filt(realtype *xv, realtype *yv, realtype in, realtype *in_filt)
{
	*in_filt = in;
}
#endif /* DSPSOFT_32_BIT */
#endif /* !defined PT_DMX_BUILD */

#endif
