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
#ifndef _SOS_H_
#define _SOS_H_

#include "slout.h"
 
/* Maximum number of Second Order Sections response handle can take */
/* #define SOS_MAX_NUM_SOS_SECTIONS 16   Setting used up to 12/23/02 */
#define SOS_MAX_NUM_SOS_SECTIONS 32

/* Section filter types */
#define SOS_GENERIC 0
#define SOS_PARA  1
#define SOS_SHELF SOS_GENERIC

/* sos.cpp */
int PT_DECLSPEC sosNew(PT_HANDLE **, CSlout *, int);
int PT_DECLSPEC sosFreeUp(PT_HANDLE **);
int PT_DECLSPEC sosDump(PT_HANDLE *);

/* sosSet.cpp */
int PT_DECLSPEC sosSetSection(PT_HANDLE *hp_sos, int i_section_num, int i_set_freq, 
                  				struct filt2ndOrderBoostCutShelfFilterType *filt,
										int i_filt_type);
int PT_DECLSPEC sosSetSectionUnityGain(PT_HANDLE *, int, int);
int PT_DECLSPEC sosSetAllSectionsUnityGain(PT_HANDLE *, int);
int PT_DECLSPEC sosSetNumActiveSections(PT_HANDLE *, int);
int PT_DECLSPEC sosSetMasterGain(PT_HANDLE *, realtype);
int PT_DECLSPEC sosSetSectionResponseFlag(PT_HANDLE *, int, int);
int PT_DECLSPEC sosZeroStateAllSections(PT_HANDLE *);
int PT_DECLSPEC sosSetAppHasHyperBassMode(PT_HANDLE *, bool);
int PT_DECLSPEC sosSetDisableBand1Flag(PT_HANDLE *, bool);
int PT_DECLSPEC sosSetVolumeNormalization(PT_HANDLE*, realtype);

/* sosGet.cpp */
int PT_DECLSPEC sosGetMasterGain(PT_HANDLE *hp_sos, realtype *);
int PT_DECLSPEC sosGetNumAllocatedSections(PT_HANDLE *, int *);
int PT_DECLSPEC sosGetNumActiveSections(PT_HANDLE *, int *);
int PT_DECLSPEC sosGetCoef(PT_HANDLE *, int, int, int, realtype *);
int PT_DECLSPEC sosGetSection(PT_HANDLE *, int, realtype *, realtype *, realtype *, 
                  realtype *, realtype *, realtype *, realtype *, int *, int *);
int PT_DECLSPEC sosGetSectionResponseFlag(PT_HANDLE *, int, int *);
int PT_DECLSPEC sosGetSectionResponseFlagArray(PT_HANDLE *, int **);
int PT_DECLSPEC sosGetCenterFreqResponseArray(PT_HANDLE *, realtype **);
int PT_DECLSPEC sosGetCenterFreqArray(PT_HANDLE *, realtype **);
int PT_DECLSPEC sosGetCenterFreqIndexArray(PT_HANDLE *, int **);

/* sosProcessBuffer.cpp */
int PT_DECLSPEC sosProcessBuffer(PT_HANDLE *hp_sos, realtype *rp_in_buf, realtype *rp_out_buf, int i_num_sample_sets, int i_num_channels);
int PT_DECLSPEC sosProcessBufferNoBias(PT_HANDLE *hp_sos, realtype *rp_in_buf, realtype *rp_out_buf, int i_num_sample_sets, int i_num_channels);
int PT_DECLSPEC sosProcessSurroundBuffer(PT_HANDLE *hp_sos, realtype *rp_in_buf, realtype *rp_out_buf, int i_num_sample_sets, int i_num_channels);

#endif //_SOS_H
