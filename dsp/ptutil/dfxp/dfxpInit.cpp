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

/* dfxpInit.cpp */

#include "codedefs.h"

#include <windows.h>
#include <stdio.h>

/* For SHGetSpecialFolderPath */
#include <shlobj.h>

#include "u_dfxp.h" 

#include "dfxp.h"
#include "pt_defs.h"
#include "slout.h"
#include "spectrum.h"
#include "reg.h"
#include "SurroundSyn.h"
#include "BinauralSyn.h"
#include "mth.h"
#include "dfxpdefs.h"
#include "dfxSharedUtil.h"


#define DFX_VERSION 13

/*
 * FUNCTION: dfxpInit() 
 * DESCRIPTION:
 *   Initialize the global data.
 */
int dfxpInit(PT_HANDLE **hpp_dfxp, 
				 wchar_t *wcp_product_name, int i_vendor_code,
				 int i_num_first_trial_days, int i_num_extend_trial1_days,
				 int i_freemium_version, int i_allow_remix, long l_host_buffer_delay_msecs,
				 int i_oem_build, int i_processing_only, int i_trace_mode, CSlout *hp_slout)
{
	struct dfxpHdlType *cast_handle;
 
	/* Allocate the handle */
	cast_handle = (struct dfxpHdlType *)calloc(1, sizeof(struct dfxpHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->fully_initialized = IS_FALSE;

	/* Init trace info */
	cast_handle->trace.mode = i_trace_mode;
	cast_handle->trace.i_process_int_samples_done = IS_FALSE;
	cast_handle->trace.i_process_real_samples_done = IS_FALSE;

	cast_handle->slout1 = hp_slout;

	if (cast_handle->trace.mode)
	   (cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpInit: Entered [1]");

	/* Store the product name */
	if (wcp_product_name == NULL)
		return(NOT_OKAY);
	swprintf(cast_handle->wcp_product_name, L"%s", wcp_product_name);

	/* Store the vendor specific info */
	cast_handle->vendor_code = i_vendor_code;
	cast_handle->i_freemium_version = i_freemium_version;
	cast_handle->oem_build = i_oem_build;
	cast_handle->processing_only = i_processing_only;
	
	cast_handle->major_version = (int)DFX_VERSION;

	/* Init the processing override */
	cast_handle->processing_override = DFXP_PROCESSING_OVERRIDE_NONE;

	/* Calculates if this is the first time DFX has been run since installation. */
   if (dfxp_InitFirstTimeRunFlag((PT_HANDLE *)cast_handle) != OKAY)
		return(NOT_OKAY);

	/* 
	 * Initialize the shared memory.
	 */
//	if ((!cast_handle->allow_remix) && (i_processing_only))

	if (dfxSharedUtilInit(&(cast_handle->hp_sharedUtil), cast_handle->trace.mode, cast_handle->slout1) != OKAY)
		return(NOT_OKAY);
	
   /* Initialize the signal settings */
   cast_handle->sampling_freq = (realtype)DFXP_INIT_SAMPLING_FREQ;
	cast_handle->sampling_period = (realtype)1.0/(cast_handle->sampling_freq);
   cast_handle->internal_sampling_freq = cast_handle->sampling_freq;
	cast_handle->internal_sampling_period = cast_handle->sampling_period;
	cast_handle->internal_rate_ratio = 1;

	// If the actual signal sampling rate is greater than the max internal rate, the
	// internal rates will be reduced.
	if( cast_handle->sampling_freq > DFXP_MAX_INTERNAL_SAMPLING_FREQ )
	{
		if(cast_handle->sampling_freq < DFXP_MAX_SAMPLING_FREQ)
		{
			cast_handle->internal_sampling_freq = cast_handle->sampling_freq/(realtype)2.0;
			cast_handle->internal_sampling_period = (realtype)1.0/(cast_handle->internal_sampling_freq);
			cast_handle->internal_rate_ratio = 2;
		}
		else // 192khz case
		{
			cast_handle->internal_sampling_freq = cast_handle->sampling_freq/(realtype)4.0;
			cast_handle->internal_sampling_period = (realtype)1.0/(cast_handle->internal_sampling_freq);
			cast_handle->internal_rate_ratio = 4;
		}
	}

   cast_handle->bits_per_sample = DFXP_INIT_BITS_PER_SAMPLE;
   cast_handle->valid_bits = DFXP_INIT_VALID_BITS;
   cast_handle->num_channels_in = DFXP_INIT_NUM_CHANNELS;
   cast_handle->num_channels_out = DFXP_INIT_NUM_CHANNELS;
	cast_handle->unsupported_format_flag = IS_FALSE;

	/* Initialize the buffer delay in msecs */
	cast_handle->l_host_buffer_delay_msecs = l_host_buffer_delay_msecs;

	if (cast_handle->trace.mode)
	{
	   (cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpInit: Calling dfxp_InitAllQnts()");
   }

	/* Initialize the qnt handle */
	if (dfxp_InitAllQnts((PT_HANDLE *)cast_handle) != OKAY)
		return(NOT_OKAY);

	if (cast_handle->trace.mode)
	{
	   (cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpInit: Calling dfxp_CommunicateInit()");
   }

	/* Initialize the comm handle */
	if (dfxp_CommunicateInit((PT_HANDLE *)cast_handle) != OKAY)
		return(NOT_OKAY);


	if (cast_handle->trace.mode)
	{
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpInit: Calling SurroundSynNew()");
   }

	/* Create the SurroundSyn (2 channel to 6/8 channel synthesis) handle */
	if (SurroundSynNew(&(cast_handle->SurroundSyn_hdl)) != OKAY)
		return(NOT_OKAY);

	if (cast_handle->trace.mode)
	{
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpInit: Calling BinauralSynNew()");
   }

	/* Create the BinauralSyn (6/8 channel to 2 channel synthesis) handle */
	if (BinauralSynNew(&(cast_handle->BinauralSyn_hdl), BINAURAL_SYN_DEFAULT_NUM_COEFFS) != OKAY)
		return(NOT_OKAY);

	cast_handle->binaural_headphone_on_flag = IS_FALSE;

	if (cast_handle->trace.mode)
	{
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpInit: Initializing Graphic EQ");
   }

	/* Initialize Graphic EQ */
	if (dfxp_EqInit((PT_HANDLE *)cast_handle) != OKAY)
		return(NOT_OKAY);

	if (cast_handle->trace.mode)
	{
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpInit: Initializing Spectrum()");
   }

	/* Initialize the spectrum info */
	if (dfxp_SpectrumInit((PT_HANDLE *)cast_handle) != OKAY)
			return(NOT_OKAY);

	if (cast_handle->trace.mode)
	{
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpInit: Initializing Stream Analysis");
   }

	/* Initialize count for communicating slider changes slowly */
	cast_handle->i_communicate_slowly_count = 0;

	cast_handle->ul_total_msecs_audio_processed_time = 0;

	cast_handle->fully_initialized = IS_TRUE;

	/****** DO THE TASKS WHICH MUST BE DONE AFTER FULLY_INITIALIZIED ******/

	/* Init the longest buffer so far to 0 */
	if (dfxp_StoreLongestBufferSize((PT_HANDLE *)cast_handle, 0) != OKAY)
		return(NOT_OKAY);

	/* Init the temporary bypass all setting to not bypass */
	if (dfxpSetTemporaryBypassAll((PT_HANDLE *)cast_handle, IS_FALSE) != OKAY)
		return(NOT_OKAY);

   *hpp_dfxp = (PT_HANDLE *)cast_handle;

	if (cast_handle->trace.mode)
	{
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpInit: returning OKAY");
   }

	return(OKAY);
}


/*
 * FUNCTION: dfxp_InitFirstTimeRunFlag()
 * DESCRIPTION:
 *
 *  Check if this is the first time the DFX has been run since the last installation.
 *  It does this by comparing the last run date with the installation date.
 *
 */
int dfxp_InitFirstTimeRunFlag(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	long installation_date;
	long last_run_date;
   int installation_date_exists;
	int last_run_date_exists;

	cast_handle->first_time_run_flag = IS_TRUE;

	/* Get the dates */
	if (dfxpGetInstallationDate(hp_dfxp, &installation_date, &installation_date_exists) != OKAY)
		return(NOT_OKAY);
	if (dfxpGetLastUsedDate(hp_dfxp, &last_run_date, &last_run_date_exists) != OKAY)
		return(NOT_OKAY);

	/* Record current date as date that dfx was last used */
   if (dfxp_RecordLastUsedDate(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	/* If either date does not exist, treat it as the first time run */
	if ((!installation_date_exists) || (!last_run_date_exists))
      return(OKAY);

   /* Check if installation date is older than last run date */
	if (installation_date < last_run_date)
	   cast_handle->first_time_run_flag = IS_FALSE;

	return(OKAY);
}

