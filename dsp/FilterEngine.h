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

#include <atomic>

#include "DfxDsp.h"
#include "FiltChain.h"
#include "FiltLowCut.h"
#include "FilterPreviewLeveller.h"

struct FilterIirParams {
	FiltChainParams chain;
	int sample_rate;
	unsigned int reset_serial;
};

#define FILTER_IIR_NUM_SLOTS   3
#define FILTER_IIR_SLOT_DIRTY  0x4
#define FILTER_IIR_SLOT_MASK   0x3

struct FilterIirExchange {
	FilterIirParams slots[FILTER_IIR_NUM_SLOTS];
	int front_slot;
	int back_slot;
	std::atomic<int> middle_slot;
};

#define FILTER_SOURCE_DRY  0
#define FILTER_SOURCE_IIR  1
#define FILTER_NUM_SOURCES 2

#define FILTER_CROSSFADE_MS  20.0
#define FILTER_FADE_MS       10.0

struct FilterAudioState {
	int ramp_rate;
	int crossfade_frames;
	int fade_frames;

	int source;
	int crossfade_from;
	int crossfade_pos;
	int gain_pos;
	int gain_dir;

	int preview_pos;
	int preview_mask;
	// Non-zero while the removed signal crossfades from preview_mask to this mask (a change between two non-empty
	// preview selections while the preview is audible); preview_switch_pos runs 0..crossfade_frames.
	int preview_mask_incoming;
	int preview_switch_pos;
	FilterPreviewLeveller preview_leveller;
	FilterPreviewLevelCoeffs preview_level_coeffs;

	FilterIirParams iir_params[2];
	FiltChainState iir_states[2];
	int iir_cur;
	bool iir_crossfading;
	int iir_crossfade_pos;
	bool iir_running;
	int iir_channels;
	unsigned int format_serial;
	bool iir_keep_blocked;
	FilterIirParams iir_taken;
	bool iir_taken_pending;
};

struct FilterSettings {
	bool filters_on;
	bool low_on;
	float low_hz;
	bool high_on;
	float high_hz;
	bool mono_on;
	float mono_hz;
};

int filterDesignedMaskFor(const FilterSettings &settings);
int filterOwnOnMaskFor(const FilterSettings &settings);
int filterAudibleMaskFor(const FilterSettings &settings);
int filterSwitchedOnMaskFor(const FilterSettings &settings);
int filterPreviewMaskFor(int selection, const FilterSettings &settings);
bool filterPreviewAvailableFor(int preview_flags, const FilterSettings &settings);
int filterPreviewSelectionFor(int requested, const FilterSettings &settings);
bool filterIirNeededFor(int selection, const FilterSettings &settings);
bool filterChangeNeedsReset(int current_selection, const FilterSettings &current, int next_selection,
	const FilterSettings &next);
int filterPreviewAfterSwitch(int selection, int stage_bit, bool on);
FilterSettings filterSettingsWithStage(FilterSettings settings, int stage_bit, bool on, float hz);
bool filterStageOn(const FilterSettings &settings, int stage_bit);
float filterStageHz(const FilterSettings &settings, int stage_bit);
void filterDesignIirParams(int sample_rate, int slope_db, float low_cut_hz, float high_cut_hz, float mono_hz,
	unsigned int reset_serial, FilterIirParams *params);
void filterDesignParams(int sample_rate, int slope_db, const FilterSettings &settings,
	unsigned int reset_serial, FilterIirParams *params);
void filterIirExchangeInit(FilterIirExchange *exchange);
void filterIirExchangePublish(FilterIirExchange *exchange, const FilterIirParams *params);
const FilterIirParams *filterIirExchangeAdopt(FilterIirExchange *exchange, bool *bp_took_new);
void filterAudioStateInit(FilterAudioState *state);
void filterAudioStateSetRate(FilterAudioState *state, int sample_rate);
void filterIirPoll(FilterAudioState *state, FilterIirExchange *exchange, int sample_rate, unsigned int reset_serial);
bool filterIirReady(const FilterAudioState *state, int sample_rate, unsigned int reset_serial);
void filterIirStop(FilterAudioState *state);
void filterAudioStateFormatChanged(FilterAudioState *state);
void filterAdoptPreviewMask(FilterAudioState *state, int i_preview_mask);
bool filterStepSource(FilterAudioState *state, int desired);
int filterRunSegment(FilterAudioState *state, float *fp_buffer, int i_num_frames,
	int i_num_channels, int i_processed_channels, int i_preview_mask);
int filterDesiredSourceFor(const FilterAudioState *state, bool filter_on, int sample_rate, unsigned int reset_serial);

template <typename DesiredSourceFn>
inline void filterProcessBuffer(FilterAudioState *state, FilterIirExchange *exchange, int sample_rate,
	unsigned int reset_serial, unsigned int format_serial, float *fp_buffer,
	int i_num_frames, int i_num_channels, int i_preview_mask, DesiredSourceFn desired_source)
{
	const int processed_channels = (i_num_channels < FILT_LOW_CUT_MAX_CHANNELS) ? i_num_channels : FILT_LOW_CUT_MAX_CHANNELS;
	int frame = 0;
	int desired;

	if (i_num_channels <= 0 || i_num_frames <= 0)
	{
		return;
	}

	filterAudioStateSetRate(state, sample_rate);
	if (format_serial != state->format_serial)
	{
		filterAudioStateFormatChanged(state);
		state->format_serial = format_serial;
	}
	filterIirPoll(state, exchange, sample_rate, reset_serial);
	desired = desired_source();

	if (state->source == FILTER_SOURCE_DRY && desired == FILTER_SOURCE_DRY && state->crossfade_from < 0 &&
		state->gain_pos >= state->fade_frames && state->preview_pos == 0 && i_preview_mask == 0)
	{
		filterIirStop(state);
		state->gain_dir = 0;
		filterAdoptPreviewMask(state, i_preview_mask);
		return;
	}

	while (frame < i_num_frames)
	{
		int switches = 0;

		if (frame > 0)
		{
			filterIirPoll(state, exchange, sample_rate, reset_serial);
			desired = desired_source();
		}
		while (filterStepSource(state, desired) && ++switches < 3)
		{
			desired = desired_source();
		}

		frame += filterRunSegment(state, &fp_buffer[(size_t)frame * (size_t)i_num_channels], i_num_frames - frame,
			i_num_channels, processed_channels, i_preview_mask);
	}
}
