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

#include "FilterEngine.h"

static_assert(DfxDsp::CUT_Q == (float)FILT_LOW_CUT_Q_NEUTRAL, "DfxDsp's cut Q must match FiltChain's fixed Q");
static_assert(DfxDsp::CUT_SLOPES_DB[0] == FILT_LOW_CUT_SLOPE_MIN_DB &&
	DfxDsp::CUT_SLOPES_DB[DfxDsp::CUT_NUM_SLOPES - 1] == FILT_LOW_CUT_SLOPE_MAX_DB, "DfxDsp's cut slopes must match FiltLowCut's");
static_assert(DfxDsp::PreviewNone == 0 && DfxDsp::PreviewLowCut == FILT_CHAIN_CHANGED_LOW_CUT &&
	DfxDsp::PreviewHighCut == FILT_CHAIN_CHANGED_HIGH_CUT && DfxDsp::PreviewMonoBass == FILT_CHAIN_CHANGED_MONO &&
	(DfxDsp::PreviewAll & FILT_CHAIN_STAGES_ALL) == 0, "DfxDsp's individual filter previews must be FiltChain's stage bits");

// Stages whose slider is not at Off. These are designed (and can be previewed) whatever the switches say.
int filterDesignedMaskFor(const FilterSettings &settings)
{
	return ((settings.low_hz > 0.0f) ? FILT_CHAIN_CHANGED_LOW_CUT : 0) |
		((settings.high_hz < (float)DfxDsp::HIGH_CUT_OFF_HZ) ? FILT_CHAIN_CHANGED_HIGH_CUT : 0) |
		((settings.mono_hz > 0.0f) ? FILT_CHAIN_CHANGED_MONO : 0);
}

// Designed stages whose own switch is on: what the global preview plays, and what is heard when global is on.
int filterOwnOnMaskFor(const FilterSettings &settings)
{
	return filterDesignedMaskFor(settings) & ((settings.low_on ? FILT_CHAIN_CHANGED_LOW_CUT : 0) |
		(settings.high_on ? FILT_CHAIN_CHANGED_HIGH_CUT : 0) | (settings.mono_on ? FILT_CHAIN_CHANGED_MONO : 0));
}

// Stages that are heard: global on, own switch on, slider not at Off.
int filterAudibleMaskFor(const FilterSettings &settings)
{
	return settings.filters_on ? filterOwnOnMaskFor(settings) : 0;
}

// Stages whose own switch is on, whatever their slider says.
int filterSwitchedOnMaskFor(const FilterSettings &settings)
{
	return (settings.low_on ? FILT_CHAIN_CHANGED_LOW_CUT : 0) | (settings.high_on ? FILT_CHAIN_CHANGED_HIGH_CUT : 0) |
		(settings.mono_on ? FILT_CHAIN_CHANGED_MONO : 0);
}

// Stages whose removed signals a preview selection plays (summed). Nothing with global off; otherwise own switches
// are ignored, except that the global preview only plays filters whose own switch is on. A selected stage whose
// slider is at Off has no sections, so it contributes silence.
int filterPreviewMaskFor(int selection, const FilterSettings &settings)
{
	if (!settings.filters_on)
	{
		return 0;
	}
	if (selection & DfxDsp::PreviewAll)
	{
		return filterSwitchedOnMaskFor(settings);
	}

	return selection & FILT_CHAIN_STAGES_ALL;
}

// True when every flag in preview_flags can be previewed. PreviewNone always can; nothing else with global off.
// With global on: an individual filter always (silence with its slider at Off), the global preview when at least
// one filter has its own switch on.
bool filterPreviewAvailableFor(int preview_flags, const FilterSettings &settings)
{
	if (preview_flags == DfxDsp::PreviewNone)
	{
		return true;
	}
	if (!settings.filters_on || (preview_flags & ~(FILT_CHAIN_STAGES_ALL | DfxDsp::PreviewAll)))
	{
		return false;
	}

	return !(preview_flags & DfxDsp::PreviewAll) || filterPreviewMaskFor(DfxDsp::PreviewAll, settings) != 0;
}

// The selection actually kept for a requested one: nothing with global off; PreviewAll wins over individual flags,
// and drops when unavailable.
int filterPreviewSelectionFor(int requested, const FilterSettings &settings)
{
	if (!settings.filters_on)
	{
		return DfxDsp::PreviewNone;
	}
	if (requested & DfxDsp::PreviewAll)
	{
		return filterPreviewAvailableFor(DfxDsp::PreviewAll, settings) ? (int)DfxDsp::PreviewAll : (int)DfxDsp::PreviewNone;
	}

	return requested & FILT_CHAIN_STAGES_ALL;
}

// Whether the IIR chain has to run (power aside): something is heard or something is previewed.
bool filterIirNeededFor(int selection, const FilterSettings &settings)
{
	return filterAudibleMaskFor(settings) != 0 || filterPreviewMaskFor(selection, settings) != 0;
}

// Whether a change must restart the IIR chain from a fresh design: it goes from not needed to needed, so no history
// from before the pause (nor a design made for it) is reused.
bool filterChangeNeedsReset(int current_selection, const FilterSettings &current, int next_selection,
	const FilterSettings &next)
{
	return !filterIirNeededFor(current_selection, current) && filterIirNeededFor(next_selection, next);
}

// The preview selection after a filter's own switch changes: switching it off also releases its own preview.
int filterPreviewAfterSwitch(int selection, int stage_bit, bool on)
{
	return on ? selection : (selection & ~stage_bit);
}

// The per-filter fields, picked by a FILT_CHAIN_CHANGED_* stage bit (one bit; anything else picks nothing).
FilterSettings filterSettingsWithStage(FilterSettings settings, int stage_bit, bool on, float hz)
{
	switch (stage_bit)
	{
	case FILT_CHAIN_CHANGED_LOW_CUT:
		settings.low_on = on;
		settings.low_hz = hz;
		break;
	case FILT_CHAIN_CHANGED_HIGH_CUT:
		settings.high_on = on;
		settings.high_hz = hz;
		break;
	case FILT_CHAIN_CHANGED_MONO:
		settings.mono_on = on;
		settings.mono_hz = hz;
		break;
	}
	return settings;
}

bool filterStageOn(const FilterSettings &settings, int stage_bit)
{
	switch (stage_bit)
	{
	case FILT_CHAIN_CHANGED_LOW_CUT:
		return settings.low_on;
	case FILT_CHAIN_CHANGED_HIGH_CUT:
		return settings.high_on;
	case FILT_CHAIN_CHANGED_MONO:
		return settings.mono_on;
	}
	return false;
}

float filterStageHz(const FilterSettings &settings, int stage_bit)
{
	switch (stage_bit)
	{
	case FILT_CHAIN_CHANGED_LOW_CUT:
		return settings.low_hz;
	case FILT_CHAIN_CHANGED_HIGH_CUT:
		return settings.high_hz;
	case FILT_CHAIN_CHANGED_MONO:
		return settings.mono_hz;
	}
	return 0.0f;
}

void filterDesignIirParams(int sample_rate, int slope_db, float low_cut_hz, float high_cut_hz, float mono_hz,
	unsigned int reset_serial, FilterIirParams *params)
{
	*params = {};
	if (sample_rate > 0)
	{
		filtChainDesign(slope_db, (double)low_cut_hz, (double)high_cut_hz, (double)mono_hz, (double)sample_rate, &params->chain);
	}
	params->sample_rate = sample_rate;
	params->reset_serial = reset_serial;
}

// What the designer publishes: every stage whose slider is not at Off is designed, the switches only set the audible
// mask. Switching a filter on or off therefore keeps the layout, and the engine crossfades it without a restart.
void filterDesignParams(int sample_rate, int slope_db, const FilterSettings &settings,
	unsigned int reset_serial, FilterIirParams *params)
{
	const float high_hz = (settings.high_hz < (float)DfxDsp::HIGH_CUT_OFF_HZ) ? settings.high_hz : 0.0f;

	filterDesignIirParams(sample_rate, slope_db, settings.low_hz, high_hz, settings.mono_hz, reset_serial, params);
	params->chain.audible_mask &= filterAudibleMaskFor(settings);
}

void filterIirExchangeInit(FilterIirExchange *exchange)
{
	int slot;

	for (slot = 0; slot < FILTER_IIR_NUM_SLOTS; slot++)
	{
		exchange->slots[slot] = {};
	}
	exchange->front_slot = 0;
	exchange->middle_slot.store(1, std::memory_order_relaxed);
	exchange->back_slot = 2;
}

void filterIirExchangePublish(FilterIirExchange *exchange, const FilterIirParams *params)
{
	exchange->slots[exchange->back_slot] = *params;
	int previous = exchange->middle_slot.exchange(exchange->back_slot | FILTER_IIR_SLOT_DIRTY, std::memory_order_acq_rel);
	exchange->back_slot = previous & FILTER_IIR_SLOT_MASK;
}

const FilterIirParams *filterIirExchangeAdopt(FilterIirExchange *exchange, bool *bp_took_new)
{
	bool took_new = false;

	if (exchange->middle_slot.load(std::memory_order_relaxed) & FILTER_IIR_SLOT_DIRTY)
	{
		int previous = exchange->middle_slot.exchange(exchange->front_slot, std::memory_order_acq_rel);
		exchange->front_slot = previous & FILTER_IIR_SLOT_MASK;
		took_new = true;
	}
	if (bp_took_new != NULL)
	{
		*bp_took_new = took_new;
	}

	return &exchange->slots[exchange->front_slot];
}

static inline float filterRampWeight(int pos, int len, float step)
{
	return (pos >= len) ? 1.0f : (float)pos * step;
}

static int filterRescaleRamp(int pos, int old_len, int new_len)
{
	if (old_len <= 0 || pos >= old_len)
	{
		return (pos > 0) ? new_len : 0;
	}

	return (int)(((long long)pos * (long long)new_len) / (long long)old_len);
}

void filterAudioStateInit(FilterAudioState *state)
{
	int slot;

	state->ramp_rate = -1;
	state->crossfade_frames = 1;
	state->fade_frames = 1;
	state->source = FILTER_SOURCE_DRY;
	state->crossfade_from = -1;
	state->crossfade_pos = 0;
	state->gain_pos = state->fade_frames;
	state->gain_dir = 0;
	state->preview_pos = 0;
	state->preview_mask = 0;
	state->preview_mask_incoming = 0;
	state->preview_switch_pos = 0;
	filterPreviewLevellerReset(&state->preview_leveller);
	state->preview_level_coeffs = {};
	for (slot = 0; slot < 2; slot++)
	{
		state->iir_params[slot] = {};
		filtChainResetState(&state->iir_states[slot]);
	}
	state->iir_cur = 0;
	state->iir_crossfading = false;
	state->iir_crossfade_pos = 0;
	state->iir_running = false;
	state->iir_taken = {};
	state->iir_taken_pending = false;
	state->iir_channels = 0;
	state->format_serial = 0;
	state->iir_keep_blocked = false;
}

void filterAudioStateSetRate(FilterAudioState *state, int sample_rate)
{
	double fs;
	int crossfade_frames, fade_frames;
	const int previous_hold_frames = state->preview_level_coeffs.peak_hold_frames;

	if (sample_rate == state->ramp_rate)
	{
		return;
	}

	fs = (sample_rate > 0) ? (double)sample_rate : 44100.0;
	crossfade_frames = (int)(fs * FILTER_CROSSFADE_MS / 1000.0 + 0.5);
	fade_frames = (int)(fs * FILTER_FADE_MS / 1000.0 + 0.5);
	if (crossfade_frames < 1)
	{
		crossfade_frames = 1;
	}
	if (fade_frames < 1)
	{
		fade_frames = 1;
	}

	state->crossfade_pos = filterRescaleRamp(state->crossfade_pos, state->crossfade_frames, crossfade_frames);
	state->preview_pos = filterRescaleRamp(state->preview_pos, state->crossfade_frames, crossfade_frames);
	state->preview_switch_pos = filterRescaleRamp(state->preview_switch_pos, state->crossfade_frames, crossfade_frames);
	state->iir_crossfade_pos = filterRescaleRamp(state->iir_crossfade_pos, state->crossfade_frames, crossfade_frames);
	state->gain_pos = filterRescaleRamp(state->gain_pos, state->fade_frames, fade_frames);
	state->crossfade_frames = crossfade_frames;
	state->fade_frames = fade_frames;

	state->preview_level_coeffs = filterPreviewLevelCoeffs(sample_rate);
	filterPreviewLevellerRescaleHold(&state->preview_leveller, previous_hold_frames,
		state->preview_level_coeffs.peak_hold_frames);
	state->ramp_rate = sample_rate;
}

static inline int filterClampCount(int count, int max_count)
{
	return (count > max_count) ? max_count : ((count < 0) ? 0 : count);
}

static void filterIirCopyParams(FilterIirParams *dst, const FilterIirParams *src)
{
	*dst = *src;
	dst->chain.num_low_cut = filterClampCount(dst->chain.num_low_cut, FILT_LOW_CUT_MAX_SECTIONS);
	dst->chain.num_low_cut_mirror = filterClampCount(dst->chain.num_low_cut_mirror, FILT_LOW_CUT_MAX_SECTIONS);
	dst->chain.num_high_cut = filterClampCount(dst->chain.num_high_cut, FILT_LOW_CUT_MAX_SECTIONS);
	dst->chain.num_high_cut_mirror = filterClampCount(dst->chain.num_high_cut_mirror, FILT_LOW_CUT_MAX_SECTIONS);
	dst->chain.num_mono_side = filterClampCount(dst->chain.num_mono_side, FILT_CHAIN_MONO_SECTIONS);
	dst->chain.num_mono_side_mirror = filterClampCount(dst->chain.num_mono_side_mirror, FILT_CHAIN_MONO_SECTIONS);
	dst->chain.audible_mask &= FILT_CHAIN_STAGES_ALL;
}

static void filterIirFinishCrossfade(FilterAudioState *state)
{
	state->iir_cur ^= 1;
	state->iir_crossfading = false;
	state->iir_crossfade_pos = 0;
}

void filterIirPoll(FilterAudioState *state, FilterIirExchange *exchange, int sample_rate, unsigned int reset_serial)
{
	const FilterIirParams *published;
	const FilterIirParams *current;
	bool took_new = false;
	int next;

	published = filterIirExchangeAdopt(exchange, &took_new);
	if (took_new)
	{
		state->iir_taken = *published;
		state->iir_taken_pending = true;
	}
	if (!state->iir_taken_pending || state->iir_crossfading)
	{
		return;
	}

	if ((int)(state->iir_taken.reset_serial - reset_serial) < 0 || state->iir_taken.sample_rate != sample_rate)
	{
		state->iir_taken_pending = false;
		return;
	}
	if (state->iir_taken.reset_serial != reset_serial)
	{
		return;
	}
	state->iir_taken_pending = false;
	state->iir_keep_blocked = false;
	published = &state->iir_taken;

	if (!state->iir_running)
	{
		filterIirCopyParams(&state->iir_params[state->iir_cur], published);
		return;
	}

	next = state->iir_cur ^ 1;
	current = &state->iir_params[state->iir_cur];
	filterIirCopyParams(&state->iir_params[next], published);
	// Per stage: a stage whose section layout is unchanged keeps its history, only changed stages restart from zero.
	// Toggling the high cut or mono bass therefore never restarts the low cut (which would thump on bass content).
	filtChainCarryState(&current->chain, &state->iir_params[next].chain, &state->iir_states[state->iir_cur],
		&state->iir_states[next]);
	state->iir_crossfading = true;
	state->iir_crossfade_pos = 0;
}

bool filterIirReady(const FilterAudioState *state, int sample_rate, unsigned int reset_serial)
{
	const FilterIirParams *params = &state->iir_params[state->iir_crossfading ? (state->iir_cur ^ 1) : state->iir_cur];

	return params->sample_rate == sample_rate && params->reset_serial == reset_serial;
}

static void filterIirStart(FilterAudioState *state)
{
	if (state->iir_running)
	{
		return;
	}
	if (state->iir_crossfading)
	{
		filterIirFinishCrossfade(state);
	}
	filtChainResetState(&state->iir_states[state->iir_cur]);
	state->iir_running = true;
	state->iir_channels = FILT_LOW_CUT_MAX_CHANNELS;
}

void filterIirStop(FilterAudioState *state)
{
	if (state->iir_crossfading)
	{
		filterIirFinishCrossfade(state);
	}
	state->iir_running = false;
}

void filterAudioStateFormatChanged(FilterAudioState *state)
{
	filterIirStop(state);
	state->iir_keep_blocked = true;
	if (state->source != FILTER_SOURCE_DRY || state->crossfade_from >= 0)
	{
		state->crossfade_from = -1;
		state->crossfade_pos = 0;
		state->gain_pos = 0;
		state->gain_dir = 0;
	}
}

// One frame through the current parameter set (crossfaded with the incoming set while one is being adopted). removed
// gets the removed signal of removed_mask; removed_b, when not NULL, that of removed_mask_b from the same run.
static inline void filterIirRunFrame(FilterAudioState *state, const float *in, int channels, float incoming_weight,
	int removed_mask, float *filtered, float *removed, int removed_mask_b, float *removed_b)
{
	const int cur = state->iir_cur;

	filtChainProcessFrameDual(in, filtered, removed, removed_mask, removed_b, removed_mask_b, channels,
		&state->iir_params[cur].chain, &state->iir_states[cur]);

	if (state->iir_crossfading)
	{
		const int next = cur ^ 1;
		float incoming_filtered[FILT_CHAIN_MAX_CHANNELS];
		float incoming_removed[FILT_CHAIN_MAX_CHANNELS];
		float incoming_removed_b[FILT_CHAIN_MAX_CHANNELS];
		int channel;

		filtChainProcessFrameDual(in, incoming_filtered, incoming_removed, removed_mask,
			(removed_b != NULL) ? incoming_removed_b : NULL, removed_mask_b, channels, &state->iir_params[next].chain,
			&state->iir_states[next]);
		for (channel = 0; channel < channels; channel++)
		{
			filtered[channel] += incoming_weight * (incoming_filtered[channel] - filtered[channel]);
			removed[channel] += incoming_weight * (incoming_removed[channel] - removed[channel]);
		}
		if (removed_b != NULL)
		{
			for (channel = 0; channel < channels; channel++)
			{
				removed_b[channel] += incoming_weight * (incoming_removed_b[channel] - removed_b[channel]);
			}
		}
	}
}

static void filterFinishPreviewSwitch(FilterAudioState *state)
{
	state->preview_mask = state->preview_mask_incoming;
	state->preview_mask_incoming = 0;
	state->preview_switch_pos = 0;
}

// Preview selection changes. Starting or stopping a preview (empty <-> non-empty) ramps preview_pos between the normal
// programme and the removed signal (filterRunSegment); a preview starts with a fresh leveller. A change between two
// non-empty selections while the preview is audible crossfades the removed signal itself, from preview_mask to
// preview_mask_incoming over crossfade_frames, with preview_pos and the leveller left running, so the normal programme
// is never heard during the switch and the loudness stays continuous. During such a crossfade:
//  - going back to preview_mask reverses the crossfade from where it is (the mix is unchanged, so no step);
//  - any other non-empty selection waits until the crossfade ends (at most crossfade_frames), then starts its own
//    crossfade from the selection just faded in (jumping to it mid-crossfade would step the removed signal);
//  - an empty selection ramps preview_pos out while the crossfade continues; once silent the crossfade is dropped.
void filterAdoptPreviewMask(FilterAudioState *state, int i_preview_mask)
{
	if (state->preview_pos == 0)
	{
		state->preview_mask_incoming = 0;
		state->preview_switch_pos = 0;
		if (state->preview_mask != i_preview_mask)
		{
			state->preview_mask = i_preview_mask;
			filterPreviewLevellerReset(&state->preview_leveller);
		}
		return;
	}
	if (i_preview_mask == 0)
	{
		return;
	}

	if (state->preview_mask_incoming == 0)
	{
		if (i_preview_mask != state->preview_mask)
		{
			state->preview_mask_incoming = i_preview_mask;
			state->preview_switch_pos = 0;
		}
	}
	else if (i_preview_mask == state->preview_mask)
	{
		state->preview_mask = state->preview_mask_incoming;
		state->preview_mask_incoming = i_preview_mask;
		state->preview_switch_pos = state->crossfade_frames - state->preview_switch_pos;
		if (state->preview_switch_pos >= state->crossfade_frames)
		{
			filterFinishPreviewSwitch(state);
		}
	}
}

static void filterSettleRamps(FilterAudioState *state)
{
	if (state->crossfade_from >= 0 && state->crossfade_pos >= state->crossfade_frames)
	{
		state->crossfade_from = -1;
		state->crossfade_pos = 0;
	}
	if ((state->gain_dir > 0 && state->gain_pos >= state->fade_frames) || (state->gain_dir < 0 && state->gain_pos <= 0))
	{
		state->gain_dir = 0;
	}
	if (state->iir_crossfading && state->iir_crossfade_pos >= state->crossfade_frames)
	{
		filterIirFinishCrossfade(state);
	}
	if (state->preview_mask_incoming != 0 && state->preview_switch_pos >= state->crossfade_frames)
	{
		filterFinishPreviewSwitch(state);
	}
}

bool filterStepSource(FilterAudioState *state, int desired)
{
	filterSettleRamps(state);

	if (state->crossfade_from >= 0)
	{
		if (desired == state->crossfade_from)
		{
			state->crossfade_from = state->source;
			state->source = desired;
			state->crossfade_pos = state->crossfade_frames - state->crossfade_pos;
			filterSettleRamps(state);
		}
		return false;
	}

	if (desired == state->source)
	{
		state->gain_dir = (state->gain_pos < state->fade_frames) ? 1 : 0;
		return false;
	}

	if (state->gain_pos >= state->fade_frames)
	{
		state->crossfade_from = state->source;
		state->source = desired;
		state->crossfade_pos = 0;
		state->gain_dir = 0;
		return false;
	}

	if (state->gain_pos > 0)
	{
		state->gain_dir = -1;
		return false;
	}

	state->source = desired;
	state->gain_dir = 1;
	return true;
}

int filterRunSegment(FilterAudioState *state, float *fp_buffer, int i_num_frames,
	int i_num_channels, int i_processed_channels, int i_preview_mask)
{
	filterSettleRamps(state);
	filterAdoptPreviewMask(state, i_preview_mask);

	const float crossfade_step = 1.0f / (float)state->crossfade_frames;
	const float fade_step = 1.0f / (float)state->fade_frames;
	const bool run_iir = state->source == FILTER_SOURCE_IIR || state->crossfade_from == FILTER_SOURCE_IIR;
	const bool crossfading = state->crossfade_from >= 0;
	const bool preview_switching = state->preview_mask_incoming != 0;
	bool level_steady;
	int preview_dir = 0;
	int segment = i_num_frames;
	int n, channel;

	if (run_iir)
	{
		filterIirStart(state);
		for (channel = state->iir_channels; channel < i_processed_channels; channel++)
		{
			filtChainResetChannelState(&state->iir_states[0].channels[channel]);
			filtChainResetChannelState(&state->iir_states[1].channels[channel]);
		}
		if (state->iir_channels < 2 && i_processed_channels >= 2)
		{
			// The mono bass side filters only run with two or more channels; drop whatever they held before.
			for (channel = 0; channel < FILT_CHAIN_MONO_SECTIONS; channel++)
			{
				state->iir_states[0].mono_side[channel] = {};
				state->iir_states[1].mono_side[channel] = {};
				state->iir_states[0].mono_side_mirror[channel] = {};
				state->iir_states[1].mono_side_mirror[channel] = {};
			}
		}
		state->iir_channels = i_processed_channels;
	}
	else
	{
		filterIirStop(state);
	}
	level_steady = state->source != FILTER_SOURCE_DRY && !crossfading &&
		state->gain_dir == 0 && state->gain_pos >= state->fade_frames;

	// Any non-empty request keeps (or ramps) the preview fully in; a different mask is crossfaded in by the removed
	// signal itself (see filterAdoptPreviewMask).
	if (i_preview_mask != 0)
	{
		if (state->preview_pos < state->crossfade_frames)
		{
			preview_dir = 1;
		}
	}
	else if (state->preview_pos > 0)
	{
		preview_dir = -1;
	}

	if (crossfading && state->crossfade_frames - state->crossfade_pos < segment)
	{
		segment = state->crossfade_frames - state->crossfade_pos;
	}
	if (state->gain_dir > 0 && state->fade_frames - state->gain_pos < segment)
	{
		segment = state->fade_frames - state->gain_pos;
	}
	else if (state->gain_dir < 0 && state->gain_pos < segment)
	{
		segment = state->gain_pos;
	}
	if (preview_dir > 0 && state->crossfade_frames - state->preview_pos < segment)
	{
		segment = state->crossfade_frames - state->preview_pos;
	}
	else if (preview_dir < 0 && state->preview_pos < segment)
	{
		segment = state->preview_pos;
	}
	if (state->iir_crossfading && state->crossfade_frames - state->iir_crossfade_pos < segment)
	{
		segment = state->crossfade_frames - state->iir_crossfade_pos;
	}
	if (preview_switching && state->crossfade_frames - state->preview_switch_pos < segment)
	{
		segment = state->crossfade_frames - state->preview_switch_pos;
	}
	if (segment < 1)
	{
		segment = 1;
	}

	for (n = 0; n < segment; n++)
	{
		float *frame = &fp_buffer[(size_t)n * (size_t)i_num_channels];
		float filtered[FILT_LOW_CUT_MAX_CHANNELS];
		float removed[FILT_LOW_CUT_MAX_CHANNELS];
		float weights[FILTER_NUM_SOURCES] = { 0.0f, 0.0f };
		float current_weight;
		float preview_mix, iir_incoming_weight, switch_weight = 0.0f;

		if (crossfading)
		{
			state->crossfade_pos++;
			current_weight = filterRampWeight(state->crossfade_pos, state->crossfade_frames, crossfade_step);
			weights[state->crossfade_from] = 1.0f - current_weight;
		}
		else
		{
			state->gain_pos += state->gain_dir;
			current_weight = filterRampWeight(state->gain_pos, state->fade_frames, fade_step);
		}
		weights[state->source] += current_weight;

		state->preview_pos += preview_dir;
		preview_mix = filterRampWeight(state->preview_pos, state->crossfade_frames, crossfade_step);
		if (state->iir_crossfading)
		{
			state->iir_crossfade_pos++;
		}
		iir_incoming_weight = filterRampWeight(state->iir_crossfade_pos, state->crossfade_frames, crossfade_step);
		if (preview_switching)
		{
			state->preview_switch_pos++;
			switch_weight = filterRampWeight(state->preview_switch_pos, state->crossfade_frames, crossfade_step);
		}

		if (run_iir)
		{
			float iir_filtered[FILT_CHAIN_MAX_CHANNELS];
			float iir_removed[FILT_CHAIN_MAX_CHANNELS];
			float iir_removed_incoming[FILT_CHAIN_MAX_CHANNELS];

			filterIirRunFrame(state, frame, i_processed_channels, iir_incoming_weight, state->preview_mask, iir_filtered,
				iir_removed, state->preview_mask_incoming, preview_switching ? iir_removed_incoming : NULL);
			if (preview_switching)
			{
				for (channel = 0; channel < i_processed_channels; channel++)
				{
					iir_removed[channel] += switch_weight * (iir_removed_incoming[channel] - iir_removed[channel]);
				}
			}
			for (channel = 0; channel < i_processed_channels; channel++)
			{
				filtered[channel] = weights[FILTER_SOURCE_DRY] * frame[channel] + weights[FILTER_SOURCE_IIR] * iir_filtered[channel];
				removed[channel] = weights[FILTER_SOURCE_IIR] * iir_removed[channel];
			}
		}
		else
		{
			for (channel = 0; channel < i_processed_channels; channel++)
			{
				filtered[channel] = weights[FILTER_SOURCE_DRY] * frame[channel];
				removed[channel] = 0.0f;
			}
		}

		if (state->preview_pos > 0)
		{
			filterPreviewLevelFrame(removed, i_processed_channels, &state->preview_level_coeffs,
				&state->preview_leveller, level_steady);
			for (channel = 0; channel < i_processed_channels; channel++)
			{
				frame[channel] = filtered[channel] + preview_mix * (removed[channel] - filtered[channel]);
			}
		}
		else
		{
			for (channel = 0; channel < i_processed_channels; channel++)
			{
				frame[channel] = filtered[channel];
			}
		}

		// Channels beyond FILT_LOW_CUT_MAX_CHANNELS are not filtered (the IIR source passes them through), but they
		// follow the same source fades and preview mix, in which they remove nothing: silent while a preview is fully in.
		if (i_num_channels > i_processed_channels)
		{
			const float programme_weight = (weights[FILTER_SOURCE_DRY] + weights[FILTER_SOURCE_IIR]) *
				((state->preview_pos > 0) ? (1.0f - preview_mix) : 1.0f);

			for (channel = i_processed_channels; channel < i_num_channels; channel++)
			{
				frame[channel] *= programme_weight;
			}
		}
	}

	filterSettleRamps(state);
	return segment;
}

int filterDesiredSourceFor(const FilterAudioState *state, bool filter_on, int sample_rate, unsigned int reset_serial)
{
	if (!filter_on)
	{
		return FILTER_SOURCE_DRY;
	}

	const bool keep_iir = state->source == FILTER_SOURCE_IIR && state->iir_running &&
		!state->iir_keep_blocked &&
		state->iir_params[state->iir_crossfading ? (state->iir_cur ^ 1) : state->iir_cur].sample_rate == sample_rate;

	if (filterIirReady(state, sample_rate, reset_serial) || keep_iir)
	{
		return FILTER_SOURCE_IIR;
	}
	return FILTER_SOURCE_DRY;
}
