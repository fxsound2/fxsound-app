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
 * FILE: dlyX.c
 * DATE: 11/9/95 Modified - 2/21/97
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: Multi- Element Delay, num set by dly_num.h
 *
 */
#ifndef DSPSOFT_TARGET
#define DSP_TARGET
#endif
#include "dly_num.h"  /* Sets number of elements */

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
#include "c_dly1.h"   /* For specific parameter mappings */
#include "dutio.h"	  /* A/D I/O macros */
#include "dutcom.h"	  /* PC to DSP and DSP to PC com routines */
#include "dutinit.h"  /* DSP and AD initialization */
#include "kerdma.h"
#include "kerdelay.h" /* Delay Line Macros  */
#include "kercntl.h"  /* For filtered parameters */

/* PTHACK for prototyping. Structure is declared globally
 * so if file read fails last values will still be present.
 */
#ifdef DSP_READ_VALS
#include "c_ReadVals.h" /* PTHACK for prototyping */
struct ReadValsType ReadVals;
#endif

static void DSPS_DLY_RUN(void);

#ifdef DSP_TARGET
void main(int argc, char *argv[])
{
	DSPS_DLY_INIT();
	while(1)
		DSPS_DLY_RUN();
}
#endif

/* Only build init function in 32 bit files (same for both) */
#if defined(DSPSOFT_32_BIT)
DSP_FUNC_DEF int DSPS_DLY_INIT(float *fp_params, float *fp_memory, long l_memsize, float *fp_state, int i_init_flag, float r_samp_freq)
{
	float *COMM_MEM_OFFSET = fp_params;
	float *MEMBANK0_START = fp_memory;

	/* Variables below must be restored from the state array and
	 * stored back at end of buffer processing
	 */
	float *ptr0 = NULL;
	float *delay0_start = NULL;
	float *delay1_start = NULL;
	float *delay2_start = NULL;
	float *delay3_start = NULL;
	float *delay4_start = NULL;
	float *delay5_start = NULL;
	float *delay6_start = NULL;
	float *delay7_start = NULL;
	long line_len = 0;
	float old_delay[NUM_ELEMS];

	long i;
 
	#define FPVAL *(volatile float *)
	#define LPVAL *(volatile long *) 

	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		FPVAL(DSP_MUTE_IN_FLAG) = 0.0;
		FPVAL(WET_GAIN) = 0.0;	  
		FPVAL(DRY_GAIN) = 0.0;	  
 
		FPVAL(ELEM0_FEEDBACK) = 0.0;
		FPVAL(ELEM0_GAIN) = 0.0;
		FPVAL(ELEM0_PAN_GAIN_LEFT) = 0.0;
		FPVAL(ELEM0_PAN_GAIN_RIGHT) = 0.0;
		FPVAL(ELEM1_FEEDBACK) = 0.0;
		FPVAL(ELEM1_GAIN) = 0.0;
		FPVAL(ELEM1_PAN_GAIN_LEFT) = 0.0;
		FPVAL(ELEM1_PAN_GAIN_RIGHT) = 0.0;
		FPVAL(ELEM2_FEEDBACK) = 0.0;
		FPVAL(ELEM2_GAIN) = 0.0;
		FPVAL(ELEM2_PAN_GAIN_LEFT) = 0.0;
		FPVAL(ELEM2_PAN_GAIN_RIGHT) = 0.0;
		FPVAL(ELEM3_FEEDBACK) = 0.0;
		FPVAL(ELEM3_GAIN) = 0.0;
		FPVAL(ELEM3_PAN_GAIN_LEFT) = 0.0;
		FPVAL(ELEM3_PAN_GAIN_RIGHT) = 0.0;
		FPVAL(ELEM4_FEEDBACK) = 0.0;
		FPVAL(ELEM4_GAIN) = 0.0;
		FPVAL(ELEM4_PAN_GAIN_LEFT) = 0.0;
		FPVAL(ELEM4_PAN_GAIN_RIGHT) = 0.0;
		FPVAL(ELEM5_FEEDBACK) = 0.0;
		FPVAL(ELEM5_GAIN) = 0.0;
		FPVAL(ELEM5_PAN_GAIN_LEFT) = 0.0;
		FPVAL(ELEM5_PAN_GAIN_RIGHT) = 0.0;
		FPVAL(ELEM6_FEEDBACK) = 0.0;
		FPVAL(ELEM6_GAIN) = 0.0;
		FPVAL(ELEM6_PAN_GAIN_LEFT) = 0.0;
		FPVAL(ELEM6_PAN_GAIN_RIGHT) = 0.0;
		FPVAL(ELEM7_FEEDBACK) = 0.0;
		FPVAL(ELEM7_GAIN) = 0.0;
		FPVAL(ELEM7_PAN_GAIN_LEFT) = 0.0;
		FPVAL(ELEM7_PAN_GAIN_RIGHT) = 0.0;
		/* Initialize delay values */
		*(volatile long *)(ELEM0_DELAY) = 1;
		*(volatile long *)(ELEM1_DELAY) = 1;
		*(volatile long *)(ELEM2_DELAY) = 1;
		*(volatile long *)(ELEM3_DELAY) = 1;
		*(volatile long *)(ELEM4_DELAY) = 1;
		*(volatile long *)(ELEM5_DELAY) = 1;
		*(volatile long *)(ELEM6_DELAY) = 1;
		*(volatile long *)(ELEM7_DELAY) = 1;
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
	#ifdef DSP_TARGET
		if(main_mem_size > exp_mem_size ) /* No expanded memory */
		{
 			line_len = (main_mem_size - dma_buf_size * 4)/NUM_ELEMS;
 			ptr0 = (float *)(MEMBANK0_START);
		}
		else /* Expanded memory present */
		{
 			/* If main mem is big, use it for delay, exp for dma buf */
 			if(main_mem_size > (MEMBANK0_LEN + DSP_PROGRAM_SIZE) )
			{
 			   line_len = main_mem_size/NUM_ELEMS;
 			   ptr0 = (float *)(MEMBANK0_START);
 			}
 			/* If main mem is small, use exp for both */
 			else
			{
 			   line_len = (exp_mem_size - dma_buf_size * 4)/NUM_ELEMS;
 			   ptr0 = (float *)(MEMBANK1_START);
 			}
		}
	#else
		line_len = DSPS_SOFT_MEM_DELAY_LENGTH/NUM_ELEMS;
		ptr0 = (float *)(MEMBANK0_START);
	#endif
 
		delay0_start = ptr0 + 0 * line_len;
		delay1_start = ptr0 + 1 * line_len;
		delay2_start = ptr0 + 2 * line_len;
		delay3_start = ptr0 + 3 * line_len;
		delay4_start = ptr0 + 4 * line_len;
		delay5_start = ptr0 + 5 * line_len;
		delay6_start = ptr0 + 6 * line_len;
		delay7_start = ptr0 + 7 * line_len;
		
		{	/* Init old delay values */
			unsigned j;
			for(j=0; j<NUM_ELEMS; j++)
				old_delay[j] =  (float)*(volatile long *)(ELEM0_DELAY + j);
		}

		/* Place variables into state array */
		{		
			/* int k; */
			float **fpp = (float **)&(fp_state[0]);
			long *lpp  =  (long *)&(fp_state[0]);
			fpp[0] = ptr0;
			/* Original version with variable memsize 
			fpp[1] = delay0_start;
			fpp[2] = delay1_start;
			fpp[3] = delay2_start;
			fpp[4] = delay3_start;
			fpp[5] = delay4_start;
			fpp[6] = delay5_start;
			fpp[7] = delay6_start;
			fpp[8] = delay7_start;
			lpp[9] = line_len;

			for(k=0; k<NUM_ELEMS; k++)
				fp_state[10 + k] = old_delay[k];
			*/
		}
	}

	/* Zero internal signal memory spaces to eliminates glitches on restarts */
	if( (i_init_flag & DSPS_INIT_MEMORY) || (i_init_flag & DSPS_ZERO_MEMORY) )
	{
		/* Now do DSPSOFT init */
		for(i=0; i<l_memsize; i++)
			fp_memory[i] = 0.0;
	}

	/* Initialize A/D and D/A converters, and set Samp Rate */
	/* Do as close as possible to running to minimize pops */
	dutilInitAIO();                                                            
	kerdmainit_MACRO();

	return(OKAY);
}
#endif /* DSPSOFT_32_BIT */

#ifdef DSPSOFT_TARGET
DSP_FUNC_DEF void DSPS_DLY_PROCESS(long *lp_data, int l_length,
								   float *fp_params, float *fp_memory, float *fp_state,
								   struct hardwareMeterValType *sp_meters,
								   int DSP_data_type)
{
	float *COMM_MEM_OFFSET = fp_params;
	float *MEMBANK0_START = fp_memory;

	/* Variables below must be restored from the state array and
	 * stored back at end of buffer processing
	 */
	float **fpp = (float **)&(fp_state[0]);
	long *lpp  =  (long *)&(fp_state[0]);
	float *ptr0 = fpp[0];
	/* Original version that handled variable memsize
	float *delay0_start = fpp[1];
	float *delay1_start = fpp[2];
	float *delay2_start = fpp[3];
	float *delay3_start = fpp[4];
	float *delay4_start = fpp[5];
	float *delay5_start = fpp[6];
	float *delay6_start = fpp[7];
	float *delay7_start = fpp[8];
	long line_len = lpp[9];
	*/
	/* Version with fixed memsize, no old_delay filtering */
	long line_len = DSPS_SOFT_MEM_DELAY_LENGTH/NUM_ELEMS;
	float *delay0_start = fp_memory + 0 * DSPS_SOFT_MEM_DELAY_LENGTH/NUM_ELEMS;
	float *delay1_start = fp_memory + 1 * DSPS_SOFT_MEM_DELAY_LENGTH/NUM_ELEMS;
	float *delay2_start = fp_memory + 2 * DSPS_SOFT_MEM_DELAY_LENGTH/NUM_ELEMS;
	float *delay3_start = fp_memory + 3 * DSPS_SOFT_MEM_DELAY_LENGTH/NUM_ELEMS;
	float *delay4_start = fp_memory + 4 * DSPS_SOFT_MEM_DELAY_LENGTH/NUM_ELEMS;
	float *delay5_start = fp_memory + 5 * DSPS_SOFT_MEM_DELAY_LENGTH/NUM_ELEMS;
	float *delay6_start = fp_memory + 6 * DSPS_SOFT_MEM_DELAY_LENGTH/NUM_ELEMS;
	float *delay7_start = fp_memory + 7 * DSPS_SOFT_MEM_DELAY_LENGTH/NUM_ELEMS;

	/* For checking for pointers past far end of memory space */
	float *delay_end = fp_memory + DSPS_SOFT_MEM_DELAY_LENGTH;

	/* float old_delay[NUM_ELEMS]; */ /* Loaded from state below */

#if(PT_DSP_BUILD == PT_DSP_DSPFX)
	/* Below only used in DSP-FX builds */
	long transfer_state = 0; /* For sending out meter values */
	long status = 0;         /* For sending run time status to PC */

	/* All the vars below are from DMA_LOCAL_DECLARATIONS.
	 * They are declared there as statics, but don't need to be
	 */
	float in_meter1_dma = 0.0;
	float in_meter2_dma = 0.0;
	float out_meter1_dma = 0.0;
	float out_meter2_dma = 0.0;
#endif

	unsigned data_index = 0;

	/* All the vars below are from DMA_GLOBAL_DECLARATIONS.
	 * They are declared there as statics, but don't need to be
	 */
	long *read_in_buf;
	long *read_out_buf;

	int i;

	#ifdef DSP_READ_VALS
	/* PTHACK for prototyping */
	ReadProtoVals(8, &ReadVals);
	#endif

	/* Complete loading vars from state array */
	/* Only needed if filtering delay (not done in software)
	{		
		int k;
		for(k=0; k<NUM_ELEMS; k++)
			old_delay[k] = fp_state[10 + k];
	}
	*/

	/* Run one buffer full of data. Zero index and averaged meter vals. */

	read_in_buf = lp_data;
	read_out_buf = lp_data;

	for(i=0; i<l_length; i++)
	{
	/* Loop forever- without input read (toolmkb), measures 488 cycles,
	 * Prior to effort to break out delay lines to minimize redundant pointer calcs.
	 * First pass to minimize pointer arithmetic went 488 -> 428.
	 * Converting dly_out vars from individual to dly_out[NUM_ELEMS] only (same code)
	 * Increased cycles 428 -> 432.
	 * Converting code to do loops using dly_out[NUM_ELEMS] worked- 432 -> 388
	 * Attempting to loop bottom code didn't work 388 -> 432.
	 * Attempting to loop to do parameter filtering 388 -> 488.
	 * Parameter filtering used added cycles when delays were set to zero.
	 * When all zero, cycles go from 488 -> 504 (toolmkb).  This gives
	 * occasional clicks, so must be right on edge.
	 * After final parameter filtering fix ups, jumped down to 440 !
	 */
	  float out1, out2;
	  float in1, in2;
	  float dly_out[NUM_ELEMS];
	  long delay_filtered[NUM_ELEMS];
	  volatile long in_count = 0;
	  volatile long out_count = 0;
	  
	  load_parameter(); /* If its been sent, loads a parameter into memory */

	  /* Next line includes kerdmaru.h run loop macro (no args) */
	  #include "kerdmaru.h"
	  
	  out1 = out2 = 0.0;
	    
	  dutilGetInputsAndMeter( in1, in2, status);
	  
	  dutilMuteInputs(in1, in2);
	  
	  {	  /* Loop to filter delay settings- looping was faster */
		  unsigned j;
		  volatile long *delay = (volatile long *)(ELEM0_DELAY);
		  for(j=0; j<NUM_ELEMS; j++)
		  {
			/* Hardware version filters delay changes, no filtering on soft dsp */
			#ifdef DSP_TARGET
			kerParamFiltExp(delay, old_delay[j], DELAY_CONTROL_ALPHA, delay_filtered[j]);		    
			#else
			delay_filtered[j] = *delay;
			#endif

     		delay++;
      	  } 
      } 

	  /* Wrap main pointer if needed. Moved up from bottom to avoid
	   * bad memory accesses when DSP/FX called init while DAW dll
	   * is making processing calls
	   */
	  if( ptr0 >= delay1_start )
		  ptr0 = delay0_start;

	  { /* Break delay line reads and updates out to use a single pointer for updates */
	  	float *rp_MACRO;
	  	float *tmp_ptr = ptr0;

		rp_MACRO = (float *)(ptr0 - delay_filtered[0] );
	 	if( (long)rp_MACRO < (long)delay0_start ) 		
			rp_MACRO += line_len; 	
		/* Note - currently in PC version, init calls are asynchronous
		 * to processing calls. This can cause bad delay setting values
		 * to be temporarily present when changing num of elements. To
		 * protect against this, do a run time check of point boundaries
		 * on both ends.
		 */
		#ifndef DSP_TARGET
	 	if( (long)rp_MACRO >= (long)delay1_start ) 		
			rp_MACRO = delay0_start;
		#endif
		dly_out[0] = *rp_MACRO; 
		*ptr0 = in1 + *(volatile float *)(ELEM0_FEEDBACK) * dly_out[0]; 	
		
#ifdef ELEM1
		tmp_ptr = ptr0 + line_len; /* These LINE_LEN increments could be done on PC side */
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[1] );
	 	if( (long)rp_MACRO < (long)delay1_start ) 		
			rp_MACRO += line_len; 	
		#ifndef DSP_TARGET
	 	if( (long)rp_MACRO >= (long)delay2_start ) 		
			rp_MACRO = delay1_start;
		#endif
		dly_out[1] = *rp_MACRO; 
		*tmp_ptr = in2 + *(volatile float *)(ELEM1_FEEDBACK) * dly_out[1]; 	
#endif
		
#ifdef ELEM2
		tmp_ptr += line_len;
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[2] );
	 	if( (long)rp_MACRO < (long)delay2_start ) 		
			rp_MACRO += line_len; 	
		#ifndef DSP_TARGET
	 	if( (long)rp_MACRO >= (long)delay3_start ) 		
			rp_MACRO = delay2_start;
		#endif
		dly_out[2] = *rp_MACRO; 
		*tmp_ptr = in1 + *(volatile float *)(ELEM2_FEEDBACK) * dly_out[2]; 	
#endif
		
#ifdef ELEM3
		tmp_ptr += line_len;
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[3] );
	 	if( (long)rp_MACRO < (long)delay3_start ) 		
			rp_MACRO += line_len; 	
		#ifndef DSP_TARGET
	 	if( (long)rp_MACRO >= (long)delay4_start ) 		
			rp_MACRO = delay3_start;
		#endif
		dly_out[3] = *rp_MACRO; 
		*tmp_ptr = in2 + *(volatile float *)(ELEM3_FEEDBACK) * dly_out[3]; 	
#endif
		
#ifdef ELEM4
		tmp_ptr += line_len;
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[4] );
	 	if( (long)rp_MACRO < (long)delay4_start ) 		
			rp_MACRO += line_len; 	
		#ifndef DSP_TARGET
	 	if( (long)rp_MACRO >= (long)delay5_start ) 		
			rp_MACRO = delay4_start;
		#endif
		dly_out[4] = *rp_MACRO; 
		*tmp_ptr = in1 + *(volatile float *)(ELEM4_FEEDBACK) * dly_out[4]; 	
#endif
		
#ifdef ELEM5
		tmp_ptr += line_len;
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[5] );
	 	if( (long)rp_MACRO < (long)delay5_start ) 		
			rp_MACRO += line_len; 	
		#ifndef DSP_TARGET
	 	if( (long)rp_MACRO >= (long)delay6_start ) 		
			rp_MACRO = delay5_start;
		#endif
		dly_out[5] = *rp_MACRO; 
		*tmp_ptr = in2 + *(volatile float *)(ELEM5_FEEDBACK) * dly_out[5]; 	
#endif

        dutilGet2ndAESInputOutput(in_meter1, in_meter2, out_meter1, out_meter2);
		
#ifdef ELEM6
		tmp_ptr += line_len;
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[6] );
	 	if( (long)rp_MACRO < (long)delay6_start ) 		
			rp_MACRO += line_len; 	
		#ifndef DSP_TARGET
	 	if( (long)rp_MACRO >= (long)delay7_start ) 		
			rp_MACRO = delay6_start;
		#endif
		dly_out[6] = *rp_MACRO; 
		*tmp_ptr = in1 + *(volatile float *)(ELEM6_FEEDBACK) * dly_out[6]; 	
#endif
		
#ifdef ELEM7
		tmp_ptr += line_len;
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[7] );
	 	if( (long)rp_MACRO < (long)delay7_start ) 		
			rp_MACRO += line_len; 	
		#ifndef DSP_TARGET
	 	if( (long)rp_MACRO >= (long)delay_end ) 		
			rp_MACRO = delay7_start;
		#endif
		dly_out[7] = *rp_MACRO;		
		*tmp_ptr = in2 + *(volatile float *)(ELEM7_FEEDBACK) * dly_out[7]; 	
#endif
		
		ptr0++; 	
		/* Note - the pointer wrapping was moved to the top to 
		   allow DSPFX app to make init calls while DAW dll
		   is making processing calls. Having it here causes
		   bad memory accesses above when preset has changed.
		if( ptr0 >= delay1_start )
			ptr0 = delay0_start;
		 */
	  }
	  
	  {
	  	unsigned j; /* Unsigned for no overhead looping */
	  	volatile float *element_gain = (volatile float *)(ELEM0_GAIN);
	  	volatile float *pan_left_gain = (volatile float *)(ELEM0_PAN_GAIN_LEFT);
	  	for(j=0; j<NUM_ELEMS; j++)
	  		out1 += (dly_out[j] *= element_gain[j]) * pan_left_gain[j];
	  }
	  	
	  {
	  	unsigned j; /* Unsigned for no overhead looping */
	  	volatile float *pan_right_gain = (volatile float *)(ELEM0_PAN_GAIN_RIGHT);
	  	for(j=0; j<NUM_ELEMS; j++)
	  		out2 += dly_out[j] * pan_right_gain[j];
	  }
					
     /* To make mono bypass correct */
	  if( !*(volatile int *)(DSP_STEREO_IN_FLAG) )
		  in2 = (realtype)0.0;


	  /* PTHACK for PROTOTYPING */
	  #ifdef DSP_READ_VALS
	  {
		  float wet_gain, dry_gain;

		  if( ReadVals.switch_vals[2] )
				dry_gain = ReadVals.f_vals[2];
		  else
				dry_gain = (float)0.0;
		  if( ReadVals.switch_vals[3] )
				wet_gain = ReadVals.f_vals[3];
		  else
				wet_gain = (float)0.0;

		  kerWetDry(in1, in2, &wet_gain, &dry_gain, out1, out2);
	  }
	  #else
	  kerWetDry(in1, in2, (WET_GAIN), (DRY_GAIN), out1, out2);
	  #endif

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
		/* int k; */
		float **fpp = (float **)&(fp_state[0]);
		long *lpp  =  (long *)&(fp_state[0]);
		fpp[0] = ptr0;
		/* Original version that allowed dynamic memsizes 
		fpp[1] = delay0_start;
		fpp[2] = delay1_start;
		fpp[3] = delay2_start;
		fpp[4] = delay3_start;
		fpp[5] = delay4_start;
		fpp[6] = delay5_start;
		fpp[7] = delay6_start;
		fpp[8] = delay7_start;
		lpp[9] = line_len;

		for(k=0; k<NUM_ELEMS; k++)
			fp_state[10 + k] = old_delay[k];
		*/
	}
} 
#endif



