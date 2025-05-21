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
#include <stdio.h>
#include "wtypes.h"

#include "codedefs.h"
#include "slout.h"
#include "pt_defs.h"
#include "reg.h"
#include "pstr.h"

#include "u_reg.h"

/*
 * FUNCTION: regCreateKeyWithKeyname_Dword_Wide()
 * DESCRIPTION:
 *
 *  Creates a registry key at the specified location with
 *  the keyname value set to the passed in dword value,
 */
int PT_DECLSPEC regCreateKeyWithKeyname_Dword_Wide(int i_key_class, 
																	wchar_t *wcp_keypath, 
																	wchar_t *wcp_keyname, 
																	unsigned long ul_setting)
{
	HKEY topkey;
	HKEY subkey;
	unsigned long key_disposition;

	if (wcp_keypath == NULL)
	   return(NOT_OKAY);
	if (wcp_keyname == NULL)
	   return(NOT_OKAY);

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

	if (RegCreateKeyExW(topkey, wcp_keypath, 0, L"", REG_OPTION_NON_VOLATILE,
		                KEY_ALL_ACCESS, NULL, &subkey, &key_disposition) != ERROR_SUCCESS)
		   return(NOT_OKAY);

	/* Set value */
	if(RegSetValueExW(subkey, wcp_keyname, 0, REG_DWORD, (CONST BYTE *)&ul_setting, sizeof(unsigned long)) != ERROR_SUCCESS)
		return(NOT_OKAY);

	if(RegCloseKey(subkey) != ERROR_SUCCESS )
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: regCreateKeyWithKeyname_String_Wide()
 * DESCRIPTION:
 *
 *  Creates a registry key at the specified location with
 *  the keyname value set to the passed in wide string value,
 */
int PT_DECLSPEC regCreateKeyWithKeyname_String_Wide(int i_key_class,
																	 wchar_t *wcp_keypath,
																	 wchar_t *wcp_keyname, 
													             wchar_t *wcp_data)
{
	HKEY topkey;
	HKEY subkey;
	unsigned long key_disposition;
	int i_num_bytes;

	if (wcp_keypath == NULL)
	   return(NOT_OKAY);
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

	if (RegCreateKeyExW(topkey, wcp_keypath, 0, L"", REG_OPTION_NON_VOLATILE,
		                 KEY_ALL_ACCESS, NULL, &subkey, &key_disposition) != ERROR_SUCCESS)
		return(NOT_OKAY);

	/* Set value */
	if(RegSetValueExW(subkey, wcp_keyname, 0, REG_SZ, (BYTE *)wcp_data, i_num_bytes) != ERROR_SUCCESS)
		return(NOT_OKAY);

	if(RegCloseKey(subkey) != ERROR_SUCCESS )
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: regReadKeyWithKeyname_String_Wide()
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
int PT_DECLSPEC regReadKeyWithKeyname_String_Wide(int i_key_class, 
																  wchar_t *wcp_subkey, 
																  wchar_t *wcp_keyname, 
																  int *ip_key_exists_flag, 
																  wchar_t *wcp_data, 
																  unsigned long ul_buffer_size)
{
	HKEY hkey;
	int error_code;

	*ip_key_exists_flag = IS_FALSE;

	if ((wcp_subkey == NULL) || (wcp_keyname == NULL))
		return(NOT_OKAY);

	/* Try to open key. If key doesn't exist, function will return non-zero value */
   if (i_key_class == REG_CLASSES_ROOT)
	   error_code = RegOpenKeyExW(HKEY_CLASSES_ROOT, wcp_subkey, 0, KEY_QUERY_VALUE, &hkey);
   else if (i_key_class == REG_LOCAL_MACHINE)
      error_code = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wcp_subkey, 0, KEY_QUERY_VALUE, &hkey);
   else if (i_key_class == REG_USERS)
      error_code = RegOpenKeyExW(HKEY_USERS, wcp_subkey, 0, KEY_QUERY_VALUE, &hkey);  
   else if (i_key_class == REG_CURRENT_USER)
      error_code = RegOpenKeyExW(HKEY_CURRENT_USER, wcp_subkey, 0, KEY_QUERY_VALUE, &hkey); 
	else
		return(NOT_OKAY);

	/* If we can't open the key, set the flag to doesn't exist and return */
	if (error_code != ERROR_SUCCESS)
		return(OKAY);

	if (RegQueryValueExW(hkey, wcp_keyname, 0, NULL, (unsigned char *)wcp_data, 
		                 &ul_buffer_size) != ERROR_SUCCESS)
		return(OKAY);

	if (RegCloseKey(hkey) != ERROR_SUCCESS)
		return(NOT_OKAY);

	*ip_key_exists_flag = IS_TRUE;

	return(OKAY);
}

/*
 * FUNCTION: regReadKeyWithKeyname_Dword_Wide()
 * DESCRIPTION:
 *
 *  If the passed in key location exists, the function reads it and
 *  sets the exists flag to one. If the key doesn't exist or is not
 *  accessible, the exists flag is set to zero. 
 *
 *  The passed passed i_key_class can be REG_CLASSES_ROOT, REG_LOCAL_MACHINE, etc.
 */
int PT_DECLSPEC regReadKeyWithKeyname_Dword_Wide(int i_key_class, 
																 wchar_t *wcp_subkey, 
																 wchar_t *wcp_keyname, 
																 int *ip_key_exists_flag, 
																 unsigned long *ulp_value)
{
	HKEY hkey;
	int error_code;
	DWORD dw_type;
	unsigned long ul_size;

	*ip_key_exists_flag = IS_FALSE;

	if ((wcp_subkey == NULL) || (wcp_keyname == NULL))
		return(NOT_OKAY);

	/* Try to open key. If key doesn't exist, function will return non-zero value */
   if (i_key_class == REG_CLASSES_ROOT)
	   error_code = RegOpenKeyExW(HKEY_CLASSES_ROOT, wcp_subkey, 0, KEY_QUERY_VALUE, &hkey);
   else if (i_key_class == REG_LOCAL_MACHINE)
      error_code = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wcp_subkey, 0, KEY_QUERY_VALUE, &hkey);
   else if (i_key_class == REG_USERS)
      error_code = RegOpenKeyExW(HKEY_USERS, wcp_subkey, 0, KEY_QUERY_VALUE, &hkey);  
   else if (i_key_class == REG_CURRENT_USER)
      error_code = RegOpenKeyExW(HKEY_CURRENT_USER, wcp_subkey, 0, KEY_QUERY_VALUE, &hkey); 
	else
		return(NOT_OKAY);

	/* If we can't open the key, set the flag to doesn't exist and return */
	if (error_code != ERROR_SUCCESS)
		return(OKAY);

	ul_size = sizeof(unsigned long);
	if (RegQueryValueExW(hkey, wcp_keyname, 0, &dw_type, (LPBYTE)ulp_value, &ul_size) != ERROR_SUCCESS)
		return(OKAY);

	if (RegCloseKey(hkey) != ERROR_SUCCESS)
		return(NOT_OKAY);

	*ip_key_exists_flag = IS_TRUE;

	return(OKAY);
}