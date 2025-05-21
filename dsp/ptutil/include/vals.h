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
#ifndef _VALS_H_
#define _VALS_H_

#include "slout.h"

#define VALS_NUM_MAIN_PARAMS    6
#define VALS_NUM_ELEMENT_PARAMS 7
#define VALS_MAX_NUM_ELEMENTS   8    

/* Defines for double params list */
#define VALS_PARAM_SET_1 1
#define VALS_PARAM_SET_2 2

/* vals.c */
int PT_DECLSPEC valsInit(PT_HANDLE **, CSlout *, int, realtype, int); 
int PT_DECLSPEC valsInitAppDependentInfo(PT_HANDLE *, int, int, int);
int PT_DECLSPEC valsResetNumAppDependentVars(PT_HANDLE *, int, int, int);
int PT_DECLSPEC valsSetMaintainMax(PT_HANDLE *, int, int);
int PT_DECLSPEC valsSetMaintainMax1and4(PT_HANDLE *, int, int);
int PT_DECLSPEC valsFreeUp(PT_HANDLE **);
int PT_DECLSPEC valsDump(PT_HANDLE *);
int PT_DECLSPEC valsCopy(PT_HANDLE *, PT_HANDLE **);
int PT_DECLSPEC valsCompare(PT_HANDLE *, PT_HANDLE *, int *);

/* valsCfg.c */
int PT_DECLSPEC valsCfgRead(PT_HANDLE **, wchar_t *, CSlout *);  
int PT_DECLSPEC valsCfgWrite(PT_HANDLE *);
int PT_DECLSPEC valsCfgCheckFileType(wchar_t *, int *);
int PT_DECLSPEC valsCfgGetTitle(PT_HANDLE *, wchar_t *);
int PT_DECLSPEC valsCfgGetNumElements(PT_HANDLE *, int *);
int PT_DECLSPEC valsCfgGetStereoInputMode(PT_HANDLE *, int *); 
int PT_DECLSPEC valsCfgGetStereoOutputMode(PT_HANDLE *, int *);
int PT_DECLSPEC valsCfgGetDefaultPreset(PT_HANDLE *, int *); 
int PT_DECLSPEC valsCfgGetMinUserPreset(PT_HANDLE *, int *);
int PT_DECLSPEC valsCfgGetPresetSaveTime(PT_HANDLE *, long *);
int PT_DECLSPEC valsCfgSetNumElements(PT_HANDLE *, int);
int PT_DECLSPEC valsCfgSetStereoInputMode(PT_HANDLE *, int);  
int PT_DECLSPEC valsCfgSetStereoOutputMode(PT_HANDLE *, int);
int PT_DECLSPEC valsCfgSetDefaultPreset(PT_HANDLE *, int); 
int PT_DECLSPEC valsCfgSetMinUserPreset(PT_HANDLE *, int);
int PT_DECLSPEC valsCfgSetPresetSaveTime(PT_HANDLE *, long);
int PT_DECLSPEC valsCfgFreeUp(PT_HANDLE **);

/* valsSet.c */ 
int PT_DECLSPEC valsSetTotalNumElements(PT_HANDLE *, int); 
int PT_DECLSPEC valsSetComment(PT_HANDLE *, wchar_t *); 
int PT_DECLSPEC valsSetParamSet(PT_HANDLE *, int);
int PT_DECLSPEC valsSetMainParamValue(PT_HANDLE *, int, int);
int PT_DECLSPEC valsSetElementParamValue(PT_HANDLE *, int, int, int); 
int PT_DECLSPEC valsSetAppDependentInt(PT_HANDLE *, int, int);
int PT_DECLSPEC valsSetAppDependentReal(PT_HANDLE *, int, realtype);
int PT_DECLSPEC valsSetAppDependentString(PT_HANDLE *, int, wchar_t *);
int PT_DECLSPEC valsSetGraphicEq(PT_HANDLE *, PT_HANDLE *, int);
int PT_DECLSPEC valsSetGraphicEqOn(PT_HANDLE *, int);

/* valsGet.c */
int PT_DECLSPEC valsGetFileVersion(PT_HANDLE *, realtype *);
int PT_DECLSPEC valsGetTotalNumElements(PT_HANDLE *, int *);
int PT_DECLSPEC valsGetComment(PT_HANDLE *, wchar_t **);
int PT_DECLSPEC valsGetMainParamValue(PT_HANDLE *, int, int *);
int PT_DECLSPEC valsGetElementParamValue(PT_HANDLE *, int, int, int *);
int PT_DECLSPEC valsGetMaxValue(PT_HANDLE *, int, int *);
int PT_DECLSPEC valsGetMaxElement(PT_HANDLE *, int, int *);
int PT_DECLSPEC valsGetMax1and4Element(PT_HANDLE *, int *); 
int PT_DECLSPEC valsGetNumAppDependentVars(PT_HANDLE *, int *, int *, int *);
int PT_DECLSPEC valsGetAppDependentInt(PT_HANDLE *, int, int *);
int PT_DECLSPEC valsGetAppDependentReal(PT_HANDLE *, int, realtype *);
int PT_DECLSPEC valsGetAppDependentString(PT_HANDLE *, int, wchar_t **);
int PT_DECLSPEC valsGetDoubleSetType(PT_HANDLE *, int *);
int PT_DECLSPEC valsGetParamSet(PT_HANDLE *, int *);
int PT_DECLSPEC valsGetGraphicEq(PT_HANDLE *, PT_HANDLE **, int *);

/* valsFile.c */
int PT_DECLSPEC valsSave(PT_HANDLE *, wchar_t *, wchar_t *);
int PT_DECLSPEC valsRead(wchar_t *, int, CSlout *, PT_HANDLE **);
int PT_DECLSPEC valsCheckFileType(wchar_t *, int *, CSlout *);
int PT_DECLSPEC valsCalcDateStrings(wchar_t *, wchar_t *);

/* valsWarp.c */
int PT_DECLSPEC valsWarp(PT_HANDLE *, PT_HANDLE *, int, realtype, int, int, int);
int PT_DECLSPEC valsWarp1and4(PT_HANDLE *, PT_HANDLE *, realtype, int, int, int, int, int, 
                  PT_HANDLE *, PT_HANDLE *); 
int PT_DECLSPEC valsPanStyleWarp(PT_HANDLE *, PT_HANDLE *, int, int, int, int);

#endif //_VALS_H_
