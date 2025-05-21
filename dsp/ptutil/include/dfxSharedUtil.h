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
#ifndef _DFX_SHARED_UTIL_H_
#define _DFX_SHARED_UTIL_H_
                                    
int PT_DECLSPEC dfxSharedUtilInit(PT_HANDLE **, int, CSlout *);
int PT_DECLSPEC dfxSharedUtilFreeUp(PT_HANDLE **);
int PT_DECLSPEC dfxSharedUtilSetSpectrumValues(PT_HANDLE *, realtype *, int);
int PT_DECLSPEC dfxSharedUtilGetSpectrumValues(PT_HANDLE *, realtype *, int);
int PT_DECLSPEC dfxSharedUtilSetTotalProcessedTime(PT_HANDLE *, unsigned long);
int PT_DECLSPEC dfxSharedUtilGetTotalProcessedTime(PT_HANDLE *, unsigned long *);
int PT_DECLSPEC dfxSharedUtilSetFlag(PT_HANDLE *, int, int);
int PT_DECLSPEC dfxSharedUtilGetFlag(PT_HANDLE *, int, int *);

#endif // _DFX_SHARED_UTIL_H_
