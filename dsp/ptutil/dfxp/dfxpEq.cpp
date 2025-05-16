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

/* dfxpEq.cpp */

#include "codedefs.h"

#include <windows.h>
#include <stdio.h>

#include "u_dfxp.h" /* Must go before codedefs.h due to mmgr */

#include "dfxp.h"
#include "DfxSdk.h"
#include "qnt.h"
#include "GraphicEq.h"
#include "reg.h"
#include "mth.h"

/*
 * FUNCTION: dfxp_EqInit() 
 * DESCRIPTION:
 */
int dfxp_EqInit(PT_HANDLE *hp_dfxp)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if (dfxpEqSetEqType(hp_dfxp, DFXP_GRAPHIC_EQ_NUM_BANDS) != OKAY)
		return(NOT_OKAY);

	/* Initialize the processing on flag */
	if (dfxpEqGetProcessingOn(hp_dfxp, DFXP_STORAGE_TYPE_REGISTRY, &(cast_handle->eq.i_processing_on)) != OKAY)
		return(NOT_OKAY);

	/* Initialize band1 since it is a special case.  If a value does not exist in the registry for it, use Hyperbass value */
	if (dfxpEqInitBand1SpecialCase(hp_dfxp) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxpEqSetEqType() 
 * DESCRIPTION:
 */
int dfxpEqSetEqType(PT_HANDLE *hp_dfxp, int i_eq_type)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* If the eq handle had previously been created, free the old one */
	if (cast_handle->eq.graphicEq_hdl != NULL)
	{
		if (GraphicEqFreeUp(&(cast_handle->eq.graphicEq_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Create the Graphic EQ handle */
	if (GraphicEqNew(&(cast_handle->eq.graphicEq_hdl), i_eq_type, cast_handle->trace.mode, cast_handle->slout1) != OKAY)
		return(NOT_OKAY);

	if (GraphicEqSetAppHasHyperBassMode(cast_handle->eq.graphicEq_hdl, true) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxpEqSetProcessingOn() 
 * DESCRIPTION:
 *
 *  i_storage_type can be either:
 *  DFXP_STORAGE_TYPE_REGISTRY
 *  DFXP_STORAGE_TYPE_MEMORY
 *  DFXP_STORAGE_TYPE_ALL
 *
 */
int dfxpEqSetProcessingOn(PT_HANDLE *hp_dfxp, int i_storage_type, int i_on)
{
	struct dfxpHdlType *cast_handle;
	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
	wchar_t wcp_key_value[DFXP_REGISTRY_BUFFER_LENGTH];

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* Store setting in memory (if requested) */
	if ((i_storage_type == DFXP_STORAGE_TYPE_MEMORY) ||
		 (i_storage_type == DFXP_STORAGE_TYPE_ALL))
	{
		cast_handle->eq.i_processing_on = i_on;
	}

	/* Store setting in registry (if requested) */
	if ((i_storage_type == DFXP_STORAGE_TYPE_REGISTRY) ||
		 (i_storage_type == DFXP_STORAGE_TYPE_ALL))
	{
		swprintf(wcp_full_key_path, L"%s\\%s\\%d\\%d\\%s\\%s\\%s", 
									DFXP_REGISTRY_TOP_WIDE, 
									cast_handle->wcp_product_name, 
									cast_handle->major_version,
									cast_handle->vendor_code, 
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
 * FUNCTION: dfxpEqInitBand1SpecialCase() 
 * DESCRIPTION:
 *
 * Initialize band1 since it is a special case.  If a value does not exist in the registry for it, use Hyperbass value.
 *
 */
int dfxpEqInitBand1SpecialCase(PT_HANDLE *hp_dfxp)
{
	wchar_t wcp_keyname[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_key_value[DFXP_REGISTRY_BUFFER_LENGTH];
	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
	int key_exists;
	float f_bass_boost_value; 
	realtype r_band1_value;

	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* Check if band1 has a value in the registry */
	swprintf(wcp_keyname, L"%s%d", DFXP_REGISTRY_EQ_BAND_NAME_WIDE, 1);

	swprintf(wcp_full_key_path, L"%s\\%s\\%d\\%d\\%s\\%s\\%s", 
									DFXP_REGISTRY_TOP_WIDE, 
									cast_handle->wcp_product_name, 
									cast_handle->major_version,
		                     cast_handle->vendor_code, 
									DFXP_REGISTRY_LASTUSED_WIDE, 
									DFXP_REGISTRY_EQ_FOLDER_NAME_WIDE,
									wcp_keyname);

	if (regReadKey_Wide(REG_CURRENT_USER, wcp_full_key_path, &key_exists, wcp_key_value,
				(unsigned long)DFXP_REGISTRY_BUFFER_LENGTH) != OKAY)
	         return(NOT_OKAY);

	/* If the key exists, there is nothing to do */
	if (key_exists == IS_TRUE)
		return(OKAY);

	/* Since band1 setting does not exist, we need to use Hyperbass setting to initialize it */
	if (dfxpGetKnobValue(hp_dfxp, DFX_UI_KNOB_BASS_BOOST, &f_bass_boost_value) != OKAY)
		return(NOT_OKAY);

	r_band1_value = (realtype)f_bass_boost_value * (realtype)10.0;

	if (dfxpEqSetBandBoostCut(hp_dfxp, DFXP_STORAGE_TYPE_ALL, 1, r_band1_value) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

int dfxpEqSetVolumeNormalization(PT_HANDLE* hp_dfxp, realtype r_target_rms)
{
	struct dfxpHdlType* cast_handle;

	cast_handle = (struct dfxpHdlType*)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if (GraphicEqSetVolumeNormalization(cast_handle->eq.graphicEq_hdl, r_target_rms) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxpEqGetProcessingOn() 
 * DESCRIPTION:
 *
 *  i_storage_type can be either:
 *  DFXP_STORAGE_TYPE_REGISTRY
 *  DFXP_STORAGE_TYPE_MEMORY
 *
 */
int dfxpEqGetProcessingOn(PT_HANDLE *hp_dfxp, int i_storage_type, int *ip_on)
{
	struct dfxpHdlType *cast_handle;
	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
	wchar_t wcp_key_value[DFXP_REGISTRY_BUFFER_LENGTH];
	int key_exists;
	int use_default;
   int is_long;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	*ip_on = IS_TRUE;

	if (i_storage_type == DFXP_STORAGE_TYPE_MEMORY)
	{
		*ip_on = cast_handle->eq.i_processing_on;
		return(OKAY);
	}

	swprintf(wcp_full_key_path, L"%s\\%s\\%d\\%d\\%s\\%s\\%s", 
									DFXP_REGISTRY_TOP_WIDE, 
									cast_handle->wcp_product_name, 
									cast_handle->major_version,
									cast_handle->vendor_code, 
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

/*
 * FUNCTION: dfxpEqGetBandCenterFrequency() 
 * DESCRIPTION:
 */
int dfxpEqGetBandCenterFrequency(PT_HANDLE *hp_dfxp, int i_band_num, realtype *rp_center_freq)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if (cast_handle->eq.graphicEq_hdl == NULL)
		return(NOT_OKAY);

	if (GraphicEqGetBandCenterFrequency(cast_handle->eq.graphicEq_hdl, i_band_num, rp_center_freq) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: dfxpEqSetBandBoostCut() 
 * DESCRIPTION:
 */
int dfxpEqSetBandBoostCut(PT_HANDLE *hp_dfxp, int i_storage_type, int i_band_num, realtype r_boost_cut)
{	
	struct dfxpHdlType *cast_handle;
	wchar_t wcp_keyname[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_key_value[DFXP_REGISTRY_BUFFER_LENGTH];
	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	if (cast_handle->eq.graphicEq_hdl == NULL)
		return(NOT_OKAY);

	if ((i_band_num < 1) || (i_band_num > DFXP_GRAPHIC_EQ_NUM_BANDS))
		return(NOT_OKAY);

	if (r_boost_cut < DFXP_GRAPHIC_EQ_MIN_BOOST_OR_CUT_DB)
		r_boost_cut = DFXP_GRAPHIC_EQ_MIN_BOOST_OR_CUT_DB;
	else if (r_boost_cut > DFXP_GRAPHIC_EQ_MAX_BOOST_OR_CUT_DB)
		r_boost_cut = DFXP_GRAPHIC_EQ_MAX_BOOST_OR_CUT_DB;

	/* Store new setting in memory */
	if ((i_storage_type == DFXP_STORAGE_TYPE_MEMORY)  ||
		 (i_storage_type == DFXP_STORAGE_TYPE_ALL))
	{
		if (GraphicEqSetBandBoostCut(cast_handle->eq.graphicEq_hdl, i_band_num, r_boost_cut) != OKAY)
			return(NOT_OKAY);
	}

	/* Store new setting in the registry */
	if ((i_storage_type == DFXP_STORAGE_TYPE_REGISTRY)  ||
		 (i_storage_type == DFXP_STORAGE_TYPE_ALL))
	{
		/* Save the new setting in the registry */
		swprintf(wcp_keyname, L"%s%d", DFXP_REGISTRY_EQ_BAND_NAME_WIDE, i_band_num);

		swprintf(wcp_full_key_path, L"%s\\%s\\%d\\%d\\%s\\%s\\%s", 
									DFXP_REGISTRY_TOP_WIDE, 
									cast_handle->wcp_product_name, 
									cast_handle->major_version,
									cast_handle->vendor_code, 
									DFXP_REGISTRY_LASTUSED_WIDE, 
									DFXP_REGISTRY_EQ_FOLDER_NAME_WIDE,
									wcp_keyname);
	
		swprintf(wcp_key_value, L"%.2f", r_boost_cut);

		if (regCreateKey_Wide(REG_CURRENT_USER, wcp_full_key_path, wcp_key_value) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: dfxpEqGetBandBoostCut_FromProcessing() 
 * DESCRIPTION:
 *
 *  Gets the currently utilized boost cut setting for the specified band.
 *
 */
int dfxpEqGetBandBoostCut_FromProcessing(PT_HANDLE *hp_dfxp, int i_band_num, realtype *rp_boost_cut, wchar_t *wcp_boost_cut)
{	
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	*rp_boost_cut = (realtype)0.0;
	swprintf(wcp_boost_cut, L"%.2f", *rp_boost_cut);

	if (cast_handle->eq.graphicEq_hdl == NULL)
		return(NOT_OKAY);

	if ((i_band_num < 1) || (i_band_num > DFXP_GRAPHIC_EQ_NUM_BANDS))
		return(NOT_OKAY);

   if (GraphicEqGetBandBoostCut(cast_handle->eq.graphicEq_hdl, i_band_num, rp_boost_cut) != OKAY)
		return(NOT_OKAY);

	swprintf(wcp_boost_cut, L"%.2f", *rp_boost_cut);

	return(OKAY);
}

/*
 * FUNCTION: dfxpEqGetBandBoostCut_FromRegistry() 
 * DESCRIPTION:
 *
 *  Gets the currently stored boost cut setting in the registry for the specified band.
 *
 */
int dfxpEqGetBandBoostCut_FromRegistry(PT_HANDLE *hp_dfxp, int i_band_num, realtype *rp_boost_cut, wchar_t *wcp_boost_cut)
{	
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	wchar_t wcp_keyname[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_key_value[DFXP_REGISTRY_BUFFER_LENGTH];
	wchar_t wcp_full_key_path[PT_MAX_PATH_STRLEN];
	int key_exists;

	*rp_boost_cut = (realtype)0.0;
	swprintf(wcp_boost_cut, L"%.2f", *rp_boost_cut);

	if (cast_handle->eq.graphicEq_hdl == NULL)
		return(NOT_OKAY);

	if ((i_band_num < 1) || (i_band_num > DFXP_GRAPHIC_EQ_NUM_BANDS))
		return(NOT_OKAY);

	/* Calculate name the registry */
	swprintf(wcp_keyname, L"%s%d", DFXP_REGISTRY_EQ_BAND_NAME_WIDE, i_band_num);

	swprintf(wcp_full_key_path, L"%s\\%s\\%d\\%d\\%s\\%s\\%s", 
									DFXP_REGISTRY_TOP_WIDE, 
									cast_handle->wcp_product_name, 
									cast_handle->major_version,
									cast_handle->vendor_code, 
									DFXP_REGISTRY_LASTUSED_WIDE, 
									DFXP_REGISTRY_EQ_FOLDER_NAME_WIDE,
									wcp_keyname);

	if (regReadKey_Wide(REG_CURRENT_USER, wcp_full_key_path, &key_exists, wcp_key_value,
				(unsigned long)DFXP_REGISTRY_BUFFER_LENGTH) != OKAY)
	         return(NOT_OKAY);

	if (key_exists == IS_FALSE)
		return(OKAY);

	if (wcslen(wcp_key_value) <= 0)
		return(OKAY);

	swscanf(wcp_key_value, L"%g\n", rp_boost_cut);

	/* Make sure value is in proper range */
	if (*rp_boost_cut < DFXP_GRAPHIC_EQ_MIN_BOOST_OR_CUT_DB) 
		*rp_boost_cut = DFXP_GRAPHIC_EQ_MIN_BOOST_OR_CUT_DB;
	else if (*rp_boost_cut > DFXP_GRAPHIC_EQ_MAX_BOOST_OR_CUT_DB)
		*rp_boost_cut = DFXP_GRAPHIC_EQ_MAX_BOOST_OR_CUT_DB;

	swprintf(wcp_boost_cut, L"%s", wcp_key_value);

	return(OKAY);
}

/*
 * FUNCTION: dfxpEqGetGraphicEqHdl() 
 * DESCRIPTION:
 */
int dfxpEqGetGraphicEqHdl(PT_HANDLE *hp_dfxp, PT_HANDLE **hpp_graphicEq)
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	*hpp_graphicEq = cast_handle->eq.graphicEq_hdl;

	return(OKAY);
}