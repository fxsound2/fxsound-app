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

#define PSTR_URL_HTTP_PROTOCOL_LOCAL_PATH_PREFIX L"file://"

/*
 * FUNCTION: pstrUrlEncode()
 * DESCRIPTION:
 *
 * Encodes the passed url string so that special characters such as '&' are taken care of.
 */
int PT_DECLSPEC pstrUrlEncode(wchar_t *wcp_original, wchar_t *wcp_encoded, int i_max_strlen)
{
	int i_strlen_original;
	int i_orig_pos;
	int i_encoded_pos;
	int done;

	if (wcp_original == NULL)
		return(NOT_OKAY);

	i_strlen_original = wcslen(wcp_original);

	done = IS_FALSE;
	i_orig_pos = 0;
	i_encoded_pos = 0;

	while (!done)
	{
		if (wcp_original[i_orig_pos] == L';')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'3';
			wcp_encoded[i_encoded_pos+2] = L'B';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L'?')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'3';
			wcp_encoded[i_encoded_pos+2] = L'F';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L'/')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'2';
			wcp_encoded[i_encoded_pos+2] = L'F';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L':')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'3';
			wcp_encoded[i_encoded_pos+2] = L'A';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L'#')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'2';
			wcp_encoded[i_encoded_pos+2] = L'3';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L'&')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'2';
			wcp_encoded[i_encoded_pos+2] = L'6';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L'=')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'3';
			wcp_encoded[i_encoded_pos+2] = L'D';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L'+')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'2';
			wcp_encoded[i_encoded_pos+2] = L'B';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L'$')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'2';
			wcp_encoded[i_encoded_pos+2] = L'4';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L',')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'2';
			wcp_encoded[i_encoded_pos+2] = L'C';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L'%')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'2';
			wcp_encoded[i_encoded_pos+2] = L'5';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L'<')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'3';
			wcp_encoded[i_encoded_pos+2] = L'C';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L'>')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'3';
			wcp_encoded[i_encoded_pos+2] = L'E';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L'~')
		{
			wcp_encoded[i_encoded_pos] = L'%';
			wcp_encoded[i_encoded_pos+1] = L'7';
			wcp_encoded[i_encoded_pos+2] = L'E';
			i_encoded_pos = i_encoded_pos + 3;
		}
		else if (wcp_original[i_orig_pos] == L' ')
		{
			wcp_encoded[i_encoded_pos] = L'+';
			i_encoded_pos = i_encoded_pos + 1;
		}
		else if (wcp_original[i_orig_pos] == L'\0')
		{
			wcp_encoded[i_encoded_pos] = L'\0';
			done = IS_TRUE;
		}
		else
		{
			wcp_encoded[i_encoded_pos] = wcp_original[i_orig_pos];
			i_encoded_pos = i_encoded_pos + 1;
		}

		i_orig_pos++;

		if (i_encoded_pos >= i_max_strlen)
			done = IS_TRUE;
	}

	return(OKAY);
}

/* 
 * FUNCTION: pstrUrlHexChar2Int()
 * DESCRIPTION:
 *
 * Converts a hex character to its integer value
 * Modified from http://www.geekhideout.com/urlcode.shtml
 */
int PT_DECLSPEC pstrUrlHexChar2Int(char ch_in, char* ch_out) 
{
	*ch_out = isdigit(ch_in) ? ch_in - '0' : tolower(ch_in) - 'a' + 10;
	return(OKAY);
}

/* 
 * FUNCTION: pstrUrlChar2Hex()
 * DESCRIPTION:
 *
 * Converts an integer value to its hex character
 * Modified from http://www.geekhideout.com/urlcode.shtml
 */
int PT_DECLSPEC pstrUrlChar2Hex(char code_in, char* code_out) 
{
	static char hex[] = "0123456789abcdef";
	*code_out = hex[code_in & 15];
	return(OKAY);
}

/* 
 * FUNCTION: pstrUrlEncodeURIComponent()
 * DESCRIPTION:
 *
 * Returns a url-encoded version of str_in
 * IMPORTANT: be sure to free() the returned string after use 
 * NOTE: To get around isalnum() assertion error with a UTF8 character, I use solution from:
 * http://social.msdn.microsoft.com/Forums/en/vcgeneral/thread/a9f2d599-54ce-41d4-99c5-dd70dafba43d
 * Modified from http://www.geekhideout.com/urlcode.shtml
 */
int PT_DECLSPEC pstrUrlEncodeURIComponent(char * str_in, char ** str_out)
{
	char *pstr = str_in;
	*str_out = (char*)malloc(strlen(str_in) * 3 + 1);
	char *pbuf = *str_out;
	char hex_code1;
	char hex_code2;
	while (*pstr) 
	{
		if (isalnum((unsigned char)*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
			*pbuf++ = *pstr;
		else if (*pstr == ' ') 
			*pbuf++ = '+';
		else
		{
			pstrUrlChar2Hex(*pstr >> 4, &hex_code1);
			pstrUrlChar2Hex(*pstr & 15, &hex_code2);
			*pbuf++ = '%';
			*pbuf++ = hex_code1;
			*pbuf++ = hex_code2;
		}
		pstr++;
	}
	*pbuf = '\0';
	return(OKAY);
}

/* 
 * FUNCTION: pstrUrlDecodeURIComponent()
 * DESCRIPTION:
 *
 * Returns a url-decoded version of str 
 * IMPORTANT: be sure to free() the returned string after use
 * Modified from http://www.geekhideout.com/urlcode.shtml
 */
int PT_DECLSPEC pstrUrlDecodeURIComponent(char * str_in, char ** str_out)
{
	char *pstr = str_in;
	*str_out = (char*)malloc(strlen(str_in) + 1);
	char *pbuf = *str_out;
	char from_hex1;
	char from_hex2;
	while (*pstr) 
	{
		if (*pstr == '%') 
		{
			if (pstr[1] && pstr[2]) 
			{
				pstrUrlHexChar2Int(pstr[1], &from_hex1);
				pstrUrlHexChar2Int(pstr[2], &from_hex2);
				*pbuf++ = from_hex1 << 4 | from_hex2;
				pstr += 2;
			}
		}else if (*pstr == '+')
		{ 
			*pbuf++ = ' ';
		}else 
		{
			*pbuf++ = *pstr;
		}
		pstr++;
	}
	*pbuf = '\0';
	return(OKAY);
}

/*
 * FUNCTION: pstrUrlConvertLocalPathToHttpProtocol()
 * DESCRIPTION:
 *
 * Converts the passed local html fullpath to the proper http protocol so it can be loaded in a browser.
 *
 * Example:
 * wcp_original = "C:\Program Files\Test\index.html"
 * wcp_converted = "file://C:/Program Files/Test/index.html"
 *
 */
int PT_DECLSPEC pstrUrlConvertLocalPathToHttpProtocol(wchar_t *wcp_original, wchar_t *wcp_converted)
{
	int i_strlen_original;
	int i_orig_pos;
	int i_converted_pos;
	int done;

	if (wcp_original == NULL)
		return(NOT_OKAY);

	i_strlen_original = wcslen(wcp_original);

	done = IS_FALSE;
	i_orig_pos = 0;
	i_converted_pos = 0;

	/* Put the http protocol prefix ( file:// ) at the beginning of the converted string */
	swprintf(wcp_converted, PSTR_URL_HTTP_PROTOCOL_LOCAL_PATH_PREFIX);
	i_converted_pos = wcslen(PSTR_URL_HTTP_PROTOCOL_LOCAL_PATH_PREFIX);

	for (i_orig_pos = 0; i_orig_pos <= i_strlen_original; i_orig_pos++)
	{
		if (wcp_original[i_orig_pos] == L'\\')
			wcp_converted[i_converted_pos] = L'/';
		else 
			wcp_converted[i_converted_pos] = wcp_original[i_orig_pos];
		
		i_converted_pos++;
	}

	return(OKAY);
}