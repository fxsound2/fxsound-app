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
#include "codedefs.h"
#include "pt_defs.h"
#include "u_DfxDsp.h"
#include "dfxp.h"
#include "DfxSdk.h"
#include "vals.h"
#include "GraphicEq.h"
#include "reg.h"
#include "mth.h"
#include "ptutil/dfxp/u_dfxp.h"
/*
* FUNCTION: eqUpdateFromRegistry()
* DESCRIPTION:
*
* 2020-04-29: Taken from dfxg_UpdateFromRegistryEQ() in dfxgEQ.cpp
*
*  Checks if the value EQ stored in the registry has changed from what we
*  currently think it is.  If it has changed, then update the display and DSP memory values accordingly.
*
* This function only gets called in the sound card case, not in the XP dll based case that uses the winmm or dsound dll's.
* It implements the dfxp calls required to update the EQ params in the DSP module.
* In the XP case these calls are implemented directly in dfxpCommunicateAllNonFixed and the calls in that function must be
* kept up to date with any change in calls in this function.
*/
int DfxDspPrivate::eqUpdateFromRegistry(int *ip_eq_changed)
{
	int i_band_num;
	realtype r_boost_cut_from_processing;
	realtype r_boost_cut_from_registry;
	wchar_t wcp_boost_cut_from_processing[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_boost_cut_from_registry[PT_MAX_GENERIC_STRLEN];
	int i_eq_on_state_from_memory;
	int i_eq_on_state_from_registry;

	*ip_eq_changed = IS_FALSE;

	/* Check if the on/off state has changed */
	if (dfxpEqGetProcessingOn(dfxp_handle_, DFXP_STORAGE_TYPE_MEMORY, &i_eq_on_state_from_memory) != OKAY)
		return(NOT_OKAY);
	if (dfxpEqGetProcessingOn(dfxp_handle_, DFXP_STORAGE_TYPE_REGISTRY, &i_eq_on_state_from_registry) != OKAY)
		return(NOT_OKAY);
	if (i_eq_on_state_from_memory != i_eq_on_state_from_registry)
	{
		*ip_eq_changed = IS_TRUE;

		/* Store the new setting in memory */
		if (dfxpEqSetProcessingOn(dfxp_handle_, DFXP_STORAGE_TYPE_MEMORY, i_eq_on_state_from_registry) != OKAY)
			return(NOT_OKAY);

		/* If eq has been turned off, then turn Hyperbass off */
		if (i_eq_on_state_from_registry == IS_FALSE)
		{
			if (dfxpSetButtonValue(dfxp_handle_, DFX_UI_BUTTON_BASS_BOOST, IS_FALSE) != OKAY)
				return(NOT_OKAY);
		}
		else /* If eq has been turned on, then force Hyperbass on and move Hyperbass to match band 1*/
		{
			/* Get the band1 eq setting */
			if (dfxpEqGetBandBoostCut_FromProcessing(dfxp_handle_, 1, &r_boost_cut_from_processing, wcp_boost_cut_from_processing) != OKAY)
				return(NOT_OKAY);

			if (eqMakeHyperBassMatchBand1(r_boost_cut_from_processing) != OKAY)
				return(NOT_OKAY);
		}
	}

	for (i_band_num = 2; i_band_num <= DFXP_GRAPHIC_EQ_NUM_BANDS; i_band_num++)
	{
		/* Get the currently utilized eq setting */
		if (dfxpEqGetBandBoostCut_FromProcessing(dfxp_handle_, i_band_num, &r_boost_cut_from_processing, wcp_boost_cut_from_processing) != OKAY)
			return(NOT_OKAY);

		/* Get the eq setting from the registry */
		if (dfxpEqGetBandBoostCut_FromRegistry(dfxp_handle_, i_band_num, &r_boost_cut_from_registry, wcp_boost_cut_from_registry) != OKAY)
			return(NOT_OKAY);

		/*
		* Check if setting has changed.
		* NOTE: Since we are comparing real values we must compare their string equivalents as they would be stored in the registry.
		*/
		if (wcscmp(wcp_boost_cut_from_processing, wcp_boost_cut_from_registry))
		{
			*ip_eq_changed = IS_TRUE;

			/* Set the new value */
			if (dfxpEqSetBandBoostCut(dfxp_handle_, DFXP_STORAGE_TYPE_MEMORY, i_band_num, r_boost_cut_from_registry) != OKAY)
				return(NOT_OKAY);

			/* If it is band 1 that is being changed (the lowest band) then we need move the Hyperbass to match the EQ */
			if (i_band_num == 1)
			{
				if (eqMakeHyperBassMatchBand1(r_boost_cut_from_registry) != OKAY)
					return(NOT_OKAY);
			}
		}
	}

	return(OKAY);
}

int DfxDspPrivate::resetEQ()
{
	int i_band_num;

	for (i_band_num = 2; i_band_num <= DFXP_GRAPHIC_EQ_NUM_BANDS; i_band_num++)
	{
		if (dfxpEqSetBandBoostCut(dfxp_handle_, DFXP_STORAGE_TYPE_ALL, i_band_num, (realtype)0.0) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}

/*
* Modeled after dfxg_GetGraphicEqInfoFromVals() in dfxgPreset.cpp.
*/
int DfxDspPrivate::getGraphicEqInfoFromVals(PT_HANDLE *hp_vals)
{
	PT_HANDLE *hp_graphicEq;
	realtype r_boost_cut;
    realtype r_freq;
	int i_band_num;
	int i_eq_on;
	float f_bass_boost_value;

	if (valsGetGraphicEq(hp_vals, &hp_graphicEq, &i_eq_on) != OKAY)
		return(NOT_OKAY);

	/*
	* If it is an old preset which does not have any eq data eq to be on but then set all the bands to be flat except band 1 which
	* should match the Hyperbass setting.
	*/
	if (hp_graphicEq == NULL)
	{
		i_eq_on = IS_TRUE;

		if (dfxpEqSetProcessingOn(dfxp_handle_, DFXP_STORAGE_TYPE_REGISTRY, i_eq_on) != OKAY)
			return(NOT_OKAY);

		for (i_band_num = 2; i_band_num <= DFXP_GRAPHIC_EQ_NUM_BANDS; i_band_num++)
		{
			if (i_band_num == 1)
			{
				/* Get the Hyperbass setting */
				if (dfxpGetKnobValue(dfxp_handle_, DFX_UI_KNOB_BASS_BOOST, &f_bass_boost_value) != OKAY)
					return(NOT_OKAY);

				r_boost_cut = (realtype)f_bass_boost_value * (realtype)10.0;
			}
			else
				r_boost_cut = (realtype)0.0;

			if (dfxpEqSetBandBoostCut(dfxp_handle_, DFXP_STORAGE_TYPE_ALL, i_band_num, r_boost_cut) != OKAY)
				return(NOT_OKAY);
		}
	}
	else
	{
		if (dfxpEqSetProcessingOn(dfxp_handle_, DFXP_STORAGE_TYPE_REGISTRY, i_eq_on) != OKAY)
			return(NOT_OKAY);

        PT_HANDLE* graphic_eq_handle;

        dfxpEqGetGraphicEqHdl(dfxp_handle_, &graphic_eq_handle);

		for (i_band_num = 2; i_band_num <= DFXP_GRAPHIC_EQ_NUM_BANDS; i_band_num++)
		{
			r_boost_cut = (realtype)0.0;

			if (GraphicEqGetBandBoostCut(hp_graphicEq, i_band_num, &r_boost_cut) != OKAY)
				return(NOT_OKAY);

			if (dfxpEqSetBandBoostCut(dfxp_handle_, DFXP_STORAGE_TYPE_ALL, i_band_num, r_boost_cut) != OKAY)
				return(NOT_OKAY);

            if (GraphicEqGetBandCenterFrequency(hp_graphicEq, i_band_num, &r_freq) != OKAY)
                return(NOT_OKAY);

            if (GraphicEqSetBandFreq(graphic_eq_handle, i_band_num, r_freq) != OKAY)
                return(NOT_OKAY);

			/* If it is band 1 that is being changed (the lowest band) then we need move the Hyperbass to match the EQ */
			if (i_band_num == 1)
			{
				if (eqMakeHyperBassMatchBand1(r_boost_cut) != OKAY)
					return(NOT_OKAY);
			}
		}
	}

	return(OKAY);
}

/*
 * Modeled after dfxg_EqMakeHyperBassMatchBand1() in dfxgEQ.cpp.
 */
int DfxDspPrivate::eqMakeHyperBassMatchBand1(realtype r_boost_cut_band1)
{
	realtype r_new_hyperbass_val;
	realtype r_normalized_hyperbass_val;
	int i_eq_on;
	int i_hyperbass_on;
	bool b_do_toggle;

	/*
	* Make the Hyperbass on/off match the EQ on/off
	*/
	if (dfxpEqGetProcessingOn(dfxp_handle_, DFXP_STORAGE_TYPE_MEMORY, &i_eq_on) != OKAY)
		return(NOT_OKAY);
	if (dfxpGetButtonValue(dfxp_handle_, DFX_UI_BUTTON_BASS_BOOST, &i_hyperbass_on) != OKAY)
		return(NOT_OKAY);

	if (i_eq_on != i_hyperbass_on)
	{
		/* Don't turn Hyperbass on if it is off and eq has been moved to 0 because that is what happens when Hyperbass is turned off */
		b_do_toggle = true;
		if ((r_boost_cut_band1 == (realtype)0.0) && (!i_hyperbass_on))
			b_do_toggle = false;

		if (b_do_toggle)
		{
			if (dfxpSetButtonValue(dfxp_handle_, DFX_UI_BUTTON_BASS_BOOST, i_eq_on) != OKAY)
				return(NOT_OKAY);
		}
	}

	/* If band1 is positive, more hyperbass to match (up to 10) */
	if (r_boost_cut_band1 >= (realtype)0.0)
	{
		if (r_boost_cut_band1 > (realtype)10.0)
			r_new_hyperbass_val = (realtype)10.0;
		else
			r_new_hyperbass_val = r_boost_cut_band1;
	}
	else /* If band1 is negative, move hyperbass to 0 */
	{
		r_new_hyperbass_val = (realtype)0.0;
	}

	/* Set the new Hyperbass value.  We need to use a normalized value between 0.0 and 1.0 for this call */
	r_normalized_hyperbass_val = r_new_hyperbass_val / (realtype)10.0;

	if (dfxpSetKnobValue(dfxp_handle_, DFX_UI_KNOB_BASS_BOOST, (float)r_normalized_hyperbass_val, true) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
* Modeled after dfxpEqSetProcessingOn() in dfxpEq.cpp.
*/
int DfxDspPrivate::eqSetProcessingOn(int i_storage_type, int i_on)
{
	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
	wchar_t wcp_key_value[DFXP_REGISTRY_BUFFER_LENGTH];


	/* Store setting in memory (if requested) */
	if ((i_storage_type == DFXP_STORAGE_TYPE_MEMORY) ||
		(i_storage_type == DFXP_STORAGE_TYPE_ALL))
	{
		eq_processing_on_ = i_on;
	}

	/* Store setting in registry (if requested) */
	if ((i_storage_type == DFXP_STORAGE_TYPE_REGISTRY) ||
		(i_storage_type == DFXP_STORAGE_TYPE_ALL))
	{
		swprintf(wcp_full_key_path, L"%s\\%s\\%d\\%d\\%s\\%s\\%s",
			DFXP_REGISTRY_TOP_WIDE,
			product_specific_.wcp_registry_product_name,
			product_specific_.major_version,
			vendor_specific_.vendor_code,
			DFXP_REGISTRY_LASTUSED_WIDE,
			DFXP_REGISTRY_EQ_FOLDER_NAME_WIDE,
			DFXP_REGISTRY_EQ_ON_WIDE);

		swprintf(wcp_key_value, L"%d", i_on);

		if (regCreateKey_Wide(REG_CURRENT_USER, wcp_full_key_path, wcp_key_value) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}

/*
* Modeled after dfxpEqGetProcessingOn() in dfxpEq.cpp.
*/
int DfxDspPrivate::eqGetProcessingOn(int i_storage_type, int *ip_on)
{
	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
	wchar_t wcp_key_value[DFXP_REGISTRY_BUFFER_LENGTH];
	int key_exists;
	int use_default;
	int is_long;

	*ip_on = IS_TRUE;

	if (i_storage_type == DFXP_STORAGE_TYPE_MEMORY)
	{
		*ip_on = eq_processing_on_;
		return(OKAY);
	}

	swprintf(wcp_full_key_path, L"%s\\%s\\%d\\%d\\%s\\%s\\%s",
		DFXP_REGISTRY_TOP_WIDE,
		product_specific_.wcp_registry_product_name,
		product_specific_.major_version,
		vendor_specific_.vendor_code,
		DFXP_REGISTRY_LASTUSED_WIDE,
		DFXP_REGISTRY_EQ_FOLDER_NAME_WIDE,
		DFXP_REGISTRY_EQ_ON_WIDE);

	if (regReadKey_Wide(REG_CURRENT_USER, wcp_full_key_path, &key_exists, wcp_key_value,
		(unsigned long)DFXP_REGISTRY_BUFFER_LENGTH) != OKAY)
		return(NOT_OKAY);

	use_default = IS_TRUE;

	if (key_exists)
	{
		if (mthIsLong_Wide(wcp_key_value, &is_long) != OKAY)
			return(NOT_OKAY);
		if (is_long)
			use_default = IS_FALSE;
	}

	if (!use_default)
		*ip_on = _wtoi(wcp_key_value);

	return(OKAY);
}

void DfxDspPrivate::eqOn(bool on)
{
	if (on)
	{
		eqSetProcessingOn(DFXP_STORAGE_TYPE_ALL, IS_TRUE);
	}
	else
	{
		eqSetProcessingOn(DFXP_STORAGE_TYPE_ALL, IS_FALSE);
	}
}

int DfxDspPrivate::getNumEqBands()
{
	int num_bands = 0;
    PT_HANDLE* graphic_eq_handle;

    dfxpEqGetGraphicEqHdl(dfxp_handle_, &graphic_eq_handle);

	GraphicEqGetNumBands(graphic_eq_handle, &num_bands);

	return num_bands;
}

float DfxDspPrivate::getEqBandFrequency(int band_num)
{
	float band_freq = 0.0;

    dfxpEqGetBandCenterFrequency(dfxp_handle_, band_num + 1, &band_freq);

	return band_freq;
}

void DfxDspPrivate::setEqBandFrequency(int band_num, float freq)
{

    PT_HANDLE* graphic_eq_handle;

    dfxpEqGetGraphicEqHdl(dfxp_handle_, &graphic_eq_handle);

    GraphicEqSetBandFreq(graphic_eq_handle, band_num + 1, freq);
}

void DfxDspPrivate::getEqBandFrequencyRange(int band_num, float* min_freq, float* max_freq)
{
    PT_HANDLE* graphic_eq_handle;

    *min_freq = 0;
    *max_freq = 0;

    dfxpEqGetGraphicEqHdl(dfxp_handle_, &graphic_eq_handle);

    GraphicEqGetBandFrequencyRange(graphic_eq_handle, band_num + 1, min_freq, max_freq);
}

float DfxDspPrivate::getEqBandBoostCut(int band_num)
{
    PT_HANDLE* graphic_eq_handle;

	float band_boost_cut = 0.0;

    dfxpEqGetGraphicEqHdl(dfxp_handle_, &graphic_eq_handle);

	GraphicEqGetBandBoostCut(graphic_eq_handle, band_num + 1, &band_boost_cut);

	return band_boost_cut;
}

void DfxDspPrivate::setEqBandBoostCut(int band_num, float boost)
{
	dfxpEqSetBandBoostCut(dfxp_handle_, DFXP_STORAGE_TYPE_ALL, band_num + 1, boost);
}