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

#include <stdio.h> 
#include "codedefs.h"

extern "C"
{
#include "c_dsps.h"
}
#include "com.h"
#include "u_com.h"

extern "C"
{
#include "comSftwr.h"
}

/*
 * FUNCTION: comGetCardExists()
 * DESCRIPTION:
 *  
 *   Pass back whether the card exists.
 *
 */
int PT_DECLSPEC comGetCardExists(PT_HANDLE *hp_com, int *ip_card_exists)
{
   struct comHdlType *cast_handle;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   *ip_card_exists = cast_handle->card_exists;
   
   return(OKAY);
} 

/*
 * FUNCTION: comGetDongleExists()
 * DESCRIPTION:
 *  
 *   Pass back whether the dongle exists.
 *
 */
int PT_DECLSPEC comGetDongleExists(PT_HANDLE *hp_com, int *ip_dongle_exists)
{
   struct comHdlType *cast_handle;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   *ip_dongle_exists = cast_handle->dongle_exists;
   
   return(OKAY);
} 


/*
 * FUNCTION: comGetAesExists()
 * DESCRIPTION:
 *  
 *   Pass back whether the digital expansion card exists.
 *
 */
int PT_DECLSPEC comGetAesExists(PT_HANDLE *hp_com, int *ip_aes_exists)
{
   struct comHdlType *cast_handle;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   *ip_aes_exists = cast_handle->aes_exists;
   
   return(OKAY);
}

/*
 * FUNCTION: comGetSerialNum()
 * DESCRIPTION:
 *  
 *   Pass back the serial number on the card.
 *   The flag passed back as ip_serial_from_card specifies that the softdsp
 *   serial number came from the hardware card.
 *
 */
int PT_DECLSPEC comGetSerialNum(PT_HANDLE *hp_com, unsigned long *ulp_serial_num)
{
   struct comHdlType *cast_handle;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   *ulp_serial_num = cast_handle->serial_num;
   
   return(OKAY);
}

/*
 * FUNCTION: comGetMainNumSamples()
 * DESCRIPTION:
 *  
 *   Pass back the number of memory samples on the main card.
 *
 */
int PT_DECLSPEC comGetMainNumSamples(PT_HANDLE *hp_com, long *lp_main_num_samples)
{
   struct comHdlType *cast_handle;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   *lp_main_num_samples = cast_handle->main_num_samples;
   
   return(OKAY);
}  

/*
 * FUNCTION: comGetExpandedNumSamples()
 * DESCRIPTION:
 *  
 *   Pass back the number of memory samples on the expansion card.
 *
 */
int PT_DECLSPEC comGetExpandedNumSamples(PT_HANDLE *hp_com, long *lp_expanded_num_samples)
{
   struct comHdlType *cast_handle;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   *lp_expanded_num_samples = cast_handle->expanded_num_samples;
   
   return(OKAY);
}

/*
 * FUNCTION: comGetHasBeenCracked()
 * DESCRIPTION:
 *  
 *   Pass back whether or not the code has been cracked.
 *
 */
int PT_DECLSPEC comGetHasBeenCracked(PT_HANDLE *hp_com, int i_uncracked_demo_flag, int *ip_cracked)
{
	int demo_flag;

   struct comHdlType *cast_handle;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   if( comSftwrGetDemoFlag(cast_handle->comSftwr_hdl, &demo_flag) != OKAY )
		return(NOT_OKAY);

   if( demo_flag != i_uncracked_demo_flag )
		*ip_cracked = 1;
   else
		*ip_cracked = 0;

   return(OKAY);
}

/*
 * FUNCTION: comGetProcessorIndex()
 * DESCRIPTION:
 *  
 *   Pass back the current Processor Index, which starts at 0.
 *   Note that the Processor Number used at the gui level is = processor_index + 1.
 *
 */
int PT_DECLSPEC comGetProcessorIndex(PT_HANDLE *hp_com, int *ip_processor_index)
{
   struct comHdlType *cast_handle;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   *ip_processor_index = cast_handle->processor_index;
   
   return(OKAY);
} 
