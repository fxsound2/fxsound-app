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
#include "pt_defs.h"

/*
 * FUNCTION: pstrCalcLocationOfCharInStr() 
 * DESCRIPTION:
 *
 * Checks if the passed character is in the passed string, and if so,
 * passes back the index location of first occurance.
 */
int PT_DECLSPEC pstrCalcLocationOfCharInStr(char *cp_string, char c_char, int *ip_location, int *ip_found)
{
	wchar_t *wcp_string;
	wchar_t wc_char;
	int wide_char_buffer_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_string, &wcp_string, &wide_char_buffer_size) != OKAY)
		return(NOT_OKAY);
	wc_char = c_char;

	if(pstrCalcLocationOfCharInStr_Wide(wcp_string, wc_char, ip_location, ip_found) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}
	
	free(wcp_string);
	
	return(OKAY);
}

/*
 * FUNCTION: pstrCalcLocationOfCharInStr_Wide() 
 * DESCRIPTION:
 *
 * Checks if the passed character is in the passed string, and if so,
 * passes back the index location of first occurance.
 */
int PT_DECLSPEC pstrCalcLocationOfCharInStr_Wide(wchar_t *wcp_string, wchar_t wc_char, int *ip_location, int *ip_found)
{
	int length;
	int index;

	if (wcp_string == NULL)
		return(NOT_OKAY);

	*ip_found = IS_FALSE;

	length = (int)wcslen(wcp_string);

	if (length <= 0)
      return(OKAY);

	for (index = 0; index < length; index++)
	{
		if (wcp_string[index] == wc_char)
		{
			*ip_location = index;
			*ip_found = IS_TRUE;
			return(OKAY);
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrCalcLocationOfStrInStr_Wide() 
 * DESCRIPTION:
 *
 * Checks if the passed string (needle) is in the passed string (haystack) starting at i_start_search_index, and if so,
 * passes back the index location of first occurance.
 */
int PT_DECLSPEC pstrCalcLocationOfStrInStr_Wide(wchar_t *wcp_haystack, wchar_t* wcp_needle, 
										   int i_start_search_index, 
										   int *ip_location, int *ip_found)
{
	wchar_t * wcp_index;
	
	if (wcp_haystack == NULL || wcp_needle == NULL)
		return(NOT_OKAY);

	if (i_start_search_index < 0)
		return(NOT_OKAY);

	*ip_found = IS_FALSE;
	*ip_location = -1;
  
	wcp_index = wcsstr(wcp_haystack + i_start_search_index, wcp_needle);
	if ( wcp_index != NULL ) // Needle is found
	{
		*ip_location = (int)(wcp_index - wcp_haystack);
		*ip_found = IS_TRUE;
	}else // Needle not found
	{
		*ip_found = IS_FALSE;
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrConstructSubstring() 
 * DESCRIPTION:
 *   Construct a substring of the passed main string which starts at
 *   the passed location and ends with the passed character or EOL.
 *   The i_include_char flag says whether or not to include the seperation
 *   character in the substring.  It also passes back the location in the
 *   main string where the character was found.
 *
 */
int PT_DECLSPEC pstrConstructSubstring(char *cp_main_str, int i_start_index, 
								       char c_end_char, int i_include_char,
									   char *cp_sub_str,
									   int *ip_found_loc)
{
	wchar_t *wcp_main_str;
	wchar_t *wcp_sub_str;
	wchar_t wc_end_char;
	int i_main_str_widechar_buffer_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_main_str, &wcp_main_str, &i_main_str_widechar_buffer_size) != OKAY)
		return(NOT_OKAY);
		
	wc_end_char = c_end_char;
	
	wcp_sub_str = (wchar_t *)calloc(strlen(cp_main_str) + 1, sizeof(wchar_t));
	if (wcp_sub_str == NULL)
	{
		free(wcp_main_str);
		return(NOT_OKAY);
	}

	if(pstrConstructSubstring_Wide(wcp_main_str, i_start_index, wc_end_char, i_include_char, wcp_sub_str, ip_found_loc) != OKAY)
	{
		free(wcp_main_str);
		free(wcp_sub_str);
		return(NOT_OKAY);
	}
	
	if(pstrConvertWideCharStringToAnsiCharString(wcp_sub_str, cp_sub_str, strlen(cp_main_str) + 1) != OKAY)
	{
		free(wcp_main_str);
		free(wcp_sub_str);
		return(NOT_OKAY);
	}
	
	free(wcp_main_str);
	free(wcp_sub_str);

	return(OKAY);

}

/*
 * FUNCTION: pstrConstructSubstring_Wide() 
 * DESCRIPTION:
 *   Construct a substring of the passed main string which starts at
 *   the passed location and ends with the passed character or EOL.
 *   The i_include_char flag says whether or not to include the seperation
 *   character in the substring.  It also passes back the location in the
 *   main string where the character was found.
 *
 */
int PT_DECLSPEC pstrConstructSubstring_Wide(wchar_t *wcp_main_str, int i_start_index, 
								   wchar_t wc_end_char, int i_include_char,
									wchar_t *wcp_sub_str,
									int *ip_found_loc)
{
	int main_index;
	int sub_index;
   int done;
   int main_str_length;

	if ((wcp_main_str == NULL) || (wcp_sub_str == NULL))
      return(NOT_OKAY);

	main_str_length = (int)wcslen(wcp_main_str);

   if (i_start_index >= main_str_length)
	   return(NOT_OKAY);

   main_index = i_start_index;
	sub_index = 0;
   wcp_sub_str[0] = L'\0';
   *ip_found_loc = 0;
   done = IS_FALSE;

	while (!done)
	{
	   if (main_index == main_str_length)
			done = IS_TRUE;
		else if (wcp_main_str[main_index] == wc_end_char)
		{
         done = IS_TRUE;
         if (i_include_char)
			{
			   wcp_sub_str[sub_index] = wc_end_char;
				wcp_sub_str[sub_index + 1] = L'\0';
			}
		}
		else if (wcp_main_str[main_index] == L'\0')
			done = IS_TRUE;
		else
		{
	      wcp_sub_str[sub_index] = wcp_main_str[main_index];
			wcp_sub_str[sub_index + 1] = L'\0';
		}
		
		main_index++;
		sub_index++;
	}

	*ip_found_loc = main_index - 1;

   return(OKAY);
}

/*
 * FUNCTION: pstrToUpper() 
 * DESCRIPTION:
 *  Convert the passed string to upper case.
 */
int PT_DECLSPEC pstrToUpper(char *cp_original_str, char *cp_upper_str)
{
	wchar_t *wcp_original_str;
	wchar_t *wcp_upper_str;
	int i_original_str_widechar_size;

	if (cp_original_str == NULL)
		return(NOT_OKAY);
	if (cp_upper_str == NULL)
		return(NOT_OKAY);

	wcp_upper_str = (wchar_t *)calloc(strlen(cp_original_str) + 1, sizeof(wchar_t));

	if (wcp_upper_str == NULL)
		return(NOT_OKAY);

	if (pstrConvertToWideCharString_WithAlloc(cp_original_str, &wcp_original_str, &i_original_str_widechar_size) != OKAY)
	{
		free(wcp_upper_str);
		return(NOT_OKAY);
	}

	if (pstrToUpper_Wide(wcp_original_str, wcp_upper_str) != OKAY)
	{
		free(wcp_original_str);
		free(wcp_upper_str);
		return(NOT_OKAY);
	}

	if (pstrConvertWideCharStringToAnsiCharString(wcp_upper_str, cp_upper_str, strlen(cp_original_str) + 1) != OKAY)
	{
		free(wcp_original_str);
		free(wcp_upper_str);
		return(NOT_OKAY);
	}

	free(wcp_original_str);
	free(wcp_upper_str);

	return(OKAY);
}

/*
 * FUNCTION: pstrToUpper_Wide() 
 * DESCRIPTION:
 *  Convert the passed string to upper case.
 */
int PT_DECLSPEC pstrToUpper_Wide(wchar_t *wcp_original_str, wchar_t *wcp_upper_str)
{
	int index;
	int done;

	if (wcp_original_str == NULL)
		return(NOT_OKAY);

	if (wcp_upper_str == NULL)
		return(NOT_OKAY);

	if (wcslen(wcp_original_str) == 0)
		return(OKAY);

	index = 0;
	done = IS_FALSE;
   while (!done)
	{
		wcp_upper_str[index] = towupper(wcp_original_str[index]);

		if (wcp_original_str[index] == L'\0')
			done = IS_TRUE;

		index++;
	}

   return(OKAY);
}

/*
 * FUNCTION: pstrRemovePrefixFromLine()
 * DESCRIPTION:
 *
 * Remove the passed prefix from the passed string which is a line
 * which ends with either '\r', '\n', or '\0'.   Passes back the result.
 *
 */
int PT_DECLSPEC pstrRemovePrefixFromLine(char *cp_with_prefix, char *cp_prefix,
							             char *cp_without_prefix)
{
	wchar_t *wcp_with_prefix;
	wchar_t *wcp_prefix;
	wchar_t *wcp_without_prefix;
	int i_with_prefix_widechar_size;
	int i_prefix_widechar_size;
	
	wcp_without_prefix = (wchar_t *)calloc(strlen(cp_with_prefix) + 1, sizeof(wchar_t));
	
	if(!wcp_without_prefix)
		return(NOT_OKAY);
	
	if(pstrConvertToWideCharString_WithAlloc(cp_with_prefix, &wcp_with_prefix, &i_with_prefix_widechar_size) != OKAY)
	{
		free(wcp_without_prefix);
		return(NOT_OKAY);
	}
	if(pstrConvertToWideCharString_WithAlloc(cp_prefix, &wcp_prefix, &i_prefix_widechar_size) != OKAY)
	{
		free(wcp_without_prefix);
		free(wcp_with_prefix);
		return(NOT_OKAY);
	}

	if(pstrRemovePrefixFromLine_Wide(wcp_with_prefix, wcp_prefix, wcp_without_prefix) != OKAY)
	{
		free(wcp_with_prefix);
		free(wcp_prefix);
		free(wcp_without_prefix);
		return(NOT_OKAY);
	}
	
	if(pstrConvertWideCharStringToAnsiCharString(wcp_without_prefix, cp_without_prefix, strlen(cp_with_prefix) + 1) != OKAY)
	{
		free(wcp_with_prefix);
		free(wcp_prefix);
		free(wcp_without_prefix);
		return(NOT_OKAY);
	}
	
	free(wcp_with_prefix);
	free(wcp_prefix);
	free(wcp_without_prefix);

	return(OKAY);

}

/*
 * FUNCTION: pstrRemovePrefixFromLine_Wide()
 * DESCRIPTION:
 *
 * Remove the passed prefix from the passed string which is a line
 * which ends with either '\r', '\n', or '\0'.   Passes back the result.
 *
 */
int PT_DECLSPEC pstrRemovePrefixFromLine_Wide(wchar_t *wcp_with_prefix, wchar_t *wcp_prefix,
							                  wchar_t *wcp_without_prefix)
{
	int prefix_len;
	int with_prefix_len;
	int with_prefix_index;
	int without_prefix_index;
	int done;

	if (wcp_with_prefix == NULL)
		return(NOT_OKAY);
	if (wcp_prefix == NULL)
		return(NOT_OKAY);
	if (wcp_without_prefix == NULL)
		return(NOT_OKAY);

	/* Get the string lengths */
   with_prefix_len = (int)wcslen(wcp_with_prefix);
	prefix_len = (int)wcslen(wcp_prefix);
   
   /* Make sure the string is long enough to hold prefix */
	if (with_prefix_len <= prefix_len)
      return(NOT_OKAY);

	with_prefix_index = prefix_len;
	without_prefix_index = 0;
	done = IS_FALSE;
	while (!done)
	{
      if ((with_prefix_index == with_prefix_len) ||
			 (wcp_with_prefix[with_prefix_index] == L'\r') || 
			 (wcp_with_prefix[with_prefix_index] == L'\n'))
		{
		   wcp_without_prefix[without_prefix_index] = L'\0';
			done = IS_TRUE;
		}
		else
		{
	      wcp_without_prefix[without_prefix_index] = 
		      wcp_with_prefix[with_prefix_index];
		   with_prefix_index++;
		   without_prefix_index++;
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrRemovePostfixFromLine()
 * DESCRIPTION:
 *
 * Remove the passed postfix from the passed string which is a line
 * which ends with either '\r', '\n', or '\0'.   Passes back the result.
 *
 */
int PT_DECLSPEC pstrRemovePostfixFromLine(char *cp_with_postfix, char *cp_postfix,
							                     char *cp_without_postfix)
{
	wchar_t *wcp_with_postfix;
	wchar_t *wcp_postfix;
	wchar_t *wcp_without_postfix;
	int i_postfix_widechar_size;
	int i_with_postfix_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_with_postfix, &wcp_with_postfix, &i_with_postfix_widechar_size) != OKAY)
		return(NOT_OKAY);
	if(pstrConvertToWideCharString_WithAlloc(cp_postfix, &wcp_postfix, &i_postfix_widechar_size) != OKAY)
	{
		free(wcp_with_postfix);
		return(NOT_OKAY);
	}
		
	wcp_without_postfix = (wchar_t *)calloc(strlen(cp_with_postfix) + 1, sizeof(wchar_t));
	
	if(wcp_without_postfix == NULL)
	{
		free(wcp_with_postfix);
		free(wcp_postfix);
		return(NOT_OKAY);
	}

	if(pstrRemovePostfixFromLine_Wide(wcp_with_postfix, wcp_postfix, wcp_without_postfix) != OKAY)
	{
		free(wcp_with_postfix);
		free(wcp_postfix);
		free(wcp_without_postfix);
		return(NOT_OKAY);
	}

	if(pstrConvertWideCharStringToAnsiCharString(wcp_without_postfix, cp_without_postfix, strlen(cp_with_postfix) + 1) != OKAY)
	{
		free(wcp_with_postfix);
		free(wcp_postfix);
		free(wcp_without_postfix);
		return(NOT_OKAY);
	}
	
	free(wcp_with_postfix);
	free(wcp_postfix);
	free(wcp_without_postfix);

	return(OKAY);
	
}

/*
 * FUNCTION: pstrRemovePostfixFromLine_Wide()
 * DESCRIPTION:
 *
 * Remove the passed postfix from the passed string which is a line
 * which ends with either '\r', '\n', or '\0'.   Passes back the result.
 *
 */
int PT_DECLSPEC pstrRemovePostfixFromLine_Wide(wchar_t *wcp_with_postfix, wchar_t *wcp_postfix,
							                   wchar_t *wcp_without_postfix)
{
	int has_postfix;
	int length_with_postfix;
 	int length_postfix;
	int postfix_start_index;

   if (wcp_with_postfix == NULL)
		return(NOT_OKAY);

	if (wcp_without_postfix == NULL)
		return(NOT_OKAY);

   /* First make sure the string originally has the postfix */
   if (pstrCheckEndsWithMatch_Wide(wcp_with_postfix, wcp_postfix, &has_postfix) != OKAY)
		return(NOT_OKAY);

	/* If it does not have the postfix, do nothing */
	if (!has_postfix)
		return(OKAY);

	length_with_postfix = (int)wcslen(wcp_with_postfix);
	length_postfix = (int)wcslen(wcp_postfix);

	/* Calculate the index position of the main string at which the postfix starts */
   postfix_start_index = length_with_postfix - length_postfix;

	/* Do nothing if for some reason the index is negative (just to be safe) */
	if (postfix_start_index < 0)
		return(OKAY);

	/* Copy the proper chaacters to create the string without the postfix */
	if (postfix_start_index == 0)
		swprintf(wcp_without_postfix, L"");
	else
	{
      wcsncpy(wcp_without_postfix, wcp_with_postfix, postfix_start_index); 
		wcp_without_postfix[postfix_start_index] = L'\0';
	}

	return(OKAY);
}


/*
 * FUNCTION: pstrReplaceCharOccurances()
 * DESCRIPTION:
 *
 * Replace all the occurances of c_old_char in cp_string with c_new_char.
 *
 */
int PT_DECLSPEC pstrReplaceCharOccurances(char *cp_string, char c_old_char,
							                             char c_new_char)
{
	wchar_t *wcp_string;
	wchar_t wc_old_char;
	wchar_t wc_new_char;
	int i_string_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_string, &wcp_string, &i_string_widechar_size) != OKAY)
		return(NOT_OKAY);
	wc_old_char = c_old_char;
	wc_new_char = c_new_char;

	if(pstrReplaceCharOccurances_Wide(wcp_string, wc_old_char, wc_new_char) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}
	
	if(pstrConvertWideCharStringToAnsiCharString(wcp_string, cp_string, strlen(cp_string) + 1) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}
	
	free(wcp_string);
	return(OKAY);
}

/*
 * FUNCTION: pstrReplaceCharOccurances_Wide()
 * DESCRIPTION:
 *
 * Replace all the occurances of c_old_char in cp_string with c_new_char.
 *
 */
int PT_DECLSPEC pstrReplaceCharOccurances_Wide(wchar_t *wcp_string, wchar_t wc_old_char,
							                   wchar_t wc_new_char)
{
	int length;
	int index;

	if (wcp_string == NULL)
      return(NOT_OKAY);

   length = (int)wcslen(wcp_string);

	for (index = 0; index < length; index++)
	{
		if (wcp_string[index] == wc_old_char)
			wcp_string[index] = wc_new_char;
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveCharOccurances()
 * DESCRIPTION:
 *
 * Remove all the occurances of c_char_to_remove from cp_string.
 *
 */
int PT_DECLSPEC pstrRemoveCharOccurances(char *cp_string, char c_char_to_remove)
{
	wchar_t *wcp_string;
	wchar_t wc_char_to_remove;
	int i_string_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_string, &wcp_string, &i_string_widechar_size) != OKAY)
		return(NOT_OKAY);
	wc_char_to_remove = c_char_to_remove;

	if(pstrRemoveCharOccurances_Wide(wcp_string, wc_char_to_remove) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}
	
	if(pstrConvertWideCharStringToAnsiCharString(wcp_string, cp_string, strlen(cp_string) + 1) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}
	
	free(wcp_string);
	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveCharOccurances_Wide()
 * DESCRIPTION:
 *
 * Remove all the occurances of c_char_to_remove from cp_string.
 *
 */
int PT_DECLSPEC pstrRemoveCharOccurances_Wide(wchar_t *wcp_string, wchar_t wc_char_to_remove)
{
	int length_orig_string;
	int index_orig_string;
	int index_new_string;

	if (wcp_string == NULL)
      return(NOT_OKAY);

   length_orig_string = (int)wcslen(wcp_string);

	index_new_string = 0;

	for (index_orig_string = 0; index_orig_string < length_orig_string; index_orig_string++)
	{
		if (wcp_string[index_orig_string] != wc_char_to_remove)
		{
			wcp_string[index_new_string] = wcp_string[index_orig_string];
			index_new_string++;
		}
	}

	wcp_string[index_new_string] = L'\0';

	return(OKAY);
}

/*
 * FUNCTION: pstrReplaceCharOccurancesWithString()
 * DESCRIPTION:
 *
 * Replace all the occurances of c_search_char in cp_original_string with cp_replacement_string
 * and put result in cp_resulting_string.
 *
 */
int PT_DECLSPEC pstrReplaceCharOccurancesWithString(char *cp_original_string, char *cp_resulting_string,
												    int i_resulting_string_buffer_length,
													char c_search_char, char *cp_replacement_string)
{
	wchar_t *wcp_original_string;
	wchar_t *wcp_resulting_string;
	wchar_t *wcp_replacement_string;
	wchar_t wc_search_char;
	int i_original_string_widechar_size;
	int i_replacement_string_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_original_string, &wcp_original_string, &i_original_string_widechar_size) != OKAY)
		return(NOT_OKAY);
	if(pstrConvertToWideCharString_WithAlloc(cp_replacement_string, &wcp_replacement_string, &i_replacement_string_widechar_size) != OKAY)
	{
		free(wcp_original_string);
		return(NOT_OKAY);
	}
	wc_search_char = c_search_char;
	
	wcp_resulting_string = (wchar_t *)calloc(i_resulting_string_buffer_length, sizeof(wchar_t));
	if(wcp_resulting_string == NULL)
	{
		free(wcp_original_string);
		free(wcp_replacement_string);
		return(NOT_OKAY);
	}

	if(pstrReplaceCharOccurancesWithString_Wide(wcp_original_string, wcp_resulting_string, wc_search_char, wcp_replacement_string) != OKAY)
	{
		free(wcp_original_string);
		free(wcp_replacement_string);
		free(wcp_resulting_string);
		return(NOT_OKAY);
	}
	
	if(pstrConvertWideCharStringToAnsiCharString(wcp_resulting_string, cp_resulting_string, i_resulting_string_buffer_length) != OKAY)
	{
		free(wcp_original_string);
		free(wcp_replacement_string);
		free(wcp_resulting_string);
		return(NOT_OKAY);
	}
	
	free(wcp_original_string);
	free(wcp_replacement_string);
	free(wcp_resulting_string);
	
	return(OKAY);
}

/*
 * FUNCTION: pstrReplaceCharOccurancesWithString_Wide()
 * DESCRIPTION:
 *
 * Replace all the occurances of c_search_char in cp_original_string with cp_replacement_string
 * and put result in cp_resulting_string.
 *
 */
int PT_DECLSPEC pstrReplaceCharOccurancesWithString_Wide(wchar_t *wcp_original_string, wchar_t *wcp_resulting_string,
													     wchar_t wc_search_char, wchar_t *wcp_replacement_string)
{
	int length_original_string;
	int length_replacement_string;

	int index_original_string;
	int index_resulting_string;
	int index_replacement_string;

	if ((wcp_original_string == NULL) ||
		 (wcp_resulting_string == NULL) ||
		 (wcp_replacement_string == NULL))
      return(NOT_OKAY);

   length_original_string = (int)wcslen(wcp_original_string);
   length_replacement_string = (int)wcslen(wcp_replacement_string);

   index_resulting_string = 0;
	for (index_original_string = 0; 
	     index_original_string < length_original_string; 
		  index_original_string++)
	{
		/* Replace any occurrance of the search character */
		if (wcp_original_string[index_original_string] == wc_search_char)
		{
			for (index_replacement_string = 0; 
			     index_replacement_string < length_replacement_string; 
				  index_replacement_string++)
			{
				wcp_resulting_string[index_resulting_string] = 
					wcp_replacement_string[index_replacement_string];
				index_resulting_string++;
			}
		}
		else
		{
			/* Since no match, just copy the next character */
			wcp_resulting_string[index_resulting_string] =
				wcp_original_string[index_original_string];
			index_resulting_string++;
		}
	}

   /* End the resulting string */
	wcp_resulting_string[index_resulting_string] = L'\0';

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveSpaces()
 * DESCRIPTION:
 *
 *   Remove all the spaces from the passed string.
 *
 */
int PT_DECLSPEC pstrRemoveAllSpaces(char *cp_string)
{
	wchar_t *wcp_string;
	int wide_char_buffer_size;
	
	if (pstrConvertToWideCharString_WithAlloc(cp_string, &wcp_string, &wide_char_buffer_size) != OKAY)
		return(NOT_OKAY);

	if (pstrRemoveAllSpaces_Wide(wcp_string) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}

	if (pstrConvertWideCharStringToAnsiCharString(wcp_string, cp_string, strlen(cp_string) + 1) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}

	free(wcp_string);

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveAllSpaces_Wide()
 * DESCRIPTION:
 *
 *   Remove all the spaces from the passed string.
 *   NOTE: The string buffer passed can only be max of 512 characters.
 *
 */
int PT_DECLSPEC pstrRemoveAllSpaces_Wide(wchar_t *wcp_string)
{
	int old_length;
	int old_index;
	int new_index;
    wchar_t new_string[512];

	if (wcp_string == NULL)
      return(NOT_OKAY);

	new_index = 0;
   old_length = (int)wcslen(wcp_string);

	for (old_index = 0; old_index < old_length; old_index++)
	{
		if (wcp_string[old_index] != ' ')
		{
			new_string[new_index] = wcp_string[old_index];
		   new_index++;
		}
	}

   new_string[new_index] = L'\0';

	if (wcslen(new_string) > 0)
	   swprintf(wcp_string, L"%s", new_string);
	else
		wcp_string[0] = L'\0';

	return(OKAY);
}


/*
 * FUNCTION: pstrRemoveAllLeadingSpaces_Wide()
 * DESCRIPTION:
 *
 *   Remove all the spaces from the passed string.
 *   NOTE: The string buffer passed can only be max of 512 characters.
 *
 */
int PT_DECLSPEC pstrRemoveAllLeadingSpaces_Wide(wchar_t *wcp_string)
{
	int old_length;
	int old_index;
	int new_index;
	wchar_t new_string[512];
	bool b_found_non_space;
	bool b_copy_char;

	if (wcp_string == NULL)
      return(NOT_OKAY);

	new_index = 0;
   old_length = (int)wcslen(wcp_string);
	b_found_non_space = false;

	if (old_length == 0)
		return(OKAY);

	for (old_index = 0; old_index < old_length; old_index++)
	{
		b_copy_char = false;

		if (b_found_non_space)
			b_copy_char = true;
		else
		{
			if (wcp_string[old_index] != ' ')
			{
				b_found_non_space = true;
				b_copy_char = true;
			}
		}

		if (b_copy_char)
		{
			new_string[new_index] = wcp_string[old_index];
		   new_index++;
		}
	}

   new_string[new_index] = L'\0';

	if (wcslen(new_string) > 0)
	   swprintf(wcp_string, L"%s", new_string);
	else
		wcp_string[0] = L'\0';

	return(OKAY);
}



/*
 * FUNCTION: pstrMakeNoDoublesBackSlashes()
 * DESCRIPTION:
 *
 *   Converts all all double backslashes in the passed string to
 *   only have one backslash.  
 *
 *   Example: Converts "C:\Tmp\\Test" to "C:\Tmp\Test"
 *
 *   NOTE: The string buffer passed can only be max of 512 characters.
 *
 */
int PT_DECLSPEC pstrMakeNoDoublesBackSlashes(char *cp_string)
{
	wchar_t *wcp_string;
	int i_string_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_string, &wcp_string, &i_string_widechar_size) != OKAY)
		return(NOT_OKAY);
    
	if(pstrMakeNoDoublesBackSlashes_Wide(wcp_string) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}
	
	if (pstrConvertWideCharStringToAnsiCharString(wcp_string, cp_string, strlen(cp_string) + 1) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}
	
	free(wcp_string);
	
	return(OKAY);
}

/*
 * FUNCTION: pstrMakeNoDoublesBackSlashes_Wide()
 * DESCRIPTION:
 *
 *   Converts all all double backslashes in the passed string to
 *   only have one backslash.  
 *
 *   Example: Converts "C:\Tmp\\Test" to "C:\Tmp\Test"
 *
 *   NOTE: The string buffer passed can only be max of 512 characters.
 *
 */
int PT_DECLSPEC pstrMakeNoDoublesBackSlashes_Wide(wchar_t *wcp_string)
{
	int old_length;
	int old_index;
	int new_index;
   wchar_t new_string[512];
	int previous_char_backslash;
   int append_char;

	if (wcp_string == NULL)
      return(NOT_OKAY);

	new_index = 0;
	previous_char_backslash = IS_FALSE;
   old_length = (int)wcslen(wcp_string);

	for (old_index = 0; old_index < old_length; old_index++)
	{
		append_char = IS_FALSE;

		/* Figure out if we should append this character */
		if (!previous_char_backslash)
			append_char = IS_TRUE;
      else
		{
		   if (wcp_string[old_index] != L'\\')
				append_char = IS_TRUE;
		}

		/* Append the character if necessary */
		if (append_char)
		{
		   new_string[new_index] = wcp_string[old_index];
		   new_index++;
		}

		/* Set the flag to remember if this character is a backslash */
		if (wcp_string[old_index] == L'\\')
		   previous_char_backslash = IS_TRUE;
		else
		   previous_char_backslash = IS_FALSE;
	}

   new_string[new_index] = L'\0';

	if (wcslen(new_string) > 0)
	   swprintf(wcp_string, L"%s", new_string);
	else
		wcp_string[0] = L'\0';

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveTrailingBackslash()
 * DESCRIPTION:
 *
 *  If the passed string ends with a backslash, this function removes it.
 *  Example: "C:\Tmp\" becomes "C:\Tmp"
 *
 */
int PT_DECLSPEC pstrRemoveTrailingBackslash(char *cp_string)
{
	wchar_t *wcp_string;
	int i_string_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_string, &wcp_string, &i_string_widechar_size) != OKAY)
		return(NOT_OKAY);
		
	if(pstrRemoveTrailingBackslash_Wide(wcp_string) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}
	
	if(pstrConvertWideCharStringToAnsiCharString(wcp_string, cp_string, strlen(cp_string) + 1) != OKAY)
	{
		free(wcp_string);
   		return(NOT_OKAY);
	}
	
	free(wcp_string);
	
   	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveTrailingBackslash_Wide()
 * DESCRIPTION:
 *
 *  If the passed string ends with a backslash, this function removes it.
 *  Example: "C:\Tmp\" becomes "C:\Tmp"
 *
 */
int PT_DECLSPEC pstrRemoveTrailingBackslash_Wide(wchar_t *wcp_string)
{
	int old_length;

	if (wcp_string == NULL)
      return(NOT_OKAY);

   old_length = (int)wcslen(wcp_string);

   if (old_length > 0)
   {
		if (wcp_string[old_length - 1] == L'\\')
			wcp_string[old_length - 1] = L'\0';
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveTrailingForwardSlash()
 * DESCRIPTION:
 *
 *  If the passed string ends with a forward slash, this function removes it.
 *  Example: "C:/Tmp/" becomes "C:/Tmp"
 *
 */
int PT_DECLSPEC pstrRemoveTrailingForwardSlash(char *cp_string)
{
	wchar_t *wcp_string;
	int i_string_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_string, &wcp_string, &i_string_widechar_size) != OKAY)
		return(NOT_OKAY);
		
	if(pstrRemoveTrailingForwardSlash_Wide(wcp_string) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}
	
	if(pstrConvertWideCharStringToAnsiCharString(wcp_string, cp_string, strlen(cp_string) + 1) != OKAY)
	{
		free(wcp_string);
   		return(NOT_OKAY);
	}
	
	free(wcp_string);
	
   	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveTrailingForwardSlash_Wide()
 * DESCRIPTION:
 *
 *  If the passed string ends with a forward slash, this function removes it.
 *  Example: "C:/Tmp/" becomes "C:/Tmp"
 *
 */
int PT_DECLSPEC pstrRemoveTrailingForwardSlash_Wide(wchar_t *wcp_string)
{
	int old_length;

	if (wcp_string == NULL)
      return(NOT_OKAY);

   old_length = (int)wcslen(wcp_string);

   if (old_length > 0)
   {
		if (wcp_string[old_length - 1] == L'/')
			wcp_string[old_length - 1] = L'\0';
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveTrailingNewLine()
 * DESCRIPTION:
 *
 *  Removes the \n character from the end of the passed string if it has one.
 *
 */
int PT_DECLSPEC pstrRemoveTrailingNewLine(char *cp_string)
{
	int length;
	int index;

	if (cp_string == NULL)
      return(NOT_OKAY);

   length = (int)strlen(cp_string);

	index = length - 1;

	if (index < 0)
		return(OKAY);

	if (cp_string[index] == '\n')
		cp_string[index] = '\0';

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveTrailingNewLine_Wide()
 * DESCRIPTION:
 *
 *  Removes the \n character from the end of the passed string if it has one.
 *
 */
int PT_DECLSPEC pstrRemoveTrailingNewLine_Wide(wchar_t *wcp_string)
{
	int length;
	int index;

	if (wcp_string == NULL)
      return(NOT_OKAY);

   length = (int)wcslen(wcp_string);

	index = length - 1;

	if (index < 0)
		return(OKAY);

	if (wcp_string[index] == '\n')
		wcp_string[index] = '\0';

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveTrailingCarriageReturns()
 * DESCRIPTION:
 *
 *  Removes all the \r or \n characters from the end of the passed string.
 *
 */
int PT_DECLSPEC pstrRemoveTrailingCarriageReturns(char *cp_string)
{
	int length;
	int done;
	int index;

	if (cp_string == NULL)
      return(NOT_OKAY);

   length = (int)strlen(cp_string);

   done = IS_FALSE;
	index = length - 1; 
	while (!done)
	{
		if (index < 0)
			done = IS_TRUE;
		else
		{
			if ((cp_string[index] == L'\r') || (cp_string[index] == L'\n'))
				cp_string[index] = L'\0';
			else
				done = IS_TRUE;

			index--;
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveTrailingCarriageReturns_Wide()
 * DESCRIPTION:
 *
 *  Removes all the \r or \n characters from the end of the passed string.
 *
 */
int PT_DECLSPEC pstrRemoveTrailingCarriageReturns_Wide(wchar_t *wcp_string)
{
	int length;
	int done;
	int index;

	if (wcp_string == NULL)
      return(NOT_OKAY);

   length = (int)wcslen(wcp_string);

   done = IS_FALSE;
	index = length - 1; 
	while (!done)
	{
		if (index < 0)
			done = IS_TRUE;
		else
		{
			if ((wcp_string[index] == L'\r') || (wcp_string[index] == L'\n'))
				wcp_string[index] = L'\0';
			else
				done = IS_TRUE;

			index--;
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveTrailingSpaces()
 * DESCRIPTION:
 *
 *  Removes all the spaces from the end of the passed string.
 *
 */
int PT_DECLSPEC pstrRemoveTrailingSpaces(char *cp_string)
{
	wchar_t *wcp_string;
	int i_string_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_string, &wcp_string, &i_string_widechar_size) != OKAY)
		return(NOT_OKAY);
		
	if(pstrRemoveTrailingSpaces_Wide(wcp_string) != OKAY)
	{
		free(wcp_string);
		return(NOT_OKAY);
	}
	
	if(pstrConvertWideCharStringToAnsiCharString(wcp_string, cp_string, strlen(cp_string) + 1) != OKAY)
	{
		free(wcp_string);
   		return(NOT_OKAY);
	}
	
	free(wcp_string);
	
   	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveTrailingSpaces_Wide()
 * DESCRIPTION:
 *
 *  Removes all the spaces from the end of the passed string.
 *
 */
int PT_DECLSPEC pstrRemoveTrailingSpaces_Wide(wchar_t *wcp_string)
{
	int length;
	int done;
	int index;

	if (wcp_string == NULL)
      return(NOT_OKAY);

   length = (int)wcslen(wcp_string);

   done = IS_FALSE;
	index = length - 1; 
	while (!done)
	{
		if (index < 0)
			done = IS_TRUE;
		else
		{
			if (wcp_string[index] == L' ')
				wcp_string[index] = L'\0';
			else
				done = IS_TRUE;

			index--;
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrCheckStartsWithMatch() 
 * DESCRIPTION:
 *
 * Check if the cp_main_string starts with the passed match_string.
 */
int PT_DECLSPEC pstrCheckStartsWithMatch(char *cp_main_string, char *cp_match_string, int *ip_match_flag)
{
	wchar_t *wcp_main_string;
	wchar_t *wcp_match_string;
	int i_main_string_widechar_size;
	int i_match_string_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_main_string, &wcp_main_string, &i_main_string_widechar_size) != OKAY)
		return(NOT_OKAY);
	if(pstrConvertToWideCharString_WithAlloc(cp_match_string, &wcp_match_string, &i_match_string_widechar_size) != OKAY)
	{
		free(wcp_main_string);
		return(NOT_OKAY);
	}

	if(pstrCheckStartsWithMatch_Wide(wcp_main_string, wcp_match_string, ip_match_flag) != OKAY)
	{
		free(wcp_main_string);
		free(wcp_match_string);
		return(NOT_OKAY);
	}
	
	free(wcp_main_string);
	free(wcp_match_string);
	
	return(OKAY);
}

/*
 * FUNCTION: pstrCheckStartsWithMatch_Wide() 
 * DESCRIPTION:
 *
 * Check if the cp_main_string starts with the passed match_string.
 */
int PT_DECLSPEC pstrCheckStartsWithMatch_Wide(wchar_t *wcp_main_string, wchar_t *wcp_match_string, int *ip_match_flag)
{
	int length_main;
	int length_match;
	int index;

	if (wcp_main_string == NULL)
		return(NOT_OKAY);

	if (wcp_match_string == NULL)
		return(NOT_OKAY);

	*ip_match_flag = IS_FALSE;

	length_main = (int)wcslen(wcp_main_string);
	length_match = (int)wcslen(wcp_match_string);

	/* Make sure the main string is at least long enough to contain match string */
	if (length_main < length_match)
		return(OKAY);

	for (index = 0; index < length_match; index++)
	{
		if (wcp_main_string[index] != wcp_match_string[index])
			return(OKAY);
	}

	*ip_match_flag = IS_TRUE;

	return(OKAY);
}

/*
 * FUNCTION: pstrCheckEndsWithMatch() 
 * DESCRIPTION:
 *
 * Check if the cp_main_string ends with the passed match_string.
 */
int PT_DECLSPEC pstrCheckEndsWithMatch(char *cp_main_string, char *cp_match_string, int *ip_match_flag)
{
	wchar_t *wcp_main_string;
	wchar_t *wcp_match_string;
	int i_main_string_widechar_size;
	int i_match_string_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_main_string, &wcp_main_string, &i_main_string_widechar_size) != OKAY)
		return(NOT_OKAY);
	if(pstrConvertToWideCharString_WithAlloc(cp_match_string, &wcp_match_string, &i_match_string_widechar_size) != OKAY)
	{
		free(wcp_main_string);
		return(NOT_OKAY);		
	}

	if(pstrCheckEndsWithMatch_Wide(wcp_main_string, wcp_match_string, ip_match_flag) != OKAY)
	{
		free(wcp_main_string);
		free(wcp_match_string);
		return(NOT_OKAY);
	}
	
	free(wcp_main_string);
	free(wcp_match_string);
	
	return(OKAY);
}

/*
 * FUNCTION: pstrCheckEndsWithMatch_Wide() 
 * DESCRIPTION:
 *
 * Check if the cp_main_string ends with the passed match_string.
 */
int PT_DECLSPEC pstrCheckEndsWithMatch_Wide(wchar_t *wcp_main_string, wchar_t *wcp_match_string, int *ip_match_flag)
{
	int length_main;
	int length_match;
	int main_index;
	int match_index;

	if (wcp_main_string == NULL)
		return(NOT_OKAY);

	if (wcp_match_string == NULL)
		return(NOT_OKAY);

	*ip_match_flag = IS_FALSE;

	length_main = (int)wcslen(wcp_main_string);
	length_match = (int)wcslen(wcp_match_string);

	/* Make sure the main string is at least long enough to contain match string */
	if (length_main < length_match)
		return(OKAY);

	main_index = length_main - 1;
	for (match_index = length_match - 1; match_index >= 0; match_index--)
	{
		if (wcp_main_string[main_index] != wcp_match_string[match_index])
			return(OKAY);
		main_index--;
	}

	*ip_match_flag = IS_TRUE;

	return(OKAY);
}


/*
 * FUNCTION: pstrReallocAndAppendString()
 * DESCRIPTION:
 *   Realloc the passed main string to accomidate what already exists in that string and the
 *   new string which is appended to it.
 */
int PT_DECLSPEC pstrReallocAndAppendString(char **cpp_main_string, char *cp_append_string)
{
	 int old_strlen;
	int append_strlen;
	int new_total_strlen;

	if (cpp_main_string == NULL)
		return(NOT_OKAY);

	if (cp_append_string == NULL)
		return(OKAY);

	append_strlen = (int)strlen(cp_append_string);

	if (*cpp_main_string == NULL)
		old_strlen = 0;
	else
		old_strlen = (int)strlen(*cpp_main_string);

	new_total_strlen = old_strlen + append_strlen;

	/* ReAllocate the main string to accomidate the new string being appended to it. */
	*cpp_main_string = (char *)realloc(*cpp_main_string, (new_total_strlen + 1) * sizeof(char));

	if (old_strlen == 0)
		*cpp_main_string[0] = '\0';

	strcat(*cpp_main_string, cp_append_string);

	return(OKAY);
}

/*
 * FUNCTION: pstrReallocAndAppendString_Wide()
 * DESCRIPTION:
 *   Realloc the passed main string to accomidate what already exists in that string and the
 *   new string which is appended to it.
 */
int PT_DECLSPEC pstrReallocAndAppendString_Wide(wchar_t **wcpp_main_string, wchar_t *wcp_append_string)
{
   int old_strlen;
	int append_strlen;
	int new_total_strlen;

	if (wcpp_main_string == NULL)
		return(NOT_OKAY);

	if (wcp_append_string == NULL)
		return(OKAY);

	append_strlen = (int)wcslen(wcp_append_string);

	if (*wcpp_main_string == NULL)
		old_strlen = 0;
	else
		old_strlen = (int)wcslen(*wcpp_main_string);

	new_total_strlen = old_strlen + append_strlen;

	/* ReAllocate the main string to accomidate the new string being appended to it. */
	*wcpp_main_string = (wchar_t *)realloc(*wcpp_main_string, (new_total_strlen + 1) * sizeof(wchar_t));

	if (old_strlen == 0)
		*wcpp_main_string[0] = L'\0';

	wcscat(*wcpp_main_string, wcp_append_string);

	return(OKAY);
}

/*
 * FUNCTION: pstrSplitIntoWords_Wide()
 * DESCRIPTION:
 *   Splits the passed string into an array of seperate words.  Passes back the number of words in the new array. 
 *   Each word is seperated by white_space[]. This is optional param. If not passed, it includes blank, newline and tab characters
 */
int PT_DECLSPEC pstrSplitIntoWords_Wide(wchar_t *wcp_string, wchar_t ***wcppp_array, int *ip_num_words, int i_max_strlen, wchar_t white_space[])
{
	wchar_t * token;
	wchar_t * wcp_next_token;

	*ip_num_words = 0;

	// Get first token
	token = wcstok_s(wcp_string, white_space, &wcp_next_token);
	
	while( token != NULL )
	{
		// Add token to the string array
		if(pstrAddStringToStringList_Wide(wcppp_array, *ip_num_words, token, IS_TRUE, IS_FALSE, IS_FALSE, i_max_strlen, ip_num_words) != OKAY)
			return(NOT_OKAY);
		// Get next token
		token = wcstok_s(NULL, white_space, &wcp_next_token);
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrGetLastError_Wide()
 * DESCRIPTION:
 *
 *  Fills in the passed string with the fomatted last error message.
 *  
 */
int PT_DECLSPEC pstrGetLastError_Wide(wchar_t *wcp_last_error, int i_buffer_length)
{
	DWORD dwLastError;

	wcscpy(wcp_last_error, L"");

	dwLastError = GetLastError(); 

	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, 0, wcp_last_error, i_buffer_length, NULL) == 0)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveIndexBasedSubset_WithAllocation()
 * DESCRIPTION:
 *
 *  Allocates and creates a new string which consists of the original string with the specified subset removed.
 *
 *  For example: wcp_original_string = "abcdefg", i_start_index = 3, i_end_index = 5, then
 *               wcp_result_string = "abcg"
 *  
 */
int PT_DECLSPEC pstrRemoveIndexBasedSubset_WithAllocation(wchar_t * wcp_original_string, int i_start_index, int i_end_index, 
														  wchar_t ** wcpp_result_string, int * ip_result_string_length)
{
	int i_first_half_string_size;
	int i_second_half_string_size;
	int i_original_string_strlen;
	wchar_t * wcp_first_half_string;
	wchar_t * wcp_second_half_string;

	if (wcp_original_string == NULL)
		return(NOT_OKAY);

	*wcpp_result_string = NULL;
	wcp_first_half_string = NULL;
	wcp_second_half_string = NULL;
	*ip_result_string_length = 0;

	/* Make sure the index values passed are legal */
	i_original_string_strlen = wcslen(wcp_original_string);
	if ((i_start_index < 0) || (i_start_index >= i_original_string_strlen))
		return(NOT_OKAY);
	if ((i_end_index < 0) || (i_end_index >= i_original_string_strlen))
		return(NOT_OKAY);
	if (i_start_index > i_end_index)
		return(NOT_OKAY);

	// Calculate size of first half of string (includes terminal null)
	i_first_half_string_size = i_start_index + 1; 
	// Allocate first half of the string
	wcp_first_half_string = (wchar_t *)calloc(i_first_half_string_size, sizeof(wchar_t) );
	if (wcp_first_half_string == NULL)
		return(NOT_OKAY);

	/* 
	 * Copy from the beginning of the original string up to
	 * For example: 
	 *  wcp_original_string = "abcdefg", i_start_index = 3, then wcp_first_half_string = "abc"
	 */
	wcsncpy(wcp_first_half_string, wcp_original_string, i_start_index);

	// wscncpy doesn't append null so we need to do it
	wcp_first_half_string[i_first_half_string_size - 1] = L'\0';
	
	// Calculate size of second half of string (includes terminal null)
	i_second_half_string_size = i_original_string_strlen - i_end_index; 

	// Allocate second half of the string
	wcp_second_half_string = (wchar_t *)calloc( i_second_half_string_size, sizeof(wchar_t) );
	if (wcp_second_half_string == NULL)
	{
		free(wcp_first_half_string);
		return(NOT_OKAY);
	}
	
	/* 
	 * Copy from the end of the skipped portion of original string to end of original string
	 * For example: 
	 *  wcp_original_string = "abcdefg", i_end_index = 5, then wcp_second_half_string = "g"
	 */
	wcsncpy(wcp_second_half_string, wcp_original_string + i_end_index + 1, 
		     i_original_string_strlen - i_end_index - 1);

	// wscncpy doesn't append null so we need to do it
	wcp_second_half_string[i_second_half_string_size - 1] = L'\0'; 

	// Combine two strings together into the final new XML string
	if (pstrReallocAndAppendString_Wide(wcpp_result_string, wcp_first_half_string) != OKAY)
	{
		free(wcp_first_half_string);
		free(wcp_second_half_string);
		*wcpp_result_string = NULL;
	
		return(NOT_OKAY);
	}
	if (pstrReallocAndAppendString_Wide(wcpp_result_string, wcp_second_half_string) != OKAY)
	{
		free(wcp_first_half_string);
		free(wcp_second_half_string);
		free(wcpp_result_string);
		*wcpp_result_string = NULL;

		return(NOT_OKAY);
	}

	free(wcp_first_half_string);
	free(wcp_second_half_string);

	*ip_result_string_length = wcslen(*wcpp_result_string);

	return(OKAY);
}

/*
 * FUNCTION: pstrInsertIndexBasedSubset_WithAllocation()
 * DESCRIPTION:
 *
 *  Allocates and creates a new string which consists of the original string with the specified string_to_insert
 *  inserted in the specified location.
 *
 *  For example: wcp_original_string = "abcdefg", i_start_index = 3, wcp_string_to_insert = "123", then
 *               wcp_result_string = "abc123defg"
 */
int PT_DECLSPEC pstrInsertIndexBasedSubset_WithAllocation(wchar_t * wcp_original_string, int i_start_index, 
														  wchar_t * wcp_string_to_insert, 
														  wchar_t ** wcpp_result_string, int * ip_result_string_length)
{
	if(wcp_original_string == NULL || i_start_index < 0)
		return(NOT_OKAY);

	int i_first_half_string_size;
	int i_second_half_string_size;
	int i_original_string_strlen;
	wchar_t * wcp_first_half_string;
	wchar_t * wcp_second_half_string;

	*wcpp_result_string = NULL;
	wcp_first_half_string = NULL;
	wcp_second_half_string = NULL;
	*ip_result_string_length = 0;

	/* Make sure the index values passed are legal */
	i_original_string_strlen = wcslen(wcp_original_string);
	if (i_start_index < 0)
		return(NOT_OKAY);

	/* Speical case: If the i_start_index is greater then the length of the string, just append it. */
	if (i_start_index >= i_original_string_strlen)
	{
		if (pstrReallocAndAppendString_Wide(wcpp_result_string, wcp_original_string) != OKAY)
		{
			return(NOT_OKAY);
		}

		if (pstrReallocAndAppendString_Wide(wcpp_result_string, wcp_string_to_insert) != OKAY)
		{
			free(wcpp_result_string);
			return(NOT_OKAY);
		}

		*ip_result_string_length = wcslen(*wcpp_result_string);

		return(OKAY);
	}

	// Calculate size of first half of string (includes terminal null)
	i_first_half_string_size = i_start_index + 1;

	// Allocate first half of the string
	wcp_first_half_string = (wchar_t *)calloc( i_first_half_string_size, sizeof(wchar_t) );
	if (wcp_first_half_string == NULL)
		return(NOT_OKAY);

	/* 
	 * Copy from the beginning of the original string up to
	 * For example: 
	 *  wcp_original_string = "abcdefg", i_start_index = 3, then wcp_first_half_string = "abc"
	 */
	wcsncpy(wcp_first_half_string, wcp_original_string, i_start_index);

	// wscncpy doesn't append null so we need to do it
	wcp_first_half_string[i_first_half_string_size - 1] = L'\0';

	// Calculate size of second half of string (includes terminal null)
	i_second_half_string_size = i_original_string_strlen - i_start_index + 1;

	// Allocate second half of the string
	wcp_second_half_string = (wchar_t *)calloc( i_second_half_string_size, sizeof(wchar_t) );
	if (wcp_second_half_string == NULL)
		return(NOT_OKAY);

	/* 
	 * Copy from the end of the skipped portion of original string to end of original string
	 * For example: 
	 *  wcp_original_string = "abcdefg",i_start_index = 3, then wcp_second_half_string = "defg"
	 */
	wcsncpy(wcp_second_half_string, wcp_original_string + i_start_index, i_original_string_strlen - i_start_index);

	// wscncpy doesn't append null so we need to do it
	wcp_second_half_string[i_second_half_string_size - 1] = L'\0';

	// Combine two strings together into the final new string
	if (pstrReallocAndAppendString_Wide(wcpp_result_string, wcp_first_half_string) != OKAY)
	{
		free(wcp_first_half_string);
		free(wcp_second_half_string);
		*wcpp_result_string = NULL;
	
		return(NOT_OKAY);
	}

	if (pstrReallocAndAppendString_Wide(wcpp_result_string, wcp_string_to_insert) != OKAY)
	{
		free(wcp_first_half_string);
		free(wcp_second_half_string);
		free(wcpp_result_string);
		*wcpp_result_string = NULL;

		return(NOT_OKAY);
	}

	if (pstrReallocAndAppendString_Wide(wcpp_result_string, wcp_second_half_string) != OKAY)
	{
		free(wcp_first_half_string);
		free(wcp_second_half_string);
		free(wcpp_result_string);
		*wcpp_result_string = NULL;

		return(NOT_OKAY);
	}

	free(wcp_first_half_string);
	free(wcp_second_half_string);

	*ip_result_string_length = wcslen(*wcpp_result_string);

	return(OKAY);
}

/*
 * FUNCTION: pstrReverse_Wide() 
 * DESCRIPTION:
 *  Reverse the characters of the passed string
 */
int PT_DECLSPEC pstrReverse_Wide(wchar_t *wcp_original_str, wchar_t *wcp_reversed_str)
{
	int index_original;
	int index_reversed;
	int length_original;

	if (wcp_original_str == NULL)
		return(NOT_OKAY);

	if (wcp_reversed_str == NULL)
		return(NOT_OKAY);

	length_original = wcslen(wcp_original_str);

	if (length_original == 0)
		return(OKAY);

	index_reversed = 0;

	for (index_original = length_original - 1; index_original >= 0; index_original--)
	{
		wcp_reversed_str[index_reversed] = wcp_original_str[index_original];
		index_reversed++;
	}

	wcp_reversed_str[length_original] = L'\0';

   return(OKAY);
}

/*
 * FUNCTION: pstrTruncate_Wide() 
 * DESCRIPTION:
 *
 * Truncates the passed string to the passed number of characters.  Optionally ends the string
 * with "..."
 */
int PT_DECLSPEC pstrTruncate_Wide(wchar_t *wcp_original_string, wchar_t *wcp_truncated_string, 
											 int i_truncated_length, BOOL b_end_with_dots)
{
	int index;
	int i_orig_length;

	if (wcp_original_string == NULL)
		return(NOT_OKAY);

	if (wcp_truncated_string == NULL)
		return(NOT_OKAY);

	/* Get the original string length */
	i_orig_length = wcslen(wcp_original_string);

	/* Check if no truncation is needed */
	if (i_truncated_length >= i_orig_length)
	{
		wsprintf(wcp_truncated_string, L"%s", wcp_original_string);
		return(OKAY);
	}

	/* Construct the truncated string */
	for (index = 0; index < i_truncated_length; index++)
	{
		wcp_truncated_string[index] = wcp_original_string[index];
	}
	wcp_truncated_string[i_truncated_length] = '\0';

	/* Optionally add the "..." */
	if ((b_end_with_dots) && (i_orig_length > 3))
	{
		wcp_truncated_string[i_truncated_length - 3] = '.';
		wcp_truncated_string[i_truncated_length - 2] = '.';
		wcp_truncated_string[i_truncated_length - 1] = '.';
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrRemoveAllLeadingTabs()
 * DESCRIPTION:
 *
 *   Remove all the leading tabs (ASCII 9) from the passed string, shifting string back over deletions.
 *
 */
int PT_DECLSPEC pstrRemoveAllLeadingTabs(char *cp_string)
{
	char *cp_str;
	bool stringWasModified;

	if( cp_string == NULL )
		return(NOT_OKAY);

	cp_str = cp_string;
	stringWasModified = false;

	// Index ptr past all leading tab chars (9) in beginning of string.
	do
	{
		if(cp_str[0] == 9)
		{
			cp_str++;
			stringWasModified = true;
		}
	}
	while(cp_str[0] == 9);

	// Copy modified string to shift it back to correct location over deleted chars.
	if( stringWasModified )
		strcpy(cp_string, cp_str);

	return(OKAY);
}


/*
 * FUNCTION: pstrReplaceIllegalFilenameChars_Wide()
 * DESCRIPTION:
 *
 * Replace all the illegal windows filename characters in cp_string with c_new_char.
 *
 */
int PT_DECLSPEC pstrReplaceIllegalFilenameChars_Wide(wchar_t *wcp_string, wchar_t wc_new_char)
{
	int length;
	int index;

	if (wcp_string == NULL)
      return(NOT_OKAY);

   length = (int)wcslen(wcp_string);

	for (index = 0; index < length; index++)
	{
		if ((wcp_string[index] == '"') ||
			 (wcp_string[index] == '<') ||
			 (wcp_string[index] == '>') ||
			 (wcp_string[index] == '|') ||
			 (wcp_string[index] == '\0') ||
			 (wcp_string[index] == '\x0001') ||
			 (wcp_string[index] == '\x0002') ||
			 (wcp_string[index] == '\x0003') ||
			 (wcp_string[index] == '\x0004') ||
			 (wcp_string[index] == '\x0005') ||
			 (wcp_string[index] == '\x0006') ||
			 (wcp_string[index] == '\a') ||
			 (wcp_string[index] == '\b') ||
			 (wcp_string[index] == '\t') ||
			 (wcp_string[index] == '\n') ||
			 (wcp_string[index] == '\v') ||
			 (wcp_string[index] == '\f') ||
			 (wcp_string[index] == '\r') ||
			 (wcp_string[index] == '\x000e') ||
			 (wcp_string[index] == '\x000f') ||
			 (wcp_string[index] == '\x0010') ||
			 (wcp_string[index] == '\x0011') ||
			 (wcp_string[index] == '\x0012') ||
			 (wcp_string[index] == '\x0013') ||
			 (wcp_string[index] == '\x0014') ||
			 (wcp_string[index] == '\x0015') ||
			 (wcp_string[index] == '\x0016') ||
			 (wcp_string[index] == '\x0017') ||
			 (wcp_string[index] == '\x0018') ||
			 (wcp_string[index] == '\x0019') ||
			 (wcp_string[index] == '\x001a') ||
			 (wcp_string[index] == '\x001b') ||
			 (wcp_string[index] == '\x001c') ||
			 (wcp_string[index] == '\x001d') ||
			 (wcp_string[index] == '\x001e') ||
			 (wcp_string[index] == '\x001f') ||
			 (wcp_string[index] == ':') ||
			 (wcp_string[index] == '*') ||
			 (wcp_string[index] ==  '?') ||
			 (wcp_string[index] == '\\') ||
			 (wcp_string[index] ==  '/'))
			wcp_string[index] = wc_new_char;
	}

	return(OKAY);
}

/*
 * FUNCTION: pstrGenerateGUID_Wide()
 * DESCRIPTION:
 *
 * Generate GUID string.
 * 
 * Example: {40F3B59C-2C64-45AE-A554-1AA4A95C25AB}
 *
 */
int PT_DECLSPEC pstrGenerateGUID_Wide(wchar_t *wcp_guid, int i_buffer_length)
{
	GUID guid;
	
	if (CoCreateGuid(&guid) != S_OK)
		return(NOT_OKAY);

	if (StringFromGUID2(guid, wcp_guid, i_buffer_length) == 0)
		return(NOT_OKAY);
	
	return(OKAY);
}