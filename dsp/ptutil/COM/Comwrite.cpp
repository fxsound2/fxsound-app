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
#include "codedefs.h"

/* Standard includes */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "pt_defs.h"
#include "slout.h"

#define PC_TARGET
#include "platform.h"
#include "boardrv1.h"

extern "C"
{
/*
#include "hrdwr.h"
 */
#include "comSftwr.h"
}

#include "com.h"
#include "u_com.h"

int fmsbintoti(realtype *,realtype *);

/*
 * FUNCTION: comIntWrite()
 * DESCRIPTION:
 *  Writes the passed integer value to the passed memory location.
 *  (memory location = i_mem_offset + COMM_MEM_BASE_ADDR).
 */
int PT_DECLSPEC comIntWrite(PT_HANDLE *hp_com, long l_mem_offset, int i_value)
{
	struct comHdlType *cast_handle;
	long long_value;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->turned_off)
		return(OKAY);

	if ((!cast_handle->program_running) && (!cast_handle->softdsp_mode))
       return(NOT_OKAY);

	if (l_mem_offset < 0L)
		return(NOT_OKAY);
    
   /* Transfer value to a long */
   long_value = (long)i_value;
    
	/* To speed up parameter writes with remote operation, use single call.
	 * This new function will handle COMM_MEM_BASE_ADDR offsetting.
	 */
	if(cast_handle->softdsp_mode)
   {
		if( comSftwrWriteParam(cast_handle->comSftwr_hdl, (long)(l_mem_offset), long_value) != OKAY)
			return(NOT_OKAY);
	}

	/* Print the sent value if in debug mode */
	if (cast_handle->debug_mode)
	{
		sprintf(cast_handle->msg1, "(int) value=%d", i_value);
		(cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
		
		sprintf(cast_handle->msg1, "   mem_offset=%ld", l_mem_offset);
		(cast_handle->slout_hdl)->Message(NEXT_LINE, cast_handle->msg1);
	}

	return(OKAY);
}

/*
 * FUNCTION: comLongIntWrite()
 * DESCRIPTION:
 *  Writes the passed long integer value to the passed memory location.
 *  (memory location = i_mem_offset + COMM_MEM_BASE_ADDR).
 */
int PT_DECLSPEC comLongIntWrite(PT_HANDLE *hp_com, long l_mem_offset, long l_value)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->turned_off)
		return(OKAY);

    if ((!cast_handle->program_running) && (!cast_handle->softdsp_mode))
       return(NOT_OKAY);

	if (l_mem_offset < 0L)
		return(NOT_OKAY);

	/* To speed up parameter writes with remote operation, use single call.
	 * This new function will handle COMM_MEM_BASE_ADDR offsetting.
	 */
	if( cast_handle->softdsp_mode )
	{
	  if( comSftwrWriteParam(cast_handle->comSftwr_hdl, (long)(l_mem_offset), l_value) != OKAY)
		return(NOT_OKAY);
	}

	/* Print the sent value if in debug mode */
	if (cast_handle->debug_mode)
	{
		sprintf(cast_handle->msg1, "(long) value=%ld", l_value);
		(cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
		
		sprintf(cast_handle->msg1, "   mem_offset=%ld", l_mem_offset);
		(cast_handle->slout_hdl)->Message(NEXT_LINE, cast_handle->msg1);
	}

	return(OKAY);
}

/*
 * FUNCTION: comRealWrite()
 * DESCRIPTION:
 *  Writes the passed real value to the passed memory location.
 *  (memory location = i_mem_offset + COMM_MEM_BASE_ADDR).
 */
int PT_DECLSPEC comRealWrite(PT_HANDLE *hp_com, long l_mem_offset, realtype r_value)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (cast_handle->turned_off)
		return(OKAY);

    if ((!cast_handle->program_running) && (!cast_handle->softdsp_mode))
       return(NOT_OKAY);

	if (l_mem_offset < 0L)
		return(NOT_OKAY);

	if( cast_handle->softdsp_mode )
	{
	    void *i_val;
	    i_val = &r_value;

  	    /* To speed up parameter writes with remote operation, use single call.
	     * This new function will handle COMM_MEM_BASE_ADDR offsetting.
	     */
	    if( comSftwrWriteParam(cast_handle->comSftwr_hdl, (long)(l_mem_offset), *(long*)i_val) != OKAY)
		  return(NOT_OKAY);
	}

	/* Print the sent value if in debug mode */
	if (cast_handle->debug_mode)
	{
		sprintf(cast_handle->msg1, "(real) value=%g", r_value);
		(cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);

		sprintf(cast_handle->msg1, "   mem_offset=%ld", l_mem_offset);
		(cast_handle->slout_hdl)->Message(NEXT_LINE, cast_handle->msg1);
	}

	return(OKAY);
}

/*
 * FUNCTION: comSetDemoMode()
 * DESCRIPTION:
 * Sets a global shared variable in the comSftwr module.
 */
int PT_DECLSPEC comSetDemoMode(PT_HANDLE *hp_com, int i_DemoModeOnFlag)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	if (comSftwrSetDemoFlag( cast_handle->comSftwr_hdl, i_DemoModeOnFlag) != OKAY)
		return(NOT_OKAY);

	return(OKAY);
}

/*
 * FUNCTIONS TO CONVERT BETWEEN TI C30,C40
 * AND PC FLOATING POINT FORMATS
 */

int fmsbintoti(float *src4, float *dest4)
	{
	unsigned char *msbin = (unsigned char *)src4;
	unsigned char *ti = (unsigned char *)dest4;
	unsigned char sign = 0x00;
	int i;

	/* Intel Binary Format MODIFIED BY PFT
	 * msbin[3] is exponent, which contains sign and bias of 63
	 * msbin[2] is MSB
	 * msbin[1] is Middle byte
	 * msbin[0] is LSB
	 *       EXP      MSB              LSB
	 *    seeeeeee|emmmmmmm|mmmmmmmm|mmmmmmmm
	 *    msbin[3]                   msbin[0]
	 * Note Intel format has a "bias" of 127 in the exponent- ie
	 * 2 =>  64    |    0   |   0    |   0      (exp is 128)
	 * To negate a number, just add 128 to msbin[3], and let it wrap
	 * -2 => 192   |    0   |   0    |   0
	 *
	 * Float = (sign) * 1.mmmm...  * 2 exp (eeeeeeee - 127)

	 *
	 * TI Single Precision Float Format- normalized with an implied most
	 * significant sign bit, for additional precision.
	 *       EXP      MSB              LSB
	 *    eeeeeeee|smmmmmmm|mmmmmmmm|mmmmmmmm
	 *      ti[3]                     ti[0]
	 *          s = sign bit
	 *          e = exponent bit
	 *          m = mantissa bit
	 *
	 * Float =  1 + .mmmm... * 2 exp (eeeeeeee) for s = 0
	 * Float = -2 + .mmmm... * 2 exp (eeeeeeee) for s = 1
	 */

	for (i=0; i<=3; i++) ti[i] = 0; /* Zero all TI bits */
	if( *src4 == 1.0 )
	  return 0; /* all zero's is a one for TI */

	/* any msbin w/ exponent of zero = zero (or also 128 for -0)*/
	if ( (msbin[3] == 0) || (msbin[3] == 128) )
	{
		ti[3] = 0x80; /* For TI normalized zero, exponent = -128 (0x80) */
		return 0;
	}

	sign = msbin[3] & 0x80;  /* 1000|0000b Is there a faster way? */

	ti[2] = msbin[2] & 0x7F; /* Strip out 7 mantissa bits */
	ti[3] = (msbin[3] << 1); /* Shift exponent bits over original sign bit */
	if( (msbin[2] & 0x80) )  /* Add last exponent bit if it was set */
		ti[3] += 1;
	ti[3] -= 127; 				 /* Remove Bias */

	ti[1] = msbin[1];			 /* Copy the rest of the mantissa */
	ti[0] = msbin[0];

	if(sign)
	{
		unsigned char exp = ti[3];  /* Save formed exponent */
		*(long *)ti &= 0x00FFFFFFL; /* Mask out exponent bits */
		*(long *)ti ^= 0x00FFFFFFL; /* Reverse all the mantissa bits */
		*(long *)ti += 1;           /* Add one, completeing mantissa negation */
											 /* Carry bit will overflow into exponent */
											 /* where it needs to be treated as a decrement */
		ti[3] = exp - ti[3];			 /* Restore exponent, decremented by carry bit */
		ti[2] |= sign;				    /* Set sign bit */
	}
	return 0;
}
