/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: u_reg.h
 * DATE: 9/10/97
 * AUTOHOR: Mark Kaplan
 * DESCRIPTION:
 *
 * Local header file for the reg module
 */
#ifndef _U_REG_H_
#define _U_REG_H_

/* Local Functions */

/* regRecursiveDelete.cpp */
DWORD reg_DeleteKeyNT_Wide(HKEY hStartKey, LPWSTR pKeyName);

#endif //_U_REG_H_