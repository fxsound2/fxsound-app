/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: u_mry.h
 * DATE: 2/18/95
 * AUTOHOR: Mark Kaplan
 * DESCRIPTION:
 *
 * Local header file for the mem module
 */
#ifndef _U_MRY_H_
#define _U_MRY_H_

#ifdef DEF_GLOBAL
char g_mem_msg[128];
#else
extern char g_mem_msg[128];
#endif // DEF_GLOBAL

#endif //_U_MRY_H_
