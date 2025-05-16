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
#ifndef _PSTR_H_
#define _PSTR_H_

#define PSTR_ADD_BRACKETS    0
#define PSTR_REMOVE_BRACKETS 1

#define PSTR_XML_KEY_OPENING_TAG    L"<key>"
#define PSTR_XML_KEY_CLOSING_TAG    L"</key>"

#define PSTR_XML_DICT_OPENING_TAG   L"<dict>"
#define PSTR_XML_DICT_CLOSING_TAG   L"</dict>"

#define PSTR_XML_DATA_OPENING_TAG   L"<data>"
#define PSTR_XML_DATA_CLOSING_TAG   L"</data>"

#define PSTR_XML_STRING_OPENING_TAG   L"<string>"
#define PSTR_XML_STRING_CLOSING_TAG   L"</string>"

/* String comparison defines */
#define PSTR_COMPARE_STRINGS_EQUAL			0
#define PSTR_COMPARE_FIRST_STRING_LOWER	    -1
#define PSTR_COMPARE_FIRST_STRING_HIGHER	1

#include <Windows.h>

/* pstr.cpp */
int PT_DECLSPEC pstrCalcLocationOfCharInStr(char *, char, int *, int *);
int PT_DECLSPEC pstrCalcLocationOfCharInStr_Wide(wchar_t *, wchar_t, int *, int *);
int PT_DECLSPEC pstrCalcLocationOfStrInStr_Wide(wchar_t *, wchar_t *, int, int *, int *);
int PT_DECLSPEC pstrConstructSubstring(char *, int, char, int, char *, int *);
int PT_DECLSPEC pstrConstructSubstring_Wide(wchar_t *, int, wchar_t, int, wchar_t *, int *);
int PT_DECLSPEC pstrToUpper(char *, char *);
int PT_DECLSPEC pstrToUpper_Wide(wchar_t *, wchar_t *);
int PT_DECLSPEC pstrRemovePrefixFromLine(char *, char *, char *);
int PT_DECLSPEC pstrRemovePrefixFromLine_Wide(wchar_t *, wchar_t *, wchar_t *);
int PT_DECLSPEC pstrRemovePostfixFromLine(char *, char *, char *);
int PT_DECLSPEC pstrRemovePostfixFromLine_Wide(wchar_t *, wchar_t *, wchar_t *);
int PT_DECLSPEC pstrReplaceCharOccurances(char *, char, char);
int PT_DECLSPEC pstrReplaceCharOccurances_Wide(wchar_t *, wchar_t, wchar_t);
int PT_DECLSPEC pstrRemoveCharOccurances(char *, char);
int PT_DECLSPEC pstrRemoveCharOccurances_Wide(wchar_t *, wchar_t);
int PT_DECLSPEC pstrReplaceCharOccurancesWithString(char *, char *, int, char, char *);
int PT_DECLSPEC pstrReplaceCharOccurancesWithString_Wide(wchar_t *, wchar_t *, wchar_t, wchar_t *);
int PT_DECLSPEC pstrRemoveAllSpaces(char *);
int PT_DECLSPEC pstrRemoveAllSpaces_Wide(wchar_t *);
int PT_DECLSPEC pstrRemoveAllLeadingSpaces_Wide(wchar_t *);
int PT_DECLSPEC pstrMakeNoDoublesBackSlashes(char *);
int PT_DECLSPEC pstrMakeNoDoublesBackSlashes_Wide(wchar_t *);
int PT_DECLSPEC pstrRemoveAllLeadingTabs(char *);
int PT_DECLSPEC pstrRemoveAllLeadingTabs_Wide(char *);
int PT_DECLSPEC pstrRemoveTrailingBackslash(char *);
int PT_DECLSPEC pstrRemoveTrailingBackslash_Wide(wchar_t *);
int PT_DECLSPEC pstrRemoveTrailingForwardSlash(char *);
int PT_DECLSPEC pstrRemoveTrailingForwardSlash_Wide(wchar_t *);
int PT_DECLSPEC pstrRemoveTrailingNewLine(char *);
int PT_DECLSPEC pstrRemoveTrailingNewLine_Wide(wchar_t *);
int PT_DECLSPEC pstrRemoveTrailingCarriageReturns(char *);
int PT_DECLSPEC pstrRemoveTrailingCarriageReturns_Wide(wchar_t *);
int PT_DECLSPEC pstrRemoveTrailingSpaces(char *);
int PT_DECLSPEC pstrRemoveTrailingSpaces_Wide(wchar_t *);
int PT_DECLSPEC pstrCheckStartsWithMatch(char *, char *, int *);
int PT_DECLSPEC pstrCheckStartsWithMatch_Wide(wchar_t *, wchar_t *, int *);
int PT_DECLSPEC pstrCheckEndsWithMatch(char *, char *, int *);
int PT_DECLSPEC pstrCheckEndsWithMatch_Wide(wchar_t *, wchar_t *, int *);
int PT_DECLSPEC pstrReallocAndAppendString(char **, char *);
int PT_DECLSPEC pstrReallocAndAppendString_Wide(wchar_t **, wchar_t *);
int PT_DECLSPEC pstrSplitIntoWords_Wide(wchar_t *, wchar_t ***, int *, int, wchar_t white_space[] = L" \t\n");
int PT_DECLSPEC pstrGetLastError_Wide(wchar_t *, int);
int PT_DECLSPEC pstrRemoveIndexBasedSubset_WithAllocation(wchar_t *, int, int, wchar_t **, int *);
int PT_DECLSPEC pstrInsertIndexBasedSubset_WithAllocation(wchar_t * , int, wchar_t *, wchar_t ** , int *);
int PT_DECLSPEC pstrReverse_Wide(wchar_t *, wchar_t *);
int PT_DECLSPEC pstrTruncate_Wide(wchar_t *, wchar_t *, int, BOOL);
int PT_DECLSPEC pstrReplaceIllegalFilenameChars_Wide(wchar_t *, wchar_t);
int PT_DECLSPEC pstrGenerateGUID_Wide(wchar_t *, int);

/* pstrExtension.cpp */
int PT_DECLSPEC pstrCalcExtension(char *, char *, int);
int PT_DECLSPEC pstrCalcExtension_Wide(wchar_t *, wchar_t *, int);
int PT_DECLSPEC pstrRemoveExtension(char *);
int PT_DECLSPEC pstrRemoveExtension_Wide(wchar_t *);
int PT_DECLSPEC pstrForceExtension(char *, char *);
int PT_DECLSPEC pstrForceExtension_Wide(wchar_t *, wchar_t *);

/* pstrUserLibraryFilename.cpp */
int PT_DECLSPEC pstrCalcFilenameFromFullpath(char *, char *, int *);
int PT_DECLSPEC pstrCalcFilenameFromFullpath_Wide(wchar_t *, wchar_t *, int *);
int PT_DECLSPEC pstrCalcLibraryFromFullpath(char *, char *, int *);
int PT_DECLSPEC pstrCalcLibraryFromFullpath_Wide(wchar_t *, wchar_t *, int *);
int PT_DECLSPEC pstrCalcUserFromFullpath(char *, char *, int *);
int PT_DECLSPEC pstrCalcUserFromFullpath_Wide(wchar_t *, wchar_t *, int *);
int PT_DECLSPEC pstrCalcUserComponentDirnameFromFullpath(char *, char *, int *);
int PT_DECLSPEC pstrCalcUserComponentDirnameFromFullpath_Wide(wchar_t *, wchar_t *, int *);
int PT_DECLSPEC pstrSplitLibrarySlashFilenameSpecification(char *, char *, char *, int *);
int PT_DECLSPEC pstrSplitLibrarySlashFilenameSpecification_Wide(wchar_t *, wchar_t *, wchar_t *, int *);
int PT_DECLSPEC pstrSplitUserLibraryFilenameSpecification(char *, char *, char *, char *, int *);
int PT_DECLSPEC pstrSplitUserLibraryFilenameSpecification_Wide(wchar_t *, wchar_t *, wchar_t *, wchar_t *, int *);
int PT_DECLSPEC pstrCalcParentFolderFullpath(char *, char *, int *);
int PT_DECLSPEC pstrCalcParentFolderFullpath_Wide(wchar_t *, wchar_t *, int *);

/* pstrTimeDisplay.cpp */
int PT_DECLSPEC pstrCalcHourMinSecondsString(int, char *);
int PT_DECLSPEC pstrCalcHourMinSecondsString_Wide(int, wchar_t *);

/* pstrArray.cpp */
int PT_DECLSPEC pstrAddStringToStringList_Wide(wchar_t ***, int, wchar_t *, int, int, int, int, int *);
int PT_DECLSPEC pstrArrayCopyStringList(wchar_t **, int, int, wchar_t ***);
int PT_DECLSPEC pstrCheckInArray_Wide(wchar_t *, wchar_t **, int, int, int, int *);
int PT_DECLSPEC pstrArrayFilterByExtension_Wide(wchar_t **, int, wchar_t *, wchar_t ***, int *);
int PT_DECLSPEC pstrArrayFilterByModifiedDate_Wide(wchar_t **, int, FILETIME*, int, wchar_t ***, int *, CSlout*);
int PT_DECLSPEC pstrArrayFilterByPrefix_Wide(wchar_t **, int , wchar_t * , wchar_t ***, int *);
int PT_DECLSPEC pstrArrayFilterByFilename_Wide(wchar_t **, int, wchar_t *, wchar_t ***, int *);
int PT_DECLSPEC pstrArrayRemoveTrailingBackslashes_Wide(wchar_t **, int, CSlout *);
int PT_DECLSPEC pstrArrayCombine_Wide(wchar_t **, int, wchar_t **, int, int, int, int, int, wchar_t ***, int *);
int PT_DECLSPEC pstrArrayPrependString_Wide(wchar_t ***, int, wchar_t *, CSlout *);
int PT_DECLSPEC pstrArrayReplacePrefix_Wide(wchar_t ***, int,  wchar_t *, wchar_t *, CSlout *);
int PT_DECLSPEC pstrFreeArrayOfStrings_Wide(wchar_t **, int, CSlout *);

/* pstrArraySort.cpp */
int PT_DECLSPEC pstrSortArray_Wide(wchar_t **, int, int);

/* pstrWide.cpp */
int PT_DECLSPEC pstrConvertToWideCharString(char *, wchar_t *, int);
int PT_DECLSPEC pstrConvertToWideCharString_WithAlloc(char *, wchar_t **, int *);
int PT_DECLSPEC pstrConvertWideCharStringToAnsiCharString(wchar_t *, char *, int);
int PT_DECLSPEC pstrCovertWideCharStringToUTF8String_WithAlloc(wchar_t *, char **, int *);
int PT_DECLSPEC pstrCovertUTF8StringToWideCharString_WithAlloc(char *, wchar_t **, int *);

/* pstrXML.cpp */
int PT_DECLSPEC pstrXmlGetNextKeyTag(wchar_t *, int, int *, wchar_t *, int ,int *, int *);
int PT_DECLSPEC pstrXmlGetNextDictTag(wchar_t *, int, int *, wchar_t *, int ,int *, int *);
int PT_DECLSPEC pstrXmlGetNextDataTag(wchar_t *, int, int *, wchar_t *, int ,int *, int *);
int PT_DECLSPEC pstrXmlGetNextStringTag(wchar_t *, int, int *, wchar_t *, int ,int *, int *);
int PT_DECLSPEC pstrXmlGetNextTag(wchar_t *, wchar_t *, wchar_t *, int, int *, wchar_t *, int, int *, int*);

/* pstrHidden.cpp */
int PT_DECLSPEC pstrMapStringToInt(wchar_t *, int, int, int *);

/* pstrCompare.cpp */
int PT_DECLSPEC pstrCompareStrings(wchar_t *, wchar_t *, int, int *);

/* pstrURL.cpp */
int PT_DECLSPEC pstrUrlEncode(wchar_t *, wchar_t *, int);
int PT_DECLSPEC pstrUrlEncodeURIComponent(char *, char **);
int PT_DECLSPEC pstrUrlDecodeURIComponent(char *, char **);
int PT_DECLSPEC pstrUrlConvertLocalPathToHttpProtocol(wchar_t *, wchar_t *);

#endif /* _PSTR_H_ */
