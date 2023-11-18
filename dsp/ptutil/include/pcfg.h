/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: pcfg.h
 * DATE: 5/25/95
 * AUTOHOR: Mark Kaplan
 * DESCRIPTION:
 *
 *  Public defines for the pcfg module.  This is for the configuration of the processors.
 */
#ifndef _PCFG_H_
#define _PCFG_H_

#include "slout.h"  
#include "codedefs.h"

/* The different input and output types */
#define PCFG_ANALOG  0
#define PCFG_AES     1 
#define PCFG_SPDIF   2

/* The different board addresses */
#define PCFG_BOARD_ADDRESS_1 0x240
#define PCFG_BOARD_ADDRESS_2 0x2C0
#define PCFG_BOARD_ADDRESS_3 0x340
#define PCFG_BOARD_ADDRESS_4 0x3A0
#define PCFG_BOARD_ADDRESS_5 0x640
#define PCFG_BOARD_ADDRESS_6 0x6C0
#define PCFG_BOARD_ADDRESS_7 0x740
#define PCFG_BOARD_ADDRESS_8 0x7A0

/* pcfg.c */
int PT_DECLSPEC pcfgCheckFileType(char *, int *);
int PT_DECLSPEC pcfgRead(PT_HANDLE **, CSlout *, char *);  
int PT_DECLSPEC pcfgWrite(PT_HANDLE *);
int PT_DECLSPEC pcfgFreeUp(PT_HANDLE **);
int PT_DECLSPEC pcfgDump(PT_HANDLE *); 
int PT_DECLSPEC pcfgCalcFilename(int, char *);

/* pcfgGet.c */
int PT_DECLSPEC pcfgGetSampFreq(PT_HANDLE *, realtype *);
int PT_DECLSPEC pcfgGetBoardAddress(PT_HANDLE *, int *);
int PT_DECLSPEC pcfgGetMidiChannel(PT_HANDLE *, int *);
int PT_DECLSPEC pcfgGetMidiInputDevice(PT_HANDLE *, int *);
int PT_DECLSPEC pcfgGetMidiOutputDevice(PT_HANDLE *, int *);
int PT_DECLSPEC pcfgGetSoundCardIn(PT_HANDLE *, int *); 
int PT_DECLSPEC pcfgGetSoundCardOut(PT_HANDLE *, int *);
int PT_DECLSPEC pcfgGetMidiControlMode(PT_HANDLE *, int *);
int PT_DECLSPEC pcfgGetInputType(PT_HANDLE *, int *); 
int PT_DECLSPEC pcfgGetOutputType(PT_HANDLE *, int *);    
int PT_DECLSPEC pcfgGetFxLinkSetting(PT_HANDLE *, int *); 
int PT_DECLSPEC pcfgGetDefaultApplication(PT_HANDLE *, char *);    
int PT_DECLSPEC pcfgGetInputWavePath(PT_HANDLE *, char *);     
int PT_DECLSPEC pcfgGetOutputWavePath(PT_HANDLE *, char *); 
int PT_DECLSPEC pcfgGetSaveWaveOutputFlag(PT_HANDLE *, int *);
int PT_DECLSPEC pcfgGetCrackCount(PT_HANDLE *, int *);
int PT_DECLSPEC pcfgGetWindowPosition(PT_HANDLE *, int *, int *, int *);
int PT_DECLSPEC pcfgGetBypassFlag(PT_HANDLE *, int *);

/* pcfgSet.c */
int PT_DECLSPEC pcfgSetSampFreq(PT_HANDLE *, realtype);
int PT_DECLSPEC pcfgSetBoardAddress(PT_HANDLE *, int);
int PT_DECLSPEC pcfgSetMidiChannel(PT_HANDLE *, int);
int PT_DECLSPEC pcfgSetMidiInputDevice(PT_HANDLE *, int);
int PT_DECLSPEC pcfgSetMidiOutputDevice(PT_HANDLE *, int);  
int PT_DECLSPEC pcfgSetSoundCardIn(PT_HANDLE *, int); 
int PT_DECLSPEC pcfgSetSoundCardOut(PT_HANDLE *, int); 
int PT_DECLSPEC pcfgSetMidiControlMode(PT_HANDLE *, int);
int PT_DECLSPEC pcfgSetInputType(PT_HANDLE *, int); 
int PT_DECLSPEC pcfgSetOutputType(PT_HANDLE *, int);  
int PT_DECLSPEC pcfgSetFxLinkSetting(PT_HANDLE *, int);
int PT_DECLSPEC pcfgSetDefaultApplication(PT_HANDLE *, char *);  
int PT_DECLSPEC pcfgSetInputWavePath(PT_HANDLE *, char *);    
int PT_DECLSPEC pcfgSetOutputWavePath(PT_HANDLE *, char *); 
int PT_DECLSPEC pcfgSetSaveWaveOutputFlag(PT_HANDLE *, int);
int PT_DECLSPEC pcfgSetCrackCount(PT_HANDLE *, int);
int PT_DECLSPEC pcfgSetWindowPosition(PT_HANDLE *, int, int);
int PT_DECLSPEC pcfgSetWindowIconization(PT_HANDLE *, int);
int PT_DECLSPEC pcfgSetBypassFlag(PT_HANDLE *, int);

/* pcfgUtil */
int PT_DECLSPEC pcfgCalcDefaultAppExeName(int, char *, char *, CSlout *, char *);

#endif //_PCFG_H_
