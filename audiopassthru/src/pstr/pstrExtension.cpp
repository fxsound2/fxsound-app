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
#include <windows.h>
#include <winbase.h>
#include <dos.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "slout.h"
#include "pstr.h"
#include "pt_defs.h"

/*
 * FUNCTION: pstrCalcExtension()
 * DESCRIPTION:
 *
 *  Passes back the extension of the passed string (all the characters after the
 *  last '.').  For example, if passed "test.txt" it passes back "txt".
 *
 */
int PT_DECLSPEC pstrCalcExtension(char *cp_string, char *cp_extension, int i_extension_buffer_size)
{
	wchar_t wcp_string[PT_MAX_PATH_STRLEN];
	wchar_t wcp_extension[PT_MAX_GENERIC_STRLEN];

	if (cp_string == NULL)
		return(NOT_OKAY);
	if (cp_extension == NULL)
		return(NOT_OKAY);

	if (pstrConvertToWideCharString(cp_string, wcp_string, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);

	if (pstrCalcExtension_Wide(wcp_string, wcp_extension, PT_MAX_GENERIC_STRLEN) != OKAY)
		return(NOT_OKAY);

	if (pstrConvertWideCharStringToAnsiCharString(wcp_extension, 
																 cp_extension,
														       i_extension_buffer_size) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: pstrCalcExtension_Wide()
 * DESCRIPTION:
 *
 *  Passes back the extension of the passed string (all the characters after the
 *  last '.').  For example, if passed "test.txt" it passes back "txt".
 *
 */
int PT_DECLSPEC pstrCalcExtension_Wide(wchar_t *wcp_string, wchar_t *wcp_extension, int i_extension_buffer_size)
{
	if (wcp_string == NULL)
		return(NOT_OKAY);
	if (wcp_extension == NULL)
		return(NOT_OKAY);

	int length;
	int orig_index;
	int ext_index;
	int location_of_last_period;
   int found;

	length = (int)wcslen(wcp_string);

	if (length <= 0)
      return(NOT_OKAY);

	found = IS_FALSE;
   orig_index = length - 1;

	while ((orig_index >= 0) && (!found))
	{
		if (wcp_string[orig_index] == L'.')
		{
			location_of_last_period = orig_index;
			found = IS_TRUE;
		}
		else
			orig_index--;
	}

	// 6/18/2013: It used to return NOT_OKAY but it really shouldn't. It is possible to have a file name without extension.
	if (!found)
		return(OKAY);

	/* Copy the extension */
	ext_index = 0;
	for (orig_index = location_of_last_period + 1; orig_index <= length; orig_index++)
	{
		wcp_extension[ext_index] = wcp_string[orig_index];
		ext_index++;
	}

   return(OKAY);
}

/*
 * FUNCTION: pstrRemoveExtension()
 * DESCRIPTION:
 *
 *  Removes the extension from the passed string (all the characters after the
 *  last '.' - including the last '.').  For example, if passed "test.txt" it passes back "test".
 *
 */
int PT_DECLSPEC pstrRemoveExtension(char *cp_string)
{
	wchar_t *wcp_string;
	int i_wide_char_buffer_size;

	if (pstrConvertToWideCharString_WithAlloc(cp_string, &wcp_string, &i_wide_char_buffer_size) != OKAY)
		return(NOT_OKAY);

	if (pstrRemoveExtension_Wide(wcp_string) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}

	if (pstrConvertWideCharStringToAnsiCharString(wcp_string, cp_string, i_wide_char_buffer_size) != OKAY)
		return(NOT_OKAY);

	free(wcp_string);

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveExtension_Wide()
 * DESCRIPTION:
 *
 *  Removes the extension from the passed string (all the characters after the
 *  last '.' - including the last '.').  For example, if passed "test.txt" it passes back "test".
 *
 */
int PT_DECLSPEC pstrRemoveExtension_Wide(wchar_t *wcp_string)
{
	wchar_t wcp_extension[PT_MAX_GENERIC_STRLEN];
	int strlen_extension;
	int strlen_original;
	int location_of_last_period;

	/* If there is no extension, then there is nothing left to do. */
   if (pstrCalcExtension_Wide(wcp_string, wcp_extension, PT_MAX_GENERIC_STRLEN) != OKAY)
		return(OKAY);
	
	strlen_extension = (int)wcslen(wcp_extension);
	strlen_original = (int)wcslen(wcp_string);

	/* Calculate the location of the last '.' */
   location_of_last_period = strlen_original - strlen_extension - 1;

	if (location_of_last_period < 0)
		return(NOT_OKAY);

	wcp_string[location_of_last_period] = L'\0';

	return(OKAY);
}

/*
 * FUNCTION: pstrForceExtension()
 * DESCRIPTION:
 *
 *  Forces the extension of the passed string to be the passed expension.  For example if
 *  cp_string is "test.xyz" and cp_extension is "txt" this function changes the string to be 
 *  "test.txt"
 */
int PT_DECLSPEC pstrForceExtension(char *cp_string, char *cp_extension)
{
	if (cp_string == NULL)
		return(NOT_OKAY);
	if (cp_extension == NULL)
		return(NOT_OKAY);

	/* First remove whatever extension there currently is on the string */
	if (pstrRemoveExtension(cp_string) != OKAY)
		return(NOT_OKAY);

	/* Append the extension */
	strcat(cp_string, ".");
	strcat(cp_string, cp_extension);

	return(OKAY);
}

/*
 * FUNCTION: pstrForceExtension_Wide()
 * DESCRIPTION:
 *
 *  Forces the extension of the passed string to be the passed expension.  For example if
 *  cp_string is "test.xyz" and cp_extension is "txt" this function changes the string to be 
 *  "test.txt"
 */
int PT_DECLSPEC pstrForceExtension_Wide(wchar_t *wcp_string, wchar_t *wcp_extension)
{
	if (wcp_string == NULL)
		return(NOT_OKAY);
	if (wcp_extension == NULL)
		return(NOT_OKAY);

	/* First remove whatever extension there currently is on the string */
	if (pstrRemoveExtension_Wide(wcp_string) != OKAY)
		return(NOT_OKAY);

	/* Append the extension */
	wcscat(wcp_string, L".");
	wcscat(wcp_string, wcp_extension);

	return(OKAY);
}