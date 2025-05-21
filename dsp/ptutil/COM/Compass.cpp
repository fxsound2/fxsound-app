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
#include <stdio.h> 
#include "codedefs.h"
#include "pt_defs.h"
extern "C"
{
#include "c_dsps.h"
}
#include "com.h"
#include "u_com.h"
#include "dongle.h"

int com_CalcPassAddress(PT_HANDLE *, int, int, int, short unsigned *);
       
/*
 * FUNCTION: comReadPassword()
 * DESCRIPTION:
 *   Reads the passed password from either the hardware card or dongle.
 *   It is in software mode, then it will read from dongle, unless the 
 *   i_force_from_card flag is true.
 *   NOTE - modified to only read from card, no longer supports dongle.
 */
int PT_DECLSPEC comReadPassword(PT_HANDLE *hp_com, int i_password_type, 
					unsigned long *ulp_password)
{
   struct comHdlType *cast_handle;
   short unsigned address;
   int read_from_card;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   /* If there is no card or dongle, there is nothing to read */
   if (!(cast_handle->card_exists) && !(cast_handle->dongle_exists))
       return(NOT_OKAY);

   /* Figure out if we should read from the dongle or the card */
   read_from_card = IS_TRUE;
   if ((cast_handle->softdsp_mode) && (cast_handle->dongle_exists))
	   read_from_card = IS_FALSE;
   
   /* Figure out the address to read from */
   if (com_CalcPassAddress(hp_com, i_password_type, read_from_card, 
		                   cast_handle->softdsp_mode, &address) != OKAY)
      return(NOT_OKAY);

   /* Read the password */
   if (read_from_card)
   {
      /* Get the password from the card */
	  if (comEepromUnsignedLongRead(hp_com, address, ulp_password) != OKAY)
         return(NOT_OKAY);
   }
   else
   {
		/* Fail if trying to read dongle
      if (dongleReadMemory((int)address, ulp_password, 
		                 cast_handle->slout_hdl) != OKAY)
	    */
	     return(NOT_OKAY);
   }
    
   return(OKAY);
} 
              
/*
 * FUNCTION: comWritePassword()
 * DESCRIPTION:
 *   Writes the passed password to either the hardware card or dongle.
 *   It is in software mode, then it will write to dongle, unless the 
 *   i_force_to_card flag is true.
 *   NOTE - modified to only read from card, no longer supports dongle.
 */
int PT_DECLSPEC comWritePassword(PT_HANDLE *hp_com, int i_password_type,
					 unsigned long ul_password)
{
	struct comHdlType *cast_handle;
	short unsigned address;
	int write_to_card;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

   /* If there is no card or dongle, there is nothing to write */
   if (!(cast_handle->card_exists) && !(cast_handle->dongle_exists))
       return(NOT_OKAY);

   /* Figure out if we should write to dongle or the card */
   write_to_card = IS_TRUE;
   if ((cast_handle->softdsp_mode) && (cast_handle->dongle_exists))
	   write_to_card = IS_FALSE;

   /* Figure out the address to write */
   if (com_CalcPassAddress(hp_com, i_password_type, write_to_card, 
		                   cast_handle->softdsp_mode, &address) != OKAY)
      return(NOT_OKAY);

   if (!write_to_card)
   {
		/* Write to dongle */
		/* Fail if trying to write to dongle
	   if (dongleWriteMemory((int)address, ul_password, 
		                  cast_handle->slout_hdl) != OKAY)
		 */
	      return(NOT_OKAY);
   }
   else
   {
      /* Write to hardware card */
	  if (comEepromUnsignedLongWrite(hp_com, address, ul_password) != OKAY)
         return(NOT_OKAY);
   }
	
   return(OKAY);
}

/*
 * FUNCTION: com_ReadSerialNum()
 * DESCRIPTION:
 *   Reads the serial number from either the hardware card or dongle.
 *   It is in software mode, then it will read from dongle, unless the 
 *   i_force_from_card flag is true.
 *   NOTE - modified to only read from card, no longer supports dongle.
 */
int com_ReadSerialNum(PT_HANDLE *hp_com, int i_force_from_card, 
					 unsigned long *ulp_serial_num)
{
   struct comHdlType *cast_handle;
   short unsigned address;
   int read_from_card;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   /* Figure out if we should read from the dongle or the card */
   read_from_card = IS_FALSE;
   if ((!(cast_handle->softdsp_mode)) || (i_force_from_card))
      read_from_card = IS_TRUE;

   /* Figure out the address */
   if (read_from_card)
      address = COM_PASS_HARD_ONCARD_SERIAL_NUMBER;
   else
      address = COM_PASS_SOFT_DONGLE_SERIAL_NUMBER;
	
   /* Read the password */
   if (read_from_card)
   {
      /* Get the serial number from the card */
	  if (comEepromUnsignedLongRead(hp_com, address, ulp_serial_num) != OKAY)
         return(NOT_OKAY);
   }
   else
   {
	  /* Read the serial number from the dongle */
		/* Fail if trying to read from dongle.
      if (dongleReadMemory((int)address, ulp_serial_num, 
		                  cast_handle->slout_hdl) != OKAY)
		 */
	     return(NOT_OKAY);
   }
    	
   return(OKAY);
} 

/*
 * FUNCTION: comWriteSerialNum()
 * DESCRIPTION:
 *   Writes the serial number to either the hardware card or dongle.
 *   It is in software mode, then it will writes to dongle, unless the 
 *   i_force_to_card flag is true.
 *   NOTE - modified to only read from card, no longer supports dongle.
 */
int PT_DECLSPEC comWriteSerialNum(PT_HANDLE *hp_com, int i_force_to_card, 
					 unsigned long ul_serial_num)
{
   struct comHdlType *cast_handle;
   short unsigned address;
   int write_to_card;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   /* Figure out if we should read from the dongle or the card */
   write_to_card = IS_FALSE;
   if ((!(cast_handle->softdsp_mode)) || (i_force_to_card))
      write_to_card = IS_TRUE;

   /* Figure out the address */
   if (write_to_card)
      address = COM_PASS_HARD_ONCARD_SERIAL_NUMBER;
   else
      address = COM_PASS_SOFT_DONGLE_SERIAL_NUMBER;
	
   /* Write the serial number */
   if (write_to_card)
   {
      /* Write the serial number to the card */
	  if (comEepromUnsignedLongWrite(hp_com, address, ul_serial_num) != OKAY)
         return(NOT_OKAY);
   }
   else
   {
	  /* Write to dongle */
	  /* Fail if trying to write to dongle.
	  if (dongleWriteMemory((int)address, ul_serial_num, 
		                  cast_handle->slout_hdl) != OKAY)
		*/
	     return(NOT_OKAY);
   }
    	
   return(OKAY);
} 

/*
 * FUNCTION: com_CalcPassAddress()
 * DESCRIPTION:
 *  Calculate the address where the passed password address is either on the
 *  card or on the dongle.
 * 
 */
int com_CalcPassAddress(PT_HANDLE *hp_com, int i_password_type, int i_from_card, 
						int i_softdsp, short unsigned *su_address)
{
   struct comHdlType *cast_handle;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   /* Figure out the address */
   if (i_password_type == COM_PASSWORD_STUDIO_SYSTEM)
   {
      if (i_from_card)
	  {
         if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_STUDIO_SYSTEM;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_STUDIO_SYSTEM;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_STUDIO_SYSTEM;
   }
	
   else if (i_password_type == COM_PASSWORD_REVERB)
   {
      if (i_from_card)
	  {
         if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_REVERB;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_REVERB;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_REVERB;   
   }

   else if (i_password_type == COM_PASSWORD_DELAY)
   {
      if (i_from_card)
	  {
         if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_DELAY;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_DELAY;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_DELAY;     
   }
	      
   else if (i_password_type == COM_PASSWORD_CHORUS)
   {
      if (i_from_card)
	  {
         if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_CHORUS;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_CHORUS;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_CHORUS;     
   }
 
   else if (i_password_type == COM_PASSWORD_FLANGE)
   {
      if (i_from_card)
	  {
         if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_FLANGE;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_FLANGE;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_FLANGE;     
   }

   else if (i_password_type == COM_PASSWORD_PITCH)
   {
      if (i_from_card)
	  {
         if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_PITCH;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_PITCH;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_PITCH;     
   }
	
   else if (i_password_type == COM_PASSWORD_PEQ)
   {
      if (i_from_card)
	  {
         if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_PEQ;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_PEQ;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_PEQ;     
   }
	
   else if (i_password_type == COM_PASSWORD_LEADSYN)
   {
      if (i_from_card)
	  {
         if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_LEADSYN;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_LEADSYN;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_LEADSYN;     
   }
	
   else if (i_password_type == COM_PASSWORD_PAN)
   {
      if (i_from_card)
	  {
         if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_PAN;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_PAN;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_PAN;     
   }
	 
   else if (i_password_type == COM_PASSWORD_TREMOLO)
   {
      if (i_from_card)
	  {
         if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_TREMOLO;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_TREMOLO;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_TREMOLO;     
   }

   else if (i_password_type == COM_PASSWORD_AURAL)
   {
      if (i_from_card)
	  {
         if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_AURAL;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_AURAL;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_AURAL;     
   }

   else if (i_password_type == COM_PASSWORD_MAXIMIZE)
   {
      if (i_from_card)
	  {
         if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_MAXIMIZE;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_MAXIMIZE;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_MAXIMIZE;     
   }

   else if (i_password_type == COM_PASSWORD_LEX)
   {
      if (i_from_card)
	  {
       if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_LEX;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_LEX;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_LEX;     
   }

   else if (i_password_type == COM_PASSWORD_APITCH)
   {
      if (i_from_card)
	  {
       if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_APITCH;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_APITCH;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_APITCH;     
   }

   else if (i_password_type == COM_PASSWORD_WID)
   {
      if (i_from_card)
	  {
       if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_WID;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_WID;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_WID;     
   }
   
   else if (i_password_type == COM_PASSWORD_COMPRESSOR)
   {
      if (i_from_card)
	  {
       if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_CMP;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_CMP;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_CMP;     
   }

   else if (i_password_type == COM_PASSWORD_PLY)
   {
      if (i_from_card)
	  {
       if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_PLY;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_PLY;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_PLY;     
   }

   else if (i_password_type == COM_PASSWORD_GVB)
   {
      if (i_from_card)
	  {
       if (i_softdsp)
		    *su_address = COM_PASS_SOFT_ONCARD_GVB;
		 else
		    *su_address = COM_PASS_HARD_ONCARD_GVB;
	  }
      else
         *su_address = COM_PASS_SOFT_DONGLE_GVB;     
   }

   return(OKAY);
}
