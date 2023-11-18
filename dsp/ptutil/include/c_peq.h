/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: c_peq.h
 * DATE: 7/2/96
 * AUTHOR: Mark Kaplan
 * DESCRIPTION:
 *
 *  Public defines to be used for communicating the parametric EQ apps
 *  parameters between the PC and the Dsp card.
 */
#ifndef _C_PEQ_H_
#define _C_PEQ_H_             

/* This is the number of parametric sections */
#define DSP_MAX_NUM_OF_PEQ_ELEMENTS 8 

/* Each shelf section has b0, b1, b2, a1, a2 */
/* Each para section has b0, b1, b2, a2 (a1=b1) */
#define DSP_PEQ_SOS_SHELF_COEFF_OFFSET 5
#define DSP_PEQ_SOS_PARA_COEFF_OFFSET 4

/* Originally 1.0/512, too slow with all 10 left sos sections */
/* Original setting with one filtering operation per loop was 1.0/64.0 */
/* #define PEQ_ALPHA 1.0/64.0  This was with 2 filtering ops */
/* NOTE- TRYING MORE THAN 128.0 GAVE PRECISION PROBLEMS on PEQ */
#define PEQ_ALPHA 1.0/128.0 

/* Defines for the SOS sections */
#define PEQ_LOW_SHELF_SECTION  0
#define PEQ_HIGH_SHELF_SECTION 1
#define PEQ_BAND1_SECTION      2  

/* Constants */
#define DSP_PEQ_MIN_FREQ_VALUE 0.0
#define DSP_PEQ_MAX_FREQ_VALUE 100.0

/* Next is total number of filtered parameters in the parameter section.
 * Includes input gains and all sos section parameters for both left and right.
 * Currently 4*5 + 8*2*4 + 3 = 87
 */
#define DSP_PEQ_NUM_FILTER_PARAMETERS (4*DSP_PEQ_SOS_SHELF_COEFF_OFFSET + (DSP_MAX_NUM_OF_PEQ_ELEMENTS)*2*DSP_PEQ_SOS_PARA_COEFF_OFFSET + 3)

/* Algorithm specific parameters */

/* Flag to turn on SHELFS, a long */
#define DSP_PEQ_SHELFS_ON 			19L + COMM_MEM_OFFSET
/* First the input parameter locations */
#define DSP_PEQ_LEFT_GAIN 			20L + COMM_MEM_OFFSET
#define DSP_PEQ_RIGHT_GAIN 			21L + COMM_MEM_OFFSET
#define DSP_PEQ_DRY_GAIN			22L + COMM_MEM_OFFSET
#define DSP_SECTION_INPUT_START 	23L + COMM_MEM_OFFSET

/* Now the filter parameter locations. Total parameter space = 20 + 2*87 = 194 < 0xD0 */
#define DSP_PEQ_LEFT_GAIN_FILT 		DSP_PEQ_LEFT_GAIN + DSP_PEQ_NUM_FILTER_PARAMETERS
#define DSP_PEQ_RIGHT_GAIN_FILT 	DSP_PEQ_RIGHT_GAIN + DSP_PEQ_NUM_FILTER_PARAMETERS
#define DSP_PEQ_DRY_GAIN_FILT  		DSP_PEQ_DRY_GAIN + DSP_PEQ_NUM_FILTER_PARAMETERS
#define DSP_SECTION_FILT_START 		DSP_SECTION_INPUT_START + DSP_PEQ_NUM_FILTER_PARAMETERS

/* Added section on off flags, 20 flags total. Currently starts at 209 */
/* NOTE- these aren't used in DSP version, and we must restrict parameter writing
 * so the PC doesn't write these to the DSP version.
 */
#define DSP_SECTION_STATUS			DSP_SECTION_FILT_START + DSP_PEQ_NUM_FILTER_PARAMETERS

/* The offsets used to write to left and right coeffs.
 * Note that first we have the low and high shelf sections,
 * then the para sections
 */
#define DSP_PEQ_LEFT_OFFSET  DSP_SECTION_INPUT_START
#define DSP_PEQ_RIGHT_OFFSET DSP_SECTION_INPUT_START + 2*DSP_PEQ_SOS_SHELF_COEFF_OFFSET + (DSP_MAX_NUM_OF_PEQ_ELEMENTS) * DSP_PEQ_SOS_PARA_COEFF_OFFSET

#endif
/* _C_PEQ_H_ */
