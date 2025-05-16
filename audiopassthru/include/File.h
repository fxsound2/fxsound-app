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
#ifndef _FILE_H_
#define _FILE_H_

#include <stdio.h>
#include <windows.h>

#include "slout.h" 

#define FILE_DEFAULT_KEY_FILENAME      "Default.txt"
#define FILE_DEFAULT_KEY_FILENAME_WIDE L"Default.txt"

/* For line based string search function */
#define FILE_MAX_LINE_READ_LEN 1024

/* File.cpp */
FILE PT_DECLSPEC *fileOpen(char *, char *, CSlout *);
FILE PT_DECLSPEC *fileOpen_Wide(wchar_t *, wchar_t *, CSlout *);
int PT_DECLSPEC fileExist(char *, int *);
int PT_DECLSPEC fileExist_Wide(wchar_t *, int *);
int PT_DECLSPEC fileDoesPathExist_Wide(wchar_t *wcp_file_path, int *ip_exist);
int PT_DECLSPEC fileRemove(char *, CSlout *);
int PT_DECLSPEC fileRemove_Wide(wchar_t *, CSlout *);
int PT_DECLSPEC fileRemoveWithStatus_Wide(wchar_t *, int *, CSlout *);
int PT_DECLSPEC fileSize_Wide(wchar_t *, unsigned __int64 *, CSlout *);
int PT_DECLSPEC fileToString(char *, char *, int, CSlout *); 
int PT_DECLSPEC fileToString_Wide(wchar_t *, char *, int , CSlout *);
int PT_DECLSPEC fileToString_WithAllocation_Wide(wchar_t *, wchar_t **, int *, CSlout *);
int PT_DECLSPEC fileWriteString_Wide(wchar_t *, wchar_t *, CSlout *);
int PT_DECLSPEC fileIsNormalFile(char *, int *, CSlout *);
int PT_DECLSPEC fileIsNormalFile_Wide(wchar_t *, int *, CSlout *);
int PT_DECLSPEC fileIsDirectory(char *, int *, CSlout *);   
int PT_DECLSPEC fileIsDirectory_Wide(wchar_t *, int *, CSlout *);   
int PT_DECLSPEC fileSplitFullpath(char *, char *, char *, CSlout *);
int PT_DECLSPEC fileSplitFullpath_Wide(wchar_t *, wchar_t *, wchar_t *, CSlout *);
int PT_DECLSPEC fileGetDriveFromFullpath_Wide(wchar_t *, wchar_t *, CSlout *);
int PT_DECLSPEC fileNumLines(char *, int *, CSlout *);
int PT_DECLSPEC fileNumLines_Wide(wchar_t *, int *, CSlout *);
int PT_DECLSPEC fileOkToWriteInDir(char *, int *, CSlout *);
int PT_DECLSPEC fileOkToWriteInDir_Wide(wchar_t *, int *, CSlout *);
int PT_DECLSPEC fileReadNextNonBlankLine_Wide(FILE *, wchar_t *, int, wchar_t *, int, int, int *);
int PT_DECLSPEC fileCreateDirectory(char *, int *, CSlout *);
int PT_DECLSPEC fileCreateDirectory_Wide(wchar_t *, int *, CSlout *);
int PT_DECLSPEC fileWaitOnWriteBlock_Wide(wchar_t *, int *, CSlout *);
int PT_DECLSPEC fileGetUNCFolderFromUNCpath_Wide(wchar_t *, wchar_t *, CSlout *);

/* FileRm.cpp */
int PT_DECLSPEC fileDirRemove(char *, CSlout *);
int PT_DECLSPEC fileDirRemove_Wide(wchar_t *, CSlout *);
int PT_DECLSPEC fileRemoveWithWildcard_Wide(wchar_t *, CSlout *);

/* FileTail.cpp */
int PT_DECLSPEC fileTail_Wide(wchar_t *, wchar_t *, int, CSlout *);

/* FileAray.cpp */
int PT_DECLSPEC fileCreateArrayOfStrings_Wide(wchar_t *, wchar_t ***, int *, int , int , int , CSlout *);
int PT_DECLSPEC fileWriteArrayOfStrings_Wide(wchar_t *, wchar_t **, int , int , CSlout *);
int PT_DECLSPEC fileFreeArrayOfStrings_Wide(wchar_t **, int, CSlout *);

/* FileCopy.cpp */
int PT_DECLSPEC fileCopy(char *, char *, CSlout *);
int PT_DECLSPEC fileCopy_Wide(wchar_t *, wchar_t *, CSlout *);
int PT_DECLSPEC fileCopyListOfFullpaths_Wide(wchar_t **, int, wchar_t *, wchar_t *, wchar_t *, CSlout *);
int PT_DECLSPEC fileCreateDirectoryAndParents(char *, int *, CSlout *);
int PT_DECLSPEC fileCreateDirectoryAndParents_Wide(wchar_t *, int *, CSlout *);
int PT_DECLSPEC fileCopyFileAndParents(char *, char *, int *, CSlout *);
int PT_DECLSPEC fileCopyFileAndParents_Wide(wchar_t *, wchar_t *, int *, CSlout *);
int PT_DECLSPEC fileCopyFolderRecursively_Wide(wchar_t *, wchar_t *, wchar_t *, CSlout *);  

/* FileDsp.cpp */
int PT_DECLSPEC fileCalcDspPath(char *, char *, char *, char *, char *, int, 
										  int, int, int, int, realtype, realtype);

/* FileDate.cpp */
int PT_DECLSPEC fileSetBackCreateTime(char *, long, CSlout *);
int PT_DECLSPEC fileSetBackCreateTime_Wide(wchar_t *, long, CSlout *);
int PT_DECLSPEC fileGetModifiedDate(char *, FILETIME *, CSlout *);
int PT_DECLSPEC fileGetModifiedDate_Wide(wchar_t *, FILETIME *, CSlout *);
int PT_DECLSPEC fileGetCreationDate(char *, FILETIME *, CSlout *);
int PT_DECLSPEC fileGetCreationDate_Wide(wchar_t *, FILETIME *, CSlout *);
int PT_DECLSPEC fileSetModifiedDate(char *, FILETIME *, CSlout *hp_slout);
int PT_DECLSPEC fileSetModifiedDate_Wide(wchar_t *, FILETIME *, CSlout *hp_slout);
int PT_DECLSPEC fileCompareDates(char *, char *, int *, CSlout *);
int PT_DECLSPEC fileCompareDates_Wide(wchar_t *, wchar_t *, int *, CSlout *);
int PT_DECLSPEC fileTouch(char *, CSlout *);
int PT_DECLSPEC fileTouch_Wide(wchar_t *, CSlout *);
int PT_DECLSPEC fileGetModifiedDateString(char *, char *, CSlout *);
int PT_DECLSPEC fileGetModifiedDateString_Wide(wchar_t *, wchar_t *, CSlout *);

/* FileRegistry.cpp */
int PT_DECLSPEC fileRegCreateKey_Wide(wchar_t *, wchar_t *, wchar_t *);
int PT_DECLSPEC fileRegReadKey_Wide(wchar_t *, int *, wchar_t *, unsigned long);
int PT_DECLSPEC fileRegRemoveKey_Wide(wchar_t *, wchar_t *);

/* FileTrace.cpp */
int PT_DECLSPEC fileTrace_Wide(wchar_t *, wchar_t *, CSlout *);

/* FileRecursiveList.cpp */
int PT_DECLSPEC fileCreateRecursiveListOfAllFullpaths_Wide(wchar_t *, wchar_t ***, int *, int, wchar_t **, int, wchar_t *, CSlout *);

/* FileSubfoldersList.cpp */
int fileCreateListOfSubfolders_Wide(wchar_t *, wchar_t ***, int *, int, CSlout *);

/* FileSubfilesList.cpp */
int fileCreateListOfSubfiles_Wide(wchar_t *, wchar_t ***, int *, int, CSlout *);

/* FileWin32Handle.cpp */
HANDLE fileWin32CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
									LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
									DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, CSlout *hp_slout);
HANDLE fileWin32CreateFile_Wide(LPCWSTR lpwFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
												  LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
												  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, CSlout *hp_slout);

/* fileSmartStringSearch.cpp */
int PT_DECLSPEC fileSmartStringSearchLineBased(wchar_t *, wchar_t *, wchar_t *, char *, wchar_t *,	bool, bool *, bool *);
int PT_DECLSPEC fileSmartStringSearchLineBasedSingleLine(wchar_t *, wchar_t *, wchar_t *, char *, wchar_t *,	bool, bool *, bool *);


#endif //_FILE_H_
