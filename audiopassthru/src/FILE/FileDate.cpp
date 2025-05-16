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
 * FUNCTION: fileSetBackCreateTime()
 * DESCRIPTION:
 *
 * Fill in the passed cp_fullpath string with the path to the DSP
 * path, based on the passed parameters.  It passes back both the 
 * path for the hardware DSP, and the software DSP.
 */
int PT_DECLSPEC fileSetBackCreateTime(char *cp_fullpath, long l_time_back_in_secs, 
						  CSlout *hp_slout)
{
	wchar_t wcp_fullpath[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_fullpath, wcp_fullpath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);

	return(fileSetBackCreateTime_Wide(wcp_fullpath, l_time_back_in_secs, hp_slout));
}

/*
 * FUNCTION: fileSetBackCreateTime_Wide()
 * DESCRIPTION:
 *
 * Fill in the passed cp_fullpath string with the path to the DSP
 * path, based on the passed parameters.  It passes back both the 
 * path for the hardware DSP, and the software DSP.
 */
int PT_DECLSPEC fileSetBackCreateTime_Wide(wchar_t *wcp_fullpath, long l_time_back_in_secs, 
						  CSlout *hp_slout)
{
	HANDLE fileh;
	FILETIME create_t, access_t, mod_t;
	__int64 *newtime64;

#ifdef FILEDATE_DEBUG
	char time_str[128];
	time_t current_time, new_time;

	time(&current_time);
	new_time = current_time - l_time_back_in_secs;
	strcpy(time_str, ctime(&new_time));
#endif

	/* Note- for the SetFileTime function, need to open file using the
	 * CreateFile call, parameterized to only open file if it exists
	 * and not create it.
	 */
	fileh = fileWin32CreateFile_Wide( wcp_fullpath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
						              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, hp_slout);

	if( fileh == INVALID_HANDLE_VALUE )
		return(NOT_OKAY);

	/* Both of the next functions return zero on failure */
	if( GetFileTime(fileh, &create_t, &access_t, &mod_t) == 0 )
	{
		CloseHandle(fileh);
		return(NOT_OKAY);
	}
	
	newtime64 = (__int64 *)&create_t;

	/* These functions use a LARGE_INTEGER count of 1e7 ticks per second */
	*newtime64 -= (__int64)l_time_back_in_secs * (__int64)10000000;

	/* Both of the next functions return zero on failure */
	if( SetFileTime(fileh, (FILETIME *)newtime64, (FILETIME *)newtime64, (FILETIME *)newtime64) == 0 )
	{
		CloseHandle(fileh);
		return(NOT_OKAY);
	}

	if( CloseHandle(fileh) == 0 )
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: fileGetModifiedDate()
 * DESCRIPTION:
 * 
 * Passes back the modified date of the passed file.
 */
int PT_DECLSPEC fileGetModifiedDate(char *cp_fullpath, FILETIME *ftp_modified_date, 
						                  CSlout *hp_slout)
{
	wchar_t wcp_fullpath[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_fullpath, wcp_fullpath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);

	return(fileGetModifiedDate_Wide(wcp_fullpath, ftp_modified_date, hp_slout));
}

/*
 * FUNCTION: fileGetModifiedDate_Wide()
 * DESCRIPTION:
 * 
 * Passes back the modified date of the passed file.
 */
int PT_DECLSPEC fileGetModifiedDate_Wide(wchar_t *wcp_fullpath, FILETIME *ftp_modified_date, 
						                  CSlout *hp_slout)
{
	HANDLE fileh;
	FILETIME creation_t, access_t, modified_t;

	/* Note- for the SetFileTime function, need to open file using the
	 * CreateFile call, parameterized to only open file if it exists
	 * and not create it.
	 */
	fileh = fileWin32CreateFile_Wide( wcp_fullpath, GENERIC_READ, FILE_SHARE_READ, NULL,
						OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, hp_slout);

	if( fileh == INVALID_HANDLE_VALUE )
		return(NOT_OKAY);

	/* Both of the next functions return zero on failure */
	if( GetFileTime(fileh, (FILETIME *)&creation_t, (FILETIME *)&access_t, 
		                    (FILETIME *)&modified_t) == 0 )
	{
		CloseHandle(fileh);
		return(NOT_OKAY);
	}

	ftp_modified_date->dwHighDateTime = modified_t.dwHighDateTime;
	ftp_modified_date->dwLowDateTime = modified_t.dwLowDateTime;

	if( CloseHandle(fileh) == 0 )
		return(NOT_OKAY);

   return(OKAY);
}

/*
 * FUNCTION: fileGetCreationDate()
 * DESCRIPTION:
 * 
 * Passes back the creation date of the passed file.
 */
int PT_DECLSPEC fileGetCreationDate(char *cp_fullpath, FILETIME *ftp_modified_date, 
						                  CSlout *hp_slout)
{
	wchar_t wcp_fullpath[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_fullpath, wcp_fullpath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);

	return(fileGetCreationDate_Wide(wcp_fullpath, ftp_modified_date, hp_slout));
}

/*
 * FUNCTION: fileGetCreationDate_Wide()
 * DESCRIPTION:
 * 
 * Passes back the creation date of the passed file.
 */
int PT_DECLSPEC fileGetCreationDate_Wide(wchar_t *wcp_fullpath, FILETIME *ftp_modified_date, 
						                  CSlout *hp_slout)
{
	HANDLE fileh;
	FILETIME creation_t, access_t, modified_t;

	/* Note- for the SetFileTime function, need to open file using the
	 * CreateFile call, parameterized to only open file if it exists
	 * and not create it.
	 */
	fileh = fileWin32CreateFile_Wide( wcp_fullpath, GENERIC_READ, FILE_SHARE_READ, NULL,
						OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, hp_slout);

	if( fileh == INVALID_HANDLE_VALUE )
		return(NOT_OKAY);

	/* Both of the next functions return zero on failure */
	if( GetFileTime(fileh, (FILETIME *)&creation_t, (FILETIME *)&access_t, 
		                    (FILETIME *)&modified_t) == 0 )
	{
		CloseHandle(fileh);
		return(NOT_OKAY);
	}

	ftp_modified_date->dwHighDateTime = creation_t.dwHighDateTime;
	ftp_modified_date->dwLowDateTime = creation_t.dwLowDateTime;

	if( CloseHandle(fileh) == 0 )
		return(NOT_OKAY);

   return(OKAY);
}

/*
 * FUNCTION: fileSetModifiedDate()
 * DESCRIPTION:
 *
 * Sets the modified date of the passed file.
 */
int PT_DECLSPEC fileSetModifiedDate(char *cp_fullpath, FILETIME *ftp_modified_date, 
						CSlout *hp_slout)
{
	wchar_t wcp_fullpath[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_fullpath, wcp_fullpath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);

	return(fileSetModifiedDate_Wide(wcp_fullpath, ftp_modified_date, hp_slout));
}

/*
 * FUNCTION: fileSetModifiedDate_Wide()
 * DESCRIPTION:
 *
 * Sets the modified date of the passed file.
 */
int PT_DECLSPEC fileSetModifiedDate_Wide(wchar_t *wcp_fullpath, FILETIME *ftp_modified_date, 
						CSlout *hp_slout)
{
	HANDLE fileh;

	/* Note- for the SetFileTime function, need to open file using the
	 * CreateFile call, parameterized to only open file if it exists
	 * and not create it.
	 */
	fileh = fileWin32CreateFile_Wide( wcp_fullpath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
						OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, hp_slout);

	if( fileh == INVALID_HANDLE_VALUE )
		return(NOT_OKAY);

	/* Both of the next functions return zero on failure */
	if( SetFileTime(fileh, ftp_modified_date, ftp_modified_date, ftp_modified_date) == 0 )
	{
		CloseHandle(fileh);
		return(NOT_OKAY);
	}

	if( CloseHandle(fileh) == 0 )
		return(NOT_OKAY);

   return(OKAY);
}

/*
 * FUNCTION: fileCompareDates()
 * DESCRIPTION:
 *
 * Compares the modified dates of the passed two files and passes back IS_TRUE or
 * IS_FALSE depending on if the first file is newer than the second.
 */
int PT_DECLSPEC fileCompareDates(char *cp_fullpath1, char *cp_fullpath2, 
											 int *ip_first_file_newer, CSlout *hp_slout)
{
	wchar_t wcp_fullpath1[PT_MAX_PATH_STRLEN];
	wchar_t wcp_fullpath2[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_fullpath1, wcp_fullpath1, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);
	if(pstrConvertToWideCharString(cp_fullpath2, wcp_fullpath2, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);

	return(fileCompareDates_Wide(wcp_fullpath1, wcp_fullpath2, ip_first_file_newer, hp_slout));
}

/*
 * FUNCTION: fileCompareDates_Wide()
 * DESCRIPTION:
 *
 * Compares the modified dates of the passed two files and passes back IS_TRUE or
 * IS_FALSE depending on if the first file is newer than the second.
 */
int PT_DECLSPEC fileCompareDates_Wide(wchar_t *wcp_fullpath1, wchar_t *wcp_fullpath2, 
											 int *ip_first_file_newer, CSlout *hp_slout)
{
   FILETIME modified_date1;
   FILETIME modified_date2;
	FILETIME creation_date1;
	FILETIME creation_date2;
	FILETIME newer_date1;
	FILETIME newer_date2;
	long l_result;

	if (wcp_fullpath1 == NULL)
		return(NOT_OKAY);

	if (wcp_fullpath2 == NULL)
		return(NOT_OKAY);

   if (fileGetModifiedDate_Wide(wcp_fullpath1, &modified_date1, hp_slout) != OKAY)
		return(NOT_OKAY);

   if (fileGetModifiedDate_Wide(wcp_fullpath2, &modified_date2, hp_slout) != OKAY)
		return(NOT_OKAY);

	if (fileGetCreationDate_Wide(wcp_fullpath1, &creation_date1, hp_slout) != OKAY)
		return(NOT_OKAY);

   if (fileGetCreationDate_Wide(wcp_fullpath2, &creation_date2, hp_slout) != OKAY)
		return(NOT_OKAY);

	 /* 
	 * We take whatever is latest between the creation time and the modifified time as the modified date.
	 * This is because during installation it changes the creation date but not the modified date, but we
	 * want it to be considered as new.
	 */
	l_result = CompareFileTime(&creation_date1, &modified_date1);
	
	if(l_result == -1) // creation_t is EARLIER then modified_t
	{
		newer_date1.dwHighDateTime = modified_date1.dwHighDateTime;
		newer_date1.dwLowDateTime = modified_date1.dwLowDateTime;
	}
	else
	{
		newer_date1.dwHighDateTime = creation_date1.dwHighDateTime;
		newer_date1.dwLowDateTime = creation_date1.dwLowDateTime;
	}
	
	l_result = CompareFileTime(&creation_date2, &modified_date2);
	
	if(l_result == -1) // creation_t is EARLIER then modified_t
	{
		newer_date2.dwHighDateTime = modified_date2.dwHighDateTime;
		newer_date2.dwLowDateTime = modified_date2.dwLowDateTime;
	}
	else
	{
		newer_date2.dwHighDateTime = creation_date2.dwHighDateTime;
		newer_date2.dwLowDateTime = creation_date2.dwLowDateTime;
	}

	l_result = CompareFileTime(&newer_date1, &newer_date2);

	if (l_result == -1) // newer_date1 is EARLIER then newer_date2
		*ip_first_file_newer = IS_FALSE;
	else
		*ip_first_file_newer = IS_TRUE;

   return(OKAY);
}

/*
 * FUNCTION: fileTouch()
 * DESCRIPTION:
 *
 *  Changes all the dates (modified, created, accessed) for the passed file to
 *  the current time.
 */
int PT_DECLSPEC fileTouch(char *cp_file_fullpath, CSlout *hp_slout)
{
	wchar_t wcp_file_fullpath[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_file_fullpath, wcp_file_fullpath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);

	return(fileTouch_Wide(wcp_file_fullpath, hp_slout));
}

/*
 * FUNCTION: fileTouch_Wide()
 * DESCRIPTION:
 *
 *  Changes all the dates (modified, created, accessed) for the passed file to
 *  the current time.
 */
int PT_DECLSPEC fileTouch_Wide(wchar_t *wcp_file_fullpath, CSlout *hp_slout)
{
	SYSTEMTIME st_current;
	HANDLE     hFile;
	FILETIME   ft_current;
	BOOL rc;

	if (wcp_file_fullpath == NULL)
		return(NOT_OKAY);

	GetSystemTime(&st_current);

	if (SystemTimeToFileTime(&st_current,&ft_current) == 0)
		return(NOT_OKAY);

	hFile=fileWin32CreateFile_Wide(wcp_file_fullpath,GENERIC_WRITE,FILE_SHARE_WRITE,
                    0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0, hp_slout);

	if (hFile == INVALID_HANDLE_VALUE)
		return(NOT_OKAY);

	rc = SetFileTime(hFile,&ft_current,&ft_current,&ft_current);

	if (rc == 0)
	{
		CloseHandle(hFile);
		return(NOT_OKAY);
	}

	CloseHandle(hFile);

	return(OKAY);
}

/*
 * FUNCTION: fileGetModifiedDate_StringFormatted()
 * DESCRIPTION:
 *
 *  Passes back a formatted string of the modified date of the specified file.
 *  (Ex. "3/1/2006 7:45:11 PM")
*/
int PT_DECLSPEC fileGetModifiedDateString(char *cp_fullpath, char *cp_formatted_datetime, CSlout *hp_slout)
{
	wchar_t wcp_fullpath[PT_MAX_PATH_STRLEN];
	wchar_t wcp_formatted_datetime[PT_MAX_GENERIC_STRLEN];

	if(pstrConvertToWideCharString(cp_fullpath, wcp_fullpath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);

	if (fileGetModifiedDateString_Wide(wcp_fullpath, wcp_formatted_datetime, hp_slout) != OKAY)
		return(NOT_OKAY);

	if (pstrConvertWideCharStringToAnsiCharString(wcp_formatted_datetime, 
																 cp_formatted_datetime,
																 PT_MAX_GENERIC_STRLEN) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: fileGetModifiedDateString_Wide()
 * DESCRIPTION:
 *
 *  Passes back a formatted string of the modified date of the specified file.
 *  (Ex. "3/1/2006 7:45:11 PM")
*/
int PT_DECLSPEC fileGetModifiedDateString_Wide(wchar_t *wcp_fullpath, wchar_t *wcp_formatted_datetime, CSlout *hp_slout)
{
	FILETIME modified_pft;
	SYSTEMTIME st; 
	SYSTEMTIME local_st;
	wchar_t wcp_date[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_time[PT_MAX_GENERIC_STRLEN];
	TIME_ZONE_INFORMATION tzone;

	// Get the modified date structure for the file
	if (fileGetModifiedDate_Wide(wcp_fullpath, &modified_pft, hp_slout) != OKAY)
		return(NOT_OKAY);
	
	// Convert modified FILETIME to SYSTEMTIME
	FileTimeToSystemTime(&modified_pft, &st);

	// Get current system time zone
	GetTimeZoneInformation(&tzone);

	// Translate UTC SYSTEMTIME into local SYSTEMTIME
	SystemTimeToTzSpecificLocalTime(&tzone, &st, &local_st);

	// Format it
	GetDateFormatW(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE , &local_st, NULL, wcp_date, PT_MAX_GENERIC_STRLEN);
	GetTimeFormatW(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE , &local_st, NULL, wcp_time, PT_MAX_GENERIC_STRLEN);

	swprintf(wcp_formatted_datetime, L"%s %s", wcp_date, wcp_time);

	return(OKAY);
}