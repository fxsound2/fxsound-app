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
#ifndef _U_SPECTRUM_H_
#define _U_SPECTRUM_H_

#include "spectrum.h"
#include "slout.h"
#include "sos.h"

// Controls the overall internal sensivity of the bands
#define SPECTRUM_SENSITIVITY_FACTOR 4.5

/* Frequency used to normalize settings for delays, etc. */
#define SPECTRUM_NORMALIZED_SAMP_FREQ			44100.0
#define SPECTRUM_MAXIMUM_SAMP_FREQ				192000.0
#define SPECTRUM_MAXIMUM_INTERNAL_SAMP_FREQ  48000.0

/* Bit location used for true silence flag in LPARAM message word */
#define SPECTRUM_TRUE_SILENCE_BIT_LOCATION 0x2000000

#define SPECTRUM_MIN_BUFFER_SIZE 128
#define SPECTRUM_MAX_BAND_BUFFER_LEN ( ((SPECTRUM_MAXIMUM_SAMP_FREQ * SPECTRUM_MAX_DELAY_SECS)/SPECTRUM_MIN_BUFFER_SIZE) + 1)

struct spectrumFiltType
{
	realtype a1;
	realtype a2;
	realtype gain;
	realtype y1;
	realtype y2;
	realtype out;
	realtype level;
	realtype squared_filtered;
};

/* Configuration Handle definition */
struct spectrumHdlType
{
	CSlout *slout_hdl;
   char msg1[1024]; /* String for messages */
	int trace_mode;

	struct spectrumFiltType sFilt[SPECTRUM_MAX_NUM_BANDS];

	int num_bands;
	int num_channels;
	realtype samp_freq;				// Passed in sampling frequency of audio data
	realtype internal_samp_freq;	// Actual internal sampling frequency used by DFX and Spectrum
	realtype internal_samp_period;
	int internal_rate_ratio;		// Ratio of internal sample freq to actual samp freq.

	// Controls the overall sensitivity of the spectrum displays
	realtype sensitivity;

	// Sets in seconds how much the spectrum is delayed to compensate for audio buffering
	realtype delay_secs;
	// Buffer count that implements the delay_secs
	int delay_count;

	int buffer_index;
	realtype refresh_rate_secs;
	realtype time_since_last_buffer_store;

	realtype band_values[SPECTRUM_MAX_NUM_BANDS];

	realtype band_buf[SPECTRUM_MAX_NUM_BANDS * (int)(SPECTRUM_MAX_BAND_BUFFER_LEN)];

	// Since filters run in parallel they can share common input vals
	realtype in_1, in_2;

	// For spectrum level filtering
	realtype time_constant;
	realtype alpha;
	realtype one_minus_alpha;
};

// Local functions
inline void spectrum_UpdateFilter(realtype r_input_sum, struct spectrumFiltType *sFilt, realtype r_alpha, realtype r_one_minus_alpha);
void spectrum_ResetFilter(struct spectrumFiltType *sFilt);

#endif /* _U_SPECTRUM_H_ */