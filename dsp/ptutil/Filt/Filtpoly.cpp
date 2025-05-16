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
#include <math.h>

#include "codedefs.h"
#include "mth.h"
#include "filt.h"

/*
 * FUNCTION: filtPolyCalc2ndOrderResponse()
 * DESCRIPTION:
 *   Calculates the frequency response of a 2nd order polynomial.
 *   Output returns in linear (not db) form.
 */
realtype PT_DECLSPEC filtPolyCalc2ndOrderResponse(realtype c0, realtype c1, realtype c2, realtype r_freq)
{   
	realtype response, omega, two_omega, real, imag;
	
	omega = (realtype)MTH_TWO_PI * r_freq;
	two_omega = (realtype)MTH_FOUR_PI * r_freq;
	real = c0 * (realtype)cos(two_omega) + c1 * (realtype)cos(omega) + c2;
	imag = c0 * (realtype)sin(two_omega) + c1 * (realtype)sin(omega);
	response = (realtype)sqrt( (realtype)(real*real + imag*imag) );	
	return(response);
}

/*
 * FUNCTION: filtPolyCalc2ndOrderPowerResponse()
 * DESCRIPTION:
 *   Calculates the frequency response of a 2nd order polynomial.
 *   Output returns in linear power (not db) form.
 */
realtype PT_DECLSPEC filtPolyCalc2ndOrderPowerResponse(realtype c0, realtype c1, realtype c2, realtype r_freq)
{   
	realtype response, omega, two_omega, real, imag;
	
	omega = (realtype)MTH_TWO_PI * r_freq;
	two_omega = (realtype)MTH_FOUR_PI * r_freq;
	real = c0 * (realtype)cos(two_omega) + c1 * (realtype)cos(omega) + c2;
	imag = c0 * (realtype)sin(two_omega) + c1 * (realtype)sin(omega);
	response = real*real + imag*imag;	
	return(response);
}

/*
 * FUNCTION: filtPolyCalcBiquadResponse()
 * DESCRIPTION:
 *   Calculates the frequency response of a biquad filter.
 *   Output returns in linear (not db) form.
 */
realtype PT_DECLSPEC filtPolyCalcBiquadResponse(realtype b0, realtype b1, realtype b2,
										   realtype a0, realtype a1, realtype a2, realtype r_freq)
{   
	realtype response;
	
	response = filtPolyCalc2ndOrderResponse(b0, b1, b2, r_freq)
			   / filtPolyCalc2ndOrderResponse(a0, a1, a2, r_freq);	
			   
	return(response);
}			

/*
 * FUNCTION: filtPolyCalcBiquadResponseFiltStruct()
 * DESCRIPTION:
 *   Calculates the frequency response of a biquad filter passed in as filt type.
 *   Output returns in linear (not db) form.
 */
realtype PT_DECLSPEC filtPolyCalcBiquadResponseFiltStruct(
									struct filt2ndOrderBoostCutShelfFilterType *f, realtype r_freq)
{   
	realtype response;
	
	response = filtPolyCalc2ndOrderResponse(f->b0, f->b1, f->b2, r_freq)
			   / filtPolyCalc2ndOrderResponse((realtype)1.0, f->a1, f->a2, r_freq);	
			   
	return(response);
}			

/*
 * FUNCTION: filtPolyCalcBiquadPowerResponse()
 * DESCRIPTION:
 *   Calculates the frequency response of a biquad filter.
 *   Output returns in linear power (not db) form.
 */
realtype PT_DECLSPEC filtPolyCalcBiquadPowerResponse(realtype b0, realtype b1, realtype b2,
										   realtype a0, realtype a1, realtype a2, realtype r_freq)
{   
	realtype response;
	
	response = filtPolyCalc2ndOrderPowerResponse(b0, b1, b2, r_freq)
			   / filtPolyCalc2ndOrderPowerResponse(a0, a1, a2, r_freq);	
			   
	return(response);
}			

/*
 * FUNCTION: filtPolyCalcBiquadPowerResponseFiltStruct()
 * DESCRIPTION:
 *   Calculates the frequency response of a biquad filter passed in as filt type.
 *   Output returns in linear power (not db) form.
 */
realtype PT_DECLSPEC filtPolyCalcBiquadPowerResponseFiltStruct(
									struct filt2ndOrderBoostCutShelfFilterType *f, realtype r_freq)
{   
	realtype response;
	
	response = filtPolyCalc2ndOrderPowerResponse(f->b0, f->b1, f->b2, r_freq)
			   / filtPolyCalc2ndOrderPowerResponse((realtype)1.0, f->a1, f->a2, r_freq);	
			   
	return(response);
}			



/*
 * FUNCTION: filtPolyCalc2ndOrderPowRespCosSupplied()
 * DESCRIPTION:
 *   Calculates the frequency response of a 2nd order polynomial.
 *   This version is for when cos and sine functions are precalculated.
 *   Output returns in linear power (not db) form.
 */
realtype PT_DECLSPEC filtPolyCalc2ndOrderPowRespCosSupplied(realtype c0, realtype c1, realtype c2, 
								    		 realtype cos_omega, realtype sin_omega,
								    		 realtype cos_2omega, realtype sin_2omega)
{   
	realtype response, real, imag;
	
/*  This version assumes c0*Z**2 + c1*Z + c2 */
	real = c0 * cos_2omega + c1 * cos_omega + c2;
	imag = c0 * sin_2omega + c1 * sin_omega;
	
/*  This version assumes c0 + c1*Z**-1 + c2*Z**-2 */
/* Doesn't seem to be any difference between these methods for plot anomalies */
/*
	real = c0 + c1 * cos_omega + c2 * cos_2omega;
	imag = c2 * sin_2omega + c1 * sin_omega;
*/
	
	response = real*real + imag*imag;	
	
	return(response);
}

/*
 * FUNCTION: filtPolyCalcBiquadPowRespCosSupplied()
 * DESCRIPTION:
 *   Calculates the frequency response of a biquad filter.
 *   Output returns in linear power (not db) form.
 */
realtype PT_DECLSPEC filtPolyCalcBiquadPowRespCosSupplied(realtype b0, realtype b1, realtype b2,
								    realtype a0, realtype a1, realtype a2, 
								    realtype cos_omega, realtype sin_omega,
								    realtype cos_2omega, realtype sin_2omega)
{   
	realtype numer, denom, response;
	
	numer = filtPolyCalc2ndOrderPowRespCosSupplied(b0, b1, b2, cos_omega, sin_omega, cos_2omega, sin_2omega);
	denom = filtPolyCalc2ndOrderPowRespCosSupplied(a0, a1, a2, cos_omega, sin_omega, cos_2omega, sin_2omega);
			   
	response = numer/denom;			   
			   
	return(response);
}			

#ifdef FILT_UNDEF
/*
 * NOTE - this was an effort to improve plot response problems
 * at low freqs. It was actually much worse, apparently due to
 * requiring the difference of cos(omega) and cos(2omega) to be accurate.
 *
 * FUNCTION: filtPolyCalcBiquadPowRespCosSupplied()
 * DESCRIPTION:
 *   Calculates the frequency response of a biquad filter.
 *   Output returns in linear power (not db) form.
 *   This version directly calculates the power. It assumes the polys
 *   have 1.0 a0,b0 coeffcients, and
 *   come in precalculated as  x0 = 1 + b1*b1 + b2*b2
 *							   x1 = 2*(b1 + b1*b2)
 *							   x2 = 2*b2
 *   and					   y0 = 1 + a1*a1 + a2*a2
 *							   y1 = 2*(a1 + a1*a2)
 *							   y2 = 2*a2
 */
realtype PT_DECLSPEC filtPolyCalcBiquadPowRespCosSuppliedMethod2(realtype x0, realtype x1, realtype x2,
								    realtype y0, realtype y1, realtype y2, 
								    realtype cos_omega, realtype cos_2omega)
{   
	realtype numer, denom, response;
	
    numer = (x0 + x1*cos_omega + x2*cos_2omega);
    denom = (y0 + y1*cos_omega + y2*cos_2omega);
    if(denom == (realtype)0.0)
    	denom = (realtype)1.0;
    if(numer == (realtype)0.0)
    	numer = (realtype)1.0;
    response = numer/denom;
    if(response == (realtype)0.0)
    	response = (realtype)1.0;
    
	return(response);
}			
#endif // FILT_UNDEF
