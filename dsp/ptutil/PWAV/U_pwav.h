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
#ifndef _U_PWAV_H_
#define _U_PWAV_H_

#ifndef __ANDROID__
#include <windows.h>  
#include <mmsystem.h> 
#endif //WIN32

#include "pwav.h"
#include "slout.h" 

/* Largest possible size of wave buffers, used for allocation.
 * Note that max size must be less than 65536, apparently there
 * must be some room for header info.
 */
#define PWAV_MAX_WAVE_BUFFER_SIZE 64000

/* Should only need 2 input buffers since we have multiple output buffers */
#define PWAV_NUM_INPUT_BUFFERS 2

/* Local Functions */
/* pwav.cpp */
int pwav_FreeBuffers(PT_HANDLE *);
int pwav_AllocateBuffers(PT_HANDLE *);

/* pwavIO.cpp */
int pwav_AllocateDataBuffer(PT_HANDLE *hp_pwav, HANDLE *hpData, 
						   HPSTR *lppData, HANDLE *hpWaveHdr,
						   LPWAVEHDR *lppWaveHdr, int *ip_headerFlag);
int pwav_PrepareHeaders(PT_HANDLE *hp_pwav);
int pwavQueueIOBuffer(PT_HANDLE *hp_pwav);

/* Pwav handle definition */
struct pwavHdlType {
   /* Initialization info */
   CSlout *slout_hdl;
   wchar_t wcp_msg1[1024]; /* String for messages */
   char msg1[1024]; /* String for messages */

   HWND master_hwnd;
   long buffer_length_samples;
   
   int input_num_channels;  /* Number of channels of input file */
   int output_num_channels; /* Number of channels of output file */
   
   HWAVEIN hWaveIn;  /* Handle to soundcard in */
   HWAVEOUT hWaveOut; /* Handle to soundcard  out */
   
   /* Wave info */
   HMMIO hmmio_in;
   HMMIO hmmio_out;
   MMCKINFO ckOut;
   MMCKINFO ckOutRiff;
   WAVEFORMATEX *pFormatIn;     
   WAVEFORMATEX *pFormatOut;

   DWORD dwDataSize; /* Number of bytes of data in file */
   long bytes_left_to_read; /* Number of bytes left to read */    

   WORD wBlockSizeIn;  /* Number of bytes per sample taking mono/stereo into account */
   WORD wBlockSizeOut; /* For example, 16 bit mono -> 2, 24 bit stereo -> 6 */
   
   int num_buffers;  /* Number of buffers */

   HANDLE *hDataIn; /* Array: num_buffers elements */
   HANDLE *hDataOut; /* Array: num_buffers elements */
   HPSTR *lpDataIn; /* Array: num_buffers elements */
   HPSTR *lpDataOut; /* Array: num_buffers elements */
   HANDLE *hWaveHdrIn;   /* Array: num_buffers elements */
   HANDLE *hWaveHdrOut;   /* Array: num_buffers elements */
   LPWAVEHDR *lpWaveHdrIn;  /* Array: num_buffers elements */
   LPWAVEHDR *lpWaveHdrOut;  /* Array: num_buffers elements */
   int *headerFlagsIn;    /* Array of flags saying if the headers have been prepared */
   int *headerFlagsOut;    /* Array of flags saying if the headers have been prepared */

	realtype *float_buffer; /* Temporary buffer used for 24 bit to float conversions */

   int save_mode;  /* IS_TRUE, or IS_FALSE */
   int next_input_buffer;
   int next_output_buffer;
	int io_24_bit_flag;
};

#endif //_U_PWAV_H_