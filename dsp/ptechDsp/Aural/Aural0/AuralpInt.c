/* (C) COPYRIGHT 1994-2000 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */

/*
 * FILE: auralpIntX.c 
 * DATE: 7/16/98
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: Aural Enhancer, Special Integer 16 bit data version.
 * Note that this version also implements odd harmonics only since
 * they are sufficient for use in portable MP3 playback systems.
 *
 */

#ifndef DSPSOFT_TARGET
#define DSP_TARGET
#endif
#include "aur_num.h"  /* Sets number of elements */

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

/* Used in implementation of integer operations in DSP code */
/* Current values assume 16 bit data, coefficients and storage
 * with 32 temporary registers available. Multiplies should use
 * saturating arithmetic.
 */

/* --- DSP PRECISION DEFINES --- */
/* Number of bits used in most storage and coefficient values */
#define NUM_DSP_BITS 16

/* Maximum positive 2's complement number represented by NUM_DSP_BITS.
 * This value must be correctly modified if NUM_DSP_BITS is modified
 */
#define MAX_INT_VAL 32768

/* Number of bits that are used in registers to store temporary
 * values during complex calculations. This value must be chosen to
 * allow headroom for additional summations into the register given
 * the bit size of the register (typically 32).
 */
#define NUM_INTERMEDIATE_BITS 24

/* Shift applied to restore multiply results to intermediate precision */
#define DSP_INTERMEDIATE_SHIFT (NUM_DSP_BITS - (NUM_INTERMEDIATE_BITS - NUM_DSP_BITS))

/* This shift is used to pre-shift coefficients to lesser precision
 * than the base value, allowing more precision to be kept in the
 * other multiply operand. It works well in coefficients such as
 * gain settings where some loss of bit precision has no effect
 * on the sound quality.
 * IMPORTANT - COEFF_REDUCE_SHIFT MUST BE <= DSP_FINAL_SHIFT
 */
#define COEFF_REDUCE_SHIFT 6

/* Shift applied to convert an intermediate precision value to final
 * NUM_DSP_BITS precision. Note that final shift is one less than the
 * sum of the bit width and the intermediate shift.
 */
#define DSP_FINAL_SHIFT (NUM_DSP_BITS - DSP_INTERMEDIATE_SHIFT - 1)

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


	long i;
	struct dspAuralStructType *s = (struct dspAuralStructType *)(COMM_MEM_OFFSET);
 
	if( i_init_flag & DSPS_INIT_PARAMS )
	{
		/* Initialization values from Quick preset 2 - Vintage Rock Booster, 44khz */
		s->dry_gain = (realtype)0.338583;	  
		s->wet_gain = (realtype)0.661417;
		s->aural_drive = (realtype)1.40259;
		s->aural_odd = (realtype)1.5;
		/* Next 3 are samp_freq dependent, need to be reset for other than 44.1kHz */
		s->gain = 0.797495;
		s->a1 = 1.55355;
		s->a0 = -0.636428;

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
	struct dspAuralStructType *s = (struct dspAuralStructType *)(COMM_MEM_OFFSET);
	int i;

	/* Set integer coefficient values. */
	/* Note drive value can exceed 2.0 so will scale integer assignment and 
	 * compensate later in the nonlinearity generation.
	 * A precision reduced drive value is also used as detailed below. */
	short int i_drive = ((short int)(s->aural_drive * MAX_INT_VAL/4)) >> (COEFF_REDUCE_SHIFT);

	/* Note odd values exceeds 1.0 so need to compensate later by one less shift.
	 * Also using coeff precision reduction but with 1 shift less than standard
	 * since the result already has a -1 post-shift reduction
	 */
	short int i_odd = ((short int)(s->aural_odd * MAX_INT_VAL/2)) >> (COEFF_REDUCE_SHIFT - 1);

	/* Note a1 value exceeds 1.0 so need to compensate later by one less shift */
	short int i_a1 = s->a1 * MAX_INT_VAL/2;

	short int i_a0 = s->a0 * MAX_INT_VAL;
	short int i_gain = s->gain * MAX_INT_VAL;

	/* Declare and initialize integer storage values */
	static short int in1_minus1 = 0;
	static short int in1_minus2 = 0;
	static short int in2_minus1 = 0;
	static short int in2_minus2 = 0;
	static short int out1_minus1 = 0;
	static short int out1_minus2 = 0;
	static short int out2_minus1 = 0;
	static short int out2_minus2 = 0;

	/* Run one buffer full of data. Zero index and averaged meter vals. */

	read_in_buf = lp_data;
	read_out_buf = lp_data;

	for(i=0; i<l_length; i++)
	{
		float out1, out2;
		float in1, in2;
		float odd1, odd2, filtH1, filtH2;
		volatile long in_count = 0;
		volatile long out_count = 0;
		int error = 0;

		/* Declare the integer temporary variables used in algorithm */
		short int i_in1, i_in2;
	  
		load_parameter(); /* If its been sent, loads a parameter into memory */

		/* Next line includes kerdmaru.h run loop macro (no args) */
		#include "kerdmaru.h"
	  
		out1 = out2 = 0.0;
		 
		dutilGetInputsAndMeter( in1, in2, status);

		i_in1 = (short int)(in1 * MAX_INT_VAL);
		i_in2 = (short int)(in2 * MAX_INT_VAL);

		/* Implement Highpass linear transformed 2nd order Butterworth filter */
		{
			long wideReg1; /* A temporary 32 bit register value */

			/* wideReg1 will be used to maintain temporary values at a higher
			 * bit precision than the native data and coefficients.
			 */
			/* After multiply operations we will shift back to the intermediate
			 * bit precision. This precision is chosen to allow headroom for a
			 * additional summation operations.
			 */
			/* Note that 16 bit data and coeffs are cast to longs prior to operations
			 * to generate 32 bit results.
			 */
			/* Note s->a1 is > 1.0, compensate with one less post-op shift */
			wideReg1 = ( ((long)out1_minus1 * (long)i_a1) >> (DSP_INTERMEDIATE_SHIFT - 1) );

			wideReg1 += ( ((long)out1_minus2 * (long)i_a0) >> DSP_INTERMEDIATE_SHIFT );

			out1_minus2 = out1_minus1;

			wideReg1 += ( 
				           (  (long)i_in1 
							   + (long)in1_minus2 
				            - ( ((long)in1_minus1) << 1)  /* Mult this value by 2 using shift */
						     ) * (long)i_gain
						   ) >> DSP_INTERMEDIATE_SHIFT;

			/* Convert wideReg1 back to 16 value for storage in out1_minus1 variable */
			out1_minus1 = wideReg1 >> DSP_FINAL_SHIFT;
			in1_minus2 = in1_minus1;
			in1_minus1 = i_in1;

			/* Note that to allow maintaining max precision in the wideReg1 value in the following
			 * drive multiplication, the drive value is precision reduced when set above.
			 * This works well since the loss of bit precision of the drive value setting does not
			 * affect the sound quality.
			 * Note that the drive value is reduced by a factor of four which will be
			 * added back after the non-linearity generation.
			 */

			wideReg1 = (wideReg1 >> (DSP_FINAL_SHIFT - COEFF_REDUCE_SHIFT));
			wideReg1 = wideReg1 * (long)i_drive;
			wideReg1 = wideReg1 >> (DSP_INTERMEDIATE_SHIFT);

			/* Refer to floating point implementation to see specifics of algorithm below.
			 * Note that the algorithm is modified to generate the correct non-linearity
			 * with a factor of 4 gain added after the generation.
			 */
			{
				long wideReg2;

				/* Prepare to multiply */
				wideReg2 = wideReg1 >> (DSP_FINAL_SHIFT);
				wideReg2 = wideReg2 * (wideReg1 >> (DSP_FINAL_SHIFT));

				/* Note that the coeff below which normally has a value of 1/6 is adjusted
				 * for the fact that a factor of 4 was taken out of the drive value above,
				 * to be added in after the non-linearity generation. The new coeff value
				 * becomes 8/3 = 4 x 2/3.
				 */
				/* Implement the 4 factor of the coeff mult below by reducing shift by 2 */
				wideReg2 = wideReg2 >> (NUM_DSP_BITS - 2);

				/* Note that coeff value is corrected for the fact that a gain of 4 was pulled
				 * out from drive value above and also coeff bit precision reductions was used..
				 */
				wideReg2 = wideReg2 * (((long)((double)MAX_INT_VAL * 2.0/3.0)) >> (COEFF_REDUCE_SHIFT));

				wideReg2 = wideReg2 >> (DSP_INTERMEDIATE_SHIFT - COEFF_REDUCE_SHIFT);

				wideReg1 = wideReg1 - wideReg2;
			}

			/* Note odd value is greater than one so compensate with one less pre-shift.
			 * Also note that odd value uses a reduced coeff precision reduction since some
			 * post-shift reduction already occurs
			 */
			wideReg1 = wideReg1 >> (DSP_FINAL_SHIFT - (COEFF_REDUCE_SHIFT - 1) - 1);

			wideReg1 = wideReg1 * (long)i_odd;

			/* Put back gain factor 4 that was taken out of the drive above */
			wideReg1 = wideReg1 >> (DSP_INTERMEDIATE_SHIFT - 2);

			/* Note that required 4 gain is placed back in wideReg1 by reducing pre-shift by 2 */
			/* Note odd value is greater than one so compensate with one less pre-shift.
			 * Also note that odd value uses a reduced coeff precision reduction since some
			 * post-shift reduction already occurs
			 */
			wideReg1 = wideReg1 + ( ((long)i_in1) << DSP_FINAL_SHIFT );
			
			/* Shift to final 16 bit precision, round if available rather than simple shift truncation. */
			wideReg1 = wideReg1 >> (DSP_FINAL_SHIFT);

			/* Saturate wideReg1 to final 16 bit value to prepare for final output. */
			{
				if( wideReg1 >= MAX_INT_VAL )
					wideReg1 = MAX_INT_VAL - 1;
				if( wideReg1 < (-MAX_INT_VAL) )
					wideReg1 = - MAX_INT_VAL;
			}

			/* Float conversion is only for test program. */
			out1 = (float)wideReg1 * ((float)1.0/(float)MAX_INT_VAL);
		}

		/* Do right channel if stereo in */
		if(s->stereo_in_flag)
		{
			long wideReg1; /* A temporary 32 bit register value */

			/* wideReg1 will be used to maintain temporary values at a higher
			 * bit precision than the native data and coefficients.
			 */
			/* After multiply operations we will shift back to the intermediate
			 * bit precision. This precision is chosen to allow headroom for a
			 * additional summation operations.
			 */
			/* Note that 16 bit data and coeffs are cast to longs prior to operations
			 * to generate 32 bit results.
			 */
			/* Note s->a1 is > 1.0, compensate with one less post-op shift */
			wideReg1 = ( ((long)out2_minus1 * (long)i_a1) >> (DSP_INTERMEDIATE_SHIFT - 1) );

			wideReg1 += ( ((long)out2_minus2 * (long)i_a0) >> DSP_INTERMEDIATE_SHIFT );

			out2_minus2 = out2_minus1;

			wideReg1 += ( 
				           (  (long)i_in2 
							   + (long)in2_minus2 
				            - ( ((long)in2_minus1) << 1)  /* Mult this value by 2 using shift */
						     ) * (long)i_gain
						   ) >> DSP_INTERMEDIATE_SHIFT;

			/* Convert wideReg1 back to 16 value for storage in out1_minus1 variable */
			out2_minus1 = wideReg1 >> DSP_FINAL_SHIFT;
			in2_minus2 = in2_minus1;
			in2_minus1 = i_in2;

			/* Note that to allow maintaining max precision in the wideReg1 value in the following
			 * drive multiplication, the drive value is precision reduced when set above.
			 * This works well since the loss of bit precision of the drive value setting does not
			 * affect the sound quality.
			 * Note that the drive value is reduced by a factor of four which will be
			 * added back after the non-linearity generation.
			 */

			wideReg1 = (wideReg1 >> (DSP_FINAL_SHIFT - COEFF_REDUCE_SHIFT));
			wideReg1 = wideReg1 * (long)i_drive;
			wideReg1 = wideReg1 >> (DSP_INTERMEDIATE_SHIFT);

			/* Refer to floating point implementation to see specifics of algorithm below.
			 * Note that the algorithm is modified to generate the correct non-linearity
			 * with a factor of 4 gain added after the generation.
			 */
			{
				long wideReg2;

				/* Prepare to multiply */
				wideReg2 = wideReg1 >> (DSP_FINAL_SHIFT);
				wideReg2 = wideReg2 * (wideReg1 >> (DSP_FINAL_SHIFT));

				/* Note that the coeff below which normally has a value of 1/6 is adjusted
				 * for the fact that a factor of 4 was taken out of the drive value above,
				 * to be added in after the non-linearity generation. The new coeff value
				 * becomes 8/3 = 4 x 2/3.
				 */
				/* Implement the 4 factor of the coeff mult below by reducing shift by 2 */
				wideReg2 = wideReg2 >> (NUM_DSP_BITS - 2);

				/* Note that coeff value is corrected for the fact that a gain of 4 was pulled
				 * out from drive value above and also coeff bit precision reductions was used..
				 */
				wideReg2 = wideReg2 * (((long)((double)MAX_INT_VAL * 2.0/3.0)) >> (COEFF_REDUCE_SHIFT));

				wideReg2 = wideReg2 >> (DSP_INTERMEDIATE_SHIFT - COEFF_REDUCE_SHIFT);

				wideReg1 = wideReg1 - wideReg2;
			}

			/* Note odd value is greater than one so compensate with one less pre-shift.
			 * Also note that odd value uses a reduced coeff precision reduction since some
			 * post-shift reduction already occurs
			 */
			wideReg1 = wideReg1 >> (DSP_FINAL_SHIFT - (COEFF_REDUCE_SHIFT - 1) - 1);

			wideReg1 = wideReg1 * (long)i_odd;

			/* Put back gain factor 4 that was taken out of the drive above */
			wideReg1 = wideReg1 >> (DSP_INTERMEDIATE_SHIFT - 2);

			/* Note that required 4 gain is placed back in wideReg1 by reducing pre-shift by 2 */
			/* Note odd value is greater than one so compensate with one less pre-shift.
			 * Also note that odd value uses a reduced coeff precision reduction since some
			 * post-shift reduction already occurs
			 */
			wideReg1 = wideReg1 + ( ((long)i_in2) << DSP_FINAL_SHIFT );
			
			/* Shift to final 16 bit precision, round if available rather than simple shift truncation. */
			wideReg1 = wideReg1 >> (DSP_FINAL_SHIFT);

			/* Saturate wideReg1 to final 16 bit value to prepare for final output. */
			{
				if( wideReg1 >= MAX_INT_VAL )
					wideReg1 = MAX_INT_VAL - 1;
				if( wideReg1 < (-MAX_INT_VAL) )
					wideReg1 = - MAX_INT_VAL;
			}

			/* Float conversion only for test program */
			out2 = (float)wideReg1 * ((float)1.0/(float)MAX_INT_VAL);
		}
#ifdef UNDEF
		if(s->stereo_in_flag)
		{
			/* Implement a second order lowpass filter on the signal */
			filtH2 = s->out2_minus1 * s->a1 + s->out2_minus2 * s->a0;
			s->out2_minus2 = s->out2_minus1;
			/* Add bias to avoid zero input cpu load increases */
			filtH2 += (in2  + 1.0e-30 - (realtype)2.0 * s->in2_minus1 + s->in2_minus2) * s->gain;
			s->out2_minus1 = filtH2;
			s->in2_minus2 = s->in2_minus1;
			s->in2_minus1 = in2;

			filtH2 *= s->aural_drive;

			/* Generate the harmonics */
			/* odd2 = (realtype)sin(filtH2); */
			/* Note that this approximation works extremely well for this usage
			 * if the input does not exceed 1.0
			 */
			odd2 = filtH2 - (filtH2 * filtH2 * filtH2) * (realtype)(1.0/6.0);

			out2 = in2 + s->aural_odd * odd2;
		}
#endif
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

