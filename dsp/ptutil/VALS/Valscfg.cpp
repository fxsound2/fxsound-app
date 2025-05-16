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

/* Standard includes */
#include <dos.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "slout.h"
#include "file.h"
#include "mry.h"
#include "vals.h"
#include "pstr.h"
#include "u_vals.h"

#define LINE_LENGTH 128

/*
 * FUNCTION: valsCfgRead()
 * DESCRIPTION:
 *   Read in and allocate a new configuration delay handle from the past
 *   file location.
 */
int PT_DECLSPEC valsCfgRead(PT_HANDLE **hpp_vals_cfg, wchar_t *wcp_file_path,
					CSlout *hp_slout)
{
	FILE *stream;
	wchar_t wcp_str[PT_MAX_PATH_STRLEN];
	realtype version;
	int str_length;
	struct valsCfgHdlType *cast_handle;
	int title_length;

	if (wcp_file_path == NULL)
		return(NOT_OKAY);

	/* Allocate the handle */
	cast_handle = (struct valsCfgHdlType *)calloc(1,
									 sizeof(struct valsCfgHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Initialize the handle */
	cast_handle->slout_hdl = hp_slout;

	/* Open the file for reading */
	stream = fileOpen_Wide(wcp_file_path, L"r", hp_slout);
	if (stream == NULL)
		return(NOT_OKAY);

    /* Store the filepath */
    swprintf(cast_handle->wcp_filepath, L"%s", wcp_file_path);

	/* Skip past the first line */
	fgetws(wcp_str, LINE_LENGTH, stream);
    
	/* Get the version number */
	fgetws(wcp_str, LINE_LENGTH, stream);
	swscanf(wcp_str, L"%g\n", &(version));

    /* Get the title (strip off the '\n') */
	fgetws(wcp_str, LINE_LENGTH, stream);
    if (wcp_str == NULL)
       return(NOT_OKAY);
    title_length = (int)wcslen(wcp_str);
    if (title_length < 1)
       return(NOT_OKAY);
	wcp_str[title_length - 1] = L'\0';
	swprintf((cast_handle->wcp_title), L"%s", wcp_str);

	/* Get the number of elements */
	fgetws(wcp_str, LINE_LENGTH, stream);
	swscanf(wcp_str, L"%d\n", &(cast_handle->num_elements));   

	/* Get the stereo input mode flag */
	fgetws(wcp_str, LINE_LENGTH, stream);
	str_length = (int)wcslen(wcp_str);
	if (str_length == 0)
	{
		swprintf(cast_handle->wcp_msg1, L"Illegal Mono/Stereo Input Specification");
		(cast_handle->slout_hdl)->Error_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	}
	if ((wcp_str[0] == L's') || (wcp_str[0] == L'S'))
	   cast_handle->stereo_input_mode = IS_TRUE;
	else 
	   cast_handle->stereo_input_mode = IS_FALSE;  
	   
    /* Get the stereo output mode flag */
	fgetws(wcp_str, LINE_LENGTH, stream);
	str_length = (int)wcslen(wcp_str);
	if (str_length == 0)
	{
		swprintf(cast_handle->wcp_msg1, L"Illegal Mono/Stereo Output Specification");
		(cast_handle->slout_hdl)->Error_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	}
	if ((wcp_str[0] == L's') || (wcp_str[0] == L'S'))
	   cast_handle->stereo_output_mode = IS_TRUE;
	else 
	   cast_handle->stereo_output_mode = IS_FALSE;	   

	/* Get the default preset */
	fgetws(wcp_str, LINE_LENGTH, stream);
	swscanf(wcp_str, L"%d\n", &(cast_handle->default_preset)); 
	
	/* Get the minimum user preset */
	fgetws(wcp_str, LINE_LENGTH, stream);
	swscanf(wcp_str, L"%d\n", &(cast_handle->min_user_preset)); 
	
	/* Get the time of last preset save */
	fgetws(wcp_str, LINE_LENGTH, stream);
	swscanf(wcp_str, L"%ld\n", &(cast_handle->preset_save_time));   	 	  	  

	fclose(stream);

	*hpp_vals_cfg = (PT_HANDLE *)cast_handle;

	return(OKAY);
}

/*
 * FUNCTION: valsCfgWrite()
 * DESCRIPTION:
 *   
 */
int PT_DECLSPEC valsCfgWrite(PT_HANDLE *hp_vals_cfg)
{	
	struct valsCfgHdlType *cast_handle;
	FILE *stream;    
    
	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);	

	/* Open the file for writing */
	stream = fileOpen_Wide(cast_handle->wcp_filepath, L"w", cast_handle->slout_hdl);
	if (stream == NULL)
		return(NOT_OKAY);  

    fwprintf(stream, L"Effect_Configuration\n");
    
    fwprintf(stream, L"1.0: Version\n");
    
    fwprintf(stream, L"%s\n", cast_handle->wcp_title);
    
    fwprintf(stream, L"%d: Number of Elements (1,2,4,8)\n", cast_handle->num_elements);
    
    if (cast_handle->stereo_input_mode)
       fwprintf(stream, L"Stereo: Mono or Stereo Input Mode\n");
    else
       fwprintf(stream, L"Mono: Mono or Stereo Input Mode\n");   
       
    if (cast_handle->stereo_output_mode)
       fwprintf(stream, L"Stereo: Mono or Stereo Output Mode\n");
    else
       fwprintf(stream, L"Mono: Mono or Stereo Output Mode\n");   

    fwprintf(stream, L"%d: Default Preset\n", cast_handle->default_preset);
    
    fwprintf(stream, L"%d: Minimum User Preset\n", cast_handle->min_user_preset);    
    
    fwprintf(stream, L"%ld: Time of last preset save\n", cast_handle->preset_save_time);     
    
	fclose(stream);

	return(OKAY);
}

/*
 * FUNCTION: valsCfgCheckFileType()
 * DESCRIPTION:
 *   Checks if the passed file is the proper effect configuration file.
 */
int PT_DECLSPEC valsCfgCheckFileType(wchar_t *wcp_file_path, int *ip_proper_type)
{
	FILE *stream;
	wchar_t wcp_line_str[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_type_str[PT_MAX_GENERIC_STRLEN];

	*ip_proper_type = IS_FALSE;

	if (wcp_file_path == NULL)
		return(NOT_OKAY);

	/* Open the file for reading */
	if (_wfopen_s(&stream, wcp_file_path, L"r") != 0)
		return(OKAY);

	/* Get the Effect Type */
	fgetws(wcp_line_str, LINE_LENGTH, stream);

	if (wcslen(wcp_line_str) <= 0)
	{
		fclose(stream);
		return(OKAY);
	}

	swscanf(wcp_line_str, L"%s\n", wcp_type_str);

	if (wcslen(wcp_type_str) <= 0)
	{
		fclose(stream);
		return(OKAY);
	}

	if (!(wcscmp(wcp_type_str, L"Effect_Configuration")))
	{
		*ip_proper_type = IS_TRUE;
		fclose(stream);
	}

	fclose(stream);

	return(OKAY);
}

/*
 * FUNCTION: valsCfgGetTitle()
 * DESCRIPTION:
 *   Fills in the passed string with the title.
 */
int PT_DECLSPEC valsCfgGetTitle(PT_HANDLE *hp_vals_cfg, wchar_t *wcp_title)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (wcp_title == NULL)
	   return(NOT_OKAY);
	   
	swprintf(wcp_title, L"%s", cast_handle->wcp_title);

	return(OKAY);
}

/*
 * FUNCTION: valsCfgGetNumElements()
 * DESCRIPTION:
 *   Get the number of elements specified in delay configuration.
 */
int PT_DECLSPEC valsCfgGetNumElements(PT_HANDLE *hp_vals_cfg, int *ip_num_elements)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_num_elements = cast_handle->num_elements;

	return(OKAY);
}

/*
 * FUNCTION: valsCfgGetStereoInputMode()
 * DESCRIPTION:
 *  Pass back a IS_TRUE or IS_FALSE flag saying if it is in Stereo Input mode.
 */
int PT_DECLSPEC valsCfgGetStereoInputMode(PT_HANDLE *hp_vals_cfg, int *ip_stereo_input_mode)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_stereo_input_mode = cast_handle->stereo_input_mode;

	return(OKAY);
} 

/*
 * FUNCTION: valsCfgGetStereoOutputMode()
 * DESCRIPTION:
 *  Pass back a IS_TRUE or IS_FALSE flag saying if it is in Stereo Output mode.
 */
int PT_DECLSPEC valsCfgGetStereoOutputMode(PT_HANDLE *hp_vals_cfg, int *ip_stereo_output_mode)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_stereo_output_mode = cast_handle->stereo_output_mode;

	return(OKAY);
} 

/*
 * FUNCTION: valsCfgGetDefaultPreset()
 * DESCRIPTION:
 *  Pass back the default preset number.
 */
int PT_DECLSPEC valsCfgGetDefaultPreset(PT_HANDLE *hp_vals_cfg, int *ip_default_preset)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_default_preset = cast_handle->default_preset;

	return(OKAY);
}  

/*
 * FUNCTION: valsCfgGetMinUserPreset()
 * DESCRIPTION:
 *  Pass back the minimum user preset number
 */
int PT_DECLSPEC valsCfgGetMinUserPreset(PT_HANDLE *hp_vals_cfg, int *ip_min_user_preset)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_min_user_preset = cast_handle->min_user_preset;

	return(OKAY);
} 

/*
 * FUNCTION: valsCfgGetMinUserPreset()
 * DESCRIPTION:
 *  Pass back the time when the last preset was saved. (Seconds since 1/1/70)
 */
int PT_DECLSPEC valsCfgGetPresetSaveTime(PT_HANDLE *hp_vals_cfg, long *lp_preset_save_time)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	*lp_preset_save_time = cast_handle->preset_save_time;

	return(OKAY);
}

/*
 * FUNCTION: valsCfgSetNumElements()
 * DESCRIPTION:
 *   Set the number of elements specified in vals configuration.
 */
int PT_DECLSPEC valsCfgSetNumElements(PT_HANDLE *hp_vals_cfg, int i_num_elements)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->num_elements = i_num_elements;

	return(OKAY);
}

/*
 * FUNCTION: valsCfgSetStereoInputMode()
 * DESCRIPTION:
 *   Sets whether or not it is in stereo input mode.
 */
int PT_DECLSPEC valsCfgSetStereoInputMode(PT_HANDLE *hp_vals_cfg, int i_stereo_input_mode)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->stereo_input_mode = i_stereo_input_mode;

	return(OKAY);
}

/*
 * FUNCTION: valsCfgSetStereoOutputMode()
 * DESCRIPTION:
 *   Sets whether or not it is in stereo output mode.
 */
int PT_DECLSPEC valsCfgSetStereoOutputMode(PT_HANDLE *hp_vals_cfg, int i_stereo_output_mode)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->stereo_output_mode = i_stereo_output_mode;

	return(OKAY);
}    
   
/*
 * FUNCTION: valsCfgSetDefaultPreset()
 * DESCRIPTION:
 *   Sets the default preset number.
 */
int PT_DECLSPEC valsCfgSetDefaultPreset(PT_HANDLE *hp_vals_cfg, int i_default_preset)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->default_preset = i_default_preset;

	return(OKAY);
}  

/*
 * FUNCTION: valsCfgSetMinUserPreset()
 * DESCRIPTION:
 *   Sets the minimum user preset number.
 */
int PT_DECLSPEC valsCfgSetMinUserPreset(PT_HANDLE *hp_vals_cfg, int i_min_user_preset)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->min_user_preset = i_min_user_preset;

	return(OKAY);
} 

/*
 * FUNCTION: valsCfgSetPresetSaveTime()
 * DESCRIPTION:
 *   Sets the time that the last preset was saved. (Seconds since 1/1/70).
 */
int PT_DECLSPEC valsCfgSetPresetSaveTime(PT_HANDLE *hp_vals_cfg, long l_preset_save_time)
{
	struct valsCfgHdlType *cast_handle;

	cast_handle = (struct valsCfgHdlType *)hp_vals_cfg;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->preset_save_time = l_preset_save_time;

	return(OKAY);
}               
   
/*
 * FUNCTION: valsCfgFreeUp()
 * DESCRIPTION:
 *   Frees the passed vals configuration handle and sets to NULL.
 */
int PT_DECLSPEC valsCfgFreeUp(PT_HANDLE **hpp_vals_cfg)
{
	struct valsCfgHdlType *cast_handle;
    
	cast_handle = (struct valsCfgHdlType *)(*hpp_vals_cfg);

	if (cast_handle != NULL)
		free(cast_handle);

	*hpp_vals_cfg = NULL;

	return(OKAY);
}


