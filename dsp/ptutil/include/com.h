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
#ifndef _COM_H_
#define _COM_H_

#include "codedefs.h"
#include "pt_defs.h"
#include "slout.h"

#define PC_TARGET
#include "platform.h"
#include "boardrv1.h"

/* Name of dsp executable for during initalization */
#define COM_DSP_INIT_EXE_NAME "cominit.a"                   
                   
/* The following are the different types of effects. */
#define COM_PASSWORD_STUDIO_SYSTEM    1
#define COM_PASSWORD_REVERB           2
#define COM_PASSWORD_DELAY            3
#define COM_PASSWORD_CHORUS           4
#define COM_PASSWORD_FLANGE           5
#define COM_PASSWORD_PITCH            6 
#define COM_PASSWORD_PEQ              7  
#define COM_PASSWORD_LEADSYN          8    
#define COM_PASSWORD_PAN              9
#define COM_PASSWORD_TREMOLO         10
#define COM_PASSWORD_COMPRESSOR      11
#define COM_PASSWORD_APITCH          12
#define COM_PASSWORD_AURAL           13
#define COM_PASSWORD_MAXIMIZE        14
#define COM_PASSWORD_LEX             15
#define COM_PASSWORD_STUDIO_SYSTEM_2 16 /* STUDIO_SYSTEM + LEX */
#define COM_PASSWORD_WID             17 
#define COM_PASSWORD_PLY             18 
#define COM_PASSWORD_STUDIO_SYSTEM_3 19 /* STUDIO_SYSTEM_2 + WIDENER */
#define COM_PASSWORD_GVB             20

/* For communicating tolerance of AES sampling frequencies */
#define COM_SAMP_FREQ_TOLERANCE_UNDEFINED 	0
#define COM_SAMP_FREQ_TOLERANCE_400PPM 		1
#define COM_SAMP_FREQ_TOLERANCE_4_PERCENT	2

/* For controlling data reading methods */
#define COM_READ_IF_READY   0
#define COM_WAIT_FOR_VALUE  1

/* Serial number if there is no card or dongle */
#define COM_NO_SERIAL_NUMBER -1L

/* com.cpp */
int PT_DECLSPEC comInit(PT_HANDLE **, int, int, int, long, char *, int, CSlout *);
int PT_DECLSPEC comLoadAndRun(PT_HANDLE *, char *, char *, realtype, int, int, int, 
                  realtype *, int *, int *);
int PT_DECLSPEC comSoftDspLoadAndRun(PT_HANDLE *, char *, realtype, int, short, int);
int PT_DECLSPEC comSoftDspLoadAndRunNonShared(PT_HANDLE *, char *, realtype, int, short);
int PT_DECLSPEC comSoftDspZeroMemory(PT_HANDLE *);
int PT_DECLSPEC comFreeUp(PT_HANDLE **);
int PT_DECLSPEC comDump(PT_HANDLE *);
int PT_DECLSPEC comTurnOff(PT_HANDLE *); 
int PT_DECLSPEC comTurnOn(PT_HANDLE *);
int PT_DECLSPEC comSetDebugMode(PT_HANDLE *, int, CSlout *);  
int PT_DECLSPEC comIsLegalSampFreq(realtype, int, int *); 
int PT_DECLSPEC comSetBufferSize(PT_HANDLE*, long); 
int PT_DECLSPEC comCheckCardExists(int, int *);

/* comGet.cpp */
int PT_DECLSPEC comGetCardExists(PT_HANDLE *, int *); 
int PT_DECLSPEC comGetDongleExists(PT_HANDLE *, int *); 
int PT_DECLSPEC comGetAesExists(PT_HANDLE *, int *);  
int PT_DECLSPEC comGetSerialNum(PT_HANDLE *, unsigned long *);
int PT_DECLSPEC comGetMainNumSamples(PT_HANDLE *, long *);
int PT_DECLSPEC comGetExpandedNumSamples(PT_HANDLE *, long *);
int PT_DECLSPEC comGetHasBeenCracked(PT_HANDLE *, int, int *);
int PT_DECLSPEC comGetProcessorIndex(PT_HANDLE *, int *);

/* comPass.cpp */
int PT_DECLSPEC comReadPassword(PT_HANDLE *, int, unsigned long *);
int PT_DECLSPEC comWritePassword(PT_HANDLE *, int, unsigned long);
int PT_DECLSPEC comWriteSerialNum(PT_HANDLE *, int, unsigned long);

/* comRead.cpp */
int PT_DECLSPEC comIntRead(PT_HANDLE *,  int *);
int PT_DECLSPEC comLongIntRead(PT_HANDLE *, long *);
int PT_DECLSPEC comRealRead(PT_HANDLE *, realtype *);
int PT_DECLSPEC comReadMeter(PT_HANDLE *, int, long *, int *);
int PT_DECLSPEC comReadStatus(PT_HANDLE *, int, long *, int *);

/* comWrite.cpp */
int PT_DECLSPEC comIntWrite(PT_HANDLE *, long, int);
int PT_DECLSPEC comLongIntWrite(PT_HANDLE *, long, long);
int PT_DECLSPEC comRealWrite(PT_HANDLE *, long, realtype);
int PT_DECLSPEC comSetDemoMode(PT_HANDLE *, int);

/* comEprom.cpp */
int PT_DECLSPEC comEepromUnsignedLongRead(PT_HANDLE *, short unsigned, unsigned long *);
int PT_DECLSPEC comEepromUnsignedLongWrite(PT_HANDLE *, short unsigned, unsigned long);

/* comWave.cpp */
int PT_DECLSPEC comProcessWaveBuffer(PT_HANDLE *, long *, realtype *, long, int, int, int, int);
int PT_DECLSPEC comProcessBuffer(PT_HANDLE *hp_com, long *lp_data, long l_length, 
                         int i_stereo_in_mode, int i_stereo_out_mode,
								 int i_buffer_type);

/* comMem.cpp */
int PT_DECLSPEC comMemReInitialize(PT_HANDLE *hp_com, realtype r_sampling_freq);
int PT_DECLSPEC comCheckOutProcessor(char *cp_dsp_function, int i_checkout_flag, int *ip_processor_num);

/* comDspInit.cpp */
int PT_DECLSPEC comInitDspAlgorithm(PT_HANDLE *hp_com, realtype r_sampling_freq, int i_init_flag);
int PT_DECLSPEC comZeroDspMemory(PT_HANDLE *hp_com);
int PT_DECLSPEC comAllocDspMem(PT_HANDLE *hp_com);
int PT_DECLSPEC comFreeDspMem(PT_HANDLE *hp_com);

/* comDspRun.cpp */
int PT_DECLSPEC comWriteParam(PT_HANDLE *hp_com, long l_offset, long l_val);
int PT_DECLSPEC comProcessWaveBuffer(PT_HANDLE *hp_com, long *lp_data, long l_length, 
                         int i_stereo_in_mode, int i_stereo_out_mode, 
								 int i_buffer_type);
int PT_DECLSPEC comProcessActiveBuffer(PT_HANDLE *hp_com, short *sp_data, long l_length, 
                         int i_stereo_in_mode, int i_stereo_out_mode, int i_buffer_type);
int PT_DECLSPEC comGetReCuePendingHdl(PT_HANDLE *hp_com, int *ip_recue_pending_flag);
int PT_DECLSPEC comSetReCuePendingHdl(PT_HANDLE *hp_com, int i_recue_pending_flag);
void PT_DECLSPEC comSetDemoFlagHdl(PT_HANDLE *hp_com, int i_flag);
int PT_DECLSPEC comGetDemoFlagHdl(PT_HANDLE *hp_com);

/* comDspSetFunc.cpp */
int PT_DECLSPEC comSetDspFunctions(PT_HANDLE *hp_com, char *cp_dspname, 
										      short s_bit_width, int i_use_old_bit_width);

/* 16 bit inlining seems to only work within a single source file */
/* #DEFINE COMHOST_INLINE inline */
#define COMHOST_INLINE 

#endif // _COM_H_
