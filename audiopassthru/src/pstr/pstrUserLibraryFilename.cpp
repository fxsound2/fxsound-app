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
#include "pt_defs.h"
#include "pstr.h"

/*
 * FUNCTION: pstrCalcFilenameFromFullpath()
 * DESCRIPTION:
 *
 *  Based on the passed fullpath this function calculates and passes back just the
 *  filename with out the leading folders.
 *
 *  Note: This function works for folders seperated with forward and backslashes.
 *
 *  Ex1. cp_fullpath: "C:\Folder1\SubFolder\file.txt"
 *      cp_filename: "file.txt"
 *
 *  Ex2. cp_fullpath: "C:/Folder1/SubFolder/file.txt"
 *      cp_filename: "file.txt"
 *
 */
int PT_DECLSPEC pstrCalcFilenameFromFullpath(char *cp_fullpath, char *cp_filename, int *ip_filename_found)
{
	wchar_t *wcp_fullpath;
	wchar_t *wcp_filename;
	int i_fullpath_widechar_size;

	if(pstrConvertToWideCharString_WithAlloc(cp_fullpath, &wcp_fullpath, &i_fullpath_widechar_size) != OKAY)
		return(NOT_OKAY);

	wcp_filename = (wchar_t *)calloc(strlen(cp_fullpath) + 1, sizeof(wchar_t));

	if(wcp_filename == NULL)
	{
		free(wcp_fullpath);
		return(NOT_OKAY);
	}

	if(pstrCalcFilenameFromFullpath_Wide(wcp_fullpath, wcp_filename, ip_filename_found) != OKAY)
	{
		free(wcp_fullpath);
		free(wcp_filename);
		return(NOT_OKAY);
	}

	if(pstrConvertWideCharStringToAnsiCharString(wcp_filename, cp_filename, strlen(cp_fullpath) + 1) != OKAY)
	{
		free(wcp_fullpath);
		free(wcp_filename);
		return(NOT_OKAY);
	}

	free(wcp_fullpath);
	free(wcp_filename);

	return(OKAY);
}

/*
 * FUNCTION: pstrCalcFilenameFromFullpath_Wide()
 * DESCRIPTION:
 *
 *  Based on the passed fullpath this function calculates and passes back just the
 *  filename with out the leading folders.
 *
 *  Note: This function works for folders seperated with forward and backslashes.
 *
 *  Ex1. cp_fullpath: "C:\Folder1\SubFolder\file.txt"
 *      cp_filename: "file.txt"
 *
 *  Ex2. cp_fullpath: "C:/Folder1/SubFolder/file.txt"
 *      cp_filename: "file.txt"
 *
 */
int PT_DECLSPEC pstrCalcFilenameFromFullpath_Wide(wchar_t *wcp_fullpath, wchar_t *wcp_filename, int *ip_filename_found)
{
	int length;
	int orig_index;
	int ext_index;
	int location_of_last_slash;
	
	if (wcp_fullpath == NULL)
		return(NOT_OKAY);
	if (wcp_filename == NULL)
		return(NOT_OKAY);

	*ip_filename_found = IS_FALSE;

	length = (int)wcslen(wcp_fullpath);

	if (length <= 0)
      return(OKAY);

   orig_index = length - 1;

	while ((orig_index >= 0) && (!(*ip_filename_found)))
	{
		if ((wcp_fullpath[orig_index] == L'\\') || (wcp_fullpath[orig_index] == L'/'))
		{
			location_of_last_slash = orig_index;
			*ip_filename_found = IS_TRUE;
		}
		else
			orig_index--;
	}

	/* If there is no slash, simply copy the fullpath to the filename */
	if (!(*ip_filename_found))
	{
		swprintf(wcp_filename, L"%s", wcp_fullpath);
	}
   else
	{
	   /* Copy the filename */
	   ext_index = 0;
	   for (orig_index = location_of_last_slash + 1; orig_index <= length; orig_index++)
		{
		   wcp_filename[ext_index] = wcp_fullpath[orig_index];
		   ext_index++;
		}
	}

   *ip_filename_found = IS_TRUE;

   return(OKAY);
}

/*
 * FUNCTION: pstrCalcLibraryFromFullpath()
 * DESCRIPTION:
 *
 *  Based on the passed fullpath this function calculates and passes back just the
 *  library with out the leading folders before the library and the filename after the library.
 *
 *  Note: This function works for folders seperated with forward and backslashes.
 *
 *
 *  Ex1. cp_fullpath: C:\Program Files\MP3 Remix\Users\Factory 3\Sounds\Free\sound1.rxs
 *      cp_ulf_type: "Free"
 *
 *  Ex2. cp_fullpath: C:/Program Files/MP3 Remix/Users/Factory 3/Sounds/Free/sound1.rxs
 *      cp_ulf_type: "Free"
 *
 */
int PT_DECLSPEC pstrCalcLibraryFromFullpath(char *cp_fullpath, char *cp_library, int *ip_library_found)
{
	wchar_t *wcp_fullpath;
	wchar_t *wcp_library;
	int i_fullpath_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_fullpath, &wcp_fullpath, &i_fullpath_widechar_size) != OKAY)
		return(NOT_OKAY);

	wcp_library = (wchar_t *)calloc(strlen(cp_fullpath) + 1, sizeof(wchar_t));

	if(wcp_library == NULL)
	{
		free(wcp_fullpath);
		return(NOT_OKAY);
	}

	if(pstrCalcLibraryFromFullpath_Wide(wcp_fullpath, wcp_library, ip_library_found) != OKAY)
	{
		free(wcp_fullpath);
		free(wcp_library);
		return(NOT_OKAY);
	}

	if(pstrConvertWideCharStringToAnsiCharString(wcp_library, cp_library, strlen(cp_fullpath) + 1) != OKAY)
	{
		free(wcp_fullpath);
		free(wcp_library);
		return(NOT_OKAY);
	}

	free(wcp_fullpath);
	free(wcp_library);

	return(OKAY);
}

/*
 * FUNCTION: pstrCalcLibraryFromFullpath_Wide()
 * DESCRIPTION:
 *
 *  Based on the passed fullpath this function calculates and passes back just the
 *  library with out the leading folders before the library and the filename after the library.
 *
 *  Note: This function works for folders seperated with forward and backslashes.
 *
 *
 *  Ex1. cp_fullpath: C:\Program Files\MP3 Remix\Users\Factory 3\Sounds\Free\sound1.rxs
 *      cp_ulf_type: "Free"
 *
 *  Ex2. cp_fullpath: C:/Program Files/MP3 Remix/Users/Factory 3/Sounds/Free/sound1.rxs
 *      cp_ulf_type: "Free"
 *
 */
int PT_DECLSPEC pstrCalcLibraryFromFullpath_Wide(wchar_t *wcp_fullpath, wchar_t *wcp_library, int *ip_library_found)
{
	if (wcp_fullpath == NULL)
		return(NOT_OKAY);
	if (wcp_library == NULL)
		return(NOT_OKAY);

	int length;
	int fullpath_index;
	int library_index;
	int location_of_last_slash;
	int location_of_second_to_last_slash;

	*ip_library_found = IS_FALSE;

	length = (int)wcslen(wcp_fullpath);

	if (length <= 0)
      return(OKAY);

	/* 
	 * Figure out the location of the last slash and second to last slash because the library
	 * name is between thoses slashes.
	 */
	location_of_last_slash = -1;
	location_of_second_to_last_slash = -1;
   for (fullpath_index = length - 1; fullpath_index >= 0; fullpath_index--)
	{
      if ((wcp_fullpath[fullpath_index] == L'\\') || (wcp_fullpath[fullpath_index] == L'/'))
		{
		   if (location_of_last_slash == -1)
			{
				location_of_last_slash = fullpath_index;
			}
			else
			{
			   location_of_second_to_last_slash = fullpath_index;
				fullpath_index = 0;
			}
		}
	}

	/* If the last slash was not found, then it is not of the correct form */
	if (location_of_last_slash == -1)
		return(OKAY);

	/* 
	 * If the second to last slash was never found then it must be of the form libray/filename
	 * in which case we want to keep location_of_second_to_last_slash at -1.
	 */

	/* Copy the characters between the two slashes into the library name */
	library_index = 0;
	for (fullpath_index = location_of_second_to_last_slash + 1; 
	     fullpath_index < location_of_last_slash; fullpath_index++)
	{
      wcp_library[library_index] = wcp_fullpath[fullpath_index];
		library_index++;
	}
	wcp_library[library_index] = L'\0';

	*ip_library_found = IS_TRUE;

   return(OKAY);
}


/*
 * FUNCTION: pstrCalcUserComponentDirnameFromFullpath()
 * DESCRIPTION:
 *
 *  Based on the passed fullpath this function calculates and passes back just the
 *  ulfType folder assuming the fullpath is of a user\ulftype\library\filename specification type.
 *
 *  Note: This function works for folders seperated with forward and backslashes.
 *
 *  Ex1. cp_fullpath: C:\Program Files\MP3 Remix\Users\Factory 3\Sounds\Free\sound1.rxs
 *      cp_ulf_type: "Sounds"
 *
 *  Ex2. cp_fullpath: C:/Program Files/MP3 Remix/Users/Factory 3/Sounds/Free/sound1.rxs
 *      cp_ulf_type: "Sounds"
 *
 */
int PT_DECLSPEC pstrCalcUserComponentDirnameFromFullpath(char *cp_fullpath, char *cp_ulf_type, int *ip_ulf_type_found)
{
	wchar_t *wcp_fullpath;
	wchar_t *wcp_ulf_type;
	int i_fullpath_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_fullpath, &wcp_fullpath, &i_fullpath_widechar_size) != OKAY)
		return(NOT_OKAY);

	wcp_ulf_type = (wchar_t *)calloc(strlen(cp_fullpath) + 1, sizeof(wchar_t));

	if(wcp_ulf_type == NULL)
	{
		free(wcp_fullpath);
		return(NOT_OKAY);
	}

	if(pstrCalcUserComponentDirnameFromFullpath_Wide(wcp_fullpath, wcp_ulf_type, ip_ulf_type_found) != OKAY)
	{
		free(wcp_fullpath);
		free(wcp_ulf_type);
		return(NOT_OKAY);
	}
	
	if(pstrConvertWideCharStringToAnsiCharString(wcp_ulf_type, cp_ulf_type, strlen(cp_fullpath) + 1) != OKAY)
	{
		free(wcp_fullpath);
		free(wcp_ulf_type);
		return(NOT_OKAY);
	}

	free(wcp_fullpath);
	free(wcp_ulf_type);

	return(OKAY);
}

/*
 * FUNCTION: pstrCalcUserComponentDirnameFromFullpath_Wide()
 * DESCRIPTION:
 *
 *  Based on the passed fullpath this function calculates and passes back just the
 *  ulfType folder assuming the fullpath is of a user\ulftype\library\filename specification type.
 *
 *  Note: This function works for folders seperated with forward and backslashes.
 *
 *  Ex1. cp_fullpath: C:\Program Files\MP3 Remix\Users\Factory 3\Sounds\Free\sound1.rxs
 *      cp_ulf_type: "Sounds"
 *
 *  Ex2. cp_fullpath: C:/Program Files/MP3 Remix/Users/Factory 3/Sounds/Free/sound1.rxs
 *      cp_ulf_type: "Sounds"
 *
 */
int PT_DECLSPEC pstrCalcUserComponentDirnameFromFullpath_Wide(wchar_t *wcp_fullpath, wchar_t *wcp_ulf_type, int *ip_ulf_type_found)
{
	if (wcp_fullpath == NULL)
		return(NOT_OKAY);
	if (wcp_ulf_type == NULL)
		return(NOT_OKAY);

	int length;
	int fullpath_index;
	int user_index;
	int location_of_last_slash;
	int location_of_second_to_last_slash;
	int location_of_third_to_last_slash;

	*ip_ulf_type_found = IS_FALSE;

	length = (int)wcslen(wcp_fullpath);

	if (length <= 0)
      return(OKAY);

	/* 
	 * Figure out the location of the last slash, second to last slash and third to last slash because 
	 * the user name is between the second to last and third to last slashes.
	 */
	location_of_last_slash = -1;
	location_of_second_to_last_slash = -1;
	location_of_third_to_last_slash = -1;
   for (fullpath_index = length - 1; fullpath_index >= 0; fullpath_index--)
	{
      if ((wcp_fullpath[fullpath_index] == L'\\') || (wcp_fullpath[fullpath_index] == L'/'))
		{
		   if (location_of_last_slash == -1)
			{
				location_of_last_slash = fullpath_index;
			}
			else if (location_of_second_to_last_slash == -1)
			{
			   location_of_second_to_last_slash = fullpath_index;
			}
			else
			{
			   location_of_third_to_last_slash = fullpath_index;
				fullpath_index = 0;
			}
		}
	}

	/* If the last slash and the second to last slash was not found, then it is not of the correct form */
	if ((location_of_last_slash == -1) || (location_of_second_to_last_slash == -1))
		return(OKAY);

	/* 
	 * If the third to last slash was never found then it must be of the form ulf_type/library/filename
	 * in which case we want to keep location_of_third_to_last_slash at -1.
	 */

	/* Copy the characters between the two slashes into the user name */
	user_index = 0;
	for (fullpath_index = location_of_third_to_last_slash + 1; 
	     fullpath_index < location_of_second_to_last_slash; fullpath_index++)
	{
      wcp_ulf_type[user_index] = wcp_fullpath[fullpath_index];
		user_index++;
	}
	wcp_ulf_type[user_index] = L'\0';

	*ip_ulf_type_found = IS_TRUE;

   return(OKAY);
}

/*
 * FUNCTION: pstrCalcUserFromFullpath()
 * DESCRIPTION:
 *
 *  Based on the passed fullpath this function calculates and passes back just the
 *  user folder assuming the fullpath is of a user\library\filename specification type.
 *
 *  Note: This function works for folders seperated with forward and backslashes.
 *
 *  Ex1. cp_fullpath: C:\Program Files\MP3 Remix\Users\Factory 3\Sounds\Free\sound1.rxs
 *      cp_ulf_type: "Factory 3"
 *
 *  Ex2. cp_fullpath: C:/Program Files/MP3 Remix/Users/Factory 3/Sounds/Free/sound1.rxs
 *      cp_ulf_type: "Factory 3"
 *
 */
int PT_DECLSPEC pstrCalcUserFromFullpath(char *cp_fullpath, char *cp_user, int *ip_user_found)
{
	wchar_t *wcp_fullpath;
	wchar_t *wcp_user;
	int i_fullpath_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_fullpath, &wcp_fullpath, &i_fullpath_widechar_size) != OKAY)
		return(NOT_OKAY);

	wcp_user = (wchar_t *)calloc(strlen(cp_fullpath) + 1, sizeof(wchar_t));

	if(wcp_user == NULL)
	{
		free(wcp_fullpath);
		return(NOT_OKAY);
	}

	if(pstrCalcUserFromFullpath_Wide(wcp_fullpath, wcp_user, ip_user_found) != OKAY)
	{
		free(wcp_fullpath);
		free(wcp_user);
		return(NOT_OKAY);
	}
	
	if(pstrConvertWideCharStringToAnsiCharString(wcp_user, cp_user, strlen(cp_fullpath) + 1) != OKAY)
	{
		free(wcp_fullpath);
		free(wcp_user);
		return(NOT_OKAY);
	}
	
	free(wcp_fullpath);
	free(wcp_user);

	return(OKAY);
}

/*
 * FUNCTION: pstrCalcUserFromFullpath_Wide()
 * DESCRIPTION:
 *
 *  Based on the passed fullpath this function calculates and passes back just the
 *  user folder assuming the fullpath is of a user\library\filename specification type.
 *
 *  Note: This function works for folders seperated with forward and backslashes.
 *
 *  Ex1. cp_fullpath: C:\Program Files\MP3 Remix\Users\Factory 3\Sounds\Free\sound1.rxs
 *      cp_ulf_type: "Factory 3"
 *
 *  Ex2. cp_fullpath: C:/Program Files/MP3 Remix/Users/Factory 3/Sounds/Free/sound1.rxs
 *      cp_ulf_type: "Factory 3"
 *
 */
int PT_DECLSPEC pstrCalcUserFromFullpath_Wide(wchar_t *wcp_fullpath, wchar_t *wcp_user, int *ip_user_found)
{
	if (wcp_fullpath == NULL)
		return(NOT_OKAY);
	if (wcp_user == NULL)
		return(NOT_OKAY);

	int length;
	int fullpath_index;
	int user_index;
	int location_of_last_slash;
	int location_of_second_to_last_slash;
	int location_of_third_to_last_slash;
	int location_of_fourth_to_last_slash;

	*ip_user_found = IS_FALSE;

	length = (int)wcslen(wcp_fullpath);

	if (length <= 0)
      return(OKAY);

	/* 
	 * Figure out the location of the last slash, second to last slash and third to last slash because 
	 * the user name is between the second to last and third to last slashes.
	 */
	location_of_last_slash = -1;
	location_of_second_to_last_slash = -1;
	location_of_third_to_last_slash = -1;
	location_of_fourth_to_last_slash = -1;
   for (fullpath_index = length - 1; fullpath_index >= 0; fullpath_index--)
	{
      if ((wcp_fullpath[fullpath_index] == L'\\') || (wcp_fullpath[fullpath_index] == L'/'))
		{
		   if (location_of_last_slash == -1)
			{
				location_of_last_slash = fullpath_index;
			}
			else if (location_of_second_to_last_slash == -1)
			{
			   location_of_second_to_last_slash = fullpath_index;
			}
			else if (location_of_third_to_last_slash == -1)
			{
			   location_of_third_to_last_slash = fullpath_index;
			}
			else
			{
			   location_of_fourth_to_last_slash = fullpath_index;
				fullpath_index = 0;
			}
		}
	}

	/* If all slashes were not found, then it is not of the correct form */
	if ((location_of_last_slash == -1) || (location_of_second_to_last_slash == -1) || (location_of_third_to_last_slash == -1))
		return(OKAY);

	/* 
	 * If the fourth to last slash was never found then it must be of the form user/ulf_type/library/filename
	 * in which case we want to keep location_of_fourth_to_last_slash at -1.
	 */

	/* Copy the characters between the two slashes into the user name */
	user_index = 0;
	for (fullpath_index = location_of_fourth_to_last_slash + 1; 
	     fullpath_index < location_of_third_to_last_slash; fullpath_index++)
	{
      wcp_user[user_index] = wcp_fullpath[fullpath_index];
		user_index++;
	}
	wcp_user[user_index] = L'\0';

	*ip_user_found = IS_TRUE;

   return(OKAY);
}


/*
 * FUNCTION: pstrSplitLibrarySlashFilenameSpecification()
 * DESCRIPTION:
 *
 *  Based on the passed full library/filename specification this function splits it up and
 *  passes back the libary and then filename.
 * 
 *  Ex1:
 *  cp_fullstring : factory\test.txt
 *    cp_library = factory
 *    cp_filename = test.txt
 *
 *  Ex2:
 *  cp_fullstring : factory/test.txt
 *    cp_library = factory
 *    cp_filename = test.txt
 */
int PT_DECLSPEC pstrSplitLibrarySlashFilenameSpecification(char *cp_fullstring, 
															char *cp_library, 
															char *cp_filename,
															int *ip_library_and_filename_found)
{
	wchar_t *wcp_fullstring;
	wchar_t *wcp_library;
	wchar_t *wcp_filename;
	int i_fullstring_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_fullstring, &wcp_fullstring, &i_fullstring_widechar_size) != OKAY)
		return(NOT_OKAY);

	wcp_library = (wchar_t *)calloc(strlen(cp_fullstring) + 1, sizeof(wchar_t));

	if(wcp_library == NULL)
	{
		free(wcp_fullstring);
		return(NOT_OKAY);
	}

	wcp_filename = (wchar_t *)calloc(strlen(cp_fullstring) + 1, sizeof(wchar_t));

	if(wcp_filename == NULL)
	{
		free(wcp_fullstring);
		free(wcp_library);
		return(NOT_OKAY);
	}

	if(pstrSplitLibrarySlashFilenameSpecification_Wide(wcp_fullstring, wcp_library, wcp_filename, ip_library_and_filename_found) != OKAY)
	{
		free(wcp_fullstring);
		free(wcp_library);
		free(wcp_filename);
		return(NOT_OKAY);
	}

	if(pstrConvertWideCharStringToAnsiCharString(wcp_library, cp_library, strlen(cp_fullstring) + 1) != OKAY)
	{
		free(wcp_fullstring);
		free(wcp_library);
		free(wcp_filename);
		return(NOT_OKAY);
	}
	if(pstrConvertWideCharStringToAnsiCharString(wcp_filename, cp_filename, strlen(cp_fullstring) + 1) != OKAY)
	{
		free(wcp_fullstring);
		free(wcp_library);
		free(wcp_filename);
		return(NOT_OKAY);
	}

	free(wcp_fullstring);
	free(wcp_library);
	free(wcp_filename);

	return(OKAY);
}

/*
 * FUNCTION: pstrSplitLibrarySlashFilenameSpecification_Wide()
 * DESCRIPTION:
 *
 *  Based on the passed full library/filename specification this function splits it up and
 *  passes back the libary and then filename.
 * 
 *  Ex1:
 *  cp_fullstring : factory\test.txt
 *    cp_library = factory
 *    cp_filename = test.txt
 *
 *  Ex2:
 *  cp_fullstring : factory/test.txt
 *    cp_library = factory
 *    cp_filename = test.txt
 */
int PT_DECLSPEC pstrSplitLibrarySlashFilenameSpecification_Wide(wchar_t *wcp_fullstring, 
																		     wchar_t *wcp_library, 
																	        wchar_t *wcp_filename,
																		     int *ip_library_and_filename_found)
{
   *ip_library_and_filename_found = IS_FALSE;

   if (pstrCalcLibraryFromFullpath_Wide(wcp_fullstring, wcp_library, ip_library_and_filename_found) != OKAY)
		return(NOT_OKAY);

	if (*ip_library_and_filename_found == IS_FALSE)
		return(OKAY);

   if (pstrCalcFilenameFromFullpath_Wide(wcp_fullstring, wcp_filename, ip_library_and_filename_found) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: pstrSplitUserLibraryFilenameSpecification()
 * DESCRIPTION:
 *
 *  Based on the passed full user/library/filename specification this function splits it up and
 *  passes back the user, library and then filename.
 * 
 *  Ex1:
 *  cp_fullstring : joe\factory\test.txt
 *    cp_user = joe
 *    cp_library = factory
 *    cp_filename = test.txt
 *
 *  Ex2:
 *  cp_fullstring : joe/factory/test.txt
 *    cp_user = joe
 *    cp_library = factory
 *    cp_filename = test.txt
 */
int PT_DECLSPEC pstrSplitUserLibraryFilenameSpecification(char *cp_fullstring, 
																			 char *cp_user, 
																		    char *cp_library, 
																	       char *cp_filename,
																		    int *ip_ulf_specification_found)
{
	wchar_t *wcp_fullstring;
	wchar_t *wcp_user;
	wchar_t *wcp_library;
	wchar_t *wcp_filename;
	int i_fullstring_widechar_size;
	
	if(pstrConvertToWideCharString_WithAlloc(cp_fullstring, &wcp_fullstring, &i_fullstring_widechar_size) != OKAY)
		return(NOT_OKAY);

	wcp_user = (wchar_t *)calloc(strlen(cp_fullstring) + 1, sizeof(wchar_t));

	if(wcp_user == NULL)
	{
		free(wcp_fullstring);
		return(NOT_OKAY);
	}

	wcp_library = (wchar_t *)calloc(strlen(cp_fullstring) + 1, sizeof(wchar_t));

	if(wcp_library == NULL)
	{
		free(wcp_fullstring);
		free(wcp_user);
		return(NOT_OKAY);
	}

	wcp_filename = (wchar_t *)calloc(strlen(cp_fullstring) + 1, sizeof(wchar_t));

	if(wcp_filename == NULL)
	{
		free(wcp_fullstring);
		free(wcp_user);
		free(wcp_library);
		return(NOT_OKAY);
	}

	if(pstrSplitUserLibraryFilenameSpecification_Wide(wcp_fullstring, wcp_user, wcp_library, wcp_filename, ip_ulf_specification_found) != OKAY)
	{
		free(wcp_fullstring);
		free(wcp_user);
		free(wcp_library);
		free(wcp_filename);
		return(NOT_OKAY);
	}
	
	if(pstrConvertWideCharStringToAnsiCharString(wcp_user, cp_user, strlen(cp_fullstring) + 1) != OKAY)
	{
		free(wcp_fullstring);
		free(wcp_user);
		free(wcp_library);
		free(wcp_filename);
		return(NOT_OKAY);
	}
	if(pstrConvertWideCharStringToAnsiCharString(wcp_library, cp_library, strlen(cp_fullstring) + 1) != OKAY)
	{
		free(wcp_fullstring);
		free(wcp_user);
		free(wcp_library);
		free(wcp_filename);
		return(NOT_OKAY);
	}
	if(pstrConvertWideCharStringToAnsiCharString(wcp_filename, cp_filename, strlen(cp_fullstring) + 1) != OKAY)
	{
		free(wcp_fullstring);
		free(wcp_user);
		free(wcp_library);
		free(wcp_filename);
		return(NOT_OKAY);
	}

	free(wcp_fullstring);
	free(wcp_user);
	free(wcp_library);
	free(wcp_filename);

	return(OKAY);
}

/*
 * FUNCTION: pstrSplitUserLibraryFilenameSpecification_Wide()
 * DESCRIPTION:
 *
 *  Based on the passed full user/library/filename specification this function splits it up and
 *  passes back the user, library and then filename.
 * 
 *  Ex1:
 *  cp_fullstring : joe\factory\test.txt
 *    cp_user = joe
 *    cp_library = factory
 *    cp_filename = test.txt
 *
 *  Ex2:
 *  cp_fullstring : joe/factory/test.txt
 *    cp_user = joe
 *    cp_library = factory
 *    cp_filename = test.txt
 */
int PT_DECLSPEC pstrSplitUserLibraryFilenameSpecification_Wide(wchar_t *wcp_fullstring, 
																			 wchar_t *wcp_user, 
																		    wchar_t *wcp_library, 
																	       wchar_t *wcp_filename,
																		    int *ip_ulf_specification_found)
{
   *ip_ulf_specification_found = IS_FALSE;

	/* 
	 * We are parsing a special type of specification that does not contain the ulf_type.  
	 * We use the Component Dirname parse function for the user because it works.
	 */
   if (pstrCalcUserComponentDirnameFromFullpath_Wide(wcp_fullstring, wcp_user, ip_ulf_specification_found) != OKAY)
		return(NOT_OKAY);

	if (*ip_ulf_specification_found == IS_FALSE)
		return(OKAY);

	/* Calc the library */
   if (pstrCalcLibraryFromFullpath_Wide(wcp_fullstring, wcp_library, ip_ulf_specification_found) != OKAY)
		return(NOT_OKAY);

	if (*ip_ulf_specification_found == IS_FALSE)
		return(OKAY);

	/* Calc the name */
   if (pstrCalcFilenameFromFullpath_Wide(wcp_fullstring, wcp_filename, ip_ulf_specification_found) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: pstrCalcParentFolderFullpath()
 * DESCRIPTION:
 *
 *  Based on the passed fullpath this function calculates and passes back the fullpath to the parent
 *  of either the file or folder specified.
 *
 *  Note: This function works for folders seperated with forward and backslashes.
 *
 *  Ex1. cp_fullpath: "C:\Folder1\SubFolder\file.txt"
 *      cp_parent_fullpath: "C:\Folder1\SubFolder"
 *
 *  Ex2. cp_fullpath: "C:/Folder1/SubFolder/file.txt"
 *      cp_parent_fullpath: "C:\Folder1\SubFolder"
 *
 */
int PT_DECLSPEC pstrCalcParentFolderFullpath(char *cp_orig_fullpath, char *cp_parent_fullpath, int *ip_parent_found)
{
	wchar_t *wcp_orig_fullpath;
	wchar_t *wcp_parent_fullpath;
	int i_orig_fullpath_widechar_size;

	if(pstrConvertToWideCharString_WithAlloc(cp_orig_fullpath, &wcp_orig_fullpath, &i_orig_fullpath_widechar_size) != OKAY)
		return(NOT_OKAY);
	
	wcp_parent_fullpath = (wchar_t *)calloc(strlen(cp_orig_fullpath) + 1, sizeof(wchar_t));

	if(wcp_parent_fullpath == NULL)
	{
		free(wcp_orig_fullpath);
		return(NOT_OKAY);
	}

	if(pstrCalcParentFolderFullpath_Wide(wcp_orig_fullpath, wcp_parent_fullpath, ip_parent_found) != OKAY)
	{
		free(wcp_orig_fullpath);
		free(wcp_parent_fullpath);
		return(NOT_OKAY);
	}
	
	if(pstrConvertWideCharStringToAnsiCharString(wcp_parent_fullpath, cp_parent_fullpath, strlen(cp_orig_fullpath) + 1) != OKAY)
	{
		free(wcp_orig_fullpath);
		free(wcp_parent_fullpath);
		return(NOT_OKAY);
	}

	free(wcp_orig_fullpath);
	free(wcp_parent_fullpath);

	return(OKAY);
}


/*
 * FUNCTION: pstrCalcParentFolderFullpath_Wide()
 * DESCRIPTION:
 *
 *  Based on the passed fullpath this function calculates and passes back the fullpath to the parent
 *  of either the file or folder specified.
 *
 *  Note: This function works for folders seperated with forward and backslashes.
 *
 *  Ex1. cp_fullpath: "C:\Folder1\SubFolder\file.txt"
 *      cp_parent_fullpath: "C:\Folder1\SubFolder"
 *
 *  Ex2. cp_fullpath: "C:/Folder1/SubFolder/file.txt"
 *      cp_parent_fullpath: "C:\Folder1\SubFolder"
 *
 */
int PT_DECLSPEC pstrCalcParentFolderFullpath_Wide(wchar_t *wcp_orig_fullpath, wchar_t *wcp_parent_fullpath, int *ip_parent_found)
{
	int length;
	int index;
	int location_of_last_slash;
	
	if (wcp_orig_fullpath == NULL)
		return(NOT_OKAY);
	if (wcp_parent_fullpath == NULL)
		return(NOT_OKAY);

	*ip_parent_found = IS_FALSE;

	length = (int)wcslen(wcp_orig_fullpath);

	if (length <= 0)
      return(OKAY);

   index = length - 1;

	while ((index >= 0) && (!(*ip_parent_found)))
	{
		if ((wcp_orig_fullpath[index] == L'\\') || (wcp_orig_fullpath[index] == L'/'))
		{
			location_of_last_slash = index;
			*ip_parent_found = IS_TRUE;
		}
		else
			index--;
	}

	/* If there is no slash, then there must not be a parent */
	if (!(*ip_parent_found))
	{
		return(OKAY);
	}
   
	/* Copy the parent fullpath */
	for (index = 0; index < location_of_last_slash; index++)
	{
		   wcp_parent_fullpath[index] = wcp_orig_fullpath[index];
	}

	wcp_parent_fullpath[location_of_last_slash] = L'\0';

   *ip_parent_found = IS_TRUE;

   return(OKAY);
}