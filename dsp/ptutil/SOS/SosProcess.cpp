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
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>

#include "codedefs.h"
#include "slout.h"
#include "vals.h"
#include "mry.h"
#include "filt.h"
#include "sos.h"
#include "u_sos.h"

/*
 * FUNCTION: sosProcessBuffer()
 * DESCRIPTION:
 *   Processes the passed in buffer using the current sos handle settings.
 *   This function is for mono or stereo signals only.
 *   This version uses a DC bias component to avoid underflow problems.
 *   PTNOTE- this appears to have a serious problem with shelf functions, the processing
 *   method uses a form specific to the coeff symmetry that occurs with parametric filters.
 */
int PT_DECLSPEC sosProcessBuffer(PT_HANDLE *hp_sos, realtype *rp_in_buf, realtype *rp_out_buf, int i_num_sample_sets, int i_num_channels)
{
	struct sosHdlType *cast_handle;

	struct sosSectionType *s;
	int i, j, k;
	realtype sum_squares = 0.0f;
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
	if (cast_handle == NULL)
       return(NOT_OKAY);

	/* Currently only handles mono or stereo cases */
	if( (i_num_channels != 1) && (i_num_channels != 2) )
		return(NOT_OKAY);

	k = 0;
	for(j=0; j<i_num_sample_sets; j++)
	{
		realtype in1, in2, out1, out2;
		int active_flag;

		if( i_num_channels == 1)
		{
#ifdef SOS_DO_DC_BLOCKING
			realtype in_tmp;
			// Do DC blocking filtering.
			in_tmp = rp_in_buf[k];
			in1 = in_tmp - cast_handle->in1_old + (realtype)SOS_DCBLOCK_ALPHA * cast_handle->outDC1_old;
			cast_handle->in1_old = in_tmp;
			cast_handle->outDC1_old = in1;
#else
			in1 = rp_in_buf[k];
#endif //SOS_DO_DC_BLOCKING

			/* Outputs are also set to handle the case where all sections are off */
			out1 = in1;
			for(i=0; i<cast_handle->num_active_sections; i++)
			{
				active_flag = cast_handle->section_on_flag[i];
				if( (i == 0) && cast_handle->disable_band_1 )
					active_flag = 0;

				if( active_flag )
				{
					s = &((cast_handle->sections)[i]);

					/* Processing derived from macro kerSosFiltDirectForm2TransParaExtState */
					out1 = s->state1 + s->b0 * in1 + (realtype)SOS_FLOAT_BIAS;
					s->state1 = (in1 - out1) * s->b1 + s->state2;
					s->state2 = s->b2 * in1 - s->a2 * out1;

					in1 = out1;
				}
			}
			rp_out_buf[k] = out1;
		}
		else /* Stereo case */
		{
#ifdef SOS_DO_DC_BLOCKING
			// Do DC blocking filtering.
			realtype in_tmp;
			in_tmp = rp_in_buf[k];
			in1 = in_tmp - cast_handle->in1_old + (realtype)SOS_DCBLOCK_ALPHA * cast_handle->outDC1_old;
			cast_handle->in1_old = in_tmp;
			cast_handle->outDC1_old = in1;

			in_tmp = rp_in_buf[k+1];
			in2 = in_tmp - cast_handle->in2_old + (realtype)SOS_DCBLOCK_ALPHA * cast_handle->outDC2_old;
			cast_handle->in2_old = in_tmp;
			cast_handle->outDC2_old = in2;
#else
			in1 = rp_in_buf[k];
			in2 = rp_in_buf[k+1];
#endif //SOS_DO_DC_BLOCKING

			/* Outputs are also set to handle the case where all sections are off */
			out1 = in1;
			out2 = in2;

			for(i=0; i<cast_handle->num_active_sections; i++)
			{
				active_flag = cast_handle->section_on_flag[i];
				if( (i == 0) && cast_handle->disable_band_1 )
					active_flag = 0;

				if( active_flag )
				{
					s = &((cast_handle->sections)[i]);

					/* Processing derived from macro kerSosFiltDirectForm2TransParaExtState */
					out1 = s->state1 + s->b0 * in1 + (realtype)SOS_FLOAT_BIAS;
					s->state1 = (in1 - out1) * s->b1 + s->state2;
					s->state2 = s->b2 * in1 - s->a2 * out1;

					in1 = out1;

					out2 = s->state3 + s->b0 * in2 + (realtype)SOS_FLOAT_BIAS;
					s->state3 = (in2 - out2) * s->b1 + s->state4;
					s->state4 = s->b2 * in2 - s->a2 * out2;

					in2 = out2;
				}
			}
			rp_out_buf[k] = out1 * cast_handle->master_gain;
			rp_out_buf[k+1] = out2 * cast_handle->master_gain;

			if (cast_handle->target_rms != 0.0f)
			{
				sum_squares += (rp_out_buf[k] * rp_out_buf[k]) + (rp_out_buf[k + 1] * rp_out_buf[k + 1]);
			}
		}

		k += i_num_channels;
	}

	if (cast_handle->target_rms != 0.0f && i_num_channels == 2)
	{		
		realtype current_rms = sqrtf(sum_squares / (i_num_sample_sets * 2));
		realtype rms_gain = std::fmin(cast_handle->target_rms / current_rms, 1.0f);
		if (rms_gain > 0.0f)
		{
			if (cast_handle->normalization_gain == 1.0f)
			{
				cast_handle->normalization_gain = rms_gain;
			}
			else
			{
				if (fabs(cast_handle->normalization_gain - rms_gain) <= 0.01f)
				{
					cast_handle->normalization_gain = rms_gain;
				}
			}
		}

		k = 0;
		for (j = 0; j < i_num_sample_sets; j++)
		{
			rp_out_buf[k] *= cast_handle->normalization_gain;
			rp_out_buf[k + 1] *= cast_handle->normalization_gain;
			k += i_num_channels;
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: sosProcessBufferNoBias()
 * DESCRIPTION:
 *   Processes the passed in buffer using the current sos handle settings.
 *   This function is for mono or stereo signals only.
 *   This is a special version that does not include any DC bias component for accuracy critical usage.
 */
int PT_DECLSPEC sosProcessBufferNoBias(PT_HANDLE *hp_sos, realtype *rp_in_buf, realtype *rp_out_buf, int i_num_sample_sets, int i_num_channels)
{
	struct sosHdlType *cast_handle;

	struct sosSectionType *s;
	int i, j, k;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
	if (cast_handle == NULL)
       return(NOT_OKAY);

	/* Currently only handles mono or stereo cases */
	if( (i_num_channels != 1) && (i_num_channels != 2) )
		return(NOT_OKAY);

	k = 0;
	for(j=0; j<i_num_sample_sets; j++)
	{
		realtype in1, in2, out1, out2;
		int active_flag;

		if( i_num_channels == 1)
		{
			/* Outputs are also set to handle the case where all sections are off */
			in1 = rp_in_buf[k];
			out1 = in1;
			for(i=0; i<cast_handle->num_active_sections; i++)
			{
				active_flag = cast_handle->section_on_flag[i];
				if( (i == 0) && cast_handle->disable_band_1 )
					active_flag = 0;

				if( active_flag )
				{
					s = &((cast_handle->sections)[i]);

					// For now to avoid parametric assumption problems with shelfs, straight ahead processing is used
					out1 = s->b0 * in1 + s->b1 * s->state_1[0] + s->b2 * s->state_1[1]
								- s->a1 * s->state_1[2] - s->a2 * s->state_1[3];

					s->state_1[1] = s->state_1[0];
					s->state_1[0] = in1;
					s->state_1[3] = s->state_1[2];
					s->state_1[2] = out1;

					in1 = out1; // Pass output on to next filter
				}
			}
			rp_out_buf[k] = out1;
		}
		else /* Stereo case */
		{
			in1 = rp_in_buf[k];
			out1 = in1;
			in2 = rp_in_buf[k+1];
			out2 = in2;
			for(i=0; i<cast_handle->num_active_sections; i++)
			{
				active_flag = cast_handle->section_on_flag[i];
				if( (i == 0) && cast_handle->disable_band_1 )
					active_flag = 0;

				if( active_flag )
				{
					s = &((cast_handle->sections)[i]);

					// For now to avoid parametric assumption problems with shelfs, straight ahead processing is used
					out1 = s->b0 * in1 + s->b1 * s->state_1[0] + s->b2 * s->state_1[1]
								- s->a1 * s->state_1[2] - s->a2 * s->state_1[3];

					s->state_1[1] = s->state_1[0];
					s->state_1[0] = in1;
					s->state_1[3] = s->state_1[2];
					s->state_1[2] = out1;

					in1 = out1; // Pass output on to next filter

					// For now to avoid parametric assumption problems with shelfs, straight ahead processing is used
					out2 = s->b0 * in2 + s->b1 * s->state_2[0] + s->b2 * s->state_2[1]
								- s->a1 * s->state_2[2] - s->a2 * s->state_2[3];

					s->state_2[1] = s->state_2[0];
					s->state_2[0] = in2;
					s->state_2[3] = s->state_2[2];
					s->state_2[2] = out2;

					in2 = out2; // Pass output on to next filter
				}
			}
			rp_out_buf[k] = out1 * cast_handle->master_gain;
			rp_out_buf[k+1] = out2 * cast_handle->master_gain;
		}

		k += i_num_channels;
	}

	return(OKAY);
}

/*
 * FUNCTION: sosProcessSurroundBuffer()
 * DESCRIPTION:
 *   Processes the passed in buffer using the current sos handle settings.
 *   This function is for Surround Sound signals. Only puts bass boost on bass channel, no other eq on bass channel.
 */
int PT_DECLSPEC sosProcessSurroundBuffer(PT_HANDLE *hp_sos, realtype *rp_in_buf, realtype *rp_out_buf, int i_num_sample_sets, int i_num_channels)
{
	struct sosHdlType *cast_handle;

	struct sosSectionType *s;
	int i, j, k;
    
	cast_handle = (struct sosHdlType *)(hp_sos);  
	
	if (cast_handle == NULL)
       return(NOT_OKAY);

	/* Currently only handles 6 or 8 channel surround */
	if( (i_num_channels != 6) && (i_num_channels != 8) )
		return(NOT_OKAY);

	//Ordering for 5.1 is: Front Left, Front Right, Front Center, Low Frequency, Back Left, Back Right
	//Ordering for 7.1 is: Front Left, Front Right, Front Center, Low Frequency, Back Left, Back Right, Side Left, Side Right
	//Note - LFE channel only gets bands 0,1 others only get bands 2->max
	for(j=0; j < (i_num_sample_sets * i_num_channels) ; j += i_num_channels)
	{
		for(k=0; k<i_num_channels; k++)
		{
			realtype in, out;
			int start, end;
			int active_flag;

			// LFE channel only gets bottom two bands, others only get upper bands
			if(k == 3)
			{
				start = 0;
				end = 2;
			}
			else
			{
				start = 2;
				end = cast_handle->num_active_sections;
			}

			/* Outputs are also set to handle the case where all sections are off */
			in = rp_in_buf[j+k];
			out = in;
			for(i=start; i<end; i++)
			{
				active_flag = cast_handle->section_on_flag[i];
				if( (i == 0) && cast_handle->disable_band_1 )
					active_flag = 0;

				if( active_flag )
				{
					s = &((cast_handle->sections)[i]);

					/* Processing derived from macro kerSosFiltDirectForm2TransParaExtState */
					out = s->state_1[k] + s->b0 * in  + (realtype)SOS_FLOAT_BIAS;
					s->state_1[k] = (in - out) * s->b1 + s->state_2[k];
					s->state_2[k] = s->b2 * in - s->a2 * out;

					in = out;
				}
			}
			rp_out_buf[j+k] = out;
		}
	}
	
	return(OKAY);
}
