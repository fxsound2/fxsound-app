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

/*
 * FILE: ptime.h
 * DATE: 5/8/98
 * AUTOHOR: Paul Titchener
 * DESCRIPTION:
 *
 *  Public defines for the ptime module (time functions)
 */

#ifndef _PTIME_H_
#define _PTIME_H_

#include <windows.h>

#include "time.h"

#define PTIME_HOUR_COUNT 3600
#define PTIME_DAY_COUNT (PTIME_HOUR_COUNT * 24)
#define PTIME_MONTH_COUNT (PTIME_DAY_COUNT * 30)
#define PTIME_YEAR_COUNT (PTIME_DAY_COUNT * 365)

/* ptime.cpp */
int PT_DECLSPEC ptimeGetTime(long *);
int PT_DECLSPEC ptimeGetElapsedDays(long, int *);
int PT_DECLSPEC ptimeCheckTimeWentForwared(long, long, int *);
int PT_DECLSPEC ptimeGetDateStr(wchar_t *, int);
int PT_DECLSPEC ptimeGetDateAndTimeStr(wchar_t *);
int PT_DECLSPEC ptimeGetTimeStr(wchar_t *);
int PT_DECLSPEC ptimeParseDateStr(char *, int, int *, int *, int *);
int PT_DECLSPEC ptimeConvertMDYtoTimeT(int, int, int, time_t *);
int PT_DECLSPEC ptimeUnixTimeToFileTime(time_t t, LPFILETIME pft);
int PT_DECLSPEC ptimeUnixTimeToSystemTime(time_t t, LPSYSTEMTIME pst);
int PT_DECLSPEC ptimeUnixTimeToFormattedString(long, bool, bool, wchar_t *, wchar_t *);
int PT_DECLSPEC ptimeFILETIMEToFormattedString(FILETIME*, wchar_t *);
int PT_DECLSPEC ptimeCheckIfTimeIsToday(long, bool *);
int PT_DECLSPEC ptimeConvertSecondsToNiceText(time_t i_seconds, wchar_t * wcp_nice_text);

#endif /* _PTIME_H_ */
