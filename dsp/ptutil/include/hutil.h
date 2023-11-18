/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: hutil.h
 * DATE: 6/6/97
 * AUTHOR: Paul Titchener
 * DESCRIPTION: Defines for hrdwr utility functions.
 * These functions can be used by the local dll or by
 * the server or client implementations.
 *
 */
 
/* Constant Defines */ 

/* Functions */ 
/* huteep.c - DSP EEPROM related functions */
int COMHRDWR_DECL comHrdEepromReadUlong(short unsigned board_address, short unsigned address, unsigned long *ulpdata);
int COMHRDWR_DECL comHrdEepromWriteUlong(short unsigned board_address, short unsigned address, unsigned long uldata);

/* hutbtld.c - Boots and Loads chosen DSP */
int COMHRDWR_DECL hutBootLoad(char *file_name, unsigned short us_processor_num);

/* husync.c - manages access to resources */
int COMHRDWR_DECL hutsyncCheckOutDsp(unsigned short us_proc_addr);
void COMHRDWR_DECL hutsyncCheckInDsp(unsigned short us_proc_addr);
void COMHRDWR_DECL hutsyncResetDsp(unsigned short us_proc_addr);

/* hutmeter.c */
int COMHRDWR_DECL hutmeterCheckIfNew(unsigned short us_proc_address);
void COMHRDWR_DECL hutmeterGetMeterValues(unsigned short us_proc_address, 
									struct hardwareMeterValType *meter_data);
/* hutmetsc.c  16 bit sound card versions */
int COMHRDWR_DECL hutmeterCheckIfNewSc(unsigned short us_proc_address);
void COMHRDWR_DECL hutmeterGetMeterValuesSc(unsigned short us_proc_address, 
									struct hardwareMeterValType *meter_data);
