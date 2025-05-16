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
 * FILE: auralX.c
 * DATE: 7/16/98
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: Aural Enhancer
 *
 */

#ifndef DSPSOFT_TARGET
#define DSP_TARGET
#endif
#include "Aur_num.h"  /* Sets number of elements */

/* Standard includes */
#include <math.h>
#include <stdio.h>
#include "codedefs.h" /* Company common defines */
#include "platform.h" /* Targets a specific DSP card       */
#include "pt_defs.h"  /* Defines common to both PC and DSP */	
#include "product_type.h" /* Sets DSPFX or DFX */
#include "boardrv1.h" /* DSP hardware specific defines, for both PC and DSP */
#include "filt.h"		 /* For filter structure and run time macros */

/* #define DSP_PROGRAM_SIZE 0x400 */ /* Overide default program size */
#ifdef DSP_TARGET
#include "dma32.h"
#endif

#include "dsp_mem1.h" /* For DSP memory configuration (typical delay version) */
#include "c_dsps.h"
#include "c_aural.h"   /* For specific parameter mappings */
#include "dutio.h"	  /* A/D I/O macros */
#include "dutcom.h"	  /* PC to DSP and DSP to PC com routines */
#include "dutinit.h"  /* DSP and AD initialization */
#include "kerdma.h"
#include "kerdelay.h"  /* Has wet-dry macro */

static void DSPS_AURAL_RUN(void);

#ifdef DSP_TARGET
void main(int argc, char *argv[])
{
	DSPS_AURAL_INIT();
	while(1)
		DSPS_AURAL_RUN();
}
#endif

/* Only build init function in 32 bit files (same for both) */
#if defined(DSPSOFT_32_BIT)
DSP_FUNC_DEF int DSPS_AURAL_INIT(float *fp_params, float *fp_memory, long l_memsize, float *fp_state, int i_init_flag, float r_samp_freq)
{
	float *COMM_MEM_OFFSET = fp_params;
	float *MEMBANK0_START = fp_memory;

	/* Variables below must be restored from the state array and
	 * stored back at end of buffer processing
	 */
	/* For this plug-in, all state vars are stored in param memory area */

	/* NOTE- Did some comparisons between using the shared parameter
	 * memory area to hold the 2nd order filter states versus putting
	 * them in an taking them out of the state memory area. As long
	 * as a single structure pointer is used to reference both the
	 * states and the coeffs, the memory areas don't make any difference.
	 * However, using a single state pointer to reference both coeffs and
	 * state vals versus a structure for the coeffs and a separate structure
	 * for the states (or just locals) makes a difference.
	 *
	 * 2 instances of enhancer, Intel release compile.
    *		One state struct (in state memory), one param struct = 13.5 %
    *		Local individual state vars (copied from state memory), one param struct = 13.5 %
	 *		Single struct * for all, in parameter memory = 11.5 %.
	 * 
	 * Moral is that for future plug-ins, use a single struct* to 
	 * reference both params and state values if possible.
	 * Should investigate combining current separate state and param
	 * shared memory, which each processor has a dedicated region for,
	 * into a single shared memory area for both state and params.
	 */

	struct dspAuralStructType *s = (struct dspAuralStructType *)(COMM_MEM_OFFSET);
 
	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		/* Initialization values from Quick preset 1, 44khz */
		s->dry_gain = (realtype)0.0;	  
		s->wet_gain = (realtype)0.0;
		s->aural_drive = (realtype)1.76993;
		s->aural_odd = (realtype)1.5;
		s->aural_even = (realtype)0.0;
		/* Next 3 are samp_freq dependent, need to be reset for other than 44.1kHz */
		s->gain = (realtype)0.788950;
		s->a1 = (realtype)1.53285;
		s->a0 = (realtype)-0.622949;

 	}

	/* Zero internal signal memory spaces to eliminates glitches on restarts */
	if( (i_init_flag & DSPS_INIT_PARAMS) || (i_init_flag & DSPS_ZERO_MEMORY) )
	{
		/* Internal filter states */
		s->out1_minus1 = 0.0;
		s->out1_minus2 = 0.0;
		s->in1_minus1 = 0.0;
		s->in1_minus2 = 0.0;
		s->out2_minus1 = 0.0;
		s->out2_minus2 = 0.0;
		s->in2_minus1 = 0.0;
		s->in2_minus2 = 0.0;
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

		/* Initialize memory pointers. Note main memsize is already
		 * compensated for 0x400 program size
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

	/* Initialize A/D and D/A converters, and set Samp Rate */
	/* Do as close as possible to running to minimize pops */
	dutilInitAIO();                                                            
	kerdmainit_MACRO();

	return(OKAY);
}
#endif /* DSPSOFT_32_BIT */

#ifdef DSPSOFT_TARGET
DSP_FUNC_DEF void DSPS_AURAL_PROCESS(long *lp_data, int l_length,
								   float *fp_params, float *fp_memory, float *fp_state,
								   struct hardwareMeterValType *sp_meters,
								   int DSP_data_type)
{
	float *COMM_MEM_OFFSET = fp_params;
	float *MEMBANK0_START = fp_memory;

	/* Variables below must be restored from the state array and
	 * stored back at end of buffer processing
	 */
	/* For this plug-in, all state vars are stored in param memory area */
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

	/* Run one buffer full of data. Zero index and averaged meter vals. */

	read_in_buf = lp_data;
	read_out_buf = lp_data;

	for(i=0; i<l_length; i++)
	{
		float out1, out2;
		float in1, in2;
		volatile long in_count = 0;
		volatile long out_count = 0;
		float odd1, odd2, even1, even2;
		float filtH1, filtH2;
		struct dspAuralStructType *s = (struct dspAuralStructType *)(COMM_MEM_OFFSET);
		realtype drive = s->aural_drive;
		realtype even = s->aural_even;
		realtype odd = s->aural_odd;
	  
		load_parameter(); /* If its been sent, loads a parameter into memory */

		/* Next line includes kerdmaru.h run loop macro (no args) */
		#include "kerdmaru.h"
	  
		out1 = out2 = 0.0;
		 
		dutilGetInputsAndMeter( in1, in2, status);

		/* Implement Highpass linear transformed 2nd order Butterworth filter */
		/* Note using Nil's method for allowing optimizer to work well, all
		 * parameters and internal states are in a structure that is accessed
		 * via a pointer.
		 */
		filtH1 = s->out1_minus1 * s->a1 + s->out1_minus2 * s->a0;
		s->out1_minus2 = s->out1_minus1;
		/* Add bias to avoid zero input cpu load increases */
		filtH1 += (in1 + (realtype)1.0e-30 - (realtype)2.0 * s->in1_minus1 + s->in1_minus2) * s->gain;
		s->out1_minus1 = filtH1;
		s->in1_minus2 = s->in1_minus1;
		s->in1_minus1 = in1;

		filtH1 *= drive;

		odd1 = (realtype)sin(filtH1);
		/* Note that this approximation works extremely well for this usage if
		 * the input does not exceed 1.0
		odd1 = filtH1 - (filtH1 * filtH1 * filtH1) * (realtype)(1.0/6.0);
		 */

		if( filtH1 > 0.0 )
			even1 = filtH1;
		else
			even1 = (realtype)0.0;
			/* even1 = odd1; Too gentle */

		out1 = in1 + (even * even1 + odd * odd1);

		/* Do right channel if stereo in */
		if(s->stereo_in_flag)
		{
			/* Implement a second order lowpass filter on the signal */
			filtH2 = s->out2_minus1 * s->a1 + s->out2_minus2 * s->a0;
			s->out2_minus2 = s->out2_minus1;
			/* Add bias to avoid zero input cpu load increases */
			filtH2 += (in2  + (realtype)1.0e-30 - (realtype)2.0 * s->in2_minus1 + s->in2_minus2) * s->gain;
			s->out2_minus1 = filtH2;
			s->in2_minus2 = s->in2_minus1;
			s->in2_minus1 = in2;
				
			filtH2 *= drive;

			/* Generate the harmonics */
			odd2 = (realtype)sin(filtH2);
			/* Note that this approximation works extremely well for this usage
			 * if the input does not exceed 1.0
			odd2 = filtH2 - (filtH2 * filtH2 * filtH2) * (realtype)(1.0/6.0);
			 */

			if( filtH2 > 0.0 )
				even2 = filtH2;
			else
				even2 = (realtype)0.0;
				/* even2 = odd2; Too gentle */

			out2 = in2 + (even * even2 + odd * odd2);
		}
		else
			/* Zero right input in mono case so bypass works right */
			in2 = (realtype)0.0;
							
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
	/* For this plug-in, all state vars are stored in param memory area */
	{	/*
		float **fpp = (float **)&(fp_state[0]);
		long *lpp  =  (long *)&(fp_state[0]);
		fpp[0] = ptr0;
		*/
	}
} 
#endif
