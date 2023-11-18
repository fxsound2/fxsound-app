/*
FxSound
Copyright (C) 2023  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * FILE: dlyX.c
 * DATE: 11/9/95 Modified - 2/21/97
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: Multi- Element Delay, num set by dly_num.h
 *
 */

/* Standard includes */
#include <math.h>
#define DSP_TARGET    /* For target dependent definitions  */
#include "codedefs.h" /* Company common defines */
#include "platform.h" /* Targets a specific DSP card       */
#include "pt_defs.h"  /* Defines common to both PC and DSP */	
#include "boardrv1.h" /* DSP hardware specific defines, for both PC and DSP */

/* #define DSP_PROGRAM_SIZE 0x400 */ /* Overide default program size */
#include "dma32.h"
#include "dsp_mem1.h" /* For DSP memory configuration (typical delay version)*/
#include "c_dly1.h"   /* For specific parameter mappings */
#include "dutio.h"	  /* A/D I/O macros */
#include "dutcom.h"	  /* PC to DSP and DSP to PC com routines */
#include "dutinit.h"  /* DSP and AD initialization */
#include "kerdma.h"
#include "kerdelay.h" /* Delay Line Macros  */
#include "kercntl.h"  /* For filtered parameters */

#include "dly_num.h"  /* Sets number of elements */

DMA_GLOBAL_DECLARATIONS

void main(int argc, char *argv[])
{
 float *ptr0;
 long line_len;
 long i;
 DMA_LOCAL_DECLARATIONS


 /* Get mem sizes from PC, zero memory */
 dutilGetMemSizes();
 dutilInitMem();
 
#define FPVAL *(volatile float *)
#define LPVAL *(volatile long *) 

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
 
 /* Initialize A/D and D/A converters, and set Samp Rate */
 /* Do as close as possible to running to minimize pops */
 dutilInitAIO();                                                            
 
 /* Initialize memory pointers. Note main memsize is already
  * compensated for 0x400 program size
  */
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
 
kerdmainit_MACRO();

 { /* Put all runtime loop variables that start with an init val here */
	long transfer_state = 0; /* For sending out meter values */
	long status = 0;         /* For sending run time status to PC */
	float old_delay[NUM_ELEMS];
	float *delay0_start, *delay1_start, *delay2_start, *delay3_start;
	float *delay4_start, *delay5_start, *delay6_start, *delay7_start;
	long in_meter1 = 0, in_meter2 = 0, out_meter1 = 0, out_meter2 = 0;
	
	/*
	long startpoints[] = {(DELAY0_START), (DELAY1_START), (DELAY2_START), (DELAY3_START),
						(DELAY4_START), (DELAY5_START), (DELAY6_START), (DELAY7_START)};
	*/
	
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
	while(1)
	{
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
	    
	  dutilGetInputsAndMeter( in1, in2, in_meter1, in_meter2, out_meter1, out_meter2);
	  
	  dutilMuteInputs(in1, in2);
	  
	  {	  /* Loop to filter delay settings- looping was faster */
		  unsigned j;
		  volatile long *delay = (volatile long *)(ELEM0_DELAY);
		  for(j=0; j<NUM_ELEMS; j++)
		  {
     		kerParamFiltExp(delay, old_delay[j], DELAY_CONTROL_ALPHA, delay_filtered[j]);		    
     		delay++;
      	  } 
      } 

	  { /* Break delay line reads and updates out to use a single pointer for updates */
	  	float *rp_MACRO;
	  	float *tmp_ptr = ptr0;

		rp_MACRO = (float *)(ptr0 - delay_filtered[0] );
	 	if( (long)rp_MACRO < (long)delay0_start ) 		
			rp_MACRO += line_len; 	
		dly_out[0] = *rp_MACRO; 
		*ptr0 = in1 + *(volatile float *)(ELEM0_FEEDBACK) * dly_out[0]; 	
		
#ifdef ELEM1
		tmp_ptr = ptr0 + line_len; /* These LINE_LEN increments could be done on PC side */
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[1] );
	 	if( (long)rp_MACRO < (long)delay1_start ) 		
			rp_MACRO += line_len; 	
		dly_out[1] = *rp_MACRO; 
		*tmp_ptr = in2 + *(volatile float *)(ELEM1_FEEDBACK) * dly_out[1]; 	
#endif
		
#ifdef ELEM2
		tmp_ptr += line_len;
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[2] );
	 	if( (long)rp_MACRO < (long)delay2_start ) 		
			rp_MACRO += line_len; 	
		dly_out[2] = *rp_MACRO; 
		*tmp_ptr = in1 + *(volatile float *)(ELEM2_FEEDBACK) * dly_out[2]; 	
#endif
		
#ifdef ELEM3
		tmp_ptr += line_len;
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[3] );
	 	if( (long)rp_MACRO < (long)delay3_start ) 		
			rp_MACRO += line_len; 	
		dly_out[3] = *rp_MACRO; 
		*tmp_ptr = in2 + *(volatile float *)(ELEM3_FEEDBACK) * dly_out[3]; 	
#endif
		
#ifdef ELEM4
		tmp_ptr += line_len;
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[4] );
	 	if( (long)rp_MACRO < (long)delay4_start ) 		
			rp_MACRO += line_len; 	
		dly_out[4] = *rp_MACRO; 
		*tmp_ptr = in1 + *(volatile float *)(ELEM4_FEEDBACK) * dly_out[4]; 	
#endif
		
#ifdef ELEM5
		tmp_ptr += line_len;
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[5] );
	 	if( (long)rp_MACRO < (long)delay5_start ) 		
			rp_MACRO += line_len; 	
		dly_out[5] = *rp_MACRO; 
		*tmp_ptr = in2 + *(volatile float *)(ELEM5_FEEDBACK) * dly_out[5]; 	
#endif

        dutilGet2ndAESInputOutput(in_meter1, in_meter2, out_meter1, out_meter2);
		
#ifdef ELEM6
		tmp_ptr += line_len;
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[6] );
	 	if( (long)rp_MACRO < (long)delay6_start ) 		
			rp_MACRO += line_len; 	
		dly_out[6] = *rp_MACRO; 
		*tmp_ptr = in1 + *(volatile float *)(ELEM6_FEEDBACK) * dly_out[6]; 	
#endif
		
#ifdef ELEM7
		tmp_ptr += line_len;
		rp_MACRO = (float *)(tmp_ptr - delay_filtered[7] );
	 	if( (long)rp_MACRO < (long)delay7_start ) 		
			rp_MACRO += line_len; 	
		dly_out[7] = *rp_MACRO;		
		*tmp_ptr = in2 + *(volatile float *)(ELEM7_FEEDBACK) * dly_out[7]; 	
#endif
		
		ptr0++; 	
		if( ptr0 >= delay1_start )
			ptr0 = delay0_start;
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
					
	  kerWetDry(in1, in2, (WET_GAIN), (DRY_GAIN), out1, out2);

	  dutilSetClipStatus(in1, in2, out1, out2, status);

	  dutilPutOutputsAndMeter(out1, out2, out_meter1, out_meter2 );

	  write_meters_and_status(in_meter1, in_meter2, out_meter1, out_meter2, status, transfer_state);
	}
 }
}

