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
#ifndef _REALSAMPLE_H_
#define _REALSAMPLE_H_

#include "slout.h"  
#include "codedefs.h"

#define REAL_SAMPLE_LEGAL_VALUE_MIN -100.0
#define REAL_SAMPLE_LEGAL_VALUE_MAX  100.0

/* realSample.cpp */
int PT_DECLSPEC realSampleNew(PT_HANDLE **, long, int, int, CSlout *);
int PT_DECLSPEC realSampleFreeUp(PT_HANDLE **);

/* realSampleCreateFromIntSample.cpp */
int PT_DECLSPEC realSampleCreateFromIntSampleHdl(PT_HANDLE **, PT_HANDLE *, CSlout *);

/* realSampleCopy.cpp */
int PT_DECLSPEC realSampleCopy(PT_HANDLE *, PT_HANDLE **);

/* realSampleForceLegalValues.cpp */
int PT_DECLSPEC realSampleForceLegalValues_ArrayOnly(realtype *, long);

/* realSampleGet.cpp */
int PT_DECLSPEC realSampleGetAttributes(PT_HANDLE *, int *, long *);
int PT_DECLSPEC realSampleGetSampleSegment(PT_HANDLE *, realtype **, long *);

/* realSampleSet.cpp */
int PT_DECLSPEC realSampleSetAttributes(PT_HANDLE *, int, long);
int PT_DECLSPEC realSampleClear(PT_HANDLE *);

/* realSampleResize.cpp */
int PT_DECLSPEC realSampleResize(PT_HANDLE *, long);

/* realSampleMix.cpp */
int PT_DECLSPEC realSampleScale(PT_HANDLE *hp_realSample, realtype r_scale_factor);
int PT_DECLSPEC realSampleSum(PT_HANDLE *, PT_HANDLE *);
int PT_DECLSPEC realSampleScaledSum(PT_HANDLE *, PT_HANDLE *, realtype, realtype);
int PT_DECLSPEC realSampleMix2(PT_HANDLE *, PT_HANDLE *, realtype, realtype, PT_HANDLE *);
int PT_DECLSPEC realSampleScaledSum_NoHandle(realtype *, int, realtype,
                                             realtype *, int,realtype);
int PT_DECLSPEC realSampleScale_NoHandle(realtype *, int, realtype);
int PT_DECLSPEC realSampleSum_NoHandle(realtype *, int, realtype *, int);

/* realSampleConvertRate.cpp */
int PT_DECLSPEC realSampleConvertRate(PT_HANDLE *, long);

/* realSampleSpecialRemix.cpp */
int PT_DECLSPEC realSampleSpecialRemixScaleOriginal_NoHandle(realtype *, int, realtype, realtype);
int PT_DECLSPEC realSampleSpecialRemixScaledSumBeats_NoHandle(realtype *, int, realtype *, int, realtype, realtype, realtype);

#endif //_REALSAMPLE_H_