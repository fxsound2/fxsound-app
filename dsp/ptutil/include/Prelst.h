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
#ifndef _PRELST_H_
#define _PRELST_H_

#include "slout.h" 

#define PRELST_FACTORY_EXTENSION_WIDE L"fac"
#define PRELST_USER_EXTENSION_WIDE    L"usr" 

#define PRELST_MAX_USER_PRESET_INDEX 1023
#define PRELIST_INDEX_NONE     -1

/* prelst.cpp */
int PT_DECLSPEC prelstCreate(PT_HANDLE **, CSlout *, wchar_t *, wchar_t *, int, int);
int PT_DECLSPEC prelstGetExists(PT_HANDLE *, int, int *);
int PT_DECLSPEC prelstConstructFullpath(PT_HANDLE *, int, wchar_t *);
int PT_DECLSPEC prelstConstructFilename(PT_HANDLE *, int , wchar_t *);
int PT_DECLSPEC prelstAskIsFactory(PT_HANDLE *, int, int *);
int PT_DECLSPEC prelstAddFile(PT_HANDLE *, int);
int PT_DECLSPEC prelstRemoveFile(PT_HANDLE *, int); 
int PT_DECLSPEC prelstCalcListToRealNum(PT_HANDLE *, int, int *);
int PT_DECLSPEC prelstCalcRealToListNum(PT_HANDLE *, int, int *); 
int PT_DECLSPEC prelstNextAvailableUserPreset(PT_HANDLE *, int *);
int PT_DECLSPEC prelstFreeUp(PT_HANDLE **);
int PT_DECLSPEC prelstDump(PT_HANDLE *);

#endif //_PRELST_H
