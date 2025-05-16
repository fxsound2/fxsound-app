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
#include "codedefs.h"

/* Standard includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "slout.h"
#include "mry.h"
#include "file.h"
#include "reg.h"

#include "u_dfxSharedUtil.h"
#include "dfxSharedUtil.h"

#define DFXP_REGISTRY_DFX_PRODUCT_NAME_WIDE L"DFX"

/*
 * FUNCTION: dfxSharedUtilInit()
 * DESCRIPTION:
 *
 *  Allocates and initializes the passed dfxSharedUtil handle.  
 *
 */
int PT_DECLSPEC dfxSharedUtilInit(PT_HANDLE **hpp_dfxSharedUtil,
											  int i_trace_mode,
											  CSlout *hp_slout)
{
	struct dfxSharedUtilHdlType *cast_handle;

	/* Allocate the handle */
	cast_handle = (struct dfxSharedUtilHdlType *)calloc(1,
				  sizeof(struct dfxSharedUtilHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);

	wchar_t wcp_dll_fullpath[PT_MAX_PATH_STRLEN];
	wchar_t wcp_top_shared_folder_path[PT_MAX_PATH_STRLEN];

	/*
	 * Store the trace mode.
	 */
	cast_handle->trace_mode = i_trace_mode;

	cast_handle->slout_hdl = hp_slout;

	cast_handle->shared_memory_hinst = NULL;
	cast_handle->sp_shared_memory_data = NULL;

#if 0
	/* Get the common files folder path ( "C:\\Program Files\\Common Files\\DFX" ) */
	if (dfxSharedUtil_RegistryGetTopSharedFolderPath((PT_HANDLE *)cast_handle, wcp_top_shared_folder_path) != OKAY)
		return(NOT_OKAY);

	/* Construct full path to shared dll */
#ifdef _WIN64
	swprintf(wcp_dll_fullpath, L"%s\\Dlls\\dfxShared64.dll", wcp_top_shared_folder_path);
#else
	swprintf(wcp_dll_fullpath, L"%s\\Dlls\\dfxShared32.dll", wcp_top_shared_folder_path);
#endif

	cast_handle->shared_memory_hinst = LoadLibrary(wcp_dll_fullpath);		
	if (cast_handle->shared_memory_hinst == NULL)
		return(NOT_OKAY);

	// Get the pointer to the DFX globals shared memory structure
	cast_handle->sp_shared_memory_data = (struct dfxSharedGlobalsType *)GetProcAddress(cast_handle->shared_memory_hinst, "dfxSharedGlobals");
#endif

	*hpp_dfxSharedUtil = (PT_HANDLE *)cast_handle;

	return(OKAY);
}

/*
 * FUNCTION: dfxSharedUtilFreeUp()
 * DESCRIPTION:
 *   Frees the passed dfxSharedUtil handle and sets to NULL.
 */
int PT_DECLSPEC dfxSharedUtilFreeUp(PT_HANDLE **hpp_dfxSharedUtil)
{
	struct dfxSharedUtilHdlType *cast_handle;
    
	cast_handle = (struct dfxSharedUtilHdlType *)(*hpp_dfxSharedUtil);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Free shared memory library */
	if (cast_handle->shared_memory_hinst != NULL)
		FreeLibrary(cast_handle->shared_memory_hinst);

	free(cast_handle);

	*hpp_dfxSharedUtil = NULL;

	return(OKAY);
}

/*
 * FUNCTION: dfxSharedUtil_RegistryGetTopSharedFolderPath()
 * DESCRIPTION:
 *
 *  Retrieves from the registry the top shared folder path
 *  (C:\\Program Files\\Common Files\\DFX)
 */
int dfxSharedUtil_RegistryGetTopSharedFolderPath(PT_HANDLE *hp_dfxSharedUtil, wchar_t *wcp_top_shared_folder_path)
{
	struct dfxSharedUtilHdlType *cast_handle;
    
	cast_handle = (struct dfxSharedUtilHdlType *)(hp_dfxSharedUtil);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
	int key_exists;

	if (wcp_top_shared_folder_path == NULL)
		return(NOT_OKAY);

	swprintf(wcp_top_shared_folder_path, L"");

	swprintf(wcp_full_key_path, L"%s\\%s\\%s\\%s", DFXP_REGISTRY_TOP_WIDE, 
			     DFXP_REGISTRY_DFX_PRODUCT_NAME_WIDE, 
				  DFXP_REGISTRY_SHARED_WIDE,
		        DFXP_REGISTRY_TOP_SHARED_FOLDER_WIDE);

	if (regReadKey_Wide(REG_LOCAL_MACHINE, wcp_full_key_path, &key_exists, wcp_top_shared_folder_path,
				(unsigned long)DFXP_REGISTRY_BUFFER_LENGTH) != OKAY)
	         return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxSharedUtilSetSpectrumValues() 
 * DESCRIPTION:
 *
 *  Stores the current spectrum values in the shared memory.
 *
 */
int PT_DECLSPEC dfxSharedUtilSetSpectrumValues(PT_HANDLE *hp_dfxSharedUtil, realtype *rp_band_values, int i_num_bands)
{
	struct dfxSharedUtilHdlType *cast_handle;
    
	cast_handle = (struct dfxSharedUtilHdlType *)(hp_dfxSharedUtil);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	int index;
	
	if (cast_handle->sp_shared_memory_data == NULL)
		return(OKAY);

	if (i_num_bands != DFXP_SPECTRUM_NUM_BANDS)
		return(NOT_OKAY);

	for (index = 0; index < DFXP_SPECTRUM_NUM_BANDS; index++)
	{
		(cast_handle->sp_shared_memory_data)->rp_meter_vals[index] = rp_band_values[index];
	}

	return(OKAY);
}

/*
 * FUNCTION: dfxSharedUtilSetTotalProcessedTime() 
 * DESCRIPTION:
 *
 *  Stores the total number of msecs processed since shared memory was loaded.
 *
 */
int PT_DECLSPEC dfxSharedUtilSetTotalProcessedTime(PT_HANDLE *hp_dfxSharedUtil, unsigned long ul_total_processed_time_msecs)
{
	struct dfxSharedUtilHdlType *cast_handle;
    
	cast_handle = (struct dfxSharedUtilHdlType *)(hp_dfxSharedUtil);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->sp_shared_memory_data == NULL)
		return(OKAY);

	(cast_handle->sp_shared_memory_data)->ul_msecs_processed = ul_total_processed_time_msecs;

	return(OKAY);
}

/*
 * FUNCTION: dfxSharedUtilGetTotalProcessedTime() 
 * DESCRIPTION:
 *
 *  Passes back the total number of msecs processed since shared memory was loaded.
 *
 */
int PT_DECLSPEC dfxSharedUtilGetTotalProcessedTime(PT_HANDLE *hp_dfxSharedUtil, unsigned long *ulp_total_processed_time_msecs)
{
	struct dfxSharedUtilHdlType *cast_handle;
    
	cast_handle = (struct dfxSharedUtilHdlType *)(hp_dfxSharedUtil);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->sp_shared_memory_data == NULL)
		return(OKAY);

	*ulp_total_processed_time_msecs = (cast_handle->sp_shared_memory_data)->ul_msecs_processed;

	return(OKAY);
}

/*
 * FUNCTION: dfxSharedUtilGetSpectrumValues() 
 * DESCRIPTION:
 *
 *  Retrieves the current spectrum values in the shared memory.
 *
 */
int PT_DECLSPEC dfxSharedUtilGetSpectrumValues(PT_HANDLE *hp_dfxSharedUtil, realtype *rp_band_values, int i_num_bands)
{
	struct dfxSharedUtilHdlType *cast_handle;
    
	cast_handle = (struct dfxSharedUtilHdlType *)(hp_dfxSharedUtil);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->sp_shared_memory_data == NULL)
		return(OKAY);

	int index;

	if (i_num_bands > DFXP_SPECTRUM_NUM_BANDS)
		return(NOT_OKAY);

	for (index = 0; index < i_num_bands; index++)
	{
		rp_band_values[index] = (cast_handle->sp_shared_memory_data)->rp_meter_vals[index];
	}

	return(OKAY);
}

/*
 * FUNCTION: dfxSharedUtilSetFlag()
 * DESCRIPTION:
 *   Frees the passed dfxSharedUtil handle and sets to NULL.
 */
int PT_DECLSPEC dfxSharedUtilSetFlag(PT_HANDLE *hp_dfxSharedUtil, int i_flag_type, int i_flag_value)
{
	struct dfxSharedUtilHdlType *cast_handle;
    
	cast_handle = (struct dfxSharedUtilHdlType *)(hp_dfxSharedUtil);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->sp_shared_memory_data == NULL)
		return(OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxSharedUtilGetFlag()
 * DESCRIPTION:
 *
 *   Sets the passed flag value into the shared memeory.
 */
int PT_DECLSPEC dfxSharedUtilGetFlag(PT_HANDLE *hp_dfxSharedUtil, int i_flag_type, int *ip_flag_value)
{
	struct dfxSharedUtilHdlType *cast_handle;
    
	cast_handle = (struct dfxSharedUtilHdlType *)(hp_dfxSharedUtil);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->sp_shared_memory_data == NULL)
		return(OKAY);

	return(OKAY);
}
