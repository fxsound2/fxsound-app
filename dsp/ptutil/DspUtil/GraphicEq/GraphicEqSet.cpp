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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "filt.h"
#include "codedefs.h"
#include "GraphicEq.h"
#include "u_GraphicEq.h"

#include "dfxpDefs.h"

void PT_DECLSPEC GraphicEqSetBalance(PT_HANDLE* hp_GraphicEq, float balance_db)
{
	struct GraphicEqHdlType* cast_handle;
	cast_handle = (struct GraphicEqHdlType*)(hp_GraphicEq);
	if (cast_handle == NULL)
		return;

	cast_handle->balance = balance_db;

	float balance_left = 1.0f; 
	float balance_right = 1.0f;

	if (balance_db > 0.0f)
	{
		balance_left = powf(10.0f, -balance_db / 20.0f);
	}
	else if (balance_db < 0.0f)
	{
		balance_right = powf(10.0f, balance_db / 20.0f);
	}

	sosSetBalance((PT_HANDLE*)(cast_handle->sos_hdl), balance_left, balance_right);
}

void PT_DECLSPEC GraphicEqSetNormalization(PT_HANDLE* hp_GraphicEq, float gain_db)
{
	struct GraphicEqHdlType* cast_handle;
	cast_handle = (struct GraphicEqHdlType*)(hp_GraphicEq);
	if (cast_handle == NULL)
		return;

	cast_handle->normalization_gain = gain_db;

	float normalization = powf(10.0f, gain_db / 20.0f);
	
	sosSetNormalization((PT_HANDLE*)(cast_handle->sos_hdl), normalization);
}

void PT_DECLSPEC GraphicEqSetMasterGain(PT_HANDLE* hp_GraphicEq, float gain_db)
{
	struct GraphicEqHdlType* cast_handle;
	cast_handle = (struct GraphicEqHdlType*)(hp_GraphicEq);
	if (cast_handle == NULL)
		return;

	cast_handle->master_gain = gain_db;

	float master_gain = powf(10.0f, gain_db / 20.0f);
	
	sosSetMasterGain((PT_HANDLE*)(cast_handle->sos_hdl), master_gain);
}

void PT_DECLSPEC GraphicEqSetFilterQ(PT_HANDLE* hp_GraphicEq, float q_multiplier)
{
	struct GraphicEqHdlType* cast_handle;
	cast_handle = (struct GraphicEqHdlType*)(hp_GraphicEq);
	if (cast_handle == NULL)
		return;

	cast_handle->Q_multiplier = q_multiplier;

	GraphicEq_InitSections(hp_GraphicEq);
}

int PT_DECLSPEC GraphicEqSetNumBands(PT_HANDLE* hp_GraphicEq, int num_bands)
{
	struct GraphicEqHdlType* cast_handle;

	cast_handle = (struct GraphicEqHdlType*)(hp_GraphicEq);	

	if (cast_handle == NULL)
		return(NOT_OKAY);

	// Check that the number of bands is valid
	if (num_bands < 1 || num_bands > 31)  // Assuming a maximum limit of 31 bands
		return(NOT_OKAY);

	// If the number of bands hasn't changed, do nothing
	if (cast_handle->num_bands == num_bands)
		return(OKAY);

	// Update the number of bands in the structure
	cast_handle->num_bands = num_bands;

	// Also update the global variable if needed
	DFXP_GRAPHIC_EQ_NUM_BANDS = num_bands;

	// Reinitialize SOS (Second Order Sections) with the new number of bands
	// Note: If sosSetNumSections doesn't exist, it might be necessary to recreate the SOS handle
	// or use a different function to update the number of sections

	// Recalculate and set the center frequencies of the bands
	// Use existing min/max values or default values
	realtype min_freq = cast_handle->min_band_freq;
	realtype max_freq = cast_handle->max_band_freq;

	// If no previous values were set, use defaults
	if (min_freq <= 0 || max_freq <= 0)
	{
		switch (num_bands)
		{
		case 10:
			min_freq = 31.25f;
			max_freq = 16000.0f;
			break;
		case 15:
			min_freq = 25.0f;
			max_freq = 16000.0f;
			break;
		case 20:
			min_freq = 20.0f;
			max_freq = 16000.0f;
			break;
		case 31:
			min_freq = 20.0f;
			max_freq = 20000.0f;
			break;
		default:
			min_freq = 20.0f;
			max_freq = 20000.0f;
			break;
		}
	}

	// Recalculate all band frequencies
	if (GraphicEqReSetAllBandFreqs(hp_GraphicEq, min_freq, max_freq) != OKAY)
		return(NOT_OKAY);

	// Reinitialize sections (this will call GraphicEq_InitSections internally)
	if (GraphicEq_InitSections != NULL)
	{
		if (GraphicEq_InitSections(hp_GraphicEq) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}


/*
 * FUNCTION: GraphicEqSetBandBoostCut()
 * DESCRIPTION:
 *  Sets the boost/cut setting in dB for the passed band.  
 *
 *  Note; The band_num starts at 1. (i.e. to set the first band use i_band_num = 1)								
 */
int PT_DECLSPEC GraphicEqSetBandBoostCut(PT_HANDLE *hp_GraphicEq, int i_band_num, realtype r_boost_cut)
{
	struct GraphicEqHdlType *cast_handle;
	realtype *rp_boost_array, *rp_freq_array;
	realtype band_freq;
	int i_section_num;
	//bool disable_band_1_flag;

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


	// Zero and turn off section if gain is 0 dB or if band freq is >= than 1/2 sampling freq 
	if( (r_boost_cut == (realtype)0.0) || ((band_freq * (realtype)2.0) >= cast_handle->sampling_freq) )
	{
		if( sosSetSectionUnityGain((PT_HANDLE *)(cast_handle->sos_hdl), i_section_num, 0) != OKAY )
			return(NOT_OKAY);
	 
		return(OKAY);
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

	/* Set center frequencies of sections */
	if( cast_handle->num_bands == 1 )
	{
		cast_handle->Q = (realtype)1.0;
		center_freq = r_min_band_freq;

		if( GraphicEqSetBandFreq(hp_GraphicEq, 1, center_freq) != OKAY )
			return(NOT_OKAY);
	}
	else
	{		
		// ==================================================================================
		//  Corrected band limits and center frequencies (before version 2.0 they was wrong) 
		// ==================================================================================

		// ---------------------------------------------- 5 Bands (Q = 1)
		// 62.5, 250, 1000, 4000, 16000
		//cast_handle->max_band_freq = 16000;
		//cast_handle->min_band_freq = 31.25;
		//cast_handle->num_bands = 10;
		// ---------------------------------------------- ISO 10 Bands (Q = 1.41421354)
		// 31.25, 62.5, 125, 250, 500, 1000, 2000, 4000, 8000, 16000
		//cast_handle->max_band_freq = 16000;
		//cast_handle->min_band_freq = 31.25;
		//cast_handle->num_bands = 10;
		// ---------------------------------------------- ISO 15 Bands (Q = 2.14757848)
		// 25, 40, 63, 100, 160, 250, 400, 630, 1000, 1600, 2500, 4000, 6300, 10000, 16000
		//cast_handle->max_band_freq = 16000;
		//cast_handle->min_band_freq = 25;
		//cast_handle->num_bands = 15;
		// ---------------------------------------------- ISO 20 Bands (Q = 2.82774258)
		// 20, 31.5, 40, 63, 80, 125, 160, 250, 315, 500, 
		// 630, 1000, 1250, 2000, 2500, 4000, 5000, 8000, 10000, 16000
		//cast_handle->max_band_freq = 16000;
		//cast_handle->min_band_freq = 20;
		//cast_handle->num_bands = 20;
		// ---------------------------------------------- ISO 31 Bands (Q = 4.33336544)
		// 20, 25, 31.5 40, 50, 63, 80, 100, 125, 160, 
		// 200, 250, 315, 400, 500, 630, 800, 1000, 1250, 1600,
		// 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000
		//cast_handle->max_band_freq = 20000;
		//cast_handle->min_band_freq = 20;
		//cast_handle->num_bands = 31;
	    // --------------------------------------------------------------------------

		if (cast_handle->num_bands == 5)
		{
			cast_handle->max_band_freq = 16000;
			cast_handle->min_band_freq = 62.5;
			realtype fCenter[] = { 62.5f, 250.0f, 1000.0f, 4000.0f, 16000.0f };
			for (int i = 0; i < cast_handle->num_bands; i++)
			{
				if (GraphicEqSetBandFreq(hp_GraphicEq, (i + 1), fCenter[i]) != OKAY)
					return(NOT_OKAY);
			}
		}
		else if (cast_handle->num_bands == 10)
		{
			cast_handle->max_band_freq = 16000;
			cast_handle->min_band_freq = 31.25;
			realtype fCenter[] = { 31.25f, 62.5f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f };
			for (int i = 0; i < cast_handle->num_bands; i++)
			{
				if (GraphicEqSetBandFreq(hp_GraphicEq, (i + 1), fCenter[i]) != OKAY)
					return(NOT_OKAY);
			}
		}
		else if (cast_handle->num_bands == 15)
		{
			cast_handle->max_band_freq = 16000;
			cast_handle->min_band_freq = 25;
			realtype fCenter[] = {   25.0f,   40.0f,   63.0f,   100.0f,   160.0f, 250.0f, 400.0f, 630.0f, 1000.0f, 1600.0f, 
				                   2500.0f, 4000.0f, 6300.0f, 10000.0f, 16000.0f };
			for (int i = 0; i < cast_handle->num_bands; i++)
			{
				if (GraphicEqSetBandFreq(hp_GraphicEq, (i + 1), fCenter[i]) != OKAY)
					return(NOT_OKAY);
			}
		}
		else if (cast_handle->num_bands == 20)
		{
			cast_handle->max_band_freq = 16000;
			cast_handle->min_band_freq = 20;
			realtype fCenter[] = {  20.0f,   31.5f,   40.0f,   63.0f,   80.0f,  125.0f,  160.0f,  250.0f,  315.0f,  500.0f, 
								   630.0f, 1000.0f, 1250.0f, 2000.0f, 2500.0f, 4000.0f, 5000.0f, 8000.0f, 10000.0f, 16000.0f };
			for (int i = 0; i < cast_handle->num_bands; i++)
			{
				if (GraphicEqSetBandFreq(hp_GraphicEq, (i + 1), fCenter[i]) != OKAY)
					return(NOT_OKAY);
			}
		}
		else if (cast_handle->num_bands == 31)
		{
			cast_handle->max_band_freq = 20000;
			cast_handle->min_band_freq = 20;
			realtype fCenter[] = {   20.0f,   25.0f,   31.5f,   40.0f,   50.0f,   63.0f,   80.0f,   100.0f,   125.0f,   160.0f, 
				                    200.0f,  250.0f,  315.0f,  400.0f,  500.0f,  630.0f,  800.0f,  1000.0f,  1250.0f,  1600.0f,
				                   2000.0f, 2500.0f, 3150.0f, 4000.0f, 5000.0f, 6300.0f, 8000.0f, 10000.0f, 12500.0f, 16000.0f, 20000.0f };
			for (int i = 0; i < cast_handle->num_bands; i++)
			{
				if (GraphicEqSetBandFreq(hp_GraphicEq, (i + 1), fCenter[i]) != OKAY)
					return(NOT_OKAY);
			}
		}
		else
		{
			d_min_freq = (double)cast_handle->min_band_freq;
			d_ratio = (double)cast_handle->max_band_freq / d_min_freq;

			for (i = 0; i < cast_handle->num_bands; i++)
			{
				d_power = (double)(i) / (double)(cast_handle->num_bands - 1);

				d_factor = pow(d_ratio, d_power);

				center_freq = (realtype)(d_min_freq * d_factor);

				if (GraphicEqSetBandFreq(hp_GraphicEq, (i + 1), center_freq) != OKAY)
					return(NOT_OKAY);
			}
		}


		// --------------------------------------------------------------------------
		//  Corrected the Q computing -  (before version 2.0 it was wrong)
		// --------------------------------------------------------------------------
		d_min_freq = (double)cast_handle->min_band_freq;
		d_ratio = (double)cast_handle->max_band_freq / d_min_freq;
		double r = pow(d_ratio, 1.0 / (cast_handle->num_bands - 1));
		cast_handle->Q = sqrt(r) / (r - 1.0);

		// ------------------------------------------------- Q_MULTIPLIER
		cast_handle->Q *= cast_handle->Q_multiplier;

		/* Limit Q to a minimum of 1.0 */
		if (cast_handle->Q < (realtype)1.0) cast_handle->Q = (realtype)1.0;

	}	
	return(OKAY);
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