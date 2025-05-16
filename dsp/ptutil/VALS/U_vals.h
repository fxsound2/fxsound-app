/*
FxSound
Copyright (C) 2025  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _U_VALS_H_
#define _U_VALS_H_

#include <Windows.h>
#include "codedefs.h"  
#include "slout.h"
#include "pt_defs.h"
                     
int vals_CalcNewMaxSum1and4(PT_HANDLE *, int);
int vals_CalcNewMax(PT_HANDLE *, int, int);
inline void vals_CalcMult(int, realtype, int, int, int, int *);
inline void vals_CalcMultRealOut(int, realtype, int, int, int, realtype *);
inline void vals_CalcMultWrapAround(int , realtype , int, int , int, int *, int *);
inline void vals_CalcAdd(int, int, int, int, int, int *);
inline void vals_CalcAddReal(int, realtype, int, int, int, int *);
inline void vals_CalcAddWrapAround(int, int, int, int, int, int *, int *);
                     
/* The max element for each element parameter type */
struct valsMaxElementData {
	int calc_flag;      /* Whether or not to caluclate max on set */
	int abs_flag;       /* Whether of not to base max on absolute value */
	int value;          /* The max value */
	long sum_scale_val; /* Scaled compare value for sum1and4 case */
	int index;          /* The element index which has max value */
};

/* The application dependent info */
struct valsAppDependData {
	int num_ints;    /* Number of app dependent integers */
	int num_reals;   /* Number of app dependent reals */
   int num_strings; /* Number of app dependent strings */
   int *int_vals;
   realtype *real_vals;
   wchar_t **wcpp_strings;
};

/* Delay Handle definition */
struct valsHdlType {
   /* Initialization info */
   CSlout *slout_hdl;
   wchar_t wcp_msg1[1024]; /* String for messages */
	int i_trace_mode;

   int total_num_elements;

   /* File info */
   realtype file_version;
   wchar_t *wcp_comment;
   
   /* Value info */  
   int double_params; /* Flag saying if using 2 sets of params */
   int param_set;     /* Which set of params to operate on VALS_PARAM_SET_1 or VALS_PARAM_SET_2 */
  
   /* Set 1 */
   int main_params[VALS_NUM_MAIN_PARAMS];
   int element_params[VALS_MAX_NUM_ELEMENTS][VALS_NUM_ELEMENT_PARAMS];

   /* Set 2 */
   int main_params_2[VALS_NUM_MAIN_PARAMS];
   int element_params_2[VALS_MAX_NUM_ELEMENTS][VALS_NUM_ELEMENT_PARAMS];
  
   /* Max element info */
   struct valsMaxElementData max[VALS_NUM_ELEMENT_PARAMS];
   
   /* 
    * Max sum of element params 1 and 4 
    * (This is used for total delay (coarse + fine)) 
    */
   struct valsMaxElementData maxSum1_4;
   int factor_1_to_4;  /* The factor by which implied value of 1 is greater than 4 */

   /* Application dependent info */
   struct valsAppDependData app_depend;

	/* EQ Info */
	PT_HANDLE *hp_graphicEq;
	int i_eq_on; 
};

/* Configuration handle definition for effect */
struct valsCfgHdlType {
	/* Initialization info */
	CSlout *slout_hdl;
	wchar_t wcp_msg1[1024]; /* String for messages */
	char cp_msg1[1024]; /* String for messages */

	/* Info */
	//char filepath[128];
	wchar_t wcp_filepath[PT_MAX_PATH_STRLEN];
	wchar_t wcp_title[PT_MAX_GENERIC_STRLEN];
	int num_elements; 
	int stereo_input_mode;  /* IS_TRUE, or IS_FALSE */
	int stereo_output_mode;  /* IS_TRUE, or IS_FALSE */
	int default_preset;
	long total_samples;
	int min_user_preset;
   long preset_save_time;  /* Number of seconds since 1/1/70 that a preset was saved */
};

/* Local function definitions */

#endif //_U_VALS_H
