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
#include <windows.h>

#include "codedefs.h"
#include "u_spectrum.h"

/*
 * FUNCTION: spectrumGetMessageValuesFromBandValues_NoHandle()
 * DESCRIPTION:
 *  Calculates the message values from the passed in band values.
 */
int PT_DECLSPEC spectrumGetMessageValuesFromBandValues_NoHandle(realtype *rp_band_values,
                                                int i_array_size,
																WPARAM *wp_band_bits,
																LPARAM *lp_band_bits)
{
	int i;
	int tmp[10];
	int true_silence_flag;
	realtype rtmp;

	if (rp_band_values == NULL)
		return(NOT_OKAY);

	// For now, hard coded to assume 10 band values, quantized as 0.0, 0.05, 0.1, ... 1.0, ie 21 values,
	// 5 bits per value. Put first 5 values in WPARAM, last 5 values in LPARAM.

	// Find int values for each band and check for true silence
	true_silence_flag = 1;
	for(i=0; i<10; i++)
	{
		rtmp = rp_band_values[i];

		tmp[i] = int((realtype)20.0 * rtmp + (realtype)0.5);

		if(rtmp != (realtype)0.0)
			true_silence_flag = 0;
	}

	// Init WPARAM to first band bits
	*wp_band_bits = tmp[0];

	// Shift bits and AND in each band
	for(i=1; i<5; i++)
	{
		*wp_band_bits <<= 5;
		*wp_band_bits ^= tmp[i];
	}

	// Init LPARAM to 6th band bits
	*lp_band_bits = tmp[5];

	// Shift bits and AND in each band
	for(i=6; i<10; i++)
	{
		*lp_band_bits <<= 5;
		*lp_band_bits ^= tmp[i];
	}

	// Set true silence flag by or-ing with bit in 26th position
	if( true_silence_flag )
		*lp_band_bits ^= SPECTRUM_TRUE_SILENCE_BIT_LOCATION;

	return(OKAY);
}

/*
 * FUNCTION: spectrumGetBandValuesFromMessageValues_NoHandle()
 * DESCRIPTION:
 *  Fills in the passed array with values from the compressed passed in words.
 *  Sets the true silence flag if the original band values were all exactly 0.0
 */
int PT_DECLSPEC spectrumGetBandValuesFromMessageValues_NoHandle(WPARAM w_band_bits,
																LPARAM l_band_bits,
							                           realtype *rp_band_values,
                                                int i_array_size,
																int *ip_true_silence_flag)
{
	int i;
	int tmp[10];
 
	if (rp_band_values == NULL)
		return(NOT_OKAY);

	*ip_true_silence_flag = 0;

	// For now, hard coded to assume 10 band values, quantized as 0.0, 0.05, 0.1, ... 1.0, ie 21 values,
	// 5 bits per value. Retrieve first 5 values from WPARAM, last 5 values from LPARAM.

	// Shift and mask w_band_bits to get correct band values
	for(i=0; i<5; i++)
	{
		tmp[i] = w_band_bits >> (5 * (4 - i));
		tmp[i] &= 31; 
	}

	// Shift and mask l_band_bits to get correct band values
	for(i=5; i<10; i++)
	{
		tmp[i] = l_band_bits >> (5 * (9 - i));
		tmp[i] &= 31; 
	}

	// Find real values for each band
	for(i=0; i<10; i++)
		rp_band_values[i] = (realtype)tmp[i] * (realtype)0.05;

	// Set true silence flag if true silence bit is set
	if( l_band_bits & SPECTRUM_TRUE_SILENCE_BIT_LOCATION )
		*ip_true_silence_flag = 1;

	return(OKAY);
}

