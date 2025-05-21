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

#include <stdio.h>
#include "wtypes.h"

#include "slout.h"
#include "pt_defs.h"
#include "reg.h"
#include "pstr.h"
#include "operatingSystem.h"

#define DEF_GLOBAL
#include "u_reg.h"

/*
 * FUNCTION: regReadTopDir_Wide()
 * DESCRIPTION:
 *
 *  Read the top level directory of DFX or DSPFX from the registry
 *  and pass it back.  Stuffs it in the passed buffer.
 */
int PT_DECLSPEC regReadTopDir_Wide(wchar_t *wcp_top_dir, 
											  int i_length, int i_product_type,
											  int i_demo_version, CSlout *hp_slout)
{
   HKEY hKeyDspfx;
   DWORD dwBufferSize;

	if (!i_demo_version)
	{
      if (i_product_type == PT_PRODUCT_DFX)
		{
         if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
								  DFX_RETAIL_REGISTRY_TOP_WIDE,
		                    0, KEY_READ, &hKeyDspfx) != ERROR_SUCCESS)
            return(NOT_OKAY);
		}
	   else
		{
         if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, DSPFX_RETAIL_REGISTRY_TOP_WIDE,
		                    0, KEY_READ, &hKeyDspfx) != ERROR_SUCCESS)
            return(NOT_OKAY);
		}
	}
   else /* Demo version */
	{
		if (i_product_type == PT_PRODUCT_DFX)
		{
         if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, DFX_DEMO_REGISTRY_TOP_WIDE,
		                    0, KEY_READ, &hKeyDspfx) != ERROR_SUCCESS)
            return(NOT_OKAY);
		}
	   else
		{
         if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, DSPFX_DEMO_REGISTRY_TOP_WIDE,
		                    0, KEY_READ, &hKeyDspfx) != ERROR_SUCCESS)
            return(NOT_OKAY);
		}
	}

   dwBufferSize = i_length;

   if (RegQueryValueExW(hKeyDspfx, L"TOP_DIR",
	      NULL, NULL,  (LPBYTE)wcp_top_dir, &dwBufferSize) != ERROR_SUCCESS)
      return(NOT_OKAY);

   RegCloseKey(hKeyDspfx);

   return(OKAY);
}

/*
 * FUNCTION: regReadRegisteredOwner()
 * DESCRIPTION:
 *
 *  Ansi version of regReadRegisteredOwner_Wide()
 */
int PT_DECLSPEC regReadRegisteredOwner(char *cp_reg_owner, int i_length)
{
	wchar_t wcp_reg_owner[PT_MAX_GENERIC_STRLEN];

	if (regReadRegisteredOwner_Wide(wcp_reg_owner, PT_MAX_GENERIC_STRLEN) != OKAY)
		return(NOT_OKAY);

	/* Create ansi version of owner */
	if (pstrConvertWideCharStringToAnsiCharString(wcp_reg_owner, cp_reg_owner, i_length) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: regReadRegisteredOwner_Wide()
 * DESCRIPTION:
 *
 *  Read the "Registered Owner" for this machine.
 *  and pass it back.  Stuffs it in the passed buffer.
 */
int PT_DECLSPEC regReadRegisteredOwner_Wide(wchar_t *wcp_reg_owner, int i_length)
{
	HKEY hKeyRegOwner;
	DWORD dwBufferSize;
	wchar_t wcp_current_version_path[PT_MAX_PATH_STRLEN];
	int is_wow64;
	REGSAM samDesired;

	swprintf(wcp_reg_owner, L"%s", REG_REGISTERED_OWNER_DEFAULT_VALUE_WIDE);

	swprintf(wcp_current_version_path, L"%s", REG_WIN_NT_CURRENT_VERSION_PATH_WIDE);

	/* 
	 * Check if this is a WOW64 process (32bit app running on a 64bit machine).  If so, we
	 * want to read from the 64bit registry for consistancy).
	 */
	if (operatingSystemIsWow64(&is_wow64) != OKAY)
		return(NOT_OKAY);

	samDesired = KEY_READ;
	if (is_wow64)
		 samDesired = samDesired | KEY_WOW64_64KEY;

	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, wcp_current_version_path,
			             0, samDesired, &hKeyRegOwner) != ERROR_SUCCESS)
		return(NOT_OKAY);

	dwBufferSize = i_length;

	/* On some Windows 10 32-bit versions, there is no RegisterdOwner key */
	if (RegQueryValueExW(hKeyRegOwner, REG_REGISTERED_OWNER_KEYNAME_WIDE,
	      NULL, NULL,  (LPBYTE)wcp_reg_owner, &dwBufferSize) != ERROR_SUCCESS)
	{
		swprintf(wcp_reg_owner, L"UnknownOwner");
      return(OKAY);
	}

	RegCloseKey(hKeyRegOwner);

	return(OKAY);
}

/*
 * FUNCTION: regRemoveKey()
 * DESCRIPTION:
 *
 *  Ansi version of regRemoveKey_Wide() 
 */
int PT_DECLSPEC regRemoveKey(int i_key_class, char *cp_keyname)
{
	wchar_t wcp_keyname[PT_MAX_GENERIC_STRLEN];

	/* Create wide version of params */
	if (pstrConvertToWideCharString(cp_keyname, wcp_keyname, PT_MAX_GENERIC_STRLEN) != OKAY)
		return(NOT_OKAY);

	if (regRemoveKey_Wide(i_key_class, wcp_keyname) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: regRemoveKey_Wide()
 * DESCRIPTION:
 *
 *  Deletes the passed registry key.  
 */
int PT_DECLSPEC regRemoveKey_Wide(int i_key_class, wchar_t *wcp_keyname)
{
   HKEY topkey;

   /* Determine topkey */
   if (i_key_class == REG_CLASSES_ROOT)
       topkey = HKEY_CLASSES_ROOT;
  else if (i_key_class == REG_LOCAL_MACHINE)
       topkey = HKEY_LOCAL_MACHINE;
  else if (i_key_class == REG_CURRENT_USER)
       topkey = HKEY_CURRENT_USER;
  else if (i_key_class == REG_USERS)
       topkey = HKEY_USERS;
   else
       return(NOT_OKAY);

   /*
    * NOTE: ACCORDING TO WIN32 API SUPERBIBLE, THIS MAY NOT WORK ON Windows NT.
    */
   if (RegDeleteKeyW(topkey, wcp_keyname) != ERROR_SUCCESS)
     return(NOT_OKAY);

   return(OKAY);
} 

/*
 * FUNCTION: regCreateKey()
 * DESCRIPTION:
 *
 *  Ansi version of regCreateKey_Wide()
 */
int PT_DECLSPEC regCreateKey(int i_key_class, char *cp_keyname, char *cp_data)
{
	wchar_t wcp_keyname[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_data[PT_MAX_GENERIC_STRLEN];

	/* Create wide version of params */
	if (pstrConvertToWideCharString(cp_keyname, wcp_keyname, PT_MAX_GENERIC_STRLEN) != OKAY)
		return(NOT_OKAY);
	if (pstrConvertToWideCharString(cp_data, wcp_data, PT_MAX_GENERIC_STRLEN) != OKAY)
		return(NOT_OKAY);

	if (regCreateKey_Wide(i_key_class, wcp_keyname, wcp_data) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: regCreateKey_Wide()
 * DESCRIPTION:
 *
 *  Creates a registry key at the specified location with
 *  the "default" value set to the passed in string value,
 */
int PT_DECLSPEC regCreateKey_Wide(int i_key_class, wchar_t *wcp_keyname, wchar_t *wcp_data)
{
	HKEY topkey;
	HKEY subkey;
	unsigned long key_disposition;
	int i_num_bytes;

	if (wcp_keyname == NULL)
		return(NOT_OKAY);
	if (wcp_data == NULL)
	   return(NOT_OKAY);

	/* Note we need the string length including the terminating null char */
	i_num_bytes = (int)(wcslen(wcp_data) + 1) * sizeof(wchar_t);

	/* Make key */
	if (i_key_class == REG_CLASSES_ROOT)
		topkey = HKEY_CLASSES_ROOT;
   else if (i_key_class == REG_LOCAL_MACHINE)
		topkey = HKEY_LOCAL_MACHINE;
   else if (i_key_class == REG_CURRENT_USER)
		topkey = HKEY_CURRENT_USER;
   else if (i_key_class == REG_USERS)
		topkey = HKEY_USERS;
	else
		return(NOT_OKAY);

	if (RegCreateKeyExW(topkey, wcp_keyname, 0, L"", REG_OPTION_NON_VOLATILE,
		                KEY_ALL_ACCESS, NULL, &subkey, &key_disposition) != ERROR_SUCCESS)
		return(NOT_OKAY);

	/* Set value */
	if( RegSetValueExW(subkey, L"", 0, REG_SZ, (BYTE *)wcp_data, i_num_bytes) != ERROR_SUCCESS)
		return(NOT_OKAY);

	if( RegCloseKey(subkey) != ERROR_SUCCESS )
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: regReadKey_Wide()
 * DESCRIPTION:
 *
 *  Ansi version of regReadKey_Wide()
 */
int PT_DECLSPEC regReadKey(int i_key_class, char *cp_keyname, 
									int *ip_key_exists_flag, char *cp_data,
									unsigned long ul_buffer_size)
{
	wchar_t wcp_keyname[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_data[PT_MAX_GENERIC_STRLEN];

	/* Create wide version of keyname */
	if (pstrConvertToWideCharString(cp_keyname, wcp_keyname, PT_MAX_GENERIC_STRLEN) != OKAY)
		return(NOT_OKAY);

	if (regReadKey_Wide(i_key_class, wcp_keyname, ip_key_exists_flag, 
							  wcp_data, PT_MAX_GENERIC_STRLEN) != OKAY)
		return(NOT_OKAY);

	/* Convert wide data to ansi */
	if (*ip_key_exists_flag == IS_TRUE) {
		if (pstrConvertWideCharStringToAnsiCharString(wcp_data, cp_data, (int)ul_buffer_size) != OKAY)
			return(NOT_OKAY);
	}
	
	return(OKAY);
}

/*
 * FUNCTION: regReadKey_Wide()
 * DESCRIPTION:
 *
 *  If the passed in key location exists, the function reads it and
 *  sets the exists flag to one. If the key doesn't exist or is not
 *  accessible, the exists flag is set to zero. 
 *
 *  The passed passed i_key_class can be REG_CLASSES_ROOT, REG_LOCAL_MACHINE, etc.
 *
 *  The passed l_buffer_size must be the size that the cp_data was allocated at.
 */
int PT_DECLSPEC regReadKey_Wide(int i_key_class, wchar_t *wcp_keyname, 
									int *ip_key_exists_flag, wchar_t *wcp_data,
									unsigned long ul_buffer_size)
{
	HKEY hkey;
	int error_code;

	*ip_key_exists_flag = IS_FALSE;

	if (wcp_keyname == NULL)
		return(NOT_OKAY);

	/* Try to open key. If key doesn't exist, function will return non-zero value */
   if (i_key_class == REG_CLASSES_ROOT)
	   error_code = RegOpenKeyExW(HKEY_CLASSES_ROOT, wcp_keyname, 0, KEY_QUERY_VALUE, &hkey);
   else if (i_key_class == REG_LOCAL_MACHINE)
      error_code = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wcp_keyname, 0, KEY_QUERY_VALUE, &hkey);
   else if (i_key_class == REG_USERS)
      error_code = RegOpenKeyExW(HKEY_USERS, wcp_keyname, 0, KEY_QUERY_VALUE, &hkey);  
   else if (i_key_class == REG_CURRENT_USER)
      error_code = RegOpenKeyExW(HKEY_CURRENT_USER, wcp_keyname, 0, KEY_QUERY_VALUE, &hkey); 
	else
		return(NOT_OKAY);

	/* If we can't open the key, set the flag to doesn't exist and return */
	if (error_code != ERROR_SUCCESS)
		return(OKAY);

	/* 
	 * "" recovers "default value" 
	 * If it fails, set the flag to doesn't exist and return.
	 */
	if (RegQueryValueExW(hkey, L"", 0, NULL, (unsigned char *)wcp_data, 
		                 &ul_buffer_size) != ERROR_SUCCESS)
	{
		return(OKAY);
	}

	if (RegCloseKey(hkey) != ERROR_SUCCESS)
		return(NOT_OKAY);

	*ip_key_exists_flag = IS_TRUE;

	return(OKAY);
}

/*
* FUNCTION: regCreateKeyTest_Wide()
* DESCRIPTION:
*
*  Test if the machine can successfully Creates a registry key at the specified location with
*  the "default" value set to the passed in string value.
*
*  NOTE: If the write fails, it does not return NOT_OKAY.  Instead it passes back the success flag.
*/
int PT_DECLSPEC regCreateKeyTest_Wide(int i_key_class, wchar_t *wcp_keyname, wchar_t *wcp_data, int *ip_success_flag)
{
	HKEY topkey;
	HKEY subkey;
	unsigned long key_disposition;
	int i_num_bytes;

	if (wcp_keyname == NULL)
		return(NOT_OKAY);
	if (wcp_data == NULL)
		return(NOT_OKAY);

	*ip_success_flag = IS_FALSE;

	/* Note we need the string length including the terminating null char */
	i_num_bytes = (int)(wcslen(wcp_data) + 1) * sizeof(wchar_t);

	/* Make key */
	if (i_key_class == REG_CLASSES_ROOT)
		topkey = HKEY_CLASSES_ROOT;
	else if (i_key_class == REG_LOCAL_MACHINE)
		topkey = HKEY_LOCAL_MACHINE;
	else if (i_key_class == REG_CURRENT_USER)
		topkey = HKEY_CURRENT_USER;
	else if (i_key_class == REG_USERS)
		topkey = HKEY_USERS;
	else
		return(NOT_OKAY);

	if (RegCreateKeyExW(topkey, wcp_keyname, 0, L"", REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &subkey, &key_disposition) != ERROR_SUCCESS)
		return(OKAY);

	/* Set value */
	if (RegSetValueExW(subkey, L"", 0, REG_SZ, (BYTE *)wcp_data, i_num_bytes) != ERROR_SUCCESS)
		return(OKAY);

	if (RegCloseKey(subkey) != ERROR_SUCCESS)
		return(OKAY);

	*ip_success_flag = IS_TRUE;

	return(OKAY);
}