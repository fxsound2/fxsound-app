/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: kerdma.h
 * DATE: 1/5/97
 * AUTHOR: Paul Titchener
 * DESCRIPTION:
 *
 *  Public defines and macros to be used for DMA based programs.
 */
#if (defined(DUIO_B) | defined(DUIO_BA) | defined(DUIO_BD))
/* Global to code - LOOK OUT- CODE SEEMS VERY SENSITIVE TO LOCATION OF THESE */
#define DMA_GLOBAL_DECLARATIONS \
volatile unsigned long *comd_stat = (unsigned long *)(0x82FFF0);\
volatile unsigned long *xfer_reg = (unsigned long *)(0x82fff3);\
long *in_data_buf0;\
long *in_data_buf1;\
long *out_data_buf0;\
long *out_data_buf1;\
long *read_in_buf;\
long *read_out_buf;\
static long parm_address_MACRO;\
static long address_valid_flag_MACRO = 0;

/* Local to code (just above main run loop) LOOK OUT- CODE SEEMS VERY SENSITIVE TO LOCATION OF THESE
 * buf_num, data_index, dma_mode_flag are set to start off, will use buf0 first.
 * Note that since load_parameter is called multiple places, we need to move declaration
 * of statics to a common place. load_parameter takes two calls to complete load.
 */
/* Note- so dutilInitAIO would work for .ba case, moved
 * parm_address_MACRO and address_valid_flag_MACRO up
 * to global declarations.
 */
#define DMA_LOCAL_DECLARATIONS \
unsigned dma_mode_flag = DMA_OFF_MODE;\
unsigned buf_num  = 1;\
unsigned data_index;\
unsigned first_time_output = 1;\
unsigned first_time_input = 1; \
static long in_meter1_dma = 0; \
static long in_meter2_dma = 0;
static long out_meter1_dma = 0; \
static long out_meter2_dma = 0;

/* Macro version. For some reason, subroutine version causes pops
 * when used in program. See subroutine version for comments.
 */
#define kerdmainit_MACRO()\
	{\
 		unsigned i;\
 		for(i=0; i<(4 * dma_buf_size); i++)\
 		in_data_buf0[i] = 0L;\
 	}\
	data_index = dma_buf_size;\
    DMA_ADDR(0)->gcontrol = 0;\
    DMA_ADDR(1)->gcontrol = 0;\
    DMA_ADDR(0)->transfer_counter = 0;\
    DMA_ADDR(1)->transfer_counter = 0;\
    DMA_ADDR(0)->source = (unsigned)(xfer_reg);\
	DMA_ADDR(0)->destination = (unsigned)in_data_buf0;\
	DMA_ADDR(1)->source = (unsigned)out_data_buf0;\
    DMA_ADDR(1)->destination = (unsigned)(xfer_reg);\
    DMA_ADDR(0)->gcontrol = INCDST | SYNC1 | TC | DMA_PRI;\
    DMA_ADDR(1)->gcontrol = INCSRC | SYNC2 | TC | DMA_PRI;\
    asm("	PUSH AR2");\
    asm("	LDI	 2001h,AR2");\
    asm("	LSH  16,AR2");\
	asm("	OR   AR2,IE");\
	asm("	POP  AR2");\
	*(volatile long *)(DSP_DMA_IN_TRANSFER) = 0L;

#endif
/* DUIO_B */

#ifdef DSPSOFT_TARGET

#define DMA_GLOBAL_DECLARATIONS
#define DMA_LOCAL_DECLARATIONS

#define kerdmainit_MACRO() ;

#endif /* DSPSOFT_TARGET */
	  
#if (defined(DUIO_A) | defined(DUIO_D))
#define kerdmainit_MACRO() ;
#define DMA_LOCAL_DECLARATIONS
#define DMA_GLOBAL_DECLARATIONS
#endif    
