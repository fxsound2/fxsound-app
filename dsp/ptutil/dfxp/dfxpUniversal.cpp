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

/* dfxpUniversal.cpp */

#include "codedefs.h"

#include <Windows.h>
#include <math.h>
#include <stdio.h>

#include "u_dfxp.h" 

#include "dfxp.h"
#include "pt_defs.h"
#include "slout.h"
#include "dfxpDefs.h"
#include "DfxSdk.h"
#include "pstr.h"
#include "dfxSharedUtil.h"

/*
 * FUNCTION: dfxp_UniversalInitPaths()
 * DESCRIPTION:
 *
 *   Initialize the top folders and other paths
 *
 */
int dfxp_UniversalInitPaths(PT_HANDLE *hp_dfxp)
{	
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	wchar_t wcp_top_vendor_specific_folder_path[PT_MAX_PATH_STRLEN];
	wchar_t wcp_parent_exe_path[PT_MAX_PATH_STRLEN];
	int i_length;

	/* Get the top vendor specific folder (ex. C:\\Program Files\\DFX\\Universal) */
	if (dfxp_RegistryGetTopVendorSpecificFolderPath(hp_dfxp, wcp_top_vendor_specific_folder_path) != OKAY)
		return(NOT_OKAY);

	/* Get the path to the dfx ui exe */
	if (dfxp_RegistryGetDfxUniversalUiFullpath(hp_dfxp, cast_handle->universal.wcp_dfx_ui_path) != OKAY)
		return(NOT_OKAY);

	/* Get the path to the parent exe (ex. C:\Program Files\Winamp\winamp.exe) */
	i_length = GetModuleFileName(0, wcp_parent_exe_path, PT_MAX_PATH_STRLEN);
	if (i_length > 0)
	{
		/* Create an uppercase version of the path so we can do a case insenstive search */
		if (pstrToUpper_Wide(wcp_parent_exe_path,  cast_handle->universal.wcp_parent_exe_path_uppercase) != OKAY)
			return(NOT_OKAY);
	}
	else
		wsprintf(cast_handle->universal.wcp_parent_exe_path_uppercase, L"");

	return(OKAY);
}

/*
 * FUNCTION: dfxpUniversalSetSignalFormat()
 * DESCRIPTION:
 *
 *   Takes the current signal format and if it has changed then update processing code settings accordingly.
 *   This function can be called with each buffer.
 *
 */
int dfxpUniversalSetSignalFormat(PT_HANDLE *hp_dfxp, int i_bps, int i_nch, int i_srate, int i_valid_bits)
{	
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* Check if the format has changed */  
	if ((i_bps != cast_handle->universal.last_called_bps) || 
		 (i_nch != cast_handle->universal.last_called_nch) || 
		 (i_srate != cast_handle->universal.last_called_srate) || 
		 (i_valid_bits != cast_handle->universal.last_called_valid_bits))
	{
	   /* Note call is always set up for 32 bit processing since conversion is done
		 * before and after processing calls
		 */
		cast_handle->universal.last_called_bps = i_bps;
		cast_handle->universal.last_called_nch = i_nch;
		cast_handle->universal.last_called_srate = i_srate;
		cast_handle->universal.last_called_valid_bits = i_valid_bits;

		if (dfxpBeginProcess(hp_dfxp, i_bps, i_nch, i_srate) != OKAY)
			return(NOT_OKAY);

		if (dfxpSetValidBits(hp_dfxp, i_valid_bits) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: dfxpUniversalModifySamples()
 * DESCRIPTION:
 *
 *   Takes the current signal format and if it has changed then update processing code settings accordingly.
 *   This function can be called with each buffer.
 *
 */
int dfxpUniversalModifySamples(PT_HANDLE *hp_dfxp, short int *si_input_samples, short int *si_output_samples, int i_num_sample_sets, int i_check_for_duplicate_buffers)
{	
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	realtype *r_in, *r_out;
	int index;
	int i;
	int process_count;				// The number of processing loops used to process buffer
	int leftover_sample_sets;		// The number of sample sets not processed in loop calls
	int num_process_loop;			// The number of sample sets that will be processed per loop
	int total_bytes_to_increment;	// Used to increment sample pointers
	int i_reorder;
	BYTE *bp_in;						// Will be used to implement processing loops
	BYTE *bp_out;
	int i_do_not_process;
	int bytes_total;
	int byte_index;
	int i_current_buffer_hash;
	int hash_index;
	BOOL b_lean_and_mean;
	int i_is_all_zeros;

/*
	if (cast_handle->trace.mode)
	{
	   swprintf(cast_handle->wcp_msg1, L"dfxpUniversalModifySamples: Entered");
	   (cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	}
*/

	/* 
	 * Set whether we are going to do lean and mean processing which is necessary for the new
	 * DFX 11 style which uses the virtual soundcard.
	 */
	b_lean_and_mean = FALSE;
	if (!cast_handle->processing_only)
	{
		b_lean_and_mean = TRUE;
	}

	/* Calculate the total number of bytes in the input buffer */
	bytes_total = i_num_sample_sets * cast_handle->universal.last_called_nch *cast_handle->universal.last_called_bps / 8;

	i_do_not_process = IS_FALSE;

	// If surround sound, set sample reorder flag
	if ( cast_handle->universal.last_called_nch > 2 )
		i_reorder = IS_TRUE;
	else
		i_reorder = IS_FALSE;

	// Note that dfxForWmp.cpp already forces a buffer size that works well for metering.
	// Here we just need a protection against a buffer that is too large, we process it in chunks.
	// Break processing into multiple calls if number of samples sets is above our desired size, but only in stereo or mono case
	// Original calc below is thus no longer needed, kept for reference.
	//
	// desired_sample_set_size = (int)( (realtype)i_srate * (realtype)DFX_FOR_WMP_MAX_BUFFER_SIZE_MS * (realtype)0.001 );

	if( i_num_sample_sets > (int)DAW_MAX_BUFFER_SIZE )
	{
		process_count = i_num_sample_sets/(int)DAW_MAX_BUFFER_SIZE;
		leftover_sample_sets = i_num_sample_sets - (process_count * (int)DAW_MAX_BUFFER_SIZE);
		num_process_loop = (int)DAW_MAX_BUFFER_SIZE;
	}
	else
	{	// This is the case were the number of sample sets is smaller than or equal to our max size
		process_count = 1;
		leftover_sample_sets = 0;
		num_process_loop = i_num_sample_sets;
	}

	bp_in = (BYTE *)si_input_samples;
	bp_out = (BYTE *)si_output_samples;
	total_bytes_to_increment = num_process_loop * cast_handle->universal.last_called_nch * (cast_handle->universal.last_called_bps/8); // Calculate the pointer increment in bytes

	for(i=0; i<process_count; i++)	// Loop to process sub-buffers
	{
		if ( (cast_handle->universal.last_called_bps == 8) || (cast_handle->universal.last_called_bps == 16) || (cast_handle->universal.last_called_bps == 24) )
		{
			if (dfxpModifyShortIntSamples(hp_dfxp, (short int *)bp_in, (short int *)bp_out, num_process_loop) != OKAY)
				return(NOT_OKAY);
		}
		else if ( cast_handle->universal.last_called_bps == 32 )
		{
			// Real processing does processing in place, first copy output to input
			r_in = (realtype *)bp_in;
			r_out = (realtype *)bp_out;

			for(index=0; index < (num_process_loop * cast_handle->universal.last_called_nch); index++)
				r_out[index] = r_in[index];

			if (dfxpModifyRealtypeSamples(hp_dfxp, r_out, num_process_loop, i_reorder) != OKAY)
				return(NOT_OKAY);
		}

		bp_in += total_bytes_to_increment;	// Increment sample pointers
		bp_out += total_bytes_to_increment;
	}

	if( leftover_sample_sets > 0 ) // If there are any leftover samples, process them
	{
		if ( (cast_handle->universal.last_called_bps == 8) || (cast_handle->universal.last_called_bps == 16) || (cast_handle->universal.last_called_bps == 24) )
		{
			if (dfxpModifyShortIntSamples(hp_dfxp, (short int *)bp_in, (short int *)bp_out, leftover_sample_sets) != OKAY)
				return(NOT_OKAY);
		}
		else if ( cast_handle->universal.last_called_bps == 32 )
		{
			// Real processing does processing in place, first copy output to input
			r_in = (realtype *)bp_in;
			r_out = (realtype *)bp_out;

			for(index=0; index < (leftover_sample_sets * cast_handle->universal.last_called_nch); index++)
				r_out[index] = r_in[index];

			if (dfxpModifyRealtypeSamples(hp_dfxp, r_out, leftover_sample_sets, i_reorder) != OKAY)
				return(NOT_OKAY);
		}
	}

	/* Calculate the hash for the processed buffer */
	if (!b_lean_and_mean)
	{
		if (i_check_for_duplicate_buffers)
		{
			if (dfxp_UniversalCalcBufferHash(hp_dfxp, (BYTE *)si_output_samples, bytes_total, &(cast_handle->universal.hash_queue_vals[cast_handle->universal.hash_queue_index])) != OKAY)
				return(NOT_OKAY);

			/* Trace the hash value of the processed buffer */
			if (cast_handle->trace.mode)
			{
				swprintf(cast_handle->wcp_msg1, L"dfxpUniversalModifySamples: processed hash value = %d", cast_handle->universal.hash_queue_vals[cast_handle->universal.hash_queue_index]);
				(cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);	
			}

			/* Increment the index for the next hash value to store */
			(cast_handle->universal.hash_queue_index)++;
			if (cast_handle->universal.hash_queue_index == DFXP_UNIVERSAL_HASH_QUEUE_SIZE)
				cast_handle->universal.hash_queue_index = 0;
		}
	}

	/* 
	 * For some strange reason, some players like the windows media player continually play 
	 * all zero (silent) buffers.  We don't want to count these buffers for the purpose of trial
	 * version processing time limit.
	 */
	if (dfxp_UniversalIsBufferAllSilence(hp_dfxp, si_input_samples, i_num_sample_sets, 
												  cast_handle->universal.last_called_nch, cast_handle->universal.last_called_valid_bits, &i_is_all_zeros) != OKAY)
		return(NOT_OKAY);

	if (!i_is_all_zeros)
	{
		/* Update the total processed time in shared memory which will eventually be sent to the UI */
		if (dfxp_UniversalUpdateTotalTimeProcessed(hp_dfxp, i_num_sample_sets) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: dfxp_UniversalIsBufferAllSilence()
 * DESCRIPTION:
 *
 *   Passes back whether or not the passed buffer is the windows media equivalent of silence.
 *   When playback has stopped in WMP and apps that use the Windows audio system like Spotify,
 *   for some reason with 32 bit float data the windows audio system keeps sending buffers with very small offset which
 *   approaches 6.3606902e-27. This function will assume that any buffer with values all less than
 *   1.0e-20 is silence.
 *
 */
int dfxp_UniversalIsBufferAllSilence(PT_HANDLE *hp_dfxp, short int *si_input_samples, int i_num_sample_sets, int i_num_channels, int i_num_bits, int *ip_is_all_zeros)
{	
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	int i_max_index;
	int total_bytes;
	BYTE *bytes;
	realtype *rpArray;
	int i;

	*ip_is_all_zeros = IS_TRUE;

	if (i_num_sample_sets == 0)
		return(OKAY);

	if (i_num_channels == 0)
		return(OKAY);

	i_max_index = i_num_sample_sets * i_num_channels;

	// In the int case they likely are not using the offset value seen in the 32 bit float case, do a normal check.
	if( i_num_bits < 32 )
	{
		total_bytes = (i_num_bits/8) * i_max_index;
		bytes = (BYTE *)si_input_samples;

		for(i=0; i<i_max_index; i++)
		{
			if (bytes[i] != 0)
			{
				*ip_is_all_zeros = IS_FALSE;
				break;
			}
		}
	}
	else
	{
		rpArray = (realtype *)si_input_samples;

		for(i=0; i<i_max_index; i++)
		{
			if ( fabs(rpArray[i]) > (realtype)1.0e-20 ) // Since window system uses and offset, can't check for zero.
			{
				*ip_is_all_zeros = IS_FALSE;
				break;
			}
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: dfxp_UniversalUpdateTotalTimeProcessed()
 * DESCRIPTION:
 *
 *   Update the total amount of time processed by adding the amount of time processed by the last buffer.
 *
 */
int dfxp_UniversalUpdateTotalTimeProcessed(PT_HANDLE *hp_dfxp, int i_num_sample_sets)
{	
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	unsigned long ul_msecs_processed_by_buffer;
	unsigned long ul_previous_total_processed_time_msecs;
	unsigned long ul_new_total_processed_time_msecs;
	int bypass_all;

	realtype r_num_secs;

	if (cast_handle->universal.last_called_srate <= 0)
		return(OKAY);

	if (cast_handle->hp_sharedUtil == NULL)
		return(OKAY);

	r_num_secs = (realtype)i_num_sample_sets / (realtype)cast_handle->universal.last_called_srate;
	ul_msecs_processed_by_buffer = (long)(r_num_secs * (realtype)1000);

	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_BYPASS, &bypass_all) != OKAY)
		return(NOT_OKAY);

	if (!bypass_all)
	{
		cast_handle->ul_total_msecs_audio_processed_time += ul_msecs_processed_by_buffer;
	}
	
	/* Get the old total processed time */
	if (dfxSharedUtilGetTotalProcessedTime(cast_handle->hp_sharedUtil, &ul_previous_total_processed_time_msecs) != OKAY)
		return(NOT_OKAY);

	/* 
	 * Calculate the new total processed time 
	 * NOTE: It is okay if this wraps around because all the comparison values will also wrap around.
	 */
	ul_new_total_processed_time_msecs = ul_previous_total_processed_time_msecs + ul_msecs_processed_by_buffer;

	/* Store the new total processed time in shared memory */
	if (dfxSharedUtilSetTotalProcessedTime(cast_handle->hp_sharedUtil, ul_new_total_processed_time_msecs) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}


/*
 * FUNCTION: dfxp_UniversalCalcBufferHash()
 * DESCRIPTION:  Generates a hash code for the audio buffer content.  
 *					  Generates a zero value if the buffer is all zero.
 */
int dfxp_UniversalCalcBufferHash(PT_HANDLE *hp_dfxp, BYTE *lpData, DWORD dw_buffer_length, int *ip_hashcode)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	const int p = 16777619; 
	int hash = (int)2166136261; 
	unsigned int i;
	BOOL all_zero = TRUE;

	*ip_hashcode = 0;

	if ((lpData == NULL) || (dw_buffer_length <= 0))
		return(OKAY);

	for (i=0; i < dw_buffer_length; i++) 
	{
		if( lpData[i] != 0 )
			all_zero = FALSE;

		hash = (hash ^ (lpData[i]) ) * p;
	}

	hash += hash << 13; 
	hash ^= hash >> 7; 
	hash += hash << 3; 
	hash ^= hash >> 17; 
	hash += hash << 5;

	if( all_zero == TRUE )
		hash = 0;

	*ip_hashcode = hash; 

	return(OKAY);
}

/*
 * FUNCTION: dfxpUniversalCheckParentCompatibility()
 * DESCRIPTION:
 *
 *   Check if the parent app is compatible with processing. 
 *
 *   For example, if the processing module is dsound.dll and the parent is Winamp, then we don't
 *   want to do processing because it will be taken care of in the dfxForWinamp dll.  We don't want
 *   to do double processing.
 *  
 */
int dfxpUniversalCheckParentCompatibility(PT_HANDLE *hp_dfxp, int i_processing_module_type, int *ip_allow_processing)
{	
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	*ip_allow_processing = IS_TRUE;

	int i_location;
	int i_found;

	if ((i_processing_module_type == DFX_UNIVERSAL_PROCESSING_TYPE_DSOUND_DLL) ||
		 (i_processing_module_type == DFX_UNIVERSAL_PROCESSING_TYPE_WINMM_DLL))
	{
		/* Check if the parent is a player with a player specific plugin */
		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"WINAMP", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			*ip_allow_processing = IS_FALSE;

		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"WINDOWS MEDIA PLAYER", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			*ip_allow_processing = IS_FALSE;

		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"WMPLAYER.EXE", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			*ip_allow_processing = IS_FALSE;

		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"BSPLAYER", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			*ip_allow_processing = IS_FALSE;

		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"FOOBAR2000", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			*ip_allow_processing = IS_FALSE;

		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"GOMPLAYER", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			*ip_allow_processing = IS_FALSE;

		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"GOM.EXE", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			*ip_allow_processing = IS_FALSE;

		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"JETAUDIO", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			*ip_allow_processing = IS_FALSE;

		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"MEDIAMONKEY", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			*ip_allow_processing = IS_FALSE;

		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"DIVX", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			*ip_allow_processing = IS_FALSE;
	}

	/* Also make sure the website is not Netflix */
/*
	int i_incompatable_website;
	int i_no_need_to_check_for_incompatible_website;

	if (i_processing_module_type == DFX_UNIVERSAL_PROCESSING_TYPE_DSOUND_DLL)
	{
		i_no_need_to_check_for_incompatible_website = IS_FALSE;

		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"ITUNES", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			i_no_need_to_check_for_incompatible_website = IS_TRUE;

		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"SPOTIFY", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			i_no_need_to_check_for_incompatible_website = IS_TRUE;

		if (pstrCalcLocationOfStrInStr_Wide(cast_handle->universal.wcp_parent_exe_path_uppercase, L"VLC.EXE", 0, &i_location, &i_found) != OKAY)
			return(NOT_OKAY);
		if (i_found)
			i_no_need_to_check_for_incompatible_website = IS_TRUE;

		if (!i_no_need_to_check_for_incompatible_website)
		{
			if (dfxp_UniversalCheckIfIncompatableWebsiteForProcessing(hp_dfxp, &i_incompatable_website) != OKAY)
				return(NOT_OKAY);
			if (i_incompatable_website)
				*ip_allow_processing = IS_FALSE;
		}
	}
*/

	if (cast_handle->trace.mode)
	{
		swprintf(cast_handle->wcp_msg1, L"dfxpUniversalCheckParentCompatibility: allow_processing = %d", *ip_allow_processing);
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	}

	return(OKAY);
}

/*
 * FUNCTION: dfxp_UniversalCheckIfIncompatableWebsiteForProcessing()
 * DESCRIPTION:
 *
 *   Some websites like Netflix which use Silverlight are incompatable with processing by DFX.
 *  
 */
int dfxp_UniversalCheckIfIncompatableWebsiteForProcessing(PT_HANDLE *hp_dfxp, int *ip_incompatable_website)
{	
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	*ip_incompatable_website = IS_FALSE;

	cast_handle->universal.i_found_incompatable_website_for_processing = IS_FALSE;

   EnumWindows((WNDENUMPROC)&dfxp_UniversalEnumWindowsProc, (LPARAM)hp_dfxp);

	if (cast_handle->universal.i_found_incompatable_website_for_processing)
		*ip_incompatable_website = IS_TRUE;

	return(OKAY);
}

/*
 * FUNCTION: dfxp_UniversalEnumWindowsProc()
 * DESCRIPTION:
 *  
 * This is the callback function for enumerating all the top level windows.  It is called from
 * the function dfxp_UniversalCheckIfIncompatableWebsiteForProcessing().
 *
 * If such a window is found, it sets the global flag stating that such a window has been found.
 *
 * NOTE: THIS IS A CALLBACK FUNCTION WHICH MUST RETURN "TRUE".
 *
 */
BOOL CALLBACK dfxp_UniversalEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(lParam);

	if (cast_handle == NULL)
		return(OKAY);

	wchar_t wcp_window_title[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_window_title_uppercase[PT_MAX_GENERIC_STRLEN];
	int i_location;
	int i_found;

	/* Note: We do not check for matching process ids because the parent exe is not the browser (it can be plugin-container.exe) */
//	int dw_current_process_id;
//	int dw_window_process_id;

	/* Get the current process id */
//	dw_current_process_id = GetCurrentProcessId();

	/* Get the process id of the top level window being enumerated */
//	dw_window_process_id = GetWindowThreadProcessId(hwnd, NULL);

	/* If the window is not from the same process as the current process, then ignore */
//	if (dw_current_process_id != dw_window_process_id)
//		return(TRUE);

	/* Get the title of the window */
	GetWindowText(hwnd, wcp_window_title, PT_MAX_GENERIC_STRLEN);

	if (wcslen(wcp_window_title) <= 0)
		return(TRUE);

	/* Create an uppercase version of the title so we can do a case insenstive search */
	if (pstrToUpper_Wide(wcp_window_title, wcp_window_title_uppercase) != OKAY)
		return(TRUE);

	/* Search for "Netflix" */
	if (pstrCalcLocationOfStrInStr_Wide(wcp_window_title_uppercase, L"NETFLIX", 0, &i_location, &i_found) != OKAY)
		return(TRUE);
	if (i_found)
		cast_handle->universal.i_found_incompatable_website_for_processing = IS_TRUE;

	return(TRUE);
}




