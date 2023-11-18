/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
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
#include "boardrv1.h" /* DSP hardware specific defines, for both PC and DSP */

/* #define DSP_PROGRAM_SIZE 0x400 */ /* Overide default program size */
#ifdef DSP_TARGET
#include "dma32.h"
#endif

#include "dsp_mem1.h" /* For DSP memory configuration (typical delay version) */
#include "c_dsps.h"
#include "c_Lex.h"   /* For specific parameter mappings */
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
		s->wet_gain = (realtype)0.0;	  
		s->dry_gain = (realtype)0.0;

		s->lat1_coeff = (realtype)0.75;
		s->lat3_coeff = (realtype)0.625;
		s->lat5_coeff = (realtype)0.70;
		s->lat6_coeff = (realtype)0.5;
		s->damping = (realtype)0.5;
		s->one_minus_damping = (realtype)0.5;
		s->old_damp_val1_l = (realtype)0.0;
		s->old_damp_val2_l = (realtype)0.0;
		s->bandwidth = (realtype)0.5;
		s->one_minus_bandwidth = (realtype)0.5;
		s->roomsize = (realtype)1.0;

		s->D4_out = (realtype)0.0;
 	}

	if( i_init_flag & DSPS_INIT_MEMORY )
	{
		/* Initialize oscillator interpolation table values. Note
		 * that normally that would be done in the init mem section below,
		 * but in the reverb that gets called every time the room size is changed,
		 * so it was moved up here with the parameters.
		 */
		s->osc_mult_table = fp_memory;
		s->osc_plus_table = s->osc_mult_table + LEX_NUM_OSC_PTS;
		s->osc_table_p = (LEX_OSC_PHASE * LEX_NUM_OSC_PTS)/360.0;                    
		s->f_num_pts = LEX_NUM_OSC_PTS;

		/* Use First (zero) point of sine up to the very last point before zero, repeating zero */ 
		{
			float x1, x2, y1, y2;                             
 		  
			for(i=0; i<(LEX_NUM_OSC_PTS); i++)
			{
				x1 = (float)i;
				x2 = (float)(i + 1);
				y1 = (float)cos(TWO_PI * x1/(LEX_NUM_OSC_PTS));
				y2 = (float)cos(TWO_PI * x2/(LEX_NUM_OSC_PTS));
				s->osc_mult_table[i] = (y1 - y2); 
				s->osc_plus_table[i] = ((x1 * y2) - (x2 * y1)); 
			}
		}
		
		/* Note that any state vars must be initialized in this section
		 * since they must be present in the dll instance that will
		 * call the processing functions.
		 * IS THIS STILL TRUE?
		 */
 		/* Get mem sizes from PC, zero memory */
		dutilGetMemSizes();
		dutilInitMem(); /* Doesn't do anything for DSPSOFT */

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

	/* Run one buffer full of data. Zero index and averaged meter vals. */

	read_in_buf = lp_data;
	read_out_buf = lp_data;

	for(i=0; i<l_length; i++)
	{
		float out1, out2;
		float in1, in2;
		float tmp_a, tmp_b;
		float tap1_out, tap2_out, tap3_out, tap4_out;
		float input_diffuser_out;
		float osc;
		float delay_real;
		volatile long in_count = 0;
		volatile long out_count = 0;
		float *ptr;
		float *MasterStart;
		float *MasterEnd;
		unsigned long MasterLen;
		float next_out;

	  	load_parameter(); /* If its been sent, loads a parameter into memory */

		/* Next line includes kerdmaru.h run loop macro (no args) */
		#include "kerdmaru.h"

	  	out1 = out2 = 0.0;
		 
		dutilGetInputsAndMeter( in1, in2, status);

		dutilMuteInputs(in1, in2);

		/* kerRunDelayLineNoPop((in1 + in2), tmp_b, s->pre_delay, s->pre_dly_start_l, s->pre_dly_end_l, s->pre_ptr_l, s->pre_dly_len_l); */
		ptr = s->ptr;
		MasterStart = s->MasterStart;
		MasterEnd = s->MasterEnd;
		MasterLen = s->MasterLen;
		/* Note that since this implements a delay, it doesn't need the next_out value.
		 * next_out supplies the oldest value of the next delay line in the sequence.
		 */
		{
			float *rp_MACRO;
			/* Note- since this is the first pointer increment, need to add 1 */
			ptr += s->pre_dly_len_l + 1;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
			rp_MACRO = (float *)(ptr - s->pre_delay );
			if((long)rp_MACRO < (long)MasterStart)
				rp_MACRO += MasterLen;
			tmp_b = *rp_MACRO;
			next_out = *ptr;
			*ptr = in1 + in2;
		}

		/* One pole bandwidth lowpass. */
		tmp_a = tmp_b * s->one_minus_bandwidth + s->bandwidth * s->old_bandwidth_val_l;
		s->old_bandwidth_val_l = tmp_a;

#define LAT1
#define LAT2
#define LAT3
#define LAT4
#define LAT5
#define D1
#define LAT6
#define D2
#define LAT7
#define D3
#define LAT8
#define D4
		/*
		kerLatticeAllPass(tmp_a, tmp_b, s->lat1_coeff, s->lat1_ptr_l, s->lat1_dly_start_l, s->lat1_dly_end_l);
		kerLatticeAllPass(tmp_b, tmp_a, s->lat1_coeff, s->lat2_ptr_l, s->lat2_dly_start_l, s->lat2_dly_end_l);
		kerLatticeAllPass(tmp_a, tmp_b, s->lat3_coeff, s->lat3_ptr_l, s->lat3_dly_start_l, s->lat3_dly_end_l);
		kerLatticeAllPass(tmp_b, input_diffuser_out, s->lat3_coeff, s->lat4_ptr_l, s->lat4_dly_start_l, s->lat4_dly_end_l);
		*/
		{ 
			float D_in, D_out;
			ptr += s->lat1_dly_len_l;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
#ifdef LAT1
			D_out = next_out;
			D_in = tmp_a - s->lat1_coeff * D_out;
			next_out = *ptr;
			*ptr = D_in;
			tmp_b = D_out + s->lat1_coeff * D_in;
#endif
		}
		{ 
			float D_in, D_out;
			ptr += s->lat2_dly_len_l;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
#ifdef LAT2
			D_out = next_out;
			D_in = tmp_b - s->lat1_coeff * D_out;
			next_out = *ptr;
			*ptr = D_in;
			tmp_a = D_out + s->lat1_coeff * D_in;
#endif
		}
		{
			float D_in, D_out;
			ptr += s->lat3_dly_len_l;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
#ifdef LAT3
			D_out = next_out;
			D_in = tmp_a - s->lat3_coeff * D_out;
			next_out = *ptr;
			*ptr = D_in;
			tmp_b = D_out + s->lat3_coeff * D_in;
#endif
		}
		{ 
			float D_in, D_out;
			ptr += s->lat4_dly_len_l;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
#ifdef LAT4
			D_out = next_out;
			D_in = tmp_b - s->lat3_coeff * D_out;
			/* next_out = *ptr; Not needed for next macro */
			*ptr = D_in;
			input_diffuser_out = D_out + s->lat3_coeff * D_in;
#endif
		}

		/* Increment Oscillator floating table pointers */
		s->osc_table_p += s->modulation_freq;
		
		/* Modulo first table pointer by number of entries */
		if(s->osc_table_p >= s->f_num_pts)
		   s->osc_table_p -= s->f_num_pts;
		/* Assign osc output */
		osc = s->osc_table_p * s->osc_mult_table[ (int)(s->osc_table_p) ] + s->osc_plus_table[ (int)(s->osc_table_p) ];

		/* Lattice 5 */
		tmp_b = input_diffuser_out + s->decay * s->D4_out;
		delay_real = s->lat5_dly_len_l + osc * s->modulation_depth;
		/* kerLatticeDecayDiffuser(tmp_b, tmp_a, s->lat5_coeff, delay_real, s->lat5_ptr_l, s->lat5_dly_start_l, s->lat5_dly_end_l, s->lat5_dly_maxlen_l); */
		{
			float dl_in, dl_out;
			float *tmp_ptr;
			float y1, y2;
			long idly = (long)delay_real;
			float del = delay_real - idly;
			ptr += s->lat5_dly_maxlen_l;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
#ifdef LAT5
			tmp_ptr = ptr - idly;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			y1 = *(tmp_ptr);
			idly++;
			tmp_ptr = ptr - idly;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			y2 = *(tmp_ptr);
			dl_out = (y1 + (y2 - y1) * del);
			dl_in = tmp_b + s->lat5_coeff * dl_out;
			next_out = *ptr;
			*ptr = dl_in;
			tmp_a = dl_out - s->lat5_coeff * dl_in;
#endif
		}

		/* kerDelay4Taps(tmp_a, s->D1_tap1, tap1_out, s->D1_tap2, tap2_out, s->D1_tap3, tap3_out, s->D1_tap4, tap4_out, s->D1_ptr_l, s->D1_dly_start_l, s->D1_dly_end_l); */
		{ 
			float *tmp_ptr;
			ptr += s->D1_tap4;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
#ifdef D1
			tap4_out = next_out;
			next_out = *ptr;
			*ptr = tmp_a;
			tmp_ptr = ptr - s->D1_tap1;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap1_out = *tmp_ptr;
			tmp_ptr = ptr - s->D1_tap2;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap2_out = *tmp_ptr;
			tmp_ptr = ptr - s->D1_tap3;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap3_out = *tmp_ptr;
#endif
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
			ptr += s->lat6_dly_len_l;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
#ifdef LAT6
			Dl_out = next_out;
			Dl_in = tmp_a - s->lat6_coeff * Dl_out;
			next_out = *ptr;
			*ptr = Dl_in;
			tmp_b = Dl_out + s->lat6_coeff * Dl_in;
			tmp_ptr = ptr - s->lat6_tap1;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap1_out = *tmp_ptr;
			tmp_ptr = ptr - s->lat6_tap2;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap2_out = *tmp_ptr;
#endif
		}
		out1 -= tap1_out;
		out2 -= tap2_out;

		/* kerDelay3Taps(tmp_b, s->D2_tap1, tap1_out, s->D2_tap2, tap2_out, s->D2_tap3, tap3_out, s->D2_ptr_l, s->D2_dly_start_l, s->D2_dly_end_l); */
		{ 
			float *tmp_ptr;
			ptr += s->D2_tap3;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
#ifdef D2
			tap3_out = next_out;
			/* next_out = *ptr; Not needed for next section */
			*ptr = tmp_b;
			tmp_ptr = ptr - s->D2_tap1;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap1_out = *tmp_ptr;
			tmp_ptr = ptr - s->D2_tap2;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap2_out = *tmp_ptr;
#endif
		}
		out1 -= tap1_out;
		out2 += tap2_out;

		/* Lattice 7 */
		tmp_b = input_diffuser_out + s->decay * tap3_out;
		delay_real = s->lat7_dly_len_l - osc * s->modulation_depth;
		/* kerLatticeDecayDiffuser(tmp_b, tmp_a, s->lat5_coeff, delay_real, s->lat7_ptr_l, s->lat7_dly_start_l, s->lat7_dly_end_l, s->lat7_dly_maxlen_l); */
		{
			float dl_in, dl_out;
			float *tmp_ptr;
			float y1, y2;
			long idly = (long)delay_real;
			float del = delay_real - idly;
			ptr += s->lat7_dly_maxlen_l;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
#ifdef LAT7
			tmp_ptr = ptr - idly;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			y1 = *(tmp_ptr);
			idly++;
			tmp_ptr = ptr - idly;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			y2 = *(tmp_ptr);
			dl_out = (y1 + (y2 - y1) * del);
			dl_in = tmp_b + s->lat5_coeff * dl_out;
			next_out = *ptr;
			*ptr = dl_in;
			tmp_a = dl_out - s->lat5_coeff * dl_in;
#endif
		}

		/* kerDelay4Taps(tmp_a, s->D3_tap1, tap1_out, s->D3_tap2, tap2_out, s->D3_tap3, tap3_out, s->D3_tap4, tap4_out, s->D3_ptr_l, s->D3_dly_start_l, s->D3_dly_end_l); */
		{
			float *tmp_ptr;
			ptr += (long)s->D3_tap4;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
#ifdef D3
			tap4_out = next_out;
			next_out = *ptr;
			*ptr = tmp_a;
			tmp_ptr = ptr - s->D3_tap1;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap1_out = *tmp_ptr;
			tmp_ptr = ptr - s->D3_tap2;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap2_out = *tmp_ptr;
			tmp_ptr = ptr - s->D3_tap3;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap3_out = *tmp_ptr;
#endif
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
			ptr += s->lat8_dly_len_l;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
#ifdef LAT8
			Dl_out = next_out;
			Dl_in = tmp_a - s->lat6_coeff * Dl_out;
			next_out = *ptr;
			*ptr = Dl_in;
			tmp_b = Dl_out + s->lat6_coeff * Dl_in;
			tmp_ptr = ptr - s->lat8_tap1;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap1_out = *tmp_ptr;
			tmp_ptr = ptr - s->lat8_tap2;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap2_out = *tmp_ptr;
#endif
		}
		out1 -= tap2_out;
		out2 -= tap1_out;

		/* kerDelay3Taps(tmp_b, s->D4_tap1, tap1_out, s->D4_tap2, tap2_out, s->D4_tap3, tap3_out, s->D4_ptr_l, s->D4_dly_start_l, s->D4_dly_end_l); */
		{ 
			float *tmp_ptr;
			ptr += (long)s->D4_tap3;
			if(ptr > MasterEnd)
				ptr -= MasterLen;
#ifdef D4
			tap3_out = next_out;
			/* next_out = *ptr; Not needed for next section */
			*ptr = tmp_b;
			tmp_ptr = ptr - s->D4_tap1;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap1_out = *tmp_ptr;
			tmp_ptr = ptr - s->D4_tap2;
			if( tmp_ptr < MasterStart )
				tmp_ptr += MasterLen;
			tap2_out = *tmp_ptr;
#endif
		}
		s->ptr = ptr;
		out1 += tap2_out;
		out2 -= tap1_out;
		s->D4_out = tap3_out;

		out1 *= (realtype)(0.6 * 0.5);
		out2 *= (realtype)(0.6 * 0.5);

		/* out1 = out2 = hack; */

		kerWetDry(in1, in2, &(s->wet_gain), &(s->dry_gain), out1, out2);

		dutilSetClipStatus(in1, in2, out1, out2, status);

		dutilPutOutputsAndMeter(out1, out2, status);

		write_meters_and_status(in_meter1, in_meter2, out_meter1, out_meter2, status, transfer_state);
	}

	/* Write averaged meter data temporarily writing as a float.
	 * Will be converted to long and factored in calling function.
	 */
	*(float *)&(sp_meters->left_in) = in_meter1_dma; 
	*(float *)&(sp_meters->right_in) = in_meter2_dma; 
	*(float *)&(sp_meters->left_out) = out_meter1_dma; 
	*(float *)&(sp_meters->right_out) = out_meter2_dma; 
	sp_meters->dsp_status = status;
   sp_meters->values_are_new = 1;

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
