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
 * FILE: Proto1X.c
 * DATE: 7/16/98
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: Prototype 1
 *
 */

#ifndef DSPSOFT_TARGET
#define DSP_TARGET
#endif
#include "Lex_num.h"  /* Sets number of elements */

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
#include "c_lex.h"   /* For specific parameter mappings */
#include "dutio.h"	  /* A/D I/O macros */
#include "dutcom.h"	  /* PC to DSP and DSP to PC com routines */
#include "dutinit.h"  /* DSP and AD initialization */
#include "kerdma.h"
#include "kerdelay.h"  /* Has wet-dry macro */
#include "kernoise.h"
#include "kerLatic.h"

static void DSPS_LEX_RUN(void);

#ifdef DSP_TARGET
void main(int argc, char *argv[])
{
	DSPS_LEX_INIT();
	while(1)
		DSPS_LEX_RUN();
}
#endif

/* Only build init function in 32 bit files (same for both) */
#if defined(DSPSOFT_32_BIT)
DSP_FUNC_DEF int DSPS_LEX_INIT(float *fp_params, float *fp_memory, long l_memsize, float *fp_state, int i_init_flag, float r_samp_freq)
{
	float *COMM_MEM_OFFSET = fp_params;
	float *MEMBANK0_START = fp_memory;

	/* Variables below must be restored from the state array and
	 * stored back at end of buffer processing
	 */

	long i;
	struct dspLexStructType *s = (struct dspLexStructType *)(COMM_MEM_OFFSET);
 
	s->dsp_sampling_freq = r_samp_freq;

	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		/* Initial values from quick pick one but with dry/wet of 0.26, 44.1kHz */

		s->wet_gain = (realtype)0.0;	  
		s->dry_gain = (realtype)0.0;
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

		s->old_damp_val1_l = (realtype)0.0;
		s->old_damp_val2_l = (realtype)0.0;
		s->old_bandwidth_val_l = (realtype)0.0;
		s->D4_out = (realtype)0.0;
 	}

	if( i_init_flag & DSPS_INIT_MEMORY )
	{
		/* Initialize oscillator interpolation table values. Note
		 * that normally that would be done in the init mem section below,
		 * but in the reverb that gets called every time the room size is changed,
		 * so it was moved up here with the parameters. Note the allowance for the
		 * extra point on the end to handle the roundoff problem.
		 */
		s->osc_mult_table = fp_memory;
		s->osc_plus_table = s->osc_mult_table + LEX_NUM_OSC_PTS;
		s->osc_table_p = (LEX_OSC_PHASE * (LEX_NUM_OSC_PTS - 1))/360.0;                    
		s->f_num_pts = LEX_NUM_OSC_PTS - 1;

		/* Use First (zero) point of sine up to the very last point before zero, repeating zero */ 
		{
			float x1, x2, y1, y2;                             
 		  
			for(i=0; i<(LEX_NUM_OSC_PTS - 1); i++)
			{
				x1 = (float)i;
				x2 = (float)(i + 1);
				y1 = (float)cos(TWO_PI * x1/(LEX_NUM_OSC_PTS - 1));
				y2 = (float)cos(TWO_PI * x2/(LEX_NUM_OSC_PTS - 1));
				s->osc_mult_table[i] = (y1 - y2); 
				s->osc_plus_table[i] = ((x1 * y2) - (x2 * y1)); 
			}
			/* Now set repeated endpoint, used to handle pointer roundoff problem */
			s->osc_mult_table[LEX_NUM_OSC_PTS - 1] = s->osc_mult_table[0]; 
			s->osc_plus_table[LEX_NUM_OSC_PTS - 1] = s->osc_plus_table[0]; 
		}
		
		/* Note that any state vars must be initialized in this section
		 * since they must be present in the dll instance that will
		 * call the processing functions.
		 * IS THIS STILL TRUE?
		 */
 		/* Get mem sizes from PC, zero memory */
		dutilGetMemSizes();
		dutilInitMem(); /* Doesn't do anything for DSPSOFT */
	}

	/* Zero internal signal memory spaces to eliminates glitches on restarts */
	if( (i_init_flag & DSPS_INIT_MEMORY) || (i_init_flag & DSPS_ZERO_MEMORY) )
	{
		/* Now do DSPSOFT init */
		for(i=0; i<(DSPS_SOFT_MEM_LEX_LENGTH - 2 * LEX_NUM_OSC_PTS); i++)
			fp_memory[i + 2 * LEX_NUM_OSC_PTS] = 0.0;
	}

	/* Need this split so after changing roomsize we can reinit memory pointers.
	 * Note that we will come into this code before the memory has been initialized,
	 * so we can't do any operations on memory here, only pointer initializationa
	 */
	if( (i_init_flag & DSPS_INIT_MEMORY) || (i_init_flag & DSPS_RE_INIT_MEMORY) )
	{
		float r_roomsize;

		s->MasterStart = s->osc_plus_table + LEX_NUM_OSC_PTS;
		s->ptr = s->MasterStart;
		s->pre_dly_len_l = (unsigned long)(r_samp_freq * (realtype)LEX_PREDELAY_LEN);
		s->MasterLen = s->pre_dly_len_l;

		s->lat1_dly_len_l = (long)(r_samp_freq * (realtype)LAT1_LEFT_DELAY_LEN);
		s->MasterLen += s->lat1_dly_len_l;

		s->lat2_dly_len_l = (long)(r_samp_freq * (realtype)LAT2_LEFT_DELAY_LEN);
		s->MasterLen += s->lat2_dly_len_l;

		s->lat3_dly_len_l = (long)(r_samp_freq * (realtype)LAT3_LEFT_DELAY_LEN);
		s->MasterLen += s->lat3_dly_len_l;

		s->lat4_dly_len_l = (long)(r_samp_freq * (realtype)LAT4_LEFT_DELAY_LEN);
		s->MasterLen += s->lat4_dly_len_l;

		/* Note that for the rest of this function, we need to normalize r_roomsize
		 * by the sampling frequency.
		 */
		r_roomsize = s->roomsize * r_samp_freq;
		
		/* Make sure non-modulated is right on an integer value */
		s->lat5_dly_len_l = (float)((long)(LAT5_LEFT_DELAY_LEN_NOMINAL * r_roomsize));
		/* Add one for roundoff room */
		s->lat5_dly_maxlen_l = (unsigned long)s->lat5_dly_len_l + (unsigned long)(r_samp_freq * (realtype)LEX_MAX_MODULATION_DELAY) + 1;
		s->MasterLen += s->lat5_dly_maxlen_l;

		s->D1_tap1 = (unsigned long)(D1_LEFT_TAP1_DELAY * r_roomsize);
		s->D1_tap2 = (unsigned long)(D1_LEFT_TAP2_DELAY * r_roomsize);
		s->D1_tap3 = (unsigned long)(D1_LEFT_TAP3_DELAY * r_roomsize);
		s->D1_tap4 = (unsigned long)(D1_LEFT_TAP4_DELAY * r_roomsize);
		s->MasterLen += s->D1_tap4;

		s->lat6_tap1 = (unsigned long)(LAT6_LEFT_TAP1_DELAY * r_roomsize);
		s->lat6_tap2 = (unsigned long)(LAT6_LEFT_TAP2_DELAY * r_roomsize);
		s->lat6_dly_len_l = (unsigned long)(LAT6_LEFT_DELAY_LEN * r_roomsize);
		s->MasterLen += s->lat6_dly_len_l;

		s->D2_tap1 = (unsigned long)(D2_LEFT_TAP1_DELAY * r_roomsize);
		s->D2_tap2 = (unsigned long)(D2_LEFT_TAP2_DELAY * r_roomsize);
		s->D2_tap3 = (unsigned long)(D2_LEFT_TAP3_DELAY * r_roomsize);
		s->MasterLen += s->D2_tap3;

		/* Make sure non-modulated is right on an integer value */
		s->lat7_dly_len_l = (float)((long)(LAT7_LEFT_DELAY_LEN_NOMINAL * r_roomsize));
		/* Add one for roundoff room */
		s->lat7_dly_maxlen_l = (unsigned long)s->lat7_dly_len_l + (unsigned long)(r_samp_freq * (realtype)LEX_MAX_MODULATION_DELAY) + 1;
		s->MasterLen += s->lat7_dly_maxlen_l;

		s->D3_tap1 = (unsigned long)(D3_LEFT_TAP1_DELAY * r_roomsize);
		s->D3_tap2 = (unsigned long)(D3_LEFT_TAP2_DELAY * r_roomsize);
		s->D3_tap3 = (unsigned long)(D3_LEFT_TAP3_DELAY * r_roomsize);
		s->D3_tap4 = (unsigned long)(D3_LEFT_TAP4_DELAY * r_roomsize);
		s->MasterLen += s->D3_tap4;

		s->lat8_tap1 = (unsigned long)(LAT8_LEFT_TAP1_DELAY * r_roomsize);
		s->lat8_tap2 = (unsigned long)(LAT8_LEFT_TAP2_DELAY * r_roomsize);
		s->lat8_dly_len_l = (unsigned long)(LAT8_LEFT_DELAY_LEN * r_roomsize);
		s->MasterLen += s->lat8_dly_len_l;

		s->D4_tap1 = (unsigned long)(D4_LEFT_TAP1_DELAY * r_roomsize);
		s->D4_tap2 = (unsigned long)(D4_LEFT_TAP2_DELAY * r_roomsize);
		s->D4_tap3 = (unsigned long)(D4_LEFT_TAP3_DELAY * r_roomsize);
		s->MasterLen += s->D4_tap3;

		/* Note that we subtract one so MasterEnd points to the last actually used location */
		s->MasterEnd = s->MasterStart + s->MasterLen - 1;
	}

	/* Initialize A/D and D/A converters, and set Samp Rate */
	/* Do as close as possible to running to minimize pops */
	dutilInitAIO();                                                            
	kerdmainit_MACRO();

	return(OKAY);
}
#endif /* DSPSOFT_32_BIT */

#ifdef DSPSOFT_TARGET
DSP_FUNC_DEF void DSPS_LEX_PROCESS(long *lp_data, int l_length,
								   float *fp_params, float *fp_memory, float *fp_state,
								   struct hardwareMeterValType *sp_meters, int DSP_data_type)
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
	struct dspLexStructType *s = (struct dspLexStructType *)(COMM_MEM_OFFSET);

	/* Run one buffer full of data. Zero index and averaged meter vals.
	 * Note- making local version of s->ptr, s->MasterStart, s->MasterEnd and s->MasterLen
	 * didn't improve performance any.
	 */

	read_in_buf = lp_data;
	read_out_buf = lp_data;

	for(i=0; i<l_length; i++)
	{
		float out1, out2;
		float in1, in2;
		float tmp_a, tmp_b;
		float tap1_out, tap2_out, tap3_out, tap4_out;
		float input_diffuser_out;
		volatile long in_count = 0;
		volatile long out_count = 0;
		float next_out;
		#if (PT_DSP_BUILD == PT_DSP_DSPFX)
		float delay_real;
		float osc;
		#endif

	  	load_parameter(); /* If its been sent, loads a parameter into memory */

		/* Next line includes kerdmaru.h run loop macro (no args) */
		#include "kerdmaru.h"

	  	out1 = out2 = 0.0;
		 
		dutilGetInputsAndMeter( in1, in2, status);

		dutilMuteInputs(in1, in2);

	   in1 += (float)DSP_DENORM_BIAS;
	   in2 += (float)DSP_DENORM_BIAS;

		/* kerRunDelayLineNoPop((in1 + in2), tmp_b, s->pre_delay, s->pre_dly_start_l, s->pre_dly_end_l, s->pre_ptr_l, s->pre_dly_len_l); */
		/* Note that since this implements a delay, it doesn't need the next_out value.
		 * next_out supplies the oldest value of the next delay line in the sequence.
		 */
		{
			float *rp_MACRO;
			/* Note- since this is the first pointer increment, need to add 1 */
			s->ptr += s->pre_dly_len_l + 1;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			rp_MACRO = (float *)(s->ptr - s->pre_delay );
			if((long)rp_MACRO < (long)s->MasterStart)
				rp_MACRO += s->MasterLen;
			tmp_b = *rp_MACRO;
			next_out = *s->ptr;
			*s->ptr = in1 + in2;
		}

		/* One pole bandwidth lowpass. */
		tmp_a = tmp_b * s->one_minus_bandwidth + s->bandwidth * s->old_bandwidth_val_l;
		s->old_bandwidth_val_l = tmp_a;

		/*
		kerLatticeAllPass(tmp_a, tmp_b, s->lat1_coeff, s->lat1_ptr_l, s->lat1_dly_start_l, s->lat1_dly_end_l);
		kerLatticeAllPass(tmp_b, tmp_a, s->lat1_coeff, s->lat2_ptr_l, s->lat2_dly_start_l, s->lat2_dly_end_l);
		kerLatticeAllPass(tmp_a, tmp_b, s->lat3_coeff, s->lat3_ptr_l, s->lat3_dly_start_l, s->lat3_dly_end_l);
		kerLatticeAllPass(tmp_b, input_diffuser_out, s->lat3_coeff, s->lat4_ptr_l, s->lat4_dly_start_l, s->lat4_dly_end_l);
		*/
		{ 
			float D_in, D_out;
			s->ptr += s->lat1_dly_len_l;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			D_out = next_out;
			D_in = tmp_a - s->lat1_coeff * D_out;
			next_out = *s->ptr;
			*s->ptr = D_in;
			tmp_b = D_out + s->lat1_coeff * D_in;
		}
		{ 
			float D_in, D_out;
			s->ptr += s->lat2_dly_len_l;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			D_out = next_out;
			D_in = tmp_b - s->lat1_coeff * D_out;
			next_out = *s->ptr;
			*s->ptr = D_in;
			tmp_a = D_out + s->lat1_coeff * D_in;
		}
		{
			float D_in, D_out;
			s->ptr += s->lat3_dly_len_l;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			D_out = next_out;
			D_in = tmp_a - s->lat3_coeff * D_out;
			next_out = *s->ptr;
			*s->ptr = D_in;
			tmp_b = D_out + s->lat3_coeff * D_in;
		}
		{ 
			float D_in, D_out;
			s->ptr += s->lat4_dly_len_l;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			D_out = next_out;
			D_in = tmp_b - s->lat3_coeff * D_out;
			/* next_out = *s->ptr; Not needed for next macro */
			*s->ptr = D_in;
			input_diffuser_out = D_out + s->lat3_coeff * D_in;
		}

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
		/* Increment Oscillator floating table pointers */
		s->osc_table_p += s->modulation_freq;
		
		/* Modulo first table pointer by number of entries.
		 * Note that with the optimizer on, registers can be used for test
		 * and be less but then be rounded up upon storage later,
		 * ie, register test value was 8191.999761, after test it got rounded up
		 * to 8192.0 . Current fix is to add one extra point to the end of the array,
		 * which the same value as the zeroth point, so if the values rounds up, the
		 * output is correct, and on next iteration, it will index to the correct spot.
		 */
		if(s->osc_table_p >= s->f_num_pts)
			s->osc_table_p -= s->f_num_pts;

		/* Assign osc output */
		{
			int table_p_int = (int)(s->osc_table_p);
			osc = s->osc_table_p * s->osc_mult_table[ table_p_int ] + s->osc_plus_table[ table_p_int ];
		}
#endif

		/* Lattice 5 */
		tmp_b = input_diffuser_out + s->decay * s->D4_out;

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
		delay_real = s->lat5_dly_len_l + osc * s->modulation_depth;
#endif

		/* kerLatticeDecayDiffuser(tmp_b, tmp_a, s->lat5_coeff, delay_real, s->lat5_ptr_l, s->lat5_dly_start_l, s->lat5_dly_end_l, s->lat5_dly_maxlen_l); */
		{
			float dl_in, dl_out;
			float *tmp_ptr;
			float y1;

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
			float y2;
			long idly = (long)delay_real;
			float del = delay_real - idly;
#else
			long idly = (long)s->lat5_dly_len_l;
#endif

			s->ptr += s->lat5_dly_maxlen_l;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			tmp_ptr = s->ptr - idly;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			y1 = *(tmp_ptr);


#if (PT_DSP_BUILD == PT_DSP_DSPFX)
			idly++;
			tmp_ptr = s->ptr - idly;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			y2 = *(tmp_ptr);
			dl_out = (y1 + (y2 - y1) * del);
#else
			dl_out = y1;
#endif

			dl_in = tmp_b + s->lat5_coeff * dl_out;
			next_out = *s->ptr;
			*s->ptr = dl_in;
			tmp_a = dl_out - s->lat5_coeff * dl_in;
		}

		/* kerDelay4Taps(tmp_a, s->D1_tap1, tap1_out, s->D1_tap2, tap2_out, s->D1_tap3, tap3_out, s->D1_tap4, tap4_out, s->D1_ptr_l, s->D1_dly_start_l, s->D1_dly_end_l); */
		{ 
			float *tmp_ptr;
			s->ptr += s->D1_tap4;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			tap4_out = next_out;
			next_out = *s->ptr;
			*s->ptr = tmp_a;
			tmp_ptr = s->ptr - s->D1_tap1;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap1_out = *tmp_ptr;
			tmp_ptr = s->ptr - s->D1_tap2;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap2_out = *tmp_ptr;
			tmp_ptr = s->ptr - s->D1_tap3;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap3_out = *tmp_ptr;
		}
		out1 = - tap2_out;
		out2 = tap1_out + tap3_out;

		/* One pole lowpass, can collapse in mult into final gain. */
		tmp_a = tap4_out * s->one_minus_damping + s->damping * s->old_damp_val1_l;
		s->old_damp_val1_l = tmp_a;

		tmp_a *= s->decay;
		/* Lattice 6 */
		/* kerLatticeAllPassTapped(tmp_a, tmp_b, s->lat6_coeff, s->lat6_tap1, tap1_out, s->lat6_tap2, tap2_out, s->lat6_ptr_l, s->lat6_dly_start_l, s->lat6_dly_end_l, s->lat6_dly_len_l); */
		{
			float Dl_in, Dl_out;
			float *tmp_ptr;
			s->ptr += s->lat6_dly_len_l;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			Dl_out = next_out;
			Dl_in = tmp_a - s->lat6_coeff * Dl_out;
			next_out = *s->ptr;
			*s->ptr = Dl_in;
			tmp_b = Dl_out + s->lat6_coeff * Dl_in;
			tmp_ptr = s->ptr - s->lat6_tap1;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap1_out = *tmp_ptr;
			tmp_ptr = s->ptr - s->lat6_tap2;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap2_out = *tmp_ptr;
		}
		out1 -= tap1_out;
		out2 -= tap2_out;

		/* kerDelay3Taps(tmp_b, s->D2_tap1, tap1_out, s->D2_tap2, tap2_out, s->D2_tap3, tap3_out, s->D2_ptr_l, s->D2_dly_start_l, s->D2_dly_end_l); */
		{ 
			float *tmp_ptr;
			s->ptr += s->D2_tap3;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			tap3_out = next_out;
			/* next_out = *s->ptr; Not needed for next section */
			*s->ptr = tmp_b;
			tmp_ptr = s->ptr - s->D2_tap1;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap1_out = *tmp_ptr;
			tmp_ptr = s->ptr - s->D2_tap2;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap2_out = *tmp_ptr;
		}
		out1 -= tap1_out;
		out2 += tap2_out;

		/* Lattice 7 */
		tmp_b = input_diffuser_out + s->decay * tap3_out;

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
		delay_real = s->lat7_dly_len_l - osc * s->modulation_depth;
#endif

		/* kerLatticeDecayDiffuser(tmp_b, tmp_a, s->lat5_coeff, delay_real, s->lat7_ptr_l, s->lat7_dly_start_l, s->lat7_dly_end_l, s->lat7_dly_maxlen_l); */
		{
			float dl_in, dl_out;
			float *tmp_ptr;
			float y1;

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
			float y2;
			long idly = (long)delay_real;
			float del = delay_real - idly;
#else
			long idly = (long)s->lat7_dly_len_l;
#endif

			s->ptr += s->lat7_dly_maxlen_l;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			tmp_ptr = s->ptr - idly;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			y1 = *(tmp_ptr);

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
			idly++;
			tmp_ptr = s->ptr - idly;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			y2 = *(tmp_ptr);
			dl_out = (y1 + (y2 - y1) * del);
#else
			dl_out = y1;
#endif

			dl_in = tmp_b + s->lat5_coeff * dl_out;
			next_out = *s->ptr;
			*s->ptr = dl_in;
			tmp_a = dl_out - s->lat5_coeff * dl_in;
		}

		/* kerDelay4Taps(tmp_a, s->D3_tap1, tap1_out, s->D3_tap2, tap2_out, s->D3_tap3, tap3_out, s->D3_tap4, tap4_out, s->D3_ptr_l, s->D3_dly_start_l, s->D3_dly_end_l); */
		{
			float *tmp_ptr;
			s->ptr += (long)s->D3_tap4;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			tap4_out = next_out;
			next_out = *s->ptr;
			*s->ptr = tmp_a;
			tmp_ptr = s->ptr - s->D3_tap1;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap1_out = *tmp_ptr;
			tmp_ptr = s->ptr - s->D3_tap2;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap2_out = *tmp_ptr;
			tmp_ptr = s->ptr - s->D3_tap3;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap3_out = *tmp_ptr;
		}
		out1 += tap1_out + tap3_out;
		out2 -= tap2_out;

		/* One pole lowpass, can collapse in mult into final gain. */
		tmp_a = tap4_out * s->one_minus_damping + s->damping * s->old_damp_val2_l;
		s->old_damp_val2_l = tmp_a;

		tmp_a *= s->decay;
		/* Lattice 8 */
		/* kerLatticeAllPassTapped(tmp_a, tmp_b, s->lat6_coeff, s->lat8_tap1, tap1_out, s->lat8_tap2, tap2_out, s->lat8_ptr_l, s->lat8_dly_start_l, s->lat8_dly_end_l, s->lat8_dly_len_l); */
		{
			float Dl_in, Dl_out;
			float *tmp_ptr;
			s->ptr += s->lat8_dly_len_l;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			Dl_out = next_out;
			Dl_in = tmp_a - s->lat6_coeff * Dl_out;
			next_out = *s->ptr;
			*s->ptr = Dl_in;
			tmp_b = Dl_out + s->lat6_coeff * Dl_in;
			tmp_ptr = s->ptr - s->lat8_tap1;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap1_out = *tmp_ptr;
			tmp_ptr = s->ptr - s->lat8_tap2;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap2_out = *tmp_ptr;
		}
		out1 -= tap2_out;
		out2 -= tap1_out;

		/* kerDelay3Taps(tmp_b, s->D4_tap1, tap1_out, s->D4_tap2, tap2_out, s->D4_tap3, tap3_out, s->D4_ptr_l, s->D4_dly_start_l, s->D4_dly_end_l); */
		{ 
			float *tmp_ptr;
			s->ptr += (long)s->D4_tap3;
			if(s->ptr > s->MasterEnd)
				s->ptr -= s->MasterLen;
			tap3_out = next_out;
			/* next_out = *s->ptr; Not needed for next section */
			*s->ptr = tmp_b;
			tmp_ptr = s->ptr - s->D4_tap1;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap1_out = *tmp_ptr;
			tmp_ptr = s->ptr - s->D4_tap2;
			if( (long)tmp_ptr < (long)s->MasterStart )
				tmp_ptr += s->MasterLen;
			tap2_out = *tmp_ptr;
		}
		out1 += tap2_out;
		out2 -= tap1_out;
		s->D4_out = tap3_out;

		out1 *= (realtype)(0.6 * 0.5);
		out2 *= (realtype)(0.6 * 0.5);

		if( !(s->stereo_in_flag) )
		{
			in2 = (realtype)0.0;
			out1 *= (realtype)0.5;
			out2 *= (realtype)0.5;
		}
		kerWetDry(in1, in2, &(s->wet_gain), &(s->dry_gain), out1, out2);

		dutilSetClipStatus(in1, in2, out1, out2, status);

		dutilPutOutputsAndMeter(out1, out2, status);

		write_meters_and_status(in_meter1, in_meter2, out_meter1, out_meter2, status, transfer_state);
	}

	/* Write averaged meter data temporarily writing as a float.
	 * Will be converted to long and factored in calling function.
	 */
	write_meter_average();

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
