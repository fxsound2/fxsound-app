/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: u_qnt.h
 * DATE: 2/24/95
 * AUTOHOR: Mark Kaplan
 * DESCRIPTION:
 *
 * Local header file for the qnt module
 */
#ifndef _U_QNT_H_
#define _U_QNT_H_

#include "slout.h"
#include "filt.h"

#define QNT_INT_TO_INT   1
#define QNT_INT_TO_REAL  2
#define QNT_INT_TO_LONG  3
#define QNT_REAL_TO_INT  4
#define QNT_REAL_TO_REAL 5
#define QNT_REAL_TO_LONG 6
#define QNT_INT_TO_REAL_VOLUME 7
#define QNT_INT_TO_BOOST_CUT 8

/* Function key list handle definition */
struct qntHdlType {
	CSlout *slout_hdl;
	char msg1[1024]; /* String for messages */

	int in_out_mode;  
    
   int array_size;
   int min_int_input;    
   int *int_array;
   long *long_array;
   realtype *real_array;
	struct filt2ndOrderBoostCutShelfFilterType *filt_array;

	realtype r_input_min;
	realtype r_input_max;

	int i_output_min;
	int i_output_max;
	realtype r_output_min;
	realtype r_output_max;
	long l_output_min;
	long l_output_max;

	int output_quantized_flag;
	int num_output_levels;
	
	int i_force_value_index;

	int levels_unequal_flag;
	realtype r_scale;
   realtype r_scale_inv;
   realtype half_delta;
}; 

#endif // _U_QNT_H_

