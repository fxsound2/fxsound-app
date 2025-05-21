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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "codedefs.h"
#include "pt_defs.h"
#include "slout.h"

#define PC_TARGET
#include "platform.h"
#include "boardrv1.h"

extern "C"
{
#include "hrdwr.h"
#include "comSftwr.h"
#include "c_dsps.h"
}

#include "comSftwrCPP.h"

#include "com.h"
#include "u_com.h"

/*
 * FUNCTION: comMemReInitialize()
 * DESCRIPTION:
 *  This function is called when a parameter written to the DSP
 *  requires that the memory of the DSP be reinitialized.
 *  For example, in the LexReverb, after the roomsize is written,
 *  this function needs to be called to reinit all the memory pointers
 *  that are used in the LexReverb.
 */
int PT_DECLSPEC comMemReInitialize(PT_HANDLE *hp_com, realtype r_sampling_freq)
{
	struct comHdlType *cast_handle;
	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

   /* Currently just does a reinitialization of memory. Flag can be set to also init parameters */
	if( comSftwrInitDspAlgorithmCPP(cast_handle->comSftwr_hdl, r_sampling_freq, 
	                                DSPS_RE_INIT_MEMORY) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTION: ComCheckOutProcessor()
 * DESCRIPTION:
 *   Checks out or checks in a processor number for use with this particular instance of this plug-in.
 *   Set i_checkout_flag to IS_TRUE to checkout a processor or to IS_FALSE to check one in.
 */
int comCheckOutProcessor(char *cp_dsp_function, int i_checkout_flag, int *ip_processor_num)
{
	int processor_found;
	int processor_index;
	int plug_in_index;
	int i, j;

	/* Statics */
	static int first_entry = IS_TRUE;

	/* Array containing processor status, 1 -> checked out, 0 -> available */
	static int processor_array[DSPFX_NUM_PLUGIN_TYPES][DSPFX_MAX_NUM_PROCS];

	/* If this is the first call to this function, zero the processor array */
	if (first_entry == IS_TRUE)
	{
		first_entry = IS_FALSE;

		for(i=0; i < DSPFX_NUM_PLUGIN_TYPES; i++)
		{
			for(j=0; j < DSPFX_MAX_NUM_PROCS; j++)
				processor_array[i][j] = 0;
		}
	}

	/* Find the plug-in type index number */
	plug_in_index = 0;

	/* Note that since for each plug-in type, different strings can be passed in,
	 * ie, flang1, flang2, we will compare only the first unique chars for each
	 * plug-in type.
	 */
	if (strncmp(cp_dsp_function, "dly", 3) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "flang", 5) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "chor", 4) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "pitch", 5) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "peq", 3) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "trm", 3) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "pan", 3) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "R1s", 3) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "cmp8", 4) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "aural", 5) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "max", 3) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "lex", 3) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "apt", 3) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "ply", 3) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "cmp0", 4) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "wid", 3) != 0)
		plug_in_index++;
	else if (strncmp(cp_dsp_function, "proto10", 7) != 0)
		plug_in_index++;

	if ( plug_in_index >= DSPFX_NUM_PLUGIN_TYPES )
		return(NOT_OKAY);

	if (i_checkout_flag == IS_TRUE)
	{
		i = 0;
		processor_found = 0;
		while ( (processor_found == 0) && (i <  DSPFX_MAX_NUM_PROCS) )
		{
			if ( processor_array[plug_in_index][i] == 0 )
			{
				/* Note that Processor numbers range from 1 to DSPFX_MAX_NUM_PROCS */
				*ip_processor_num = i + 1;
				processor_array[plug_in_index][i] = 1;
				processor_found = 1;
			}
			i++;
		}

		if (processor_found == 0)
		{
			/* We hit this case if all processors are assigned */
			return(NOT_OKAY);
		}
	}
	else
	{
		/* Checkin case */
		processor_index = *ip_processor_num - 1;

		if ( (processor_index < 0) || (processor_index >= DSPFX_MAX_NUM_PROCS) )
			return(NOT_OKAY);

		processor_array[plug_in_index][processor_index] = 0;
	}

	return(OKAY);
}


