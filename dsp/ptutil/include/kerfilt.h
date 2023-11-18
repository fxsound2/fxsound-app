/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: kerfilt.h
 * DATE: 1/31/97
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: Filtering related macros
 *
 */
 
/* 
 * MACRO kerSosFiltDirectForm1()
 * DESCRIPTION: Performs 2nd order section filter. Assumes that coeffs comes in
 *				pointing to the memory location of the a1 denominator coeff, 
 *				a coeffs come in with negative values,
 *				and the coeffs are ordered -a1, -a2, b0, b1, b2, .
 *				a0 is assumed to be 1.0 .
 *				coeffs comes out pointing to the first coeff of the next section.
 *				An additional possible optimization would be to assume that out
 *				still contains the old output val when coming into the macro.
 *
 * This version keeps output val around internally. It also assumes
 * both b0 and a0 are 1.0
 * NOTE- DIRECT FORM 1 IS NOT RECOMMENDED.
 */
#define kerSosFiltDirectForm1(in, coeffs, out)\
{\
	static float b1_val_MACRO = 0.0;\
	static float b2_val_MACRO = 0.0;\
	static float a1_val_MACRO = 0.0;\
	static float a2_val_MACRO = 0.0;\
	out = in;\
	out += *(volatile float *)(coeffs++) * a1_val_MACRO;\
	out += *(volatile float *)(coeffs++) * a2_val_MACRO;\
	out += *(volatile float *)(coeffs++) * b1_val_MACRO;\
	out += *(volatile float *)(coeffs++) * b2_val_MACRO;\
	b2_val_MACRO = b1_val_MACRO;\
	b1_val_MACRO = in;\
	a2_val_MACRO = a1_val_MACRO;\
	a1_val_MACRO = out;\
}

/* This general purpose Transformed Direct2 version allows non-unity b0,
 * and makes no assumptions about other coeff relationships.
 * coeffs must be ordered b0, b1, -a1, b2, -a2.
 * NOTE- Original DSP only version had multiple coeffs+ operators
 * on each line, which were incremented left to right by the
 * DSP compiler. PC compiler incremented these right to left,
 * so they were broken out into individual lines.
 * Note that there is a slight possibility this could change the
 * DSP code performance.
 */
#define kerSosFiltDirectForm2Trans(in, coeffs, out)\
{\
	static float w1 = 0.0;\
	static float w2 = 0.0;\
	out = w1 + *(volatile float *)(coeffs++) * in;\
	w1 = *(volatile float *)(coeffs++) * in;\
	w1 += *(volatile float *)(coeffs++) * out + w2;\
	w2 = *(volatile float *)(coeffs++) * in;\
	w2 += *(volatile float *)(coeffs++) * out;\
}

/* This is a version for the PC that uses external state */
#define kerSosFiltDirectForm2TransExtState(in, coeffs, state, out)\
{\
	out = *state + *(volatile float *)(coeffs++) * in;\
	*state = *(volatile float *)(coeffs++) * in;\
	*state += *(volatile float *)(coeffs++) * out + *(state + 1);\
	state++;\
	*state = *(volatile float *)(coeffs++) * in;\
	*state += *(volatile float *)(coeffs++) * out;\
	state++;\
}

/* This special purpose Transformed Direct2 version allows non-unity b0, but
 * assumes b1=a1, as in parametric boost/cut filters.
 * coeffs must be ordered b0, b1, b2, -a2.
 */
#define kerSosFiltDirectForm2TransPara(in, coeffs, out)\
{\
	static float w1 = 0.0;\
	static float w2 = 0.0;\
	out = w1 + *(volatile float *)(coeffs++) * in;\
	w1 = (in - out) * *(volatile float *)(coeffs++) + w2;\
	w2 = *(volatile float *)(coeffs++) * in;\
	w2 += *(volatile float *)(coeffs++) * out;\
}

/* This is a version for the PC that uses external state */
#define kerSosFiltDirectForm2TransParaExtState(in, coeffs, state, out)\
{\
	out = *state + *(volatile float *)(coeffs++) * in;\
	*state = (in - out) * *(volatile float *)(coeffs++) + *(state + 1);\
	state++;\
	*state = *(volatile float *)(coeffs++) * in;\
	*state += *(volatile float *)(coeffs++) * out;\
	state++;\
}

/* This special purpose Direct2 version allows non-unity b0, but
 * assumes b1=a1, as in parametric boost/cut filters.
 * coeffs must be ordered -a1, -a2, b0, b2.
 * The direct2 form doesn't seem as ez to code as the transposed D2 form.
 */
#define kerSosFiltDirectForm2Para(in, coeffs, out)\
{\
	static float w1 = 0.0;\
	static float w2 = 0.0;\
	float tmp, w0;\
	tmp = *(volatile float *)(coeffs++) * w1;\
	w0 =  tmp + *(volatile float *)(coeffs++) * w2 + in;\
	out = *(volatile float *)(coeffs++) * w0;\
	out -= tmp + *(volatile float *)(coeffs++) * w2;\
	w2 = w1;\
	w1 = w0;\
}
