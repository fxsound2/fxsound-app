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
#ifndef _COMSFTWR_H_
#define _COMSFTWR_H_

/* Turns on message box printouts */
/* #define COMSFTWR_MESSAGE_BOXES */

/* Turns on DSP check in/out for software version.
 * Requires comsftwr and com link to scard.
 * Doesn't seem necessary at the moment.
 */
/* #define COMSFTWR_CHECK_OUT_DSP */

/* 661500 is 15 secs at 44.1khz */
#define COMSFTWR_DEMO_SAMPLES_ALLOWED 441000
#define COMSFTWR_DEMO_SILENCE_COUNT COMSFTWR_DEMO_SAMPLES_ALLOWED/5

/* The different types of buffers */
#define COM_16_BIT_SAMPLES 0
#define COM_32_BIT_FLOAT_SAMPLES 1
#define COM_24_BIT_SAMPLES 2

/* Maximum value of function index */
#define COMSFTWR_PROCESSOR_INDEX_MAX 15
#define COMSFTWR_FUNCTION_INDEX_MAX 62

/* Next lines set whether functions are declared for static libs or dlls */
#if defined(COMSFTWR_DLL_EXPORT) /* Defined for compiling DLL's (versus static lib) */
	#if defined(WIN32)
	#define COMSFTWR_DECL __declspec( dllexport ) /* 32 bit dll version */
	#else
	#define COMSFTWR_DECL _export /* 16 bit dll version */
	#endif
#elif defined(SFTWR_DLL_IMPORT) /* For compiling apps that link to dll */
	#if defined(WIN32)
	#define COMSFTWR_DECL __declspec( dllimport ) /* 32 bit dll version */
	#else
	#define COMSFTWR_DECL
	#endif
#else /* Library (non-dll) case */
	#define COMSFTWR_DECL
#endif
 
/* Constant Defines */ 
/* Functions */ 
/* comSftwr.c */
int COMSFTWR_DECL comSftwrInitDspAlgorithm(PT_HANDLE *hp_comSftwr, realtype r_sampling_freq, int i_init_flag);
int COMSFTWR_DECL comSftwrZeroDspMemory(PT_HANDLE *hp_comSftwr);

int COMSFTWR_DECL comSftwrWriteParam(PT_HANDLE *hp_comSftwr, long l_offset, long l_val);

int COMSFTWR_DECL comSftwrProcessWaveBuffer(PT_HANDLE *hp_comSftwr, long *lp_data, long l_length, 
                         int i_stereo_in_mode, int i_stereo_out_mode, int i_buffer_type);

int COMSFTWR_DECL comSftwrProcessActiveBuffer(PT_HANDLE *hp_comSftwr, short *sp_data, long l_length, 
                         int i_stereo_in_mode, int i_stereo_out_mode, int);

int COMSFTWR_DECL comSftwrInitFunctions(PT_HANDLE *hp_comSftwr);
int COMSFTWR_DECL comSftwrSetFunctionIndex(PT_HANDLE *hp_comSftwr, char *cp_dspname, short s_bit_width, int i_use_current_bit_width);
int COMSFTWR_DECL comSftwrAllocDspMem(PT_HANDLE *hp_comSftwr);
int COMSFTWR_DECL comSftwrFreeDspMem(PT_HANDLE *hp_comSftwr);
int COMSFTWR_DECL comSftwrGetReCuePending(PT_HANDLE *hp_comSftwr, int *ip_recue_pending_flag);
int COMSFTWR_DECL comSftwrSetReCuePending(PT_HANDLE *hp_comSftwr, int i_recue_pending_flag);

int COMSFTWR_DECL comSftwrGetDemoFlag(PT_HANDLE *hp_comSftwr, int *);
int COMSFTWR_DECL comSftwrSetDemoFlag(PT_HANDLE *hp_comSftwr, int);

#endif /* _COMSFTWR_H_ */
