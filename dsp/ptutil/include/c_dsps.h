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
#ifndef _C_DSPS_H_
#define _C_DSPS_H_

/* To turn on debug message boxes */
/* #define DSPS_MESSAGE_BOXES */

/* For conditional compilation of DSP processing routines */
#if defined(DSPSOFT_TARGET)
	/* Software plug-in case */
	#if defined(DSP_DLL_EXPORT)
	#define DSP_FUNC_DEF __declspec( dllexport )
	#elif defined(DSP_DLL_IMPORT)
	#define DSP_FUNC_DEF __declspec( dllimport )
	#else
	#define DSP_FUNC_DEF
	#endif
#else
#if defined(DSP_TARGET)
	/* Hardware card case */
	#define DSP_FUNC_DEF static inline /* Eventually make static inline */
	#else
	#define DSP_FUNC_DEF
	#endif
#endif

/* For setting operation of Dsp Algorithm init function */
#define DSPS_INIT_MEMORY		1 /* Sets up memory addresses */
#define DSPS_INIT_PARAMS		2 /* Sets up and initializes parameter memory and values */
#define DSPS_RE_INIT_MEMORY	4 /* Currently only used in Lex reverb, not a memory clearing operation */
#define DSPS_ZERO_MEMORY		8 /* Zero's signal memory to avoid clicks on restarts */

/* Max number of algorithm parameters */
#define DSPS_MAX_NUM_PARAMS 128

/* Max number of dsp functions. Note with apitch,
 * compressor and exciter, currently using 134 functions.
 */
#define DSPS_MAX_NUM_PROC_FUNCTIONS 144

/* For software dsp algorithms, size of array used to save state */
/* Currently, peq uses 4 * (DSP_MAX_NUM_OF_PEQ_ELEMENTS + 2) 
 * which is 40
 */
#define DSPS_NUM_STATE_VARS 64

/* NOTE - these memory sizes originally assumed a max 48k sampling rate.
 * To support 96k and higher, the sizes have been doubled or converted to
 * use the DSPS_SOFT_MAX_SAMP_FREQ define.
 */
/* Definitions related to software versions of DSP effects */
/* Software DSP processor memory size in longs */
#define DSPS_SOFT_MAX_SAMP_FREQ 96000.00
#define DSPS_SOFT_MEM_DELAY_LENGTH (131072 * 2) /* 128k, same as new card */
#define DSPS_SOFT_MEM_SMALL_LENGTH (32768 * 2)
#define DSPS_SOFT_MEM_REVERB_LENGTH (32768 * 2)
#define DSPS_SOFT_MEM_CHORUS_LENGTH (32768 * 2)
#define DSPS_SOFT_MEM_FLANGE_LENGTH (32768 * 2)
#define DSPS_SOFT_MEM_PITCH_LENGTH (32768 * 2)
#define DSPS_SOFT_MEM_PEQ_LENGTH 0
#define DSPS_SOFT_MEM_TREMOLO_LENGTH (32768 * 2)
#define DSPS_SOFT_MEM_PAN_LENGTH (32768 * 2)
#define DSPS_SOFT_MEM_AUTO_PITCH_LENGTH (32768 * 2)
#define DSPS_SOFT_MEM_MULTI_COMP_LENGTH (32768 * 2)
#define DSPS_SOFT_MEM_AURAL_ENHANCER_LENGTH 0
#define DSPS_SOFT_MEM_MAXIMIZER_LENGTH 0
#define DSPS_SOFT_MEM_APIT_LENGTH (32768 * 2)
#define DSPS_SOFT_MEM_COMP_LENGTH 0

/* 25 ms * 48000 kHz * 3 delay lines */
#define DSPS_SOFT_MEM_WIDE_LENGTH  (int32_t)((25.1/1000.0) * 3.0 * DSPS_SOFT_MAX_SAMP_FREQ)

/* Note length includes room for oscillator tables */
/* Currently roomsize scales up by 1.5 factor. Nominal memory is 759.805 ms */
/* Note extra repeated endpoint for oscillator */
#define DSPS_SOFT_MEM_LEX_LENGTH (int32_t)(2 * (8192+1) + 1.5 * (860 * 0.001 * DSPS_SOFT_MAX_SAMP_FREQ) )

/* Use smaller memory space for DMX version */
#ifndef PT_DMX_BUILD
#define DSPS_SOFT_MEM_PLAY_LENGTH (int32_t)(DSPS_SOFT_MEM_AURAL_ENHANCER_LENGTH + DSPS_SOFT_MEM_MAXIMIZER_LENGTH + DSPS_SOFT_MEM_LEX_LENGTH + DSPS_SOFT_MEM_WIDE_LENGTH + DSPS_SOFT_MEM_DELAY_LENGTH)
#else
#define DSPS_SOFT_MEM_PLAY_LENGTH (int32_t)(DSPS_SOFT_MEM_AURAL_ENHANCER_LENGTH + DSPS_SOFT_MEM_MAXIMIZER_LENGTH)
#endif

#define DSP_WID_MAX_DELAY_LEN

/* For prototype lexicon reverb, currently pseudo-stereo */
#define DSPS_SOFT_MEM_PROTO1_LENGTH (int32_t)((756 + 100 + 1) * 2.0 * 0.001 * DSPS_SOFT_MAX_SAMP_FREQ)

/* DSP processing function declarations */

/* Special function with zero output (no init needed) */
DSP_FUNC_DEF void dspsZeroOutput(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);

/* DlyX.c */
DSP_FUNC_DEF int dspsDly1Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsDly1Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsDly1Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);

DSP_FUNC_DEF int dspsDly2Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsDly2Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsDly2Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsDly3Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsDly3Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsDly3Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsDly4Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsDly4Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsDly4Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsDly5Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsDly5Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsDly5Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsDly6Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsDly6Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsDly6Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsDly7Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsDly7Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsDly7Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsDly8Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsDly8Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsDly8Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
/* flangX.c */
DSP_FUNC_DEF int dspsFlang1Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsFlang1Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsFlang1Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsFlang2Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsFlang2Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsFlang2Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
/* chorX.c */
DSP_FUNC_DEF int dspsChor1Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsChor1Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsChor1Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsChor2Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsChor2Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsChor2Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsChor3Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsChor3Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsChor3Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsChor4Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsChor4Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsChor4Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
/* pitchX.c */
DSP_FUNC_DEF int dspsPitch1Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPitch1Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPitch1Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsPitch2Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPitch2Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPitch2Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
/* peqX.c */
DSP_FUNC_DEF int dspsPeq1Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPeq1Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPeq1Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsPeq2Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPeq2Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPeq2Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsPeq3Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPeq3Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPeq3Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsPeq4Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPeq4Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPeq4Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsPeq5Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPeq5Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPeq5Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsPeq6Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPeq6Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPeq6Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsPeq7Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPeq7Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPeq7Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsPeq8Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPeq8Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPeq8Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
/* trmX.c */
DSP_FUNC_DEF int dspsTrm1Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsTrm1Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsTrm1Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsTrm2Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsTrm2Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsTrm2Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
/* panX.c */
DSP_FUNC_DEF int dspsPan1Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPan1Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPan1Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsPan2Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPan2Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPan2Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
/* aural.c */
DSP_FUNC_DEF int dspsAuralInit(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsAuralProcess(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsAuralProcess32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);

/* max.c */
DSP_FUNC_DEF int dspsMaximizerInit(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsMaximizerProcess(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsMaximizerProcess32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);

/* lex.c */
DSP_FUNC_DEF int dspsLexReverbInit(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsLexReverbProcess(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsLexReverbProcess32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);

/* apit.c */
DSP_FUNC_DEF int dspsApitInit(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsApitProcess(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsApitProcess32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);

/* Play.c */
DSP_FUNC_DEF int dspsPlayInit(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsPlayProcess(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsPlayProcess32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);

/* Comp.c */
DSP_FUNC_DEF int dspsCompInit(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsCompProcess(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsCompProcess32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);

/* Wide.c */
DSP_FUNC_DEF int dspsWideInit(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsWideProcess(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsWideProcess32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);

/* proto1.c */
DSP_FUNC_DEF int dspsProto1Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsProto1Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsProto1Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);

/* reverb10X.c */
DSP_FUNC_DEF int dspsR1S44R20Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R20Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R20Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R25Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R25Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R25Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R30Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R30Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R30Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R35Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R35Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R35Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R40Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R40Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R40Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R45Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R45Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R45Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R50Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R50Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R50Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R55Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R55Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R55Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R60Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R60Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R60Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R65Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R65Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R65Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R70Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R70Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R70Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R75Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R75Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R75Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R80Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R80Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R80Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R85Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R85Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R85Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R90Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R90Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R90Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R95Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R95Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R95Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S44R00Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S44R00Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S44R00Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R20Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R20Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R20Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R25Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R25Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R25Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R30Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R30Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R30Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R35Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R35Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R35Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R40Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R40Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R40Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R45Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R45Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R45Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R50Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R50Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R50Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R55Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R55Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R55Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R60Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R60Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R60Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R65Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R65Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R65Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R70Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R70Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R70Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R75Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R75Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R75Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R80Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R80Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R80Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R85Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R85Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R85Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R90Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R90Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R90Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R95Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R95Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R95Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
DSP_FUNC_DEF int dspsR1S48R00Init(float *, float *, int32_t, float *, int, float);
DSP_FUNC_DEF void dspsR1S48R00Process(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
DSP_FUNC_DEF void dspsR1S48R00Process32(int32_t *, int, float *, float *, float *, struct hardwareMeterValType *, int);
								   
#endif /* _C_DSPS_H */
