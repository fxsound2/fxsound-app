/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: dsp_mem1.h
 * DATE: 9/1/95
 * AUTHOR: Paul Titchener
 * DESCRIPTION:
 *
 *  Public defines to be used for memory configuration for DSP apps.
 *  This version is typically used in dly, chorus and flanging apps.
 */

/* Set up default DSP Program Size. Currently pitch.4 is biggest at 0x1000 */
#ifdef DSP_TARGET
#ifndef DSP_PROGRAM_SIZE
#define DSP_PROGRAM_SIZE 0x1000
#endif
#else
#define DSP_PROGRAM_SIZE 0
#endif

#if defined(DSPSOFT_TARGET)
/* PC DSP algorithm case */
/* Declare PC memory array. Will be used via pointer references
 * for all array memory inside algorithms
 *
 * Original declared in dsps\dspFunc.c. Since processing calls
 * are only made from a single dll instance, this memory does
 * not need to be shared, and is unique to
 * each instance of the dll.
 */
/* extern long MEMBANK0_START[]; MOVED INTO .C FILES */

#define MEMBANK0_LEN 0
#define MEMBANK0_BIG_LEN DSPSOFT_MEM_LENGTH
#define MEMBANK1_START NULL

#else

/* Hardware case, Board Specific Defines */
#if defined(DSPFX1)
#define MEMBANK0_START (0x1000 + DSP_PROGRAM_SIZE) /* Includes room for executable */
#define MEMBANK0_LEN (0x8000 - DSP_PROGRAM_SIZE)   /* 32k in first bank */
#define MEMBANK0_END (MEMBANK0_START + MEMBANK0_LEN)
#define MEMBANK0_BIG_LEN (0x20000 - DSP_PROGRAM_SIZE) /* 128k when big mem present */
#define MEMBANK0_BIG_END (MEMBANK0_START + MEMBANK0_BIG_LEN)
#define MEMBANK0_MID (MEMBANK0_START + MEMBANK0_LEN/2)
#define MEMBANK1_START 0x00900000
#define MEMBANK1_LEN 0x20000 /* 128k optionally installed in 2nd bank */
#define MEMBANK1_END (MEMBANK1_START + MEMBANK1_LEN)
#define MEMBANK1_MID (MEMBANK1_START + MEMBANK1_LEN/2)
#endif /* DSPFX1 */

#endif

#ifdef NAMM_SHOW
/* TEMPORARY HACKED MEMORY BANK SETTINGS TO USE 128K (0x20000) FOR NAMM */
/* Note - switching memory access between banks adds a wait state */
/* Required modifying file dsplink.l30 for memory destinations */
/* ALSO REQUIRED MODIFYING FILE pt32a.cmd, used by hex30 converter */
#ifdef DSPFX1
#define MEMBANK0_START (0x00900000 + DSP_PROGRAM_SIZE) /* Includes room for executable */
#define MEMBANK0_LEN (0x20000 - DSP_PROGRAM_SIZE) /* 128k optionally installed in 2nd bank */
#define MEMBANK0_END (MEMBANK0_START + MEMBANK0_LEN)
#define MEMBANK0_MID (MEMBANK0_START + MEMBANK0_LEN/2)
#endif /* DSPFX1 */       
#endif /* NAMM_SHOW */

/* TIGER32 board has 64K words in 2 32K word banks */
/* Best "Block interleaved" Order- 0,7,3,4,1,6,2,5 */
/* Currently program uses memory up to 0x12c3 */
#ifdef DSPR_TIGER32
#define MEMBANK0_START 0x1300 /* Includes 0x300 room for executable */
/* #define MEMBANK0_START 0x00900000 */ /* Includes 0x300 room for executable */
#define MEMBANK0_LEN (0x8000 - 0x300)
#define MEMBANK0_END (MEMBANK0_START + MEMBANK0_LEN)
#define MEMBANK0_MID (MEMBANK0_START + MEMBANK0_LEN/2)
#define MEMBANK1_START 0x00900000
#define MEMBANK1_LEN 0x8000
#define MEMBANK1_END (MEMBANK1_START + MEMBANK1_LEN)
#define MEMBANK1_MID (MEMBANK1_START + MEMBANK1_LEN/2)
#endif /* DSPR_TIGER32 */

/* Using DRAM memory on Ariel Cyclops board, has 512K words in one bank */
/* Best "Block interleaved" Order- 0,7,3,4,1,6,2,5 */
#ifdef ARIEL_CYCLOPS            
#define MEMBANK0_START 0x80000000 /* Includes 0x300 room for executable */
#define MEMBANK0_LEN 0x20000
#define MEMBANK0_END (MEMBANK0_START + MEMBANK0_LEN)
#define MEMBANK0_MID (MEMBANK0_START + MEMBANK0_LEN/2)
#define MEMBANK1_START 0x00900000
#define MEMBANK1_LEN 0x8000
#define MEMBANK1_END (MEMBANK1_START + MEMBANK1_LEN)
#define MEMBANK1_MID (MEMBANK1_START + MEMBANK1_LEN/2)
#endif /* ARIEL_CYCLOPS */


