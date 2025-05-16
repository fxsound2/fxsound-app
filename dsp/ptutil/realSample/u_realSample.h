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
#ifndef _U_REALSAMPLE_H_
#define _U_REALSAMPLE_H_

/* Sets the mix point at which the original signal starts to lower in volume */
#define REALSAMPLE_SPECIAL_MIX_BREAKPOINT 0.5

#include "slout.h"
#include "realSample.h"

/* Configuration Handle definition */
struct realSampleHdlType {
	/* Initialization info */
	CSlout *slout_hdl;
	char msg1[1024]; /* String for messages */

	/* Attributes */
	int num_channels;
	long samples_per_sec;

	/* Samples info */
	long num_samples;
	realtype *rp_samples;
};

/*
 * LOCAL FUNCTIONS 
 */

/* realSample.cpp */
int realSample_FreeUpSamplesArray(PT_HANDLE *);

#endif //_U_REALSAMPLE_H_