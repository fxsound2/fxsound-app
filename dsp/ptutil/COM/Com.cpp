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
#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#endif

#include <math.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "codedefs.h"
#include "platform.h"
#include "pt_defs.h"
#include "pcfg.h"

#define PC_TARGET
#include "platform.h"
#include "boardrv1.h"

extern "C"
{
#include "hrdwr.h"
#include "comSftwr.h"
#include "hutil.h"
#include "c_dsps.h"
}

#include "comSftwrCPP.h"

#include "com.h"
#include "u_com.h"
#include "mry.h"
#include "dongle.h"

/* Local defines */ 
int com_SetCardStatus(PT_HANDLE *, char *);
int com_SetSoftDspStatus(PT_HANDLE *hp_com);
int com_LoadDspExe(PT_HANDLE *); 
int com_LoadProgram(PT_HANDLE *, char *, char *);
int com_SendSamplingFreq(PT_HANDLE *, realtype);  
int com_ReadSamplingFreq(PT_HANDLE *, realtype *, int *); 
int com_SendIoInfo(PT_HANDLE *);

/*
 * FUNCTION: comInit()
 * DESCRIPTION:
 *  Allocates and initializes the passed com handle.  It is passed
 *  the base memory address that parameters will be written to
 *
 *  Pass NULL for the cp_exe_dirpath, if there is no initialization
 *  execuable..
 */
int PT_DECLSPEC comInit(PT_HANDLE **hpp_com, int i_softdsp, int i_board_address, 
			int i_processor_num, long l_base_address, char *cp_exe_dirpath, 
			int i_num_cards_configured, CSlout *hp_slout)
{
	struct comHdlType *cast_handle;

	if (l_base_address < 0L)
		return(NOT_OKAY);

	/* Allocate the handle */
	cast_handle = (struct comHdlType *)calloc(1,
						sizeof(struct comHdlType));
	if (cast_handle == NULL)
		return(NOT_OKAY);
    
	cast_handle->slout_hdl = hp_slout;
    /* Used to index software processor memory spaces and vars */
	cast_handle->processor_index = i_processor_num - 1;
    /* Used to access correct physical card for hardware processors.
	 * Note this var does not neccessary correlate to processor_num
	 */
	cast_handle->board_address = i_board_address;
	cast_handle->base_address = l_base_address;

	cast_handle->program_loaded = IS_FALSE;
	cast_handle->program_running = IS_FALSE;

	cast_handle->executable = NULL;
	cast_handle->arguments = NULL;

	cast_handle->turned_off = IS_FALSE;
	cast_handle->debug_mode = IS_FALSE;
   cast_handle->debug_slout_hdl = NULL;   
    
	cast_handle->buffer_size = 0;
	cast_handle->aes_exists = IS_FALSE;

	cast_handle->softdsp_mode = i_softdsp;
	
	cast_handle->cracked_flag = IS_FALSE;
	cast_handle->dongle_exists = IS_FALSE;

   /* 
    * Check if there is a hardware card, initializes it if it exists. 
    * If there are no cards configured, don't even check.
    */
   if (i_num_cards_configured == 0)
   {    
	   cast_handle->card_exists = IS_FALSE;
   }
   else
   { 
      if (comCheckCardExists(i_board_address, &(cast_handle->card_exists)) != OKAY)
	     return(NOT_OKAY);
   }

	if( i_softdsp )
	{
	/* Initialize the software communication to DSP  */
		/* Set up comSftwr handle */
		if( comSftwrInitCPP( &(cast_handle->comSftwr_hdl) ) != OKAY)
			return(NOT_OKAY);

	   /* Sets up array of function pointers */
	   if( comSftwrInitFunctions(cast_handle->comSftwr_hdl) != OKAY )
		   return(NOT_OKAY);
		
      /* Set info about software dsp. Checks to see if a dongle is present. */
      if (com_SetSoftDspStatus((PT_HANDLE *)cast_handle) != OKAY)
		{
         (cast_handle->slout_hdl)->Error(FIRST_LINE, "Call to com_SetSoftDspStatus() failed");
         return(NOT_OKAY);
		}
	}

   *hpp_com = (PT_HANDLE *)cast_handle;

   return(OKAY);
}  

/*
 * FUNCTION: comSetBufferSize()
 * DESCRIPTION:
 *   Stores the passed buffer_size..
 */
int PT_DECLSPEC comSetBufferSize(PT_HANDLE *hp_com, long l_buffer_size)
{
   struct comHdlType *cast_handle;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);   
   
   /* Store the pointer */
   cast_handle->buffer_size = l_buffer_size;
  
   return(OKAY);
}

/*
 * FUNCTION: comCheckCardExists()
 * DESCRIPTION:
 *    Checks to see if a card exists at the passed board address.
 */
int PT_DECLSPEC comCheckCardExists(int i_board_address, int *ip_card_exists)
{
	// This version of the com module only supports software processing.
	// See the Archives folder under com for the hardware version
	*ip_card_exists = IS_FALSE;
   
	return(OKAY);
}

/*
 * FUNCTION: com_SetCardStatus()
 * DESCRIPTION:
 *    Sets the additional info about the card.
 *
 *    If NULL is passed as the cp_exe_dirpath, it does not call the executable
 *    to get the initialization info.
 */
int com_SetCardStatus(PT_HANDLE *hp_com, char *cp_exe_dirpath)
{
	// This version of the com module only supports software processing.
	// See the Archives folder under com for the hardware version
	struct comHdlType *cast_handle;
   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);   

   /* Initialize values (make it possible to run with no card. */
   cast_handle->aes_exists = IS_FALSE;
   cast_handle->main_num_samples = 0L;
   cast_handle->expanded_num_samples = 0L;

   return(OKAY);
}

/*
 * FUNCTION: com_SetSoftDspStatus()
 * DESCRIPTION:
 *    Checks to see if a card exists at the board address that the com handle
 *    was initialized at.
 *    
 */
int com_SetSoftDspStatus(PT_HANDLE *hp_com)
{
   struct comHdlType *cast_handle;

   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);   
   
   /* Initialize values (make it possible to run with no card
    * and with software dsp routines
    */
   /* Note- currently only delay uses longer sample space, so
    * initialize to delay sample count
	*/
   cast_handle->main_num_samples = DSPS_SOFT_MEM_DELAY_LENGTH;
   cast_handle->expanded_num_samples = 0L;

   if (cast_handle->turned_off)
		return(OKAY);

	// In hardware com version, would check both for dongle and card and
	// execute line below if neither was present.
	cast_handle->serial_num = COM_NO_SERIAL_NUMBER;

   return(OKAY);
}

/*
 * FUNCTION: comSoftDspLoadAndRun()
 * DESCRIPTION: Sets up software Dsp based processing function.
 *				Initializes function ptr to correct processing
 *				routine, and initializes parameters and memory.
 *          NOTE - This is the original version structured to
 *          work with the shared memory/Mdly plug-ins.
 *          With this call part of the initialization is done
 *          via a DAW call.
 *          When using the dll GUI, use 
 *             comSoftDspLoadAndRunNonShared
 */
int PT_DECLSPEC comSoftDspLoadAndRun(PT_HANDLE *hp_com, char *cp_dspname, 
				         realtype r_sampling_freq, int i_is_buffered,
						 short s_bit_width, int i_use_current_bitwidth)
{
   struct comHdlType *cast_handle;
   int init_flag;
		
   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   /* Set up index to correct init and processing functions.
    * Shared index is used by DAW dll to call correct functions.
	* Note that we must wait until we can checkout the DSP resource.
	*/
#ifdef COMSFTWR_CHECK_OUT_DSP
   while( hutsyncCheckOutDsp( (unsigned short)cast_handle->processor_index )  == 0 ) 
	   Sleep(10);
#endif

   if( comSftwrSetFunctionIndex(cast_handle->comSftwr_hdl, cp_dspname, s_bit_width, i_use_current_bitwidth) != OKAY)
		return(NOT_OKAY);

   /* If in buffered mode, initialize both parameters and memory space
    * in softdsp routine. The memory space initialization should
    * only be done in the DLL instance that will make processing
    * calls, since it will cause memory to be allocated to the
    * the calling  DLL. If not in buffered mode, processing calls
	* are being made by a DAW DLL instance, so only init the params.
    */
   if( i_is_buffered )
   {
	   init_flag = DSPS_INIT_MEMORY | DSPS_INIT_PARAMS;
   }
   else
	   init_flag = DSPS_INIT_PARAMS;

   if( comSftwrInitDspAlgorithmCPP(cast_handle->comSftwr_hdl, r_sampling_freq, 
	                                init_flag) != OKAY)
		return(NOT_OKAY);

   /* Since dsp initialization is completed, we can check DSP back in */
#ifdef COMSFTWR_CHECK_OUT_DSP
   hutsyncCheckInDsp( (unsigned short)cast_handle->processor_index );
#endif

   return(OKAY);
}

/*
 * FUNCTION: comSoftDspLoadAndRunNonShared()
 * DESCRIPTION: Sets up software Dsp based processing function.
 *				Initializes function ptr to correct processing
 *				routine, and initializes parameters and memory.
 *          NOTE - This is the newer version structured to
 *          work with the non-shared memory plug-ins.
 *
 *				IMPORTANT- this function causes memory to be allocated
 *				in the DSP module. This memory must remain allocated while
 *				any DSP processing is continuing. To free this memory, you
 *				must call the comSoftFreeDspMemory function below.
 */
int PT_DECLSPEC comSoftDspLoadAndRunNonShared(PT_HANDLE *hp_com, char *cp_dspname, 
				         realtype r_sampling_freq, int i_stereo_flag,
						   short s_bit_width)
{
   struct comHdlType *cast_handle;
   int init_flag;
	int use_current_bitwidth = IS_FALSE;
		
   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   /* Write stereo mode prior to DSP initialization call since this is required
	 * for the DFX plug-in. Note this is not a restriction for the other plug-ins
	 * since input mode typically cannot be changed during runtime in software systems.
	 */
	if (comIntWrite(hp_com, DSP_STEREO_IN_FLAG, i_stereo_flag) != OKAY)
      return(NOT_OKAY);

	/* Set up index to correct init and processing functions.*/
   if( comSftwrSetFunctionIndex(cast_handle->comSftwr_hdl, cp_dspname, s_bit_width, use_current_bitwidth) != OKAY)
		return(NOT_OKAY);

   /* Initialize both parameters and memory space. */
   init_flag = DSPS_INIT_MEMORY | DSPS_INIT_PARAMS;

   if( comSftwrInitDspAlgorithmCPP(cast_handle->comSftwr_hdl, r_sampling_freq, 
	                                init_flag) != OKAY)
		return(NOT_OKAY);

   return(OKAY);
}

/*
 * FUNCTION: comSoftDspZeroMemory()
 *		Zeros DSP signal memory. Useful to avoid glitches when song play is stopped and 
 *    then a different song is selected.
 */
int PT_DECLSPEC comSoftDspZeroMemory(PT_HANDLE *hp_com)
{
   struct comHdlType *cast_handle;
		
   cast_handle = (struct comHdlType *)hp_com;

   if (cast_handle == NULL)
      return(NOT_OKAY);

   if( comSftwrZeroDspMemoryCPP(cast_handle->comSftwr_hdl) != OKAY)
		return(NOT_OKAY);

   return(OKAY);
}

/*
 * FU0NCTION: comIsLegalSampFreq()
 * DESCRIPTION:
 *
 * Passes back whether or not the passed sampling frequency is legal.  The passed
 * i_digital_flag says if the signal is digital or not. 
 *
 */
int PT_DECLSPEC comIsLegalSampFreq(realtype r_sampfreq, int i_digital_flag, int *ip_legal)    
{
   *ip_legal = IS_FALSE;
  
   if (!i_digital_flag)
   {
      if (r_sampfreq > 0.0)
         *ip_legal = IS_TRUE;
   }
   else 
   {
      /* 
       * These are the legal digital sampling freqs.
       * NOTE: Any change here requires a parallel change in the dsp code
       *       aeswait.c
       */
      if ((r_sampfreq == 44100.0) || (r_sampfreq == 44056.0) || 
          (r_sampfreq == 48000.0))
         *ip_legal = IS_TRUE;
   }
      
   return(OKAY);
}

/*
 * FUNCTION: comFreeUp()
 * DESCRIPTION:
 *   Frees the passed com handle and sets to NULL.
 */
int PT_DECLSPEC comFreeUp(PT_HANDLE **hpp_com)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)(*hpp_com);

	if (cast_handle == NULL)
		return(OKAY);

	if (cast_handle->executable != NULL)
		free(cast_handle->executable);

	if (cast_handle->arguments != NULL)
		free(cast_handle->arguments);

	if( comSftwrFreeUp( &(cast_handle->comSftwr_hdl) ) != OKAY)
		return(NOT_OKAY);

	free(cast_handle);

	*hpp_com = NULL;

	return(OKAY);
}

/*
 * FUNCTION: comTurnOff()
 * DESCRIPTION:
 *  Allows the calling program to turn off all com activity.
 *
 */
int PT_DECLSPEC comTurnOff(PT_HANDLE *hp_com)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->turned_off = IS_TRUE;

	return(OKAY);
}   

/*
 * FUNCTION: comTurnOn()
 * DESCRIPTION:
 *   Allows the com activity to be started again after it had been turned
 *   off with comTurnOff().
 */
int PT_DECLSPEC comTurnOn(PT_HANDLE *hp_com)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->turned_off = IS_FALSE;

	return(OKAY);
}

/*
 * FUNCTION: comSetDebugMode()
 * DESCRIPTION:
 *   Sets whether debug mode is on or off.  If it is set on, then all
 *   com write calls will also be sent to the primary message handle
 *   and the passed message handle.
 */
int PT_DECLSPEC comSetDebugMode(PT_HANDLE *hp_com, int i_debug_mode,
						  CSlout *hp_debug_slout)
{
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cast_handle->debug_mode = i_debug_mode;

	if (i_debug_mode)
		cast_handle->debug_slout_hdl = hp_debug_slout;

	return(OKAY);
}


/*
 * FUNCTION: comDump()
 * DESCRIPTION:
 *  Prints out handle to the screen.
 */
int PT_DECLSPEC comDump(PT_HANDLE *hp_com)
{
/*
	struct comHdlType *cast_handle;

	cast_handle = (struct comHdlType *)hp_com;

	if (cast_handle == NULL)
		return(NOT_OKAY);

	cout << "com Handle:\n";
	if (cast_handle->slout_hdl == NULL)
		cout << "   slout_hdl is NULL\n";
	else
		cout << "   slout_hdl is not NULL\n";

	cout << "   Base address = " << cast_handle->base_address << "\n";

	cout << "   Program Loaded = " << cast_handle->program_loaded <<
	   ", Program Running = " << cast_handle->program_running << "\n";

	if (cast_handle->executable == NULL)
		cout << "   No Executable\n";
	else
		cout << "   Executable = " << cast_handle->executable << "\n";

	if (cast_handle->arguments == NULL)
		cout << "   No arguments\n";
	else
		cout << "   Arguments = " << cast_handle->arguments << "\n";

	if (cast_handle->turned_off)
		cout << "   Com handle turned off\n";
	else
		cout << "   Com handle not turned off\n";
*/

	return(OKAY);
}
