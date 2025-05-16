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
#ifndef _FILT_H_
#define _FILT_H_

#include "codedefs.h"

/* For precision of sos coeff calculations */
#define biqdRealtype realtype

struct filt2ndOrderButFilterType
{
	/* A special filter structure that contains 2nd order Linear Transform
	 * type coefficients and all internal states required for a 2 channel filter.
	 */
	realtype gain;
	realtype a1;
	realtype a0;
	realtype out1_minus1;
	realtype out1_minus2;
	realtype in1_minus1;
	realtype in1_minus2;
	realtype out2_minus1;
	realtype out2_minus2;
	realtype in2_minus1;
	realtype in2_minus2;
};

struct filt2ndOrderBoostCutShelfFilterType
{
	/* A special filter structure that contains 2nd order Linear Transform
	 * type coefficients and all internal states required for a 2 channel filter.
	 */
	/* Numerator coeffs */
	realtype b0;
	realtype b1;
	realtype b2;
	/* Denominator coeffs, a0 = 1.0 */
	realtype a1;
	realtype a2;
	realtype r_samp_freq;
	realtype r_center_freq;
	realtype boost;
	realtype Q;
	int section_on_flag;
};

#define FILT_LO_SHELF  0 
#define FILT_HI_SHELF  1
#define FILT_BOOST_CUT 2

/* filtbiqd.cpp */
int PT_DECLSPEC filtSosParametric(PT_HANDLE *, int, realtype, realtype, realtype, realtype);
int PT_DECLSPEC filtSosPtParametric(PT_HANDLE *, int, realtype, realtype, realtype, realtype);
int PT_DECLSPEC filtDoubleSosParametric(PT_HANDLE *, int, double, double, double, double);
int PT_DECLSPEC filtSosShelf(PT_HANDLE *, int, int, realtype, realtype, realtype);

/* filtpoly.cpp */
realtype PT_DECLSPEC filtPolyCalc2ndOrderResponse(realtype, realtype, realtype, realtype);
realtype PT_DECLSPEC filtPolyCalcBiquadResponse(realtype, realtype, realtype,
								    realtype, realtype, realtype, realtype);
realtype PT_DECLSPEC filtPolyCalcBiquadResponseFiltStruct(
									struct filt2ndOrderBoostCutShelfFilterType *f, realtype r_freq);
realtype PT_DECLSPEC filtPolyCalc2ndOrderPowerResponse(realtype, realtype, realtype, realtype);
realtype PT_DECLSPEC filtPolyCalcBiquadPowerResponse(realtype, realtype, realtype,
								    realtype, realtype, realtype, realtype);
realtype PT_DECLSPEC filtPolyCalcBiquadPowerResponseFiltStruct(
									struct filt2ndOrderBoostCutShelfFilterType *f, realtype r_freq);
realtype PT_DECLSPEC filtPolyCalc2ndOrderPowRespCosSupplied(realtype, realtype, realtype, 
												realtype, realtype, realtype, realtype);
realtype PT_DECLSPEC filtPolyCalcBiquadPowRespCosSupplied(realtype, realtype, realtype,
								    		  realtype, realtype, realtype,
								    		  realtype, realtype, realtype, realtype);

/* Fil12But.cpp */
void PT_DECLSPEC filtDesignSimple1rstLowPass(realtype r_omega, realtype *rp_a0);
void PT_DECLSPEC filtDesign1rstLowPass(realtype r_omega, realtype *rp_gain, realtype *rp_a0);
void PT_DECLSPEC filtDesign1rstHighPass(realtype r_omega, realtype *rp_gain, realtype *rp_a0);
void PT_DECLSPEC filtDesign2ndButLowPass(realtype r_omega, realtype *rp_gain, realtype *rp_a1, realtype *rp_a0);
void PT_DECLSPEC filtDesign2ndButHighPass(realtype r_omega, realtype *rp_gain, realtype *rp_a1, realtype *rp_a0);

/* FiltRun.cpp */
void PT_DECLSPEC filtRun1rstLowPass(realtype r_in, realtype *rp_in_old,
								realtype *rp_out, realtype *rp_out_old,
								realtype r_gain, realtype r_a0);
void PT_DECLSPEC filtRun2ndLowPass(realtype r_in, realtype *rp_in_minus1, realtype *rp_in_minus2,
							  realtype *rp_out, realtype *rp_out_minus1, realtype *rp_out_minus2,
							  realtype r_gain, realtype r_a1, realtype r_a0);
void PT_DECLSPEC filtRun1rstHighPass(realtype r_in, realtype *rp_in_old,
								realtype *rp_out, realtype *rp_out_old,
								realtype r_gain, realtype r_a0);
void PT_DECLSPEC filtRun2ndHighPass(realtype r_in, realtype *rp_in_minus1, realtype *rp_in_minus2,
							  realtype *rp_out, realtype *rp_out_minus1, realtype *rp_out_minus2,
							  realtype r_gain, realtype r_a1, realtype r_a0);

/* FiltCalcBiqd.cpp */
int filtCalcParametric(struct filt2ndOrderBoostCutShelfFilterType *f);
int filtCalcShelf(struct filt2ndOrderBoostCutShelfFilterType *f, int high_or_low);

/* FiltCalcFilterResponse.cpp */
double PT_DECLSPEC filtCalcFirResponse(double *dp_coeffs, int i_num_coeffs, double d_freq);

#endif
/* _FILT_H_ */
