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
 * FILE: hrdwr\comSftwr\comSftwr.cpp
 * DATE: 9/7/97
 * AUTHOR: Paul Titchener
 * DESCRIPTION:
 *
 *  These functions handle the direct reads and writes to the DSP card.
 *  This is the version for use with the software dsp dll.
 */

#include "codedefs.h"

/* Standard includes */
#ifdef WIN32
#include <windows.h>
#include <winuser.h>
#endif /* WIN32 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "pt_defs.h"

#include "platform.h"
#include "boardrv1.h"

/* For setting demo bypass mode */
//#include "product_type.h"

#include "hrdwr.h"
#include "hutil.h"


extern "C"
{
#include "u_comSftwr.h"
#include "comSftwr.h"
#include "c_dsps.h"
}

#include "comSftwrCPP.h"
#include "demodef.h"

/*
 * FUNCTION: comSftwrInit()
 * DESCRIPTION:
 *  Allocates and initializes the passed comSftwr handle.
 */
int COMSFTWR_DECL comSftwrInitCPP(PT_HANDLE **hpp_comSftwr)
{
	int i;
	struct comSftwrHdlType *cast_handle;

	/* Allocate the handle */
	cast_handle = (struct comSftwrHdlType *)calloc(1, sizeof(struct comSftwrHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);
    
	for(i=0; i<(DSPFX_MAX_NUM_PROCS * 2 * DSPS_MAX_NUM_PARAMS); i++)
		cast_handle->dsp_params[i] = (realtype)0.0;

	cast_handle->dsp_memory_size_required = 0;
	cast_handle->dsp_memory = NULL;
	cast_handle->dsp_memory_size = 0;

	for(i=0; i<(DSPFX_MAX_NUM_PROCS * DSPS_NUM_STATE_VARS); i++)
		cast_handle->dsp_state[i] = (realtype)0.0;

	cast_handle->dsp_function_index = 0;
	cast_handle->comSftwrDemoFlag = 1;
	cast_handle->comSftwrReCuePending = 0;
	cast_handle->comSftwrBitWidth = 0;

	cast_handle->ComSftwrMeterData.values_are_new = 0;
	cast_handle->ComSftwrMeterData.left_in = 0;
	cast_handle->ComSftwrMeterData.right_in = 0;
	cast_handle->ComSftwrMeterData.left_out = 0;
	cast_handle->ComSftwrMeterData.right_out = 0;
	cast_handle->ComSftwrMeterData.dsp_status = 0;

	for(i=0; i<DSPS_MAX_NUM_PROC_FUNCTIONS; i++)
	{
		cast_handle->comSftDspInitPtr[i] = NULL;
		cast_handle->comSftDspProcessPtr[i] = NULL;
	}

	cast_handle->sample_count = 0;
	cast_handle->silence_count = 0;

   *hpp_comSftwr = (PT_HANDLE *)cast_handle;

   return(OKAY);
}  

/* For software based Dsp engine, reallocates memory if needed,
 * and initializes the memory and the parameter space.
 * Causes memory to be allocated only in the calling dll instance,
 * so should only be called from dll instance that will also be
 * making the processing calls.
 */
int COMSFTWR_DECL comSftwrInitDspAlgorithmCPP(PT_HANDLE *hp_comSftwr, realtype r_sampling_freq, int i_init_flag)
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

#ifdef COMSFTWR_MESSAGE_BOXES
	if( (i_processor_index < 0) || (i_processor_index > COMSFTWR_PROCESSOR_INDEX_MAX) )
	   MessageBox(NULL, "Invalid i_processor_index in comSftwrInitDspAlgorithm", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 
#endif

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
	   MessageBox(NULL, "Sound will be muted for a period after every 10 seconds.",
			      "DSP/FX Demo Version", 
				(MB_OK | MB_ICONINFORMATION | MB_TOPMOST) );
#endif
    }
	*/

	if( i_init_flag & DSPS_INIT_MEMORY )
	{
		/* If needed, reallocates DSP memory */
		if( comSftwrAllocDspMemCPP(hp_comSftwr) != OKAY )
			return(NOT_OKAY);
	}

	/* Set up pointer to correct parameter space */
   param_addr = &(cast_handle->dsp_params[0]);

	index = cast_handle->dsp_function_index;

#ifdef COMSFTWR_MESSAGE_BOXES
	if( (index < 0) || (index > COMSFTWR_FUNCTION_INDEX_MAX) )
	   MessageBox(NULL, "Invalid function index in comSftwrInitDspAlgorithm", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 

	if(param_addr == NULL)
	   MessageBox(NULL, "param_addr NULL in comSftwrProcessWaveBuffer", NULL, 
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
int COMSFTWR_DECL comSftwrZeroDspMemoryCPP(PT_HANDLE *hp_comSftwr)
{
	float *param_addr;
	int index;
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)hp_comSftwr;

	if (cast_handle == NULL)
		return(NOT_OKAY);

#ifdef COMSFTWR_MESSAGE_BOXES
	if( (i_processor_index < 0) || (i_processor_index > COMSFTWR_PROCESSOR_INDEX_MAX) )
	   MessageBox(NULL, "Invalid i_processor_index in comSftwrZeroDspMemory", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 
#endif

	/* Set up pointer to correct parameter space */
   param_addr = &(cast_handle->dsp_params[0]);

	index = cast_handle->dsp_function_index;

#ifdef COMSFTWR_MESSAGE_BOXES
	if( (index < 0) || (index > COMSFTWR_FUNCTION_INDEX_MAX) )
	   MessageBox(NULL, "Invalid function index in comSftwrInitDspAlgorithm", NULL, 
					(MB_OK | MB_ICONERROR | MB_TASKMODAL ) ); 

	if(param_addr == NULL)
	   MessageBox(NULL, "param_addr NULL in comSftwrProcessWaveBuffer", NULL, 
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

/* Based on the memsize that was set by DSPFX, this function
 * reallocates the correct memory size. For DAW processing, this
 * will be called by the DAW DLL instance, or for wave processing
 * it will be called by DSPFX. Note that allocated memory is only
 * associated with the calling DLL instance.
 */
int COMSFTWR_DECL comSftwrAllocDspMemCPP(PT_HANDLE *hp_comSftwr)
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

/*
 * FUNCTION: comSftwrFreeUp()
 * DESCRIPTION:
 *   Frees the passed comSftwr handle and sets to NULL.
 */
int COMSFTWR_DECL comSftwrFreeUp(PT_HANDLE **hpp_comSftwr)
{
	struct comSftwrHdlType *cast_handle;

	cast_handle = (struct comSftwrHdlType *)(*hpp_comSftwr);

	if (cast_handle == NULL)
		return(OKAY);

	if( cast_handle->dsp_memory != NULL )
	{
		/* Free dsp memory */
		free( cast_handle->dsp_memory );

		/* Set the memory size variable to zero */
		cast_handle->dsp_memory_size = 0;
	}

	free(cast_handle);

	*hpp_comSftwr = NULL;

	return(OKAY);
}

