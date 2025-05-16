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

/* dfxpSpectrum.cpp */

#include "codedefs.h"

#include <windows.h>
#include <stdio.h>
#include <share.h>

#include "u_dfxp.h" 

#include "dfxp.h"
#include "spectrum.h"
#include "DfxSdk.h"
#include "reg.h"
#include "file.h"
#include "dfxSharedUtil.h"

/*
 * FUNCTION: dfxp_SpectrumInit() 
 * DESCRIPTION:
 *
 *  Initialize the spectrum handle and the associated data.
 *
 */
int dfxp_SpectrumInit(PT_HANDLE *hp_dfxp)
{
    struct dfxpHdlType *cast_handle;

    cast_handle = (struct dfxpHdlType *)(hp_dfxp);

    if (cast_handle == NULL)
        return(OKAY);

    realtype r_host_buffer_delay_secs;
    realtype r_spectrum_refresh_rate_secs;

    /* Initialize the spectrum handle */
    r_host_buffer_delay_secs = (realtype)cast_handle->l_host_buffer_delay_msecs * (realtype)0.001;
    r_spectrum_refresh_rate_secs = (realtype)DFXP_SPECTRUM_REFRESH_RATE_MSECS * (realtype)0.001;

     if (spectrumNew(&(cast_handle->spectrum.spectrum_hdl), 
                                DFXP_SPECTRUM_NUM_BANDS, 
                                r_host_buffer_delay_secs, 
                                r_spectrum_refresh_rate_secs,
                                cast_handle->slout1,
                               cast_handle->trace.mode) != OKAY)
        return(NOT_OKAY);

    cast_handle->spectrum.sample_sets_since_last_spectrum_save = 0;

    if (dfxpSpectrumSendClearValues((PT_HANDLE *)cast_handle) != OKAY)
        return(NOT_OKAY);

    return(OKAY);
}

/*
 * FUNCTION: dfxpSpectrumSendClearValues() 
 * DESCRIPTION:
 *
 *  Sends all zero spectrum values to the GUI.
 *
 */
int dfxpSpectrumSendClearValues(PT_HANDLE *hp_dfxp)
{
    struct dfxpHdlType *cast_handle;

    cast_handle = (struct dfxpHdlType *)(hp_dfxp);

    if (cast_handle == NULL)
        return(OKAY);

    /* Clear the spectrum file */
    if (dfxp_SpectrumStoreCurrentValuesInSharedMemory(hp_dfxp, IS_TRUE) != OKAY)
        return(NOT_OKAY);

    return(OKAY);
}

/*
 * FUNCTION: dfxpSpectrumGetBandValues()
 * DESCRIPTION:
 *
 *  Get the array with the previously calculated band values
 *
 */
int dfxpSpectrumGetBandValues(PT_HANDLE* hp_dfxp, realtype* rp_band_values, int i_array_size)
{
    struct dfxpHdlType *cast_handle;

    cast_handle = (struct dfxpHdlType *)(hp_dfxp);

    if (cast_handle == NULL)
        return(OKAY);

    if (cast_handle->spectrum.spectrum_hdl == NULL)
        return(OKAY);

    if (spectrumGetBandValues(cast_handle->spectrum.spectrum_hdl, rp_band_values, i_array_size) != OKAY)
        return(NOT_OKAY);

    return(OKAY);
}

/*
 * FUNCTION: dfxp_SpectrumStoreCurrentValuesInSharedMemory() 
 * DESCRIPTION:
 *
 *  Stores the current spectrum values in the shared memory so they can be sent to the UI.
 *
 */
int dfxp_SpectrumStoreCurrentValuesInSharedMemory(PT_HANDLE *hp_dfxp, int i_clear_values)
{
    struct dfxpHdlType *cast_handle;

    cast_handle = (struct dfxpHdlType *)(hp_dfxp);

    if (cast_handle == NULL)
        return(OKAY);

   realtype rp_band_values[DFXP_SPECTRUM_NUM_BANDS];
    int index;

    if (cast_handle->spectrum.spectrum_hdl == NULL)
        return(OKAY);
    
    if (cast_handle->hp_sharedUtil == NULL)
        return(OKAY);

    if (i_clear_values)
    {
        for (index = 0; index < DFXP_SPECTRUM_NUM_BANDS; index++)
        {
            rp_band_values[index] = (realtype)0.0;
        }
    }
    else
    {
        if (spectrumGetBandValues(cast_handle->spectrum.spectrum_hdl, rp_band_values, DFXP_SPECTRUM_NUM_BANDS) != OKAY)
            return(NOT_OKAY);
    }

    /* Store the values in shared memory */
    if (dfxSharedUtilSetSpectrumValues(cast_handle->hp_sharedUtil, rp_band_values, DFXP_SPECTRUM_NUM_BANDS) != OKAY)
        return(NOT_OKAY);

    return(OKAY);
}