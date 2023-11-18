/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: mry.h
 * DATE: 2/16/95
 * AUTOHOR: Mark Kaplan
 * DESCRIPTION:
 *
 *  Public defines for the memory module
 */
#ifndef _MRY_H_
#define _MRY_H_
                                    
int PT_DECLSPEC mryFreeUpStringArray(char ***, int);
int PT_DECLSPEC mryFreeUpStringArray_Wide(wchar_t ***, int);

#endif // _MRY_H
