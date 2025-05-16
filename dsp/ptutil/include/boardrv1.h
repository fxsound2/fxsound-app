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
#ifndef _BOARDRV1_H_
#define _BOARDRV1_H_

/* Defines to set DSPFX or DFX build type */
#define PT_DSP_DSPFX 1
#define PT_DSP_DFX 2

/* Board Dependent Defines for parameter address offsets */
/**********************/
/*   DSPFX1 Defines   */
/**********************/
#ifdef PC_TARGET /* For building Mdly */
	#ifdef DSPFX1
		#define COMM_MEM_BASE_ADDR 0x87fe00 /* DSP parameter base memory address
											 * PARAMS (fast internal mem), len = 0xC0
											 */
		#define COMM_MEM_OFFSET 0	/* 0 allows PC to index array vals */
	#endif

#elif defined(DSP_TARGET) /* For building hardware DSP algorithms */
	#ifdef DSPFX1
		#define COMM_MEM_BASE_ADDR 0x87fe00 /* DSP parameter base memory address
											 * PARAMS (fast internal mem), len = 0xC0
											 */
		#define COMM_MEM_OFFSET COMM_MEM_BASE_ADDR   /* Allows DSP to reference mem directly */
	#endif

#elif defined(DSPSOFT_TARGET)	/* For building and libs linking to PC DSP algorithms */
		/* float *COMM_MEM_OFFSET; MOVED INTO CODE .C FILES */
#else
	#error PC_TARGET or DSP_TARGET or DSPSOFT_TARGET not defined
#endif

#ifdef DSPFX1
#define C32_IO_ADR 0x82FFF0
#define SER_0_ADR       0x808040
#define SER_0_XMIT		0x808048
#define SER_0_RCV		0x80804C
#define DEF_SER_CTL     0x0C3C0000 /* serial port default control word */
/* DMA defines */        
#define DMA0_CONTROL	 0x808000
#define DMA0_SOURCE 	 0x808004 /* Destination for DMA transfer */
#define DMA0_DESTINATION 0x808006 /* Destination for DMA transfer */
#define DMA0_COUNTER	 0x808008 /* Counter for DMA transfer */
#define DMA1_CONTROL	 0x808010
#define DMA1_SOURCE 	 0x808014 /* Destination for DMA transfer */
#define DMA1_DESTINATION 0x808016 /* Destination for DMA transfer */
#define DMA1_COUNTER	 0x808018 /* Counter for DMA transfer */

/* NOTE - Buffer size is in 32 bit longs. */
#define DSP_DMA_WAV_BUF_SIZE 16000L
#define DSP_TOTAL_DMA_WAV_BUF_SIZE (4 * DSP_DMA_WAV_BUF_SIZE)
/* This needs to be 16 minus the binary shift amount equal to the number
 * of samples in the buffer.
 */
#define DSP_DMA_METER_SHIFT (16 - 14)

#define DMA_OFF_MODE 0L
#define DMA_IN_MODE  1L
#define DMA_OUT_MODE 2L

#endif /* DSPFX1 */

/* DSP to PC Status related defines */ 
/* Max float representation of <<16 left
 * shifted 16 bit integer
 */                
/* Examining the converted float values in the debugger, you sometimes
 * get +/- 2.1474836 but often get 2.1474181
 */
#define DSP_FLOAT_OUTPUT_CLIP_LEVEL 2.1474181e+9 

#define FLOAT_OUTPUT_CLIP_LEVEL 2.1474836e+9
#define OUTPUT_CLIP_LEVEL 2147418112.0

/* Note that clipped longs received from Crystal 16 bit part via a DSP float
 * variable are +2147418112 -2147483648, as contrasted to 
 * max long values +/- 2147483647 defined in limit.h .
 */
#define LONG_OUTPUT_CLIP_LEVEL_CRYSTAL 2147418122L

/* The clipped negative long number from the cystal 16 bit part.
 * Note that this is the max neg legal long - 1 (-2147483648 or 0x80000000).
 */
#define LONG_MAX_NEGATIVE_CRYSTAL 0x80000000

/* PC longs wrap around when assigning float vals over the
 * clip point, so need these defines to implement
 * saturation clipping.
 */
#define PC_24BIT_FLOAT_PLUS_CLIP   2.147483392e9 /* (float)0x7fffff00 */
#define PC_24BIT_FLOAT_MINUS_CLIP -2.147483392e9 /* (float)0x80000100 */

#ifdef DSPSOFT_TARGET
/* 1.0e-36 seems to fix reverb (smallest room, dryest settings).
 * EQ required 1.0e-34 when on high q settings
 */
#define DSP_DENORM_BIAS 1.0e-36;
#define DSP_DENORM_BIAS_LARGE 1.0e-34;
#endif /* PC_TARGET */

/*
 * These are the offsets from the base address of the dual port memory
 * where the different parameters common to all algorithms will be placed.
 * NOTE- COMM_MEM_OFFSET value will typically be different on PC vs. DSP
 * chip to allow appropriate access methods (array indexing on PC, direct
 * addressing on DSP)
 */

#ifdef UNDEF /* FOR NEW STRUCTURE BASED METHOD */
#define PC_TO_DSP_FLAGS        f_pars->m0
#define DSP_TO_PC_FLAGS        f_pars->m1
#define DSP_NUMBER_OF_ELEMENTS f_pars->m2
#define DSP_SAMPLING_FREQ      f_pars->m3
#define DSP_STEREO_IN_FLAG     f_pars->m4
#define DSP_MUTE_IN_FLAG       f_pars->m5

#define DRY_GAIN    f_pars->m10
#define WET_GAIN    f_pars->m11
#define MASTER_GAIN f_pars->m12
#define DSP_DMA_IN_TRANSFER f_pars->m13
#endif

/* PC_TARGET case */
#define PC_TO_DSP_FLAGS        0 + COMM_MEM_OFFSET
#define DSP_TO_PC_FLAGS        1 + COMM_MEM_OFFSET
#define DSP_NUMBER_OF_ELEMENTS 2 + COMM_MEM_OFFSET
#define DSP_SAMPLING_FREQ      3 + COMM_MEM_OFFSET
/* NOTE - Location of stereo in flag index  is used in
 * play plug-in to replicate its value for all the
 * multiple dsp functions. Changing it here will require
 * also fixing up C_play.h
 */
#define DSP_STEREO_IN_FLAG     4 + COMM_MEM_OFFSET
#define DSP_MUTE_IN_FLAG       5 + COMM_MEM_OFFSET

#define DRY_GAIN    10L + COMM_MEM_OFFSET
#define WET_GAIN    11L + COMM_MEM_OFFSET
#define MASTER_GAIN 12L + COMM_MEM_OFFSET
#define DSP_DMA_IN_TRANSFER 13L + COMM_MEM_OFFSET

#endif
/* _BOARDRV1_H */

