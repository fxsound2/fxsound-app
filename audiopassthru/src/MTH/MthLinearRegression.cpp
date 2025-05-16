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

/* Standard includes */
#include <math.h>

#include "codedefs.h"
#include "mth.h"
#include "u_mth.h"

/*
 * FUNCTION: mthLinearRegression()
 * DESCRIPTION:
 *
 *   Does a straight line fit to a set of x-y data points.
 *   See "Probability and Statistics for Engineers", Walpole and Myers, p. 283
 *
 *	  Forms estimates for slope and offset so that:    y ~= slope * x + offset
 *
 */
int PT_DECLSPEC mthLinearRegression(realtype *datax, realtype *datay, int num_points,
												realtype *rp_slope, realtype *rp_offset,
												realtype *rp_mean_sqr_error)
{
	realtype sumx = 0.0;
	realtype sumy = 0.0;
	realtype sumxy = 0.0;
	realtype sumxsqrd = 0.0;
	realtype sumysqrd = 0.0;
	realtype Syy, Sxy, SSE;
	realtype denom = 0.0;
	realtype num_points_inv;
	int i;

	for(i=0; i<num_points; i++)
	{
		sumx = sumx + datax[i];
		sumy = sumy + datay[i];
		sumxy = sumxy + (datax[i] * datay[i]);
		sumxsqrd = sumxsqrd + (datax[i]) * (datax[i]);
		sumysqrd = sumysqrd + (datay[i]) * (datay[i]);
	}

	/* Calculate denominator first to make sure its not zero */
	denom = num_points * sumxsqrd - sumx * sumx;
	if(denom == 0.0)
	{
		*rp_slope = 0.0;
		*rp_offset = 0.0;
		return(OKAY);
	}

	num_points_inv = (realtype)1.0/(realtype)num_points;

	*rp_slope = (num_points * sumxy - sumx * sumy) / denom;

	*rp_offset = (sumy - *rp_slope * sumx) * num_points_inv;

	Syy = sumysqrd - (sumy * sumy) * num_points_inv;

	Sxy = sumxy - (sumx * sumy) * num_points_inv;

	/* Note - this form of calculation is sometimes giving a negative number in single precision.
	 * However, the size of the value is close the correct value, just has wrong sign, so fabs is used
	 * to fix it.
	 */
	SSE = (realtype)fabs(Syy - *rp_slope * Sxy);

	*rp_mean_sqr_error = (realtype)sqrt(SSE) * num_points_inv;

	return(OKAY);
}

/*
 * FUNCTION: mthLinearRegressionIntX()
 * DESCRIPTION:
 *
 *	  Forms estimates for slope and offset so that:    y ~= slope * x + offset
 *   In this special version the x data points are assumed to be of the form:
 *   0, 1, 2, 3, . . . (num_points - 1)
 *
 */
int PT_DECLSPEC mthLinearRegressionIntX(realtype *datay, int num_points,
													 realtype *rp_slope, realtype *rp_offset,
													 realtype *rp_mean_sqr_error)
{
	realtype sumx = 0.0;
	realtype sumy = 0.0;
	realtype sumxy = 0.0;
	realtype sumxsqrd = 0.0;
	realtype sumysqrd = 0.0;
	realtype Syy, Sxy, SSE;
	realtype denom;
	realtype num_points_inv;
	int i;

	for(i=0; i<num_points; i++)
	{
		realtype datax;
		datax = (realtype)i;
		sumx = sumx + datax;
		sumy = sumy + datay[i];
		sumxy = sumxy + (datax * datay[i]);
		sumxsqrd = sumxsqrd + (datax) * (datax);
		sumysqrd = sumysqrd + (datay[i]) * (datay[i]);
	}

	/* Calculate denominator first to make sure its not zero */
	denom = num_points * sumxsqrd - sumx * sumx;
	if(denom == 0.0)
	{
		*rp_slope = 0.0;
		*rp_offset = 0.0;
		return(OKAY);
	}

	num_points_inv = (realtype)1.0/(realtype)num_points;

	*rp_slope = (num_points * sumxy - sumx * sumy) / denom;

	*rp_offset = (sumy - *rp_slope * sumx) * num_points_inv;

	Syy = sumysqrd - (sumy * sumy) * num_points_inv;

	Sxy = sumxy - (sumx * sumy) * num_points_inv;

	/* Note - this form of calculation is sometimes giving a negative number in single precision.
	 * However, the size of the value is close the correct value, just has wrong sign, so fabs is used
	 * to fix it.
	 */
	SSE = (realtype)fabs(Syy - *rp_slope * Sxy);

	/*
	realtype SSE_sum = 0.0;
	for(i=0; i<num_points; i++)
	{
		realtype error;
		error = datay[i] - (*rp_slope * i + *rp_offset);
		SSE_sum += error * error;
	}
	 */

	*rp_mean_sqr_error = (realtype)sqrt(SSE) * num_points_inv;

	return(OKAY);
}
