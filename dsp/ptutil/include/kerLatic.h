/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: KerLatic.h
 * DATE: 10/7/98
 * AUTHOR: Paul F. Titchener
 * DESCRIPTION: Lattice filter structures
 *
 */

/* 
 *  MACRO kerLatticeAllPass()
 *  DESCRIPTION: Lattice form allpass filter.
 */
#define kerLatticeAllPass(in, out, coeff, ptr, mem_start, mem_end)\
{\
	float D_in, D_out;\
	D_out = *(ptr);\
	D_in = in - coeff * D_out;\
	*(ptr) = D_in;\
	out = D_out + coeff * D_in;\
	(ptr)++;\
	if( ptr >= mem_end )\
		ptr = mem_start;\
}

/* 
 *  MACRO kerLatticeAllPassTapped()
 *  DESCRIPTION: Lattice form allpass filter with additional taps
 *  that access specified delayed taps of the internal delay line.
 */
#define kerLatticeAllPassTapped(in, out, coeff, tap1, tap1_out, tap2, tap2_out, ptr, mem_start, mem_end, mem_len)\
{\
	float Dl_in, Dl_out;\
	float *tmp_ptr;\
	Dl_out = *(ptr);\
	Dl_in = in - coeff * Dl_out;\
	*(ptr) = Dl_in;\
	out = Dl_out + coeff * Dl_in;\
	tmp_ptr = ptr - tap1;\
	if( tmp_ptr < mem_start)\
		tmp_ptr += mem_len;\
	tap1_out = *tmp_ptr;\
	tmp_ptr = ptr - tap2;\
	if( tmp_ptr < mem_start)\
		tmp_ptr += mem_len;\
	tap2_out = *tmp_ptr;\
	(ptr)++;\
	if( ptr >= mem_end )\
		ptr = mem_start;\
}

/* 
 *  MACRO kerLatticeDecayDiffuser()
 *  DESCRIPTION: Special lattice form reverb decay diffusion elememt,.
 *  Accepts real fractional delay amount (delay_real) and uses linear interpolation.
 *  see AES Vol.45 Number 9, pg. 662.
 */
#define kerLatticeDecayDiffuser(in, out, coeff, delay_real, ptr, mem_start, mem_end, mem_len)\
{\
	float dl_in, dl_out;\
	float *tmp_ptr;\
	realtype y1, y2;\
	long idly = (long)delay_real;\
	float del = delay_real - idly;\
	tmp_ptr = ptr - idly;\
	if( tmp_ptr < mem_start )\
		tmp_ptr += mem_len;\
	y1 = *(tmp_ptr);\
	idly++;\
	tmp_ptr = ptr - idly;\
	if( tmp_ptr < mem_start )\
		tmp_ptr += mem_len;\
	y2 = *(tmp_ptr);\
	dl_out = (y1 + (y2 - y1) * del);\
	dl_in = in + coeff * dl_out;\
	*(ptr) = dl_in;\
	out = dl_out - coeff * dl_in;\
	(ptr)++;\
	if( ptr >= mem_end )\
		ptr = mem_start;\
}

