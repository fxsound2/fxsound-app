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
#ifndef _PT_BINAURAL_SYN_H_
#define _PT_BINAURAL_SYN_H_

#include "codedefs.h"

/* Defines */
#define BINAURAL_SYN_MAX_SAMP_FREQ 48000
#define BINAURAL_SYN_MIN_SAMP_FREQ 32000
#define BINAURAL_SYN_DEFAULT_SAMP_FREQ 44100
#define BINAURAL_SYN_MAX_NUM_COEFFS 512
#define BINAURAL_SYN_DEFAULT_NUM_COEFFS 96

#define BINAURAL_SYN_FRONT_CHANNELS 1
#define BINAURAL_SYN_REAR_CHANNELS  2
#define BINAURAL_SYN_SIDE_CHANNELS  3

#define BINAURAL_SYN_COEFF_SAMP_RATE_44_1 0
#define BINAURAL_SYN_COEFF_SAMP_RATE_48	1

/* BinauralSynInit.cpp */
int PT_DECLSPEC BinauralSynNew(PT_HANDLE **hpp_BinauralSyn, int i_num_coeffs);
int PT_DECLSPEC BinauralSynFreeUp(PT_HANDLE **hpp_BinauralSyn);

/* BinauralSynSet.cpp */
int PT_DECLSPEC BinauralSynSetCoeffs(PT_HANDLE *hp_BinauralSyn, int i_channels_to_set, realtype *rp_coeff_pairs, int i_num_coeffs, int i_samp_rate_flag);
int PT_DECLSPEC BinauralSynSetMemoryToZero(PT_HANDLE *hp_BinauralSyn);

/* BinauralSynGet.cpp */
int PT_DECLSPEC BinauralSynGetNumCoeffs(PT_HANDLE *hp_BinauralSyn, int *ip_num_coeffs);

/* BinauralSynProcess.cpp */
/* Does a "crossover" convolution of a buffer of stereo signal sets. */
int PT_DECLSPEC BinauralSynProcessStereoFormat(PT_HANDLE *hp_BinauralSyn,
							realtype *rp_stereo_in,
							int i_samp_freq,
							int i_num_sample_sets,
							realtype *rp_stereo_out);
/* Full 6 or 8 channel surround processing */
int PT_DECLSPEC BinauralSynProcessSurroundFormatWindowsOrdering(PT_HANDLE *hp_BinauralSyn,
							int i_num_channels,
							int i_samp_freq,
							realtype *rp_input,
							int i_num_sample_sets,
							realtype *rp_output);

#endif //_PT_BINAURAL_SYN_H
