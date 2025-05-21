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
#ifndef _DFXP_H_
#define _DFXP_H_ 

#include "dfxpDefs.h"
#include "slout.h"

/* dfxpComm.cpp */
int dfxpCommunicateAll(PT_HANDLE *);
int dfxpCommunicateAllNonFixed(PT_HANDLE *, int);

/* dfxpEq */
int dfxpEqSetProcessingOn(PT_HANDLE *, int, int);
int dfxpEqGetProcessingOn(PT_HANDLE *, int, int *);
int dfxpEqSetEqType(PT_HANDLE *, int);
int dfxpEqGetBandCenterFrequency(PT_HANDLE *, int, realtype *);
int dfxpEqSetBandBoostCut(PT_HANDLE *, int, int, realtype);
int dfxpEqGetBandBoostCut_FromProcessing(PT_HANDLE *, int, realtype *, wchar_t *);
int dfxpEqGetBandBoostCut_FromRegistry(PT_HANDLE *, int, realtype *, wchar_t *);
int dfxpEqGetGraphicEqHdl(PT_HANDLE *, PT_HANDLE **);
int dfxpEqInitBand1SpecialCase(PT_HANDLE *);
int dfxpEqSetVolumeNormalization(PT_HANDLE *, realtype);

/* dfxpGet */
int dfxpGetKnobValue(PT_HANDLE *, int, float *);
int dfxpGetButtonValue(PT_HANDLE *, int, int *);
int dfxpGetSamplingFreq(PT_HANDLE *, realtype *);
int dfxpGetProcessingOverride(PT_HANDLE *, int *);
int dfxpGetFirstTimeRunFlag(PT_HANDLE *, int *);
int dfxpGetTemporaryBypassAll(PT_HANDLE *, int *);
int dfxpGetDfxTunedTrackPlaying(PT_HANDLE *, int *);
int dfxpSetDfxTunedTrackPlaying(PT_HANDLE *, int);
int dfxpGetTotalAudioProcessedTime(PT_HANDLE *, unsigned long *);

/* dfxpInit */
int dfxpInit(PT_HANDLE **, wchar_t *, int, int, int, int, int, long, int, int, int, CSlout *);

/* dfxpProcess */
int dfxpSetValidBits(PT_HANDLE *, int);
int dfxpModifyRealtypeSamples(PT_HANDLE *, realtype *, int, int);
int dfxpClearPreviousBufferedAudio(PT_HANDLE *);

/* dfxpQuit */
int dfxpQuit(PT_HANDLE **);

/* dfxpRegistryStandard */
int dfxpGetInstallationDate(PT_HANDLE *, long *, int *);
int dfxpGetLastUsedDate(PT_HANDLE *, long *, int *);
int dfxpGetInstallationDate_NoHandle(wchar_t *, int, long *, int *);

/* dfxpSet */
int dfxpSetKnobValue(PT_HANDLE *, int, float, bool);
int dfxpSetButtonValue(PT_HANDLE *, int, int);
int dfxpConvertFaderRealValueToInt(PT_HANDLE *, realtype, int *, int);
int dfxpSetProcessingOverride(PT_HANDLE *, int);
int dfxpSetTemporaryBypassAll(PT_HANDLE *, int);
int dfxpSetTotalAudioProcessedTime(PT_HANDLE *, unsigned long);

/* dfxpSharedMemory */
int dfxpSharedMemorySetFlag(PT_HANDLE *, int, int);
int dfxpSharedMemoryGetFlag(PT_HANDLE *, int, int *);
int dfxpSharedMemoryGetCurrentSpectrumValues(PT_HANDLE *, realtype *, int);
int dfxpSharedMemoryStoreCurrentSpectrumValues_FromArray(PT_HANDLE *, realtype *, int);

/* dfxpSpectrum */
int dfxpSpectrumSendClearValues(PT_HANDLE *);
int dfxpSpectrumGetBandValues(PT_HANDLE *, realtype *, int);


/* dfxpUniversal */
int dfxpUniversalInit(PT_HANDLE **, long, int, CSlout *);
int dfxpUniversalSetSignalFormat(PT_HANDLE *, int, int, int, int);
int dfxpUniversalModifySamples(PT_HANDLE *, short int *, short int *, int, int);
int dfxpUniversalCheckParentCompatibility(PT_HANDLE *, int, int *);

#endif //_DFXP_H_