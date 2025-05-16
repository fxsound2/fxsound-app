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

#include "codedefs.h"
#include "BinauralSyn.h"
#include "u_BinauralSyn.h"

// Global coefficients
extern float FrontNearCoeffs[]; // 44.1k hz coeffs
extern float FrontFarCoeffs[];
extern float RearNearCoeffs[];
extern float RearFarCoeffs[];
extern float SideNearCoeffs[];
extern float SideFarCoeffs[];

extern float FrontNearCoeffs48[];  // 48k hz coeffs
extern float FrontFarCoeffs48[];
extern float RearNearCoeffs48[];
extern float RearFarCoeffs48[];
extern float SideNearCoeffs48[];
extern float SideFarCoeffs48[];

/*
 * FUNCTION: BinauralSynSetCoeffs()
 * DESCRIPTION:
 *  Sets the filter coefficient values for the specified channels. Input coefficients are a stereo
 *  array of left/right coefficient pairs, i_num_coeffs specifies the number of pairs.
 *
 */
int PT_DECLSPEC BinauralSynSetCoeffs(PT_HANDLE *hp_BinauralSyn, int i_channels_to_set, realtype *rp_coeff_pairs, int i_num_coeffs, int i_samp_rate_flag)
{
	struct BinauralSynHdlType *cast_handle;
	int i;

	cast_handle = (struct BinauralSynHdlType *)(hp_BinauralSyn);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	if( (i_num_coeffs <= 0) || (i_num_coeffs > BINAURAL_SYN_MAX_NUM_COEFFS) )
		return(NOT_OKAY);

	switch ( i_channels_to_set )
	{
	case BINAURAL_SYN_FRONT_CHANNELS :
		for(i=0; i<i_num_coeffs; i++)
		{
			if( i_samp_rate_flag == BINAURAL_SYN_COEFF_SAMP_RATE_44_1 )
			{
				FrontNearCoeffs[i] = rp_coeff_pairs[i * 2];
				FrontFarCoeffs[i] = rp_coeff_pairs[i * 2 + 1];
			}
			else
			{
				FrontNearCoeffs48[i] = rp_coeff_pairs[i * 2];
				FrontFarCoeffs48[i] = rp_coeff_pairs[i * 2 + 1];
			}
		}
		break;

	case BINAURAL_SYN_REAR_CHANNELS :
		for(i=0; i<i_num_coeffs; i++)
		{
			if( i_samp_rate_flag == BINAURAL_SYN_COEFF_SAMP_RATE_44_1 )
			{
				RearNearCoeffs[i] = rp_coeff_pairs[i * 2];
				RearFarCoeffs[i] = rp_coeff_pairs[i * 2 + 1];
			}
			else
			{
				RearNearCoeffs48[i] = rp_coeff_pairs[i * 2];
				RearFarCoeffs48[i] = rp_coeff_pairs[i * 2 + 1];
			}
		}
		break;

	case BINAURAL_SYN_SIDE_CHANNELS :
		for(i=0; i<i_num_coeffs; i++)
		{
			if( i_samp_rate_flag == BINAURAL_SYN_COEFF_SAMP_RATE_44_1 )
			{
				SideNearCoeffs[i] = rp_coeff_pairs[i * 2];
				SideFarCoeffs[i] = rp_coeff_pairs[i * 2 + 1];
			}
			else
			{
				SideNearCoeffs48[i] = rp_coeff_pairs[i * 2];
				SideFarCoeffs48[i] = rp_coeff_pairs[i * 2 + 1];
			}
		}
		break;
	}

	return(OKAY);
}

/*
 * FUNCTION: BinauralSynSetMemoryToZero()
 * DESCRIPTION:
 *  Zeros the internal sample memory.  
 *
 */
int PT_DECLSPEC BinauralSynSetMemoryToZero(PT_HANDLE *hp_BinauralSyn)
{
	struct BinauralSynHdlType *cast_handle;
	int i;

	cast_handle = (struct BinauralSynHdlType *)(hp_BinauralSyn);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	// Zero the sample memory
	for(i=0; i<cast_handle->num_coeffs; i++)
	{
		cast_handle->LFsamples[i] = (realtype)0.0;
		cast_handle->RFsamples[i] = (realtype)0.0;
		cast_handle->LRsamples[i] = (realtype)0.0;
		cast_handle->RRsamples[i] = (realtype)0.0;
		cast_handle->LSsamples[i] = (realtype)0.0;
		cast_handle->RSsamples[i] = (realtype)0.0;
	}

	cast_handle->sample_index = 0;

	return(OKAY);
}
