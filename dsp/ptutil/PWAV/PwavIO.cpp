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
#include <dos.h>
#include <conio.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "slout.h"
#include "mry.h"
#include "pwav.h"
#include "u_pwav.h"
#include "demodef.h"

/*
 * FUNCTION: pwavSetupReadFromInput()
 * DESCRIPTION:
 *   Sets up and reads from the input.
 *
 */
int PT_DECLSPEC pwavSetupReadFromInput(PT_HANDLE *hp_pwav, int i_num_channels_in,
						   int i_num_channels_out, realtype r_samples_per_sec,
						   int i_wave_in_dev, int i_wave_out_dev, int *ip_24_bit_flag)
{
   struct pwavHdlType *cast_handle;
   HANDLE hFormatIn;
   HANDLE hFormatOut;  
   DWORD dwFmtSize;
	int return_val, return_val2;
 
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY);

	/* In wave routines, use 44100 if 44056 is requested */
	if( r_samples_per_sec == (realtype)44056.0 )
		r_samples_per_sec = 44100.0;

   /* NOTE- at sound card, always run the same number of input
	 * channels as output channels.
	 */
	i_num_channels_in = i_num_channels_out;

   /* Get the size of the input format chunk, allocate and lock memory for it */
   dwFmtSize = sizeof(WAVEFORMATEX); 

   hFormatIn = LocalAlloc(LMEM_MOVEABLE, LOWORD(dwFmtSize));
   if (!hFormatIn)
   {
      sprintf(cast_handle->msg1, "Out of memory");
      (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
      return(NOT_OKAY);     
   }
   cast_handle->pFormatIn = (WAVEFORMATEX *)LocalLock(hFormatIn);
   if (!(cast_handle->pFormatIn))
   {
      sprintf(cast_handle->msg1, "Failed to lock memory");
      (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
      return(NOT_OKAY);     
   }   
   
   /* Allocate and lock the memory for the output format chunk */ 
   hFormatOut = LocalAlloc(LMEM_MOVEABLE, LOWORD(dwFmtSize));
   if (!hFormatOut)
   {
      sprintf(cast_handle->msg1, "Out of memory");
      (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
      return(NOT_OKAY);     
   }
   cast_handle->pFormatOut = (WAVEFORMATEX *)LocalLock(hFormatOut);
   if (!(cast_handle->pFormatOut))
   {
      sprintf(cast_handle->msg1, "Failed to lock memory");
      (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
      return(NOT_OKAY);     
   }
   
   /* Initialize formats to standard PCM */
   memset( cast_handle->pFormatIn,  0, (size_t)dwFmtSize);
   memset( cast_handle->pFormatOut, 0, (size_t)dwFmtSize);
   cast_handle->pFormatIn->wFormatTag  = WAVE_FORMAT_PCM;
   cast_handle->pFormatOut->wFormatTag = WAVE_FORMAT_PCM;

   /* Initialize format to reasonable settings */
   cast_handle->pFormatIn->cbSize  = 0;
   cast_handle->pFormatOut->cbSize = 0;

   /* NOTE- at sound card, always run the same number of input
	 * channels as output channels.
	 */
	cast_handle->pFormatIn->nChannels  = i_num_channels_in;
   cast_handle->pFormatOut->nChannels = i_num_channels_out;
   cast_handle->pFormatIn->nSamplesPerSec  = (long)r_samples_per_sec;
   cast_handle->pFormatOut->nSamplesPerSec = (long)r_samples_per_sec;

   /* Set the pass back format info */
   cast_handle->input_num_channels  = i_num_channels_in;      
   
   /* Store number of output channels */
   cast_handle->output_num_channels = i_num_channels_out;
   
	/* Query the card to see if 24 bit input is supported */
	*ip_24_bit_flag = 1;
   cast_handle->pFormatIn->wBitsPerSample  = 24;
   cast_handle->pFormatOut->wBitsPerSample = 24;
   cast_handle->pFormatIn->nBlockAlign  = i_num_channels_in * (24/8);
   cast_handle->pFormatOut->nBlockAlign = i_num_channels_out * (24/8);
   cast_handle->pFormatIn->nAvgBytesPerSec = 
	   cast_handle->pFormatIn->nBlockAlign * cast_handle->pFormatIn->nSamplesPerSec;
   cast_handle->pFormatOut->nAvgBytesPerSec = 
	   cast_handle->pFormatOut->nBlockAlign * cast_handle->pFormatOut->nSamplesPerSec;

   /* Check if waveform input and output devices support 24 bit format. Adb card apparently
	 * doesn't support 24 bit mono.
	 */
   return_val = waveInOpen(&(cast_handle->hWaveIn), i_wave_in_dev, 
                  cast_handle->pFormatIn, 0L, 
                  0L, WAVE_FORMAT_QUERY);

   return_val2 = waveOutOpen((LPHWAVEOUT)&(cast_handle->hWaveOut), i_wave_out_dev,
                   (cast_handle->pFormatOut), 0L,
                   0L, WAVE_FORMAT_QUERY);

	if ((return_val != MMSYSERR_NOERROR) || (return_val2 != MMSYSERR_NOERROR))
	{
		*ip_24_bit_flag = 0;
	   cast_handle->pFormatIn->wBitsPerSample  = 16;
		cast_handle->pFormatOut->wBitsPerSample = 16;
		cast_handle->pFormatIn->nBlockAlign  = i_num_channels_in * (16/8);
		cast_handle->pFormatOut->nBlockAlign = i_num_channels_out * (16/8);
		cast_handle->pFormatIn->nAvgBytesPerSec = 
			cast_handle->pFormatIn->nBlockAlign * cast_handle->pFormatIn->nSamplesPerSec;
		cast_handle->pFormatOut->nAvgBytesPerSec = 
			cast_handle->pFormatOut->nBlockAlign * cast_handle->pFormatOut->nSamplesPerSec;
	}

	/* Now that we have the bit resolution, set other properties */
	cast_handle->wBlockSizeIn = cast_handle->pFormatIn->nBlockAlign;
	cast_handle->wBlockSizeOut = cast_handle->pFormatOut->nBlockAlign;
	
	/* Open the waveform input device (soundcard). For this mode, we'll use
    * the recording operations to generate the callbacks
    */
   return_val = waveInOpen((LPHWAVEIN)&(cast_handle->hWaveIn), i_wave_in_dev,
                   (cast_handle->pFormatIn), 
                   (DWORD)cast_handle->master_hwnd, 
                   NULL, (DWORD)CALLBACK_WINDOW);

	if (return_val != MMSYSERR_NOERROR)
   {
      sprintf(cast_handle->msg1, "Soundcard incompatibility.");
      (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);    
      sprintf(cast_handle->msg1, "Check Sampling Frequency.");
      (cast_handle->slout_hdl)->Message(NEXT_LINE, cast_handle->msg1);    
      LocalUnlock(hFormatIn);
      LocalFree(hFormatIn);
      LocalUnlock(hFormatOut);
      LocalFree(hFormatOut);
      return(NOT_OKAY);    
   }
   
   /* Open the waveform output device, no callbacks for output */
   return_val = waveOutOpen((LPHWAVEOUT)&(cast_handle->hWaveOut), i_wave_out_dev,
                   (cast_handle->pFormatOut), 
                   (DWORD)cast_handle->master_hwnd, 
                   NULL, (DWORD)CALLBACK_NULL);

	if (return_val != MMSYSERR_NOERROR)
   {
      sprintf(cast_handle->msg1, "Soundcard incompatibility.");
      (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);    
      sprintf(cast_handle->msg1, "Check Sampling Frequency.");
      (cast_handle->slout_hdl)->Message(NEXT_LINE, cast_handle->msg1);    
      LocalUnlock(hFormatIn);
      LocalFree(hFormatIn);
      LocalUnlock(hFormatOut);
      LocalFree(hFormatOut);
      return(NOT_OKAY);    
   }

   /* Put the output device in pause for now, so buffers written to it won't play
    * until it is unpaused.
	 */
   if (waveOutPause(cast_handle->hWaveOut))
   {
      sprintf(cast_handle->msg1, "Failed to pause soundcard.");
      (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);    
      LocalUnlock(hFormatIn);
      LocalFree(hFormatIn);
      LocalUnlock(hFormatOut);
      LocalFree(hFormatOut);
      return(NOT_OKAY);    
   }
   
   if (pwav_PrepareHeaders(hp_pwav) != OKAY)
   {
      sprintf(cast_handle->msg1, "Failed to prepare wave headers.");
      (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);    
      LocalUnlock(hFormatIn);
      LocalFree(hFormatIn);
      LocalUnlock(hFormatOut);
      LocalFree(hFormatOut);
      return(NOT_OKAY);    
   }

   /* We're done with the format header, free it. */
   LocalUnlock(hFormatIn);
   LocalFree(hFormatIn);  
   LocalUnlock(hFormatOut);
   LocalFree(hFormatOut);

	/* Note - Adb card and Gina were working with only one record and
	 * playback buffer queued before we started. Upon start, another
	 * record buffer was immediately queued. Layla wouldn't work that way,
	 * so we changed to the method below where two record and playback
	 * buffers are queued before we call the start function.
	 */
   /* Queue two playback buffers. Note that the synchronization constraints of
	 * the Echo force that no devices will start until all devices are started and
	 * have a buffer queued. This forces us to queue up the buffer belows for playback
	 * even though they're not processed because we can't start the recording by itself.
	 */

	/* Zero and queue 2 playback buffers */
	memset(cast_handle->lpDataOut[cast_handle->next_output_buffer], 0,
			 (size_t)(cast_handle->buffer_length_samples * cast_handle->wBlockSizeIn) );

	if (pwavPlayIOBuffer(hp_pwav, (long *)cast_handle->lpDataOut[cast_handle->next_output_buffer]) != OKAY)
		return(NOT_OKAY);

	memset(cast_handle->lpDataOut[cast_handle->next_output_buffer], 0,
			 (size_t)(cast_handle->buffer_length_samples * cast_handle->wBlockSizeIn) );

	if (pwavPlayIOBuffer(hp_pwav, (long *)cast_handle->lpDataOut[cast_handle->next_output_buffer]) != OKAY)
		return(NOT_OKAY);

	/* Queue 2 record buffers */
   if (pwavQueueIOBuffer(hp_pwav) != OKAY)
	  return(NOT_OKAY);
   if (pwavQueueIOBuffer(hp_pwav) != OKAY)
	  return(NOT_OKAY);

   return(OKAY);
}  

/*
 * FUNCTION: pwavStartIOProcessing()
 * DESCRIPTION:
 *   Read the next block of data into to passed buffer.
 *
 */
int PT_DECLSPEC pwavStartIOProcessing(PT_HANDLE *hp_pwav)
{
	int wResult;
   struct pwavHdlType *cast_handle;
   
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY);

   /* Note that Echo/Event cards don't start any open i/o devices until
	 * all devices are started and have buffers queued, so we can't hold
	 * off on starting the output device. Need to start it with a dummy
	 * buffer.
	 */
	/* Start recording. */
   wResult = waveInStart(cast_handle->hWaveIn);
   if (wResult != OKAY)
   {
      sprintf(cast_handle->msg1, "Record start failed.");
      (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);    
	  return(NOT_OKAY);
   }

	/* Start playback. */
	wResult = waveOutRestart(cast_handle->hWaveOut);
	if (wResult != 0)
	{
	  sprintf(cast_handle->msg1, "Unable to play to soundcard");
	  (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1); 
	  sprintf(cast_handle->msg1, "Specified format not supported");
	  (cast_handle->slout_hdl)->Message(NEXT_LINE, cast_handle->msg1);         
	  return(NOT_OKAY);
	}

   return(OKAY);
}   
   
/*
 * FUNCTION: pwavReadNextIOBuffer()
 * DESCRIPTION:
 *   Read the next block of data into to passed buffer.
 *
 */
int PT_DECLSPEC pwavReadNextIOBuffer(PT_HANDLE *hp_pwav, long **lpp_buffer_data)
{
   struct pwavHdlType *cast_handle;
   
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY);

   /* Make sure legal input buffer num */
   if ((cast_handle->next_input_buffer < 0) || 
	   (cast_handle->next_input_buffer >= PWAV_NUM_INPUT_BUFFERS))
      return(NOT_OKAY);
   
	*lpp_buffer_data = (long *)(cast_handle->lpDataIn[cast_handle->next_input_buffer]);

   return(OKAY);
}   
   
/*
 * FUNCTION: pwavQueueIOBuffer()
 * DESCRIPTION:
 *   Queues buffer for use in recording. Typically buffer has just been
 *   read from. Increments buffer pointer to next buffer.
 *
 */
int PT_DECLSPEC pwavQueueIOBuffer(PT_HANDLE *hp_pwav)
{
   struct pwavHdlType *cast_handle;
   WORD wResult;
   
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY);

   /* Make sure legal input buffer num */
   if ((cast_handle->next_input_buffer < 0) || 
	   (cast_handle->next_input_buffer >= PWAV_NUM_INPUT_BUFFERS))
      return(NOT_OKAY);
   
   wResult = waveInAddBuffer(cast_handle->hWaveIn, 
                             cast_handle->lpWaveHdrIn[cast_handle->next_input_buffer],
                             sizeof(WAVEHDR));
   if (wResult != 0)
   {
      sprintf(cast_handle->msg1, "Unable to queue record buffer to soundcard");
      (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1); 
      return(NOT_OKAY);
   }

   cast_handle->next_input_buffer++;
   if ( cast_handle->next_input_buffer >= PWAV_NUM_INPUT_BUFFERS )
	   cast_handle->next_input_buffer = 0;
   
   return(OKAY);
}   
   
/*
 * FUNCTION: pwavPlayIOBuffer()
 * DESCRIPTION:
 *   Play the passed buffer.
 */
int PT_DECLSPEC pwavPlayIOBuffer(PT_HANDLE *hp_pwav, long *lp_buffer_data)
{
   struct pwavHdlType *cast_handle;
   WORD wResult;
  
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   /* Make sure legal output buffer num */
   if ((cast_handle->next_output_buffer < 0) || 
	   (cast_handle->next_output_buffer >= cast_handle->num_buffers))
      return(NOT_OKAY);

	/* Copy passed in buffer into output buffer */
	memcpy(cast_handle->lpDataOut[cast_handle->next_output_buffer], (char *)lp_buffer_data,
			 (size_t)(cast_handle->buffer_length_samples * cast_handle->wBlockSizeIn) );
	       
   /* Reset flags field (remove WHDR_DONE attribute) */
   (cast_handle->lpWaveHdrOut[cast_handle->next_output_buffer])->dwFlags 
	   = (DWORD)WHDR_PREPARED;

   wResult = waveOutWrite(cast_handle->hWaveOut, 
                          cast_handle->lpWaveHdrOut[cast_handle->next_output_buffer],
                          sizeof(WAVEHDR));
   if (wResult != 0)
   {
      sprintf(cast_handle->msg1, "Unable to play to soundcard");
      (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1); 
      sprintf(cast_handle->msg1, "Specified format not supported");
      (cast_handle->slout_hdl)->Message(NEXT_LINE, cast_handle->msg1);         
      return(NOT_OKAY);
   }

	/* Note- synchronization constraints of Echo/Event sound cards won't work with
	 * this scheme below. No i/o devices start on the Echo cards until all opened
	 * devices have buffers and are started.
	 */
   /* If requested number of buffers have been queued, start playback */
	/*
   if (cast_handle->play_start_wait_count == 0)
   {
	   wResult = waveOutRestart(cast_handle->hWaveOut);
	   if (wResult != 0)
	   {
		  sprintf(cast_handle->msg1, "Unable to play to soundcard");
		  (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1); 
		  sprintf(cast_handle->msg1, "Specified format not supported");
		  (cast_handle->slout_hdl)->Message(NEXT_LINE, cast_handle->msg1);         
		  return(NOT_OKAY);
	   }
   }
   cast_handle->play_start_wait_count--;
	*/
   
   cast_handle->next_output_buffer++;
   if ( cast_handle->next_output_buffer >= cast_handle->num_buffers )
	   cast_handle->next_output_buffer = 0;

   return(OKAY);
} 

/*
 * FUNCTION: pwavStopIO()
 * DESCRIPTION:
 *   Stop reading and processing input.
 *
 */
int PT_DECLSPEC pwavStopIO(PT_HANDLE *hp_pwav)
{
   struct pwavHdlType *cast_handle;
   int retval;
   int index;

   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY); 
      
   cast_handle->bytes_left_to_read = 0L;   
   
	/* ddrec example does waveXXXReset, then unprepares headers, then does WaveXXClose */
	/* Stops input, marks all pending buffers as done and "returns them to the application" */
   retval = waveInReset(cast_handle->hWaveIn);
   retval = waveOutReset(cast_handle->hWaveOut);

   for (index=0; index < PWAV_NUM_INPUT_BUFFERS; index++)
   {
      waveInUnprepareHeader(cast_handle->hWaveIn, 
                             cast_handle->lpWaveHdrIn[index], 
                             sizeof(WAVEHDR));
   }

   for (index=0; index < cast_handle->num_buffers; index++)
   {
      waveOutUnprepareHeader(cast_handle->hWaveOut, 
                             cast_handle->lpWaveHdrOut[index], 
                             sizeof(WAVEHDR));
   }                     
   
   retval = waveInClose(cast_handle->hWaveIn);
   retval = waveOutClose(cast_handle->hWaveOut);
   
   return(OKAY);
}
 
/*
 * FUNCTION: pwav_PrepareHeaders()
 * DESCRIPTION:
 */
int pwav_PrepareHeaders(PT_HANDLE *hp_pwav)
{
   struct pwavHdlType *cast_handle;
   int index;
   int error_msg;

   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY);

   for (index=0; index < PWAV_NUM_INPUT_BUFFERS; index++)
   {
	   cast_handle->lpWaveHdrIn[index]->dwBufferLength 
			= cast_handle->buffer_length_samples * cast_handle->pFormatIn->nBlockAlign;
	   cast_handle->lpWaveHdrIn[index]->dwFlags = 0; 

      error_msg = waveInPrepareHeader(cast_handle->hWaveIn, 
		                       (cast_handle->lpWaveHdrIn)[index], sizeof(WAVEHDR));
	   if (error_msg != MMSYSERR_NOERROR)
	   {
          sprintf(cast_handle->msg1, "Error %d from PrepareHeader.", error_msg);
          (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1); 
	      return(NOT_OKAY);
	   }
   }  
   
   for (index=0; index < cast_handle->num_buffers; index++)
   {
	   cast_handle->lpWaveHdrOut[index]->dwBufferLength 
			= cast_handle->buffer_length_samples * cast_handle->pFormatOut->nBlockAlign;
	   cast_handle->lpWaveHdrOut[index]->dwFlags = 0; 

      error_msg = waveOutPrepareHeader(cast_handle->hWaveOut, 
		                       (cast_handle->lpWaveHdrOut)[index], sizeof(WAVEHDR));
	   if (error_msg != MMSYSERR_NOERROR)
	   {
          sprintf(cast_handle->msg1, "Error %d from PrepareHeader.", error_msg);
          (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1); 
	      return(NOT_OKAY);
	   }
   }  

   return(OKAY);
}

/*
 * FUNCTION: pwav_AllocateDataBuffer()
 * DESCRIPTION:
 *
 * Allocates, locks and initializes wave file buffers.
 */
int pwav_AllocateDataBuffer(PT_HANDLE *hp_pwav, HANDLE *hpData, HPSTR *lppData, HANDLE *hpWaveHdr, LPWAVEHDR *lppWaveHdr, int *ip_headerFlag)
{
   struct pwavHdlType *cast_handle;
  
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY);
  
   *hpData = NULL;
   *lppData = NULL;
   *hpWaveHdr = NULL;
   *lppWaveHdr = NULL;  
   *ip_headerFlag = IS_FALSE;
  
   *hpData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, PWAV_MAX_WAVE_BUFFER_SIZE);
  
   if (!*hpData)
   { 
     sprintf(cast_handle->msg1, "Out of memory");
     (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
     return(NOT_OKAY);    
   }
  
   *lppData = (HPSTR)GlobalLock(*hpData);

   if (!*lppData)
   {
     sprintf(cast_handle->msg1, "Failed to lock memory for data chunk");      
     (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
     return(NOT_OKAY); 
   }

   /* 
    * Allocate waveform data header.  
    * The WAVEHDR must be globally allocated and locked.
    */
   *hpWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (DWORD)sizeof(WAVEHDR));
   if (!*hpWaveHdr)
   {
     sprintf(cast_handle->msg1, "Out of memory for header");
     (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1); 
     return(NOT_OKAY);
   }
   *lppWaveHdr = (LPWAVEHDR)GlobalLock(*hpWaveHdr);
   if (!*lppWaveHdr)
   {
     sprintf(cast_handle->msg1, "Failed to lock header memory");
     (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
     return(NOT_OKAY); 
   }

   /* Zero out wave header */
   memset(*lppWaveHdr, 0, sizeof(WAVEHDR));

   /* Set size of allocated buffer */
   (*lppWaveHdr)->dwBufferLength = 0;

   /* Reset flags */
   (*lppWaveHdr)->dwFlags = 0;

   /* Set data pointer */
   (*lppWaveHdr)->lpData = *lppData;

   return(OKAY);
}