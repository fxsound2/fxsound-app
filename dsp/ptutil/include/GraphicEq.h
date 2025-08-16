/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
	www.theremino.com (2025)
	
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
#ifndef _GRAPHIC_EQ_H_
#define _GRAPHIC_EQ_H_

#include "codedefs.h"
#include "slout.h"

/* Defines */

// Default default minimum and maximum band frequencies in hz. */

// The standard ISO frequencies place octaves at
//  10 bands = 31.25, 62.5, 125, 250, 500, 1000, 2000, 4000, 8000, 16000
//  15 bands = 25, 40, 63, 100, 160, 250, 400, 630, 1000, 1600, 2500, 4000, 6300, 10000, 16000
//  31 Bands = 20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 
//             1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000

#define GRAPHIC_EQ_DEFAULT_FIRST_BAND_FREQ 20	
#define GRAPHIC_EQ_DEFAULT_LAST_BAND_FREQ 20000.0

#define GRAPHIC_EQ_DEFAULT_SAMPLING_FREQ 44100.0

/* Define lowest and highest band frequencies allowed */
#define GRAPHIC_EQ_MIN_BAND_FREQ 10		               
#define GRAPHIC_EQ_MAX_BAND_FREQ 21000.0

/* Maximum allowed Boost or Cut in dB */
#define GRAPHIC_EQ_DEFAULT_MAX_BOOST_OR_CUT 20.0

/* GraphicEqInit.cpp */
int PT_DECLSPEC GraphicEqNew(PT_HANDLE **hpp_GraphicEq, int i_type, int, CSlout *);
int PT_DECLSPEC GraphicEqFreeUp(PT_HANDLE **hpp_GraphicEq);

/* GraphicEqSetMasterGain GraphicEqSetFilterQ GraphicEqSetNumBands */
void PT_DECLSPEC GraphicEqSetMasterGain(PT_HANDLE* hp_GraphicEq, float gain_db);
void PT_DECLSPEC GraphicEqSetFilterQ(PT_HANDLE* hp_GraphicEq, float q_multiplier);
int PT_DECLSPEC GraphicEqSetNumBands(PT_HANDLE* hp_GraphicEq, int num_Bands);
void PT_DECLSPEC GraphicEqSetBalance(PT_HANDLE* hp_GraphicEq, float gain_db);
void PT_DECLSPEC GraphicEqSetNormalization(PT_HANDLE* hp_GraphicEq, float gain_db);

/* GraphicEqSet.cpp */
int PT_DECLSPEC GraphicEqSetBandBoostCut(PT_HANDLE *hp_GraphicEq, int i_band_num, realtype r_boost_cut);
int PT_DECLSPEC GraphicEqReCalcAllBandCoeffs(PT_HANDLE *hp_GraphicEq);
int PT_DECLSPEC GraphicEqReSetAllBandFreqs(PT_HANDLE *hp_GraphicEq, realtype r_min_band_freq, realtype r_max_band_freq);
int PT_DECLSPEC GraphicEqSetBandFreq(PT_HANDLE *hp_GraphicEq, int i_band_num, realtype r_band_freq);

/* GraphicEqGet.cpp */
int PT_DECLSPEC GraphicEqGetNumBands(PT_HANDLE *hp_GraphicEq, int *ip_num_bands);
int PT_DECLSPEC GraphicEqGetBandBoostCut(PT_HANDLE *hp_GraphicEq, int i_band_num, realtype *rp_boost_cut);
int PT_DECLSPEC GraphicEqGetBandCenterFrequency(PT_HANDLE *hp_GraphicEq, int i_band_num, float *fp_center_freq);
int PT_DECLSPEC GraphicEqGetBandFrequencyRange(PT_HANDLE *hp_GraphicEq, int i_band_num, float *fp_min_freq, float* fp_max_freq);
int PT_DECLSPEC GraphicEqGetMasterGain(PT_HANDLE* hp_GraphicEq, float* gain_db);
int PT_DECLSPEC GraphicEqGetNormalization(PT_HANDLE* hp_GraphicEq, float* gain_db);
int PT_DECLSPEC GraphicEqGetFilterQ(PT_HANDLE* hp_GraphicEq, float* q_multiplier);
int PT_DECLSPEC GraphicEqGetBalance(PT_HANDLE* hp_GraphicEq, float* balance_db);

/* GraphicEqProcess.cpp */
int PT_DECLSPEC GraphicEqProcess(PT_HANDLE *hp_GraphicEq,
								 realtype *rp_signal_in,	 /* Input signal, points interleaved */
								 realtype *rp_signal_out, /* Array to store the processed signal */
								 int i_num_sample_sets,   /* Number of mono sample points or stereo sample pairs */
								 int i_num_channels,      /* 1 for mono, 2 for stereo */
								 realtype r_samp_freq     /* Sampling frequency in hz. */
								);

int PT_DECLSPEC GraphicEqProcess_MasterGainOnly(PT_HANDLE* hp_GraphicEq,
												realtype* rp_signal_in,	 /* Input signal, points interleaved */
												realtype* rp_signal_out, /* Array to store the processed signal */
												int i_num_sample_sets,   /* Number of mono sample points or stereo sample pairs */
												int i_num_channels,      /* 1 for mono, 2 for stereo */
												realtype r_samp_freq     /* Sampling frequency in hz. */
											   );

#endif //_GRAPHIC_EQ_H
