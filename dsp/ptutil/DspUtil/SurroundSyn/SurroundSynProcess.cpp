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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "codedefs.h"
#include "SurroundSyn.h"
#include "u_SurroundSyn.h"

/*
 * FUNCTION: SurroundSynProcess()
 * DESCRIPTION:
 *  Synthesizes Surround Sound output from mono or stereo input
 *  This version only does 6 channel 5.1
 *  Output buffer must be large enough for 6 channels, signal in MS format.
 *
 */
int PT_DECLSPEC SurroundSynProcess(PT_HANDLE *hp_SurroundSyn,
							/* Input Signal Info */
							realtype *rp_signal_in,		/* Input signal, points interleaved in stereo case */
							realtype *rp_signal_out,	/* Output signal, points interleaved in MS format */
							int i_num_sample_sets,  /* Number of mono sample points or stereo sample input pairs */
							int i_num_channels,     /* 1 for mono, 2 for stereo */
                     realtype r_samp_freq,   /* Sampling frequency in hz. */
							int i_processing_on     /* Flag stating if processing is currently on */
					 	)
{
	struct SurroundSynHdlType *cast_handle;
	float *xv, *yv;
	int i,j;

	cast_handle = (struct SurroundSynHdlType *)(hp_SurroundSyn);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	xv = cast_handle->xv;
	yv = cast_handle->yv;

	// If processing is off, copy left and right or mono input channels to left and right surround channels
	if( i_processing_on == 0 )
	{
		if( i_num_channels == 1 )
		{	// Mono in case
			for(i=0; i<i_num_sample_sets; i++)
			{
				j = i * 6;
				rp_signal_out[j] = rp_signal_out[j+1] = rp_signal_in[i];
				rp_signal_out[j+2] = rp_signal_out[j+3] = rp_signal_out[j+4] = rp_signal_out[j+5] = (realtype)0.0;
			}
		}
		else
		{  //Stereo in case
			for(i=0; i<(2 * i_num_sample_sets); i += 2)
			{
				j = i * 3;
				rp_signal_out[j] = rp_signal_in[i];
				rp_signal_out[j+1] = rp_signal_in[i+1];
				rp_signal_out[j+2] = rp_signal_out[j+3] = rp_signal_out[j+4] = rp_signal_out[j+5] = (realtype)0.0;
			}
		}

		return(OKAY);
	}

	//Ordering for 5.1 is: Front Left, Front Right, Front Center, Low Frequency, Back Left, Back Right
	//Ordering for 7.1 is: Front Left, Front Right, Front Center, Low Frequency, Back Left, Back Right, Side Left, Side Right
	if( i_num_channels == 2 )
	{	// Stereo in case
		float stereo_left, stereo_right;
		float left, right, center, lfe, back_left, back_right;
		float filt_in;

		for(i=0; i<(i_num_sample_sets * 2); i += 2)
		{
			j = i * 3;

			stereo_left = rp_signal_in[i];
			stereo_right = rp_signal_in[i+1];

			filt_in = (stereo_left + stereo_right) * (float)0.5 + (float)SURROUND_SYN_BIAS;

			{  xv[0] = xv[1]; xv[1] = xv[2]; 
				xv[2] = filt_in * (float)(SURROUND_SYN_LPGAIN);
				yv[0] = yv[1]; yv[1] = yv[2]; 
				yv[2] =   (xv[0] + xv[2]) + 2 * xv[1]
								+ ( (float)-0.9761110145 * yv[0]) + (  (float)1.9758222021 * yv[1]);
				lfe = yv[2];
			}
			left = (float)SURROUND_SYN_FRONT_MAIN * stereo_left - (float)SURROUND_SYN_FRONT_BLEED * stereo_right;
			right = (float)SURROUND_SYN_FRONT_MAIN * stereo_right - (float)SURROUND_SYN_FRONT_BLEED * stereo_left;
			center = (float)SURROUND_SYN_CENTER_GAIN * filt_in;
			back_left = (float)SURROUND_SYN_REAR_GAIN * (left - (float)SURROUND_SYN_REAR_DIFF * right);
			back_right = (float)SURROUND_SYN_REAR_GAIN * (right - (float)SURROUND_SYN_REAR_DIFF * left);

			rp_signal_out[j] = left;
			rp_signal_out[j+1] = right;
			rp_signal_out[j+2] = center;
			rp_signal_out[j+3] = lfe;
			rp_signal_out[j+4] = back_left;
			rp_signal_out[j+5] = back_right;
		}
	}
	else
	{	// Mono in case
		float stereo_left, stereo_right;
		float left, right, center, lfe, back_left, back_right;
		float filt_in;

		for(i=0; i<i_num_sample_sets; i++)
		{
			j = i * 6;

			// Note - to improve could do a simple comb filter split here
			stereo_left = rp_signal_in[i];
			stereo_right = rp_signal_in[i];

			filt_in = stereo_left + (float)SURROUND_SYN_BIAS;

			{  xv[0] = xv[1]; xv[1] = xv[2]; 
				xv[2] = filt_in * (float)(SURROUND_SYN_LPGAIN);
				yv[0] = yv[1]; yv[1] = yv[2]; 
				yv[2] =   (xv[0] + xv[2]) + 2 * xv[1]
								+ ( (float)-0.9761110145 * yv[0]) + (  (float)1.9758222021 * yv[1]);
				lfe = yv[2];
			}
			left = (float)SURROUND_SYN_FRONT_MAIN * stereo_left - (float)SURROUND_SYN_FRONT_BLEED * stereo_right;
			right = (float)SURROUND_SYN_FRONT_MAIN * stereo_right - (float)SURROUND_SYN_FRONT_BLEED * stereo_left;
			center = (float)SURROUND_SYN_CENTER_GAIN * filt_in;
			back_left = (float)SURROUND_SYN_REAR_GAIN * (left - (float)SURROUND_SYN_REAR_DIFF * right);
			back_right = (float)SURROUND_SYN_REAR_GAIN * (right - (float)SURROUND_SYN_REAR_DIFF * left);

			rp_signal_out[j] = left;
			rp_signal_out[j+1] = right;
			rp_signal_out[j+2] = center;
			rp_signal_out[j+3] = lfe;
			rp_signal_out[j+4] = back_left;
			rp_signal_out[j+5] = back_right;
		}
	}

	return(OKAY);
}

