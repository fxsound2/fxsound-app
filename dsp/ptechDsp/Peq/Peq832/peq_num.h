/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */

/* File peq_num.h
 * Allows same basic code for peq1 -> peq8.
 * Need an edited copy in each peq directory. 
 */
#define NUM_ELEMS 8
#define ELEM2
#define ELEM3
#define ELEM4
#define ELEM5
#define ELEM6
#define ELEM7
#define ELEM8
#define DSPSOFT_32_BIT

/* Define function call names */
#define DSPS_PEQ_RUN dspsPeq8Run
#define DSPS_PEQ_INIT dspsPeq8Init
#define DSPS_PEQ_PROCESS dspsPeq8Process32
