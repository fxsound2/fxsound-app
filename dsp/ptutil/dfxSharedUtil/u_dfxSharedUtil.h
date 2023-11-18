/* (C) COPYRIGHT 1994-2012 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.

/*
 * FILE: u_dfxSharedUtil.h
 * DATE: 1/9/2012
 * AUTOHOR: Mark Kaplan
 * DESCRIPTION:
 *
 * Local header file for the dfxSharedUtil module
 */

#ifndef _U_DFX_SHARED_UTIL_H_
#define _U_DFX_SHARED_UTIL_H_

#include <windows.h>
#include <stdio.h>

#include "codedefs.h"  
#include "slout.h"
#include "dfxSharedUtil.h"
#include "pt_defs.h"

#include "dfxSharedGlobals.h"

/* dfxSharedUtil Handle definition */
struct dfxSharedUtilHdlType {

   /* Initialization info */
   CSlout *slout_hdl;
   wchar_t wcp_msg1[1024]; /* String for messages */
   char cp_msg1[1024]; /* String for messages */
	int trace_mode; /* IS_TRUE if trace is on */

	/* Shared memory */
	HINSTANCE shared_memory_hinst;
	struct dfxSharedGlobalsType *sp_shared_memory_data;
};

/* Local function definitions */

/* dfxSharedUtil.cpp */
int dfxSharedUtil_RegistryGetTopSharedFolderPath(PT_HANDLE *, wchar_t *);

#endif //_U_DFX_SHARED_UTIL_H_
