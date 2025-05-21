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

#include "filt.h"
#include "codedefs.h"
#include "GraphicEq.h"
#include "u_GraphicEq.h"

/*
 * FUNCTION: GraphicEqSetBandBoostCut()
 * DESCRIPTION:
 *  Sets the boost/cut setting in dB for the passed band.  
 *
 *  Note; The band_num starts at 1. (i.e. to set the first band use i_band_num = 1)
 * 
 *  Notes on frequency bands and Q with default end bands:
 *  16 band - freq ratio = 1.4473, Q = 1.954
 *  10 band - freq ratio = 1.852,  Q = 1.527
 *  8  band - freq ratio = 2.208,  Q = 1.281
 *  4  band - freq ratio = 6.350,  Q = 1.0 (limited)
 */
int PT_DECLSPEC GraphicEqSetBandBoostCut(PT_HANDLE *hp_GraphicEq, int i_band_num, realtype r_boost_cut)
{
	struct GraphicEqHdlType *cast_handle;
	realtype *rp_boost_array, *rp_freq_array;
	realtype band_freq;
	int i_section_num;
	bool disable_band_1_flag;

	cast_handle = (struct GraphicEqHdlType *)(hp_GraphicEq);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

   /* Make sure the band_num is legal */
	if ((i_band_num <= 0) || (i_band_num > cast_handle->num_bands)) 
		return(NOT_OKAY);

	i_section_num = i_band_num - 1;

	/* Get the section center freq and response arrays */
	if( sosGetCenterFreqArray((PT_HANDLE *)(cast_handle->sos_hdl), &rp_freq_array) != OKAY)
		return(NOT_OKAY);

	/* Get the section response array */
	if( sosGetCenterFreqResponseArray((PT_HANDLE *)(cast_handle->sos_hdl), &rp_boost_array) != OKAY)
		return(NOT_OKAY);

	band_freq = rp_freq_array[i_section_num];

	// Zero and turn off section if gain is 0 dB or if band freq is >= than 1/2 sampling freq or if this is Band1, it has positive boost and synced mod is on.
	if( (r_boost_cut == (realtype)0.0) || ((band_freq * (realtype)2.0) >= cast_handle->sampling_freq) )
			{
		if( sosSetSectionUnityGain((PT_HANDLE *)(cast_handle->sos_hdl), i_section_num, 0) != OKAY )
			return(NOT_OKAY);

		return(OKAY);
	}

	// Handle the Hyperbass and Band 1 synced case.
	if( cast_handle->app_has_hyperbass )
	{
		if(i_band_num == 1)
		{
			if( r_boost_cut > (realtype)0.0 )
				disable_band_1_flag = true;

			if( r_boost_cut < (realtype)0.0 )
				disable_band_1_flag = false;

			if( sosSetDisableBand1Flag((PT_HANDLE *)(cast_handle->sos_hdl), disable_band_1_flag) != OKAY )
				return(NOT_OKAY);
		}
	}

	/* We have a non-zero boost or cut, first limit it */
	if( r_boost_cut > GRAPHIC_EQ_DEFAULT_MAX_BOOST_OR_CUT )
		r_boost_cut = GRAPHIC_EQ_DEFAULT_MAX_BOOST_OR_CUT;

	if( r_boost_cut < -GRAPHIC_EQ_DEFAULT_MAX_BOOST_OR_CUT )
		r_boost_cut = -GRAPHIC_EQ_DEFAULT_MAX_BOOST_OR_CUT;

	/* If section boost/cut has changed, recalc the section */
	if( r_boost_cut != rp_boost_array[i_section_num] )
	{
		if( filtSosParametric((PT_HANDLE *)(cast_handle->sos_hdl), i_section_num, cast_handle->sampling_freq, 
                          rp_freq_array[i_section_num], r_boost_cut, cast_handle->Q) != OKAY )
			return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: GraphicEqReCalcAllBandCoeffs()
 * DESCRIPTION:
 *  Recalculates all band coeffs, typically called when sampling frequency changes.  
 *
 */
int PT_DECLSPEC GraphicEqReCalcAllBandCoeffs(PT_HANDLE *hp_GraphicEq)
{
	struct GraphicEqHdlType *cast_handle;
	realtype *rp_boost_array;
	realtype r_boost_cut;
	int num_bands;
	int i_section_num;

	cast_handle = (struct GraphicEqHdlType *)(hp_GraphicEq);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Get the section response array */
	if( sosGetCenterFreqResponseArray((PT_HANDLE *)(cast_handle->sos_hdl), &rp_boost_array) != OKAY)
		return(NOT_OKAY);

	/* Get the number of bands */
	num_bands = cast_handle->num_bands;

	for(i_section_num=0; i_section_num<num_bands; i_section_num++)
	{
		/* Copy and zero boost value to force a refresh of filter coeffs */
		r_boost_cut = rp_boost_array[i_section_num];
		rp_boost_array[i_section_num] = (realtype)0.0;

		if( GraphicEqSetBandBoostCut(hp_GraphicEq, (i_section_num + 1), r_boost_cut) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: GraphicEqReSetAllBandFreqs()
 * DESCRIPTION:
 *  Sets all band frequencies using exponential spacing so that each band
 *  has equal separation on an octave basis.  
 *
 *  Note; The band_num starts at 1. (i.e. to set the first band use i_band_num = 1)
 */
int PT_DECLSPEC GraphicEqReSetAllBandFreqs(PT_HANDLE *hp_GraphicEq, realtype r_min_band_freq, realtype r_max_band_freq)
{
	struct GraphicEqHdlType *cast_handle;
	realtype center_freq;
	double d_min_freq, d_ratio, d_power, d_factor;
	int i;

	cast_handle = (struct GraphicEqHdlType *)(hp_GraphicEq);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Check for reasonable band frequency values */
	if ( r_min_band_freq <= (realtype)GRAPHIC_EQ_MIN_BAND_FREQ )
		return(NOT_OKAY);
	if ( r_max_band_freq >= (realtype)GRAPHIC_EQ_MAX_BAND_FREQ )
		return(NOT_OKAY);
	if ( r_min_band_freq >= r_max_band_freq )
		return(NOT_OKAY);
	
	cast_handle->min_band_freq = r_min_band_freq;
	cast_handle->max_band_freq = r_max_band_freq;

	/* Set center frequencies of sections and overall Q setting */
	if( cast_handle->num_bands == 1 )
	{
		cast_handle->Q = (realtype)1.0;
		center_freq = r_min_band_freq;

		if( GraphicEqSetBandFreq(hp_GraphicEq, 1, center_freq) != OKAY )
			return(NOT_OKAY);
	}
	else
	{
		d_ratio = (double)cast_handle->max_band_freq/(double)cast_handle->min_band_freq;
		d_min_freq = (double)cast_handle->min_band_freq;

		/* Normalize Q based on ISO 10 band Q of 1.414213 */
		d_power = 1.0/(double)(cast_handle->num_bands - 1);
		cast_handle->Q = (realtype)( 1.414213 * 2.0/pow( d_ratio, d_power) );
		/* Limit Q to a minimum of 1.0 */
		if( cast_handle->Q < (realtype)1.0 )
			cast_handle->Q = (realtype)1.0;

		for(i=0; i<cast_handle->num_bands; i++)
		{
			d_power = (double)(i)/(double)(cast_handle->num_bands - 1);

			d_factor = pow( d_ratio, d_power);

			center_freq = (realtype)(d_min_freq * d_factor);

			if( GraphicEqSetBandFreq(hp_GraphicEq, (i + 1), center_freq) != OKAY )
				return(NOT_OKAY);
		}
	}	return(OKAY);
}

/*
 * FUNCTION: GraphicEqSetBandFreq()
 * DESCRIPTION:
 *  Sets an individual band frequency.
 *
 *  Note; The band_num starts at 1. (i.e. to set the first band use i_band_num = 1)
 */
int PT_DECLSPEC GraphicEqSetBandFreq(PT_HANDLE *hp_GraphicEq, int i_band_num, realtype r_band_freq)
{
	struct GraphicEqHdlType *cast_handle;
	realtype *rp_freq_array, *rp_boost_array;
	realtype original_boost;
	int i_section_num;

	cast_handle = (struct GraphicEqHdlType *)(hp_GraphicEq);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

  /* Make sure the band_num and frequency are legal */
	if ((i_band_num <= 0) || (i_band_num > cast_handle->num_bands)) 
		return(NOT_OKAY);

	i_section_num = i_band_num - 1;

	if (r_band_freq < GRAPHIC_EQ_MIN_BAND_FREQ)
		r_band_freq = GRAPHIC_EQ_MIN_BAND_FREQ;
		
	if (r_band_freq > GRAPHIC_EQ_MAX_BAND_FREQ)
		r_band_freq = GRAPHIC_EQ_MAX_BAND_FREQ;

	/* Get the section center freq and boost arrays */
	if( sosGetCenterFreqArray((PT_HANDLE *)(cast_handle->sos_hdl), &rp_freq_array) != OKAY)
		return(NOT_OKAY);

	if( sosGetCenterFreqResponseArray((PT_HANDLE *)(cast_handle->sos_hdl), &rp_boost_array) != OKAY)
		return(NOT_OKAY);

	rp_freq_array[i_section_num] = r_band_freq;
	
	/* Copy and zero original boost setting to force a refresh of filter coeffs */
	original_boost = rp_boost_array[i_section_num];
	rp_boost_array[i_section_num] = (realtype)0.0;

	if( GraphicEqSetBandBoostCut(hp_GraphicEq, i_band_num, original_boost) != OKAY )
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: GraphicEqSetAppHasHyperBassMode()
 * DESCRIPTION:
 *  Sets the mode for an app that has both DFX Hyperbass and EQ Band1 controls.
 *  When set forces a syncing and warping of the Hyperbass and Band1 controls as follows.
 *
 *  When the Hyperbass control is moved from settings of 0.0 to 10.0 the bass boost is implemented by Hyperbass (no Band1 processing)
 *  and the value of Band1 is set to match that value.
 *
 *  When the Band1 control is moved from settings of 0.0 to 10.0 the bass boost is implemented by Hyperbass (no Band1 processing)
 *  and the value of Hyperbass is set to match that value.
 *
 *  When the Band1 control is moved from settings of just above 10.0 to 12.0 the bass boost is implemented by Hyperbass with a fixed value of 10.0 (no Band1 processing)
 *  and the value of Hyperbass is set to 10.0 . The Band1 setting is the specific setting between 10.0 and 12.0 .
 *
 *  When the Band1 control is moved from settings of just below 0.0 to -12.0 the bass cut is implemented by Band1 (no Hyperbass processing)
 *  and the value of Hyperbass is set to 0.0 . The Band1 setting is the specific setting between 0.0 and -12.0 .
 *
 */
int PT_DECLSPEC GraphicEqSetAppHasHyperBassMode(PT_HANDLE *hp_GraphicEq, bool b_app_has_hyperbass)
{
	struct GraphicEqHdlType *cast_handle;

	cast_handle = (struct GraphicEqHdlType *)(hp_GraphicEq);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->app_has_hyperbass = b_app_has_hyperbass;

	return(OKAY);
}

int PT_DECLSPEC GraphicEqSetVolumeNormalization(PT_HANDLE *hp_GraphicEq, realtype r_target_rms)
{
	struct GraphicEqHdlType* cast_handle;

	cast_handle = (struct GraphicEqHdlType*)(hp_GraphicEq);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (sosSetVolumeNormalization((PT_HANDLE*)(cast_handle->sos_hdl), r_target_rms) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}