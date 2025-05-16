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
 * FILE: Wide32.c 
 * DATE: 4/24/99
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: Stereo Widener plug-in
 *
 */

#ifndef DSPSOFT_TARGET
#define DSP_TARGET
#endif
#include "Wide_num.h"  /* Sets number of elements */

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
#include "c_wid.h"   /* For specific parameter mappings */
#include "dutio.h"	  /* A/D I/O macros */
#include "dutcom.h"	  /* PC to DSP and DSP to PC com routines */
#include "dutinit.h"  /* DSP and AD initialization */
#include "kerdma.h"
#include "kerdelay.h"  /* Has wet-dry macro */

static void DSPS_WIDE_RUN(void);

#ifdef DSP_TARGET
void main(int argc, char *argv[])
{
	DSPS_WIDE_INIT();
	while(1)
		DSPS_WIDE_RUN();
}
#endif

/* Only build init function in 32 bit files (same for both) */
#if defined(DSPSOFT_32_BIT)
DSP_FUNC_DEF int DSPS_WIDE_INIT(float *fp_params, float *fp_memory, long l_memsize, float *fp_state, int i_init_flag, float r_samp_freq)
{
	float *COMM_MEM_OFFSET = fp_params;
	float *MEMBANK0_START = fp_memory;

	/* Variables below must be restored from the state array and
	 * stored back at end of buffer processing
	 */

	struct dspWideStructType *s = (struct dspWideStructType *)(COMM_MEM_OFFSET);

	s->dsp_sampling_freq = r_samp_freq;

	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		/* Starting Presets - at 44.1 hHz.
		 * Instensity - 35
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

	if( i_init_flag & DSPS_INIT_MEMORY )
	{
		/* Note that any state vars must be initialized in this section
		 * since they must be present in the dll instance that will
		 * call the processing functions.
		 * IS THIS STILL TRUE?
		 */

 		/* Get mem sizes from PC, zero memory */
		dutilGetMemSizes();
		dutilInitMem(); /* Doesn't do anything for DSPSOFT */

		/* Initialize memory pointers. Note main memsize is already
		 * compensated for 0x400 program size
		 */
		s->dly_start_l = fp_memory; 
		s->dly_start_r = s->dly_start_l + (long)(DSPS_SOFT_MEM_WIDE_LENGTH/3);
		s->dly_start_mono = s->dly_start_r + (long)(DSPS_SOFT_MEM_WIDE_LENGTH/3);

		s->ptr_l = s->dly_start_l;
		s->ptr_r = s->dly_start_r;
		s->ptr_mono = s->dly_start_mono;
	}

	/* Zero internal signal memory spaces to eliminates glitches on restarts */
	if( (i_init_flag & DSPS_INIT_MEMORY) || (i_init_flag & DSPS_ZERO_MEMORY) )
	{
		int i;
		/* Now do DSPSOFT init */
		for(i=0; i<(DSPS_SOFT_MEM_WIDE_LENGTH); i++)
			fp_memory[i] = 0.0;

		s->out1_minus1 = 0.0;
		s->out1_minus2 = 0.0;
		s->in1_minus1 = 0.0;
		s->in1_minus2 = 0.0;
		s->out2_minus1 = 0.0;
		s->out2_minus2 = 0.0;
		s->in2_minus1 = 0.0;
		s->in2_minus2 = 0.0;
	}

	/* Initialize A/D and D/A converters, and set Samp Rate */
	/* Do as close as possible to running to minimize pops */
	dutilInitAIO();                                                            
	kerdmainit_MACRO();

	return(OKAY);
}
#endif /* DSPSOFT_32_BIT */

#ifdef DSPSOFT_TARGET
DSP_FUNC_DEF void DSPS_WIDE_PROCESS(long *lp_data, int l_length,
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
		volatile long in_count = 0;
		volatile long out_count = 0;
		float dly_l_out, dly_r_out, dly_mono;

		struct dspWideStructType *s = (struct dspWideStructType *)(COMM_MEM_OFFSET);
		float filtH1, filtH2;
		float mono_sig, l_minus_mono, r_minus_mono;
		
	  	load_parameter(); /* If its been sent, loads a parameter into memory */

		/* Next line includes kerdmaru.h run loop macro (no args) */
		#include "kerdmaru.h"

	  	out1 = out2 = (realtype)0.0;
		 
		dutilGetInputsAndMeter( in1, in2, status);

		mono_sig = (in1 + in2) * (realtype)0.5;
		l_minus_mono = in1 - mono_sig;
		r_minus_mono = in2 - mono_sig;

		/* Implement Highpass linear transformed 2nd order Butterworth filter */
		/* Note using Nil's method for allowing optimizer to work well, all
		 * parameters and internal states are in a structure that is accessed
		 * via a pointer.
		 */
		{
			filtH1 = s->out1_minus1 * s->a1 + s->out1_minus2 * s->a0;
			s->out1_minus2 = s->out1_minus1;
			/* Add bias to avoid zero input cpu load increases */
			filtH1 += (l_minus_mono + (realtype)1.0e-30 - (realtype)2.0 * s->in1_minus1 + s->in1_minus2) * s->gain;
			s->out1_minus1 = filtH1;
			s->in1_minus2 = s->in1_minus1;
			s->in1_minus1 = l_minus_mono;

			filtH2 = s->out2_minus1 * s->a1 + s->out2_minus2 * s->a0;
			s->out2_minus2 = s->out2_minus1;
			/* Add bias to avoid zero input cpu load increases */
			filtH2 += (r_minus_mono  + (realtype)1.0e-30 - (realtype)2.0 * s->in2_minus1 + s->in2_minus2) * s->gain;
			s->out2_minus1 = filtH2;
			s->in2_minus2 = s->in2_minus1;
			s->in2_minus1 = r_minus_mono;
		}

		dly_l_out = *(s->ptr_l);
		*(s->ptr_l) = filtH1;
		(s->ptr_l)++;
		if( s->ptr_l >= (s->dly_start_l + s->dispersion_l) )
			s->ptr_l = s->dly_start_l;

		dly_r_out = *(s->ptr_r);
		*(s->ptr_r) = filtH2;
		(s->ptr_r)++;
		if( s->ptr_r >= (s->dly_start_r + s->dispersion_r) )
			s->ptr_r = s->dly_start_r;

		/* Skip this section if there is no delay to the mono signal */
		if( s->center_depth > 1 )
		{
			dly_mono = *(s->ptr_mono);
			*(s->ptr_mono) = mono_sig;
			(s->ptr_mono)++;
			if( s->ptr_mono >= (s->dly_start_mono + s->center_depth) )
				s->ptr_mono = s->dly_start_mono;
		}
		else
			dly_mono = mono_sig;

		dly_mono *= s->center_gain;

		out1 = l_minus_mono + dly_mono - (realtype)5.0 * s->intensity * (s->width * dly_r_out + s->reverse_width * dly_l_out);
		out1 *= s->master_gain;
		out2 = r_minus_mono + dly_mono - (realtype)5.0 * s->intensity * (s->width * dly_l_out + s->reverse_width * dly_r_out);
		out2 *= s->master_gain;

		if( !(s->stereo_in_flag) )
		{
			in2 = (realtype)0.0;
			out1 *= (realtype)0.5;
			out2 *= (realtype)0.5;
		}

		if( s->bypass_flag )
		{
			out1 = in1;
			out2 = in2;
		}

		dutilSetClipStatus(in1, in2, out1, out2, status);

		dutilPutOutputsAndMeter(out1, out2, status);

		write_meters_and_status(in_meter1, in_meter2, out_meter1, out_meter2, status, transfer_state);
	}

	/* Write averaged meter data temporarily writing as a float.
	 * Will be converted to long and factored in calling function.
	 */
	write_meter_average();

	/*
	{
		sp_meters->aux_vals[0] = (realtype)WIDE_PLOT_PT_UNDETECTABLE;
		sp_meters->aux_vals[1] = (realtype)WIDE_PLOT_PT_UNDETECTABLE;
	}
	*/

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
