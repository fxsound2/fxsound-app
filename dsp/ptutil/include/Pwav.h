/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: pwav.h
 * DATE: 1/29/97
 * AUTOHOR: Mark Kaplan
 * DESCRIPTION:
 *
 *  Public defines for the pwav module
 */  
#ifndef _PWAV_H_
#define _PWAV_H_

#include "slout.h"  

/* Max and min buffer sizes in samples */
#define PWAV_MIN_BUFFER_SIZE 2048
#define PWAV_MAX_BUFFER_SIZE 10666

/* pwav.cpp */
int PT_DECLSPEC pwavNew(PT_HANDLE **, CSlout *, long, DWORD, int); 
int PT_DECLSPEC pwavResetBufferSize(PT_HANDLE *, long, int);
int PT_DECLSPEC pwavReadHeader(PT_HANDLE *, wchar_t *, wchar_t *, int, int, int *, long *, long *, int *);
int PT_DECLSPEC pwavReadNextBuffer(PT_HANDLE *, int, long **, long *, long *, int, int);  
int PT_DECLSPEC pwavGetBuffer(PT_HANDLE *, int, long **);
int PT_DECLSPEC pwavGetTempFloatBuffer(PT_HANDLE *, realtype **);
int PT_DECLSPEC pwavPlayBuffer(PT_HANDLE *, int, long); 
int PT_DECLSPEC pwavGetDoneReading(PT_HANDLE *, int *);
int PT_DECLSPEC pwavStop(PT_HANDLE *, int);
int PT_DECLSPEC pwavCheckLegalFiles(PT_HANDLE *, wchar_t *, wchar_t *, int, int *);
int PT_DECLSPEC pwavFreeUp(PT_HANDLE **);
int PT_DECLSPEC pwavDump(PT_HANDLE *); 

/* pwavConvert.cpp */
int PT_DECLSPEC pwav24BitToFloat(char *cp_24_bit, realtype *rp_float,
							int i_length, int i_stereo_flag);
int PT_DECLSPEC pwavFloatTo24Bit(realtype *rp_float, char *cp_24_bit,
							int i_length, int i_stereo_flag);

/* pwavIO.cpp */
int PT_DECLSPEC pwavSetupReadFromInput(PT_HANDLE *hp_pwav, int i_num_channels_in,
						   int i_num_channels_out, realtype r_samples_per_sec,
						   int i_wave_in_dev, int i_wave_out_dev, int *ip_24_bit_flag);
int PT_DECLSPEC pwavStopIO(PT_HANDLE *hp_pwav);
int PT_DECLSPEC pwavReadNextIOBuffer(PT_HANDLE *hp_pwav, long **lpp_buffer_data);
int PT_DECLSPEC pwavPlayIOBuffer(PT_HANDLE *hp_pwav, long *lp_buffer_data);
int PT_DECLSPEC pwavQueueIOBuffer(PT_HANDLE *hp_pwav);
int PT_DECLSPEC pwavStartIOProcessing(PT_HANDLE *hp_pwav);

/* pwavUtil */
int PT_DECLSPEC pwavCheckIfLegalSampFreq(long, int *);

#endif //_PWAV_H
