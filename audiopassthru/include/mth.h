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
#ifndef _MTH_H_
#define _MTH_H_

/* Common constants */
#define MTH_PI 3.1415926536
#define MTH_TWO_PI 6.2831853072
#define MTH_FOUR_PI 12.5663706144
#define MTH_SQRT_TWO 1.4142136
#define MTH_SQRT_TWO_INV 0.70710678

/* Scaling factor for int to real conversion */
#define MTH_8_BIT_REAL_CONVERSION_FACTOR 128.0
#define MTH_16_BIT_REAL_CONVERSION_FACTOR	32768.0
#define MTH_20_BIT_REAL_CONVERSION_FACTOR	524288.0
#define MTH_24_BIT_REAL_CONVERSION_FACTOR 8388608.0
#define MTH_32_BIT_REAL_CONVERSION_FACTOR 2147483648.0

/* Standard values */
#define MTH_STANDARD_REAL_COMPARE_DELTA 0.00001

/* mthUtil.cpp */
int PT_DECLSPEC mthIntInRange(int, int, int, int, int *);
int PT_DECLSPEC mthSetIfClose(realtype *, realtype, realtype, int *);
int PT_DECLSPEC mthRealCheckEqual(realtype, realtype, realtype, int *);
int PT_DECLSPEC mthCalcCoarseFineRange(realtype, int, int, int, realtype *, realtype *);
int PT_DECLSPEC mthCalcFineFromCoarseRange(realtype, int, int, int, realtype *);
void PT_DECLSPEC mthCalcQuantDelta(realtype, realtype, int, realtype *);
realtype PT_DECLSPEC mthCalcRoundedValue(realtype, realtype, realtype, realtype);
realtype PT_DECLSPEC mthCalcClosestNiceValue(realtype r_input_value, realtype r_num_places);
int PT_DECLSPEC mthRotN(char *, int, int);
int PT_DECLSPEC mthRotN_Wide(wchar_t *, int, int);
int PT_DECLSPEC mthIsLong(char *, int *);
int PT_DECLSPEC mthIsLong_Wide(wchar_t *, int *);
int PT_DECLSPEC mthIsHex(char *, int *);
int PT_DECLSPEC mthIsHex_Wide(wchar_t *, int *);
int PT_DECLSPEC mthSearchForClosestValueInRealArray(realtype, realtype *, int, int *);
int PT_DECLSPEC mthRoundRealToInt(realtype, int *);
int PT_DECLSPEC mthCalcIterativeAverage(realtype, realtype, long, realtype *);
realtype PT_DECLSPEC mthFastSqrt(realtype r_x);

/* mthRand.cpp */
int PT_DECLSPEC mthGenRandomLongFromSeed(long init_rand, long *value);
int PT_DECLSPEC mthGenerateRandomInt(int i_max_val, int *ip_random_val);
int PT_DECLSPEC mthGenerateRandomUnsignedLong(unsigned long, unsigned long, unsigned long *);

/* mthfreq.cpp */
int PT_DECLSPEC mthMidiOctaveFreqs( realtype *, int );
int PT_DECLSPEC mthMidiOctaveFreqsPara( realtype *freqs, int num_pts );

/* mthcrypt.cpp */
int PT_DECLSPEC mthEncryptLong(unsigned long ul_key, long l_inval, long *lp_encrypted);
int PT_DECLSPEC mthDecryptLong(unsigned long ul_key, long l_inval, long *lp_decrypted);

/* mthStat.cpp */
int PT_DECLSPEC mthGenerateStatistics( realtype *rp_values, int i_num_vals, realtype *rp_average,
								   realtype *rp_variance, realtype *rp_normalized_variance);
/* mthTrig.cpp */
void PT_DECLSPEC mthCalcCosHarmonics(realtype r_cos_in, int i_num_harmonics, realtype *rp_harmonics);

/* mthBuffer.cpp */
int PT_DECLSPEC mthConvert16BitIntBufToRealtype(int, short int *, float *);
int PT_DECLSPEC mthConvertIntBufToRealtype(int, int, int, short int *, float *);
int PT_DECLSPEC mthConvertIntBufToRealtypeSurroundPreProcess(int, int, int, short int *, float *);
int PT_DECLSPEC mthConvertRealtypeBufTo16BitIntBuf(int, realtype *, short int *);
int PT_DECLSPEC mthConvertRealtypeBufTo16BitIntBufWithClipping(int, realtype *, short int *);
int PT_DECLSPEC mthConvertRealtypeBufToIntBuf(int, int, int, float *, short int *);
int PT_DECLSPEC mthConvertRealtypeBufToIntBufSurroundPostProcess(int, int, int, float *, short int *);

/* mthWarp.cpp */
int PT_DECLSPEC mthWarp(int *, int, realtype, int, int, int);
int PT_DECLSPEC mthPanStyleWarp(int *, int, int, int, int);

/* mthLinearRegression.cpp */
int PT_DECLSPEC mthLinearRegression(realtype *datax, realtype *datay, int num_points,
												realtype *rp_slope, realtype *rp_offset,
												realtype *rp_mean_sqr_error);
int PT_DECLSPEC mthLinearRegressionIntX(realtype *datay, int num_points,
													 realtype *rp_slope, realtype *rp_offset,
													 realtype *rp_mean_sqr_error);

/* mthRegion.cpp */
int PT_DECLSPEC mthCheckIfRegion1WithinRegion2(int, int, int, int, int, int, int, int, int *);



#endif /* _MTH_H_ */
