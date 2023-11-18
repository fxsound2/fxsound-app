/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: kercntl.h
 * DATE: 1/12/96
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: Parameter control related macros
 *
 */

/*
 * MACRO: kerParamFiltExp(ip_delay_in, r_old_delay, alpha, i_delay_out)
 * DESCRIPTION: Filters integer based control signals, primarily for
 * use in delay line length changes.
 * This version uses exponentially based one pole filtering.
 * This is also an integer version (should be added to name ).
 * NOTE- Due to roundoff, for arbitrary values of alpha, with input of 1.0,
 * output can converge to 0.9999xxx .  If alpha is chosen to be a number that
 * can be exactly represented (such as 1.0/8192.0 then output will converge to 1.0.
 */
#define kerParamFiltExp(ip_delay_in, r_old_delay, alpha, i_delay_out) \
{ \
	float r_tmp_MACRO; \
	r_tmp_MACRO = alpha * *(volatile long *)ip_delay_in \
	           + ((float)1.0 - alpha) * r_old_delay; \
    r_old_delay = r_tmp_MACRO; \
	i_delay_out = r_tmp_MACRO; \
}                           

/*  VERSION THAT USES STRAIGHT LINE INTERPOLATION - Required
 * Last line as shown to compile. Didn't sound so hot with delay */
#define kerParamFiltLinear(ip_delay_in, r_old_delay, i_delay_out) \
{ \
	long itmp_MACRO;\
	i_delay_out = r_old_delay;\
	itmp_MACRO = *(volatile long *)(ip_delay_in);\
	if( i_delay_out < itmp_MACRO ) i_delay_out += 1;\
	else if( i_delay_out > itmp_MACRO ) i_delay_out -= 1; r_old_delay = i_delay_out; }

/* Floating input value version */
#define kerFloatParamFiltExp(rp_param_in, r_old_param, alpha, r_param_out) \
{ \
	float r_tmp_MACRO; \
	r_tmp_MACRO = (float)alpha * *(volatile float *)rp_param_in \
	           + (float)((float)1.0 - (float)alpha) * r_old_param; \
	r_old_param = r_tmp_MACRO; \
	r_param_out = r_tmp_MACRO; \
}
/* Floating version with local storage */
#define kerFloatParamFiltExpLocal(rp_param_in, alpha, r_param_out, r_init_val) \
{ \
	float r_tmp_MACRO; \
	static float r_old_param_MACRO = r_init_val; \
	r_tmp_MACRO = alpha * *(volatile float *)rp_param_in \
	           + ((float)1.0 - alpha) * r_old_param_MACRO; \
	r_old_param_MACRO = r_tmp_MACRO; \
	r_param_out = r_tmp_MACRO; \
}
