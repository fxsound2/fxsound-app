/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: reg.h
 * DATE: 8/10/97
 * AUTOHOR: Mark Kaplan
 * DESCRIPTION:
 *
 *  Public defines for the file module
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
