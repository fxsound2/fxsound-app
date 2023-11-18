/* (C) COPYRIGHT 1994-2002 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: u_BinauralSyn.h
 * DATE: 10/12/2010
 * AUTHOR: Paul Titchener	
 * DESCRIPTION:
 *
 * Local header file for the BinauralSyn module
 */
#ifndef _U_BINAURAL_SYN_H_
#define _U_BINAURAL_SYN_H_

#include "BinauralSyn.h"

/* BinauralSyn Handle definition */
struct BinauralSynHdlType
{
	int num_coeffs;

	int sample_index;		// Holds internal sample circular buffer index, higher index means older samples

	int s_ratio;			// Holds sample skipping ratio used for high sampling frequencies

	int s_index;			// Holds sample skipping index used for high sampling frequencies

	int last_samp_freq;  // Holds the last used sampling frequency, used to detect rate changes

	int internal_samp_rate_flag;	// The actual internally used sampling rate, sets the coeffs to use.
											// Either BINAURAL_SYN_COEFF_SAMP_RATE_44_1 or BINAURAL_SYN_COEFF_SAMP_RATE_48

	realtype last_left;	// Used for sample repeating
	realtype last_right;

	realtype LFsamples[BINAURAL_SYN_MAX_NUM_COEFFS];	// Used for left front channel
	realtype RFsamples[BINAURAL_SYN_MAX_NUM_COEFFS];	// Used for right front channel
	realtype LRsamples[BINAURAL_SYN_MAX_NUM_COEFFS];	// Used for left rear channel
	realtype RRsamples[BINAURAL_SYN_MAX_NUM_COEFFS];	// Used for right rear channel
	realtype LSsamples[BINAURAL_SYN_MAX_NUM_COEFFS];	// Used for left side channel
	realtype RSsamples[BINAURAL_SYN_MAX_NUM_COEFFS];	// Used for right side channel
};

#endif /* _U_BINAURAL_SYN_H_ */