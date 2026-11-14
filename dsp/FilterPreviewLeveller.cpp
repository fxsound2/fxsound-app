/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
	www.theremino.com (2025)
	
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
#include <math.h>

#include "FilterPreviewLeveller.h"

FilterPreviewLevelCoeffs filterPreviewLevelCoeffs(int sample_rate)
{
	FilterPreviewLevelCoeffs coeffs;
	double fs = (sample_rate > 0) ? (double)sample_rate : 44100.0;

	coeffs.rms = (float)(1.0 - exp(-1000.0 / (DFXG_FILTER_PREVIEW_RMS_MS * fs)));
	coeffs.rise = (float)(1.0 - exp(-1000.0 / (DFXG_FILTER_PREVIEW_RISE_MS * fs)));
	coeffs.fall = (float)(1.0 - exp(-1000.0 / (DFXG_FILTER_PREVIEW_FALL_MS * fs)));
	coeffs.peak_release = (float)(1.0 - exp(-1000.0 / (DFXG_FILTER_PREVIEW_PEAK_RELEASE_MS * fs)));
	coeffs.peak_hold_frames = (int)(fs * DFXG_FILTER_PREVIEW_PEAK_HOLD_MS / 1000.0 + 0.5);
	return coeffs;
}

void filterPreviewLevellerReset(FilterPreviewLeveller *leveller)
{
	leveller->mean_square = 0.0f;
	leveller->gain = 1.0f;
	leveller->peak_env = 0.0f;
	leveller->peak_hold = 0;
}

// Keeps the remaining peak hold at the same fraction of the hold time when the sample rate changes (the same
// rescale the engine applies to its ramps).
void filterPreviewLevellerRescaleHold(FilterPreviewLeveller *leveller, int old_hold_frames, int new_hold_frames)
{
	if (old_hold_frames <= 0 || leveller->peak_hold >= old_hold_frames)
	{
		leveller->peak_hold = (leveller->peak_hold > 0) ? new_hold_frames : 0;
		return;
	}

	leveller->peak_hold = (int)(((long long)leveller->peak_hold * (long long)new_hold_frames) / (long long)old_hold_frames);
}

// leveller->peak_env / peak_hold track on every frame, also when b_update freezes the rms gain.
void filterPreviewLevelFrame(float *fp_frame, int i_num_channels, const FilterPreviewLevelCoeffs *cp_coeffs,
	FilterPreviewLeveller *leveller, bool b_update)
{
	float sum = 0.0f;
	float desired_gain = leveller->gain;
	float peak = 0.0f;
	float applied_gain;
	int channel;

	if (i_num_channels <= 0)
	{
		return;
	}

	if (b_update)
	{
		for (channel = 0; channel < i_num_channels; channel++)
		{
			sum += fp_frame[channel] * fp_frame[channel];
		}
		leveller->mean_square += cp_coeffs->rms * (sum / (float)i_num_channels - leveller->mean_square);
		if (leveller->mean_square < 1e-20f)
		{
			leveller->mean_square = 0.0f;
		}

		if (leveller->mean_square > DFXG_FILTER_PREVIEW_SILENCE_MEAN_SQUARE)
		{
			desired_gain = DFXG_FILTER_PREVIEW_TARGET_RMS / sqrtf(leveller->mean_square);
			if (desired_gain > DFXG_FILTER_PREVIEW_MAX_GAIN)
			{
				desired_gain = DFXG_FILTER_PREVIEW_MAX_GAIN;
			}
			else if (desired_gain < 1.0f)
			{
				desired_gain = 1.0f;
			}
		}
		leveller->gain += ((desired_gain > leveller->gain) ? cp_coeffs->rise : cp_coeffs->fall) * (desired_gain - leveller->gain);
	}

	for (channel = 0; channel < i_num_channels; channel++)
	{
		const float magnitude = fabsf(fp_frame[channel]);

		peak = (magnitude > peak) ? magnitude : peak;
	}
	if (peak >= leveller->peak_env)
	{
		leveller->peak_env = peak;
		leveller->peak_hold = cp_coeffs->peak_hold_frames;
	}
	else if (peak >= DFXG_FILTER_PREVIEW_PEAK_HOLD_RELOAD * leveller->peak_env)
	{
		leveller->peak_hold = cp_coeffs->peak_hold_frames;
	}
	else if (leveller->peak_hold > 0)
	{
		leveller->peak_hold--;
	}
	else
	{
		leveller->peak_env += cp_coeffs->peak_release * (peak - leveller->peak_env);
		if (leveller->peak_env < 1e-20f)
		{
			leveller->peak_env = 0.0f;
		}
	}
	applied_gain = leveller->gain;
	if (applied_gain > 1.0f && leveller->peak_env * applied_gain > DFXG_FILTER_PREVIEW_PEAK_CAP)
	{
		applied_gain = DFXG_FILTER_PREVIEW_PEAK_CAP / leveller->peak_env;
		if (applied_gain < 1.0f)
		{
			applied_gain = 1.0f;
		}
	}

	for (channel = 0; channel < i_num_channels; channel++)
	{
		float sample = fp_frame[channel] * applied_gain;
		float magnitude = fabsf(sample);

		if (magnitude > DFXG_FILTER_PREVIEW_LIMIT)
		{
			float over = (magnitude - DFXG_FILTER_PREVIEW_LIMIT) / (1.0f - DFXG_FILTER_PREVIEW_LIMIT);
			magnitude = DFXG_FILTER_PREVIEW_LIMIT + (1.0f - DFXG_FILTER_PREVIEW_LIMIT) * tanhf(over);
			sample = (sample < 0.0f) ? -magnitude : magnitude;
		}
		fp_frame[channel] = sample;
	}
}
