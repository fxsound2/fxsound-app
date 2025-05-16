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

#include <sys/types.h>
#include <sys/stat.h>
#include <IO.H>
#include <direct.h>
#include <fcntl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Shlwapi.h"
#include "errno.h"
#include "slout.h"
#include "file.h"
#include "string.h"
#include "pstr.h"
#include "mry.h"
#include "pt_defs.h"
#include "operatingSystem.h"

#define DEF_GLOBAL
#include "u_file.h"

/*
 * FUNCTION: fileOpen()
 * DESCRIPTION:
 *
 *  Works just like fopen() except it prints an error message into
 *  the passed slout handle on error.  You can pass NULL for the hp_slout handle and
 *  it will not print an error message.
 */
FILE PT_DECLSPEC *fileOpen(char *cp_name, char *cp_mode, CSlout *hp_slout)
{
	wchar_t wcp_name[PT_MAX_PATH_STRLEN];
	wchar_t wcp_mode[PT_MAX_GENERIC_STRLEN];

	if(pstrConvertToWideCharString(cp_name, wcp_name, PT_MAX_PATH_STRLEN) != OKAY)
		return NULL;
	if(pstrConvertToWideCharString(cp_mode, wcp_mode, PT_MAX_GENERIC_STRLEN) != OKAY)
		return NULL;

	return(fileOpen_Wide(wcp_name, wcp_mode, hp_slout));
}
/*
 * FUNCTION: fileOpen_Wide()
 * DESCRIPTION:
 *
 *  Works just like fopen() except it prints an error message into
 *  the passed slout handle on error.  You can pass NULL for the hp_slout handle and
 *  it will not print an error message.
 */
FILE PT_DECLSPEC *fileOpen_Wide(wchar_t *wcp_name, wchar_t *wcp_mode, CSlout *hp_slout)
{
	FILE *stream;
	int length;
	wchar_t wcp_file_msg[128];

	if (wcp_name == NULL)
	{
		if (hp_slout != NULL)
		{
		   swprintf(wcp_file_msg, L"Unable to open file with no name.");
		   hp_slout->Message_Wide(FIRST_LINE, wcp_file_msg);
		}
		return(NULL);
	}

	if (_wfopen_s(&stream, wcp_name, wcp_mode) != 0)
	{
		if (hp_slout != NULL)
		{
		   swprintf(wcp_file_msg, L"%s: %s", wcp_name, _wcserror(errno));

		   /* Get rid of the carriage return */
		   length = (int)wcslen(wcp_file_msg);
		   if (length > 0)
		   {
		      if (wcp_file_msg[length - 1] == '\n')
			     wcp_file_msg[length - 1] = '\0';
		   }   
           
		   hp_slout->Error_Wide(FIRST_LINE, wcp_file_msg);
		}
		return(NULL);
	}

	return(stream);
}

/*
 * FUNCTION: fileWaitOnWriteBlock_Wide()
 * DESCRIPTION:
 *
 *  This function tries to open wcp_name file repeatedly with "w" mode until it succeeds or times out.
 */
int PT_DECLSPEC fileWaitOnWriteBlock_Wide(wchar_t *wcp_name, int *ip_timed_out, CSlout *hp_slout)
{
	FILE *stream;
	wchar_t wcp_file_msg[128];
	time_t current_time;
	time_t end_time;
	errno_t fopen_retval;

	if (wcp_name == NULL)
	{
		if (hp_slout != NULL)
		{
		   swprintf(wcp_file_msg, L"Unable to open file with no name.");
		   hp_slout->Message_Wide(FIRST_LINE, wcp_file_msg);
		}
		
		return(NOT_OKAY);
	}

	*ip_timed_out = IS_FALSE;
	current_time = time(NULL);
	end_time = current_time + FILE_WAIT_ON_WRITE_BLOCK_MAX_WAIT_SEC;

	do
	{
		Sleep(FILE_WAIT_ON_WRITE_BLOCK_SLEEP_INTERVAL_MSEC);
		fopen_retval = _wfopen_s(&stream, wcp_name, L"w");
		current_time = time(NULL);
	}
	while ((fopen_retval != 0) && (current_time < end_time));

	if (fopen_retval != 0)
		*ip_timed_out = IS_TRUE;

	fclose(stream);

	return(OKAY);
}

/*
 * FUNCTION: fileExist()
 * DESCRIPTION:
 *
 * Passes back whether or not the passed file exists.
 */
int PT_DECLSPEC fileExist(char *cp_name, int *ip_exist)
{
	wchar_t wcp_name[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_name, wcp_name, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);
	
	return(fileExist_Wide(wcp_name, ip_exist));
}

/*
 * FUNCTION: fileExist_Wide()
 * DESCRIPTION:
 *
 * Passes back whether or not the passed file exists.
 */
int PT_DECLSPEC fileExist_Wide(wchar_t *wcp_name, int *ip_exist)
{
	struct _stat64i32 stbuf;

	if (wcp_name == NULL)
		return(NOT_OKAY);

	if (_wstat(wcp_name, &stbuf) == -1)
		*ip_exist = IS_FALSE;
	else
		*ip_exist = IS_TRUE;

	return(OKAY);
}

/*
* FUNCTION: fileDoesPathExist_Wide()
* DESCRIPTION:
*
* Passes back whether or not the passed file path exists. It can be used to check if a directory exists.
* 2018-04-05: This was written to replace fileIsDirectory_Wide() as it did not work on some machines for whatever reason. We should
* use this to detect whether or not a directory exists from now on. 
* 
* NOTE: This depends on Shlwapi.lib
*/
int PT_DECLSPEC fileDoesPathExist_Wide(wchar_t *wcp_file_path, int *ip_exist)
{
	*ip_exist = PathFileExistsW(wcp_file_path);
	return(OKAY);
}
/*
 * FUNCTION: fileRemove()
 * DESCRIPTION:
 *
 * Removes the passed file.  You can pass NULL for the slout handle.
 */
int PT_DECLSPEC fileRemove(char *cp_name, CSlout *hp_slout)
{
	wchar_t wcp_name[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_name, wcp_name, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);
		
	return(fileRemove_Wide(wcp_name, hp_slout));
	
}

/*
 * FUNCTION: fileRemove_Wide()
 * DESCRIPTION:
 *
 * Removes the passed file.  You can pass NULL for the slout handle.
 */
int PT_DECLSPEC fileRemove_Wide(wchar_t *wcp_name, CSlout *hp_slout)
{
	int length;
	int file_exists;
	wchar_t wcp_file_msg[PT_MAX_GENERIC_STRLEN];

	if (wcp_name == NULL)
		return(NOT_OKAY);

	/* First check if the file exists */
	if (fileExist_Wide(wcp_name, &file_exists) != OKAY)
	   return(NOT_OKAY);
	
	if (!file_exists)
		return(OKAY);

	if (_wunlink(wcp_name) == -1)
	{
		if (hp_slout != NULL)
		{
		   swprintf(wcp_file_msg, L"%s: %s", wcp_name, _wcserror(errno));

		   /* Get rid of the carriage return */
		   length = (int)wcslen(wcp_file_msg);
		   if (length > 0)
		   {
		      if (wcp_file_msg[length - 1] == L'\n')
			     wcp_file_msg[length - 1] = L'\0';
		   }

		   hp_slout->Error_Wide(FIRST_LINE, wcp_file_msg);
		}
		return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: fileRemoveWithStatus_Wide()
 * DESCRIPTION:
 *
 * Removes the passed file if current file permissions allow removal, sets flag giving success or fail status.
 * Fails will typically happen if the file is open in another process.
 */
int PT_DECLSPEC fileRemoveWithStatus_Wide(wchar_t *wcp_name, int *ip_success_flag, CSlout *hp_slout)
{
	int file_exists;

	if (wcp_name == NULL)
		return(NOT_OKAY);

	*ip_success_flag = IS_TRUE;

	/* First check if the file exists */
	if (fileExist_Wide(wcp_name, &file_exists) != OKAY)
	   return(NOT_OKAY);
	
	if (!file_exists)
		return(OKAY);

	/* If remove fails set status flag. */
	if (_wunlink(wcp_name) == -1)
		*ip_success_flag = IS_FALSE;

	return(OKAY);
}

/*
 * FUNCTION: fileSize_Wide()
 * DESCRIPTION:
 */
int PT_DECLSPEC fileSize_Wide(wchar_t *wcp_file_fullpath, unsigned __int64 *ip64_size_in_bytes, CSlout *hp_slout)
{
	int file_exists;
	struct _stati64 file_stat;

	if (wcp_file_fullpath == NULL)
		return(NOT_OKAY);

	*ip64_size_in_bytes = 0;

	/* First check if the file exists */
	if (fileExist_Wide(wcp_file_fullpath, &file_exists) != OKAY)
	   return(NOT_OKAY);
	
	if (!file_exists)
		return(OKAY);

	if (_wstati64(wcp_file_fullpath, &file_stat) != 0)
		return(NOT_OKAY);

	*ip64_size_in_bytes = file_stat.st_size;

	return(OKAY);
}

/*
 * FUNCTION: fileToString()
 * DESCRIPTION:
 *   Moves the contents of the passed file into the passed (already allocated string).
 *   The length of the string must be passed.
 */
int PT_DECLSPEC fileToString(char *cp_filepath, char *cp_buffer, int i_max_length, CSlout *hp_slout)
{
	wchar_t wcp_filepath[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_filepath, wcp_filepath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);
		
	return(fileToString_Wide(wcp_filepath, cp_buffer, i_max_length, hp_slout));
} 

/*
 * FUNCTION: fileToString_Wide()
 * DESCRIPTION:
 *   Moves the contents of the passed file into the passed (already allocated string).
 *   The length of the string must be passed.
 */
int PT_DECLSPEC fileToString_Wide(wchar_t *wcp_filepath, char *cp_buffer, int i_max_length, CSlout *hp_slout)
{
	FILE *fpFile;
	int done;
	char next_char; 
	int index; 
	int file_exists;
	int prev_char_return;
	char cp_msg1[512];
	char cp_filepath[PT_MAX_PATH_STRLEN];
   
	/* Check if the file exists */
	if (fileExist_Wide(wcp_filepath, &file_exists) != OKAY)
		return(NOT_OKAY);
	if (!file_exists)
		return(NOT_OKAY); 
      
	/* Open the file */
   fpFile = fileOpen_Wide(wcp_filepath, L"r", hp_slout);
   if (fpFile == NULL)
	{
		if (hp_slout != NULL)
		{
			pstrConvertWideCharStringToAnsiCharString(wcp_filepath, cp_filepath, PT_MAX_PATH_STRLEN);
			sprintf(cp_msg1, "Unable to open file \"%s\"", cp_filepath);
			hp_slout->Error(FIRST_LINE, cp_msg1);
		}

      return(NOT_OKAY);   
	}
   
   /* 
    * Loop through adding reading one character at a time until the EOF 
    * is found.
    */
   done = IS_FALSE;
   index = 0;
   prev_char_return = IS_FALSE;

   /* The (-2) number is used to allow room for inserting a '\r' */
   while ((!done) && (index < (i_max_length - 2)))
   {      
      next_char = (char)getc(fpFile);
      if (next_char == EOF)
         done = IS_TRUE;
      else
      {
         /* 
			 * Change all the '\n' characters to '\r''\n' unless the previous
			 * character was '\r'.
			 */
         if ((next_char == '\n') && (!prev_char_return))
         {
            cp_buffer[index] = '\r';
            cp_buffer[index + 1] = '\n';
            index = index + 2;
         }
         else
         {
            cp_buffer[index] = next_char;
				index++;
         }

			/* Store if this character is a '\r' */
         if (next_char == '\r')
			   prev_char_return = IS_TRUE;
			else
			   prev_char_return = IS_FALSE;
      }
   }
   
   /* Put the end of string character */
   cp_buffer[index] = '\0';
   
   fclose(fpFile);
   
   return(OKAY);
} 

/*
 * FUNCTION: fileToString_WithAllocation_Wide()
 * DESCRIPTION:
 *   Moves the contents of the passed file into a character array (string) that is allocated in the function.
 *   Caller must free wcpp_string.
 */
int PT_DECLSPEC fileToString_WithAllocation_Wide(wchar_t * wcp_filepath, wchar_t ** wcpp_string, int * ip_strlen, CSlout *hp_slout)
{
	FILE *fpFile;
	int file_exists;
	wchar_t wcp_msg1[PT_MAX_GENERIC_STRLEN];
	char cp_chunk[FILE_TO_STRING_CHUNK_SIZE];
	wchar_t wcp_chunk[FILE_TO_STRING_CHUNK_SIZE];
	int i_characters_read;

	/* Check if the file exists */
	if (fileExist_Wide(wcp_filepath, &file_exists) != OKAY)
		return(NOT_OKAY);
	if (!file_exists)
		return(NOT_OKAY); 
      
	*wcpp_string = NULL;
	*ip_strlen = 0;

	/* Open the file */
	fpFile = fileOpen_Wide(wcp_filepath, L"r", hp_slout);
	if (fpFile == NULL)
	{
		if (hp_slout != NULL)
		{
			swprintf(wcp_msg1, L"Unable to open file \"%s\"", wcp_filepath);
			hp_slout->Error_Wide(FIRST_LINE, wcp_msg1);
		}

		return(NOT_OKAY);   
	}

	while (!feof(fpFile))
	{
		// Read 512 bytes/chars at a time, saving 1 byte/char for the null character
		i_characters_read = fread(cp_chunk, sizeof(char), FILE_TO_STRING_CHUNK_SIZE - 1, fpFile);
		*ip_strlen += i_characters_read;
		cp_chunk[i_characters_read] = '\0';
		// Read in data is in bytes (char), need to convert to wide characters
		if (pstrConvertToWideCharString(cp_chunk, wcp_chunk, FILE_TO_STRING_CHUNK_SIZE) != OKAY)
		{
			fclose(fpFile);
			return(NOT_OKAY);
		}
		if (pstrReallocAndAppendString_Wide(wcpp_string, wcp_chunk) != OKAY)
		{
			fclose(fpFile);
			return(NOT_OKAY);
		}
	}

	fclose(fpFile);

	return(OKAY);
}

/*
 * FUNCTION: fileWriteString_Wide()
 * DESCRIPTION:
 *   Given a string of wide chars, write it to a file specified by wcp_filepath.
 */
int PT_DECLSPEC fileWriteString_Wide(wchar_t * wcp_filepath, wchar_t * wcp_string, CSlout *hp_slout)
{
	FILE *fpFile;
	wchar_t wcp_msg1[PT_MAX_GENERIC_STRLEN];

	if(wcp_string == NULL)
		return(NOT_OKAY);

	/* Open the file */
	fpFile = fileOpen_Wide(wcp_filepath, L"w", hp_slout);
	if (fpFile == NULL)
	{
		if (hp_slout != NULL)
		{
			swprintf(wcp_msg1, L"Unable to open file \"%s\"", wcp_filepath);
			hp_slout->Error_Wide(FIRST_LINE, wcp_msg1);
		}

		return(NOT_OKAY);   
	}

	if(fputws(wcp_string, fpFile) == WEOF)
	{
		fclose(fpFile);
		return(NOT_OKAY);
	}

	fclose(fpFile);

	return(OKAY);
}

/*
 * FUNCTION: fileIsNormalFile()
 * DESCRIPTION:
 *
 * Passes back whether or not the passed fullpath points to a normal file.
 */
int PT_DECLSPEC fileIsNormalFile(char *cp_fullpath, int *ip_is_normal_file, CSlout *hp_slout)
{
	wchar_t wcp_fullpath[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_fullpath, wcp_fullpath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);
	
	return(fileIsNormalFile_Wide(wcp_fullpath, ip_is_normal_file, hp_slout));
} 

/*
 * FUNCTION: fileIsNormalFile_Wide()
 * DESCRIPTION:
 *
 * Passes back whether or not the passed fullpath points to a normal file.
 */
int PT_DECLSPEC fileIsNormalFile_Wide(wchar_t *wcp_fullpath, int *ip_is_normal_file, CSlout *hp_slout)
{
	struct _stat64i32 stat_buf;
    
    *ip_is_normal_file = IS_FALSE;
    
	if (wcp_fullpath == NULL)
		return(NOT_OKAY);

	if (_wstat(wcp_fullpath, &stat_buf) == -1)
		return(NOT_OKAY);
		
    if (stat_buf.st_mode & S_IFREG)
       *ip_is_normal_file = IS_TRUE;   

	return(OKAY);
} 

/*
 * FUNCTION: fileIsDirectory()
 * DESCRIPTION:
 *
 * Passes back whether or not the passed fullpath points to a directory.
 */
int PT_DECLSPEC fileIsDirectory(char *cp_fullpath, int *ip_is_directory, CSlout *hp_slout)
{
	wchar_t wcp_fullpath[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_fullpath, wcp_fullpath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);
		
	return(fileIsDirectory_Wide(wcp_fullpath, ip_is_directory, hp_slout));
}

/*
 * FUNCTION: fileIsDirectory_Wide()
 * DESCRIPTION:
 *
 * Passes back whether or not the passed fullpath points to a directory.
 */
int PT_DECLSPEC fileIsDirectory_Wide(wchar_t *wcp_fullpath, int *ip_is_directory, CSlout *hp_slout)
{
	struct _stat64i32 stat_buf;

   *ip_is_directory = IS_FALSE;
    
	if (wcp_fullpath == NULL)
		return(NOT_OKAY);

	if (_wstat(wcp_fullpath, &stat_buf) == -1)
		return(OKAY);
		
    if (stat_buf.st_mode & S_IFDIR)
	 {
       *ip_is_directory = IS_TRUE;   
	 }

	return(OKAY);
}  

/*
 * FUNCTION: fileSplitFullpath()
 * DESCRIPTION:
 *
 * Based on the passed fullpath, splits and passes back the directory, and the 
 * filename.  The passed directory and filename buffers must already be allocated.
 */
int PT_DECLSPEC fileSplitFullpath(char *cp_fullpath, char *cp_directory, 
								  char *cp_filename, CSlout *hp_slout)
{
	wchar_t wcp_fullpath[PT_MAX_PATH_STRLEN];
	wchar_t wcp_directory[PT_MAX_PATH_STRLEN];
	wchar_t wcp_filename[PT_MAX_PATH_STRLEN];
	
	if (pstrConvertToWideCharString(cp_fullpath, wcp_fullpath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);
	
   if(fileSplitFullpath_Wide(wcp_fullpath, wcp_directory, wcp_filename, hp_slout) != OKAY)
		return(NOT_OKAY);
   else
   {
		if(pstrConvertWideCharStringToAnsiCharString(wcp_directory, cp_directory, PT_MAX_PATH_STRLEN) != OKAY)
			return(NOT_OKAY);
   			
		if(pstrConvertWideCharStringToAnsiCharString(wcp_filename, cp_filename, PT_MAX_PATH_STRLEN) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: fileSplitFullpath_Wide()
 * DESCRIPTION:
 *
 * Based on the passed fullpath, splits and passes back the directory, and the 
 * filename.  The passed directory and filename buffers must already be allocated.
 */
int PT_DECLSPEC fileSplitFullpath_Wide(wchar_t *wcp_fullpath, wchar_t *wcp_directory, 
                                       wchar_t *wcp_filename, CSlout *hp_slout)
{
   wchar_t drive[_MAX_EXT];
   wchar_t dir[_MAX_EXT];
   wchar_t fname[_MAX_EXT];
   wchar_t ext[_MAX_EXT];
   
   if (wcp_fullpath == NULL)
      return(NOT_OKAY);
      
   if ((wcp_directory == NULL) || (wcp_filename == NULL))
      return(NOT_OKAY);
      
   _wsplitpath(wcp_fullpath, drive, dir, fname, ext);      
      
   swprintf(wcp_directory, L"%s%s", drive, dir);
   swprintf(wcp_filename, L"%s%s", fname, ext);
   
   return(OKAY);
}

/*
 * FUNCTION: fileGetDriveFromFullpath_Wide()
 * DESCRIPTION:
 *
 *   Given a fullpath, returns the drive letter in upper-case (without the colon).
 */
int PT_DECLSPEC fileGetDriveFromFullpath_Wide(wchar_t *wcp_fullpath, wchar_t *wcp_drive_letter, CSlout *hp_slout)
{
	wchar_t wcp_temp_drive_letter[_MAX_DRIVE];

   if (wcp_fullpath == NULL)
      return(NOT_OKAY);
    
	if (wcp_drive_letter == NULL)
		return(NOT_OKAY);

	// We only need the Drive component so we pass NULL to the rest
   _wsplitpath(wcp_fullpath, wcp_temp_drive_letter, NULL , NULL , NULL );

	// Remove the trailing colon
	if (pstrRemoveCharOccurances_Wide(wcp_temp_drive_letter, L':') != OKAY)
		return(NOT_OKAY);

	// Convert the drive letter to upper-case
	*wcp_drive_letter = towupper(wcp_temp_drive_letter[0]);

   return(OKAY);
}

/*
 * FUNCTION: fileGetUNCFolderFromUNCpath_Wide()
 * DESCRIPTION:
 *
 */
int PT_DECLSPEC fileGetUNCFolderFromUNCpath_Wide(wchar_t *wcp_unc_path, wchar_t *wcp_unc_folder_fullpath, CSlout *hp_slout)
{
	wchar_t **wcpp_array;
	wchar_t wcp_separator[] = L"\\";
	int i_num_words;
	wchar_t wcp_unc_path_backup[PT_MAX_PATH_STRLEN];

   if (wcp_unc_path == NULL)
      return(NOT_OKAY);
    
	if (wcp_unc_folder_fullpath == NULL)
		return(NOT_OKAY);

	wcpp_array = NULL;

	// Is it a valid UNC path? (ie, start with "//")
	if (wcp_unc_path[0] != L'\\' || wcp_unc_path[1] != L'\\')
		return(NOT_OKAY);

	// Make a copy of the unc path because the function pstrSplitIntoWords_Wide() butchered up the original.
	swprintf_s(wcp_unc_path_backup, PT_MAX_PATH_STRLEN, L"%s", wcp_unc_path);

	if (pstrSplitIntoWords_Wide(wcp_unc_path_backup, &wcpp_array, &i_num_words, PT_MAX_GENERIC_STRLEN, wcp_separator) != OKAY)
		return(NOT_OKAY);

	swprintf(wcp_unc_folder_fullpath, L"\\\\%s\\%s\\", wcpp_array[0], wcpp_array[1]);

	if (mryFreeUpStringArray_Wide(&wcpp_array, i_num_words) != OKAY)
		return(NOT_OKAY);

   return(OKAY);
}

/*
 * FUNCTION: fileNumLines()
 * DESCRIPTION:
 *
 *  Passes back how many lines there are in the passed file.
 */
int PT_DECLSPEC fileNumLines(char *cp_filepath, int *ip_num_lines, CSlout *hp_slout)
{
 	wchar_t wcp_filepath[PT_MAX_PATH_STRLEN];
 	
 	if(pstrConvertToWideCharString(cp_filepath, wcp_filepath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);
	
	return(fileNumLines_Wide(wcp_filepath, ip_num_lines, hp_slout));
}

/*
 * FUNCTION: fileNumLines_Wide()
 * DESCRIPTION:
 *
 *  Passes back how many lines there are in the passed file.
 */
int PT_DECLSPEC fileNumLines_Wide(wchar_t *wcp_filepath, int *ip_num_lines, CSlout *hp_slout)
{
	FILE *fp_file;
	int file_exists;
	char next_ch;
	char cp_msg1[512];
	char cp_filepath[PT_MAX_PATH_STRLEN];

	*ip_num_lines = 0;

	/* Make sure the message file exists */
	if (fileExist_Wide(wcp_filepath, &file_exists) != OKAY)
	  return(NOT_OKAY);

	if (!file_exists)
	  return(NOT_OKAY);

	/* Open the file */
	fp_file = fileOpen_Wide(wcp_filepath, L"r", hp_slout);
	if (fp_file == NULL)
	{
		if (hp_slout != NULL)
		{
			pstrConvertWideCharStringToAnsiCharString(wcp_filepath, cp_filepath, PT_MAX_PATH_STRLEN);
			sprintf(cp_msg1, "Unable to open file \"%s\"", cp_filepath);
			hp_slout->Error(FIRST_LINE, cp_msg1);
		}

	  return(NOT_OKAY);
	}

	/* Loop through all the lines */
	while (!feof(fp_file))
	{
	   next_ch = fgetc(fp_file);
	   if (next_ch == '\n')
	   {
		   *ip_num_lines = *ip_num_lines + 1;
	   }
	}

	// The last line before EOF needs to be counted, too.
	if (next_ch != '\n' && *ip_num_lines != 0)
		*ip_num_lines = *ip_num_lines + 1;

	fclose(fp_file);

	return(OKAY);
}

/*
 * FUNCTION: fileOkToWriteInDir()
 * DESCRIPTION:
 *
 * Passes back if it is ok to write a file in the passed directory.
 */
int PT_DECLSPEC fileOkToWriteInDir(char *cp_dirpath, int *ip_location_ok, CSlout *hp_slout)
{
	wchar_t wcp_dirpath[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_dirpath, wcp_dirpath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);

	return(fileOkToWriteInDir_Wide(wcp_dirpath, ip_location_ok, hp_slout));	
}    

/*
 * FUNCTION: fileOkToWriteInDir_Wide()
 * DESCRIPTION:
 *
 * Passes back if it is ok to write a file in the passed directory.
 */
int PT_DECLSPEC fileOkToWriteInDir_Wide(wchar_t *wcp_dirpath, int *ip_location_ok, CSlout *hp_slout)
{
   int is_dir;
   wchar_t tempfile_path[512];
   FILE *stream; 

   *ip_location_ok = IS_FALSE;

   if (fileIsDirectory_Wide(wcp_dirpath, &is_dir, hp_slout) != OKAY)
      return(OKAY);

   if (is_dir)
   {
	  /* Try to open a temp file in the directory. */
      swprintf(tempfile_path, L"%s\\m5xe9hk2.u3w", wcp_dirpath);
      stream = fileOpen_Wide(tempfile_path, L"w", hp_slout);
      
	  /* If we couldn't open, simply return that location is not ok to write */
	  if (stream == NULL)
         return(OKAY); 

	  /* Close the temp file */
      fclose(stream);

	  /* Delete the temp file */
      if (fileRemove_Wide(tempfile_path, hp_slout) != OKAY)
	     return(NOT_OKAY);

      *ip_location_ok = IS_TRUE;
   }

   return(OKAY);
}    

/*
 * FUNCTION: fileReadNextNonBlankLine_Wide()
 * DESCRIPTION:
 *  Reads and passes back the next non-blank line from the passed file.  A line is considered blank if
 *  it has no text or starts with the passed cp_comment_str.  If it hits the EOF or if
 *  there is a problem, then it closes the file.  The i_eof_return_value is the value to return on
 *  EOF
 */
int PT_DECLSPEC fileReadNextNonBlankLine_Wide(FILE *fp_file, wchar_t *wcp_line_str, int i_max_str_length, 
													  wchar_t *wcp_comment_str, int i_remove_trailing_spaces,
													  int i_eof_return_value, int *ip_eof)
{
	int done;
	int line_strlen;
	int comment_strlen;
	int index;

	done = IS_FALSE;
   *ip_eof = IS_FALSE;

	swprintf(wcp_line_str, L"");

	if (wcp_comment_str == NULL)
	   comment_strlen = 0;
	else
	   comment_strlen = (int)wcslen(wcp_comment_str);

	while (!done)
	{
	   if (fgetws(wcp_line_str, i_max_str_length, fp_file) == NULL)
		{
			*ip_eof = IS_TRUE;
			done = IS_TRUE;
		}
		else
		{
			line_strlen = (int)wcslen(wcp_line_str);

	      if (line_strlen > 0)
			{
				if (wcp_line_str[0] != L'\r' && wcp_line_str[0] != L'\n')
				{
					/* If it is a non-blank line that does not start with the comment_str, then we are done */
					if (wcp_comment_str != NULL)
					{
                  if (comment_strlen <= line_strlen)
						{
							for (index = 0; index < comment_strlen; index++)
							{
								if (wcp_comment_str[index] != wcp_line_str[index])
									done = IS_TRUE;
							}
						}
					}
					else
		            done = IS_TRUE;
				}
			}
		}
	}

	if (*ip_eof)
	{
		fclose(fp_file);
		return(i_eof_return_value);
   }

   /* Get rid of all the trailing \r or \n characters */
	if (pstrRemoveTrailingCarriageReturns_Wide(wcp_line_str) != OKAY)
		return(NOT_OKAY);

   /* Get rid of all the trailing spaces if requested */
	if (i_remove_trailing_spaces)
	{
	   if (pstrRemoveTrailingSpaces_Wide(wcp_line_str) != OKAY)
		   return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: fileCreateDirectory()
 * DESCRIPTION:
 *  Attempts to create the directory specified by the passed fullpath and passes back whether or
 *  not the creation was successful.  
 *
 *  If the directory already exists, then it simply passes back IS_TRUE for successful.
 */
int PT_DECLSPEC fileCreateDirectory(char *cp_fullpath, int *ip_successful, CSlout *hp_slout)
{
	wchar_t wcp_fullpath[PT_MAX_PATH_STRLEN];
	
	if(pstrConvertToWideCharString(cp_fullpath, wcp_fullpath, PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);
		
	return(fileCreateDirectory_Wide(wcp_fullpath, ip_successful, hp_slout));
}

/*
 * FUNCTION: fileCreateDirectory_Wide()
 * DESCRIPTION:
 *  Attempts to create the directory specified by the passed fullpath and passes back whether or
 *  not the creation was successful.  
 *
 *  If the directory already exists, then it simply passes back IS_TRUE for successful.
 */
int PT_DECLSPEC fileCreateDirectory_Wide(wchar_t *wcp_fullpath, int *ip_successful, CSlout *hp_slout)
{
	int is_directory;
	char cp_msg1[512];
	wchar_t wcp_fullpath_no_trailing_backslash[PT_MAX_PATH_STRLEN];
	char cp_fullpath_no_trailing_backslash[PT_MAX_PATH_STRLEN];

	*ip_successful = IS_FALSE;

	if (wcp_fullpath == NULL)
		return(NOT_OKAY);

	/* Make a version of the folder path with a trailing backslash */
	swprintf(wcp_fullpath_no_trailing_backslash, L"%s", wcp_fullpath);
	if (pstrRemoveTrailingBackslash_Wide(wcp_fullpath_no_trailing_backslash) != OKAY)
		return(NOT_OKAY);

	/* First check to see if the directory already exists */
   if (fileDoesPathExist_Wide(wcp_fullpath_no_trailing_backslash, &is_directory) != OKAY)
		return(NOT_OKAY);

	if (is_directory)
	{
		*ip_successful = IS_TRUE;
		return(OKAY);
	}
	
	/* Try to create the folder */
	*ip_successful = IS_FALSE;
	if (CreateDirectoryW(wcp_fullpath_no_trailing_backslash, NULL) != TRUE)
	{
		if (hp_slout != NULL)
		{
			pstrConvertWideCharStringToAnsiCharString(wcp_fullpath_no_trailing_backslash, cp_fullpath_no_trailing_backslash, PT_MAX_PATH_STRLEN);
			sprintf(cp_msg1, "Unable to create folder \"%s\"", cp_fullpath_no_trailing_backslash);
			hp_slout->Error(FIRST_LINE, cp_msg1);
		}
	}
	else
	   *ip_successful = IS_TRUE;


	return(OKAY);
}
