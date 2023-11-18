/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: comSftwrCOP.h
 * DATE: 11/4/02
 * AUTHOR: Paul Titchener
 * DESCRIPTION: Contains comsftwr functions that can be called as CPP functions
 *
 */
#ifndef _COMSFTWRCPP_H_
#define _COMSFTWRCPP_H_

/* Next lines set whether functions are declared for static libs or dlls */
#if defined(COMSFTWR_DLL_EXPORT) /* Defined for compiling DLL's (versus static lib) */
	#if defined(WIN32)
	#define COMSFTWR_DECL __declspec( dllexport ) /* 32 bit dll version */
	#else
	#define COMSFTWR_DECL _export /* 16 bit dll version */
	#endif
#elif defined(SFTWR_DLL_IMPORT) /* For compiling apps that link to dll */
	#if defined(WIN32)
	#define COMSFTWR_DECL __declspec( dllimport ) /* 32 bit dll version */
	#else
	#define COMSFTWR_DECL
	#endif
#else /* Library (non-dll) case */
	#define COMSFTWR_DECL
#endif
 
/* Constant Defines */ 
/* Functions */ 
/* comSftwrCPP.cpp */
int COMSFTWR_DECL comSftwrInitCPP(PT_HANDLE **hpp_comSftwr);
int COMSFTWR_DECL comSftwrInitDspAlgorithmCPP(PT_HANDLE *hp_comSftwr, realtype r_sampling_freq, int i_init_flag);
int COMSFTWR_DECL comSftwrZeroDspMemoryCPP(PT_HANDLE *hp_comSftwr);
int COMSFTWR_DECL comSftwrAllocDspMemCPP(PT_HANDLE *hp_comSftwr);
int COMSFTWR_DECL comSftwrFreeUp(PT_HANDLE **hpp_comSftwr);

#endif /* _COMSFTWR_H_ */
