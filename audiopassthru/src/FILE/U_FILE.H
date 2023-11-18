/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: u_file.h
 * DATE: 2/18/95
 * AUTOHOR: Mark Kaplan
 * DESCRIPTION:
 *
 * Local header file for the file module
 */
#ifndef _U_FILE_H_
#define _U_FILE_H_

#define FILE_TO_STRING_CHUNK_SIZE 512
#define FILE_WAIT_ON_WRITE_BLOCK_SLEEP_INTERVAL_MSEC 100
#define FILE_WAIT_ON_WRITE_BLOCK_MAX_WAIT_SEC        60

/* FileRecursiveList.cpp */
int file_CreateRecursiveListOfAllFullpaths_Loop_Wide(wchar_t *, wchar_t ***, int, int *, int, wchar_t **, int, wchar_t *, CSlout *);

#endif //_U_FILE_H_