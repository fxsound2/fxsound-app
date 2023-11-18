/* (C) COPYRIGHT 1994-2012 Power Technology. All Rights Reserved. 
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * 
 * FILE: u_sndDevices.h
 * DATE: 7/17/2012
 * AUTOHOR: Mark Kaplan
 * DESCRIPTION:
 *
 * Local header file for the sndDevices module
 */

#ifndef _U_SND_DEVICES_H_
#define _U_SND_DEVICES_H_

#include "codedefs.h"
#include "slout.h"
#include "pt_defs.h"
#include "sndDevices.h"

// This must be tuned along with SND_DEVICES_CAPTURE_BUFFER_SIZE_SECS to give minimum delay with minimum glitching
// On my slow Win7 PC, 0.2 delay and ratio of 2 works pretty well.
#define SND_DEVICES_MAX_TO_MIN_PLAYBACK_SIZE_RATIO 2 // Sets ratio of max buffer size to min allowed playback buffer size

/* Operation returns */
#define SND_DEVICES_SETUP_OK 0
#define SND_DEVICES_SETUP_NO_DFX_DEVICES_FOUND 1
#define SND_DEVICES_SETUP_ONLY_TWO_DEVICES_FOUND 2

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }
#endif

// Macro to simplify slout calls
#define SLOUT_FIRST_LINE(x) if((cast_handle->i_trace_on) && (cast_handle->slout_hdl))\
{swprintf(cast_handle->wcp_msg1, (x)); cast_handle->slout_hdl->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);}

/* Local functions */

/* sndDevices_GetAll.cpp */
int PT_DECLSPEC sndDevices_GetAll( PT_HANDLE *, int *);
                             
/* sndDevicesInit */
int PT_DECLSPEC sndDevices_FreeReuseableObjects(PT_HANDLE *);
int PT_DECLSPEC sndDevices_StopAndReleaseAllAudioObjects(PT_HANDLE *);

/* sndDevices_Utils.cpp */
int PT_DECLSPEC sndDevices_UtilsGetIndexFromID(PT_HANDLE *, wchar_t *, int *);
int PT_DECLSPEC sndDevices_UtilsGetIndexFromType(PT_HANDLE *, int, int *);
int PT_DECLSPEC sndDevices_UtilsTerminateEncodingThreads(PT_HANDLE *, int *);

#endif //_U_SND_DEVICES_H
