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
#ifndef _REG_H_
#define _REG_H_

#include "slout.h" 

/* Different types of root registry classes */
#define REG_CLASSES_ROOT  1
#define REG_LOCAL_MACHINE 2
#define REG_USERS         3
#define REG_CURRENT_USER  4

#define REG_MAX_KEY_LENGTH 256

#define REG_WIN_95_CURRENT_VERSION_PATH_WIDE		 L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion"
#define REG_WIN_NT_CURRENT_VERSION_PATH_WIDE		 L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"

#define REG_REGISTERED_OWNER_KEYNAME_WIDE			 L"RegisteredOwner"
#define REG_REGISTERED_OWNER_DEFAULT_VALUE_WIDE  L"default"

/* regWithoutKeyname.cpp */
int PT_DECLSPEC regReadTopDir_Wide(wchar_t *, int, int, int, CSlout *);
int PT_DECLSPEC regReadRegisteredOwner(char *, int);
int PT_DECLSPEC regReadRegisteredOwner_Wide(wchar_t *, int);
int PT_DECLSPEC regRemoveKey(int, char *);
int PT_DECLSPEC regRemoveKey_Wide(int, wchar_t *);
int PT_DECLSPEC regCreateKey(int, char *, char *);
int PT_DECLSPEC regCreateKey_Wide(int, wchar_t *, wchar_t *);
int PT_DECLSPEC regReadKey(int, char *, int *, char *, unsigned long);
int PT_DECLSPEC regReadKey_Wide(int, wchar_t *, int *, wchar_t *, unsigned long);
int PT_DECLSPEC regCreateKeyTest_Wide(int, wchar_t *, wchar_t *, int *);

/* regWithKeyname.cpp */
int PT_DECLSPEC regCreateKeyWithKeyname_Dword_Wide(int, wchar_t *, wchar_t *, unsigned long);
int PT_DECLSPEC regCreateKeyWithKeyname_String_Wide(int, wchar_t *, wchar_t *, wchar_t *);
int PT_DECLSPEC regReadKeyWithKeyname_String_Wide(int, wchar_t *, wchar_t *, int *, wchar_t *, unsigned long);
int PT_DECLSPEC regReadKeyWithKeyname_Dword_Wide(int, wchar_t *, wchar_t *, int *, unsigned long *);

/* regRecursiveDelete.cpp */
int PT_DECLSPEC regRecursiveDeleteFolder_Wide(int, wchar_t *);

#endif //_REG_H_
