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
#pragma once

struct FilterPreviewLevelCoeffs {
	float rms;
	float rise;
	float fall;
	float peak_release;
	int peak_hold_frames;
};

// The preview's level follower: an rms leveller, and the peak follower behind DFXG_FILTER_PREVIEW_PEAK_CAP with its
// remaining hold in frames.
struct FilterPreviewLeveller {
	float mean_square;
	float gain;
	float peak_env;
	int peak_hold;
};

#define DFXG_FILTER_PREVIEW_TARGET_RMS   0.1f
#define DFXG_FILTER_PREVIEW_MAX_GAIN     100.0f
#define DFXG_FILTER_PREVIEW_RMS_MS       100.0
#define DFXG_FILTER_PREVIEW_RISE_MS      400.0
#define DFXG_FILTER_PREVIEW_FALL_MS      10.0
// The applied gain is capped so the levelled peak stays at or below PEAK_CAP (never below unity gain): an
// instant-attack peak follower of the unlevelled signal, held for PEAK_HOLD_MS after each peak (longer than half a
// 20 Hz period, so it does not sag between a low tone's crests and flat-top them) and then released over
// PEAK_RELEASE_MS. A crest within PEAK_HOLD_RELOAD of the held peak reloads the hold too, since a steady tone's sampled
// crests differ slightly from cycle to cycle. The rms leveller above is
// too slow for a sudden loud signal after a quiet one (a louder passage, or a switch between previews, which keeps
// the leveller's gain); the cap pulls the gain down on the first loud sample instead of letting it burst.
#define DFXG_FILTER_PREVIEW_PEAK_CAP     0.25f
#define DFXG_FILTER_PREVIEW_PEAK_RELEASE_MS  100.0
#define DFXG_FILTER_PREVIEW_PEAK_HOLD_MS     50.0
#define DFXG_FILTER_PREVIEW_PEAK_HOLD_RELOAD 0.9f
#define DFXG_FILTER_PREVIEW_LIMIT        0.9f
#define DFXG_FILTER_PREVIEW_SILENCE_MEAN_SQUARE  1e-8f

FilterPreviewLevelCoeffs filterPreviewLevelCoeffs(int sample_rate);
void filterPreviewLevellerReset(FilterPreviewLeveller *leveller);
void filterPreviewLevellerRescaleHold(FilterPreviewLeveller *leveller, int old_hold_frames, int new_hold_frames);
void filterPreviewLevelFrame(float *fp_frame, int i_num_channels, const FilterPreviewLevelCoeffs *cp_coeffs,
	FilterPreviewLeveller *leveller, bool b_update);
