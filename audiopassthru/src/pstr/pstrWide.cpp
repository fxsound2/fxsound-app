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

/* Standard includes */
#include <windows.h>
#include <winbase.h>
#include <dos.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "codedefs.h"

#include "slout.h"
#include "pstr.h"

/*
 * FUNCTION: pstrConvertToWideCharString()
 * DESCRIPTION:
 *
 *  Converts the passed original char * to the equivelent wide character string.  The wide
 *  character string must already be allocated.
 *
 */
int PT_DECLSPEC pstrConvertToWideCharString(char *cp_original_string, 
														  wchar_t *wcp_converted_string,
														  int i_wide_char_buffer_size)
{
	if (cp_original_string == NULL)
		return(NOT_OKAY);

	if (wcp_converted_string == NULL)
		return(NOT_OKAY);

	if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cp_original_string, -1, 
		                     wcp_converted_string, i_wide_char_buffer_size) == 0)
      return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: pstrConvertToWideCharString_WithAlloc()
 * DESCRIPTION:
 *
 *  Converts the passed original char * to the equivelent wide character string.  The wide
 *  function allocates the new wide character string.
 *
 */
int PT_DECLSPEC pstrConvertToWideCharString_WithAlloc(char *cp_original_string, 
														  wchar_t **wcpp_converted_string,
														  int *ip_wide_char_buffer_size)
{
	if (cp_original_string == NULL)
		return(NOT_OKAY);

	if (wcpp_converted_string == NULL)
		return(NOT_OKAY);

	/* Determine the necessary number of wide characters to allocate */
	*ip_wide_char_buffer_size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cp_original_string, -1, 
																	NULL, 0);

	/* Allocate the wide string to be proper size */
   *wcpp_converted_string = (wchar_t *)calloc(*ip_wide_char_buffer_size, sizeof(wchar_t));
	if (*wcpp_converted_string == NULL)
		return(NOT_OKAY);

	/* Convert the original string to wide characters */
	if (pstrConvertToWideCharString(cp_original_string, 
											  *wcpp_converted_string,
												*ip_wide_char_buffer_size) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: pstrConvertWideCharStringToAnsiCharString()
 * DESCRIPTION:
 *
 *  Converts the passed original wchar_t * to the equivelent ANSI character string.  The ANSI
 *  character string must already be allocated.
 *
 */
int PT_DECLSPEC pstrConvertWideCharStringToAnsiCharString(wchar_t *wcp_original_string, 
														  char *cp_converted_string,
														  int i_ansi_char_buffer_size)
{
	if (wcp_original_string == NULL)
		return(NOT_OKAY);

	if (cp_converted_string == NULL)
		return(NOT_OKAY);
   
	if(WideCharToMultiByte(CP_ACP, 0, wcp_original_string, -1, cp_converted_string, 
	                       i_ansi_char_buffer_size, NULL, NULL) == 0)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: pstrCovertWideCharStringToUTF8String_WithAlloc()
 * DESCRIPTION:
 *   Convert a wide character string (USC-2) to UTF-8. 
 *   It allocates the output char* array and return the number of bytes allocated (including the null terminator).
 *   The caller needs to free the UTF-8 string.
 *   Sample use:
 *   char * cp_utf8 = NULL;
 *	  int num_bytes;
 *	  pstrCovertWideCharStringToUTF8String_WithAlloc(L"C:\\Users\\洋\\Music\\desktop.ini洋abc洋and地図", &cp_utf8, &num_bytes);
 *	  FILE * f;
 *	  f = fileOpen_Wide(L"c:\\tmp\\utf8.txt", L"w", NULL);
 *	  fprintf(f, "%s", cp_utf8); 
 *   OR
 *	  fwrite(cp_utf8, 1, num_bytes - 1, f); // -1 to not output the NULL character
 *	  fclose(f);
 */
int PT_DECLSPEC pstrCovertWideCharStringToUTF8String_WithAlloc(wchar_t *wcp_original_string, char **cpp_converted_utf8_string, int *ip_number_of_bytes)
{
	*cpp_converted_utf8_string = NULL;
	*ip_number_of_bytes = 0;

	*ip_number_of_bytes = WideCharToMultiByte(CP_UTF8, 0, wcp_original_string, -1, 0, 0, 0, 0);

	if (*ip_number_of_bytes == 0)
		return(NOT_OKAY);

	// Allocate cp_converted_utf8_string with the needed size
	*cpp_converted_utf8_string = (char *)calloc(*ip_number_of_bytes, sizeof(char));
	if (*cpp_converted_utf8_string == NULL)
		return(NOT_OKAY);
 
	*ip_number_of_bytes = WideCharToMultiByte(CP_UTF8, 0, wcp_original_string, -1, *cpp_converted_utf8_string, *ip_number_of_bytes, 0, 0);

	if (*ip_number_of_bytes == 0)
	{
		free(*cpp_converted_utf8_string);
		*cpp_converted_utf8_string = NULL;
		return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrCovertUTF8StringToWideCharString_WithAlloc()
 * DESCRIPTION:
 *   Convert a UTF-8 to wide character string (USC-2).
 *   It allocates the output wchar_t* array and return the number of wchar_t allocated (including the null terminator).
 *   The caller needs to free the resutled wchar_t* string.
 */
int PT_DECLSPEC pstrCovertUTF8StringToWideCharString_WithAlloc(char *cp_original_utf8_string, wchar_t **wcpp_converted_string, int *ip_number_of_wchar)
{
	*wcpp_converted_string = NULL;
	*ip_number_of_wchar = 0;

	*ip_number_of_wchar = MultiByteToWideChar(CP_UTF8, 0, cp_original_utf8_string, -1, NULL, 0);

	if (*ip_number_of_wchar == 0)
		return(NOT_OKAY);

	// Allocate cp_converted_utf8_string with the needed size
	*wcpp_converted_string = (wchar_t *)calloc(*ip_number_of_wchar, sizeof(wchar_t));
	if (*wcpp_converted_string == NULL)
		return(NOT_OKAY);
 
	*ip_number_of_wchar = MultiByteToWideChar(CP_UTF8, 0, cp_original_utf8_string, -1, *wcpp_converted_string, *ip_number_of_wchar);

	if (*ip_number_of_wchar == 0)
	{
		free(*wcpp_converted_string);
		*wcpp_converted_string = NULL;
		return(NOT_OKAY);
	}

	return(OKAY);
}