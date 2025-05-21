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

/* dfxpRegistry.cpp */

#include "codedefs.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <shlobj.h>

#include "u_dfxp.h" 

#include "dfxp.h"
#include "reg.h"
#include "mth.h"
#include "DfxSdk.h"
#include "mth.h"
#include "file.h"
#include "pstr.h"

/*
 * FUNCTION: dfxpGetInstallationDate() 
 * DESCRIPTION:
 *   Get the date that the dfx was installed.
 */
int dfxpGetInstallationDate(PT_HANDLE *hp_dfxp, long *lp_date, int *ip_date_exists)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	*ip_date_exists = IS_FALSE;

	if (cast_handle == NULL)
		return(OKAY);

	if (dfxpGetInstallationDate_NoHandle(cast_handle->wcp_product_name, cast_handle->vendor_code, 
													 lp_date, ip_date_exists) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxpGetInstallationDate_NoHandle() 
 * DESCRIPTION:
 *   Get the date that the dfx was installed.  This version of the function does not require a dfxp handle.
 *   It is useful for getting the installation date at a point in the program at which the dfxp handle has not
 *   been completely initialized yet.
 */
int dfxpGetInstallationDate_NoHandle(wchar_t *wcp_product_name, int i_vendor_code, long *lp_date, int *ip_date_exists)
{
	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
   wchar_t wcp_date[DFXP_REGISTRY_BUFFER_LENGTH];
	int is_long;

	*ip_date_exists = IS_FALSE;

	if (i_vendor_code == 0)
      return(OKAY);
	if (wcp_product_name == NULL)
		return(NOT_OKAY);

	swprintf(wcp_full_key_path, L"%s\\%s\\%d\\%s", DFXP_REGISTRY_TOP_WIDE, 
			                  wcp_product_name, 
									i_vendor_code,
		                     DFXP_REGISTRY_DATE_INSTALLED_WIDE);

	if (regReadKey_Wide(REG_LOCAL_MACHINE, wcp_full_key_path, ip_date_exists, wcp_date,
	   (unsigned long)DFXP_REGISTRY_BUFFER_LENGTH) != OKAY)
	      return(NOT_OKAY);

	if (!(*ip_date_exists))
		return(OKAY);

	if (mthIsLong_Wide(wcp_date, &is_long) != OKAY)
		return(NOT_OKAY);

	if (!is_long)
		return(OKAY);

	*lp_date = _wtol(wcp_date);

	*ip_date_exists = IS_TRUE;

	return(OKAY);
}

/*
 * FUNCTION: dfxpGetLastUsedDate() 
 * DESCRIPTION:
 *   Passes the date that the current user last used dfx. 
 */
int dfxpGetLastUsedDate(PT_HANDLE *hp_dfxp, long *lp_date, int *ip_date_exists)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
	int is_long;
   wchar_t wcp_date[DFXP_REGISTRY_BUFFER_LENGTH];

	*ip_date_exists = IS_FALSE;

	if (cast_handle->vendor_code == 0)
      return(OKAY);

	swprintf(wcp_full_key_path, L"%s\\%s\\%d\\%d\\%s", 
									DFXP_REGISTRY_TOP_WIDE, 
									cast_handle->wcp_product_name, 
									cast_handle->major_version,
									cast_handle->vendor_code,
		                     DFXP_REGISTRY_DATE_LASTUSED_WIDE);

	if (regReadKey_Wide(REG_CURRENT_USER, wcp_full_key_path, ip_date_exists, wcp_date,
	               (unsigned long)DFXP_REGISTRY_BUFFER_LENGTH) != OKAY)
	      return(NOT_OKAY);

	if (!(*ip_date_exists))
		return(OKAY);

	if (mthIsLong_Wide(wcp_date, &is_long) != OKAY)
		return(NOT_OKAY);

	if (!is_long)
		return(OKAY);

	*lp_date = _wtol(wcp_date);

	*ip_date_exists = IS_TRUE;

	return(OKAY);
}

/*
 * FUNCTION: dfxp_RecordLastUsedDate() 
 * DESCRIPTION:
 *   Records in the registry the last used date as the current number of seconds
 *   since 1/1/1970.
 */
int dfxp_RecordLastUsedDate(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

   wchar_t wcp_time[DFXP_REGISTRY_BUFFER_LENGTH];
	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
	time_t secs;

	if (cast_handle->vendor_code == 0)
      return(OKAY);

	time(&secs);
	
	swprintf(wcp_time, L"%ld", (long)secs);
	
	swprintf(wcp_full_key_path, L"%s\\%s\\%d\\%d\\%s", 
									DFXP_REGISTRY_TOP_WIDE, 
									cast_handle->wcp_product_name, 
									cast_handle->major_version,
									cast_handle->vendor_code,
		                     DFXP_REGISTRY_DATE_LASTUSED_WIDE);
	
	/* NOTE: THIS CAN FAIL IF WE ARE IN A PROCESS SUCH AS INTERNET EXPLORER. */
	if (regCreateKey_Wide(REG_CURRENT_USER, wcp_full_key_path, wcp_time) != OKAY)
		return(OKAY);

	return(OKAY);
}


/*
 * FUNCTION: dfxp_RegistryGetTopSharedFolderPath() 
 * DESCRIPTION:
 *   
 *  Retrieves from the registry the top shared folder path
 *  (C:\\Program Files\\Common Files\\DFX)
 */
int dfxp_RegistryGetTopSharedFolderPath(PT_HANDLE *hp_dfxp, 
													 wchar_t *wcp_top_shared_folder_path)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
	int key_exists;

	if (wcp_top_shared_folder_path == NULL)
		return(NOT_OKAY);

	swprintf(wcp_top_shared_folder_path, L"");

	swprintf(wcp_full_key_path, L"%s\\%s\\%s\\%s", DFXP_REGISTRY_TOP_WIDE, 
			     cast_handle->wcp_product_name, 
				  DFXP_REGISTRY_SHARED_WIDE,
		        DFXP_REGISTRY_TOP_SHARED_FOLDER_WIDE);

	if (regReadKey_Wide(REG_LOCAL_MACHINE, wcp_full_key_path, &key_exists, wcp_top_shared_folder_path,
				(unsigned long)DFXP_REGISTRY_BUFFER_LENGTH) != OKAY)
	         return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_RegistryGetTopVendorSpecificFolderPath() 
 * DESCRIPTION:
 *   
 *  Retrieves from the registry the top vendor specific folder path
 *  (C:\\Program Files\\DFX\\Universal)
 */
int dfxp_RegistryGetTopVendorSpecificFolderPath(PT_HANDLE *hp_dfxp, 
													         wchar_t *wcp_top_vendor_specific_folder_path)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
	int key_exists;

	if (wcp_top_vendor_specific_folder_path == NULL)
		return(NOT_OKAY);

	swprintf(wcp_top_vendor_specific_folder_path, L"");

	swprintf(wcp_full_key_path, L"%s\\%s\\%d\\%s", DFXP_REGISTRY_TOP_WIDE, 
			     cast_handle->wcp_product_name, 
				  cast_handle->vendor_code,
		        DFXP_REGISTRY_TOP_FOLDER_WIDE);

	if (regReadKey_Wide(REG_LOCAL_MACHINE, wcp_full_key_path, &key_exists, wcp_top_vendor_specific_folder_path,
				(unsigned long)DFXP_REGISTRY_BUFFER_LENGTH) != OKAY)
	         return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxp_RegistryGetDfxUniversalUiFullpath() 
 * DESCRIPTION:
 *   
 *  Retrieves from the fullpath to the universal dfx ui exe (dfx.exe)
 */
int dfxp_RegistryGetDfxUniversalUiFullpath(PT_HANDLE *hp_dfxp, wchar_t *wcp_dfx_ui_path)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
	int key_exists;

	if (wcp_dfx_ui_path == NULL)
		return(NOT_OKAY);

	swprintf(wcp_dfx_ui_path, L"");

	swprintf(wcp_full_key_path, L"%s\\%s\\%d\\%s", DFXP_REGISTRY_TOP_WIDE, 
			     cast_handle->wcp_product_name, 
				  cast_handle->vendor_code,
		        DFXP_REGISTRY_DFX_UNIVERSAL_UI_PATH_WIDE);

	if (regReadKey_Wide(REG_LOCAL_MACHINE, wcp_full_key_path, &key_exists, wcp_dfx_ui_path,
				(unsigned long)DFXP_REGISTRY_BUFFER_LENGTH) != OKAY)
	         return(NOT_OKAY);

	return(OKAY);
}
