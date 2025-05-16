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
 * FILE: peqX.c
 * DATE: 6/7/96
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: Parametric Eq.
 * PC version that gets copied into all PC eq directories.
 *
 */
#ifndef DSPSOFT_TARGET
#define DSP_TARGET
#endif
#include "peq_num.h"  /* Sets number of peq elements */

/* Standard includes */
#include <math.h>
#include <stdio.h>
#include "codedefs.h" /* Company common defines */
#include "platform.h" /* Targets a specific DSP card       */
#include "pt_defs.h"  /* Defines common to both PC and DSP */	
#include "product_type.h" /* Sets DSPFX or DFX */
#include "boardrv1.h" /* DSP hardware specific defines, for both PC and DSP */

#ifdef DSP_TARGET
#include "dma32.h"
/* #define DSP_PROGRAM_SIZE 0x600 */ /* Overide default program size */
#endif

#include "dsp_mem1.h" /* For DSP memory configuration (typical delay version)*/
#include "c_dsps.h"
#include "c_peq.h"    /* For specific parameter mappings */
#include "dutio.h"	  /* A/D I/O macros */
#include "dutcom.h"	  /* PC to DSP and DSP to PC com routines */
#include "dutinit.h"  /* DSP and AD initialization */
#include "kerdma.h"
#include "kerdelay.h" /* Delay Line Macros  */
#include "kercntl.h"  /* Parameter filtering */
#include "kerfilt.h"  /* Filtering macros */
 
static void DSPS_PEQ_RUN(void);

#ifdef DSP_TARGET
void main(int argc, char *argv[])
{
	DSPS_PEQ_INIT();
	while(1)
		DSPS_PEQ_RUN();
}
#endif

/* Only build init function in 32 bit files (same for both) */
#if defined(DSPSOFT_32_BIT)
DSP_FUNC_DEF int DSPS_PEQ_INIT(float *fp_params, float *fp_memory, long l_memsize, float *fp_state, int i_init_flag, float r_samp_freq)
{
	float *COMM_MEM_OFFSET = fp_params;
	float *MEMBANK0_START = fp_memory;

	/* Variables below must be restored from the state array and
	 * stored back at end of buffer processing
	 */
	/* peq uses fp_state as an array for internal filter state storage.
	 * Used directly, so no need to read and write it.
	 */

	int i;

	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		/* Initialize filter parameter coeffs */
		{
			/* Point to first parameters in parameter sections */
			float *input_params = (float *)(DSP_PEQ_LEFT_GAIN);
			float *filt_params = (float *)(DSP_PEQ_LEFT_GAIN_FILT);
			unsigned i;
			for(i=0; i<DSP_PEQ_NUM_FILTER_PARAMETERS; i++)
			{
				input_params[i] = (float)0.3;
				filt_params[i] = (float)0.3;
			}
		}	
		*(volatile float *)(DSP_PEQ_LEFT_GAIN) = 0.0;
		*(volatile float *)(DSP_PEQ_RIGHT_GAIN) = 0.0;
		*(volatile float *)(DSP_PEQ_DRY_GAIN) = 0.0;
		*(volatile float *)(DSP_PEQ_LEFT_GAIN_FILT) = 0.0;
		*(volatile float *)(DSP_PEQ_RIGHT_GAIN_FILT) = 0.0;
		*(volatile float *)(DSP_PEQ_DRY_GAIN_FILT) = 0.0;
		*(volatile long *)(DSP_PEQ_SHELFS_ON) = 0;		
	}

	if( i_init_flag & DSPS_INIT_MEMORY )
	{
		/* Note that any state vars must be initialized in this section
		 * since they must be present in the dll instance that will
		 * call the processing functions
		 */
		/* Get mem sizes from PC, zero memory */
		dutilGetMemSizes();
		/* No allocated memory */

		/* State space is used directly for internal filter states. */
		for(i=0; i<(4 * (DSP_MAX_NUM_OF_PEQ_ELEMENTS + 2)); i++)
			fp_state[i] = (float)0.0;
	}
    
    /* Initialize A/D and D/A converters, and set Samp Rate */
    /* Do as close as possible to running to minimize pops */
    dutilInitAIO();
    
 	kerdmainit_MACRO();

	return(OKAY);
}
#endif /* DSPSOFT_32_BIT */
 	
#ifdef DSPSOFT_TARGET
DSP_FUNC_DEF void DSPS_PEQ_PROCESS(long *lp_data, int l_length,
								 float *fp_params, float *fp_memory, float *fp_state,
								 struct hardwareMeterValType *sp_meters,
								 int DSP_data_type)
{
	float *COMM_MEM_OFFSET = fp_params;
	float *MEMBANK0_START = fp_memory;
	/* Variables below must be restored from the state array and
	 * stored back at end of buffer processing
	 */
	/* NO STATE VARS FOR PEQ */

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
	  float in1, in2, out1, out2;
	  float *coeffs, *state;
#ifdef DSP_TARGET
	  unsigned k;
#endif
	  
	  load_parameter(); /* If its been sent, loads a parameter into memory */
	  
 	  /* Next line includes kerdmaru.h run loop macro (no args) */
	  #include "kerdmaru.h"
	  
	  /* Current AES split is 256 top, 232 bottom cycles. Moving below param filtering was worse. */
	  dutilGetInputsAndMeter( in1, in2, status);

	  /* Add bias to inputs to avoid zero input cpu load increases */
	  in1 += (float)DSP_DENORM_BIAS_LARGE;
	  in2 += (float)DSP_DENORM_BIAS_LARGE;
	  
	  /* Filter all floating parameters coming in */
	  /* Adding an outside loop with 2 steps jumped cycles from 504 to 540!! */
	  /* Inline step method used below, with only one end check. Note this overuns
	   * end memory space, so need extra space there.
	   */
#ifdef DSP_TARGET
	  for(k=0; k<8; k++)
	  {
        {
      	  static unsigned j=0;         
	      filt_params[j] = (1.0 - PEQ_ALPHA) * filt_params[j] + PEQ_ALPHA * input_params[j];
		  j++;
		  if( j >= DSP_PEQ_NUM_FILTER_PARAMETERS )
		    j = 0;
		}
	  }
	
	  coeffs = (float *)(DSP_SECTION_FILT_START);
#else
	  /* PC version uses straight unfiltered coeffs */
	  coeffs = (float *)(DSP_SECTION_INPUT_START);
#endif
	  state = fp_state; /* For internal filter states */
	  {
	    realtype outa, outb;
	    
#define COEFF_SKIP 4 /* Used to increment over unused coeffs, states */
#define STATE_SKIP 2

		/* if( *(volatile long *)(DSP_PEQ_SHELFS_ON) ) NO LONGER USED */
		if( *(volatile long *)(DSP_SECTION_STATUS + 0) )
		{
			/* First 2 shelfs get normal, rest get para macro */
			kerSosFiltDirectForm2TransExtState( in1, coeffs, state, outa);
		}
		else
		{
			/* Shelf sections have an extra coeff */
			coeffs += (COEFF_SKIP + 1);
			state  += STATE_SKIP ;
			outa = in1;
		}

		if( *(volatile long *)(DSP_SECTION_STATUS + 1) )
		{
			/* First 2 shelfs get normal, rest get para macro */
			kerSosFiltDirectForm2TransExtState(outa, coeffs, state, outb);
		}
		else
		{
			/* Shelf sections have an extra coeff */
			coeffs += (COEFF_SKIP + 1);
			state  += STATE_SKIP;
			outb = outa;
		}

		/* Now do parametric sections */
		if( *(volatile long *)(DSP_SECTION_STATUS + 2) )
		{
			kerSosFiltDirectForm2TransParaExtState(outb, coeffs, state, outa);
		}
		else
		{
			outa = outb;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM2)
		out1 = outa;
		coeffs += COEFF_SKIP * 7;
		state  += STATE_SKIP * 7;
		#endif
		#if defined(ELEM2)
		if( *(volatile long *)(DSP_SECTION_STATUS + 3) )
		{
			kerSosFiltDirectForm2TransParaExtState(outa, coeffs, state, outb);
		}
		else
		{
			outb = outa;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM3)
		out1 = outb;
		coeffs += COEFF_SKIP * 6;
		state  += STATE_SKIP * 6;
		#endif
		#endif
		#if defined(ELEM3)
		if( *(volatile long *)(DSP_SECTION_STATUS + 4) )
		{
			kerSosFiltDirectForm2TransParaExtState(outb, coeffs, state, outa);
		}
		else
		{
			outa = outb;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM4)
		out1 = outa;
		coeffs += COEFF_SKIP * 5;
		state  += STATE_SKIP * 5;
		#endif
		#endif
		#if defined(ELEM4)
		if( *(volatile long *)(DSP_SECTION_STATUS + 5) )
		{
		    kerSosFiltDirectForm2TransParaExtState(outa, coeffs, state, outb);
		}
		else
		{
			outb = outa;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM5)
		out1 = outb;
		coeffs += COEFF_SKIP * 4;
		state  += STATE_SKIP * 4;
		#endif
		#endif
		#if defined(ELEM5)
		if( *(volatile long *)(DSP_SECTION_STATUS + 6) )
		{
			kerSosFiltDirectForm2TransParaExtState(outb, coeffs, state, outa);
		}
		else
		{
			outa = outb;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM6)
		out1 = outa;
		coeffs += COEFF_SKIP * 3;
		state  += STATE_SKIP * 3;
		#endif
		#endif
	  	  
        /* Top section-240, bottom section 236, 476 total */
        dutilGet2ndAESInputOutput(in_meter1, in_meter2, out_meter1, out_meter2);

		#if defined(ELEM6)
		if( *(volatile long *)(DSP_SECTION_STATUS + 7) )
		{
			kerSosFiltDirectForm2TransParaExtState(outa, coeffs, state, outb);
		}
		else
		{
			outb = outa;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM7)
		out1 = outb;
		coeffs += COEFF_SKIP * 2;
		state  += STATE_SKIP * 2;
		#endif
		#endif
		#if defined(ELEM7)
		if( *(volatile long *)(DSP_SECTION_STATUS + 8) )
		{
			kerSosFiltDirectForm2TransParaExtState(outb, coeffs, state, outa);
		}
		else
		{
			outa = outb;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM8)
		out1 = outa;
		coeffs += COEFF_SKIP * 1;
		state  += STATE_SKIP * 1;
		#endif
		#endif
		#if defined(ELEM8)
		if( *(volatile long *)(DSP_SECTION_STATUS + 9) )
		{
			kerSosFiltDirectForm2TransParaExtState(outa, coeffs, state, out1);
		}
		else
		{
			out1 = outa;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#endif
	  }
	  
	  /* Only do right channel if stereo input. */
	  if( *(volatile long *)(DSP_STEREO_IN_FLAG) )
	  {	  
	    realtype outa, outb;
	    
		/* if( *(volatile long *)(DSP_PEQ_SHELFS_ON) ) NO LONGER USED */
		if( *(volatile long *)(DSP_SECTION_STATUS + 10) )
		{
			/* First 2 shelfs get normal, rest get para macro */
			kerSosFiltDirectForm2TransExtState( in2, coeffs, state, outa);
		}
		else
		{
			/* Shelf sections have an extra coeff */
			coeffs += (COEFF_SKIP + 1);
			state  += STATE_SKIP;
			outa = in2;
		}

		if( *(volatile long *)(DSP_SECTION_STATUS + 11) )
		{
			/* First 2 shelfs get normal, rest get para macro */
			kerSosFiltDirectForm2TransExtState(outa, coeffs, state, outb);
		}
		else
		{
			/* Shelf sections have an extra coeff */
			coeffs += (COEFF_SKIP + 1);
			state  += STATE_SKIP;
			outb = outa;
		}

		/* Now do parametric sections */
		if( *(volatile long *)(DSP_SECTION_STATUS + 12) )
		{
			kerSosFiltDirectForm2TransParaExtState(outb, coeffs, state, outa);
		}
		else
		{
			outa = outb;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM2)
		out2 = outa;
		#endif
		#if defined(ELEM2)
		if( *(volatile long *)(DSP_SECTION_STATUS + 13) )
		{
			kerSosFiltDirectForm2TransParaExtState(outa, coeffs, state, outb);
		}
		else
		{
			outb = outa;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM3)
		out2 = outb;
		#endif
		#endif
		#if defined(ELEM3)
		if( *(volatile long *)(DSP_SECTION_STATUS + 14) )
		{
			kerSosFiltDirectForm2TransParaExtState(outb, coeffs, state, outa);
		}
		else
		{
			outa = outb;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM4)
		out2 = outa;
		#endif
		#endif
		#if defined(ELEM4)
		if( *(volatile long *)(DSP_SECTION_STATUS + 15) )
		{
			kerSosFiltDirectForm2TransParaExtState(outa, coeffs, state, outb);
		}
		else
		{
			outb = outa;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM5)
		out2 = outb;
		#endif
		#endif
		#if defined(ELEM5)
		if( *(volatile long *)(DSP_SECTION_STATUS + 16) )
		{
			kerSosFiltDirectForm2TransParaExtState(outb, coeffs, state, outa);
		}
		else
		{
			outa = outb;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM6)
		out2 = outa;
		#endif
		#endif
		#if defined(ELEM6)
		if( *(volatile long *)(DSP_SECTION_STATUS + 17) )
		{
			kerSosFiltDirectForm2TransParaExtState(outa, coeffs, state, outb);
		}
		else
		{
			outb = outa;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM7)
		out2 = outb;
		#endif
		#endif
		#if defined(ELEM7)
		if( *(volatile long *)(DSP_SECTION_STATUS + 18) )
		{
			kerSosFiltDirectForm2TransParaExtState(outb, coeffs, state, outa);
		}
		else
		{
			outa = outb;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}

		#if !defined(ELEM8)
		out2 = outa;
		#endif
		#endif
		#if defined(ELEM8)
		if( *(volatile long *)(DSP_SECTION_STATUS + 19) )
		{
			kerSosFiltDirectForm2TransParaExtState(outa, coeffs, state, out2);
		}
		else
		{
			out2 = outa;
			coeffs += COEFF_SKIP;
			state  += STATE_SKIP;
		}
		#endif
	  }
	  else
	  {
		  in2 = 0.0;
		  out2 = 0.0;
	  }
	  
	  {
#ifdef DSP_TARGET
		 out1 *= *(volatile float *)(DSP_PEQ_LEFT_GAIN_FILT);
	     out2 *= *(volatile float *)(DSP_PEQ_RIGHT_GAIN_FILT); 
	     out1 += in1 * *(volatile float *)(DSP_PEQ_DRY_GAIN_FILT);
	     out2 += in2 * *(volatile float *)(DSP_PEQ_DRY_GAIN_FILT);
#else
		 out1 *= *(volatile float *)(DSP_PEQ_LEFT_GAIN);
	     out2 *= *(volatile float *)(DSP_PEQ_RIGHT_GAIN); 
	     out1 += in1 * *(volatile float *)(DSP_PEQ_DRY_GAIN);
	     out2 += in2 * *(volatile float *)(DSP_PEQ_DRY_GAIN);
#endif
	  }
	  
	  dutilSetClipStatus(in1, in2, out1, out2, status);
	  
	  dutilPutOutputsAndMeter(out1, out2, status );

	  write_meters_and_status(in_meter1, in_meter2, out_meter1, out_meter2, status, transfer_state);
	}

	/* Write averaged meter data temporarily writing as a float.
	 * Will be converted to long and factored in calling function.
	 */
	write_meter_average();
} 
#endif


