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

#include "u_reg.h"

/*
 * FUNCTION: regRecursiveDeleteFolder_Wide()
 * DESCRIPTION:
 *
 *  Recursively deletes the specified registry folder.
 *
 *  NOTE: If the passed folder does not exist, this function does nothing but still returns OKAY.
 */
int PT_DECLSPEC regRecursiveDeleteFolder_Wide(int i_key_class, wchar_t *wcp_reg_folder_top)
{
	HKEY topkey;

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

	if (wcp_reg_folder_top == NULL)
		return(NOT_OKAY);

	if (reg_DeleteKeyNT_Wide(topkey, wcp_reg_folder_top) == ERROR_BADKEY)
		return(OKAY);

	return(OKAY);
}

/*
 * FUNCTION: reg_DeleteKeyNT_Wide()
 * DESCRIPTION:
 *
 *  Recursively deletes the passed registry folder.
 *  
 *  Example: 
 *     hStartKey = HKEY_USERS (HKEY)
 *     pKeyName = "Software\DFX\11"
 *
 *  NOTE: THIS CODE CAME FROM MISCROSOFT MSDN AND HAS BEEN LEFT AS IS.
 *
 */
DWORD reg_DeleteKeyNT_Wide(HKEY hStartKey, LPWSTR pKeyName)
{
	DWORD   dwRtn, dwSubKeyLength;
   LPWSTR  pSubKey = NULL;
   WCHAR   szSubKey[REG_MAX_KEY_LENGTH]; 
   HKEY    hKey;

   // Do not allow NULL or empty key name
   if (pKeyName &&  lstrlenW(pKeyName))
   {
		if ((dwRtn = RegOpenKeyExW(hStartKey,pKeyName,
                               0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey )) == ERROR_SUCCESS)
      {
			while (dwRtn == ERROR_SUCCESS )
         {
				dwSubKeyLength = REG_MAX_KEY_LENGTH;
            dwRtn=RegEnumKeyExW(
                              hKey,
                              0,       // always index zero
                              szSubKey,
                              &dwSubKeyLength,
                              NULL,
                              NULL,
                              NULL,
                              NULL
                            );

            if (dwRtn == ERROR_NO_MORE_ITEMS)
            {
					dwRtn = RegDeleteKeyW(hStartKey, pKeyName);
               break;
            }
            else if (dwRtn == ERROR_SUCCESS)
					dwRtn = reg_DeleteKeyNT_Wide(hKey, szSubKey);
         }

         RegCloseKey(hKey);
         // Do not save return code because error
         // has already occurred
      }
	}
   else
      dwRtn = ERROR_BADKEY;

	return dwRtn;
}

