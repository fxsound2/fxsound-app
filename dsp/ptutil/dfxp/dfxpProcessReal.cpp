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

/* dfxpProcessReal.cpp */

#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "u_dfxp.h" /* Must go before codedefs.h due to mmgr */
#include "codedefs.h"

#include "dfxp.h"

#include "mth.h"
#include "realSample.h"
#include "spectrum.h"
#include "com.h"
#include "DfxSdk.h"
#include "BinauralSyn.h"
#include "GraphicEq.h"

extern "C" {
#include "comSftwr.h"
}

/*
 * FUNCTION: dfxpModifyRealtypeSamples() 
 * DESCRIPTION:
 *   Process the passed buffer of realtype point values.  
 *
 *   NOTE: This processing is always done in-place.
 */
int dfxpModifyRealtypeSamples(PT_HANDLE *hp_dfxp, realtype *rp_samples, int i_num_sample_sets, int i_reorder)
{
	struct dfxpHdlType *cast_handle;
   int stereo_in_mode;
	int stereo_out_mode;
	int total_buffer_length;
	realtype tmp_float;
	realtype *rp_channels;
   int bypass_all;
	int input_slipped;
	realtype *rp_buf;
	int i,j,k;
	int spectrum_process_num_channels;
	int i_current_buffer_msecs;
	int i_dfx_tuned_track_playing;
	int center_nonzero, sub_nonzero, side_nonzero, rear_nonzero;
	realtype ftmp;
	BOOL b_lean_and_mean;
	int i_eq_on;

	cast_handle = (struct dfxpHdlType *)(hp_dfxp);

	if (cast_handle == NULL)
		return(OKAY);

	/* 
	 * Set whether we are going to do lean and mean processing which is necessary for the new
	 * DFX 11 style which uses the virtual soundcard.
	 */
	b_lean_and_mean = FALSE;
	if (!cast_handle->processing_only)
	{
		b_lean_and_mean = TRUE;
	}

	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Entered");

   if (!(cast_handle->fully_initialized))
		return(NOT_OKAY);

	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Calling dfxp_UpdateBufferLengthInfo()");

	if (!b_lean_and_mean)
	{
		/* 
		 * Check if this is the longest buffer so far, and if so record it's length in the registry 
		 * Also update the average buffer length info.
		 */
		if (dfxp_UpdateBufferLengthInfo(hp_dfxp, i_num_sample_sets, &i_current_buffer_msecs) != OKAY)
			return(NOT_OKAY);
	}

	total_buffer_length = cast_handle->num_channels_out * i_num_sample_sets;

	//If buffer is bigger than max size or format is unsupported, just return with no processing applied
	//Note buffer is sized to account for multiple channels, so test below is correct
	if( (i_num_sample_sets > DAW_MAX_BUFFER_SIZE ) || (cast_handle->unsupported_format_flag == IS_TRUE) )
		return(OKAY);

	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Calling dfxp_ClearBuffersIfSongStart()");

	/* Check if this is a new song and therefore we need to clear out the previous buffers */
	if (!b_lean_and_mean)
	{
		if (dfxp_ClearBuffersIfSongStart(hp_dfxp) != OKAY)
			return(NOT_OKAY);
	}

	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Calling dfxpGetButtonValue()");

	/* Get the bypass all setting */;
	if (dfxpGetButtonValue(hp_dfxp, DFX_UI_BUTTON_BYPASS, &bypass_all) != OKAY)
		return(NOT_OKAY);

	/* 
	 * Check if a DFX printed track (iDFX) is playing.  
	 * If so, then bypass the processing.
	 */
	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Calling dfxpGetDfxPrintedTrackPlaying()");

	if (dfxpGetDfxTunedTrackPlaying(hp_dfxp, &i_dfx_tuned_track_playing) != OKAY)
		return(NOT_OKAY);
	if (i_dfx_tuned_track_playing)
		bypass_all = IS_TRUE;

	// Do EQ processing
	if (!bypass_all)
	{
		if (dfxpEqGetProcessingOn(hp_dfxp, DFXP_STORAGE_TYPE_MEMORY, &i_eq_on) != OKAY)
			return(NOT_OKAY);

		if ((i_eq_on) &&
			(cast_handle->num_channels_out <= 2) || (cast_handle->num_channels_out == 6) || (cast_handle->num_channels_out == 8) )
		{
			if (GraphicEqProcess(cast_handle->eq.graphicEq_hdl, 
										 rp_samples, rp_samples, i_num_sample_sets, cast_handle->num_channels_out,
										cast_handle->sampling_freq) != OKAY)
				return(NOT_OKAY);
		}
	}

	// Binaural processing to map 6/8 channel signal to stereo for headphone usage.
	// For stereo input, adds depth to stereo output.
	//
	// PTNOTE - currently binaural headphone processing is only implemented for 48k and lower sampling rates
	if( cast_handle->binaural_headphone_on_flag && (!bypass_all) && (cast_handle->sampling_freq <= (realtype)48000.0) )
	{
		if ( cast_handle->num_channels_out == 2 )
		{
			if ( BinauralSynProcessStereoFormat(cast_handle->BinauralSyn_hdl, 
										 rp_samples, (int)cast_handle->sampling_freq, i_num_sample_sets, rp_samples) != OKAY )
				return(NOT_OKAY);
		}

		if ( (cast_handle->num_channels_out == 6) || (cast_handle->num_channels_out == 8) )
		{
			if( BinauralSynProcessSurroundFormatWindowsOrdering(cast_handle->BinauralSyn_hdl,
								cast_handle->num_channels_out, (int)cast_handle->sampling_freq,
								rp_samples, i_num_sample_sets, rp_samples) != OKAY)
				return(NOT_OKAY);
		}
	}

	/* Set the stereo in and out modes */
	if (cast_handle->num_channels_out == 1)
	{
		stereo_in_mode = IS_FALSE;
		stereo_out_mode = IS_FALSE;
	}
	else
	{
		stereo_in_mode = IS_TRUE;
		stereo_out_mode = IS_TRUE;
	}

	/* Initialize input slipped flag */
	input_slipped = IS_FALSE;

	/* Do the stream id analysis */
/*
		if (dfxp_StreamAnalysisForSongID(hp_dfxp, 
												  rp_samples, i_num_sample_sets,
			    							     cast_handle->num_channels_in, cast_handle->sampling_freq) != OKAY)
			return(NOT_OKAY);
*/

	// If input is SurroundSound in standard Windows format, reorder for DFX processing
	// See MS support doc "Multiple Channel Audio Data and WAVE Files" http://www.microsoft.com/whdc/device/audio/multichaud.mspx#EVHAC
	// for info on multi-channel signal ordering.
	//Ordering for 5.1 is: Front Left, Front Right, Front Center, Low Frequency, Back Left, Back Right
	//Ordering for 7.1 is: Front Left, Front Right, Front Center, Low Frequency, Back Left, Back Right, Side Left, Side Right
 
	// Zero "zero check" flags
	center_nonzero = 0;
	sub_nonzero = 0;
	side_nonzero = 0;
	rear_nonzero = 0;

	// Handle quad case
	if ( (cast_handle->num_channels_out == 4) && (i_reorder) )
	{
		rp_buf = cast_handle->r_samples_reordered;

		for(i=0; i<i_num_sample_sets; i++)
		{
			j = i * 2;
			k = i * 4;

			// Front channels
		   rp_buf[j] = rp_samples[k];
		   rp_buf[j + 1] = rp_samples[k+1];

			// Rear channels
		   ftmp = rp_samples[k+2];
			if( ftmp != (realtype)0.0 )
				rear_nonzero = 1;
		   rp_buf[i_num_sample_sets * 2 + j] = ftmp;

		   ftmp = rp_samples[k+3];
			if( ftmp != (realtype)0.0 )
				rear_nonzero = 1;
		   rp_buf[i_num_sample_sets * 2 + j + 1] = ftmp;
		}
	}
	else if ( (cast_handle->num_channels_out == 6) && (i_reorder) )
	{
		rp_buf = cast_handle->r_samples_reordered;

		for(i=0; i<i_num_sample_sets; i++)
		{
			j = i * 2;
			k = i * 6;

			// Front channels
		   rp_buf[j] = rp_samples[k];
		   rp_buf[j + 1] = rp_samples[k+1];

			// Center channel
		   ftmp = rp_samples[k+2];
			if( ftmp != (realtype)0.0 )
				center_nonzero = 1;
		   rp_buf[i_num_sample_sets * 2 + i] = ftmp;

			// Subwoofer channel
		   ftmp = rp_samples[k+3];
			if( ftmp != (realtype)0.0 )
				sub_nonzero = 1;
		   rp_buf[i_num_sample_sets * 3 + i] = ftmp;

			// Rear channels
		   ftmp = rp_samples[k+4];
			if( ftmp != (realtype)0.0 )
				rear_nonzero = 1;
		   rp_buf[i_num_sample_sets * 4 + j] = ftmp;

		   ftmp = rp_samples[k+5];
			if( ftmp != (realtype)0.0 )
				rear_nonzero = 1;
		   rp_buf[i_num_sample_sets * 4 + j + 1] = ftmp;
		}
	}
	else if ( (cast_handle->num_channels_out == 8) && (i_reorder) )
	{
		rp_buf = cast_handle->r_samples_reordered;

		for(i=0; i<i_num_sample_sets; i++)
		{
			j = i * 2;
			k = i * 8;

			// Front channels
		   rp_buf[j] = rp_samples[k];
		   rp_buf[j + 1] = rp_samples[k+1];

			// Center channel
		   ftmp = rp_samples[k+2];
			if( ftmp != (realtype)0.0 )
				center_nonzero = 1;
		   rp_buf[i_num_sample_sets * 2 + i] = ftmp;

			// Subwoofer channel
		   ftmp = rp_samples[k+3];
			if( ftmp != (realtype)0.0 )
				sub_nonzero = 1;
		   rp_buf[i_num_sample_sets * 3 + i] = ftmp;

			// Rear channels
		   ftmp = rp_samples[k+4];
			if( ftmp != (realtype)0.0 )
				rear_nonzero = 1;
		   rp_buf[i_num_sample_sets * 4 + j] = ftmp;

		   ftmp = rp_samples[k+5];
			if( ftmp != (realtype)0.0 )
				rear_nonzero = 1;
		   rp_buf[i_num_sample_sets * 4 + j + 1] = ftmp;

			// Side channels
		   ftmp = rp_samples[k+6];
			if( ftmp != (realtype)0.0 )
				side_nonzero = 1;
		   rp_buf[i_num_sample_sets * 6 + j] = ftmp;

		   ftmp = rp_samples[k+7];
			if( ftmp != (realtype)0.0 )
				side_nonzero = 1;
		   rp_buf[i_num_sample_sets * 6 + j + 1] = ftmp;
		}
	}
	else
		rp_buf = rp_samples;

	/* 
	 * Do the DFX processing.  
	 * In Remix case, we only want to do this to avoid clipping.
	 */
	if (!bypass_all)
   {
		if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
			(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Calling realSampleForceLegalValues_ArrayOnly()");

		/* First do a pass on the buffer to make sure all the values are in legal range */
		if (!b_lean_and_mean)
		{
			if (realSampleForceLegalValues_ArrayOnly(rp_buf, (long)total_buffer_length) != OKAY)
				return(NOT_OKAY);
		}

		switch (cast_handle->num_channels_out)
		{
		case 1: case 2:
			if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
				(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Calling comProcessWaveBuffer() : Case 1 or 2");
			
			
			if (comProcessWaveBuffer(cast_handle->com_hdl_front, (long *)rp_buf, &tmp_float, (long)i_num_sample_sets, 
                               stereo_in_mode, stereo_out_mode, cast_handle->internal_rate_ratio,(int)COM_32_BIT_FLOAT_SAMPLES) != OKAY)
				return(NOT_OKAY);
			
			break;

		case 4:
			rp_channels = rp_buf;

			if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
				(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Calling comProcessWaveBuffer() : Case 4");

			if (comProcessWaveBuffer(cast_handle->com_hdl_front, (long *)rp_channels, &tmp_float, (long)i_num_sample_sets, 
                               IS_TRUE, IS_TRUE, cast_handle->internal_rate_ratio, (int)COM_32_BIT_FLOAT_SAMPLES) != OKAY)
				return(NOT_OKAY);

			rp_channels += 2 * i_num_sample_sets;
			if( rear_nonzero )
			{
				if (comProcessWaveBuffer(cast_handle->com_hdl_rear, (long *)rp_channels, &tmp_float, (long)i_num_sample_sets, 
											 IS_TRUE, IS_TRUE, cast_handle->internal_rate_ratio, (int)COM_32_BIT_FLOAT_SAMPLES) != OKAY)
					return(NOT_OKAY);
			}
			break;

		case 6:
			rp_channels = rp_buf;

			if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
				(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Calling comProcessWaveBuffer() : Case 6");

			if (comProcessWaveBuffer(cast_handle->com_hdl_front, (long *)rp_channels, &tmp_float, (long)i_num_sample_sets, 
                               IS_TRUE, IS_TRUE, cast_handle->internal_rate_ratio, (int)COM_32_BIT_FLOAT_SAMPLES) != OKAY)
				return(NOT_OKAY);

			rp_channels += 2 * i_num_sample_sets;
			if( center_nonzero )
			{
				if (comProcessWaveBuffer(cast_handle->com_hdl_center, (long *)rp_channels, &tmp_float, (long)i_num_sample_sets, 
											 IS_FALSE, IS_FALSE, cast_handle->internal_rate_ratio, (int)COM_32_BIT_FLOAT_SAMPLES) != OKAY)
					return(NOT_OKAY);
			}

			rp_channels += i_num_sample_sets;
			if( sub_nonzero )
			{
				if (comProcessWaveBuffer(cast_handle->com_hdl_subwoofer, (long *)rp_channels, &tmp_float, (long)i_num_sample_sets, 
											 IS_FALSE, IS_FALSE, cast_handle->internal_rate_ratio, (int)COM_32_BIT_FLOAT_SAMPLES) != OKAY)
					return(NOT_OKAY);
			}

			rp_channels += i_num_sample_sets;
			if( rear_nonzero )
			{
				if (comProcessWaveBuffer(cast_handle->com_hdl_rear, (long *)rp_channels, &tmp_float, (long)i_num_sample_sets, 
											 IS_TRUE, IS_TRUE, cast_handle->internal_rate_ratio, (int)COM_32_BIT_FLOAT_SAMPLES) != OKAY)
					return(NOT_OKAY);
			}

			break;

		case 8:
			rp_channels = rp_buf;

			if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
				(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Calling comProcessWaveBuffer() : Case 8");

			if (comProcessWaveBuffer(cast_handle->com_hdl_front, (long *)rp_channels, &tmp_float, (long)i_num_sample_sets, 
                               IS_TRUE, IS_TRUE, cast_handle->internal_rate_ratio, (int)COM_32_BIT_FLOAT_SAMPLES) != OKAY)
				return(NOT_OKAY);

			rp_channels += 2 * i_num_sample_sets;
			if( center_nonzero )
			{
				if (comProcessWaveBuffer(cast_handle->com_hdl_center, (long *)rp_channels, &tmp_float, (long)i_num_sample_sets, 
											 IS_FALSE, IS_FALSE, cast_handle->internal_rate_ratio, (int)COM_32_BIT_FLOAT_SAMPLES) != OKAY)
					return(NOT_OKAY);
			}

			rp_channels += i_num_sample_sets;
			if( sub_nonzero )
			{
				if (comProcessWaveBuffer(cast_handle->com_hdl_subwoofer, (long *)rp_channels, &tmp_float, (long)i_num_sample_sets, 
											 IS_FALSE, IS_FALSE, cast_handle->internal_rate_ratio, (int)COM_32_BIT_FLOAT_SAMPLES) != OKAY)
					return(NOT_OKAY);
			}

			rp_channels += i_num_sample_sets;
			if( rear_nonzero )
			{
				if (comProcessWaveBuffer(cast_handle->com_hdl_rear, (long *)rp_channels, &tmp_float, (long)i_num_sample_sets, 
											 IS_TRUE, IS_TRUE, cast_handle->internal_rate_ratio, (int)COM_32_BIT_FLOAT_SAMPLES) != OKAY)
					return(NOT_OKAY);
			}

			rp_channels += 2 * i_num_sample_sets;
			if( side_nonzero )
			{
				if (comProcessWaveBuffer(cast_handle->com_hdl_side, (long *)rp_channels, &tmp_float, (long)i_num_sample_sets, 
											 IS_TRUE, IS_TRUE, cast_handle->internal_rate_ratio, (int)COM_32_BIT_FLOAT_SAMPLES) != OKAY)
					return(NOT_OKAY);
			}

			break;
		}
   }

	/* Analyse the output buffer's spectrum */
	if (cast_handle->spectrum.spectrum_hdl != NULL)
	{
		/* 
		 * Generate the new spectrum values based on the buffer 
		 * Note: In surround sound case will just do the spectrum of the front two channels, 
		 *       before reordering 
		 */
		spectrum_process_num_channels = 2; 
		if (cast_handle->num_channels_in == 1)
				spectrum_process_num_channels = 1;

		if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
			(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Calling spectrumProcess()");

		if (spectrumProcess(cast_handle->spectrum.spectrum_hdl, rp_buf, i_num_sample_sets, 
								  spectrum_process_num_channels, cast_handle->sampling_freq,
								  (!bypass_all)) != OKAY)
			return(NOT_OKAY);

		/*
		 * Send the spectrum to the GUI.
		 * Note: In case of small buffers, it only write once every time a specified number
		 *       of samples sets have been processed.
		 */
		cast_handle->spectrum.sample_sets_since_last_spectrum_save = 
			cast_handle->spectrum.sample_sets_since_last_spectrum_save + i_num_sample_sets;

		if (cast_handle->spectrum.sample_sets_since_last_spectrum_save > DFXP_SAMPLE_SETS_PER_SAVE_SPECTRUM)
		{
			if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
				(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Calling dfxp_SpectrumSaveToFile()");

			if (dfxp_SpectrumStoreCurrentValuesInSharedMemory(hp_dfxp, IS_FALSE) != OKAY)
				return(NOT_OKAY);

			cast_handle->spectrum.sample_sets_since_last_spectrum_save = 0;
		}
	}

	// If input is Quad
	if ( (cast_handle->num_channels_out == 4) && (i_reorder) )
		for(i=0; i<i_num_sample_sets; i++)
		{
			j = i * 2;
			k = i * 4;

			// Front channels
			rp_samples[k]   = rp_buf[j];
			rp_samples[k+1] = rp_buf[j+1];

			// Rear channels
			rp_samples[k+2] = rp_buf[i_num_sample_sets * 2 + j];
			rp_samples[k+3] = rp_buf[i_num_sample_sets * 2 + j + 1];
		}
	// If input is SurroundSound in standard Windows format, put output back in that order
	else if ( (cast_handle->num_channels_out == 6) && (i_reorder) )
		for(i=0; i<i_num_sample_sets; i++)
		{
			j = i * 2;
			k = i * 6;

			// Front channels
			rp_samples[k]   = rp_buf[j];
			rp_samples[k+1] = rp_buf[j+1];

			// Center channel
			rp_samples[k+2] = rp_buf[i_num_sample_sets * 2 + i];

			// Subwoofer channel
			rp_samples[k+3] = rp_buf[i_num_sample_sets * 3 + i];

			// Rear channels
			rp_samples[k+4] = rp_buf[i_num_sample_sets * 4 + j];
			rp_samples[k+5] = rp_buf[i_num_sample_sets * 4 + j + 1];
		}
	else	if ( (cast_handle->num_channels_out == 8) && (i_reorder) )
		for(i=0; i<i_num_sample_sets; i++)
		{
			j = i * 2;
			k = i * 8;

			// Front channels
			rp_samples[k]   = rp_buf[j];
			rp_samples[k+1] = rp_buf[j+1];

			// Center channel
			rp_samples[k+2] = rp_buf[i_num_sample_sets * 2 + i];

			// Subwoofer channel
			rp_samples[k+3] = rp_buf[i_num_sample_sets * 3 + i];

			// Rear channels
			rp_samples[k+4] = rp_buf[i_num_sample_sets * 4 + j];
			rp_samples[k+5] = rp_buf[i_num_sample_sets * 4 + j + 1];

			// Rear channels
			rp_samples[k+6] = rp_buf[i_num_sample_sets * 6 + j];
			rp_samples[k+7] = rp_buf[i_num_sample_sets * 6 + j + 1];
		}

	/* Take care of recording */
	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyShortIntSamples(): Calling dfxp_RecordBufferProcessed()");

	if ((cast_handle->trace.mode) && (!cast_handle->trace.i_process_real_samples_done))
		(cast_handle->slout1)->Message_Wide(FIRST_LINE, L"dfxpModifyRealtypeSamples(): Success");

	cast_handle->trace.i_process_real_samples_done = IS_TRUE;

	return(OKAY);
}