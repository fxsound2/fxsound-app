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

    if (cast_handle->num_bands == 1)
    {
        cast_handle->Q = (realtype)1.0;

        *fp_min_freq = rp_freq_array[i_band_num - 1];
        *fp_max_freq = rp_freq_array[i_band_num - 1];
    }
    else
    {
        d_ratio = (double)cast_handle->max_band_freq / (double)cast_handle->min_band_freq;

        if (i_band_num == 1)
        {
            *fp_min_freq = rp_freq_array[i_band_num - 1];
        }
        else
        {
            d_power = (double)((i_band_num-1)*2 - 1) / (double)(cast_handle->num_bands*2 - 2);

            d_factor = pow(d_ratio, d_power);

            *fp_min_freq = round((realtype)(cast_handle->min_band_freq * d_factor));
            if (*fp_min_freq < 1000)
                (*fp_min_freq)++;
            else
                (*fp_min_freq)+=10;
        }
        

        if (i_band_num == cast_handle->num_bands)
        {
            *fp_max_freq = rp_freq_array[i_band_num - 1];
        }
        else
        {
            d_power = (double)(i_band_num*2 - 1) / (double)(cast_handle->num_bands*2 - 2);

            d_factor = pow(d_ratio, d_power);

            *fp_max_freq = round((realtype)(cast_handle->min_band_freq * d_factor));
        }
        
        
    }	return(OKAY);
}