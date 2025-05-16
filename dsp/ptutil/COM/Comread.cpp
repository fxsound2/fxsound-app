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

#define PC_TARGET
#include "platform.h"
#include "boardrv1.h"
extern "C"
{
#include "c_dsps.h"
}
#include "com.h"
#include "u_com.h"

// This set of includes is for DSP interface code reads
extern "C"
{
#include "hrdwr.h"
}

/* #include "c_dly1.h" */ /* For clip level and status defines */

/*
 * FUNCTION: comIntRead()
 * DESCRIPTION:
 *  Reads the integer value from the DSP.  Values are passed
 *  serially and are thus order dependent.
 */
int PT_DECLSPEC comIntRead(PT_HANDLE *hp_com, int *ip_value)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->turned_off)
	{
		*ip_value = 0;
		return(OKAY);
    }
    
#ifdef DSPFX1
	{
		long tmp_val;
		if( comHrdwrGetChar(cast_handle->board_address, &tmp_val) != OKAY )
		 	return(NOT_OKAY);
		*ip_value = (int) tmp_val;
	}
#endif

#ifdef DSPR_TIGER32
	*ip_value = read_int_fromTIGER32();
#endif
#ifdef ARIEL_CYCLOPS
/* Problem with Ariel since values are passed via dual port instead of serially */
#endif
	return(OKAY);
}

/*
 * FUNCTION: comLongIntRead()
 * DESCRIPTION:
 * Reads the long integer value from DSP. Values are read 
 * serially from a stream, and are thus order dependent.
 */
int PT_DECLSPEC comLongIntRead(PT_HANDLE *hp_com, long *lp_value)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->turned_off)
	{
	    *lp_value = 0L;
		return(OKAY);
    }
    
#ifdef DSPFX1
	if( comHrdwrGetChar(cast_handle->board_address, lp_value) != OKAY )
	 	return(NOT_OKAY);
#endif
#ifdef DSPR_TIGER32
	*lp_value = ( (read_int_fromTIGER32())<<16 & (read_int_fromTIGER32()) );
#endif
#ifdef ARIEL_CYCLOPS
/* Problem with Ariel since tranfers are done with dual port, not serial stream */
#endif

	return(OKAY);
}  

/*
 * FUNCTION: comRealRead()
 * DESCRIPTION:
 * Reads the real value from DSP. Values are read serially from a stream
 * and are thus order dependent.
 */
int PT_DECLSPEC comRealRead(PT_HANDLE *hp_com, realtype *rp_value)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->turned_off)
	{
	    *rp_value = 0.0;
		return(OKAY);
    }
    
#ifdef DSPFX1
	if( comHrdwrGetChar(cast_handle->board_address, (long *)rp_value) != OKAY )
	 	return(NOT_OKAY);
#endif
#ifdef DSPR_TIGER32
	*rp_value = ( (read_int_fromTIGER32())<<16 & (read_int_fromTIGER32()) );
#endif
#ifdef ARIEL_CYCLOPS
/* Problem with Ariel since tranfers are done with dual port, not serial stream */
#endif

	return(OKAY);
}

/*
 * FUNCTION: comReadMeter()
 * DESCRIPTION:
 * 
 *  Read the next meter value.  This function should be called for the meters in the
 *  same order that the dsp code wrote them.
 *
 */
int PT_DECLSPEC comReadMeter(PT_HANDLE *hp_com, int i_wait_for_value, long *lp_meter_value, int *ip_got_value)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->turned_off)
	{
	    *lp_meter_value = 0L;
	    *ip_got_value = 1;
		return(OKAY);
    }

    if (!cast_handle->program_running)
    {
       *ip_got_value = 0;
       return(NOT_OKAY);
    }
    
#ifdef DSPFX1
	/* Get meter val.
	 * NOTES- From dspfx1 card with Crystal 16 bit part,
	 * MAX POS is  2147418122 ( close to ? 32767 << 16) while
	 * MAX NEG is -2147483648 (-32768 << 16) (also 0x80000000).
	 * Note that limits.h shows long is +/- 2147483647  .
	 * Also note that labs fails for labs( -2147483648 ).
	 */
 	if( i_wait_for_value )
	{
		if( comHrdwrGetChar(cast_handle->board_address, lp_meter_value) != OKAY )
		{
			*ip_got_value = 0;
	 		return(NOT_OKAY);
	 	}
	 	*ip_got_value = 1;
	}	
	else
	{
		/* Only gets char if DSP is ready and char is present */
		/* Currently don't wait if DSP is not available */
		/* NOTE- CURRENTLY THIS MODE IS NOT USED. IF NEEDED, THE
		 * FUNCTION BELOW SHOULD BE REIMPLEMENTED IN THE
		 * comHrdwr LAYER SO COM DOESN'T NEED TO LINK TO SCARD.
		 */
		/*
		if( hrdwrReadXferRegIfFullAndNotBusy(cast_handle->board_address, (short FAR *)&busy, 
		                           (long FAR *)lp_meter_value, 
		                           (short FAR *)&full) != OKAY)
			return(NOT_OKAY);
		*ip_got_value = full;
		*/
		*ip_got_value = 0;
	}

	if( *ip_got_value )
	{
		if(*lp_meter_value == LONG_MAX_NEGATIVE_CRYSTAL )
	   		(*lp_meter_value)--;
		else
	   		*lp_meter_value = labs(*lp_meter_value );
	}
#endif
#ifdef DSPR_TIGER32
	*ip_meter_value = abs( read_int_fromTIGER32() );
	*ip_got_value = 1;
#endif
#ifdef ARIEL_CYCLOPS
	DSPC40_SetCounter(cast_handle->board_address, INPUT_LEVEL_REG);
	*ip_meter_value = abs( DSPC40_RamLowerRead(cast_handle->board_address, MEM_CLOCK) );
	*ip_got_value = 1;
#endif

	return(OKAY);
}

/*
 * FUNCTION: comReadStatus()
 * DESCRIPTION:
 * 
 *  Read the next status value.  This function should be called in the
 *  same order that the dsp code wrote them.
 *
 */
int PT_DECLSPEC comReadStatus(PT_HANDLE *hp_com, int i_wait_for_value, long *lp_status_value, int *ip_got_value)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->turned_off)
	{
	    *lp_status_value = 0L;
	    *ip_got_value = 1;
		return(OKAY);
    }

    if (!cast_handle->program_running)
    {
       *ip_got_value = 0;
       return(NOT_OKAY);
    }
    
#ifdef DSPFX1
	/* Get status val. */
	if( i_wait_for_value )
	{
		if( comHrdwrGetChar(cast_handle->board_address, lp_status_value) != OKAY )
		{
			*ip_got_value = 0;
	 		return(NOT_OKAY);
	 	}
	}	
	else
	{
		/* Currently don't wait if DSP is not available */
		/* NOTE- CURRENTLY THIS MODE IS NOT USED. IF NEEDED, THE
		 * FUNCTION BELOW SHOULD BE REIMPLEMENTED IN THE
		 * comHrdwr LAYER SO COM DOESN'T NEED TO LINK TO SCARD.
		 */
		/*
		if( hrdwrReadXferRegIfFullAndNotBusy(cast_handle->board_address, (short FAR *)&busy,
		                           (long FAR *)lp_status_value,
								   (short FAR *)&full) != OKAY)
				return(NOT_OKAY);
		 */
		*ip_got_value = 0;
	}
#endif
#ifdef DSPR_TIGER32
#endif
#ifdef ARIEL_CYCLOPS
#endif

	return(OKAY);
}
