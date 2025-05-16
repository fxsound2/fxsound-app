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

/*
 * FILE: hrdwr\comSftwr\comSftwr.c
 * DATE: 9/7/97
 * AUTHOR: Paul Titchener
 * DESCRIPTION:
 *
 *  These functions handle the direct reads and writes to the DSP card.
 *  This is the version for use with the software dsp dll.
 */

/* Standard includes */
#ifdef WIN32
#include <windows.h>
#include <winuser.h>
#endif

#include <time.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "codedefs.h"
#include "pt_defs.h"

#include "platform.h"
#include "boardrv1.h"

/* For setting demo bypass mode */
//#include "product_type.h"

/*
#include "hrdwr.h"
#include "hutil.h"
 */
#include "u_comSftwr.h"
#include "comSftwr.h"

#include "c_dsps.h"
#include "demodef.h"

/* Make sure PT_DSP_BUILD is set */
#if !defined(PT_DSP_BUILD)
#error PT_DSP_BUILD Not Defined.
#endif

/* For writing parameters to DSP.
 * Note that the hrdwrWriteParameterIfNotBusy uses a hard wait loop to
 * complete the second write operation (the actual value).
 */
int COMSFTWR_DECL comSftwrWriteParam(PT_HANDLE *hp_comSftwr, long l_offset, long l_val)
{
	float *flt_ptr;
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL) 
		return(NOT_OKAY);

	flt_ptr = (float *)&l_val;

#ifdef COMSFTWR_MESSAGE_BOXES

   if((l_offset < 0) || (l_offset > 256))
	   MessageBoxA(NULL, "Invalid l_offset in comSftwrWriteParam", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 

   if( dsp_params == NULL )
	   MessageBoxA(NULL, "dsp_params is NULL in comSftwrWriteParam", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 

#endif

	cast_handle->dsp_params[l_offset] = *flt_ptr;

	/* Set the recue pending flag */
	cast_handle->comSftwrReCuePending = 1;

	return(OKAY);    	
}

/*
 * FUNCTION: comSftwrProcessWaveBuffer()
 * DESCRIPTION:
 *  
 *  Process the passed wave buffer, and send it back (no A/D or D/A).
 *  Currently only processes stereo to stereo or mono to mono signals.
 *
 */
int COMSFTWR_DECL comSftwrProcessWaveBuffer(PT_HANDLE *hp_comSftwr, long *lp_data, long l_length, 
                         int i_stereo_in_mode, int i_stereo_out_mode, 
								 int i_buffer_type)
{
   float *param_addr;
   int index;
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

   /* l_length comes in with the buffer size in samples */

   /* Don't process the buffer if DSP is not available */
#ifdef COMSFTWR_CHECK_OUT_DSP
   if( hutsyncCheckOutDsp( (unsigned short)i_processor_index) == 0 )
	   return(OKAY);
#endif

   /* Set up pointer to correct parameter space */
   param_addr = &(cast_handle->dsp_params[0]);

   index = cast_handle->dsp_function_index;

#ifdef COMSFTWR_MESSAGE_BOXES
   if( (index < 0) || (index > 62) )
	   MessageBoxA(NULL, "Invalid function index in comSftwrProcessWaveBuffer", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 

   if(lp_data == NULL)
	   MessageBoxA(NULL, "lpdata NULL in comSftwrProcessWaveBuffer", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 

   if(param_addr == NULL)
	   MessageBoxA(NULL, "param_addr NULL in comSftwrProcessWaveBuffer", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 

   if( (cast_handle->comSftDspProcessPtr[index]) == NULL)
	   MessageBoxA(NULL, "NULL function ptr in comSftwrProcessWaveBuffer", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 

   if( (l_length < 0) || (l_length > 256000) )
	   MessageBoxA(NULL, "Invalid function index in comSftwrProcessWaveBuffer", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 
#endif

/* Only do DSPFX style muting if this is the DSPFX product */
#if (PT_DSP_BUILD == PT_DSP_DSPFX)
   /* Note zeroing operation is dependent on input/output modes */
   if(  comSftwrDemoFlag[i_processor_index] && 
	      (sample_count[i_processor_index] > COMSFTWR_DEMO_SAMPLES_ALLOWED) )
   {
	   dspsZeroOutput(lp_data, l_length,
					 param_addr, 
					 dsp_memory[i_processor_index],
					 dsp_state[i_processor_index], 
                &(ComSftwrMeterData[i_processor_index]), i_buffer_type);

	   silence_count[i_processor_index] += l_length;
	   if( silence_count[i_processor_index] > COMSFTWR_DEMO_SILENCE_COUNT )
	   {
		   silence_count[i_processor_index] = 0;
		   sample_count[i_processor_index] = 0;
	   }
   }
   else
#endif
   {

/* 
#define COMSFTWR_AUTO_DEMO
#define COMSFTWR_AUTO_DEMO_SECS 7.5
 */ 

#ifdef COMSFTWR_AUTO_DEMO
		{
			/* On off toggling for demo */
			static int count = 0;
			static int on = 0;
			long int tmp_buf[16384];

			count += l_length;
			if( count > 44100 * COMSFTWR_AUTO_DEMO_SECS )
			{
				count = 0;
				if( on )
					on = 0;
				else
					on = 1;
			}

			if( on )
			{
				(*cast_handle->comSftDspProcessPtr[index])(lp_data, l_length,
								 param_addr, 
								 cast_handle->dsp_memory,
								 cast_handle->dsp_state, 
								 &(cast_handle->ComSftwrMeterData), i_buffer_type);
			}
			else
			{
				int k;
				int len;

				len = l_length;
				if(i_stereo_in_mode)
					len *= 2;

				for(k=0; k<=len; k++)
					tmp_buf[k] = lp_data[k];

				(*cast_handle->comSftDspProcessPtr[index])(tmp_buf, l_length,
								 param_addr, 
								 cast_handle->dsp_memory,
								 cast_handle->dsp_state, 
								 &(cast_handle->ComSftwrMeterData), i_buffer_type);
			}
		}
#else
		(*cast_handle->comSftDspProcessPtr[index])(lp_data, l_length,
							 param_addr, 
							 cast_handle->dsp_memory,
							 cast_handle->dsp_state, 
	                   &(cast_handle->ComSftwrMeterData), i_buffer_type);
#endif
   }

/* Only do DSPFX style metering correction if this is the DSPFX product */
#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	/* Correct meter values for averaging. Values are coming back as floats,
	 * we need to convert them.
	 */
   {
		float factor = (float)PC_24BIT_FLOAT_PLUS_CLIP/(float)l_length;

		cast_handle->ComSftwrMeterData.left_in = 
			(long)( *(float *)&(ComSftwrMeterData[i_processor_index].left_in) * factor); 
   
		cast_handle->ComSftwrMeterData.right_in = 
			(long)( *(float *)&(ComSftwrMeterData.right_in) * factor); 

		cast_handle->ComSftwrMeterData[i_processor_index].left_out = 
			(long)( *(float *)&(ComSftwrMeterData.left_out) * factor); 

		cast_handle->ComSftwrMeterData.right_out = 
			(long)( *(float *)&(ComSftwrMeterData.right_out) * factor); 
	}
#endif

   /* Check DSP back in */
#ifdef COMSFTWR_CHECK_OUT_DSP
   hutsyncCheckInDsp( (unsigned short)i_processor_index );
#endif

   cast_handle->sample_count += l_length;


   return(OKAY);
} 

/*
 * FUNCTION: comSftwrProcessActiveBuffer()
 * DESCRIPTION:
 *  
 *  Process the passed Active Movie buffer, and send it back (no A/D or D/A).
 *
 */
int COMSFTWR_DECL comSftwrProcessActiveBuffer(PT_HANDLE *hp_comSftwr, short *sp_data, long l_length, 
                         int i_stereo_in_mode, int i_stereo_out_mode,
								 int i_buffer_type)
{
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

   /* l_length comes in with the buffer size in samples */

   /* For now, call original saw style processing */
   if( comSftwrProcessWaveBuffer(hp_comSftwr, (long *)sp_data, l_length, 
                         i_stereo_in_mode, i_stereo_out_mode, i_buffer_type) != OKAY)
		return(NOT_OKAY);

   return(OKAY);
}

/* For software based Dsp engine, reallocates memory if needed,
 * and initializes the memory and the parameter space.
 * Causes memory to be allocated only in the calling dll instance,
 * so should only be called from dll instance that will also be
 * making the processing calls.
 */
int COMSFTWR_DECL comSftwrInitDspAlgorithm(PT_HANDLE *hp_comSftwr, realtype r_sampling_freq, int i_init_flag)
{
	int index;
	float *param_addr;
#ifdef WIN32
	__int64 perf_count;
#else
	long long perf_count;
#endif
	long sample_count_init;
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Reset sample, silence count for demo mode. Use perf count to
	 * get a pseudo-random number, then modulo by total cycle length
	 * to cause start time to vary.
	 */
#ifdef WIN32
	QueryPerformanceCounter( (LARGE_INTEGER *)&perf_count );
#else
	perf_count = 1;
#endif

#ifdef WIN32
	sample_count_init = (long)(perf_count % (__int64)COMSFTWR_DEMO_SAMPLES_ALLOWED);
#else
	sample_count_init = (long)(perf_count % (long long)COMSFTWR_DEMO_SAMPLES_ALLOWED);
#endif
	cast_handle->sample_count = sample_count_init;

	/* Used to keep track of mute time */
	cast_handle->silence_count = 0L;

	/* Seemed to be causing problems with SAW and Gina. Apparently since
	 * the message box beeps, it was conflicting with the sound card. Other
	 * non-Gina machines seem to handle this OK.
	 * Message has been moved to MDLY.
	 */
	/*
    if( (comSftwrDemoMessageBoxDisplayed == 0) && comSftwrDemoFlag[i_processor_index] )
    {		   
	   comSftwrDemoMessageBoxDisplayed = 1;
#ifdef WIN32
	   MessageBoxA(NULL, "Sound will be muted for a period after every 10 seconds.",
			      "DSP/FX Demo Version", 
				(MB_OK | MB_ICONINFORMATION | MB_TOPMOST) );
#endif
    }
	*/

	if( i_init_flag & DSPS_INIT_MEMORY )
	{
		/* If needed, reallocates DSP memory */
		if( comSftwrAllocDspMem(hp_comSftwr) != OKAY )
			return(NOT_OKAY);
	}

	/* Set up pointer to correct parameter space */
   param_addr = &(cast_handle->dsp_params[0]);

	index = cast_handle->dsp_function_index;

#ifdef COMSFTWR_MESSAGE_BOXES
	if( (index < 0) || (index > COMSFTWR_FUNCTION_INDEX_MAX) )
	   MessageBoxA(NULL, "Invalid function index in comSftwrInitDspAlgorithm", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 

	if(param_addr == NULL)
	   MessageBoxA(NULL, "param_addr NULL in comSftwrProcessWaveBuffer", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 
#endif

	/* Calls DSP algorithm initialization */
	if( ((*cast_handle->comSftDspInitPtr[index])(param_addr,
								    cast_handle->dsp_memory,
									 cast_handle->dsp_memory_size,
								    cast_handle->dsp_state,
									 i_init_flag, r_sampling_freq)) != OKAY )
		return(NOT_OKAY);

	return(OKAY);
}

/* Zero's internal signal memory of DSP.
 * Useful when song playback is stopped then switched to another song.
 */
int COMSFTWR_DECL comSftwrZeroDspMemory(PT_HANDLE *hp_comSftwr)
{
	float *param_addr;
	int index;
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Set up pointer to correct parameter space */
   param_addr = &(cast_handle->dsp_params[0]);

	index = cast_handle->dsp_function_index;

#ifdef COMSFTWR_MESSAGE_BOXES
	if( (index < 0) || (index > COMSFTWR_FUNCTION_INDEX_MAX) )
	   MessageBoxA(NULL, "Invalid function index in comSftwrInitDspAlgorithm", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 

	if(param_addr == NULL)
	   MessageBoxA(NULL, "param_addr NULL in comSftwrProcessWaveBuffer", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 
#endif

	/* Calls DSP algorithm initialization with flag set to zero memory. Sampling freq value isn't used */
	if( ((*cast_handle->comSftDspInitPtr[index])(param_addr,
								    cast_handle->dsp_memory,
									 cast_handle->dsp_memory_size,
								    cast_handle->dsp_state,
									 DSPS_ZERO_MEMORY, (realtype)44100.0)) != OKAY )
		return(NOT_OKAY);

	return(OKAY);
}

/* For software based Dsp engine, sets function index
 * to reference the desired init and processing functions.
 * This could be improved by having DSP/FX pass a defined
 * int to specify the processing instead of a program name.
 *
 * The indexes are in an array with index for each processor.
 * The array is declared in the shared memory section so that 
 * the DSP/FX instance of the DLL can set the function index
 * which is used by the DAW instance of the DLL to do the
 * initialization and processing.
 *
 */
int COMSFTWR_DECL comSftwrSetFunctionIndex(PT_HANDLE *hp_comSftwr, char *cp_dspname, 
										  short s_bit_width, int i_use_old_bit_width)
{
	int index = 0;
	long memsize = 0;
	int offset = 0;
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* PTHACK- until we redo the initialization, use this flag to see if we
	 * should use old bitwidth instead of passed in value. This is a workaround
	 * for the problem of shutting down DSP/FX and starting it again.
	 */
	if( i_use_old_bit_width )
		s_bit_width = cast_handle->comSftwrBitWidth;
	else
		cast_handle->comSftwrBitWidth = s_bit_width;

	/* Set to correct function, 0 => 16 bit, 1 => 32 bit */
	if(s_bit_width == 32)
		offset = 1;

	/* Use do/while structure to enable use of break to jump
	 * out when we match the program name.
	 * dly-8, fla-2, chor-4, pitch-2, peq-8, trm-2, pan-2, rvrb-34, (62)
	 * multi-comp-1, aural-1, Max-1, lex-1, apit-1, play-1, comp-1, wide-1, proto-1 = (9)
	 * NOTE- CURRENTLY (62 + 9) * 2 = 142 FUNCTIONS.
	 * Check against setting of DSPS_MAX_NUM_PROC_FUNCTIONS
	 * Note that we have to skip 2 to jump over both 16 and 32 bit functions.
	 */
	do
	{
		memsize = DSPS_SOFT_MEM_DELAY_LENGTH;
		if( strcmp(cp_dspname, "dly1") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "dly2") == 0 )
			break;
		else 
			index += 2;
		if( strcmp(cp_dspname, "dly3") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "dly4") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "dly5") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "dly6") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "dly7") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "dly8") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_FLANGE_LENGTH;
		if( strcmp(cp_dspname, "flang1") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "flang2") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_CHORUS_LENGTH;
		if( strcmp(cp_dspname, "chor1") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "chor2") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "chor3") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "chor4") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_PITCH_LENGTH;
		if( strcmp(cp_dspname, "pitch1") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "pitch2") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_PEQ_LENGTH;
		if( strcmp(cp_dspname, "peq1") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "peq2") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "peq3") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "peq4") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "peq5") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "peq6") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "peq7") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "peq8") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_TREMOLO_LENGTH;
		if( strcmp(cp_dspname, "trm1") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "trm2") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_PAN_LENGTH;
		if( strcmp(cp_dspname, "pan1") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "pan2") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_REVERB_LENGTH;
		if( strcmp(cp_dspname, "R1s44r20") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r25") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r30") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r35") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r40") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r45") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r50") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r55") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r60") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r65") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r70") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r75") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r80") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r85") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r90") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r95") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s44r00") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r20") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r25") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r30") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r35") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r40") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r45") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r50") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r55") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r60") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r65") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r70") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r75") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r80") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r85") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r90") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r95") == 0 )
			break;
		else
			index += 2;
		if( strcmp(cp_dspname, "R1s48r00") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_MULTI_COMP_LENGTH;
		if( strcmp(cp_dspname, "cmp8") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_AURAL_ENHANCER_LENGTH;
		if( strcmp(cp_dspname, "aural0") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_MAXIMIZER_LENGTH;
		if( strcmp(cp_dspname, "max0") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_LEX_LENGTH;
		if( strcmp(cp_dspname, "lex0") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_APIT_LENGTH;
		if( strcmp(cp_dspname, "apt0") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_PLAY_LENGTH;
		if( strcmp(cp_dspname, "ply0") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_COMP_LENGTH;
		if( strcmp(cp_dspname, "cmp0") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_WIDE_LENGTH;
		if( strcmp(cp_dspname, "wid0") == 0 )
			break;
		else
			index += 2;

		memsize = DSPS_SOFT_MEM_PROTO1_LENGTH;
		if( strcmp(cp_dspname, "proto10") == 0 )
			break;

		else /* Fall through case, no match */
		{
			return(NOT_OKAY);
		}
	}
	while(0);

	/* Now point to correct 16 or 32 bit function */
	index += offset;

	/* Set shared global index array to correct function index.
	 * Will be used in the DAW dll instance to call the correct
	 * init and processing functions.
	 */
	cast_handle->dsp_function_index = index;

	/* This var is in the DLL shared memory section, and will
	 * be used by allocation function in DAW dll instance to 
	 * reallocate the correct memory size.
	 */
	cast_handle->dsp_memory_size_required = memsize;

	return(OKAY);
}

/* Based on the memsize that was set by DSPFX, this function
 * reallocates the correct memory size. For DAW processing, this
 * will be called by the DAW DLL instance, or for wave processing
 * it will be called by DSPFX. Note that allocated memory is only
 * associated with the calling DLL instance.
 */
int COMSFTWR_DECL comSftwrAllocDspMem(PT_HANDLE *hp_comSftwr)
{
	long memsize_required;
	long current_memsize;
	float *mem_ptr;
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	memsize_required = cast_handle->dsp_memory_size_required;
	current_memsize = cast_handle->dsp_memory_size;

	/* This case- current size is zero, required is non-zero */
	if( (current_memsize == 0) && (memsize_required != 0) )
	{
		/* Memory space has not been allocated yet, so malloc it */
		mem_ptr = (float *)malloc( memsize_required * sizeof(float) );
		if( mem_ptr == NULL )
		{
			cast_handle->dsp_memory = NULL;
			cast_handle->dsp_memory_size = 0;
			return(NOT_OKAY);
		}
		else
		{
			cast_handle->dsp_memory = mem_ptr;
			cast_handle->dsp_memory_size = memsize_required;
		}
	}
	else if( current_memsize != memsize_required )
	/* This case- current size is non-zero, but not correct, realloc */
	{
		/* Memory space needs to be changed, so realloc it */
		mem_ptr = (float *)realloc( cast_handle->dsp_memory, memsize_required * sizeof(float) ); 

		/* Note- NULL is returned if realloc fails, or if you request zero size */
		if( (mem_ptr == NULL) && (memsize_required != 0) )
		{
			cast_handle->dsp_memory = NULL;
			cast_handle->dsp_memory_size = 0;
			return(NOT_OKAY);
		}
		else
		{
			cast_handle->dsp_memory = mem_ptr;
			cast_handle->dsp_memory_size = memsize_required;
		}
	}

	/* If drops through to here, current setting is ok */
	return(OKAY);
}

/* Frees the memory allocated by comSftwrAllocDspMemBased
 */
int COMSFTWR_DECL comSftwrFreeDspMem(PT_HANDLE *hp_comSftwr)
{
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if( cast_handle->dsp_memory != NULL )
	{
		/* Free dsp memory */
		free( cast_handle->dsp_memory );

		/* Set the memory size variable to zero */
		cast_handle->dsp_memory_size = 0;

	}
	
	return(OKAY);
}

/* For software based Dsp engine, sets up the arrays of
 * dsp init and processing functions. These will be referenced
 * using an index.
 * Note that currently the same init function is used for both
 * 16 and 32 bit, but is repeated in the array to allow a single
 * index to correctly set both the init and process functions.
 */
int COMSFTWR_DECL comSftwrInitFunctions(PT_HANDLE *hp_comSftwr)
{
	int index = 0;
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

#ifdef COMSFTWR_MESSAGE_BOXES
	/* Message to show that message boxes are on in comsftwr */
	MessageBoxA(NULL, "Debug Message Boxes are active in comSftwr", "Special Message", 
				(MB_OK | MB_TASKMODAL ) ); 

#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsDly1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly1Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsDly1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly1Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsDly2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly2Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsDly2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly2Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsDly3Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly3Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsDly3Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly3Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsDly4Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly4Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsDly4Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly4Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsDly5Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly5Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsDly5Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly5Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsDly6Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly6Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsDly6Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly6Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsDly7Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly7Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsDly7Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly7Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsDly8Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly8Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsDly8Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsDly8Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsFlang1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsFlang1Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsFlang1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsFlang1Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsFlang2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsFlang2Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsFlang2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsFlang2Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsChor1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsChor1Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsChor1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsChor1Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsChor2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsChor2Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsChor2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsChor2Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsChor3Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsChor3Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsChor3Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsChor3Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsChor4Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsChor4Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsChor4Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsChor4Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsPitch1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPitch1Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPitch1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPitch1Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsPitch2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPitch2Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPitch2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPitch2Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsPeq1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq1Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPeq1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq1Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsPeq2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq2Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPeq2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq2Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsPeq3Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq3Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPeq3Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq3Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsPeq4Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq4Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPeq4Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq4Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsPeq5Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq5Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPeq5Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq5Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsPeq6Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq6Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPeq6Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq6Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsPeq7Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq7Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPeq7Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq7Process32;
	index++;
#else
	index += 2;
#endif

	/* Note - 8 band eq is enabled for DFX */
	cast_handle->comSftDspInitPtr[index] = &dspsPeq8Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq8Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPeq8Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPeq8Process32;
	index++;

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsTrm1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsTrm1Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsTrm1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsTrm1Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsTrm2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsTrm2Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsTrm2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsTrm2Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsPan1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPan1Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPan1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPan1Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsPan2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPan2Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPan2Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsPan2Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R20Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R20Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R20Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R20Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R25Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R25Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R25Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R25Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R30Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R30Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R30Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R30Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R35Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R35Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R35Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R35Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R40Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R40Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R40Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R40Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R45Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R45Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R45Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R45Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R50Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R50Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R50Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R50Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R55Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R55Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R55Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R55Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R60Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R60Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R60Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R60Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R65Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R65Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R65Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R65Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R70Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R70Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R70Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R70Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R75Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R75Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R75Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R75Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R80Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R80Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R80Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R80Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R85Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R85Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R85Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R85Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R90Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R90Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R90Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R90Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R95Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R95Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R95Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R95Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R00Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R00Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S44R00Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S44R00Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R20Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R20Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R20Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R20Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R25Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R25Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R25Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R25Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R30Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R30Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R30Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R30Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R35Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R35Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R35Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R35Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R40Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R40Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R40Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R40Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R45Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R45Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R45Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R45Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R50Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R50Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R50Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R50Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R55Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R55Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R55Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R55Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R60Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R60Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R60Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R60Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R65Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R65Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R65Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R65Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R70Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R70Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R70Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R70Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R75Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R75Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R75Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R75Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R80Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R80Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R80Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R80Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R85Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R85Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R85Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R85Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R90Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R90Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R90Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R90Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R95Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R95Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R95Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R95Process32;
	index++;
#else
	index += 2;
#endif

#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R00Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R00Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R00Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R00Process32;
	index++;
#else
	index += 2;
#endif

	/* TEMP PTHACK - indexes for multiband compressor are set to reverbs */
#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R00Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R00Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsR1S48R00Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsR1S48R00Process32;
	index++;
#else
	index += 2;
#endif

	/* Indexes for aural enhancer  (used in DFX) */
#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsAuralInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsAuralProcess;
#endif
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsAuralInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsAuralProcess32;
	index++;

	/* Indexes for maximizer  (used in DFX) */
#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsMaximizerInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsMaximizerProcess;
#endif
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsMaximizerInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsMaximizerProcess32;
	index++;

	/* Indexes for lex reverb (used in DFX) */
#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsLexReverbInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsLexReverbProcess;
#endif
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsLexReverbInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsLexReverbProcess32;
	index++;

	/* Indexes for Auto Pitch */
#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsApitInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsApitProcess;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsApitInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsApitProcess32;
	index++;
#else
	index += 2;
#endif

	/* Indexes for MP3 Player Plug-In (keep both 16 and 32 bit for now */
	cast_handle->comSftDspInitPtr[index] = &dspsPlayInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsPlayProcess;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsPlayInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsPlayProcess32;
	index++;

	/* Indexes for stereo Compressor Plug-In */
#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsCompInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsCompProcess;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsCompInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsCompProcess32;
	index++;
#else
	index += 2;
#endif

	/* Indexes for stereo widener Plug-In (used in DFX) */
#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsWideInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsWideProcess;
#endif
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsWideInit;
	cast_handle->comSftDspProcessPtr[index] = &dspsWideProcess32;
	index++;

	/* Indexes for proto1 */
#if (PT_DSP_BUILD == PT_DSP_DSPFX)
	cast_handle->comSftDspInitPtr[index] = &dspsProto1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsProto1Process;
	index++;
	cast_handle->comSftDspInitPtr[index] = &dspsProto1Init;
	cast_handle->comSftDspProcessPtr[index] = &dspsProto1Process32;
	index++;
#else
	index += 2;
#endif

	return(OKAY);
}

/*
 * FUNCTION: comSftwrGetReCuePending()
 * DESCRIPTION:
 *  
 *  Gets the value for the global recue pending flag.
 *
 */
int COMSFTWR_DECL comSftwrGetReCuePending(PT_HANDLE *hp_comSftwr, int *ip_recue_pending_flag)
{
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_recue_pending_flag = cast_handle->comSftwrReCuePending;

	return(OKAY);
}

/*
 * FUNCTION: comSftwrSetReCuePending()
 * DESCRIPTION:
 *  
 *  Sets the value for the global recue pending flag.
 *
 */
int COMSFTWR_DECL comSftwrSetReCuePending(PT_HANDLE *hp_comSftwr, int i_recue_pending_flag)
{
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->comSftwrReCuePending = i_recue_pending_flag;

	return(OKAY);
}

/*
 * FUNCTION: comSftwrSetDemoFlag()
 * DESCRIPTION:
 *  
 *  Sets the value for the demo flag.
 *
 */
int COMSFTWR_DECL comSftwrSetDemoFlag(PT_HANDLE *hp_comSftwr, int i_flag)
{
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->comSftwrDemoFlag = i_flag;

	return(OKAY);
}

/*
 * FUNCTION: comSftwrGetDemoFlag()
 * DESCRIPTION:
 *  
 *  Returns the value for the demo flag.
 *
 */
int COMSFTWR_DECL comSftwrGetDemoFlag(PT_HANDLE *hp_comSftwr, int *ip_demo_flag)
{
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_demo_flag = cast_handle->comSftwrDemoFlag;

	return(OKAY);
}
