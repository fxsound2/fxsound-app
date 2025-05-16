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
#include "pt_defs.h"
#include "u_vals.h"
#include "GraphicEq.h"

#define LINE_LENGTH 128

/*
 * FUNCTION: valsSave()
 * DESCRIPTION:
 *   Save the handle to the passed file in the passed fullpath directory.
 */
int PT_DECLSPEC valsSave(PT_HANDLE *hp_vals, wchar_t *wcp_dir_path, wchar_t *wcp_filename)
{
	FILE *stream;
	struct valsHdlType *cast_handle;
	int param_index;
	int element_index;
	int index;
	int i_cp_string_utf8_length;
	char *cp_string_utf8;
	wchar_t fullpath_str[PT_MAX_PATH_STRLEN];
	int i_num_bands;
	realtype r_center_freq;
	realtype r_boost_cut;
	int i_band_num;

	cast_handle = (struct valsHdlType *)(hp_vals);

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (wcp_dir_path == NULL)
		return(NOT_OKAY);

	/* Construct the fullpath */
	swprintf(fullpath_str, L"%s\\%s", wcp_dir_path, wcp_filename);

	/* Open the file for writing */
	stream = fileOpen_Wide(fullpath_str, L"w", cast_handle->slout_hdl);
	if (stream == NULL)
		return(NOT_OKAY);

	fwprintf(stream, L"CLASS1 : Effect Type\n");

	fwprintf(stream, L"%g: Version\n", cast_handle->file_version);

	if (cast_handle->wcp_comment == NULL)
		fwprintf(stream, L"\n");
	else
	{	
		// Write the comment (preset name) as a UTF-8 encoded string
		if (pstrCovertWideCharStringToUTF8String_WithAlloc(cast_handle->wcp_comment, &cp_string_utf8, &i_cp_string_utf8_length) != OKAY)
			return(NOT_OKAY);
		fprintf(stream, "%s\n", cp_string_utf8);
		free(cp_string_utf8);
		cp_string_utf8 = NULL;
	}

    /* Write the double params flags. (Old presets don't have it) */
    if (cast_handle->file_version > 1.0)
    {
       fwprintf(stream, L"%d: Double Params Flag\n", cast_handle->double_params);
    }

	fwprintf(stream, L"%d: Total number of elements\n", cast_handle->total_num_elements);

	/* Write the main params */
	for (param_index = 0; param_index < VALS_NUM_MAIN_PARAMS; param_index++)
	{
	   fwprintf(stream, L"%d: Main %d\n", cast_handle->main_params[param_index], 
	                                    param_index); 
	   if (cast_handle->double_params)
	   {
	      fwprintf(stream, L"%d: Main_2 %d\n", cast_handle->main_params_2[param_index], 
	                                       param_index); 
	   }
	}
	
	/* Write the element params */
	for (element_index = 0; element_index < cast_handle->total_num_elements; 
	     element_index++)
	{
	    fwprintf(stream, L"%d: Element Number\n", element_index);
		
		for (param_index = 0; param_index < VALS_NUM_ELEMENT_PARAMS; param_index++)
	    {
	       fwprintf(stream, L"   %d: Param %d\n", 
	               cast_handle->element_params[element_index][param_index], param_index);
           
           if (cast_handle->double_params)
           {
              fwprintf(stream, L"   %d: Param_2 %d\n", 
	                  cast_handle->element_params_2[element_index][param_index], 
	                  param_index);
           }
	    }
	}
	
	/* Write the application dependent info */
	fwprintf(stream, L"%d: Number of Application Dependent Integers\n", 
	        cast_handle->app_depend.num_ints);
	fwprintf(stream, L"%d: Number of Application Dependent Reals\n", 
	        cast_handle->app_depend.num_reals);
	fwprintf(stream, L"%d: Number of Application Dependent Strings\n", 
	        cast_handle->app_depend.num_strings);
    
    for (index = 0; index < cast_handle->app_depend.num_ints; index++)
    {
		 fwprintf(stream, L"%d: Integer[%d]\n", 
               cast_handle->app_depend.int_vals[index], index);
    }

    for (index = 0; index < cast_handle->app_depend.num_reals; index++)
    {
       fwprintf(stream, L"%g: Real[%d]\n", 
               cast_handle->app_depend.real_vals[index], index);
    }

    for (index = 0; index < cast_handle->app_depend.num_strings; index++)
    {
       fwprintf(stream, L"String[%d]:\n", index);
       if (cast_handle->app_depend.wcpp_strings[index] == NULL)
          fwprintf(stream, L"\n");
       else
          fwprintf(stream, L"%s\n", cast_handle->app_depend.wcpp_strings[index]);
    }				

	/* Write the Graphic EQ settings */
	if (cast_handle->hp_graphicEq != NULL)
	{
		if (GraphicEqGetNumBands(cast_handle->hp_graphicEq, &i_num_bands) != OKAY)
			return(NOT_OKAY);

		fwprintf(stream, L"%d: Number of EQ Bands\n", i_num_bands);

		fwprintf(stream, L"%d: On/Off Flag\n", cast_handle->i_eq_on);

		for (i_band_num = 1; i_band_num <= i_num_bands; i_band_num++)
		{
			if (GraphicEqGetBandCenterFrequency(cast_handle->hp_graphicEq, i_band_num, &r_center_freq) != OKAY)
				return(NOT_OKAY);
			if (GraphicEqGetBandBoostCut(cast_handle->hp_graphicEq, i_band_num, &r_boost_cut) != OKAY)
				return(NOT_OKAY);

			fwprintf(stream, L"Band %d\n", i_band_num);
			fwprintf(stream, L"   %g: CF\n", r_center_freq);
			fwprintf(stream, L"   %g: Boost/Cut\n", r_boost_cut);
		}
	}

	fclose(stream);

	return(OKAY);
}

/*
 * FUNCTION: valsCalcDateStrings()
 * DESCRIPTION:
 *   Fills in the first passed sting with an 11 character unique filename
 *   based on the current date and time.  The format is as follows:
 *
 *   DDMMYYSS.SSS  (The SS.SSS part is the number of seconds since
 *                  midnight.  It will befrom 0 - 86400)
 *
 *   The second string is filled in with a formatted representation of
 *   the date and time (eg. Sun Jan 3 15:14:13 1988)
 */
int PT_DECLSPEC valsCalcDateStrings(wchar_t *wcp_filename, wchar_t *wcp_formatted)
{
	time_t time_structure;
	struct tm *tm_struct_ptr;
	long seconds;
	wchar_t wcp_short_sec_str[8];
	wchar_t wcp_sec_str[8];
	wchar_t wcp_date_str[8];
	int length;

	/* Get the current time */
	time(&time_structure);
	tm_struct_ptr = localtime(&time_structure);
	mktime(tm_struct_ptr);

	/* Fill in formatted string */
	if (wcsftime(wcp_formatted, 24, L"%m/%d/%y %H:%M:%S", tm_struct_ptr) == 0)
		return(NOT_OKAY);

	/* Calculate number of seconds since midnight */
	seconds = (long)(tm_struct_ptr->tm_hour * 3600L) +
				 (long)(tm_struct_ptr->tm_min * 60L) +
				 (long)(tm_struct_ptr->tm_sec);

	if (wcsftime(wcp_date_str, 16, L"%m%d%y", tm_struct_ptr) == 0)
		return(NOT_OKAY);

	/*
	 * Fill in the five characters of the seconds portion of the string.
	 * This is tricky because we want to fill in leading 0's.  For example
	 * If seconds are only 12 we want it to be 00012.
	 */
	swprintf(wcp_short_sec_str, L"%ld", seconds);
	length = (int)wcslen(wcp_short_sec_str);
	if (length == 5)
		swprintf(wcp_sec_str, L"%c%c.%c%c%c",
			wcp_short_sec_str[0], wcp_short_sec_str[1],
			wcp_short_sec_str[2], wcp_short_sec_str[3], wcp_short_sec_str[4]);
	else if (length == 4)
		swprintf(wcp_sec_str, L"0%c.%c%c%c",
			wcp_short_sec_str[0], wcp_short_sec_str[1],
			wcp_short_sec_str[2], wcp_short_sec_str[3]);
	else if (length == 3)
		swprintf(wcp_sec_str, L"00.%c%c%c",
			wcp_short_sec_str[0], wcp_short_sec_str[1],
			wcp_short_sec_str[2]);
	else if (length == 2)
		swprintf(wcp_sec_str, L"00.0%c%c",
			wcp_short_sec_str[0], wcp_short_sec_str[1]);
	else if (length == 1)
		swprintf(wcp_sec_str, L"00.00%c",
			wcp_short_sec_str[0]);
	else if (length == 0)
		swprintf(wcp_sec_str, L"00.000");

	swprintf(wcp_filename, L"%s%s", wcp_date_str, wcp_sec_str);

	return(OKAY);
}

/*
 * FUNCTION: valsRead()
 * DESCRIPTION:
 *   Read in and allocate a new vals handle from the past file location.
 */
int PT_DECLSPEC valsRead(wchar_t *wcp_file_path, int i_trace_mode, CSlout *hp_slout, PT_HANDLE **hpp_vals)
{
	FILE *stream;
	char cp_str_utf8[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_str[PT_MAX_GENERIC_STRLEN];
	wchar_t* wcp_utf8_comment;
	int wcp_utf8_comment_length;
	struct valsHdlType *cast_handle;
	int str_length;
	int param_index;
	int element_index;
	int read_num;
	int num_ints;
	int num_reals;
	int num_strings;
	int index;
	int i_num_bands;
	realtype r_center_freq;
	realtype r_boost_cut;
	int i_band_num;
    
	if (wcp_file_path == NULL)
		return(NOT_OKAY);

	/* Allocate the handle */
	cast_handle = (struct valsHdlType *)calloc(1,
									 sizeof(struct valsHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);

	/* Initialize the handle */
	cast_handle->slout_hdl = hp_slout;
	cast_handle->i_trace_mode = i_trace_mode;

	if (cast_handle->i_trace_mode)
	{
		swprintf(cast_handle->wcp_msg1, L"valsRead(): Entered, wcp_file_path = %s", wcp_file_path);
		(cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
	}

	/* Open the file for reading */
	stream = fileOpen_Wide(wcp_file_path, L"r", hp_slout);
	if (stream == NULL)
		return(NOT_OKAY);

	/* Get the Effect Type */
	fgetws(wcp_str, LINE_LENGTH, stream);

	/* Get the version number */
	fgetws(wcp_str, LINE_LENGTH, stream);
	swscanf(wcp_str, L"%g\n", &(cast_handle->file_version));

	/* Get the Comment. The Comment can be Unicode encoded in UTF-8 so we can support
	   international Preset names. */
	fgets(cp_str_utf8, LINE_LENGTH, stream);
	if (pstrCovertUTF8StringToWideCharString_WithAlloc(cp_str_utf8, &wcp_utf8_comment, &wcp_utf8_comment_length) != OKAY)
		return(NOT_OKAY);
	swprintf(wcp_str, L"%s", wcp_utf8_comment);
	free(wcp_utf8_comment);
	wcp_utf8_comment = NULL;

	str_length = (int)wcslen(wcp_str);
	if (str_length > 0)
	{
		/* Get rid of the \n from the comment */
		wcp_str[str_length - 1] = L'\0';

		/* Save comment in structure */
		cast_handle->wcp_comment = (wchar_t *)calloc(1, ((wcslen(wcp_str) + 1)*(sizeof(wchar_t))));
		if (cast_handle->wcp_comment == NULL)
		{
			fclose(stream);
			return(NOT_OKAY);
		}
		swprintf(cast_handle->wcp_comment, L"%s", wcp_str);
	}
	else
		cast_handle->wcp_comment = NULL;

	/* 
	 * Get the double param flag. 
	 * Note: If it is an old vals handle, it will not be there.  We
	 *       will therefore assume false.
	 */
	if (cast_handle->file_version > 1.0)
    {
	   fgetws(wcp_str, LINE_LENGTH, stream);
	   swscanf(wcp_str, L"%d\n", &(cast_handle->double_params));
	}
    else
    {    
	   cast_handle->double_params = IS_FALSE;
    }
       
    /* Set the param set */
    cast_handle->param_set = VALS_PARAM_SET_1;   
       
	/* Get the total number of possible elements */
	fgetws(wcp_str, LINE_LENGTH, stream);
	swscanf(wcp_str, L"%d\n", &(cast_handle->total_num_elements));

	/* Read the main parameters */
	for (param_index = 0; param_index < VALS_NUM_MAIN_PARAMS; param_index++)
	{
	   fgetws(wcp_str, LINE_LENGTH, stream);
	   swscanf(wcp_str, L"%d\n", &(cast_handle->main_params[param_index]));
	   
	   if (cast_handle->double_params)
	   {
	   	fgetws(wcp_str, LINE_LENGTH, stream);
	      swscanf(wcp_str, L"%d\n", &(cast_handle->main_params_2[param_index]));
	   }
	}
	
	/* Read the element params */
	for (element_index = 0; element_index < cast_handle->total_num_elements; 
	     element_index++)
	{
		/* Read the element number */
		fgetws(wcp_str, LINE_LENGTH, stream);
		swscanf(wcp_str, L"%d\n", &read_num);
		
		if (read_num != element_index)
			return(NOT_OKAY);

		for (param_index = 0; param_index < VALS_NUM_ELEMENT_PARAMS; param_index++)
		{
			fgetws(wcp_str, LINE_LENGTH, stream);
			swscanf(wcp_str, L"%d\n", &(cast_handle->element_params[element_index][param_index]));
	       
			if (cast_handle->double_params)
			{
				fgetws(wcp_str, LINE_LENGTH, stream);
				swscanf(wcp_str, L"%d\n", &(cast_handle->element_params_2[element_index][param_index]));	       
			}
		}
	} 
	
	/* Read number of application dependent variables */
	fgetws(wcp_str, LINE_LENGTH, stream);
	swscanf(wcp_str, L"%d", &num_ints);	
	fgetws(wcp_str, LINE_LENGTH, stream);
	swscanf(wcp_str, L"%d", &num_reals);	
	fgetws(wcp_str, LINE_LENGTH, stream);
	swscanf(wcp_str, L"%d", &num_strings);		
    
	/* Allocate the application dependent variables */
	if (valsInitAppDependentInfo((PT_HANDLE *)cast_handle, num_ints, num_reals, 
	                             num_strings) != OKAY)
	   return(NOT_OKAY);
    
	/* Read the ints */
	for (index = 0; index < num_ints; index++)
	{
		fgetws(wcp_str, LINE_LENGTH, stream);
	   swscanf(wcp_str, L"%d", &(cast_handle->app_depend.int_vals[index]));   
	} 
    
	/* Read the reals */
	for (index = 0; index < num_reals; index++)
	{
		fgetws(wcp_str, LINE_LENGTH, stream);
	   swscanf(wcp_str, L"%g", &(cast_handle->app_depend.real_vals[index]));   
	} 
    
	/* Read the strings */
	for (index = 0; index < num_strings; index++)
	{
		/* Get the string, (Note: We skip the line which says the index) */
		fgetws(wcp_str, LINE_LENGTH, stream);
		fgetws(wcp_str, LINE_LENGTH, stream);
		str_length = (int)wcslen(wcp_str);
	   if (str_length > 0)
	   {
			/* Get rid of the \n from the string */
			wcp_str[str_length - 1] = L'\0';

			/* Store the string */          
	      if (valsSetAppDependentString((PT_HANDLE *)cast_handle, index, 
	                                    wcp_str) != OKAY)
				return(NOT_OKAY);
		}
		else
	      cast_handle->app_depend.wcpp_strings[index] = NULL;
	}     
    
	/* Read the Graphic EQ settings */
	if (cast_handle->file_version >= 9)
	{
		/* Free up the old graphicEq in handle if it exists */
		if (cast_handle->hp_graphicEq != NULL)
		{
			if (cast_handle->i_trace_mode)
			{
				swprintf(cast_handle->wcp_msg1, L"valsRead(): Calling GraphicEqFreeUp()");
				(cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
			}

			if (GraphicEqFreeUp(&cast_handle->hp_graphicEq) != OKAY)
				return(NOT_OKAY);
		}

		/* Read the number of bands */
		fgetws(wcp_str, LINE_LENGTH, stream);
	   swscanf(wcp_str, L"%d", &i_num_bands);   

		/* Read the eq on/off flag */
		fgetws(wcp_str, LINE_LENGTH, stream);
		swscanf(wcp_str, L"%d", &(cast_handle->i_eq_on));

		/* Create the graphicEq handle */
		if (GraphicEqNew(&cast_handle->hp_graphicEq, i_num_bands, i_trace_mode, hp_slout) != OKAY)
			return(NOT_OKAY);

		/* Read all the bands */
		for (i_band_num = 1; i_band_num <= i_num_bands; i_band_num++)
		{
			/* Skip the band number line */
			fgetws(wcp_str, LINE_LENGTH, stream);

			/* Read the center freq */
			fgetws(wcp_str, LINE_LENGTH, stream);
			swscanf(wcp_str, L"%g", &r_center_freq);  

			/* Read the boost/cut */
			fgetws(wcp_str, LINE_LENGTH, stream);
			swscanf(wcp_str, L"%g", &r_boost_cut); 

			/* Store the band settings */
			if (GraphicEqSetBandBoostCut(cast_handle->hp_graphicEq, i_band_num, r_boost_cut) != OKAY)
				return(NOT_OKAY);
			if (GraphicEqSetBandFreq(cast_handle->hp_graphicEq, i_band_num, r_center_freq) != OKAY)
				return(NOT_OKAY);
		}
	}

	fclose(stream);

	*hpp_vals = (PT_HANDLE *)cast_handle;

	return(OKAY);
}

/*
 * FUNCTION: valsCheckFileType()
 * DESCRIPTION:
 *   Checks if the passed file is the proper type of effect file.
 */
int PT_DECLSPEC valsCheckFileType(wchar_t *wcp_file_path, int *ip_proper_type, CSlout *hp_slout)
{
	FILE *stream;
	wchar_t wcp_line_str[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_type_str[PT_MAX_GENERIC_STRLEN];
	int exists;

	*ip_proper_type = IS_FALSE;

	if (wcp_file_path == NULL)
		return(NOT_OKAY);
                          
    /* Make sure the file exists */
    if (fileExist_Wide(wcp_file_path, &exists) != OKAY)
       return(NOT_OKAY);
    if (!exists)
       return(OKAY);                     
                          
	/* Open the file for reading */
	stream = fileOpen_Wide(wcp_file_path, L"r", hp_slout);
	if (stream == NULL)
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

	if (!(wcscmp(wcp_type_str, L"CLASS1")))
	{
		*ip_proper_type = IS_TRUE;
		fclose(stream);
	}

	fclose(stream);

	return(OKAY);
}
