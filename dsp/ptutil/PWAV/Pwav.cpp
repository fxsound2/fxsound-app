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
 * FUNCTION: pwavNew()
 * DESCRIPTION:
 *  Allocates and initializes the passed pwav handle to have the passed number of points
 *
 *  !!!!!!!!!!!!! WARNING - SUPER HACK !!!!!!!!!!!!!!!
 *  The master_hwnd is passed as a WORD even though it is an HWND. The reason
 *  is because the linker can't handle passing the HWND.
 *
 *  For the 32-bit port try to use HWND.  If that does not work, use a DWORD.  (32-bits)
 *
 *  I called Microsoft and they told me to do this.     
 *
 */
int PT_DECLSPEC pwavNew(PT_HANDLE **hpp_pwav, CSlout *hp_slout, 
            long l_buffer_length_samples, DWORD master_hwnd,
            int i_num_buffers)
{
   struct pwavHdlType *cast_handle;  
	
   /* Allocate the handle */
   cast_handle = (struct pwavHdlType *)calloc(1, 
                  sizeof(struct pwavHdlType));
   if (cast_handle == NULL)
		return(NOT_OKAY);  

   /* Store the slout */
   cast_handle->slout_hdl = hp_slout;
    
   /* Check the buffer length for legal value */
   if (l_buffer_length_samples <= 0)
      return(NOT_OKAY);  
   if (i_num_buffers < 0)
      return(NOT_OKAY);  
       
   /* Store the passed parameters */
   cast_handle->buffer_length_samples = l_buffer_length_samples;
   cast_handle->master_hwnd = (HWND)master_hwnd;  
   cast_handle->hWaveIn = NULL;
   cast_handle->hWaveOut = NULL;
 
   /* Initialize data */
   cast_handle->hmmio_in = NULL;
   cast_handle->hmmio_out = NULL;

	cast_handle->float_buffer = NULL;
   
   cast_handle->num_buffers = i_num_buffers;
   cast_handle->next_input_buffer = 0;
   cast_handle->next_output_buffer = 0;
   cast_handle->io_24_bit_flag = 0;

   /* Allocate and initialize the buffers */
   if (pwav_AllocateBuffers((PT_HANDLE *)cast_handle) != OKAY)
	   return(NOT_OKAY);
   
   *hpp_pwav = (PT_HANDLE *)cast_handle;		                                               
		                                               
   return(OKAY);
}    
 
/*
 * FUNCTION: pwav_AllocateBuffers()
 * DESCRIPTION:
 *  Allocate and initialize the buffers
 */
int pwav_AllocateBuffers(PT_HANDLE *hp_pwav)
{
   struct pwavHdlType *cast_handle;
   int index;

   cast_handle = (struct pwavHdlType *)(hp_pwav);
 
   /* Allocate the arrays of pointers */
   cast_handle->hDataIn = (HANDLE *)calloc(PWAV_NUM_INPUT_BUFFERS, 
	                              sizeof(HANDLE));
   if (cast_handle->hDataIn == NULL)
		return(NOT_OKAY);  
   cast_handle->hDataOut = (HANDLE *)calloc(cast_handle->num_buffers, 
	                              sizeof(HANDLE));
   if (cast_handle->hDataOut == NULL)
		return(NOT_OKAY);

   cast_handle->lpDataIn = (HPSTR *)calloc(PWAV_NUM_INPUT_BUFFERS, 
	                              sizeof(HPSTR));
   if (cast_handle->lpDataIn == NULL)
		return(NOT_OKAY);
   cast_handle->lpDataOut = (HPSTR *)calloc(cast_handle->num_buffers, 
	                              sizeof(HPSTR));
   if (cast_handle->lpDataOut == NULL)
		return(NOT_OKAY);

   cast_handle->hWaveHdrIn = (HANDLE *)calloc(PWAV_NUM_INPUT_BUFFERS, 
	                                 sizeof(HANDLE));
   if (cast_handle->hWaveHdrIn == NULL)
		return(NOT_OKAY);
   cast_handle->hWaveHdrOut = (HANDLE *)calloc(cast_handle->num_buffers, 
	                                 sizeof(HANDLE));
   if (cast_handle->hWaveHdrOut == NULL)
		return(NOT_OKAY);

   cast_handle->lpWaveHdrIn = (LPWAVEHDR *)calloc(PWAV_NUM_INPUT_BUFFERS,
	                                 sizeof(LPWAVEHDR));
   if (cast_handle->lpWaveHdrIn == NULL)
		return(NOT_OKAY);
   cast_handle->lpWaveHdrOut = (LPWAVEHDR *)calloc(cast_handle->num_buffers,
	                                 sizeof(LPWAVEHDR));
   if (cast_handle->lpWaveHdrOut == NULL)
		return(NOT_OKAY);
	
   cast_handle->headerFlagsIn = (int *)calloc(PWAV_NUM_INPUT_BUFFERS,
		                              sizeof(int));
   cast_handle->headerFlagsOut = (int *)calloc(cast_handle->num_buffers,
		                              sizeof(int));

   cast_handle->float_buffer = (realtype *)calloc(2 * PWAV_MAX_BUFFER_SIZE,
		                              sizeof(realtype));
    
   /* Allocate each data buffer */
   for (index = 0; index < PWAV_NUM_INPUT_BUFFERS; index++)
   {
	   if (pwav_AllocateDataBuffer(hp_pwav,
		                          &(cast_handle->hDataIn[index]),
								  &(cast_handle->lpDataIn[index]),
		                          &(cast_handle->hWaveHdrIn[index]),
								  &(cast_handle->lpWaveHdrIn[index]),
								  &(cast_handle->headerFlagsIn[index])) != OKAY)
			return(NOT_OKAY);

   }
   /* Initialize the output wave buffer data */
   for (index = 0; index < cast_handle->num_buffers; index++)
   {
	   if (pwav_AllocateDataBuffer(hp_pwav,
		                          &(cast_handle->hDataOut[index]),
								  &(cast_handle->lpDataOut[index]),
		                          &(cast_handle->hWaveHdrOut[index]),
								  &(cast_handle->lpWaveHdrOut[index]),
								  &(cast_handle->headerFlagsOut[index])) != OKAY)
			return(NOT_OKAY);

   }

   return(OKAY);
}

/*
 * FUNCTION: pwavResetBufferSize()
 * DESCRIPTION:
 *   Changes the number of buffer and the buffer length.
 *
 */
int PT_DECLSPEC pwavResetBufferSize(PT_HANDLE *hp_pwav, long l_buffer_length_samples, 
						int i_num_buffers)
{
   struct pwavHdlType *cast_handle;

   cast_handle = (struct pwavHdlType *)(hp_pwav);

	/* If handle has not been allocated yet, don't try to change its values.
	 * The changed values will be filled in the first time its allocated.
	 */
	if( cast_handle == NULL)
		return(OKAY);
   	
   /* Check if anything has changed (if not, do nothing) */
   if ((l_buffer_length_samples == cast_handle->buffer_length_samples) &&
	   (i_num_buffers == cast_handle->num_buffers))
      return(OKAY);
   
   /* Check the buffer length for legal value */
   if (l_buffer_length_samples <= 0)
      return(NOT_OKAY); 
   if (i_num_buffers < 0)
      return(NOT_OKAY);
       
   /* Free the old buffers and reallocate them */
	/* Using max size allocated buffers now, just change sample length size.
   if (pwav_FreeBuffers(hp_pwav) != OKAY)
	   return(NOT_OKAY);
   if (pwav_AllocateBuffers((PT_HANDLE *)cast_handle) != OKAY)
	   return(NOT_OKAY);
	 */

   /* Store the passed parameters */
   cast_handle->buffer_length_samples = l_buffer_length_samples;
   cast_handle->num_buffers = i_num_buffers;   
   
   return(OKAY);
}    
			
/*
 * FUNCTION: pwavReadHeader()
 * DESCRIPTION:
 *   Read the header info of the passed .wav file.
 *
 *   The passed cp_filepath_out is the output file.  If it is
 *   NULL than it does not save to an output file.
 */
int PT_DECLSPEC pwavReadHeader(PT_HANDLE *hp_pwav, 
						 wchar_t *wcp_filepath_in, wchar_t *wcp_filepath_out,
                   int i_num_channels_out, int i_waveout_dev, 
                   int *ip_num_channels_in, long *lp_size_samples, 
                   long *lp_samples_per_sec, int *ip_bits_per_sample)
{
   struct pwavHdlType *cast_handle;
   HANDLE hFormatIn;
   HANDLE hFormatOut;  
   MMCKINFO mmckinfoParent;
   MMCKINFO mmckinfoSubchunk;
   DWORD dwFmtSize;
 
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY);
   
   /* Set the save mode */
   if (wcp_filepath_out == NULL)
      cast_handle->save_mode = IS_FALSE;
   else 
      cast_handle->save_mode = IS_TRUE;
   
   /* Open the given file for reading */
   if (!(cast_handle->hmmio_in = 
      mmioOpenW(wcp_filepath_in, NULL, MMIO_READ | MMIO_ALLOCBUF)))
   {
      swprintf(cast_handle->wcp_msg1, L"Unable to open input file");
      (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);    
      return(NOT_OKAY);
   }
      
   /* Open the output file for writing */  
   if (cast_handle->save_mode)
   {     
	  if (!(cast_handle->hmmio_out = 
         mmioOpenW(wcp_filepath_out, NULL, MMIO_ALLOCBUF | MMIO_WRITE | MMIO_CREATE)))
      {
         swprintf(cast_handle->wcp_msg1, L"Unable to open output file.");
         (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);    
         swprintf(cast_handle->wcp_msg1, L"   File is probably in use by");
         (cast_handle->slout_hdl)->Message_Wide(NEXT_LINE, cast_handle->wcp_msg1);   
         swprintf(cast_handle->wcp_msg1, L"   another application.");
         (cast_handle->slout_hdl)->Message_Wide(NEXT_LINE, cast_handle->wcp_msg1);             
         return(NOT_OKAY);
      }
   }     
   
   /* 
    * Locate a 'RIFF' chunk with a 'WAVE' form type to make sure it is 
    * a WAVE file.
    */
   (mmckinfoParent).fccType = mmioFOURCC('W', 'A', 'V', 'E');
   if (mmioDescend(cast_handle->hmmio_in, (LPMMCKINFO) &(mmckinfoParent), 
                   NULL, MMIO_FINDRIFF))
   {
      swprintf(cast_handle->wcp_msg1, L"Illegal WAVE file");
      (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
      return(NOT_OKAY); 
   }
   
   /* 
    * Now find the format chunk (form type 'fmt').  It should be a subchunk of the 
    * 'RIFF' parent chunk.
    */
   (mmckinfoSubchunk).ckid = mmioFOURCC('f', 'm', 't', ' ');
   if (mmioDescend(cast_handle->hmmio_in, &(mmckinfoSubchunk),
                   &(mmckinfoParent), MMIO_FINDCHUNK))
   {
      swprintf(cast_handle->wcp_msg1, L"WAVE file is corrupted");
      (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
      return(NOT_OKAY);   
   }
   
   /* Get the size of the input format chunk, allocate and lock memory for it */
   dwFmtSize = (mmckinfoSubchunk).cksize; 
   hFormatIn = LocalAlloc(LMEM_MOVEABLE, LOWORD(dwFmtSize));
   if (!hFormatIn)
   {
      swprintf(cast_handle->wcp_msg1, L"Out of memory");
      (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
      return(NOT_OKAY);     
   }
   cast_handle->pFormatIn = (WAVEFORMATEX *)LocalLock(hFormatIn);
   if (!(cast_handle->pFormatIn))
   {
      swprintf(cast_handle->wcp_msg1, L"Failed to lock memory");
      (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
      return(NOT_OKAY);     
   }   
   
   /* Allocate and lock the memory for the output format chunk */ 
   hFormatOut = LocalAlloc(LMEM_MOVEABLE, LOWORD(dwFmtSize));
   if (!hFormatOut)
   {
      swprintf(cast_handle->wcp_msg1, L"Out of memory");
      (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
      return(NOT_OKAY);     
   }
   cast_handle->pFormatOut = (WAVEFORMATEX *)LocalLock(hFormatOut);
   if (!(cast_handle->pFormatOut))
   {
      swprintf(cast_handle->wcp_msg1, L"Failed to lock memory");
      (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);
      return(NOT_OKAY);     
   }
   
   /* Read the format chunk */
   if (mmioRead(cast_handle->hmmio_in, (HPSTR)cast_handle->pFormatIn, 
                dwFmtSize) != (LONG) dwFmtSize)
   {
      swprintf(cast_handle->wcp_msg1, L"Failed to read format chunk");
      (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);    
      LocalUnlock(hFormatIn);
      LocalFree(hFormatIn);   
      LocalUnlock(hFormatOut);
      LocalFree(hFormatOut);      
      return(NOT_OKAY);        
   }
   
   /* Make sure it's a PCM file */
   if ((cast_handle->pFormatIn)->wFormatTag != WAVE_FORMAT_PCM)
	{
      swprintf(cast_handle->wcp_msg1, L"Not a PCM file");
      (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);    
      LocalUnlock(hFormatIn);
      LocalFree(hFormatIn); 
      LocalUnlock(hFormatOut);
      LocalFree(hFormatOut);      
      return(NOT_OKAY);    
	}

   /* Set the pass back format info */
   cast_handle->input_num_channels = (int)((cast_handle->pFormatIn)->nChannels);      
   *ip_num_channels_in = cast_handle->input_num_channels;   
   *lp_samples_per_sec = (long)((cast_handle->pFormatIn)->nSamplesPerSec); 
   
   /* Store number of output channels */
   cast_handle->output_num_channels = i_num_channels_out;
   /*
    * TEMPORARY HACK!!! We will force number of input channels to equal number
	* of output channels.
    */
   cast_handle->output_num_channels = cast_handle->input_num_channels;
   
   /* Set up the output format */  
   (cast_handle->pFormatOut)->wBitsPerSample =
      (cast_handle->pFormatIn)->wBitsPerSample; 
   (cast_handle->pFormatOut)->wFormatTag = (cast_handle->pFormatIn)->wFormatTag;
   (cast_handle->pFormatOut)->nChannels = cast_handle->output_num_channels;
   (cast_handle->pFormatOut)->nSamplesPerSec =(cast_handle->pFormatIn)->nSamplesPerSec;
   (cast_handle->pFormatOut)->nBlockAlign = 
      ((cast_handle->output_num_channels) * (cast_handle->pFormatOut)->wBitsPerSample)/8;  
   (cast_handle->pFormatOut)->nAvgBytesPerSec = 
      (cast_handle->pFormatOut)->nSamplesPerSec * (cast_handle->pFormatOut)->nBlockAlign;  

   /* Set up the sample factors (sample to byte calculations */
   cast_handle->wBlockSizeIn = (cast_handle->pFormatIn)->nBlockAlign;
   cast_handle->wBlockSizeOut = (cast_handle->pFormatOut)->nBlockAlign;

	*ip_bits_per_sample = (cast_handle->pFormatIn)->wBitsPerSample;

   /* Ascend out of the format subchunk */
   mmioAscend(cast_handle->hmmio_in, &(mmckinfoSubchunk), 0);
   
   /* Find the data subchunk */  
   (mmckinfoSubchunk).ckid = mmioFOURCC('d', 'a', 't', 'a');
   if (mmioDescend(cast_handle->hmmio_in, &(mmckinfoSubchunk), 
                  &(mmckinfoParent), MMIO_FINDCHUNK))
   {
      swprintf(cast_handle->wcp_msg1, L"No data chunk");
      (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);    
      LocalUnlock(hFormatIn);
      LocalFree(hFormatIn);    
      LocalUnlock(hFormatOut);
      LocalFree(hFormatOut);      
      return(NOT_OKAY);      
   }
   
   /* Get the size of the data subchunk */
   cast_handle->dwDataSize = (mmckinfoSubchunk).cksize;
   
   if (cast_handle->dwDataSize == 0L)
   {
      swprintf(cast_handle->wcp_msg1, L"The data chunk has no data");
      (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);    
      LocalUnlock(hFormatIn);
      LocalFree(hFormatIn);   
      LocalUnlock(hFormatOut);
      LocalFree(hFormatOut);      
      return(NOT_OKAY);        
   }

   /* Set the pass back size */
   cast_handle->bytes_left_to_read = (long)(cast_handle->dwDataSize);
   *lp_size_samples = cast_handle->bytes_left_to_read / cast_handle->wBlockSizeIn;
      
   /* Create the output file RIFF chunk of form type wave */  
   if (cast_handle->save_mode)
   {
      (cast_handle->ckOutRiff).fccType = mmioFOURCC('W', 'A', 'V', 'E');
      /* Let the function determine the size. */
      (cast_handle->ckOutRiff).cksize = 0L; 
      if (mmioCreateChunk(cast_handle->hmmio_out, &(cast_handle->ckOutRiff), 
                          MMIO_CREATERIFF) != 0)
      {
         swprintf(cast_handle->wcp_msg1, L"Failed to create wave chunk");
         (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);    
         LocalUnlock(hFormatIn);
         LocalFree(hFormatIn); 
         LocalUnlock(hFormatOut);
         LocalFree(hFormatOut);         
         return(NOT_OKAY);      
      }

      /* 
       * We are now dwscended into the 'RIFF' chunk we just created.
       * Now create the 'fmt' chunk.  Since we know the size of this chunk,
       * specify it in the MMCKINFO struct so MMIO doesn't have to seek back
       * and set the chunk size after ascending from the chunk.
       */
      (cast_handle->ckOut).ckid = mmioFOURCC('f', 'm', 't', ' ');
      (cast_handle->ckOut).cksize = sizeof(WAVEFORMATEX);
      if (mmioCreateChunk(cast_handle->hmmio_out, &(cast_handle->ckOut), 0) != 0)
      {
         swprintf(cast_handle->wcp_msg1, L"Failed to create format chunk");
         (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);    
         LocalUnlock(hFormatIn);
         LocalFree(hFormatIn);
         LocalUnlock(hFormatOut);
         LocalFree(hFormatOut);         
         return(NOT_OKAY);        
      }
       
      /* Set the output number of channels for writing */  
      (cast_handle->pFormatOut)->nChannels = cast_handle->output_num_channels;
       
      /* Write the WAVEFORMATEX structure to the 'fmt' chunk */
      if (mmioWrite(cast_handle->hmmio_out, (HPSTR)(cast_handle->pFormatOut), 
                 sizeof(WAVEFORMATEX)) != sizeof(WAVEFORMATEX))
      {
         swprintf(cast_handle->wcp_msg1, L"Failed to write format chunk");
         (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);    
         LocalUnlock(hFormatIn);
         LocalFree(hFormatIn);     
         LocalUnlock(hFormatOut);
         LocalFree(hFormatOut);         
         return(NOT_OKAY);        
      }
   
      /* Ascend out of the 'fmt' chunk, back into the 'RIFF' chunk. */
      if (mmioAscend(cast_handle->hmmio_out, &(cast_handle->ckOut), 0) != 0)
      {
         swprintf(cast_handle->wcp_msg1, L"Unable to write file");
         (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);    
         LocalUnlock(hFormatIn);
         LocalFree(hFormatIn); 
         LocalUnlock(hFormatOut);
         LocalFree(hFormatOut);
         return(NOT_OKAY);      
      }
      
      /* Create the data chunk that holds the waveform samples */
      (cast_handle->ckOut).ckid = mmioFOURCC('d', 'a', 't', 'a');
      /* Let the funciton determine the size */
      (cast_handle->ckOut).cksize = 0L;
      if (mmioCreateChunk(cast_handle->hmmio_out, &(cast_handle->ckOut), 0) != 0)
      {
         swprintf(cast_handle->wcp_msg1, L"Unable to create data chunk");
         (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);    
         LocalUnlock(hFormatIn);
         LocalFree(hFormatIn); 
         LocalUnlock(hFormatOut);
         LocalFree(hFormatOut);
         return(NOT_OKAY);   
      }
   } 
   
   /* Make sure a waveform output device supports this format */
	if (waveOutOpen(&(cast_handle->hWaveOut), i_waveout_dev, 
                  cast_handle->pFormatOut, 0L, 
                  0L, WAVE_FORMAT_QUERY))
	{
      swprintf(cast_handle->wcp_msg1, L"No soundcard support.");
      (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1); 
      swprintf(cast_handle->wcp_msg1, L"   A soundcard must be installed");
      (cast_handle->slout_hdl)->Message_Wide(NEXT_LINE, cast_handle->wcp_msg1);  
      swprintf(cast_handle->wcp_msg1, L"   which can play the output.");
      (cast_handle->slout_hdl)->Message_Wide(NEXT_LINE, cast_handle->wcp_msg1);               
      LocalUnlock(hFormatIn);
      LocalFree(hFormatIn);   
      LocalUnlock(hFormatOut);
      LocalFree(hFormatOut);      
      return(NOT_OKAY);
	}
         
   /* Open the waveform output device (soundcard) */
	if (cast_handle->master_hwnd != NULL)
	{
      if (waveOutOpen((LPHWAVEOUT)&(cast_handle->hWaveOut), i_waveout_dev,
                      (cast_handle->pFormatOut), 
                      (DWORD)cast_handle->master_hwnd, 
                      0L, (DWORD)CALLBACK_WINDOW))
		{
         swprintf(cast_handle->wcp_msg1, L"Failed to open soundcard.");
         (cast_handle->slout_hdl)->Message_Wide(FIRST_LINE, cast_handle->wcp_msg1);    
         LocalUnlock(hFormatIn);
         LocalFree(hFormatIn);
         LocalUnlock(hFormatOut);
         LocalFree(hFormatOut);
         return(NOT_OKAY);    
		}
   }
		
   /* We're done with the format header, free it. */
   LocalUnlock(hFormatIn);
   LocalFree(hFormatIn);  
   LocalUnlock(hFormatOut);
   LocalFree(hFormatOut);

   return(OKAY);
}  

/*
 * FUNCTION: pwavReadNextBuffer()
 * DESCRIPTION:
 *   Read the next block of data into to passed buffer.
 *   The i_first_time flag says if it is the first time this buffer is being
 *   read.  It determines if that buffer's header should be unprepared.
 *
 *   It also passes back the total number of samples left to read.
 */
int PT_DECLSPEC pwavReadNextBuffer(PT_HANDLE *hp_pwav, int i_buffer_num,
                       long **lpp_buffer_data, long *lp_buffer_length_samples,
                       long *lp_samples_left, int i_first_time, int i_prepare_header)
{
   struct pwavHdlType *cast_handle;
   long num_bytes_to_read;
   long read_buffer_length_bytes;
   long read_return_bytes;
   
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY);

   /* Make sure legal buffer num */
   if ((i_buffer_num < 0) || (i_buffer_num >= cast_handle->num_buffers))
      return(NOT_OKAY);
   
   /* 
    * Figure out how many samples to read.  Either a full buffer, or less if
    * it is the last read. 
    */
	read_buffer_length_bytes = cast_handle->buffer_length_samples * cast_handle->wBlockSizeIn;

   if (cast_handle->bytes_left_to_read < read_buffer_length_bytes)
      num_bytes_to_read = cast_handle->bytes_left_to_read;
   else
      num_bytes_to_read = read_buffer_length_bytes;     

   /* Read the waveform data subchunk */
   if (num_bytes_to_read == 0)
   {
      *lp_buffer_length_samples = (long)0L;
      *lpp_buffer_data = NULL;      
   }
   else
   {
      if (!i_first_time && i_prepare_header)
      {
         waveOutUnprepareHeader(cast_handle->hWaveOut, 
                                cast_handle->lpWaveHdrOut[i_buffer_num], 
                                sizeof(WAVEHDR));
		 (cast_handle->headerFlagsOut)[i_buffer_num] = IS_FALSE;
      }
      read_return_bytes = 
          (long)(mmioRead(cast_handle->hmmio_in, 
                         (HPSTR)(cast_handle->lpDataOut[i_buffer_num]), 
                         num_bytes_to_read));
      *lpp_buffer_data = (long *)(cast_handle->lpDataOut[i_buffer_num]);
   }
  
   *lp_buffer_length_samples = num_bytes_to_read / cast_handle->wBlockSizeIn;
   
   /* calculate number of bytes left to read */
   cast_handle->bytes_left_to_read = 
      cast_handle->bytes_left_to_read - num_bytes_to_read;    

   /* Prepare the buffer */
	if (i_prepare_header)
	{
      (cast_handle->lpWaveHdrOut[i_buffer_num])->lpData = 
         cast_handle->lpDataOut[i_buffer_num];
/*  
      (cast_handle->lpWaveHdrOut[i_buffer_num])->dwBufferLength = num_bytes_to_read;

*/
      (cast_handle->lpWaveHdrOut[i_buffer_num])->dwBufferLength = read_buffer_length_bytes;
      (cast_handle->lpWaveHdrOut[i_buffer_num])->dwFlags = 0L;
      (cast_handle->lpWaveHdrOut[i_buffer_num])->dwLoops = 0L; 
      (cast_handle->lpWaveHdrOut[i_buffer_num])->dwUser = 0L;
      (cast_handle->lpWaveHdrOut[i_buffer_num])->reserved = 0L;   
   
      if (waveOutPrepareHeader(cast_handle->hWaveOut, 
                               cast_handle->lpWaveHdrOut[i_buffer_num], 
                               sizeof(WAVEHDR)))
		{
         sprintf(cast_handle->msg1, "Unable to prepare wave header");
         (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
         return(NOT_OKAY);
		}
      (cast_handle->headerFlagsOut)[i_buffer_num] = IS_TRUE;
   }

   /* Pass back number of samples left to read */
   *lp_samples_left = 
      cast_handle->bytes_left_to_read / cast_handle->wBlockSizeIn;
   
   return(OKAY);
}   

/*
 * FUNCTION: pwavGetBuffer()
 * DESCRIPTION:
 *   
 *  Pass back pointer to the specified buffer
 */
int PT_DECLSPEC pwavGetBuffer(PT_HANDLE *hp_pwav, 
                  int i_buffer_num, 
                  long **lpp_buffer_data)
                       
{
   struct pwavHdlType *cast_handle;
   
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY);

   *lpp_buffer_data = (long *)(cast_handle->lpDataOut[i_buffer_num]); 
   
   return(OKAY);
}      
   
/*
 * FUNCTION: pwavGetTempFloatBuffer()
 * DESCRIPTION:
 *   
 *  Pass back pointer to the specified buffer
 */
int PT_DECLSPEC pwavGetTempFloatBuffer(PT_HANDLE *hp_pwav, realtype **rpp_buffer_data)
{
   struct pwavHdlType *cast_handle;
   
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY);

   *rpp_buffer_data = cast_handle->float_buffer; 
   
   return(OKAY);
}      
   
/*
 * FUNCTION: pwavGetDoneReading()
 * DESCRIPTION:
 *   Pass back if it is done reading, meaning that there are no more bytes
 *   of data to read.
 */
int PT_DECLSPEC pwavGetDoneReading(PT_HANDLE *hp_pwav, int *ip_done)
{
   struct pwavHdlType *cast_handle;
   
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY);  
      
   if (cast_handle->bytes_left_to_read == 0L)
      *ip_done = IS_TRUE;      
   else
      *ip_done = IS_FALSE;   

   return(OKAY);
}
 
/*
 * FUNCTION: pwavPlayBuffer()
 * DESCRIPTION:
 *   Play the passed buffer for the passed length.
 */
int PT_DECLSPEC pwavPlayBuffer(PT_HANDLE *hp_pwav, int i_buffer_num, long l_length_samples)
{
   struct pwavHdlType *cast_handle;
   WORD wResult;
   long length_bytes;
  
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   /* Make sure the length is legal to play */
   if (l_length_samples <= 0L)
      return(OKAY);
   
   /* Calculate the buffer length in bytes */
   length_bytes = l_length_samples * cast_handle->wBlockSizeOut;

   /* If in save mode, also save the buffer */  
   if (cast_handle->save_mode)
   {
      mmioWrite(cast_handle->hmmio_out, (HPSTR)cast_handle->lpDataOut[i_buffer_num], 
               (long)length_bytes);
   } 
     
	/* For debugging
		DWORD flags = cast_handle->lpWaveHdrOut[i_buffer_num]->dwFlags;
		DWORD buflen = cast_handle->lpWaveHdrOut[i_buffer_num]->dwBufferLength;
		DWORD loops = cast_handle->lpWaveHdrOut[i_buffer_num]->dwLoops;
		LPSTR lpdata = cast_handle->lpWaveHdrOut[i_buffer_num]->lpData;
	 */

   /* Then the data block can be sent to the output device */
   cast_handle->lpWaveHdrOut[i_buffer_num]->dwBufferLength = length_bytes;

   wResult = waveOutWrite(cast_handle->hWaveOut, 
                          cast_handle->lpWaveHdrOut[i_buffer_num],
                          sizeof(WAVEHDR));
   if (wResult != MMSYSERR_NOERROR )
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
 * FUNCTION: pwavStop()
 * DESCRIPTION:
 *   Stop reading the .wav file.
 *
 *   The i_complete_play flag says if the wave playing completed normally,
 *   and therefore the headers should be unprepared.
 */
int PT_DECLSPEC pwavStop(PT_HANDLE *hp_pwav, int i_complete_play)
{
   struct pwavHdlType *cast_handle;
   int retval;
   int index;

   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY); 
      
   cast_handle->bytes_left_to_read = 0L;   
   
   /* If in save mode, do last tasks and close */
   if (cast_handle->save_mode)
   {
      if (cast_handle->hmmio_out != NULL)
      {
         /* 
          * Ascend the output file out of the 'data' chunk.
          * This will cause the chunk size of the 'data' chunk to be written.
          */
         if (mmioAscend(cast_handle->hmmio_out, &(cast_handle->ckOut), 0) != 0)
            return(NOT_OKAY);
         
         /* 
          * Ascend the output file out of the 'RIFF' chunk.
          * This will cause the chunk size of the 'RIFF' chunk to be written.
          */
         if (mmioAscend(cast_handle->hmmio_out, &(cast_handle->ckOutRiff), 0) != 0)
            return(NOT_OKAY);     
         
         /* Close */
         mmioClose(cast_handle->hmmio_out, 0);
      }
   }    
   
   if (cast_handle->hmmio_in != NULL)
      mmioClose(cast_handle->hmmio_in, 0);    
      
   if (cast_handle->hmmio_out != NULL)
      mmioClose(cast_handle->hmmio_out, 0);          

   retval = waveOutReset(cast_handle->hWaveOut);
      
   for (index=0; index < cast_handle->num_buffers; index++)
   {
      if ((cast_handle->headerFlagsOut)[index])
	  {
         waveOutUnprepareHeader(cast_handle->hWaveOut, 
                                cast_handle->lpWaveHdrOut[index], 
                                sizeof(WAVEHDR));
	  }
   }                     
   
   retval = waveOutClose(cast_handle->hWaveOut);
   
   return(OKAY);
}
 
/*
 * FUNCTION: pwavCheckLegalFiles()
 * DESCRIPTION:
 *  Check if the passed input and output files are legal.
 */
int PT_DECLSPEC pwavCheckLegalFiles(PT_HANDLE *hp_pwav, 
                        char *cp_filepath_in, char *cp_filepath_out, 
                        int i_save_output, int *ip_legal_files)
{
   struct pwavHdlType *cast_handle;
   
   cast_handle = (struct pwavHdlType *)(hp_pwav);
   
   /* Wait till all checks are done, to set to legal */
   *ip_legal_files = IS_FALSE;
   
   if (cast_handle == NULL)
      return(NOT_OKAY); 
                        
   /* Check if input filepath is NULL */
   if (cp_filepath_in == NULL)
      return(OKAY);
   
   /* If saving output, make sure the input and output names are not the same */
   if (i_save_output)
   {
      if (cp_filepath_out == NULL)
         return(OKAY);
      if (_stricmp(cp_filepath_in, cp_filepath_out) == 0)
      {
         (cast_handle->slout_hdl)->Error(FIRST_LINE, "Illegal wave files");
         (cast_handle->slout_hdl)->Error(NEXT_LINE, "Files can not be the same.");
         return(OKAY);
      }
   }
    
   *ip_legal_files = IS_TRUE; 
    
   return(OKAY);
}                     
                       
/*
 * FUNCTION: pwavFreeUp()
 * DESCRIPTION:
 *   Frees the passed pwav handle and sets to NULL.
 */
int PT_DECLSPEC pwavFreeUp(PT_HANDLE **hpp_pwav)
{
   struct pwavHdlType *cast_handle;

   cast_handle = (struct pwavHdlType *)(*hpp_pwav);

   if (cast_handle == NULL)
		return(OKAY);

   /* Free the buffers */
   if (pwav_FreeBuffers(*hpp_pwav) != OKAY)
	   return(NOT_OKAY);

   free(cast_handle);

   *hpp_pwav = NULL;

   return(OKAY);
}

/*
 * FUNCTION: pwav_FreeBuffers()
 * DESCRIPTION:
 *   Frees the buffers
 */
int pwav_FreeBuffers(PT_HANDLE *hp_pwav)
{
   struct pwavHdlType *cast_handle;
   int index;

   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
		return(OKAY);

   /* Free the input buffers */
   for (index=0; index < PWAV_NUM_INPUT_BUFFERS; index++)
   {
	   if (cast_handle->lpDataIn[index] != NULL)
         GlobalUnlock(cast_handle->lpDataIn[index]); 
        
	   if (cast_handle->lpWaveHdrIn[index] != NULL)   
         GlobalUnlock(cast_handle->lpWaveHdrIn[index]);

	   if (cast_handle->hDataIn[index] != NULL)  
         GlobalFree(cast_handle->hDataIn[index]);         
   
       if (cast_handle->hWaveHdrIn[index] != NULL)  
         GlobalFree(cast_handle->hWaveHdrIn[index]);    

   }  
   
   /* Free the output buffers */
   for (index=0; index < cast_handle->num_buffers; index++)
   {
       if (cast_handle->lpDataOut[index] != NULL)
         GlobalUnlock(cast_handle->lpDataOut[index]); 
        
	   if (cast_handle->lpWaveHdrOut[index] != NULL)   
         GlobalUnlock(cast_handle->lpWaveHdrOut[index]);

	   if (cast_handle->hDataOut[index] != NULL)  
         GlobalFree(cast_handle->hDataOut[index]);         
   
       if (cast_handle->hWaveHdrOut[index] != NULL)  
         GlobalFree(cast_handle->hWaveHdrOut[index]);    
   }  
   
   /* Free the input arrays */
   if (cast_handle->hDataIn != NULL)
      free(cast_handle->hDataIn);
   if (cast_handle->lpDataIn != NULL)
      free(cast_handle->lpDataIn);   
   if (cast_handle->hWaveHdrIn != NULL)
      free(cast_handle->hWaveHdrIn);
   if (cast_handle->lpWaveHdrIn != NULL)
      free(cast_handle->lpWaveHdrIn);                 

   /* Free the output arrays */
   if (cast_handle->hDataOut != NULL)
      free(cast_handle->hDataOut);
   if (cast_handle->lpDataOut != NULL)
      free(cast_handle->lpDataOut);   
   if (cast_handle->hWaveHdrOut != NULL)
      free(cast_handle->hWaveHdrOut);
   if (cast_handle->lpWaveHdrOut != NULL)
      free(cast_handle->lpWaveHdrOut);                 

	/* Free tmp float buffer */
   if (cast_handle->float_buffer != NULL)
      free(cast_handle->float_buffer);                 

   return(OKAY);
}

/*
 * FUNCTION: pwavDump()
 * DESCRIPTION:
 *   Dump the passed resp handle to the screen.
 */
int PT_DECLSPEC pwavDump(PT_HANDLE *hp_pwav)
{
   struct pwavHdlType *cast_handle;
  
   cast_handle = (struct pwavHdlType *)(hp_pwav);

   if (cast_handle == NULL)
      return(NOT_OKAY);
  
   sprintf(cast_handle->msg1, "The pwav handle");
   (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);  
  
   sprintf(cast_handle->msg1, "Buffer Length Samples= %d", 
      cast_handle->buffer_length_samples);
   (cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);

   return(OKAY);
}