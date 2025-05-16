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

#include <windows.h>
#include "slout.h"
#include "file.h"
#include "u_file.h"
#include "pt_defs.h"
#include "pstr.h"
/*
 * FUNCTION: fileWin32CreateFile()
 * DESCRIPTION:
 *
 * This function is an outer layer around the Win32 CreateFile() function.  It also takes a slout
 * handle so it can issue error messages.
 */
HANDLE PT_DECLSPEC fileWin32CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
												  LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
												  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, CSlout *hp_slout)
{
	HANDLE fileh;
	wchar_t wcp_msg1[512];
	
	fileh = CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
						dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if( fileh == INVALID_HANDLE_VALUE )
	{
		if (hp_slout != NULL)
		{
 			swprintf(wcp_msg1, L"Unable to create file \"%s\"", lpFileName);
			hp_slout->Error_Wide(FIRST_LINE, wcp_msg1);
		}

		return(fileh);
	}

	return(fileh);
}

/*
 * FUNCTION: fileWin32CreateFile_Wide()
 * DESCRIPTION:
 *
 * This function is an outer layer around the Win32 CreateFile() function.  It also takes a slout
 * handle so it can issue error messages.
 */
HANDLE PT_DECLSPEC fileWin32CreateFile_Wide(LPCWSTR lpwFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
												  LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
												  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, CSlout *hp_slout)
{
	HANDLE fileh;
	char cp_msg1[512];
	char lpFileName[PT_MAX_GENERIC_STRLEN];
	
	fileh = CreateFileW(lpwFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
						dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if( fileh == INVALID_HANDLE_VALUE )
	{
		if (hp_slout != NULL)
		{
			if(pstrConvertWideCharStringToAnsiCharString((wchar_t *)lpwFileName, lpFileName, PT_MAX_GENERIC_STRLEN) != OKAY)
				return(fileh);

 			sprintf(cp_msg1, "Unable to create file \"%s\"", lpFileName);
			hp_slout->Error(FIRST_LINE, cp_msg1);
		}

		return(fileh);
	}

	return(fileh);
}