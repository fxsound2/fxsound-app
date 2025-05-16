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
#include <float.h>

#include "codedefs.h"
#include "filt.h"

/* Floating absolute-value macro */
biqdRealtype flx_flatmp;
#define flabs(arg)  (((flx_flatmp = (arg)) >= 0) ? flx_flatmp : -flx_flatmp)

#define PI 3.141592653589793238462643
#define ROOT2O2 0.7071067811965475244
#define SPN 1.65436e-24 	/* Smallest positive number ORIGINAL SETTING */
/* #define SPN FLT_MIN */	/* Smallest positive number- from float.h */

/* Local defines */
biqdRealtype filtBW2ANGLE(biqdRealtype a, biqdRealtype bandwidth);

/* ------------------------------------------------------------------------
	filtBW2ANGLE - Given bilinear transform parameter and desired bandwidth
	(as normalized frequency), computes bandedge, e, of filter
	as if it were centered at the frequency .25 (or pi/2 or srate/4).
	The bandwidth would then be 2*(.25-e). e is guaranteed to be between
	0 and .25 .

	To state it differently, given a filter centered on .25 with low
	bandedge e and high bandedge .5-e, the bilinear transform by a
	will produce a new filter with bandwidth (ie, difference between the
	high bandedge frequency and low bandedge frequency) of bandwidth.
	
	NOTE - At low Q settings (example, Q=.36, samp_f = 48k, boost=15,
	       center_freq=8869.84) this function acts wrong. It looks like
	       when bandwith >= 0.5, the function acts incorrectly. The tmp
	       val goes neg, and theta stops getting set to tmp. Doesn't
	       seem to be a precision problem.
	       ADDED A HACK to approximate a solution for this case.
------------------------------------------------------------------------ */
biqdRealtype filtBW2ANGLE(biqdRealtype a, biqdRealtype bandwidth)
{	
	biqdRealtype T, d, sn, cs, mag, delta, theta, tmp, a2, a4, asnd;
	
	T = (biqdRealtype)tan((biqdRealtype)(2*PI)*bandwidth);
	a2 = a*a;
	a4 = a2 * a2;
	d = 2*a2*T;
	sn = (1+a4)*T;
	cs = (1-a4);
	mag = (biqdRealtype)sqrt(sn*sn + cs*cs);
	d /= mag;
	delta = (biqdRealtype)atan2(sn, cs);

	asnd = (biqdRealtype)asin(d);
	
	/* Note- fix hi freq low Q problems */
	theta = (biqdRealtype)0.5 * ((biqdRealtype)PI - asnd - delta);	/* Bandedge for prototype */
	tmp = (biqdRealtype)0.5 * (asnd - delta);			/* Take principal branch */
	if (tmp > (biqdRealtype)0.0 && tmp < theta) 
		theta = tmp;
		
    /* Max out returned theta for this case - hi freq, hi Q */
    if ( (bandwidth >= (biqdRealtype)0.5) )		
    	theta = (biqdRealtype)0.005;
    	
	return(theta/(biqdRealtype)(2*PI));				/* Return norm frequency */
}

/* ------------------------------------------------------------------------
	filtCalcParametric - Design straightforward 2nd-order presence filter, and
	load into specified second order section.
	Must be given (normalized) center frequency, Q, and
	boost/cut in db.  Frequencies are normalized to s_freq = 1.
	
	Note that internal coeffs mean:

                         -1      -2
	         b0 + b1 Z  + b2 Z
	T(Z) = ---------------------
                         -1      -2
	         1 + a1 Z  + a2 Z

	Modified from original version to be smart at 0 boost and to send
	back normalized sections of the form:

	Note that internal coeffs mean:

                         -1      -2
	              1 + b1 Z  + b2 Z
	T(Z) = g * ---------------------
                         -1      -2
	             1 + a1 Z  + a2 Z
------------------------------------------------------------------------ */
int filtCalcParametric(struct filt2ndOrderBoostCutShelfFilterType *f)
{
  biqdRealtype bandwidth;
  biqdRealtype a, A, F, xfmbw, C, tmp, alphan, alphad, a0, recipa0, asq, F2, a2plus1, ma2plus1;
  biqdRealtype center_freq;

  /* Note that for 0.0 boost, design routine sends back non-zero b1,b2,a1,a2 coeffs that cause
   * numerator and denominator to cancel. Overridding these with 1.0, 0, 0 ...doesn't
   * work because it causes boinks. If cancelling coeffs are sent as is, hi Q settings
   * cause problems at low freqs/mid freqs. (as high as 440) because of precision
   * limitations. To minimize the boinks and precision probs., we warp Q settings with
   * the boost below 3db to at 0 boost we have a Q of 0.2.
   */
#ifdef FILT_COEFF_ZERO   
  if( boost == 0.0 )
  {
  	 *b0 = 1.0;
  	 f->b1 = f->b2 = f->a1 = f->a2 = 0.0;
  }
  else
#endif //FILT_COEFF_ZERO  

  /* If boost is zero, set the section off */
  if( f->boost == 0.0 )
  {
    f->section_on_flag = IS_FALSE;
  	 f->b0 = 1.0;
  	 f->b1 = f->b2 = f->a1 = f->a2 = 0.0;
  }
  else /* Section on */
  {
   /* For now, because of low frequency accuracy problems, limit Q at low freqs.
    * Limit to Q of 1 at 20 hz, starting taper at FILT_Q_LIMIT_FREQ hz
    */
    realtype maxQ, abs_boost;     
     
	/* Local definitions (used in this function only) */
	#define FILT_Q_UPPER_LIMIT_FREQ (realtype)60.0     
	#define FILT_Q_LOWER_LIMIT_FREQ (realtype)20.0     
	#define FILT_Q_UPPER_LIMIT (realtype)20.0     
	#define FILT_Q_LOWER_LIMIT (realtype)1.0     
	#define FILT_Q_LIMIT_SCALE (FILT_Q_UPPER_LIMIT - FILT_Q_LOWER_LIMIT)/(FILT_Q_UPPER_LIMIT_FREQ - FILT_Q_LOWER_LIMIT_FREQ)
	
	#define FILT_BOOST_WARP_LEVEL (realtype)6.0     
	#define FILT_BOOST_MAX_Q (realtype)20.0     
	#define FILT_BOOST_MIN_Q (realtype)0.2     
	#define FILT_BOOST_SCALE (FILT_BOOST_MAX_Q - FILT_BOOST_MIN_Q)/(FILT_BOOST_WARP_LEVEL)

    f->section_on_flag = IS_TRUE;

	/* Limit low freq Q */
    if( f->r_center_freq < FILT_Q_UPPER_LIMIT_FREQ )
    {
    	maxQ = (f->r_center_freq - FILT_Q_LOWER_LIMIT_FREQ) * FILT_Q_LIMIT_SCALE
    		   + FILT_Q_LOWER_LIMIT;
        if( f->Q > maxQ )
        	f->Q = maxQ;
    }
    			
	/* Limit low boost Q */
    abs_boost = (realtype)fabs(f->boost);
    if( abs_boost < FILT_BOOST_WARP_LEVEL )
    {
    	maxQ = abs_boost * FILT_BOOST_SCALE
    		   + FILT_BOOST_MIN_Q;
        if( f->Q > maxQ )
        	f->Q = maxQ;
    }

    center_freq = f->r_center_freq/f->r_samp_freq;
    bandwidth = center_freq/f->Q;

	a = (biqdRealtype)tan( (biqdRealtype)PI*(center_freq - (biqdRealtype)0.25) );		/* Warp factor */
	asq = a*a;
	A = (biqdRealtype)pow((biqdRealtype)10.0, (biqdRealtype)f->boost/(biqdRealtype)20.0);		/* Cvrt dB to factor */
	if (f->boost < (realtype)6.0 && f->boost > (realtype)-6.0) 
		F = (biqdRealtype)sqrt(A);
	else if (A > (biqdRealtype)1.0) 
		F = A/(biqdRealtype)sqrt(2.0);
	else F = A * (biqdRealtype)sqrt(2.0);
		/* If |boost/cut| < 6dB, then doesn't make sense to use 3dB pt.
		   use of root makes bandedge at half the boost/cut amount
		 */
	xfmbw = filtBW2ANGLE(a, bandwidth);

	C = (biqdRealtype)1.0 / (biqdRealtype)tan((biqdRealtype)(2*PI)*xfmbw);	/* co-tangent of angle */
	F2 = F*F;
	tmp = A*A - F2;
	if (fabs(tmp) <= SPN)
		alphad = C;
	else 
		alphad = (biqdRealtype)sqrt( C*C * (F2 - (biqdRealtype)1.0) / tmp);
	alphan = A*alphad;

	a2plus1 = (biqdRealtype)1.0 + asq;
	ma2plus1 = (biqdRealtype)1.0 - asq;
	f->b0 = (realtype)(a2plus1 + alphan*ma2plus1);
	f->b1 = (realtype)((biqdRealtype)4.0*a);
	f->b2 = (realtype)(a2plus1 - alphan*ma2plus1);

	a0 = (realtype)(a2plus1 + alphad*ma2plus1);
	f->a2 = (realtype)(a2plus1 - alphad*ma2plus1);

	/* Normalize a0 */
	recipa0 = (biqdRealtype)1.0/(biqdRealtype)a0;
	f->b0 = (realtype)((biqdRealtype)f->b0 * recipa0);
	f->b1 = (realtype)((biqdRealtype)f->b1 * recipa0);
	f->b2 = (realtype)((biqdRealtype)f->b2 * recipa0);
	f->a1 = f->b1;
	f->a2 = (realtype)((biqdRealtype)f->a2 * recipa0);
  }
		
  return(OKAY);
}

/* ------------------------------------------------------------------------
	filtCalcShelf - Design straightforward 2nd-order shelving filter, and
	load into specified second order section.
	Must be given (normalized) center (corner) frequency normalized to s_freq = 1.

	Note that internal coeffs mean:

                         -1      -2
	         b0 + b1 Z  + b2 Z
	T(Z) = ---------------------
                         -1      -2
	         1 + a1 Z  + a2 Z

	Modified from original version to be smart at 0 boost and to send
	back normalized sections of the form:

	Note that internal coeffs mean:

                         -1      -2
	              1 + b1 Z  + b2 Z
	T(Z) = g * ---------------------
                         -1      -2
	             1 + a1 Z  + a2 Z
------------------------------------------------------------------------ */
int filtCalcShelf(struct filt2ndOrderBoostCutShelfFilterType *f, 
								 int high_or_low)
{	

  biqdRealtype a, A, F, tmp, a0, recipa0, asq, F2, gamma2, siggam2, gam2p1;
  biqdRealtype gamman, gammad, ta0, ta1, ta2, tb0, tb1, tb2, aa1, ab1;
  biqdRealtype center_freq;

#ifdef FILT_COEFF_ZERO   
  if( boost == 0.0 )
  {
    f->section_on_flag = IS_FALSE;
  	 f->b0 = 1.0;
  	 f->b1 = f->b2 = f->a1 = f->a2 = 0.0;
  }
  else
#endif //FILT_COEFF_ZERO  

  /* If boost is zero, set the section off */
  if( f->boost == 0.0 )
  {
    f->section_on_flag = IS_FALSE;
  	 f->b0 = 1.0;
  	 f->b1 = f->b2 = f->a1 = f->a2 = 0.0;
  }
  else /* Section on */
  {
    f->section_on_flag = IS_TRUE;

	center_freq = f->r_center_freq/f->r_samp_freq;

	a = (biqdRealtype)tan( (biqdRealtype)PI*(center_freq - (biqdRealtype)0.25) );		/* Warp factor */
	asq = a*a;
	A = (biqdRealtype)pow((biqdRealtype)10.0, f->boost/(biqdRealtype)20.0);		/* Cvrt dB to factor */
	if (f->boost < (realtype)6.0 && f->boost > (realtype)-6.0) F = (biqdRealtype)sqrt(A);
	else if (A > (biqdRealtype)1.0) F = A/(biqdRealtype)sqrt((biqdRealtype)2.0);
	else F = A * (biqdRealtype)sqrt((biqdRealtype)2.0);
		/* If |boost/cut| < 6dB, then doesn't make sense to use 3dB pt.
		   use of root makes bandedge at half the boost/cut amount
		 */

	F2 = F*F;
	tmp = A*A - F2;
	if (fabs(tmp) <= SPN) 
		gammad = (biqdRealtype)1.0;
	else 
		gammad = (biqdRealtype)pow( (F2 - (biqdRealtype)1.0)/tmp, (biqdRealtype)0.25);  /* Fourth root */
	gamman = (biqdRealtype)sqrt(A)*gammad;

	/* Once for the numerator */
	gamma2 = gamman*gamman;
	gam2p1 = (biqdRealtype)1.0 + gamma2;
	siggam2 = (biqdRealtype)(2.0*ROOT2O2)*gamman;

	ta0 = gam2p1 + siggam2;
	ta1 = (biqdRealtype)-2.0*((biqdRealtype)1.0 - gamma2);
	if(high_or_low)
		ta1 = -ta1;
	ta2 = gam2p1 - siggam2;

	/* And again for the denominator */
	gamma2 = gammad*gammad;
	gam2p1 = (biqdRealtype)1.0 + gamma2;
	siggam2 = (biqdRealtype)(2.0*ROOT2O2)*gammad;

	tb0 = gam2p1 + siggam2;
	tb1 = (biqdRealtype)-2.0*((biqdRealtype)1.0 - gamma2);
	if(high_or_low)
		tb1 = -tb1;
	tb2 = gam2p1 - siggam2;

	/* Now bilinear transform to proper center frequency */
	aa1 = a*ta1;
	f->b0 = (realtype)(ta0 + aa1 + asq*ta2);
	f->b1 = (realtype)((biqdRealtype)2.0*a*(ta0+ta2)+((biqdRealtype)1.0+asq)*ta1);
	f->b2 = (realtype)(asq*ta0 + aa1 + ta2);

	ab1 = a*tb1;
	a0 = (realtype)(tb0 + ab1 + asq*tb2);
	f->a1 = (realtype)((biqdRealtype)2.0*a*(tb0+tb2)+((biqdRealtype)1.0+asq)*tb1);
	f->a2 = (realtype)(asq*tb0 + ab1 + tb2);

	/* Normalize a0 to 1.0 */
	recipa0 = (biqdRealtype)1.0/(biqdRealtype)a0;
	f->b0 = (realtype)((biqdRealtype)f->b0 * recipa0);
	f->b1 = (realtype)((biqdRealtype)f->b1 * recipa0);
	f->b2 = (realtype)((biqdRealtype)f->b2 * recipa0);
	f->a1 = (realtype)((biqdRealtype)f->a1 * recipa0);
	f->a2 = (realtype)((biqdRealtype)f->a2 * recipa0);
  }
	
  return(OKAY);
}

#ifdef FILT_UNDEF
/* DEVELOPMENT - This version uses a direct frequency computation instead of conformal mapping.
 * Potentially could be faster then AES code, but needs correct pole/zero "staggering" to
 * yield correct gain away from center freqs.
 */
int PT_DECLSPEC filtSosPtParametric(PT_HANDLE *hp_sos, int section_num, realtype r_samp_freq, 
                      realtype r_center_freq, realtype boost, realtype Q)
{
  biqdRealtype bandwidth;
  realtype b1, b2, a1, a2, g;
  biqdRealtype A, F;
  biqdRealtype center_freq;

  { 
  	biqdRealtype rad_p, rad_z, cos_omega, gain_z, gain_p;
  	
    center_freq = 2.0 * PI * r_center_freq/r_samp_freq;
    bandwidth = center_freq/Q;

	A = (biqdRealtype)pow((biqdRealtype)10.0, (biqdRealtype)boost/(biqdRealtype)20.0);		/* Cvrt dB to factor */
	if (boost < (realtype)6.0 && boost > (realtype)-6.0) 
		F = (biqdRealtype)sqrt(A);
	else if (A > (biqdRealtype)1.0) 
		F = A/(biqdRealtype)sqrt(2.0);
	else F = A * (biqdRealtype)sqrt(2.0);
		/* If |boost/cut| < 6dB, then doesn't make sense to use 3dB pt.
		   use of root makes bandedge at half the boost/cut amount
		 */
	/* With zero radius inside circle, makes a wa-wa type filter */
	
	/* Done by seat of pants, but seems to be correct */
	rad_p = 1.0 - PI/(Q * r_samp_freq/r_center_freq);		 
	/* With zero radius inside circle, makes a wa-wa type filter */
	/* rad_z = 1.0 - A * (1.0 - rad_p); */
	rad_z = 1.0 - A * (1.0 - rad_p);
	
	cos_omega = cos(center_freq);
	b1 = -2.0 * rad_z * cos_omega;
	b2 = rad_z * rad_z;
	a1 = -2.0 * rad_p * cos_omega;
	a2 = rad_p * rad_p;
	
	if(section_num == 2)
		b1 = b2 = 0.0;
	if(section_num == 3)
		a1 = a2 = 0.0;
		
	gain_z = (1.0 + b1 + b2);
	gain_p = (1.0 + a1 + a2);
	g = gain_p/gain_z;                                         
  }
		
  /* Now store coeffs into sos- note a versus b notation is unconventional */
  if( sosSetSection(hp_sos, section_num, IS_TRUE, r_center_freq, boost, b1, b2, a1, a2, g, 
	                SOS_PARA) != OKAY ) 
    return(NOT_OKAY); 
		
  return(OKAY);
}                
#endif // FILT_UNDEF
