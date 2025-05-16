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

int pstr_compare_case_sensitive_wide(const void *arg1, const void *arg2);
int pstr_compare_non_case_sensitive_wide(const void *arg1, const void *arg2);

/*
 * FUNCTION: pstrSortArray_Wide() 
 * DESCRIPTION:
 *
 *
 * The passed flag says whether or not the sorting should be case sensitive.
 *
 */
int PT_DECLSPEC pstrSortArray_Wide(wchar_t **wcpp_array, int i_array_size, int i_case_sensitive)
{
	if (i_case_sensitive)
	{
      qsort((void *)wcpp_array, (size_t)i_array_size, sizeof(wchar_t *), 
			    pstr_compare_case_sensitive_wide);
   }
	else
   {
      qsort((void *)wcpp_array, (size_t)i_array_size, sizeof(wchar_t *), 
			    pstr_compare_non_case_sensitive_wide);
	}

   return(OKAY);
}

/*
 * FUNCTION: pstr_compare_case_sensitive_wide() 
 * DESCRIPTION:
 *
 * Compares the passed two strings and returns the comparison.
 * This is used in the pstrSortArray function.  It does a case
 * sensitive comparision.
 */
int pstr_compare_case_sensitive_wide(const void *arg1, const void *arg2 )
{
   /* Do a non case sensitive comparison of the strings */
	return wcscmp( * ( wchar_t** ) arg1, * ( wchar_t** ) arg2 );
}

/*
 * FUNCTION: pstr_compare_non_case_sensitive_wide() 
 * DESCRIPTION:
 *
 * Compares the passed two strings and returns the comparison.
 * This is used in the pstrSortArray function.  It does a non case
 * sensitive comparision.
 */
int pstr_compare_non_case_sensitive_wide(const void *arg1, const void *arg2 )
{
   /* Do a lower case comparison of the strings */
	return _wcsicmp( * ( wchar_t** ) arg1, * ( wchar_t** ) arg2 );
}