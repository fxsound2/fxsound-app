/*
FxSound
Copyright (C) 2023  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
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
#include <time.h>

#include "slout.h"
#include "Ptime.h"
#include "mth.h"
#include "pt_defs.h"
#include "pstr.h"

/* #include "u_ptime.h" (don't need one yet) */

/*
 * FUNCTION: ptimeGetTime()
 * DESCRIPTION:
 *
 * Passes back the time in seconds since 1/1/70.
 */
int PT_DECLSPEC ptimeGetTime(long *lp_seconds)
{
	time_t secs;

	time(&secs);
	
	*lp_seconds = (long)secs;
	
	return(OKAY);
}
   
/*
 * FUNCTION: ptimeCheckIfTimeIsToday()
 * DESCRIPTION:
 *
 * Based on the passed unix time, passes back if it is from today.
 */
int PT_DECLSPEC ptimeCheckIfTimeIsToday(long l_check_time, bool *bp_is_today)
{
	long l_current_time;
	SYSTEMTIME st_current_time;
	SYSTEMTIME st_check_time;
	SYSTEMTIME local_st_current_time;
	SYSTEMTIME local_st_check_time;
	wchar_t wcp_date_current_time[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_date_check_time[PT_MAX_GENERIC_STRLEN];
	TIME_ZONE_INFORMATION tzone;

	*bp_is_today = false;

	if (ptimeGetTime(&l_current_time) != OKAY)
		return(NOT_OKAY);

	// Format the timestamps into readable date time string
	ptimeUnixTimeToSystemTime(l_current_time, &st_current_time);
	ptimeUnixTimeToSystemTime(l_check_time, &st_check_time);

	// Get current system time zone
	GetTimeZoneInformation(&tzone);

	// Translate UTC SYSTEMTIME into local SYSTEMTIME
	SystemTimeToTzSpecificLocalTime(&tzone, &st_current_time, &local_st_current_time);
	SystemTimeToTzSpecificLocalTime(&tzone, &st_check_time, &local_st_check_time);

	// Format them
	GetDateFormatW(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE , &local_st_current_time, NULL, wcp_date_current_time, PT_MAX_GENERIC_STRLEN);
	GetDateFormatW(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE , &local_st_check_time, NULL, wcp_date_check_time, PT_MAX_GENERIC_STRLEN);

	/* Check if the two dates are the same */
	if (wcscmp(wcp_date_current_time, wcp_date_check_time) == 0)
		*bp_is_today = true;

	return(OKAY);
}

/*
 * FUNCTION: ptimeGetElapsedDays()
 * DESCRIPTION:
 *
 * Passes back the number of complete days since the 
 * time of the passed in second count.
 */
int PT_DECLSPEC ptimeGetElapsedDays(long l_seconds, int *ip_elapsed_days)
{
	time_t secs;

	time(&secs);
	
	*ip_elapsed_days = (int)((secs - l_seconds)/(PTIME_DAY_COUNT));
	
	return(OKAY);
}

/*
 * FUNCTION: ptimeCheckTimeWentForwared()
 * DESCRIPTION:
 *
 * Passes back if the first time is less than the second time (yet
 * accounts for daylight savings.
 */
int PT_DECLSPEC ptimeCheckTimeWentForwared(long l_time1, long l_time2, int *ip_time_went_forward)
{
   long time1_back_1_hour;

   *ip_time_went_forward = IS_FALSE;

   /* 
    * Subtract 1 hour from time 1 to allow for daylight savings with an extra
	* hour of slop.
    */
   time1_back_1_hour = l_time1 - 7200L;

   if (time1_back_1_hour <= l_time2)
      *ip_time_went_forward = IS_TRUE;
   
   return(OKAY);
}

/*
 * FUNCTION: ptimeGetDateStr()
 * DESCRIPTION:
 *   Fills in the passed string with a formatted string representation of
 *   the current date.
 */
int PT_DECLSPEC ptimeGetDateStr(wchar_t *wcp_formatted, int i_with_dashes)
{
	time_t time_structure;
	struct tm *tm_struct_ptr;

	/* Get the current time */
	time(&time_structure);
	tm_struct_ptr = localtime(&time_structure);
	mktime(tm_struct_ptr);

	/* Fill in formatted string */
	if (i_with_dashes)
   {
	   if (wcsftime(wcp_formatted, 24, L"%m/%d/%y",
					    tm_struct_ptr) == 0)
		   return(NOT_OKAY);
	}
	else
	{
	   if (wcsftime(wcp_formatted, 24, L"%m%d%y",
					    tm_struct_ptr) == 0)
		   return(NOT_OKAY);
	}
	
	return(OKAY);
}

/*
 * FUNCTION: ptimeGetDateAndTimeStr()
 * DESCRIPTION:
 *   Fills in the passed string with a formatted string representation of
 *   the current date and time.
 */
int PT_DECLSPEC ptimeGetDateAndTimeStr(wchar_t *wcp_formatted)
{
	time_t time_structure;
	struct tm *tm_struct_ptr;

	/* Get the current time */
	time(&time_structure);
	tm_struct_ptr = localtime(&time_structure);
	mktime(tm_struct_ptr);

	/* Fill in formatted string */
	if (wcsftime(wcp_formatted, 24, L"%m/%d/%Y %H:%M:%S",
					 tm_struct_ptr) == 0)
	   return(NOT_OKAY);
	
	return(OKAY);
}

/*
 * FUNCTION: ptimeGetTimeStr()
 * DESCRIPTION:
 *   Fills in the passed string with a formatted string representation of
 *   the current time.
 */
int PT_DECLSPEC ptimeGetTimeStr(wchar_t *wcp_formatted)
{
	time_t time_structure;
	struct tm *tm_struct_ptr;

	/* Get the current time */
	time(&time_structure);
	tm_struct_ptr = localtime(&time_structure);
	mktime(tm_struct_ptr);

	/* Fill in formatted string */
	if (wcsftime(wcp_formatted, 24, L"%H:%M:%S",
					 tm_struct_ptr) == 0)
	   return(NOT_OKAY);
	
	return(OKAY);
}

/*
 * FUNCTION: ptimeParseDateStr()
 * DESCRIPTION:
 *   Parses the passed date string and returns the number of the month day
 *   and year.
 *
 *   If i_with_dashes is IS_TRUE it must be for example "08/13/64"
 *   If i_with_dashes is IS_FALSE it must be for example "081364"
 */
int PT_DECLSPEC ptimeParseDateStr(char *cp_formatted, int i_with_dashes,
											 int *ip_month, int *ip_day, int *ip_year)
{
	int is_long;
	char cp_month[10];
	char cp_day[10];
	char cp_year[10];

	if (cp_formatted == NULL)
		return(NOT_OKAY);

	if (i_with_dashes)
	{
		/* Make sure it has the proper number of characters */ 
		if (strlen(cp_formatted) != 8)
			return(NOT_OKAY);
		
		cp_month[0] = cp_formatted[0];
		cp_month[1] = cp_formatted[1];
		cp_month[2] = L'\0';

		cp_day[0] = cp_formatted[3];
		cp_day[1] = cp_formatted[4];
		cp_day[2] = L'\0';

		cp_year[0] = cp_formatted[6];
		cp_year[1] = cp_formatted[7];
		cp_year[2] = L'\0';
	}
	else
	{
		/* Make sure it has the proper number of characters */ 
		if (strlen(cp_formatted) != 6)
			return(NOT_OKAY);
		
		cp_month[0] = cp_formatted[0];
		cp_month[1] = cp_formatted[1];
		cp_month[2] = L'\0';

		cp_day[0] = cp_formatted[2];
		cp_day[1] = cp_formatted[3];
		cp_day[2] = L'\0';

		cp_year[0] = cp_formatted[4];
		cp_year[1] = cp_formatted[5];
		cp_year[2] = L'\0';
	}

	/* Make sure everything is a legal integer */
	if (mthIsLong(cp_month, &is_long) != OKAY)
		return(NOT_OKAY);
	if (!is_long)
		return(NOT_OKAY);

	if (mthIsLong(cp_day, &is_long) != OKAY)
		return(NOT_OKAY);
	if (!is_long)
		return(NOT_OKAY);

	if (mthIsLong(cp_year, &is_long) != OKAY)
		return(NOT_OKAY);
	if (!is_long)
		return(NOT_OKAY);

	/* Convert to integers */
	*ip_month = atoi(cp_month);
	*ip_day = atoi(cp_day);
	*ip_year = atoi(cp_year);

   return(OKAY);
}

/*
 * FUNCTION: ptimeConvertMDYtoTimeT()
 * DESCRIPTION:
 *   Converts the passed month, day, and year into a time_t value.
 */
int PT_DECLSPEC ptimeConvertMDYtoTimeT(int i_month, int i_day, int i_year, 
													time_t *tt_converted)
{
	struct tm time_struct;

	time_struct.tm_mon = i_month - 1;
	time_struct.tm_mday = i_day;

	/* 
	 * Take care of the Y2K problem by putting in that if the year
	 * is less than or equal to 70, than it must be in the 21st Century.
	 */
	if (i_year <= 70)
		i_year = i_year + 100;

	time_struct.tm_year = i_year;

	/* Have mktime calculate daylight savings */
   time_struct.tm_isdst = -1;

	/* Fill in the other default values */
   time_struct.tm_sec = 0;
	time_struct.tm_min = 0;
	time_struct.tm_hour = 0;


	*tt_converted = mktime(&time_struct);

	if (*tt_converted == -1)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: ptimeCompareSystemTimes()
 * DESCRIPTION:
 *
 *   Compare the passed system times structures.
 *
 */
int PT_DECLSPEC ptimeCompareSystemTimes(SYSTEMTIME *st_time1,
													 SYSTEMTIME *st_time2,
													 int *ip_times_same,
													 int *ip_time1_earlier)
{
   FILETIME ft_time1;
   FILETIME ft_time2;
	LONG compare_value;

	if ((st_time1 == NULL) || (st_time2 == NULL))
		return(NOT_OKAY);

	/* Convert the system times to filetimes */
   if (SystemTimeToFileTime(st_time1, &ft_time1) == 0)
		return(NOT_OKAY);
   if (SystemTimeToFileTime(st_time2, &ft_time2) == 0)
      return(NOT_OKAY);

   compare_value = CompareFileTime(&ft_time1, &ft_time2);

	if (compare_value == 0L)
	{
		*ip_times_same = IS_TRUE;
	   *ip_time1_earlier = IS_FALSE;
		return(OKAY);
	}
	else if (compare_value == -1L)
	{
	   *ip_times_same = IS_FALSE;
	   *ip_time1_earlier = IS_TRUE;
	}
	else if (compare_value == 1L)
	{
	   *ip_times_same = IS_FALSE;
	   *ip_time1_earlier = IS_FALSE;
	}
	else
		return(NOT_OKAY);

   return(OKAY);
}

/*
 * FUNCTION: ptimeUnixTimeToFileTimes()
 * DESCRIPTION:
 *
 *   Taken from Microsoft's KB article Q167297
 *   http://support.microsoft.com/default.aspx?scid=KB;en-us;q167296
 *
 */
int PT_DECLSPEC ptimeUnixTimeToFileTime(time_t t, LPFILETIME pft)
{
     // Note that LONGLONG is a 64-bit value
     LONGLONG ll;

     ll = (t * 10000000LL) + 116444736000000000LL;
     pft->dwLowDateTime = (DWORD)ll;
     pft->dwHighDateTime = (DWORD)(ll >> 32);

	 return(OKAY);
}

/*
 * FUNCTION: ptimeUnixTimeToSystemTime()
 * DESCRIPTION:
 *
 *   Taken from Microsoft's KB article Q167297
 *   http://support.microsoft.com/default.aspx?scid=KB;en-us;q167296
 */
int PT_DECLSPEC ptimeUnixTimeToSystemTime(time_t t, LPSYSTEMTIME pst)
{
     FILETIME ft;

     ptimeUnixTimeToFileTime(t, &ft);
     FileTimeToSystemTime(&ft, pst);

	 return(OKAY);
}

/*
 * FUNCTION: ptimeUnixTimeToFormattedString()
 * DESCRIPTION:
 *
 * Convert the passed unix time to a formatted string for display.  Optionally, it can hide the date portion of the 
 * string if the date is the same as the current day's.
 */
int PT_DECLSPEC ptimeUnixTimeToFormattedString(long l_timestamp, 
															  bool b_hide_todays_date, 
															  bool b_only_use_legal_filename_chars,
															  wchar_t *wcp_word_today_translation,
															  wchar_t *wcp_formatted_datetime)
{
	wchar_t wcp_date[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_time[PT_MAX_GENERIC_STRLEN];
	SYSTEMTIME st;
	SYSTEMTIME local_st;
	TIME_ZONE_INFORMATION tzone;
	bool b_display_date;
	bool b_time_is_today;

	if(l_timestamp < 0)
		return(NOT_OKAY);

	// Format the timestamp into readable date time string
	ptimeUnixTimeToSystemTime(l_timestamp, &st);

	// Get current system time zone
	GetTimeZoneInformation(&tzone);

	// Translate UTC SYSTEMTIME into local SYSTEMTIME
	SystemTimeToTzSpecificLocalTime(&tzone, &st, &local_st);

	// Format it
	GetDateFormatW(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE , &local_st, NULL, wcp_date, PT_MAX_GENERIC_STRLEN);
	GetTimeFormatW(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE , &local_st, NULL, wcp_time, PT_MAX_GENERIC_STRLEN);
	
	if (b_only_use_legal_filename_chars)
	{
		wchar_t wc_slash = L'/';
		wchar_t wc_dash = L'-';
		wchar_t wc_colon = L':';

		if (pstrReplaceCharOccurances_Wide(wcp_date, wc_slash, wc_dash) != OKAY)
			return(NOT_OKAY);
		if (pstrReplaceCharOccurances_Wide(wcp_time, wc_colon, wc_dash) != OKAY)
			return(NOT_OKAY);
	}

	/* Determine if we should include the date portion of the string */
	b_display_date = true;
	if (b_hide_todays_date)
	{
		if (ptimeCheckIfTimeIsToday(l_timestamp, &b_time_is_today) != OKAY)
			return(NOT_OKAY);
		if (b_time_is_today)
			b_display_date = false;
	}

	if (b_display_date)
		swprintf(wcp_formatted_datetime, L"%s - %s", wcp_time, wcp_date);
	else
		swprintf(wcp_formatted_datetime, L"%s - %s", wcp_time, wcp_word_today_translation);

	return(OKAY);
}

/*
 * FUNCTION: ptimeFILETIMEToFormattedString()
 * DESCRIPTION:
 *
 * Convert the passed FILETIME to a formatted string for display.
 */
int PT_DECLSPEC ptimeFILETIMEToFormattedString(FILETIME* pft_timestamp, wchar_t *wcp_formatted_datetime)
{
	wchar_t wcp_date[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_time[PT_MAX_GENERIC_STRLEN];
	SYSTEMTIME st;
	SYSTEMTIME local_st;
	TIME_ZONE_INFORMATION tzone;

	// Convert FILETIME to SYSTEMTIME
	FileTimeToSystemTime(pft_timestamp, &st);

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

/*
* FUNCTION: ptimeConvertSecondsToNiceText()
* DESCRIPTION:
*
* Convert i_seconds to textual representation of the time in either number of days, hours, minutes or seconds.
*
* For example, if i_seconds is 60, it returns "1 minutes". If i_seconds is 86500, it retunrs "1 day".
*/
int PT_DECLSPEC ptimeConvertSecondsToNiceText(time_t i_seconds, wchar_t * wcp_nice_text)
{
	struct tm *data;
	data = gmtime(&i_seconds);
	int seconds = data->tm_sec;
	int minutes = data->tm_min;
	int hours = data->tm_hour;
	int days = data->tm_yday;

	if (days != 0)
	{
		if (days == 1)
		{
			if (hours == 1)
			{
				swprintf(wcp_nice_text, L"%d day 1 hour", days);
			}
			else if (hours == 0)
			{
				swprintf(wcp_nice_text, L"%d day", days);
			}
			else
			{
				swprintf(wcp_nice_text, L"%d day %d hours", days, hours);
			}
		}
		else
		{
			if (hours == 1)
			{
				swprintf(wcp_nice_text, L"%d days 1 hour", days);
			}
			else if (hours == 0)
			{
				swprintf(wcp_nice_text, L"%d days", days);
			}
			else
			{
				swprintf(wcp_nice_text, L"%d days %d hours", days, hours);
			}
		}
	}
	else if (hours != 0)
	{
		if (hours == 1)
		{
			swprintf(wcp_nice_text, L"%d hour", hours);
		}
		else
		{
			swprintf(wcp_nice_text, L"%d hours", hours);
		}
	}
	else if (minutes != 0)
	{
		if (minutes == 1)
		{
			swprintf(wcp_nice_text, L"%d minute", minutes);
		}
		else
		{
			swprintf(wcp_nice_text, L"%d minutes", minutes);
		}
	}
	else if (seconds != 0)
	{
		if (seconds == 1)
		{
			swprintf(wcp_nice_text, L"%d second", seconds);
		}
		else
		{
			swprintf(wcp_nice_text, L"%d seconds", seconds);
		}
	}

	// HACK
	//swprintf(wcp_nice_text, L"%d seconds", seconds);
	// END HACK

	return(OKAY);
}