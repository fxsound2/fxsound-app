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
#ifndef _SPECTRUM_H_
#define _SPECTRUM_H_

#include "pt_defs.h"
#include "codedefs.h"
#include "slout.h"

#define SPECTRUM_MAX_NUM_BANDS 10

// For warping relative band sensitivities
#define SPECTRUM_BAND_1_WARP  0.6
#define SPECTRUM_BAND_2_WARP  0.6
#define SPECTRUM_BAND_3_WARP  1.0
#define SPECTRUM_BAND_4_WARP  1.0
#define SPECTRUM_BAND_5_WARP  1.3
#define SPECTRUM_BAND_6_WARP  1.3
#define SPECTRUM_BAND_7_WARP  1.3
#define SPECTRUM_BAND_8_WARP  1.3
#define SPECTRUM_BAND_9_WARP  1.5
#define SPECTRUM_BAND_10_WARP 1.5

#define SPECTRUM_MIN_OUTPUT_VALUE 0.0
#define SPECTRUM_MAX_OUTPUT_VALUE 1.0

// Display time constant range for spectrumSetTimeConstant()
#define SPECTRUM_MIN_TIME_CONSTANT 1.0
#define SPECTRUM_MAX_TIME_CONSTANT 200.0

// 10.0 was the version 8 setting
#define SPECTRUM_DEFAULT_TIME_CONSTANT 10.0

// For adjusting band sensitivity using spectrumSetSensitivity()
#define SPECTRUM_MIN_SENSITIVITY 0.0
#define SPECTRUM_MAX_SENSITIVITY 10.0
#define SPECTRUM_DEFAULT_SENSITIVITY 1.0

#define SPECTRUM_MAX_DELAY_SECS 5.0
#define SPECTRUM_MIN_DELAY_SECS 0.0

/* spectrumInit.cpp */
int PT_DECLSPEC spectrumNew(PT_HANDLE **hpp_spectrum, int, realtype, realtype, CSlout *, int);
int PT_DECLSPEC spectrumReset(PT_HANDLE *hp_spectrum);
int PT_DECLSPEC spectrumFreeUp(PT_HANDLE **hpp_spectrum);

/* spectrumProcess.cpp */
int PT_DECLSPEC spectrumProcess(PT_HANDLE *, realtype *,	int, int, realtype, int);

/* spectrumGet.cpp */
int PT_DECLSPEC spectrumGetBandValues(PT_HANDLE *, realtype *,	int);

/* spectrumMessageValues.cpp */
int PT_DECLSPEC spectrumGetMessageValuesFromBandValues_NoHandle(realtype *, int, WPARAM *, LPARAM *);
int PT_DECLSPEC spectrumGetBandValuesFromMessageValues_NoHandle(WPARAM, LPARAM, realtype *, int, int *);
int PT_DECLSPEC spectrumSetBandValuesFromMessageValues_NoHandle(WPARAM, LPARAM, realtype *, int, int *);

/* spectrumSet.cpp */
int PT_DECLSPEC spectrumSetTimeConstant(PT_HANDLE *, realtype);
int PT_DECLSPEC spectrumSetSensitivity(PT_HANDLE *, realtype);
int PT_DECLSPEC spectrumSetDelay(PT_HANDLE *, realtype);

#endif /* _SPECTRUM_H_*/
