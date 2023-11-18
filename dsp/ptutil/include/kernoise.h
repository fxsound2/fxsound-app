/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: KerNoise.h
 * DATE: 9/16/98
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: Noise Generation Macros
 *
 */

/* For setting quantization levels */
#define KERNOISE_QUANTIZE_8	0
#define KERNOISE_QUANTIZE_12	1
#define KERNOISE_QUANTIZE_16	2
#define KERNOISE_QUANTIZE_20	3
#define KERNOISE_QUANTIZE_24	4

/* These defines are used to do the quantization */
#define KERNOISE_PEAK_LEVEL_8  128
#define KERNOISE_PEAK_LEVEL_12 2048
#define KERNOISE_PEAK_LEVEL_16 32768
#define KERNOISE_PEAK_LEVEL_20 524288
#define KERNOISE_PEAK_LEVEL_24 8388608

/* These defines are used to set a peak to peak amplitude
 * equal to +/- 1/2 LSB for each bit precision, assuming 2's complement.
 */
#define KERNOISE_HALF_LSB_PEAK_LEVEL_8  1.0/256.0
#define KERNOISE_HALF_LSB_PEAK_LEVEL_12 1.0/4096.0
#define KERNOISE_HALF_LSB_PEAK_LEVEL_16 1.0/65536.0
#define KERNOISE_HALF_LSB_PEAK_LEVEL_20 1.0/1048576.0
#define KERNOISE_HALF_LSB_PEAK_LEVEL_24 1.0/16777216.0

/* To set various dither types */
#define KERNOISE_DITHER_NONE			0
#define KERNOISE_DITHER_UNIFORM		1
#define KERNOISE_DITHER_TRIANGULAR	2
#define KERNOISE_DITHER_SHAPED		3

/* 
 *  MACRO kerUniformWhiteNoise()
 *  DESCRIPTION: Generates uniform distribution white random noise with
 *  a level of +/- r_PeakVal.
 */
#define kerUniformWhiteNoise(ul_seed, r_PeakVal, r_output)\
{\
	ul_seed = (3141592621L * ul_seed + 2718282829L) % 4294967291L;\
	r_output = (realtype)*(long *)&ul_seed * ((realtype)r_PeakVal * (realtype)1.0/(realtype)2147483648L);\
}

/* 
 *  MACRO kerTriangularWhiteNoise()
 *  DESCRIPTION: Generates triangular distribution white random noise 
 *  with a level of +/- r_PeakVal.
 */
#define kerTriangularWhiteNoise(ul_seed, r_PeakVal, r_output)\
{\
	ul_seed = (3141592621L * ul_seed + 2718282829L) % 4294967291L;\
	r_output = (realtype)*(long *)&ul_seed * ((realtype)r_PeakVal * (realtype)0.5/(realtype)2147483648L);\
	ul_seed = (3141592621L * ul_seed + 2718282829L) % 4294967291L;\
	r_output += (realtype)*(long *)&ul_seed * ((realtype)r_PeakVal * (realtype)0.5/(realtype)2147483648L);\
}

/* 
 *  MACRO kerQuantize()
 *  DESCRIPTION: Quantizes to the chosen peak level value.
 */
#define kerQuantize(r_val, r_peak_level)\
{\
	r_val = r_val * (realtype)r_peak_level;\
	if( r_val >= (realtype)0.0 )\
		r_val = (realtype)((long)(r_val + (realtype)0.5));\
	else\
		r_val = (realtype)((long)(r_val - (realtype)0.5));\
	r_val *= (realtype)((realtype)1.0/(realtype)r_peak_level);\
}

/* 
 *  MACRO kerQuantizeWithError()
 *  DESCRIPTION: Quantizes to the chosen peak level value, passes back error,
 *   normalized between =/- 0.5 .
 */
#define kerQuantizeWithError(r_val, r_peak_level, r_error)\
{\
	r_error = r_val * (realtype)r_peak_level;\
	if( r_error >= (realtype)0.0 )\
		r_val = (long)(r_error + (realtype)0.5);\
	else\
		r_val = (long)(r_error - (realtype)0.5);\
	r_error -= r_val;\
	r_val *= (realtype)((realtype)1.0/(realtype)r_peak_level);\
}

/* 
 *  MACRO kerQuantizeAndDither()
 *  DESCRIPTION: Quantizes to the chosen peak level value, adds
 *  passed in dither, typically +/- 0.5 or =/- 1.0 in level.
 */
#define kerQuantizeAndDither(r_val, r_peak_level, r_dither)\
{\
	r_val = r_val * (realtype)r_peak_level + r_dither;\
	if( r_val >= (realtype)0.0 )\
		r_val = (realtype)((long)(r_val + (realtype)0.5));\
	else\
		r_val = (realtype)((long)(r_val - (realtype)0.5));\
	r_val *= (realtype)((realtype)1.0/(realtype)r_peak_level);\
}
