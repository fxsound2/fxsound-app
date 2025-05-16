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

#include <time.h> 
#include <stdio.h>

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

extern "C"
{
	#include "hrdwr.h"
	#include "hutil.h"
}
                     
/*
 * FUNCTION: comEepromUnsignedLongRead()
 * DESCRIPTION:
 *   Reads the unsigned long value from eeprom. 
 */
int PT_DECLSPEC comEepromUnsignedLongRead(PT_HANDLE *hp_com, short unsigned i_address, unsigned long *ulp_value)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

    /* Program cannot be running during eeprom access */
    if ((cast_handle->program_running) || (cast_handle->program_loaded))
       return(NOT_OKAY);
 
	if (cast_handle->turned_off)
	{
		*ulp_value = (unsigned)0L;
		return(OKAY);
	}
		
	if ((i_address < 0)||(i_address > COM_EEPROM_MAX_MEMORY))
		return(NOT_OKAY);

	if ( comHrdEepromReadUlong(cast_handle->board_address, (short unsigned)i_address, ulp_value) != OKAY)
       return(NOT_OKAY);
    
	return(OKAY);
} 

/*
 * FUNCTION: comEepromUnsignedLongWrite()
 * DESCRIPTION:
 *  Writes the passed unsigned long integer value to the specified board eemprom address.
 *
 */
int PT_DECLSPEC comEepromUnsignedLongWrite(PT_HANDLE *hp_com, short unsigned i_address, unsigned long ul_value)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle->turned_off)
		return(OKAY);

	if (cast_handle == NULL)
		return(NOT_OKAY);

    /* Program cannot be running during eeprom access */
    if ((cast_handle->program_running) || (cast_handle->program_loaded))
       return(NOT_OKAY);

	if ((i_address < 0)||(i_address > COM_EEPROM_MAX_MEMORY))
		return(NOT_OKAY);
		
    if( comHrdEepromWriteUlong(cast_handle->board_address, (unsigned)i_address, ul_value) < 0)
	   return(NOT_OKAY);
	
	/* Print the sent value if in debug mode */
	if (cast_handle->debug_mode)
	{
		sprintf(cast_handle->msg1, "(unsigned long) EEPROM value=%lu", ul_value);
		(cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
		
		sprintf(cast_handle->msg1, "   address=%d", i_address);
		(cast_handle->slout_hdl)->Message(NEXT_LINE, cast_handle->msg1);
	}

	return(OKAY);
}
