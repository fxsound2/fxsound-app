

/*
 * FILE: dly8.c
 * DATE: 11/9/95
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: 8 Element Delay
 *
 */

/* #define NAMM_SHOW */ /* Cause delays to use upper memory */

/* Standard includes */
#include <math.h>
#define DSP_TARGET    /* For target dependent definitions  */
#include "platform.h" /* Targets a specific DSP card       */
#include "hardware.h" /* Common Parameter Definitions and Offsets */
#include "codedefs.h"

#ifdef ARIEL_CYCLOPS
#include "axp.h"
#include "int.h"
#include "memory.h"
#include "dutil.h"
#endif

#include "dsp_mem1.h" /* For DSP memory configuration (typical delay version)*/
#include "c_dly1.h"   /* For specific parameter mappings */
#include "dutilaio.h" /* A/D and D/A Macros */
#include "kerdelay.h" /* Delay Line Macros  */

void main(int argc, char *argv[])
{
 float *ptr0, *ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6, *ptr7;
 long i;

#define LINE_LEN (MEMBANK0_LEN/8)
#define DELAY0_START (MEMBANK0_START)
#define DELAY1_START (MEMBANK0_START + LINE_LEN)
#define DELAY2_START (MEMBANK0_START + 2 * LINE_LEN)
#define DELAY3_START (MEMBANK0_START + 3 * LINE_LEN)
#define DELAY4_START (MEMBANK0_START + 4 * LINE_LEN)
#define DELAY5_START (MEMBANK0_START + 5 * LINE_LEN)
#define DELAY6_START (MEMBANK0_START + 6 * LINE_LEN)
#define DELAY7_START (MEMBANK0_START + 7 * LINE_LEN)

 ptr0 = (float *)DELAY0_START;

 /* Initialize circular buffers-Need one location for each delay, min 1 */
 for(i=0; i< (8 * LINE_LEN); i++)
 {
		 ptr0[i] = 0.0;
 }

 /* Initialize A/D and D/A converters, and set Samp Rate */
 /* Do as close as possible to running to minimize pops */
 dutilInitAIO();                                                            

 { /* Put all runtime loop variables that start with an init val here */
	long transfer_state = 0; /* For sending out meter values */
	long status = 0;         /* For sending run time status to PC */
	/*
	long startpoints[] = {(DELAY0_START), (DELAY1_START), (DELAY2_START), (DELAY3_START),
						(DELAY4_START), (DELAY5_START), (DELAY6_START), (DELAY7_START)};
	*/

	/* Loop forever- without input read (toolmkb), measures 488 cycles,
	 * Prior to effort to break out delay lines to minimize redundant pointer calcs.
	 * First pass to minimize pointer arithmetic went 488 -> 428.
	 * Converting dly_out vars from individual to dly_out[8] only (same code)
	 * Increased cycles 428 -> 432.
	 * Converting code to do loops using dly_out[8] worked- 432 -> 388
	 */
	while(1)
	{
	  float out1, out2;
	  float in1, in2;
	  /* float dly_out0, dly_out1, dly_out2, dly_out3, dly_out4, dly_out5, dly_out6, dly_out7; */
	  float dly_out[8];
	  long in_meter1, in_meter2, out_meter1, out_meter2;
	  /* volatile long in_count = 0; */
	  
	  load_parameter(); /* If its been sent, loads a parameter into memory */

	  out1 = out2 = 0.0;
	    
	  dutilGetInputsAndMeter( in1, in2, transfer_state, in_meter1, in_meter2);
	  
	  if(*(volatile long *)(DSP_MUTE_IN_FLAG))
	  {
	  	 in1 = 0.0;
	  	 in2 = 0.0;
	  } 

#ifdef UNDEF /* Attempting to collapse code below 388 -> 432 */	  
	  { 	
		float *rp_MACRO;
		float *tmp_ptr = ptr0;
		volatile long *delay = (volatile long *)(ELEM0_DELAY);
		volatile float *feedback = (volatile float *)(ELEM0_FEEDBACK);
		unsigned j;
		for(j=0; j<8; j++)
		{
			rp_MACRO = (float *)(tmp_ptr - delay[j]);
	 		if( (long)rp_MACRO < startpoints[j] ) 		
				rp_MACRO += (LINE_LEN); 	
			dly_out[j] = *rp_MACRO; 
			*tmp_ptr = in1 + feedback[j] * dly_out[j]; 	
			tmp_ptr += (LINE_LEN);
		}		
	  }
  	  ptr0++; 	
	  if(ptr0 >= (float *)(DELAY1_START) )
		ptr0 = (float *)(DELAY0_START);
#endif /* UNDEF */

	  { /* Break delay line reads and updates out to use a single pointer for updates */
	  	float *rp_MACRO;
	  	float *tmp_ptr = ptr0;

		rp_MACRO = (float *)(ptr0 - *(volatile long *)(ELEM0_DELAY) );
	 	if( (long)rp_MACRO < (long)((DELAY0_START) + 0 * (LINE_LEN)) ) 		
			rp_MACRO += (LINE_LEN); 	
		dly_out[0] = *rp_MACRO; 
		*ptr0 = in1 + *(volatile float *)(ELEM0_FEEDBACK) * dly_out[0]; 	
		
		tmp_ptr = ptr0 + LINE_LEN; /* These LINE_LEN increments could be done on PC side */
		rp_MACRO = (float *)(tmp_ptr - *(volatile long *)(ELEM1_DELAY) );
	 	if( (long)rp_MACRO < (long)((DELAY0_START) + 1 * (LINE_LEN)) ) 		
			rp_MACRO += (LINE_LEN); 	
		dly_out[1] = *rp_MACRO; 
		*tmp_ptr = in2 + *(volatile float *)(ELEM1_FEEDBACK) * dly_out[1]; 	
		
		tmp_ptr += (LINE_LEN);
		rp_MACRO = (float *)(tmp_ptr - *(volatile long *)(ELEM2_DELAY) );
	 	if( (long)rp_MACRO < (long)((DELAY0_START) + 2 * (LINE_LEN)) ) 		
			rp_MACRO += (LINE_LEN); 	
		dly_out[2] = *rp_MACRO; 
		*tmp_ptr = in1 + *(volatile float *)(ELEM2_FEEDBACK) * dly_out[2]; 	
		
		tmp_ptr += (LINE_LEN);
		rp_MACRO = (float *)(tmp_ptr - *(volatile long *)(ELEM3_DELAY) );
	 	if( (long)rp_MACRO < (long)((DELAY0_START) + 3 * (LINE_LEN)) ) 		
			rp_MACRO += (LINE_LEN); 	
		dly_out[3] = *rp_MACRO; 
		*tmp_ptr = in2 + *(volatile float *)(ELEM3_FEEDBACK) * dly_out[3]; 	
		
		tmp_ptr += (LINE_LEN);
		rp_MACRO = (float *)(tmp_ptr - *(volatile long *)(ELEM4_DELAY) );
	 	if( (long)rp_MACRO < (long)((DELAY0_START) + 4 * (LINE_LEN)) ) 		
			rp_MACRO += (LINE_LEN); 	
		dly_out[4] = *rp_MACRO; 
		*tmp_ptr = in1 + *(volatile float *)(ELEM4_FEEDBACK) * dly_out[4]; 	
		
		tmp_ptr += (LINE_LEN);
		rp_MACRO = (float *)(tmp_ptr - *(volatile long *)(ELEM5_DELAY) );
	 	if( (long)rp_MACRO < (long)((DELAY0_START) + 5 * (LINE_LEN)) ) 		
			rp_MACRO += (LINE_LEN); 	
		dly_out[5] = *rp_MACRO; 
		*tmp_ptr = in2 + *(volatile float *)(ELEM5_FEEDBACK) * dly_out[5]; 	
		
		tmp_ptr += (LINE_LEN);
		rp_MACRO = (float *)(tmp_ptr - *(volatile long *)(ELEM6_DELAY) );
	 	if( (long)rp_MACRO < (long)((DELAY0_START) + 6 * (LINE_LEN)) ) 		
			rp_MACRO += (LINE_LEN); 	
		dly_out[6] = *rp_MACRO; 
		*tmp_ptr = in1 + *(volatile float *)(ELEM6_FEEDBACK) * dly_out[6]; 	
		
		tmp_ptr += (LINE_LEN);
		rp_MACRO = (float *)(tmp_ptr - *(volatile long *)(ELEM7_DELAY) );
	 	if( (long)rp_MACRO < (long)((DELAY0_START) + 7 * (LINE_LEN)) ) 		
			rp_MACRO += (LINE_LEN); 	
		dly_out[7] = *rp_MACRO;		
		*tmp_ptr = in2 + *(volatile float *)(ELEM7_FEEDBACK) * dly_out[7]; 	
		
		ptr0++; 	
		if(ptr0 >= (float *)(DELAY1_START) )
			ptr0 = (float *)(DELAY0_START);
	  }
	  
	  {
	  	unsigned j; /* Unsigned for no overhead looping */
	  	volatile float *element_gain = (volatile float *)(ELEM0_GAIN);
	  	volatile float *pan_left_gain = (volatile float *)(ELEM0_PAN_GAIN_LEFT);
	  	for(j=0; j<8; j++)
	  		out1 += (dly_out[j] *= element_gain[j]) * pan_left_gain[j];
	  }
	  	
	  {
	  	unsigned j; /* Unsigned for no overhead looping */
	  	volatile float *pan_right_gain = (volatile float *)(ELEM0_PAN_GAIN_RIGHT);
	  	for(j=0; j<8; j++)
	  		out2 += dly_out[j] * pan_right_gain[j];
	  }
					
	  kerWetDry(in1, in2, (WET_GAIN), (DRY_GAIN), out1, out2);

	  dutilSetClipStatus(in1, in2, out1, out2, status);

	  dutilPutOutputsAndMeter(out1, out2, transfer_state, out_meter1, out_meter2 );

	  write_meters_and_status(in_meter1, in_meter2, out_meter1, out_meter2, status, transfer_state);
	}
 }
}
