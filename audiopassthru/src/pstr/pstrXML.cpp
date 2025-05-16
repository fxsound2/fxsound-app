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
//#include "xml.h"
#include "pt_defs.h"

/*
 * FUNCTION: pstrXmlGetNextKeyTag()
 * DESCRIPTION:
 *
 * Given a XML string, it finds the next <key>value</key> after i_start_search_index.
 * wcp_found_key_value holds the value if found and must be pre-allocated.
 * See pstrXmlGetNextTag() for more information.
 */
int PT_DECLSPEC pstrXmlGetNextKeyTag(wchar_t * wcp_xml_string, int i_start_search_index, int* ip_key_found, 
								     wchar_t * wcp_found_key_value, int i_found_key_value_buffer_size,
								     int * ip_start_key_index, int * ip_end_key_index)
{

	return(pstrXmlGetNextTag(wcp_xml_string, PSTR_XML_KEY_OPENING_TAG, PSTR_XML_KEY_CLOSING_TAG, i_start_search_index, ip_key_found, 
		                     wcp_found_key_value, i_found_key_value_buffer_size,
							 ip_start_key_index, ip_end_key_index));
}

/*
 * FUNCTION: pstrXmlGetNextDictTag()
 * DESCRIPTION:
 *
 * Given a XML string, it finds the next <dict>value</dict> after i_start_search_index.
 * wcp_found_dict_value holds the value if found and must be pre-allocated.
 * See pstrXmlGetNextTag() for more information.
 */
int PT_DECLSPEC pstrXmlGetNextDictTag(wchar_t * wcp_xml_string, int i_start_search_index, int* ip_dict_found, 
								      wchar_t * wcp_found_dict_value, int i_found_dict_value_buffer_size,
								      int * ip_start_dict_index, int * ip_end_dict_index)
{

	return(pstrXmlGetNextTag(wcp_xml_string, PSTR_XML_DICT_OPENING_TAG, PSTR_XML_DICT_CLOSING_TAG, i_start_search_index, ip_dict_found, 
		                     wcp_found_dict_value, i_found_dict_value_buffer_size,
							 ip_start_dict_index, ip_end_dict_index));
}

/*
 * FUNCTION: pstrXmlGetNextDataTag()
 * DESCRIPTION:
 *
 * Given a XML string, it finds the next <data>value</data> after i_start_search_index.
 * wcp_found_data_value holds the value if found and must be pre-allocated.
 * See pstrXmlGetNextTag() for more information.
 */
int PT_DECLSPEC pstrXmlGetNextDataTag(wchar_t * wcp_xml_string, int i_start_search_index, int* ip_data_found, 
								      wchar_t * wcp_found_data_value, int i_found_data_value_buffer_size,
								      int * ip_start_data_index, int * ip_end_data_index)
{

	return(pstrXmlGetNextTag(wcp_xml_string, PSTR_XML_DATA_OPENING_TAG, PSTR_XML_DATA_CLOSING_TAG, i_start_search_index, ip_data_found, 
		                     wcp_found_data_value, i_found_data_value_buffer_size,
							 ip_start_data_index, ip_end_data_index));
}

/*
 * FUNCTION: pstrXmlGetNextStringTag()
 * DESCRIPTION:
 *
 * Given a XML string, it finds the next <string>value</string> after i_start_search_index.
 * wcp_found_data_value holds the value if found and must be pre-allocated.
 * See pstrXmlGetNextTag() for more information.
 */
int PT_DECLSPEC pstrXmlGetNextStringTag(wchar_t * wcp_xml_string, int i_start_search_index, int* ip_data_found, 
								        wchar_t * wcp_found_data_value, int i_found_data_value_buffer_size,
								        int * ip_start_data_index, int * ip_end_data_index)
{

	return(pstrXmlGetNextTag(wcp_xml_string, PSTR_XML_STRING_OPENING_TAG, PSTR_XML_STRING_CLOSING_TAG, i_start_search_index, ip_data_found, 
		                     wcp_found_data_value, i_found_data_value_buffer_size,
							 ip_start_data_index, ip_end_data_index));
}

/*
 * FUNCTION: pstrXmlGetNextTag()
 * DESCRIPTION:
 *
 * Given a XML string, it finds the next <start tag>value</end tag> after i_start_search_index.
 *                                       ^                        ^
 *                                       |                        |
 *                                       |--> ip_start_tag_index  |--> ip_end_tag_index
 * wcp_found_tag_value holds the tag value if found and must be pre-allocated.
 */
int PT_DECLSPEC pstrXmlGetNextTag(wchar_t * wcp_xml_string, wchar_t * wcp_start_tag, wchar_t * wcp_end_tag, 
								  int i_start_search_index, int * ip_tag_found, wchar_t * wcp_found_tag_value, 
								  int i_found_tag_value_buffer_size, int * ip_start_tag_index, int * ip_end_tag_index)
{
	if (wcp_xml_string == NULL || wcp_start_tag == NULL || wcp_end_tag == NULL)
		return(NOT_OKAY);

	if (i_start_search_index < 0)
		return(NOT_OKAY);

	int i_value_start_index;
	int index_source;
	int index_dest;
	
	*ip_tag_found = IS_FALSE;
	*ip_start_tag_index = -1;
	*ip_end_tag_index = -1;

	/*
	 * Find the index of opening tag 
	 * For example: <start tag>value</end tag>, *ip_start_tag_index will be this index
	 *              ^
	 *              |
	 */
	if (pstrCalcLocationOfStrInStr_Wide(wcp_xml_string, wcp_start_tag, i_start_search_index, ip_start_tag_index, ip_tag_found) != OKAY)
		return(NOT_OKAY);

	if(*ip_tag_found == IS_FALSE)
	{
		return(OKAY);
	}
	/*
	 * Find the index of closing tag
	 *
	 * For example: <start tag>value</end tag>, *ip_end_tag_index will be this index
	 *                              ^
	 *                              |
	 */
	if (pstrCalcLocationOfStrInStr_Wide(wcp_xml_string, wcp_end_tag, *ip_start_tag_index, ip_end_tag_index, ip_tag_found) != OKAY)
		return(NOT_OKAY);

	if(*ip_tag_found == IS_FALSE)
	{
		return(OKAY);
	}

	/*
	 * Set the start index of the value to be after start tag to get the value
	 *
	 * For example: <start tag>value</end tag>, i_value_start_index will be this index
	 *                         ^
	 *                         |
	 */
	i_value_start_index = *ip_start_tag_index + wcslen(wcp_start_tag);

	// Copy the value string to wcp_found_tag_value
	index_dest = 0;
	for(index_source = i_value_start_index; index_source < *ip_end_tag_index; index_source++)
	{
		// Respect i_found_tag_value_buffer_size
		if(index_dest >= i_found_tag_value_buffer_size - 1 )
			break;
		wcp_found_tag_value[index_dest] = wcp_xml_string[index_source];
		index_dest++;
		
	}
	// Append null to complete the string
	wcp_found_tag_value[index_dest] = L'\0';

	// Advance ip_end_tag_index to be pointing at the last '>'
	*ip_end_tag_index += wcslen(wcp_end_tag) - 1;
		
	return(OKAY);
}