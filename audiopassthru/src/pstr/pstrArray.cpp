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
#include <windows.h>
#include <winbase.h>
#include <dos.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "slout.h"
#include "pstr.h"
#include "ptime.h"
#include "file.h"
#include "mry.h"
#include "pt_defs.h"

/*
 * FUNCTION: pstrAddStringToStringList_Wide()
 * DESCRIPTION:
 *
 *  Adds the passed string to the passed string array.
 *
 */
int PT_DECLSPEC pstrAddStringToStringList_Wide(wchar_t ***wcppp_array, int i_original_array_length,
														wchar_t *wcp_new_string, int i_allow_duplicates,
														int i_sort, int i_case_sensitive, int i_max_strlen,
														int *ip_new_array_length)
{
	int already_exists;

	/* Set the array length in case the strign does not get added. */
	*ip_new_array_length = i_original_array_length;

	if (wcppp_array == NULL)
		return(NOT_OKAY);
	if (wcp_new_string == NULL)
		return(NOT_OKAY);

	/* If we are not supposed to allow duplicates, make sure the string does not already exist */
	if (!i_allow_duplicates)
	{
      if (pstrCheckInArray_Wide(wcp_new_string, *wcppp_array, i_original_array_length, 
									i_case_sensitive, i_sort, &already_exists) != OKAY)
		   return(NOT_OKAY);

		if (already_exists)
			return(OKAY);
	}

	/* If array has never been allocated, then allocate it now */
	if (*wcppp_array == NULL)
	{
		/* Initially allocate the array list */
      *wcppp_array = (wchar_t **)calloc(1, sizeof(wchar_t *));
	}
	else
	{
		/* Add one to the length of the array strings */
      *wcppp_array = (wchar_t **)realloc(*wcppp_array, 
	      (i_original_array_length + 1) * sizeof(wchar_t *));
	}

	if (*wcppp_array == NULL)
      return(NOT_OKAY);

	/* Increment the number of strings */
   *ip_new_array_length = i_original_array_length + 1;

	/* Allocate the space for the new string in the array */
   (*wcppp_array)[i_original_array_length] = (wchar_t *)calloc(i_max_strlen, sizeof(wchar_t));
	if ((*wcppp_array)[i_original_array_length] == NULL)
		return(NOT_OKAY);

	/* Copy the new string */
	swprintf((*wcppp_array)[i_original_array_length], L"%s", wcp_new_string);

	/* Sort the array if requested */
	if (i_sort)
	{
      if (pstrSortArray_Wide(*wcppp_array, *ip_new_array_length, i_case_sensitive) != OKAY)
			return(NOT_OKAY);
	}

   return(OKAY);
}

/*
 * FUNCTION: pstrArrayCopyStringList()
 * DESCRIPTION:
 *
 *  Copies the passed array of strings.
 *
 */
int PT_DECLSPEC pstrArrayCopyStringList(wchar_t **wcpp_original_array, 
													 int i_array_length,
													 int i_max_strlen,
													 wchar_t ***wcppp_new_array)
{
	int index;

	if (wcpp_original_array == NULL)
		return(NOT_OKAY);
	if (wcppp_new_array == NULL)
		return(NOT_OKAY);

	*wcppp_new_array = NULL;

	/* Allocate the new array list */
	*wcppp_new_array = (wchar_t **)calloc(i_array_length, sizeof(wchar_t *));

	for (index = 0; index < i_array_length; index++)
	{
		/* Allocate the space for the new string in the array */
		(*wcppp_new_array)[index] = (wchar_t *)calloc(i_max_strlen, sizeof(wchar_t));
		if ((*wcppp_new_array)[index] == NULL)
			return(NOT_OKAY);

		/* Copy the new string */
		swprintf((*wcppp_new_array)[index], L"%s", wcpp_original_array[index]);
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrCheckInArray_Wide()
 * DESCRIPTION:
 *
 *  Checks to see if passed in string is in the passed array.
 *
 */
int PT_DECLSPEC pstrCheckInArray_Wide(wchar_t *wcp_string, wchar_t **wcpp_array, 
											int i_array_length, int i_case_sensitive, 
											int i_pre_sorted, int *ip_inside_array)
{
   int index;
	int done;
	int low_index;
	int mid_index;
	int high_index;
	int comparison;

	if (wcp_string == NULL)
		return(NOT_OKAY);

	if (i_array_length < 0)
		return(NOT_OKAY);

	*ip_inside_array = IS_FALSE;

   if (i_array_length == 0)
		return(OKAY);

	/* 
	 * If it is not pre-sorted then just do a sequential check
	 * for the string 
	 */
   if (!i_pre_sorted)
	{
	   for (index = 0; index < i_array_length; index++)
		{
		   if (wcpp_array[index] != NULL)
			{
			   /* Case sensitive check */
			   if (i_case_sensitive)
				{
				   /* Check for a match */
				   if (wcscmp(wcp_string, wcpp_array[index]) == 0)
					{
					   *ip_inside_array = IS_TRUE;
					   return(OKAY);
					}
				}
			   else /* Case insensitive check */
				{
					/* Check for a match */
				   if (_wcsicmp(wcp_string, wcpp_array[index]) == 0)
					{
					   *ip_inside_array = IS_TRUE;
					   return(OKAY);
					}
				}
			}
		}
	}
	else /* Pre-sorted binary search */
   {
		low_index = 0;
		high_index = i_array_length - 1;
		done = IS_FALSE;

		while (!done)
		{
			if (high_index < low_index)
			   done = IS_TRUE;
			else
			{
				mid_index = (low_index + high_index) / 2;
			   
				if (i_case_sensitive)
				{
					comparison = wcscmp(wcp_string, wcpp_array[mid_index]);
				}
				else
				{			   
					comparison = _wcsicmp(wcp_string, wcpp_array[mid_index]);
				}

				if (comparison == 0)
				{
					*ip_inside_array = IS_TRUE;
					done = IS_TRUE;
				}
				else if (comparison < 0)
				{
					high_index = mid_index - 1;
				}
				else 
				{
					low_index = mid_index + 1;
				}

			}
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrArrayFilterByExtension_Wide()
 * DESCRIPTION:
 *  Returns a new list (cppp_new_array) containing all fullpath file names 
 *  from cpp_original_array excluding the file names with extension of cp_extension.
 *
 */
int PT_DECLSPEC pstrArrayFilterByExtension_Wide(wchar_t **wcpp_original_array, int i_num_orig_elements, 
										   wchar_t *wcp_extension, wchar_t ***wcppp_new_array, 
										   int *ip_num_new_elements)
{
	if (wcpp_original_array == NULL)
		return(NOT_OKAY);
	if (wcp_extension == NULL)
		return(NOT_OKAY);
	if (wcppp_new_array == NULL)
		return(NOT_OKAY);
	if (ip_num_new_elements == NULL)
		return(NOT_OKAY);

	// Reset ip_num_new_elements in case it is not zero
	*ip_num_new_elements = 0;

	wchar_t wcp_current_extension[PT_MAX_GENERIC_STRLEN];
	int i_num_new_array_length;	
	int result;
	int index;

	i_num_new_array_length = 0;
	*wcppp_new_array = NULL;

	for(index = 0; index < i_num_orig_elements; index++)
	{
		// Extract the extension from the full path file
 		if(pstrCalcExtension_Wide(wcpp_original_array[index], wcp_current_extension, PT_MAX_GENERIC_STRLEN) != OKAY) // No extension, skip
			continue;
		// Case-insensitive string compare
		result = _wcsicmp(wcp_current_extension, wcp_extension);

		if(result != 0) // Strings Not equal
		{
			// Append the full path to new array
			if(pstrAddStringToStringList_Wide(wcppp_new_array, i_num_new_array_length,
														wcpp_original_array[index], IS_TRUE,
														IS_FALSE, IS_FALSE, 512,
														ip_num_new_elements) != OKAY)
				return(NOT_OKAY);
			i_num_new_array_length = *ip_num_new_elements;
		}
	}
	
	return(OKAY);
}

/*
 * FUNCTION: pstrArrayFilterByPrefix_Wide()
 * DESCRIPTION:
 *  Returns a new list (cppp_new_array) containing all fullpath file names 
 *  from cpp_original_array excluding the file names with specified prefix.
 *
 */
int PT_DECLSPEC pstrArrayFilterByPrefix_Wide(wchar_t **wcpp_original_array, int i_num_orig_elements, 
										wchar_t *wcp_prefix, wchar_t ***wcppp_new_array, 
										int *ip_num_new_elements)
{
	if(wcpp_original_array == NULL)
		return(NOT_OKAY);
	if(wcp_prefix == NULL)
		return(NOT_OKAY);
	if(wcppp_new_array == NULL)
		return(NOT_OKAY);
	if(ip_num_new_elements == NULL)
		return(NOT_OKAY);

	// Reset ip_num_new_elements in case it is not zero
	*ip_num_new_elements = 0;

	int i_num_new_array_length;
	int starts_with_prefix;
	int index;

	i_num_new_array_length = 0;
	*wcppp_new_array = NULL;

	for(index = 0; index < i_num_orig_elements; index++)
	{
		/* Check if the fullpath starts with the prefix */
		if (pstrCheckStartsWithMatch_Wide(wcpp_original_array[index], wcp_prefix, &starts_with_prefix) != OKAY)
			return(NOT_OKAY);

		if (!starts_with_prefix) // Not start with cp_prefix, then we add the fullpath string to the new array.
		{
			// Append the full path to new array
			if(pstrAddStringToStringList_Wide(wcppp_new_array, i_num_new_array_length,
														 wcpp_original_array[index], IS_TRUE,
														 IS_FALSE, IS_FALSE, 512,
														 ip_num_new_elements) != OKAY)
				return(NOT_OKAY);
			i_num_new_array_length = *ip_num_new_elements;
		}
	}
	
	return(OKAY);
}

/*
 * FUNCTION: pstrArrayFilterByFilename_Wide()
 * DESCRIPTION:
 *  Returns a new list (cppp_new_array) containing all fullpath file names 
 *  from cpp_original_array excluding the file names with specified filename.
 *
 */
int PT_DECLSPEC pstrArrayFilterByFilename_Wide(wchar_t **wcpp_original_array, int i_num_orig_elements, 
										wchar_t *wcp_filename, wchar_t ***wcppp_new_array, 
										int *ip_num_new_elements)
{
	if(wcpp_original_array == NULL)
		return(NOT_OKAY);
	if(wcp_filename == NULL)
		return(NOT_OKAY);
	if(wcppp_new_array == NULL)
		return(NOT_OKAY);
	if(ip_num_new_elements == NULL)
		return(NOT_OKAY);

	// Reset ip_num_new_elements in case it is not zero
	*ip_num_new_elements = 0;

	wchar_t wcp_current_directory[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_current_filename[PT_MAX_GENERIC_STRLEN];
	int i_num_new_array_length;
	int index;
	int result;

	i_num_new_array_length = 0;
	*wcppp_new_array = NULL;

	for(index = 0; index < i_num_orig_elements; index++)
	{
		if( fileSplitFullpath_Wide(wcpp_original_array[index], wcp_current_directory, wcp_current_filename, NULL) != OKAY )
			return(NOT_OKAY);
	
		// Case-insensitive string compare
		result = _wcsicmp(wcp_filename, wcp_current_filename);

		if(result != 0) // Strings Not equal
		{
			// Append the full path to new array
			if(pstrAddStringToStringList_Wide(wcppp_new_array, i_num_new_array_length,
														wcpp_original_array[index], IS_TRUE,
														IS_FALSE, IS_FALSE, 512,
														ip_num_new_elements) != OKAY)
				return(NOT_OKAY);
			i_num_new_array_length = *ip_num_new_elements;
		}
	}
	
	return(OKAY);
}

/*
 * FUNCTION: pstrArrayFilterByModifiedDate_Wide()
 * DESCRIPTION:
 *  Returns a new list (cppp_new_array) containing all fullpath file names 
 *  from cpp_original_array only if the file's modified datetime is later 
 *  than l_utc_time (unix timestamp).
 *
 *	 The i_remove_non_existing_files flag specifies whether or not to remove from the list
 *  files paths which do not exist.
 *
 *
 */
int PT_DECLSPEC pstrArrayFilterByModifiedDate_Wide(wchar_t **wcpp_original_array, int i_num_orig_elements, 
											  FILETIME* pfiletimeCompare, int i_remove_non_existing_files,
											  wchar_t ***wcppp_new_array, int *ip_num_new_elements, CSlout *hp_slout)
{
	int i_num_new_array_length;
	int index;
	FILETIME filetimeFileModified;
	LONG l_result;
	int add_to_list;
	int file_exists;

	if(wcpp_original_array == NULL)
		return(NOT_OKAY);
	if(wcppp_new_array == NULL)
		return(NOT_OKAY);
	if(ip_num_new_elements == NULL)
		return(NOT_OKAY);
	
	// Reset ip_num_new_elements in case it is not zero
	*ip_num_new_elements = 0;
	*wcppp_new_array = NULL;

	i_num_new_array_length = 0;

	for(index = 0; index < i_num_orig_elements; index++)
	{
		add_to_list = IS_FALSE;

		// Check if the file exists 
		if (fileExist_Wide(wcpp_original_array[index], &file_exists) != OKAY)
			return(NOT_OKAY);

		if (!file_exists)
		{
			if (!i_remove_non_existing_files)
				add_to_list = IS_TRUE;
		}
		else
		{
			// Get the file's modified datetime 
			if (fileGetModifiedDate_Wide(wcpp_original_array[index], &filetimeFileModified, hp_slout) != OKAY)
				return(NOT_OKAY);

			// Now we compare
			l_result = CompareFileTime(&filetimeFileModified, pfiletimeCompare);

			if(l_result == 1) // file's modified time is later than the specified time
				add_to_list = IS_TRUE;
		}

		if (add_to_list)
		{
			// Append the full path to new array
			if(pstrAddStringToStringList_Wide(wcppp_new_array, i_num_new_array_length,
														wcpp_original_array[index], IS_TRUE,
														IS_FALSE, IS_FALSE, 512,
														ip_num_new_elements) != OKAY)
				return(NOT_OKAY);
			i_num_new_array_length = *ip_num_new_elements;
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrArrayRemoveTrailingBackslashes_Wide()
 * DESCRIPTION:
 *  Remove trailing backslashes from all the elements of the passed string array.
 *
 */
int PT_DECLSPEC pstrArrayRemoveTrailingBackslashes_Wide(wchar_t **wcpp_str_array, 
																	int i_num_elements, 
																	CSlout *hp_slout)
{
	int index;

	if(wcpp_str_array == NULL)
		return(NOT_OKAY);
	if(i_num_elements <= 0)
		return(OKAY);

	for (index = 0; index < i_num_elements; index++)
	{
		if (pstrRemoveTrailingBackslash_Wide(wcpp_str_array[index]) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrArrayPrependString_Wide()
 * DESCRIPTION:
 *  Prepend a string prefix to all the elements of the passed string array.
 *
 */
int PT_DECLSPEC pstrArrayPrependString_Wide(wchar_t ***wcppp_str_array,
													int i_num_elements,
													wchar_t *wcp_prefix_to_prepend,
													CSlout *hp_slout)
{
	wchar_t **wcpp_new_str_array;
	wchar_t wcp_temp_string[PT_MAX_GENERIC_STRLEN];
	int index;
	int i_old_num_elements;
	int i_new_num_elements;

	if(wcppp_str_array == NULL)
		return(NOT_OKAY);
	if(wcp_prefix_to_prepend == NULL)
		return(NOT_OKAY);
	if(i_num_elements <= 0)
		return(OKAY);

	wcpp_new_str_array = NULL;
	i_old_num_elements = 0;
	i_new_num_elements = 0;

	for (index = 0; index < i_num_elements; index++)
	{
		swprintf(wcp_temp_string, L"%s%s", wcp_prefix_to_prepend, (*wcppp_str_array)[index]);

		if (pstrAddStringToStringList_Wide(&wcpp_new_str_array, i_old_num_elements,
												wcp_temp_string, IS_TRUE,
												IS_FALSE, IS_FALSE, 512, &i_new_num_elements) != OKAY)
			return(NOT_OKAY);
		i_old_num_elements = i_new_num_elements;
	}
	
	// Free original array of strings
	if (mryFreeUpStringArray_Wide(wcppp_str_array, i_num_elements) != OKAY)
			return(NOT_OKAY);
	// Reassign the new array to be passed back
	*wcppp_str_array = wcpp_new_str_array;
	
	return(OKAY);
}

/*
 * FUNCTION: pstrArrayReplacePrefix_Wide()
 * DESCRIPTION:
 *  Replace cp_source_prefix with cp_new_prefix to all the elements of the passed string array.
 *
 */
int PT_DECLSPEC pstrArrayReplacePrefix_Wide(wchar_t ***wcppp_str_array,
													int i_num_elements,
													wchar_t *wcp_source_prefix,
													wchar_t *wcp_new_prefix,
													CSlout *hp_slout)
{
	wchar_t **wcpp_new_str_array;
	wchar_t wcp_without_prefix[PT_MAX_GENERIC_STRLEN];
	wchar_t wcp_temp_string[PT_MAX_GENERIC_STRLEN];
	int index;
	int i_old_num_elements;
	int i_new_num_elements;
	int starts_with_prefix;
	
	if(wcppp_str_array == NULL)
		return(NOT_OKAY);
	if(wcp_source_prefix == NULL)
		return(NOT_OKAY);
	if(wcp_new_prefix == NULL)
		return(NOT_OKAY);
	if(i_num_elements <= 0)
		return(OKAY);

	wcpp_new_str_array = NULL;
	i_old_num_elements = 0;
	i_new_num_elements = 0;

	for (index = 0; index < i_num_elements; index++)
	{
		/* Check if the fullpath starts with the prefix */
		if (pstrCheckStartsWithMatch_Wide((*wcppp_str_array)[index], wcp_source_prefix, &starts_with_prefix) != OKAY)
			return(NOT_OKAY);

		if (starts_with_prefix) // If start with cp_source_prefix, replace it with cp_new_prefix
		{
				if(pstrRemovePrefixFromLine_Wide((*wcppp_str_array)[index], wcp_source_prefix, wcp_without_prefix) != OKAY)
					return(NOT_OKAY);

				swprintf(wcp_temp_string, L"%s%s", wcp_new_prefix, wcp_without_prefix);

				if (pstrAddStringToStringList_Wide(&wcpp_new_str_array, i_old_num_elements,
												wcp_temp_string, IS_TRUE,
												IS_FALSE, IS_FALSE, PT_MAX_GENERIC_STRLEN, &i_new_num_elements) != OKAY)
					return(NOT_OKAY);
				i_old_num_elements = i_new_num_elements;
		}
	}
	
	// Free original array of strings
	if (mryFreeUpStringArray_Wide(wcppp_str_array, i_num_elements) != OKAY)
			return(NOT_OKAY);
	// Reassign the new array to be passed back
	*wcppp_str_array = wcpp_new_str_array;
	
	return(OKAY);
}

/*
 * FUNCTION: pstrArrayCombine_Wide()
 * DESCRIPTION:
 *  Combine the two arrays of strings into one new array of strings.
 *
 */
int PT_DECLSPEC pstrArrayCombine_Wide(wchar_t **wcpp_original_array1, int i_num_orig_elements_1, 
											wchar_t **wcpp_original_array2, int i_num_orig_elements_2, 
											int i_allow_duplicates, int i_sort, int i_case_sensitive,
											int i_max_strlen,
											wchar_t ***wcppp_new_combined_array, int *ip_new_total_num_elements)
{
	int index;

	*ip_new_total_num_elements = 0;
	*wcppp_new_combined_array = NULL;

	for (index = 0; index < i_num_orig_elements_1; index++)
	{
		if (pstrAddStringToStringList_Wide(wcppp_new_combined_array, *ip_new_total_num_elements,
												wcpp_original_array1[index], i_allow_duplicates,
												i_sort, i_case_sensitive, i_max_strlen, ip_new_total_num_elements) != OKAY)
			return(NOT_OKAY);

	}

	for (index = 0; index < i_num_orig_elements_2; index++)
	{
		if (pstrAddStringToStringList_Wide(wcppp_new_combined_array, *ip_new_total_num_elements,
												wcpp_original_array2[index], i_allow_duplicates,
												i_sort, i_case_sensitive, i_max_strlen, ip_new_total_num_elements) != OKAY)
			return(NOT_OKAY);

	}

	return(OKAY);
}

/*
 * FUNCTION: pstrFreeArrayOfStrings_Wide()
 * DESCRIPTION:
 *
 * Frees the passed array of strings.
 * 
 */
int  PT_DECLSPEC pstrFreeArrayOfStrings_Wide(wchar_t **wcpp_array, int i_num_strings, CSlout *hp_slout)
{
	int index;

	if (wcpp_array == NULL)
		return(OKAY);

	for (index = 0; index < i_num_strings; index++)
	{
		if (wcpp_array[index] != NULL)
		{
			free(wcpp_array[index]);
		}
   }

	free(wcpp_array);

	return(OKAY);
}