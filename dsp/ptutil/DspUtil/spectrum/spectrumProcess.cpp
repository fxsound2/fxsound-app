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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "codedefs.h"
#include "spectrum.h"
#include "u_spectrum.h"

/*
 * FUNCTION: spectrumProcess()
 * DESCRIPTION:
 *  Calculates the spectrum values for the passed buffer.
 */
int PT_DECLSPEC spectrumProcess(PT_HANDLE *hp_spectrum,
							/* Input Signal Info */
							realtype *rp_signal,		/* Input signal, points interleaved */
							int i_num_sample_sets,  /* Number of mono sample points or stereo sample pairs */
							int i_num_channels,     /* 1 for mono, 2 for stereo */
							realtype r_samp_freq,   /* Sampling frequency in hz. */
							int i_processing_on     /* Flag stating if processing is currently on */
					 	)
{
	struct spectrumHdlType *cast_handle;
	int i, j;
	int delay_index;
	int loop_inc;

	cast_handle = (struct spectrumHdlType *)(hp_spectrum);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	// NOTE - since the DFX processing does internal processing at 48khz max, internal sampling frequency here will
	// also be limited to those values in the call to spectrumReset
	if( (r_samp_freq != cast_handle->samp_freq)  || (i_num_channels != cast_handle->num_channels) )
	{
		cast_handle->samp_freq = r_samp_freq;
		cast_handle->num_channels = i_num_channels;
		if( spectrumReset( hp_spectrum ) != OKAY )
			return(NOT_OKAY);
	}

	// Increment thru data based on current sampling frequency determined data reduction rate
	loop_inc = cast_handle->internal_rate_ratio;

	for(i=0; i<i_num_sample_sets; i += loop_inc)
	{
		realtype in;
		realtype input_sum;

		if(i_num_channels == 2)
			in = rp_signal[i * 2] + rp_signal[i * 2 + 1];
		else
			in = rp_signal[i];

		if (i_processing_on)
		{
			// Note all filters use same in and in_2, filter needs them as a difference (see design notes)
			input_sum = in - cast_handle->in_2;
		}
		else
		{
			// If processing is currently off, then set input to filter to 0.0
			input_sum = (realtype)0.0;
		}

		for(j=0; j<SPECTRUM_MAX_NUM_BANDS; j++)
			spectrum_UpdateFilter(input_sum, &(cast_handle->sFilt[j]),
			                      cast_handle->alpha, cast_handle->one_minus_alpha );

		// Shift input signals
		cast_handle->in_2 = cast_handle->in_1;
		cast_handle->in_1 = in;
	}

	// Check to see if its time to do a buffer update cycle
	// On average this will be done at the period set by cast_handle->
	cast_handle->time_since_last_buffer_store += cast_handle->internal_samp_period * i_num_sample_sets/loop_inc;

	if( cast_handle->time_since_last_buffer_store >= cast_handle->refresh_rate_secs )
	{
		cast_handle->time_since_last_buffer_store -= cast_handle->refresh_rate_secs;

		// Store band values in delay buffer
		for(i=0; i<SPECTRUM_MAX_NUM_BANDS; i++)
			cast_handle->band_buf[ cast_handle->buffer_index + i ] = cast_handle->sFilt[i].level;

		delay_index = (cast_handle->buffer_index - cast_handle->delay_count * SPECTRUM_MAX_NUM_BANDS);

		if(delay_index < 0)
			delay_index += SPECTRUM_MAX_NUM_BANDS * (int)SPECTRUM_MAX_BAND_BUFFER_LEN;

		// Protect against index still being too small
		if(delay_index < 0)
			delay_index = 0;

		// If processing is off, zero all buffer values behind just stored values
		if( !i_processing_on )
		{
			int next_buffer_index;

			next_buffer_index = cast_handle->buffer_index + SPECTRUM_MAX_NUM_BANDS;

			if( cast_handle->buffer_index > delay_index )
			{
				// Check to see if only one set remains at end of buffer
				if( next_buffer_index < (int)(SPECTRUM_MAX_BAND_BUFFER_LEN) * SPECTRUM_MAX_NUM_BANDS )
				{
					// Zero values ahead of buffer_index
					for( i= next_buffer_index; i<(int)(SPECTRUM_MAX_BAND_BUFFER_LEN) * SPECTRUM_MAX_NUM_BANDS; i++)
						cast_handle->band_values[i] = 0;
				}

				// Zero values behind delay_index
				for(i=0; i<delay_index; i++)
					cast_handle->band_values[i] = 0;
			}
			else
			{
				// Zero values between buffer_index and delay_index
				if( next_buffer_index < (int)(SPECTRUM_MAX_BAND_BUFFER_LEN) * SPECTRUM_MAX_NUM_BANDS )
					for(i=next_buffer_index; i<delay_index; i++)
						cast_handle->band_values[i] = 0;
			}
		}

		// Assign delayed band values
		for(i=0; i<SPECTRUM_MAX_NUM_BANDS; i++)
			cast_handle->band_values[i] = cast_handle->band_buf[delay_index + i];

		cast_handle->buffer_index += SPECTRUM_MAX_NUM_BANDS;
		if(cast_handle->buffer_index >= ( (int)(SPECTRUM_MAX_BAND_BUFFER_LEN) * SPECTRUM_MAX_NUM_BANDS ) )
			cast_handle->buffer_index = 0;
	}

	return(OKAY);
}

inline void spectrum_UpdateFilter(realtype r_input_sum, struct spectrumFiltType *sFilt, realtype r_alpha, realtype r_one_minus_alpha)
{
	realtype tmp;

	// Implements a simple 2 pole resonant filter
	// Include bias to eliminate Intel underflow problems.
	sFilt->out = r_input_sum + sFilt->a1 * sFilt->y1 + sFilt->a2 * sFilt->y2 + (float)1.0e-5;
	sFilt->y2 = sFilt->y1;
	sFilt->y1 = sFilt->out;
	sFilt->out *= sFilt->gain;

	tmp = sFilt->out * sFilt->out;

	// Bias above appears to eliminate the need for bias on the next calculation
	// sFilt->squared_filtered = r_one_minus_alpha * tmp + r_alpha * sFilt->squared_filtered + (float)1.0e-5;
	sFilt->squared_filtered = r_one_minus_alpha * tmp + r_alpha * sFilt->squared_filtered;

	if( sFilt->squared_filtered > (realtype)SPECTRUM_MAX_OUTPUT_VALUE )
		sFilt->level = (realtype)SPECTRUM_MAX_OUTPUT_VALUE;
	else
	{
		// Using an inlined version of a square root approximation from the mth module.
		// This approximation can improve performance as much as 10 times for signals smaller than zero.
		// Error is typically no more than 6%
        union
        {
                int tmp;
                float x;
        } u;
        u.x = sFilt->squared_filtered;
        u.tmp -= 1<<23; /* Remove last bit so 1.0 gives 1.0 */
        /* tmp is now an approximation to logbase2(r_x) */
        u.tmp >>= 1; /* divide by 2 */
        u.tmp += 1<<29; /* add 64 to exponent: (e+127)/2 =(e/2)+63, */
        /* that represents (e/2)-64 but we want e/2 */
        sFilt->level = u.x;
	}
}

void spectrum_ResetFilter(struct spectrumFiltType *sFilt)
{
	sFilt->out = (realtype)0.0;
	sFilt->y2 = (realtype)0.0;
	sFilt->y1 = (realtype)0.0;
	sFilt->level = (realtype)0.0;
	sFilt->squared_filtered = (realtype)0.0;
}
