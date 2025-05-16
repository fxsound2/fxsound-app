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

/*
 * FUNCTION: pstrCalcHourMinSecondsString()
 * DESCRIPTION:
 *   Based on the passed number of seconds, calculate the appropriate Hours:Minutes:Seconds string.
 */
int PT_DECLSPEC pstrCalcHourMinSecondsString(int i_num_seconds, char *cp_hms_representation)
{
   int tmp_seconds_left;
	int num_hours;
	int num_minutes;
	int num_seconds;

	if (cp_hms_representation == NULL)
		return(NOT_OKAY);

   tmp_seconds_left = i_num_seconds;
   num_hours = tmp_seconds_left / 3600; 
   tmp_seconds_left = tmp_seconds_left % 3600;
   num_minutes = tmp_seconds_left / 60;
   num_seconds = tmp_seconds_left % 60;

	if (num_hours > 0)
      sprintf(cp_hms_representation, "%d:%02d:%02d", num_hours, num_minutes, num_seconds);
	else
      sprintf(cp_hms_representation, "%d:%02d", num_minutes, num_seconds);

	return(OKAY);
}

/*
 * FUNCTION: pstrCalcHourMinSecondsString_Wide()
 * DESCRIPTION:
 *   Based on the passed number of seconds, calculate the appropriate Hours:Minutes:Seconds string.
 */
int PT_DECLSPEC pstrCalcHourMinSecondsString_Wide(int i_num_seconds, wchar_t *wcp_hms_representation)
{
   int tmp_seconds_left;
	int num_hours;
	int num_minutes;
	int num_seconds;

	if (wcp_hms_representation == NULL)
		return(NOT_OKAY);

   tmp_seconds_left = i_num_seconds;
   num_hours = tmp_seconds_left / 3600; 
   tmp_seconds_left = tmp_seconds_left % 3600;
   num_minutes = tmp_seconds_left / 60;
   num_seconds = tmp_seconds_left % 60;

	if (num_hours > 0)
      swprintf(wcp_hms_representation, L"%d:%02d:%02d", num_hours, num_minutes, num_seconds);
	else
      swprintf(wcp_hms_representation, L"%d:%02d", num_minutes, num_seconds);

	return(OKAY);
}