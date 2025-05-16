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
 * FILE: MaxiX.c
 * DATE: 9/13/2000
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: Maximizer
 *
 *  NOTE - this is the new Auto version that backs off on the gain boost
 *  if the estimated boosted output level exceeds a target level.
 *
 */

#ifndef DSPSOFT_TARGET
#define DSP_TARGET
#endif
#include "Maxi_num.h"  /* Sets bit precision */

/* Standard includes */
#include <math.h>
#include <stdio.h>
#include "codedefs.h" /* Company common defines */
#include "platform.h" /* Targets a specific DSP card       */
#include "pt_defs.h"  /* Defines common to both PC and DSP */	
#include "product_type.h" /* Sets DSPFX or DFX */
#include "boardrv1.h" /* DSP hardware specific defines, for both PC and DSP */

/* #define DSP_PROGRAM_SIZE 0x400 */ /* Overide default program size */
#ifdef DSP_TARGET
#include "dma32.h"
#endif

#include "dsp_mem1.h" /* For DSP memory configuration (typical delay version) */
#include "c_dsps.h"
#include "c_max.h"   /* For specific parameter mappings */
#include "dutio.h"	  /* A/D I/O macros */
#include "dutcom.h"	  /* PC to DSP and DSP to PC com routines */
#include "dutinit.h"  /* DSP and AD initialization */
#include "kerdma.h"
#include "kerdelay.h"  /* Has wet-dry macro */
#include "kernoise.h"

static void DSPS_MAXI_RUN(void);

#ifdef DSP_TARGET
void main(int argc, char *argv[])
{
	DSPS_MAXI_INIT();
	while(1)
		DSPS_MAXI_RUN();
}
#endif

/* Only build init function in 32 bit files (same for both) */
#if defined(DSPSOFT_32_BIT)
DSP_FUNC_DEF int DSPS_MAXI_INIT(float *fp_params, float *fp_memory, long l_memsize, float *fp_state, int i_init_flag, float r_samp_freq)
{
	float *COMM_MEM_OFFSET = fp_params;
	float *MEMBANK0_START = fp_memory;

	/* Variables below must be restored from the state array and
	 * stored back at end of buffer processing
	 */

	long i;
	struct dspMaxiStructType *s = (struct dspMaxiStructType *)(COMM_MEM_OFFSET);
 
	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		s->wet_gain = (realtype)0.0;	  
		s->dry_gain = (realtype)0.0;

		/* Initializations from quick pick 1, 44.1khz (recalc for different s_freq */
		s->gain_boost = (realtype)1.99526;
		s->max_output = (realtype)0.966051;
		s->max_delay = 33;
		s->release_time_beta = (realtype)0.997776;
		s->num_quant_bits = KERNOISE_QUANTIZE_16; /* Hardcoded for 16 bit quantization */
		s->dither_type = KERNOISE_DITHER_SHAPED;

		s->ramp_count_l = 0;
		s->ramp_count_r = 0;
		s->max_abs_l = (realtype)0.0;
		s->max_abs_r = (realtype)0.0;
		s->env_l = (realtype)0.0;
		s->env_r = (realtype)0.0;
		s->noise1_old = (realtype)0.0;
		s->noise2_old = (realtype)0.0;

		/* New params and state variables from auto mode */
		/* target is the targeted output level. Setting this too low results in
		 * output reduction even with zero boost, while setting it too high
		 * causes too much boost related distortion,so its a tradeoff.
		 * On first shot, a target of 0.28 seems about right. Really hot
		 * songs have an level estimate that exceeds that.
		 * Note that in DFX this is currently set in the function
		 * dfxp_CommunicateFixedQnts_Opt(), so this is an initialization
		 * below that is overwritten by DFX.
		 */
		s->target_level = (realtype)MAXIMIZE_TARGET_LEVEL_SETTING;
		s->level = (realtype)0.0;

		{
			/* Design the single pole level estimation lowpass filter.
			 * Looks like a cuttoff freq of 0.01 hz is about the limit for
			 * single precision implementation (a0 = 0.999999), needs double precision design
			 * process
			 */
			double r_omega = 6.283185 * MAXIMIZE_LEVEL_FILT_CUTOFF/r_samp_freq;

			double cos_om;
			double root_calc;
			double d_tmp;

			cos_om = cos(r_omega);
			root_calc = sqrt(cos_om * cos_om - 4.0 * cos_om + 3.0);

			d_tmp = (2.0 - cos_om - root_calc);
			s->a0 = (realtype)d_tmp;
			s->filt_gain = (realtype)(1.0 - d_tmp);
		}
 	}

	if( i_init_flag & DSPS_INIT_MEMORY )
	{
		/* Note that any state vars must be initialized in this section
		 * since they must be present in the dll instance that will
		 * call the processing functions
		 */
 		/* Get mem sizes from PC, zero memory */
		dutilGetMemSizes();
		dutilInitMem(); /* Doesn't do anything for DSPSOFT */
		/* Now do DSPSOFT init */
		/*  No allocated memory in this plug-in
		for(i=0; i<l_memsize; i++)
			fp_memory[i] = 0.0;
		 */

		/* Initialize memory pointers. Note main memsize is already
		 * compensated for 0x400 program size
		 */
		s->ptr_l = s->dly_start_l;
		s->ptr_r = s->dly_start_r;

 		/* Place variables into state array */
		{
			/*
			float **fpp = (float **)&(fp_state[0]);
			long *lpp  =  (long *)&(fp_state[0]);
			fpp[0] = ptr0;
			*/
		}
	}

	/* Zero internal signal memory spaces to eliminate glitches on restarts */
	if( (i_init_flag & DSPS_INIT_MEMORY) || (i_init_flag & DSPS_ZERO_MEMORY) )
	{
		for(i=0; i<MAXI_MAX_DELAY_LEN; i++)
			s->dly_start_l[i] = s->dly_start_r[i] = (realtype)0.0;
	}

	/* Initialize A/D and D/A converters, and set Samp Rate */
	/* Do as close as possible to running to minimize pops */
	dutilInitAIO();                                                            
	kerdmainit_MACRO();

	return(OKAY);
}
#endif /* DSPSOFT_32_BIT */

#ifdef DSPSOFT_TARGET
DSP_FUNC_DEF void DSPS_MAXI_PROCESS(long *lp_data, int l_length,
								   float *fp_params, float *fp_memory, float *fp_state,
								   struct hardwareMeterValType *sp_meters,
								   int DSP_data_type)
{
	float *COMM_MEM_OFFSET = fp_params;
	float *MEMBANK0_START = fp_memory;

	/* Variables below must be restored from the state array and
	 * stored back at end of buffer processing
	 */
	/*
	float **fpp = (float **)&(fp_state[0]);
	long *lpp  =  (long *)&(fp_state[0]);
	float *ptr0 = fpp[0];
	*/

	long transfer_state = 0; /* For sending out meter values */
	long status = 0;         /* For sending run time status to PC */

	/* All the vars below are from DMA_LOCAL_DECLARATIONS.
	 * They are declared there as statics, but don't need to be
	 */
	unsigned data_index = 0;
	float in_meter1_dma = 0.0;
	float in_meter2_dma = 0.0;
	float out_meter1_dma = 0.0;
	float out_meter2_dma = 0.0;

	/* All the vars below are from DMA_GLOBAL_DECLARATIONS.
	 * They are declared there as statics, but don't need to be
	 */
	long *read_in_buf;
	long *read_out_buf;

	int i;
	struct dspMaxiStructType *s = (struct dspMaxiStructType *)(COMM_MEM_OFFSET);

	/* For supplemental data (peak limiting info) back to the PC */
	float peak_in1 = (float)1.0e-20;
	float peak_in2 = (float)1.0e-20;
	float peak_out1 = (float)1.0e-20;
	float peak_out2 = (float)1.0e-20;

	/* Run one buffer full of data. Zero index and averaged meter vals. */

	read_in_buf = lp_data;
	read_out_buf = lp_data;

	for(i=0; i<l_length; i++)
	{
		float out1, out2;
		float in1, in2;
		volatile long in_count = 0;
		volatile long out_count = 0;
		float dly_l_out, dly_r_out;
		float new_abs_l, new_abs_r;
		float in_sqr;
		float sqrt_level;
		float gain_boost;

	  	load_parameter(); /* If its been sent, loads a parameter into memory */

		/* Next line includes kerdmaru.h run loop macro (no args) */
		#include "kerdmaru.h"

	  	out1 = out2 = 0.0;
		 
		dutilGetInputsAndMeter( in1, in2, status);

		/* Use just left input for level estimate */
		in_sqr = in1 * in1;

		/* This filter type and cutoff frequency are a compromise.
		 * Currently a single pole lowpass is used, with a cuttoff of
		 * 0.06 hz. To implement a two pole (for example Butterworth)
		 * filter at this low a frequency looks like it would take both
		 * a double precision design and coefficient and internal storage implementation.
		 */
		s->level = s->level * s->a0 + in_sqr * s->filt_gain;

		sqrt_level = (float)sqrt(s->level);

		{
			realtype result = s->gain_boost * sqrt_level;

			if( result > s->target_level )
			{
				/* gain_boost = s->gain_boost * THRESH/sqrt_level; */
				/*
				realtype tmp_boost = THRESH/sqrt_level;

				if(tmp_boost > (realtype)1.0)
					gain_boost = THRESH/sqrt_level;
				else
					gain_boost = s->gain_boost;
				 */
				gain_boost = s->target_level/sqrt_level;
			
				/* 11/4/04 Modifications to help fix volume pumping */
				if(gain_boost < (float)1.06)
					gain_boost = (float)1.06;
				
			}
			else
				gain_boost = s->gain_boost;
		}

		dly_l_out = *(s->ptr_l);
		*(s->ptr_l) = gain_boost * s->max_output * in1;
		new_abs_l = (float)fabs(*(s->ptr_l));
		(s->ptr_l)++;
		if( s->ptr_l >= (s->dly_start_l + s->max_delay) )
			s->ptr_l = s->dly_start_l;

		/* Envelope Update - start by checking if we're in ramping mode to meet a peak */
		if( s->ramp_count_l )
		{
			realtype tmp_delta;

			/* First check if the envelope has decayed to the point where the current output
			 * value will exceed the targeted output level.
			 */
			{
				 realtype abs_out = (float)fabs(dly_l_out);
				 if ( abs_out > s->env_l )
					 s->env_l = abs_out;
			}

			/* Check to see if new input is higher than the ramp target */
			if(new_abs_l > s->max_abs_l )
			{
				/* Reset for new higher target level */
				s->max_abs_l = new_abs_l;
				s->ramp_count_l = s->max_delay;
				/* Note that since envelope ramping starts immediately on this sample,
				 * divisor of delta calc is delay plus one.
				 * To handle the case where the new envelope would fall under the last max
				 * val, only change the delta if its larger.
				 */
				tmp_delta = (new_abs_l - s->env_l)/(realtype)(s->max_delay + 1);
				if( tmp_delta > s->delta_l )
					s->delta_l = tmp_delta;
			}
			else
				s->ramp_count_l--;

			s->env_l += s->delta_l;
		}
		else
		{
			/* Not in ramp mode, first decay the envelope. */
			 s->env_l = s->env_l * s->release_time_beta + (realtype)MAXI_ENVELOPE_BIAS;
				
			/* Check if the envelope has decayed to the point where the current output
			 * value will exceed the targeted output level.
			 */
			{
				 realtype abs_out = (float)fabs(dly_l_out);
				 if ( abs_out > s->env_l )
					 s->env_l = abs_out;
			}

			/* Check to see if we need to start ramp mode */
			if(new_abs_l > s->env_l)
			{
				s->max_abs_l = new_abs_l;
  			   /* Note that since envelope ramping starts immediately on this sample,
				 * divisor of delta calc is delay plus one.
				 */
				s->delta_l = (new_abs_l - s->env_l)/(realtype)(s->max_delay + 1);
				s->env_l += s->delta_l;
				s->ramp_count_l = s->max_delay;
			}
		}
		/* End of envelope update */

		/* Calculate outputs and extra peak info to send back */
		if( s->env_l > s->max_output )
		{
			/* Normalize peak */
			out1 = dly_l_out * s->max_output/s->env_l;
			/* Check if max peak for graph */
			if(s->env_l > peak_in1)
			{
				/* Note we will later take gain boost out of input peak */
				peak_in1 = s->env_l;
				peak_out1 = s->max_output;
			}
		}
		else
		{
			out1 = dly_l_out;
			if(s->env_l > peak_in1)
			{
				peak_in1 = s->env_l;
				peak_out1 = s->env_l;
			}
		}

		if(s->stereo_in_flag)
		{
			dly_r_out = *(s->ptr_r);
			*(s->ptr_r) = gain_boost * s->max_output * in2;
			new_abs_r = (float)fabs(*(s->ptr_r));
			(s->ptr_r)++;
			if( s->ptr_r >= (s->dly_start_r + s->max_delay) )
				s->ptr_r = s->dly_start_r;

			/* Envelope Update - start by checking if we're in ramping mode to meet a peak */
			if( s->ramp_count_r )
			{
				realtype tmp_delta;

				/* First check if the envelope has decayed to the point where the current output
				 * value will exceed the targeted output level.
				 */
				{
					 realtype abs_out = (float)fabs(dly_r_out);
					 if ( abs_out > s->env_r )
						 s->env_r = abs_out;
				}

				/* Check to see if new input is higher than the ramp target */
				if(new_abs_r > s->max_abs_r )
				{
					/* Reset for new higher target level */
					s->max_abs_r = new_abs_r;
					s->ramp_count_r = s->max_delay;
					/* Note that since envelope ramping starts immediately on this sample,
					 * divisor of delta calc is delay plus one.
					 * To handle the case where the new envelope would fall under the last max
					 * val, only change the delta if its larger.
					 */
					tmp_delta = (new_abs_r - s->env_r)/(realtype)(s->max_delay + 1);
					if( tmp_delta > s->delta_r )
						s->delta_r = tmp_delta;
				}
				else
					s->ramp_count_r--;

				s->env_r += s->delta_r;
			}
			else
			{
				/* Not in ramp mode, first decay the envelope. */
				 s->env_r = s->env_r * s->release_time_beta + (realtype)MAXI_ENVELOPE_BIAS;
					
				/* Check if the envelope has decayed to the point where the current output
				 * value will exceed the targeted output level.
				 */
				{
					 realtype abs_out = (float)fabs(dly_r_out);
					 if ( abs_out > s->env_r )
						 s->env_r = abs_out;
				}

				/* Check to see if we need to start ramp mode */
				if(new_abs_r > s->env_r)
				{
					s->max_abs_r = new_abs_r;
  					/* Note that since envelope ramping starts immediately on this sample,
					 * divisor of delta calc is delay plus one.
					 */
					s->delta_r = (new_abs_r - s->env_r)/(realtype)(s->max_delay + 1);
					s->env_r += s->delta_r;
					s->ramp_count_r = s->max_delay;
				}
			}
			/* End of envelope update */

			/* Calculate outputs and extra peak info to send back */
			if( s->env_r > s->max_output )
			{
				/* Normalize peak */
				out2 = dly_r_out * s->max_output/s->env_r;
				/* Check if max peak for graph */
				if(s->env_r > peak_in2)
				{
					/* Note we will later take gain boost out of input peak */
					peak_in2 = s->env_r;
					peak_out2 = s->max_output;
				}
			}
			else
			{
				out2 = dly_r_out;
				if(s->env_r > peak_in2)
				{
					peak_in2 = s->env_r;
					peak_out2 = s->env_r;
				}
			}
		}

		if( s->quantize_on_flag )
		{
			static unsigned long seed = MAXIMIZE_NOISE_SEED;
			realtype dither1, dither2;

			switch (s->dither_type)
			{
			case KERNOISE_DITHER_NONE:
				dither1 = (realtype)0.0;
				dither2 = (realtype)0.0;
				/* Experimental method */
				/*
				{
					static int mode = 0;
					if( mode )
					{
						dither1 = (realtype)0.5;
						dither2 = (realtype)0.5;
						mode = 0;
					}
					else
					{
						dither1 = -(realtype)0.5;
						dither2 = -(realtype)0.5;
						mode = 1;
					}
				}
				*/
				break;
			case KERNOISE_DITHER_UNIFORM:
				kerUniformWhiteNoise(seed, 0.5, dither1);
				kerUniformWhiteNoise(seed, 0.5, dither2);
				break;
			case KERNOISE_DITHER_TRIANGULAR:
				/* Note- "traditional" style is +/- 1.0 peak */
				kerTriangularWhiteNoise(seed, 1.0, dither1);
				kerTriangularWhiteNoise(seed, 1.0, dither2);
				break;
			case KERNOISE_DITHER_SHAPED:
				{
					realtype noise_tmp;

					/* Note- "traditional" style is +/- 0.5 peak */
					/* +/- .325 seems like a better compromise between
					 * noise level and dynamic range increases. Getting close
					 * to 20 db improvement, but a little better with uniform.
					 */
					kerUniformWhiteNoise(seed, 0.325, noise_tmp);
					dither1 = noise_tmp - s->noise1_old;
					s->noise1_old = noise_tmp;

					kerUniformWhiteNoise(seed, 0.325, noise_tmp);
					dither2 = noise_tmp - s->noise2_old;
					s->noise2_old = noise_tmp;
				}
				break;
			}

			switch (s->num_quant_bits)
			{
			case KERNOISE_QUANTIZE_24:
				/* Experimental method */
				/*
				{
					realtype error, noise_tmp;

					kerUniformWhiteNoise(seed, 0.5, noise_tmp);
					kerQuantizeWithError(out1,KERNOISE_PEAK_LEVEL_8, error);
					if(noise_tmp >= error)
						out1 += (realtype)(1.0/KERNOISE_PEAK_LEVEL_8);
					else
						out1 -= (realtype)(1.0/KERNOISE_PEAK_LEVEL_8);

					kerUniformWhiteNoise(seed, 0.5, noise_tmp);
					kerQuantizeWithError(out2,KERNOISE_PEAK_LEVEL_8, error);
					if(noise_tmp >= error)
						out2 += (realtype)(1.0/KERNOISE_PEAK_LEVEL_8);
					else
						out2 -= (realtype)(1.0/KERNOISE_PEAK_LEVEL_8);
				}
				*/
				break;
			case KERNOISE_QUANTIZE_20:
				kerQuantizeAndDither(out1, KERNOISE_PEAK_LEVEL_20, dither1);
				kerQuantizeAndDither(out2, KERNOISE_PEAK_LEVEL_20, dither2);
				break;
			case KERNOISE_QUANTIZE_16:
				kerQuantizeAndDither(out1, KERNOISE_PEAK_LEVEL_16, dither1);
				kerQuantizeAndDither(out2, KERNOISE_PEAK_LEVEL_16, dither2);
				break;
			case KERNOISE_QUANTIZE_12:
				kerQuantizeAndDither(out1, KERNOISE_PEAK_LEVEL_12, dither1);
				kerQuantizeAndDither(out2, KERNOISE_PEAK_LEVEL_12, dither2);
				break;
			case KERNOISE_QUANTIZE_8:
				kerQuantizeAndDither(out1, KERNOISE_PEAK_LEVEL_8, dither1);
				kerQuantizeAndDither(out2, KERNOISE_PEAK_LEVEL_8, dither2);
					break;
			}
		}

		if( !(s->stereo_in_flag) )
		{
			in2 = (realtype)0.0;
			out2 = (realtype)0.0;
		}

		/* WHY IS THIS NEEDED AT ALL ?? (clipping?) */
		kerWetDry(in1, in2, &(s->wet_gain), &(s->dry_gain), out1, out2);

		dutilSetClipStatus(in1, in2, out1, out2, status);

		dutilPutOutputsAndMeter(out1, out2, status);

		write_meters_and_status(in_meter1, in_meter2, out_meter1, out_meter2, status, transfer_state);
	}

	/* Write averaged meter data temporarily writing as a float.
	 * Will be converted to long and factored in calling function.
	 */
	write_meter_average();

#if defined(DSPSOFT_TARGET) & (PT_DSP_BUILD == PT_DSP_DSPFX)
	/* Write extra graphic data. Take gain boost off of maximum input peak */
	sp_meters->aux_vals[0] = (realtype)20.0 * log10(peak_in1/(s->gain_boost * s->max_output));
	if( sp_meters->aux_vals[0] > (realtype)0.0)
		sp_meters->aux_vals[0] = (realtype)0.0; 

	sp_meters->aux_vals[2] = (realtype)20.0 * log10(peak_in2/(s->gain_boost * s->max_output));
	if( sp_meters->aux_vals[2] > (realtype)0.0)
		sp_meters->aux_vals[2] = (realtype)0.0; 

	sp_meters->aux_vals[1] = (realtype)20.0 * log10(peak_out1);
	sp_meters->aux_vals[3] = (realtype)20.0 * log10(peak_out2);
#endif

	/* Place variables into state array */
	{
		/*
		float **fpp = (float **)&(fp_state[0]);
		long *lpp  =  (long *)&(fp_state[0]);
		fpp[0] = ptr0;
		*/
	}
} 
#endif
