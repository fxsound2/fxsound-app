/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: hrdwr.h
 * DATE: 6/6/97
 * AUTHOR: Paul Titchener
 * DESCRIPTION: Defines for the lowest level DSP commnunction functions.
 *
 */

/* Next lines set whether functions are declared for static libs or dlls */
#if defined(COMHRDWR_DLL_EXPORT) /* Defined for compiling DLL's (versus static lib) */
	#if defined(WIN32)
	#define COMHRDWR_DECL __declspec( dllexport ) /* 32 bit dll version */
	#else
	#define COMHRDWR_DECL _export /* 16 bit dll version */
	#endif
#elif defined(COMHRDWR_DLL_IMPORT) /* For compiling apps that link to dll */
	#if defined(WIN32)
	#define COMHRDWR_DECL __declspec( dllimport ) /* 32 bit dll version */
	#else
	#define COMHRDWR_DECL
	#endif
#else
	#define COMHRDWR_DECL /* Library case */
#endif
 
/* Constant Defines */ 
/* For checking if hardware is busy doing sound card operations. */
#define HRDWR_BUSY 1
#define HRDWR_NOT_BUSY 0

/* For checking if xfer regs can be read or written */
#define HRDWR_XFER_READ_FULL 1
#define HRDWR_XFER_READ_NOT_FULL 0
#define HRDWR_XFER_WRITE_EMPTY 1
#define HRDWR_XFER_WRITE_NOT_EMPTY 0

/* For checking if xfer reg has data to read or is empty to write to */
#define HRDWR_XFER_READ_FULL_MASK 0x200
#define HRDWR_XFER_WRITE_EMPTY_MASK 0x100

#define DSP_TO_PC_FLAG_MASK 0x1000
#define PC_TO_DSP_FLAG_MASK 0x2000

/* For Nil's waiting loops */
#define HRDWR_TO_COUNT 100000
#define HRDWR_TIMEOUT_SECS 5

/* Table of error codes for different initialization types */
#define HRDWR_INIT_OKAY 0
#define HRDWR_RESET_OKAY 1
#define HRDWR_SERIAL_CLIENT_RESET_ERROR 2
#define HRDWR_SERIAL_SERVER_RESET_ERROR 3

/* Sets size in longs of buffer transmitted during program loading */
#define HRDWR_LDCHAR_BUF_SIZE 128

/* hrdwr Function Codes */
#define HRDWR_NOP_0			 	(unsigned char)(0x00 << 4)
#define HRDWR_DATE_CODE 		(unsigned char)(0x01 << 4)
#define HRDWR_WRITE_COMMAND 	(unsigned char)(0x02 << 4)
#define HRDWR_READ_STATUS 		(unsigned char)(0x03 << 4)
#define HRDWR_WRITE_TRANSFER 	(unsigned char)(0x04 << 4)
#define HRDWR_READ_TRANSFER 	(unsigned char)(0x05 << 4)
#define HRDWR_LOAD_PROGRAM 		(unsigned char)(0x06 << 4)
#define HRDWR_WRITE_EEPROM 		(unsigned char)(0x07 << 4)
#define HRDWR_READ_EEPROM 		(unsigned char)(0x08 << 4)
#define HRDWR_LDCHAR_BUF	 	(unsigned char)(0x09 << 4)
#define HRDWR_WRITE_PARAMETER	(unsigned char)(0x0A << 4)
#define HRDWR_RESET_COMM		(unsigned char)(0x0B << 4)
#define HRDWR_EPROM_READ		(unsigned char)(0x0C << 4)

/* Mask for decoding commands/processor numbers */
#define HRDWR_PROC_MASK 0x0F

/* Serial interface defines */
#define HRDWR_BUF_SIZE 2048
#define HRDWR_BAUD_RATE Baud115200

/* Functions */ 
/* comHrdwr.c */
int COMHRDWR_DECL comHrdwrPutChar(int, long l_val);
int COMHRDWR_DECL comHrdwrWriteParam(int, long, long);
int COMHRDWR_DECL comHrdwrGetChar(int, long *lp_val);
int COMHRDWR_DECL comHrdwrWaitForFlag(int);
int COMHRDWR_DECL comHrdwrCheckFlag( int, int * );
int COMHRDWR_DECL comHrdwrProcessWaveBuffer(int i_processor_num, long *lp_data, long l_length, 
                         int i_stereo_in_mode, int i_stereo_out_mode);
int COMHRDWR_DECL comHrdwrProcessActiveBuffer(int i_processor_num, short *sp_data, long l_length, 
                         int i_stereo_in_mode, int i_stereo_out_mode);

/* comHrdut.c */
int COMHRDWR_DECL comHrdwrInitializeCommHrdwr(int *);
int COMHRDWR_DECL comHrdwrResetCommHrdwr(int *);
int COMHRDWR_DECL comHrdwrLdcharBuf(unsigned short us_process_num, unsigned short us_buflen, long *lp_databuf);

/* hrdwrsc.c */
int COMHRDWR_DECL hrdwrResetProcessor(unsigned short us_processor_num);

int COMHRDWR_DECL hrdwrReadXferReg(unsigned short us_proc_num, long *lp_data);
int COMHRDWR_DECL hrdwrReadXferRegIfNotBusy(unsigned short us_proc_num, short *sp_busy, long *lp_data);
int COMHRDWR_DECL hrdwrReadXferRegIfFull(unsigned short s_proc_num, long *lp_data, short *sp_full);
int COMHRDWR_DECL hrdwrReadXferRegIfFullAndNotBusy(unsigned short s_proc_num, short *sp_busy, long *lp_data, short *sp_full);
int COMHRDWR_DECL hrdwrWaitForXferRegFull(unsigned short us_proc_num, long *lp_data);

int COMHRDWR_DECL hrdwrWriteXferReg(unsigned short us_proc_num, long l_data);
int COMHRDWR_DECL hrdwrWriteXferRegIfNotBusy(unsigned short us_proc_num, long l_data, short *sp_busy);
int COMHRDWR_DECL hrdwrWriteXferRegIfEmpty(unsigned short us_proc_num, long l_data, short *sp_empty);
int COMHRDWR_DECL hrdwrWriteXferRegIfEmptyAndNotBusy(unsigned short us_proc_num, long l_data, short *sp_busy, short *sp_empty);
int COMHRDWR_DECL hrdwrWriteParameterIfEmpty(unsigned short, long, long, short *);
int COMHRDWR_DECL hrdwrWriteParameterIfEmptyAndNotBusy(unsigned short, long, long, short *, short *);

int COMHRDWR_DECL hrdwrWriteCommandReg(unsigned short us_proc_num, unsigned short u_data);
int COMHRDWR_DECL hrdwrWriteCommandRegIfNotBusy(unsigned short us_proc_num, unsigned short u_data, short *sp_busy);
int COMHRDWR_DECL hrdwrReadStatusReg(unsigned short us_proc_num, unsigned short *up_data);
int COMHRDWR_DECL hrdwrReadStatusRegIfNotBusy(unsigned short us_proc_num, short *sp_busy, unsigned short *up_data);
int COMHRDWR_DECL hrdwrGetDspFlag(unsigned short us_process_num, unsigned short *usp_dspflag);

int COMHRDWR_DECL hrdwrWaitStat(unsigned short us_process_num, unsigned short us_mask, unsigned short us_stat);

int COMHRDWR_DECL hrdwrDMAtransmitStereo(unsigned short us_proc_num, long *lp_data, long l_length);
int COMHRDWR_DECL hrdwrDMAtransmitMono(unsigned short us_proc_num, long *lp_data, long l_length);
int COMHRDWR_DECL hrdwrDMAreceiveStereo(unsigned short us_proc_num, long *lp_data, long l_length);
int COMHRDWR_DECL hrdwrDMAreceiveMono(unsigned short us_proc_num, long *lp_data, long l_length);
