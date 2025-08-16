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

#include "codedefs.h"
#include "u_GraphicEq.h"

/*
 * FUNCTION: GraphicEqGetNumBands()
 * DESCRIPTION:
 *  Gets the number of bands.
 */
int PT_DECLSPEC GraphicEqGetNumBands(PT_HANDLE *hp_GraphicEq, int *ip_num_bands)
{
	struct GraphicEqHdlType *cast_handle;

	cast_handle = (struct GraphicEqHdlType *)(hp_GraphicEq);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_num_bands = cast_handle->num_bands;

	return(OKAY);
}

/*
 * FUNCTION: GraphicEqGetBandBoostCut()
 * DESCRIPTION:
 *  Gets the boost/cut setting for the passed band.  
 *
 *  Note; The band_num starts at 1. (i.e. to get the first band use i_band_num = 1)
 */
int PT_DECLSPEC GraphicEqGetBandBoostCut(PT_HANDLE *hp_GraphicEq, int i_band_num, realtype *rp_boost_cut)
{
	struct GraphicEqHdlType *cast_handle;
	realtype *rp_boost_array;

	cast_handle = (struct GraphicEqHdlType *)(hp_GraphicEq);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

   /* Make sure the band_num is legal */
	if ((i_band_num <= 0) || (i_band_num > cast_handle->num_bands)) 
		return(NOT_OKAY);

	if( sosGetCenterFreqResponseArray((PT_HANDLE *)(cast_handle->sos_hdl), &rp_boost_array) != OKAY)
		return(NOT_OKAY);

	*rp_boost_cut = rp_boost_array[i_band_num - 1];

	return(OKAY);
}

/*
 * FUNCTION: GraphicEqGetBandCenterFrequency()
 * DESCRIPTION:
 *  Gets the center frequency setting for the passed band.  
 *
 *  Note; The band_num starts at 1. (i.e. to get the first band use i_band_num = 1)
 */
int PT_DECLSPEC GraphicEqGetBandCenterFrequency(PT_HANDLE *hp_GraphicEq, int i_band_num, realtype *rp_center_freq)
{
	struct GraphicEqHdlType *cast_handle;
	realtype *rp_freq_array;

	cast_handle = (struct GraphicEqHdlType *)(hp_GraphicEq);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

   /* Make sure the band_num is legal */
	if ((i_band_num <= 0) || (i_band_num > cast_handle->num_bands)) 
		return(NOT_OKAY);

		/* Get the section response array */
	if( sosGetCenterFreqArray((PT_HANDLE *)(cast_handle->sos_hdl), &rp_freq_array) != OKAY)
		return(NOT_OKAY);

	*rp_center_freq = rp_freq_array[i_band_num - 1];

	return(OKAY);
}

int PT_DECLSPEC GraphicEqGetBandFrequencyRange(PT_HANDLE *hp_GraphicEq, int i_band_num, float *fp_min_freq, float* fp_max_freq)
{
    struct GraphicEqHdlType *cast_handle;
    realtype *rp_freq_array;
    double d_ratio, d_power, d_factor;

    cast_handle = (struct GraphicEqHdlType *)(hp_GraphicEq);

    if (cast_handle == NULL)
        return(NOT_OKAY);

    /* Make sure the band_num is legal */
    if ((i_band_num <= 0) || (i_band_num > cast_handle->num_bands))
        return(NOT_OKAY);

    if (sosGetCenterFreqArray((PT_HANDLE *)(cast_handle->sos_hdl), &rp_freq_array) != OKAY)
        return(NOT_OKAY);

    // ---------------------------------------------------- Compute fp_min_freq fp_max_freq

    *fp_min_freq = rp_freq_array[i_band_num - 1] * 0.7;
    *fp_max_freq = rp_freq_array[i_band_num - 1] * 1.3;
    
    return(OKAY);
}

int PT_DECLSPEC GraphicEqGetMasterGain(PT_HANDLE* hp_GraphicEq, float* gain_db)
{
	struct GraphicEqHdlType* cast_handle;

	cast_handle = (struct GraphicEqHdlType*)(hp_GraphicEq);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*gain_db = cast_handle->master_gain;

	return(OKAY);
}

int PT_DECLSPEC GraphicEqGetNormalization(PT_HANDLE* hp_GraphicEq, float* gain_db)
{
	struct GraphicEqHdlType* cast_handle;

	cast_handle = (struct GraphicEqHdlType*)(hp_GraphicEq);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*gain_db = cast_handle->normalization_gain;

	return(OKAY);
}


int PT_DECLSPEC GraphicEqGetFilterQ(PT_HANDLE* hp_GraphicEq, float* q_multiplier)
{
	struct GraphicEqHdlType* cast_handle;

	cast_handle = (struct GraphicEqHdlType*)(hp_GraphicEq);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*q_multiplier = cast_handle->Q_multiplier;

	return(OKAY);
}

int PT_DECLSPEC GraphicEqGetBalance(PT_HANDLE* hp_GraphicEq, float* balance_db)
{
	struct GraphicEqHdlType* cast_handle;

	cast_handle = (struct GraphicEqHdlType*)(hp_GraphicEq);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*balance_db = cast_handle->balance;

	return(OKAY);
}