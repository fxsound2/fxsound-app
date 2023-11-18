/* (C) COPYRIGHT 1994-2002 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: u_GraphicEq.h
 * DATE: 12/19/2002
 * AUTHOR: Mark Kaplan	
 * DESCRIPTION:
 *
 * Local header file for the GraphicEq module
 */
#ifndef _U_GRAPHIC_EQ_H_
#define _U_GRAPHIC_EQ_H_

#include "GraphicEq.h"
#include "sos.h"

#define GRAPHIC_EQ_MAX_NUM_BANDS SOS_MAX_NUM_SOS_SECTIONS

/* GraphicEq Handle definition */
struct GraphicEqHdlType
{
	CSlout *slout_hdl;
	wchar_t wcp_msg1[1024]; /* String for messages */
	int i_trace_mode;

	/* Main EQ settings */
	int num_bands;

	/* Frequencies of the highest and lowest bands in hz. */
	realtype min_band_freq;
	realtype max_band_freq;

	/* Filter Q setting */
	realtype Q;

	/* Current sampling frequency */
	realtype sampling_freq;

	/* SOS sections and parameters for each band */
	PT_HANDLE *sos_hdl;

	/* Flag to set mode where application has synching and warping of DFX Hyperbass and EQ Band1 controls. */
	bool app_has_hyperbass;
};

/* Local Functions */

/* GraphicEqInitSections.cpp */
int GraphicEq_InitSections(PT_HANDLE *);

#endif /* _U_GRAPHIC_EQ_H_ */