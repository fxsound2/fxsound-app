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

/*
 * FILE: u_comSftwr.h
 * DATE: 9/27/04
 * AUTOHOR: Paul Titchener
 * DESCRIPTION:
 *
 * Local header file for the comSftwr module
 */
#ifndef _U_COMSFTWR_H_
#define _U_COMSFTWR_H_

#include "codedefs.h"
#include "pt_defs.h"
#include "c_dsps.h"

/* Local Functions */

/* comSftwr handle definition */
struct comSftwrHdlType {

	/* ---> Globals formerly in dspFunc.c <--- */
	/* Note that in orig version, dsp_params had a size of DSPFX_MAX_NUM_PROCS * 2 * DSPS_MAX_NUM_PARAMS */
	/* Note that since player shares state and param space with all the dsp functs these still have to be big */
	float dsp_state[DSPFX_MAX_NUM_PROCS * DSPS_NUM_STATE_VARS];
	float dsp_params[DSPFX_MAX_NUM_PROCS * 2 * DSPS_MAX_NUM_PARAMS];
	long dsp_memory_size_required;
	long dsp_memory_size;
	float *dsp_memory;

	/* ---> Globals formerly in comSftwr.c <--- */
	int dsp_function_index;
	int comSftwrDemoFlag;
	int comSftwrReCuePending;
	int comSftwrBitWidth;
	struct hardwareMeterValType ComSftwrMeterData;
	int (*comSftDspInitPtr[DSPS_MAX_NUM_PROC_FUNCTIONS]) (float *, float *, long, float *, int, float);
	void (*comSftDspProcessPtr[DSPS_MAX_NUM_PROC_FUNCTIONS]) (long *, int, float *, float *, float *, struct hardwareMeterValType *, int);
	long sample_count;
	long silence_count;
};

#endif //_U_COMSFTWR_H
