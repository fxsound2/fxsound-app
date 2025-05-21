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

#include "codedefs.h"
#include "BinauralSyn.h"
#include "u_BinauralSyn.h"

//#define BINAURAL_SYN_FILE_BASED_COEFFS  // Only to be used for development and debugging, loads coeffs from files.
#define BINAURAL_SYN_1057R_BASED_COEFFS	// Used for release versions, declares and inits coeff values in code.

#ifdef BINAURAL_SYN_FILE_BASED_COEFFS
float FrontNearCoeffs[128];	// For the default 44.1khz sample rate
float FrontFarCoeffs[128];
float RearNearCoeffs[128];
float RearFarCoeffs[128];
float SideNearCoeffs[128];
float SideFarCoeffs[128];

float FrontNearCoeffs48[128];	// For the 48khz sample rate
float FrontFarCoeffs48[128];
float RearNearCoeffs48[128];
float RearFarCoeffs48[128];
float SideNearCoeffs48[128];
float SideFarCoeffs48[128];

#endif // BINAURAL_SYN_FILE_BASED_COEFFS

// These includes define and initialize the coeff arrays shown above
#ifdef BINAURAL_SYN_1057R_BASED_COEFFS
// For the default 44.1khz sample rate
#include "IRC_1057_R_R0195_T015_P015TrimComp2DC.h"		// Front coeffs declaration and initiation
#include "IRC_1057_R_R0195_T090_P000TrimComp2.h"		// Side coeffs declaration and initiation
#include "IRC_1057_R_R0195_T165_P000TrimComp2.h"		// Rear coeffs declaration and initiation

// For the 48khz sample rate
#include "IRC_1057_R_R0195_T015_P015TrimComp2DC_48.h"		// Front coeffs declaration and initiation
#include "IRC_1057_R_R0195_T090_P000TrimComp2_48.h"		// Side coeffs declaration and initiation
#include "IRC_1057_R_R0195_T165_P000TrimComp2_48.h"		// Rear coeffs declaration and initiation
#endif // BINAURAL_SYN_FILE_BASED_COEFFS

/*
 * FUNCTION: BinauralSynProcessSurroundFormatWindowsOrdering()
 * DESCRIPTION:
 *  This function processes a buffer of 6 or 8 channel signal sets.
 *  Processing can be performed in-place by passing the same arrays in for input and output.
 *  It implements "crossover" filtering as shown below, assuming that the coeffs are designed
 *  to place the left channel and are swapped over to place the right channel:
 *  Left Out  = (Left In)  * NearCoeffs + (Right In) * FarCoeffs
 *  Right Out = (Right In) * NearCoeffs + (Left In)  * FarCoeffs
 *  Ordering for 5.1 is: Front Left, Front Right, Front Center, Low Frequency, Back Left, Back Right
 *  Ordering for 7.1 is: Front Left, Front Right, Front Center, Low Frequency, Back Left, Back Right, Side Left, Side Right
 */
int PT_DECLSPEC BinauralSynProcessSurroundFormatWindowsOrdering(PT_HANDLE *hp_BinauralSyn,
							int i_nchans,
							int i_samp_freq,
							realtype *rp_input,
							int i_num_sample_sets,
							realtype *rp_output)
{
	int i, j;
	int max;
	int index;
	realtype *LFsamples;
	realtype *RFsamples;
	realtype *LRsamples;
	realtype *RRsamples;
	realtype *LSsamples;
	realtype *RSsamples;
	realtype left_out, right_out;
	int mode_7_1;
	realtype sub;
	realtype center;
	int s_ratio;
	int s_index;
	int internal_samp_rate_flag;

	struct BinauralSynHdlType *cast_handle;

	cast_handle = (struct BinauralSynHdlType *)(hp_BinauralSyn);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	if( (i_nchans != 6) &&  (i_nchans != 8) )
		return(NOT_OKAY);

	// PTNOTE - for now, no processing is performed for samp freqs above 48khz.
	if( i_samp_freq > BINAURAL_SYN_MAX_SAMP_FREQ )
		return(OKAY);

	if( i_samp_freq < BINAURAL_SYN_MIN_SAMP_FREQ ) // No processing is performed if below min samp freq.
		return(OKAY);

	if( i_samp_freq != cast_handle->last_samp_freq )
	{
		cast_handle->last_left = (realtype)0.0;
		cast_handle->last_right = (realtype)0.0;
		cast_handle->s_ratio = 1;
		cast_handle->s_index = 0;
		cast_handle->sample_index = 0;
		cast_handle->last_samp_freq = i_samp_freq;
		cast_handle->internal_samp_rate_flag = (int)BINAURAL_SYN_COEFF_SAMP_RATE_44_1;

		if( i_samp_freq > BINAURAL_SYN_MAX_SAMP_FREQ )
			cast_handle->s_ratio = i_samp_freq/(int)BINAURAL_SYN_DEFAULT_SAMP_FREQ; // Note this rounds down as desired.

		// Check to see if sample rate is an integer multiple of 44.1hz or 48khz.
		if( ((float)i_samp_freq / (float)((int)44100 * cast_handle->s_ratio)) > (float)1.02 )
			cast_handle->internal_samp_rate_flag = (int)BINAURAL_SYN_COEFF_SAMP_RATE_48;

		for(i=0; i<BINAURAL_SYN_MAX_NUM_COEFFS; i++)
		{
			cast_handle->LFsamples[i] = (realtype)0.0;
			cast_handle->RFsamples[i] = (realtype)0.0;
			cast_handle->LRsamples[i] = (realtype)0.0;
			cast_handle->RRsamples[i] = (realtype)0.0;
			cast_handle->LSsamples[i] = (realtype)0.0;
			cast_handle->RSsamples[i] = (realtype)0.0;
		}
	}

	mode_7_1 = 0;
	if( i_nchans == 8)
		mode_7_1 = 1;

	max = cast_handle->num_coeffs;

	LFsamples = cast_handle->LFsamples;
	RFsamples = cast_handle->RFsamples;
	LRsamples = cast_handle->LRsamples;
	RRsamples = cast_handle->RRsamples;
	LSsamples = cast_handle->LSsamples;
	RSsamples = cast_handle->RSsamples;

	index = cast_handle->sample_index;
	s_ratio = cast_handle->s_ratio;
	s_index = cast_handle->s_index;
	internal_samp_rate_flag = cast_handle->internal_samp_rate_flag;

	// Note that s_index controls sample skipping and filling and maintains state until next buffer
	// so odd number of sample sets are handled correctly.
	for( j=0; j < (i_num_sample_sets * i_nchans); j += i_nchans )
	{
		if( s_index != 0 ) // Check to see if to process or skip and fill
		{
			// Note in this mode only stereo channels are output, others are set to zero.
			rp_output[j]	  = cast_handle->last_left;
			rp_output[j + 1] = cast_handle->last_right;
			rp_output[j + 2] = (realtype)0.0;
			rp_output[j + 3] = (realtype)0.0;
			rp_output[j + 4] = (realtype)0.0;
			rp_output[j + 5] = (realtype)0.0;

			if( mode_7_1 ) // Only do side channels for 7.1 case
			{
				rp_output[j + 6] = (realtype)0.0;
				rp_output[j + 7] = (realtype)0.0;
			}

			s_index++;
			if( s_index >= s_ratio )
				s_index = 0;
		}
		else // Process
		{
			s_index++;
			left_out = (realtype)0.0;
			right_out = (realtype)0.0;

			// Update circular buffers, assumes index is pointing to next input location
			// Buffer is organized with sample age increasing with index increases.
			LFsamples[index] = rp_input[j];			// Front channels
			RFsamples[index] = rp_input[j + 1];

			center = rp_input[j + 2];
			sub	 = rp_input[j + 3];

			LRsamples[index] = rp_input[j + 4];	// Rear channels
			RRsamples[index] = rp_input[j + 5];

			if( mode_7_1 ) // Only do side channels for 7.1 case
			{
				LSsamples[index] = rp_input[j + 6];
				RSsamples[index] = rp_input[j + 7];
			}

			// Do convolution, use either 44.1 or 48khz coeffs
			if( internal_samp_rate_flag == BINAURAL_SYN_COEFF_SAMP_RATE_44_1 )
				for(i=0; i<max; i++) // 44.1khz case
				{
					left_out  += LFsamples[index] * FrontNearCoeffs[i] + RFsamples[index] * FrontFarCoeffs[i];
					left_out  += LRsamples[index] * RearNearCoeffs[i]  + RRsamples[index] * RearFarCoeffs[i];

					right_out += RFsamples[index] * FrontNearCoeffs[i] + LFsamples[index] * FrontFarCoeffs[i];
					right_out += RRsamples[index] * RearNearCoeffs[i]  + LRsamples[index] * RearFarCoeffs[i];

					if( mode_7_1)
					{
						left_out  += LSsamples[index] * SideNearCoeffs[i] + RSsamples[index] * SideFarCoeffs[i];
						right_out += RSsamples[index] * SideNearCoeffs[i] + LSsamples[index] * SideFarCoeffs[i];
					}

					index++;

					// Note that this method only uses the amount of sample buffer equal to the coeff size.
					if(index >= max)
						index = 0;
				}
			else
				for(i=0; i<max; i++) // 48khz case
				{
					left_out  += LFsamples[index] * FrontNearCoeffs48[i] + RFsamples[index] * FrontFarCoeffs48[i];
					left_out  += LRsamples[index] * RearNearCoeffs48[i]  + RRsamples[index] * RearFarCoeffs48[i];

					right_out += RFsamples[index] * FrontNearCoeffs48[i] + LFsamples[index] * FrontFarCoeffs48[i];
					right_out += RRsamples[index] * RearNearCoeffs48[i]  + LRsamples[index] * RearFarCoeffs48[i];

					if( mode_7_1)
					{
						left_out  += LSsamples[index] * SideNearCoeffs48[i] + RSsamples[index] * SideFarCoeffs48[i];
						right_out += RSsamples[index] * SideNearCoeffs48[i] + LSsamples[index] * SideFarCoeffs48[i];
					}

					index++;

					// Note that this method only uses the amount of sample buffer equal to the coeff size.
					if(index >= max)
						index = 0;
				}

			// Set index pointing to the oldest current sample set, which is where the next new sample set will go.
			index--;

			if(index < 0)
				index = max - 1;

			// Note in this mode only stereo channels are output, others are set to zero.
			rp_output[j]	  = left_out  + (realtype)0.5 * (center + sub);
			rp_output[j + 1] = right_out + (realtype)0.5 * (center + sub);

			cast_handle->last_left = rp_output[j];
			cast_handle->last_right = rp_output[j + 1];

			rp_output[j + 2] = (realtype)0.0;
			rp_output[j + 3] = (realtype)0.0;
			rp_output[j + 4] = (realtype)0.0;
			rp_output[j + 5] = (realtype)0.0;

			if( mode_7_1 ) // Only do side channels for 7.1 case
			{
				rp_output[j + 6] = (realtype)0.0;
				rp_output[j + 7] = (realtype)0.0;
			}
		}
	}

	cast_handle->sample_index = index;
	cast_handle->s_ratio = s_ratio;
	cast_handle->s_index = s_index;	
	cast_handle->internal_samp_rate_flag = internal_samp_rate_flag;

	return(OKAY);
}

/*
 * FUNCTION: BinauralSynProcessStereoFormat()
 * DESCRIPTION:
 *  This function processes a buffer of stereo signal sets.
 *  Processing can be performed in-place by passing the same arrays in for input and output.
 *  It implements "crossover" filtering as shown below, assuming that the coeffs are designed
 *  to place the left channel and are swapped over to place the right channel:
 *  Left Out  = (Left In)  * NearCoeffs + (Right In) * FarCoeffs
 *  Right Out = (Right In) * NearCoeffs + (Left In)  * FarCoeffs
 */
int PT_DECLSPEC BinauralSynProcessStereoFormat(PT_HANDLE *hp_BinauralSyn,
							realtype *rp_stereo_in,
							int i_samp_freq,
							int i_num_sample_sets,
							realtype *rp_stereo_out)
{
	int i, j;
	int max;
	int index;
	realtype *Lsamples;
	realtype *Rsamples;
	realtype left_out, right_out;
	int s_ratio;
	int s_index;
	int internal_samp_rate_flag;

	struct BinauralSynHdlType *cast_handle;

	cast_handle = (struct BinauralSynHdlType *)(hp_BinauralSyn);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	// PTNOTE - for now, no processing is performed for samp freqs above 48khz.
	if( i_samp_freq > BINAURAL_SYN_MAX_SAMP_FREQ )
		return(OKAY);

	if( i_samp_freq < BINAURAL_SYN_MIN_SAMP_FREQ ) // No processing is performed if below min samp freq.
		return(OKAY);

	if( i_samp_freq != cast_handle->last_samp_freq )
	{
		cast_handle->last_left = (realtype)0.0;
		cast_handle->last_right = (realtype)0.0;
		cast_handle->s_ratio = 1;
		cast_handle->s_index = 0;
		cast_handle->sample_index = 0;
		cast_handle->last_samp_freq = i_samp_freq;
		cast_handle->internal_samp_rate_flag = (int)BINAURAL_SYN_COEFF_SAMP_RATE_44_1;

		if( i_samp_freq > BINAURAL_SYN_MAX_SAMP_FREQ )
			cast_handle->s_ratio = i_samp_freq/(int)BINAURAL_SYN_DEFAULT_SAMP_FREQ; // Note this rounds down as desired.

		// Check to see if sample rate is an integer multiple of 44.1hz or 48khz.
		if( ((float)i_samp_freq / (float)((int)44100 * cast_handle->s_ratio)) > (float)1.02 )
			cast_handle->internal_samp_rate_flag = (int)BINAURAL_SYN_COEFF_SAMP_RATE_48;

		for(i=0; i<BINAURAL_SYN_MAX_NUM_COEFFS; i++)
		{
			cast_handle->LFsamples[i] = (realtype)0.0;
			cast_handle->RFsamples[i] = (realtype)0.0;
		}
	}

	max = cast_handle->num_coeffs;
	Lsamples = cast_handle->LFsamples;
	Rsamples = cast_handle->RFsamples;
	index = cast_handle->sample_index;
	s_ratio = cast_handle->s_ratio;
	s_index = cast_handle->s_index;
	internal_samp_rate_flag = cast_handle->internal_samp_rate_flag;

	// Note that s_index controls sample skipping and filling and maintains state until next buffer
	// so odd number of sample sets are handled correctly.
	for(j=0; j< i_num_sample_sets * 2; j += 2)
	{
		if( s_index != 0 ) // Check to see if to process or skip and fill
		{
			rp_stereo_out[j]		= cast_handle->last_left;
			rp_stereo_out[j + 1] = cast_handle->last_right;

			s_index++;
			if( s_index >= s_ratio )
				s_index = 0;
		}
		else // Process
		{
			s_index++;
			left_out = (realtype)0.0;
			right_out = (realtype)0.0;

			// Update circular buffer, assumes index is pointing to next input location
			// Buffer is organized with sample age increasing with index increases.
			Lsamples[index] = rp_stereo_in[j];
			Rsamples[index] = rp_stereo_in[j + 1];

			// Do convolution, use either 44.1 or 48khz coeffs
			if( internal_samp_rate_flag == BINAURAL_SYN_COEFF_SAMP_RATE_44_1 )
				for(i=0; i<max; i++)	// 44.1khz case
				{
					left_out  += Lsamples[index] * FrontNearCoeffs[i] + Rsamples[index] * FrontFarCoeffs[i];

					right_out += Rsamples[index] * FrontNearCoeffs[i] + Lsamples[index] * FrontFarCoeffs[i];

					index++;

					// Note that this method only uses the amount of sample buffer equal to the coeff size.
					if(index >= max)
						index = 0;
				}
			else
				for(i=0; i<max; i++)	// 48khz case
				{
					left_out  += Lsamples[index] * FrontNearCoeffs48[i] + Rsamples[index] * FrontFarCoeffs48[i];

					right_out += Rsamples[index] * FrontNearCoeffs48[i] + Lsamples[index] * FrontFarCoeffs48[i];

					index++;

					// Note that this method only uses the amount of sample buffer equal to the coeff size.
					if(index >= max)
						index = 0;
				}

			// Set index pointing to the oldest current sample pair, which is where the next new sample pair will go.
			index--;

			if(index < 0)
				index = max - 1;

			rp_stereo_out[j] = left_out;
			rp_stereo_out[j + 1] = right_out;

			cast_handle->last_left	= left_out;
			cast_handle->last_right = right_out;
		}
	}

	cast_handle->sample_index = index;
	cast_handle->s_ratio = s_ratio;
	cast_handle->s_index = s_index;
	cast_handle->internal_samp_rate_flag = internal_samp_rate_flag;

	return(OKAY);
}
