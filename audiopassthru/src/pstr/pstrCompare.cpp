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

#include <string.h>
#include <stdio.h>

#include "slout.h"
#include "pstr.h"
#include "pt_defs.h"


/*
 * FUNCTION: pstrCompareStrings()
 * DESCRIPTION:
 *
 *  Compares the passed strings.  
 *  Passes back either PSTR_COMPARE_STRINGS_EQUAL, PSTR_COMPARE_FIRST_STRING_LOWER, or PSTR_COMPARE_FIRST_STRING_HIGHER
 *   
 */
int PT_DECLSPEC pstrCompareStrings(wchar_t *wcp_string1, 
											  wchar_t *wcp_string2,
											  int i_case_sensitive,
											  int* ip_comparison)
{
	int strcmp_value;
	wchar_t wcp_upper_string1[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_upper_string2[PT_MAX_GENERIC_STRLEN];

	if (wcp_string1 == NULL || wcp_string2 == NULL)
		return(NOT_OKAY);

	if (i_case_sensitive)
	{
		strcmp_value = wcsncmp(wcp_string1, wcp_string2, PT_MAX_GENERIC_STRLEN);
		
	}else
	{
		if (pstrToUpper_Wide(wcp_string1, wcp_upper_string1) != OKAY)
			return(NOT_OKAY);
		if (pstrToUpper_Wide(wcp_string2, wcp_upper_string2) != OKAY)
			return(NOT_OKAY);

		strcmp_value = wcsncmp(wcp_upper_string1, wcp_upper_string2, PT_MAX_GENERIC_STRLEN);
	}

	if (strcmp_value == 0)
		*ip_comparison = PSTR_COMPARE_STRINGS_EQUAL;

	if (strcmp_value > 0)
		*ip_comparison = PSTR_COMPARE_FIRST_STRING_HIGHER;

	if (strcmp_value < 0)
		*ip_comparison = PSTR_COMPARE_FIRST_STRING_LOWER;

	return(OKAY);
}
