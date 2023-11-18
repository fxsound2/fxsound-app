/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: Dongle.h
 * DATE:10/10/97
 * AUTOHOR: Mark Kaplan
 * DESCRIPTION:
 *
 *  Public defines for the dongle module
 */
#ifndef _DONGLE_H_
#define _DONGLE_H_

#include "slout.h" 

/* dongle.cpp */
int PT_DECLSPEC dongleCheckValid(int *, CSlout *);
int PT_DECLSPEC dongleWriteMemory(int, unsigned long, CSlout *hp_slout);
int PT_DECLSPEC dongleReadMemory(int, unsigned long *, CSlout *hp_slout);

#endif //_DONGLE_H_
