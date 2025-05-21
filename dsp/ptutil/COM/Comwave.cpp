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

/* Standard includes */

#include "codedefs.h"

#include <stdio.h> 
#include <stdlib.h>
#include <malloc.h> 

#ifndef __ANDROID__
#include <conio.h>
#include <time.h>
#include <windows.h>
#else
#ifndef DWORD
#define DWORD unsigned int
#endif
#endif //WIN32

#include "pcfg.h"
#include "pt_defs.h"

#define PC_TARGET
#include "platform.h"
#include "boardrv1.h"

extern "C"
{
#include "hrdwr.h"
#include "comSftwr.h"
}

#include "pwav.h"
#include "com.h"
#include "u_com.h"

/* FUNCTION: comProcessBuffer()
 * DESCRIPTION:
 *  
 *  Process the passed buffer.
 *
 */
int PT_DECLSPEC comProcessBuffer(PT_HANDLE *hp_com, long *lp_data, long l_length, 
                         int i_stereo_in_mode, int i_stereo_out_mode,
								 int i_buffer_type)
{
   struct comHdlType *cast_handle;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   if (comSftwrProcessWaveBuffer(cast_handle->comSftwr_hdl, lp_data, l_length, 
                                 i_stereo_in_mode, i_stereo_out_mode,
											i_buffer_type) != OKAY)
		return(NOT_OKAY);

   return(OKAY);
}

/* FUNCTION: comProcessWaveBuffer()
 * DESCRIPTION:
 *  
 *  Process the passed wave buffer.
 *  If packed 24 bit format, uses rp_float as a temporary buffer
 *  to convert to MS floating format (-1.0 to 1.0).
 *  i_down_sample_ratio is used on data with sampling frequencies greater than 48khz to down sample the data
 *  before processing and then upsample it after processing.
 *
 */
int PT_DECLSPEC comProcessWaveBuffer(PT_HANDLE *hp_com, long *lp_data, float *rp_float, long l_length, 
                         int i_stereo_in_mode, int i_stereo_out_mode, int i_down_sample_ratio,
								 int i_format_flag)
{
   /* l_length comes in with the buffer size in sample sets */
   struct comHdlType *cast_handle;
	long *l_ptr;
	float *f_ptr;
	int leftover_samples;
	int num_down_sample_sets;
	int num_sample_sets_to_process;
	int i,j;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

	num_sample_sets_to_process = l_length;

   if ( i_format_flag == COM_24_BIT_SAMPLES )
	{
		if (pwav24BitToFloat((char *)lp_data, rp_float, l_length, i_stereo_in_mode) != OKAY)
			return(NOT_OKAY);

		l_ptr = (long *)rp_float;
	}
	else
		l_ptr = lp_data;

   // 2020-05-01: Initialize local variable pointer f_ptr here so get around error "error C4703: potentially uninitialized local pointer variable 'f_ptr' used"
   // since /sdl option is turned on in DfxDsp (it is turned off in original code base so this was treated as warning instead of error).
	f_ptr = (float *)l_ptr;
   
	// Downsample the data if needed for higher data sampling frequencies
	if(i_down_sample_ratio > 1)
	{
		f_ptr = (float *)l_ptr;
		leftover_samples = l_length % i_down_sample_ratio;
		num_down_sample_sets = l_length/i_down_sample_ratio;
		num_sample_sets_to_process = num_down_sample_sets;

		if(i_stereo_in_mode)
		{
			for(i=0; i<num_down_sample_sets; i++)
			{
				f_ptr[i * 2]     = f_ptr[i * 2 * i_down_sample_ratio];
				f_ptr[i * 2 + 1] = f_ptr[i * 2 * i_down_sample_ratio + 1];
			}
		}
		else // Mono case
		{
			for(i=0; i< num_down_sample_sets; i++)
			{
				f_ptr[i] = f_ptr[i * i_down_sample_ratio];
			}
		}
	}

	if (cast_handle->softdsp_mode)
   {
      if (comSftwrProcessWaveBuffer(cast_handle->comSftwr_hdl, l_ptr, num_sample_sets_to_process, 
                                 i_stereo_in_mode, i_stereo_out_mode,
											i_format_flag) != OKAY)
		return(NOT_OKAY);
   }
   else
   {
		/* Hardware currently not supported */
		/*
      if (comHrdwrProcessWaveBuffer(cast_handle->board_address, lp_data, l_length, 
                                 i_stereo_in_mode, i_stereo_out_mode) != OKAY)
		 */
		return(NOT_OKAY);
   }

	// Upsample the data if needed for higher data sampling frequencies
	if(i_down_sample_ratio > 1)
	{
		if(i_stereo_out_mode)
		{
			for(i=(num_down_sample_sets - 1); i >= 0; i--)
			{
				// To be replace with sample averaging operation
				for(j=0; j<i_down_sample_ratio; j++)
				{
					f_ptr[i * 2 * i_down_sample_ratio + j * 2] = f_ptr[i * 2];
					f_ptr[i * 2 * i_down_sample_ratio + 1 + j * 2] = f_ptr[i * 2 + 1];
				}
			}
			if( leftover_samples > 0 ) // Fill in leftover samples with last processed value
				for(i=0; i<leftover_samples; i++)
				{
					f_ptr[ (num_down_sample_sets * i_down_sample_ratio + i) * 2 ] = 
							f_ptr[ (num_down_sample_sets * i_down_sample_ratio - 1) * 2 ];

					f_ptr[ (num_down_sample_sets * i_down_sample_ratio + i) * 2 + 1 ] = 
							f_ptr[ (num_down_sample_sets * i_down_sample_ratio - 1) * 2 + 1 ];
				}
		}
		else // Mono case
		{
			for(i=(num_down_sample_sets - 1); i >= 0; i--)
			{
				for(j=0; j<i_down_sample_ratio; j++)
					f_ptr[i * i_down_sample_ratio + j] = f_ptr[i];
			}

			if( leftover_samples > 0 ) // Fill in leftover samples with last processed value
				for(i=0; i<leftover_samples; i++)
				{
					f_ptr[ num_down_sample_sets * i_down_sample_ratio + i] = 
							f_ptr[ num_down_sample_sets * i_down_sample_ratio - 1];
				}
		}
	}

	if ( i_format_flag == COM_24_BIT_SAMPLES )
	{
		if (pwavFloatTo24Bit(rp_float, (char *)lp_data, l_length, i_stereo_out_mode) != OKAY)
			return(NOT_OKAY);
	}

   return(OKAY);
}
