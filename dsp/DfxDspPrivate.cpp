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
#include "u_DfxDsp.h"
#include "codedefs.h"
#include "DfxSdk.h"
#include "dfxp.h"
#include "prelst.h"
#include "file.h"
#include "qnt.h"
#include <string>
#include <math.h>
#include <cassert>
#include <vector>
#include <chrono>

#include "BinauralSyn.h"
#include "ptutil\dfxp\u_dfxp.h"
#include "com.h"
#include "dfxSharedUtil.h"
#include "GraphicEq.h"
#include "spectrum.h"
#include "SurroundSyn.h"

#define DFXG_REGISTRY_DFX_PRODUCT_NAME_WIDE		L"DFX"
#define DFXG_DISPLAYED_DFX_PRODUCT_NAME_WIDE    L"FxSound"
#define DFXP_VENDOR_CODE_UNIVERSAL			  23

#define DFXG_MIN_USER_PRESET_INDEX				 99 /* 0 based number of min user preset (preset number 100) */
#define DFXG_MAX_PRESET_NAME_LENGTH              128
#define DFXG_NO_PROCESSING_PRESET                0
#define DFXG_DEFAULT_PRESET_INDEX                0 /* 0 based, i.e. index of 2 corresponds to preset number 3 */
#define DFXG_FREE_PRESET_MIN_INDEX               0 
#define DFXG_FREE_PRESET_MAX_INDEX               0


#define MIDI_MIN_VALUE          0
#define MIDI_MAX_VALUE          127

static_assert(DfxDsp::LOW_CUT_MIN_Q == (float)FILT_LOW_CUT_Q_MIN && DfxDsp::LOW_CUT_MAX_Q == (float)FILT_LOW_CUT_Q_MAX &&
	DfxDsp::LOW_CUT_DEFAULT_Q == (float)FILT_LOW_CUT_Q_NEUTRAL, "DfxDsp's low cut Q range must match FiltLowCut's");
static_assert(DfxDsp::LOW_CUT_SLOPES_DB[0] == FILT_LOW_CUT_SLOPE_MIN_DB &&
	DfxDsp::LOW_CUT_SLOPES_DB[DfxDsp::LOW_CUT_NUM_SLOPES - 1] == FILT_LOW_CUT_SLOPE_MAX_DB, "DfxDsp's low cut slopes must match FiltLowCut's");
#define DFXG_LOW_CUT_PREVIEW_TARGET_RMS   0.1f
#define DFXG_LOW_CUT_PREVIEW_MAX_GAIN     100.0f
#define DFXG_LOW_CUT_PREVIEW_RMS_MS       100.0
#define DFXG_LOW_CUT_PREVIEW_RISE_MS      400.0
#define DFXG_LOW_CUT_PREVIEW_FALL_MS      10.0
#define DFXG_LOW_CUT_PREVIEW_LIMIT        0.9f
#define DFXG_LOW_CUT_PREVIEW_SILENCE_MEAN_SQUARE  1e-8f

static LowCutPreviewLevelCoeffs lowCutPreviewLevelCoeffs(int sample_rate)
{
	LowCutPreviewLevelCoeffs coeffs;
	double fs = (sample_rate > 0) ? (double)sample_rate : 44100.0;

	coeffs.rms = (float)(1.0 - exp(-1000.0 / (DFXG_LOW_CUT_PREVIEW_RMS_MS * fs)));
	coeffs.rise = (float)(1.0 - exp(-1000.0 / (DFXG_LOW_CUT_PREVIEW_RISE_MS * fs)));
	coeffs.fall = (float)(1.0 - exp(-1000.0 / (DFXG_LOW_CUT_PREVIEW_FALL_MS * fs)));
	return coeffs;
}

static void lowCutLevelPreviewFrame(float *fp_frame, int i_num_channels, const LowCutPreviewLevelCoeffs *cp_coeffs,
	float *fp_mean_square, float *fp_gain, bool b_update)
{
	float sum = 0.0f;
	float desired_gain = *fp_gain;
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
		*fp_mean_square += cp_coeffs->rms * (sum / (float)i_num_channels - *fp_mean_square);
		if (*fp_mean_square < 1e-20f)
		{
			*fp_mean_square = 0.0f;
		}

		if (*fp_mean_square > DFXG_LOW_CUT_PREVIEW_SILENCE_MEAN_SQUARE)
		{
			desired_gain = DFXG_LOW_CUT_PREVIEW_TARGET_RMS / sqrtf(*fp_mean_square);
			if (desired_gain > DFXG_LOW_CUT_PREVIEW_MAX_GAIN)
			{
				desired_gain = DFXG_LOW_CUT_PREVIEW_MAX_GAIN;
			}
			else if (desired_gain < 1.0f)
			{
				desired_gain = 1.0f;
			}
		}
		*fp_gain += ((desired_gain > *fp_gain) ? cp_coeffs->rise : cp_coeffs->fall) * (desired_gain - *fp_gain);
	}

	for (channel = 0; channel < i_num_channels; channel++)
	{
		float sample = fp_frame[channel] * *fp_gain;
		float magnitude = fabsf(sample);

		if (magnitude > DFXG_LOW_CUT_PREVIEW_LIMIT)
		{
			float over = (magnitude - DFXG_LOW_CUT_PREVIEW_LIMIT) / (1.0f - DFXG_LOW_CUT_PREVIEW_LIMIT);
			magnitude = DFXG_LOW_CUT_PREVIEW_LIMIT + (1.0f - DFXG_LOW_CUT_PREVIEW_LIMIT) * tanhf(over);
			sample = (sample < 0.0f) ? -magnitude : magnitude;
		}
		fp_frame[channel] = sample;
	}
}

static void lowCutDesignIirParams(int sample_rate, int slope_db, float q, float cutoff_hz, unsigned int reset_serial,
	LowCutIirParams *params)
{
	*params = {};
	if (sample_rate > 0)
	{
		params->num_hp_sections = filtLowCutDesign(slope_db, (double)q, (double)cutoff_hz, (double)sample_rate, 1, params->hp_sections);
		params->num_lp_sections = filtLowCutDesign(slope_db, (double)q, (double)cutoff_hz, (double)sample_rate, 0, params->lp_sections);
	}
	params->sample_rate = sample_rate;
	params->reset_serial = reset_serial;
}

static void lowCutIirExchangeInit(LowCutIirExchange *exchange)
{
	int slot;

	for (slot = 0; slot < LOW_CUT_IIR_NUM_SLOTS; slot++)
	{
		exchange->slots[slot] = {};
	}
	exchange->front_slot = 0;
	exchange->middle_slot.store(1, std::memory_order_relaxed);
	exchange->back_slot = 2;
}

static void lowCutIirExchangePublish(LowCutIirExchange *exchange, const LowCutIirParams *params)
{
	exchange->slots[exchange->back_slot] = *params;
	int previous = exchange->middle_slot.exchange(exchange->back_slot | LOW_CUT_IIR_SLOT_DIRTY, std::memory_order_acq_rel);
	exchange->back_slot = previous & LOW_CUT_IIR_SLOT_MASK;
}

static const LowCutIirParams *lowCutIirExchangeAdopt(LowCutIirExchange *exchange, bool *bp_took_new)
{
	bool took_new = false;

	if (exchange->middle_slot.load(std::memory_order_relaxed) & LOW_CUT_IIR_SLOT_DIRTY)
	{
		int previous = exchange->middle_slot.exchange(exchange->front_slot, std::memory_order_acq_rel);
		exchange->front_slot = previous & LOW_CUT_IIR_SLOT_MASK;
		took_new = true;
	}
	if (bp_took_new != NULL)
	{
		*bp_took_new = took_new;
	}

	return &exchange->slots[exchange->front_slot];
}

static bool lowCutIirSameLayout(const LowCutIirParams *a, const LowCutIirParams *b)
{
	int i;

	if (a->num_hp_sections != b->num_hp_sections || a->num_lp_sections != b->num_lp_sections)
	{
		return false;
	}
	for (i = 0; i < a->num_hp_sections; i++)
	{
		if (a->hp_sections[i].order != b->hp_sections[i].order)
		{
			return false;
		}
	}
	for (i = 0; i < a->num_lp_sections; i++)
	{
		if (a->lp_sections[i].order != b->lp_sections[i].order)
		{
			return false;
		}
	}

	return true;
}

static bool lowCutIirNeedsReset(const LowCutIirParams *params, const LowCutIirParams *history_params, bool ran_last_buffer)
{
	return !ran_last_buffer || history_params == NULL || !lowCutIirSameLayout(params, history_params);
}

static inline float lowCutRampWeight(int pos, int len, float step)
{
	return (pos >= len) ? 1.0f : (float)pos * step;
}

static int lowCutRescaleRamp(int pos, int old_len, int new_len)
{
	if (old_len <= 0 || pos >= old_len)
	{
		return (pos > 0) ? new_len : 0;
	}

	return (int)(((long long)pos * (long long)new_len) / (long long)old_len);
}

static void lowCutAudioStateInit(LowCutAudioState *state)
{
	int slot, channel;

	state->ramp_rate = -1;
	state->crossfade_frames = 1;
	state->fade_frames = 1;
	state->source = LOW_CUT_SOURCE_DRY;
	state->crossfade_from = -1;
	state->crossfade_pos = 0;
	state->gain_pos = state->fade_frames;
	state->gain_dir = 0;
	state->preview_pos = 0;
	state->preview_was_on = false;
	state->preview_mean_square = 0.0f;
	state->preview_gain = 1.0f;
	state->preview_level_coeffs = {};
	for (slot = 0; slot < 2; slot++)
	{
		state->iir_params[slot] = {};
		for (channel = 0; channel < FILT_LOW_CUT_MAX_CHANNELS; channel++)
		{
			filtLowCutResetChannelState(&state->iir_states[slot][channel]);
		}
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

static void lowCutAudioStateSetRate(LowCutAudioState *state, int sample_rate)
{
	double fs;
	int crossfade_frames, fade_frames;

	if (sample_rate == state->ramp_rate)
	{
		return;
	}

	fs = (sample_rate > 0) ? (double)sample_rate : 44100.0;
	crossfade_frames = (int)(fs * LOW_CUT_CROSSFADE_MS / 1000.0 + 0.5);
	fade_frames = (int)(fs * LOW_CUT_FADE_MS / 1000.0 + 0.5);
	if (crossfade_frames < 1)
	{
		crossfade_frames = 1;
	}
	if (fade_frames < 1)
	{
		fade_frames = 1;
	}

	state->crossfade_pos = lowCutRescaleRamp(state->crossfade_pos, state->crossfade_frames, crossfade_frames);
	state->preview_pos = lowCutRescaleRamp(state->preview_pos, state->crossfade_frames, crossfade_frames);
	state->iir_crossfade_pos = lowCutRescaleRamp(state->iir_crossfade_pos, state->crossfade_frames, crossfade_frames);
	state->gain_pos = lowCutRescaleRamp(state->gain_pos, state->fade_frames, fade_frames);
	state->crossfade_frames = crossfade_frames;
	state->fade_frames = fade_frames;

	state->preview_level_coeffs = lowCutPreviewLevelCoeffs(sample_rate);
	state->ramp_rate = sample_rate;
}

static void lowCutIirCopyParams(LowCutIirParams *dst, const LowCutIirParams *src)
{
	*dst = *src;
	if (dst->num_hp_sections > FILT_LOW_CUT_MAX_SECTIONS)
	{
		dst->num_hp_sections = FILT_LOW_CUT_MAX_SECTIONS;
	}
	else if (dst->num_hp_sections < 0)
	{
		dst->num_hp_sections = 0;
	}
	if (dst->num_lp_sections > FILT_LOW_CUT_MAX_SECTIONS)
	{
		dst->num_lp_sections = FILT_LOW_CUT_MAX_SECTIONS;
	}
	else if (dst->num_lp_sections < 0)
	{
		dst->num_lp_sections = 0;
	}
}

static void lowCutIirFinishCrossfade(LowCutAudioState *state)
{
	state->iir_cur ^= 1;
	state->iir_crossfading = false;
	state->iir_crossfade_pos = 0;
}

static void lowCutIirPoll(LowCutAudioState *state, LowCutIirExchange *exchange, int sample_rate, unsigned int reset_serial)
{
	const LowCutIirParams *published;
	const LowCutIirParams *current;
	bool took_new = false;
	int next, channel;

	published = lowCutIirExchangeAdopt(exchange, &took_new);
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
		lowCutIirCopyParams(&state->iir_params[state->iir_cur], published);
		return;
	}

	next = state->iir_cur ^ 1;
	current = &state->iir_params[state->iir_cur];
	lowCutIirCopyParams(&state->iir_params[next], published);
	if (lowCutIirNeedsReset(&state->iir_params[next], current, true))
	{
		for (channel = 0; channel < FILT_LOW_CUT_MAX_CHANNELS; channel++)
		{
			filtLowCutResetChannelState(&state->iir_states[next][channel]);
		}
	}
	else
	{
		for (channel = 0; channel < FILT_LOW_CUT_MAX_CHANNELS; channel++)
		{
			state->iir_states[next][channel] = state->iir_states[state->iir_cur][channel];
		}
	}
	state->iir_crossfading = true;
	state->iir_crossfade_pos = 0;
}

static bool lowCutIirReady(const LowCutAudioState *state, int sample_rate, unsigned int reset_serial)
{
	const LowCutIirParams *params = &state->iir_params[state->iir_crossfading ? (state->iir_cur ^ 1) : state->iir_cur];

	return params->sample_rate == sample_rate && params->reset_serial == reset_serial;
}

static void lowCutIirStart(LowCutAudioState *state)
{
	int channel;

	if (state->iir_running)
	{
		return;
	}
	if (state->iir_crossfading)
	{
		lowCutIirFinishCrossfade(state);
	}
	for (channel = 0; channel < FILT_LOW_CUT_MAX_CHANNELS; channel++)
	{
		filtLowCutResetChannelState(&state->iir_states[state->iir_cur][channel]);
	}
	state->iir_running = true;
	state->iir_channels = FILT_LOW_CUT_MAX_CHANNELS;
}

static void lowCutIirStop(LowCutAudioState *state)
{
	if (state->iir_crossfading)
	{
		lowCutIirFinishCrossfade(state);
	}
	state->iir_running = false;
}

static void lowCutAudioStateFormatChanged(LowCutAudioState *state)
{
	lowCutIirStop(state);
	state->iir_keep_blocked = true;
	if (state->source != LOW_CUT_SOURCE_DRY || state->crossfade_from >= 0)
	{
		state->crossfade_from = -1;
		state->crossfade_pos = 0;
		state->gain_pos = 0;
		state->gain_dir = 0;
	}
}

static inline void lowCutIirRunChannel(LowCutAudioState *state, int channel, realtype r_input, float r_incoming_weight,
	float *fp_filtered, float *fp_removed)
{
	const LowCutIirParams *params = &state->iir_params[state->iir_cur];
	FiltLowCutChannelState *history = &state->iir_states[state->iir_cur][channel];
	float filtered, removed;

	filtered = (float)filtLowCutProcessSample(r_input, params->hp_sections, params->num_hp_sections, history->hp_sections);
	removed = (params->num_lp_sections > 0) ?
		(float)filtLowCutProcessSample(r_input, params->lp_sections, params->num_lp_sections, history->lp_sections) : 0.0f;

	if (state->iir_crossfading)
	{
		const int next = state->iir_cur ^ 1;
		const LowCutIirParams *incoming = &state->iir_params[next];
		FiltLowCutChannelState *incoming_history = &state->iir_states[next][channel];
		float incoming_filtered, incoming_removed;

		incoming_filtered = (float)filtLowCutProcessSample(r_input, incoming->hp_sections, incoming->num_hp_sections,
			incoming_history->hp_sections);
		incoming_removed = (incoming->num_lp_sections > 0) ?
			(float)filtLowCutProcessSample(r_input, incoming->lp_sections, incoming->num_lp_sections, incoming_history->lp_sections) : 0.0f;
		filtered += r_incoming_weight * (incoming_filtered - filtered);
		removed += r_incoming_weight * (incoming_removed - removed);
	}

	*fp_filtered = filtered;
	*fp_removed = removed;
}

static void lowCutSettleRamps(LowCutAudioState *state)
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
		lowCutIirFinishCrossfade(state);
	}
}

static bool lowCutStepSource(LowCutAudioState *state, int desired)
{
	lowCutSettleRamps(state);

	if (state->crossfade_from >= 0)
	{
		if (desired == state->crossfade_from)
		{
			state->crossfade_from = state->source;
			state->source = desired;
			state->crossfade_pos = state->crossfade_frames - state->crossfade_pos;
			lowCutSettleRamps(state);
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

static int lowCutRunSegment(LowCutAudioState *state, float *fp_buffer, int i_num_frames,
	int i_num_channels, int i_processed_channels, bool b_preview_on)
{
	lowCutSettleRamps(state);

	const float crossfade_step = 1.0f / (float)state->crossfade_frames;
	const float fade_step = 1.0f / (float)state->fade_frames;
	const bool run_iir = state->source == LOW_CUT_SOURCE_IIR || state->crossfade_from == LOW_CUT_SOURCE_IIR;
	const bool crossfading = state->crossfade_from >= 0;
	bool level_steady;
	int preview_dir = 0;
	int segment = i_num_frames;
	int n, channel;

	if (run_iir)
	{
		lowCutIirStart(state);
		for (channel = state->iir_channels; channel < i_processed_channels; channel++)
		{
			filtLowCutResetChannelState(&state->iir_states[0][channel]);
			filtLowCutResetChannelState(&state->iir_states[1][channel]);
		}
		state->iir_channels = i_processed_channels;
	}
	else
	{
		lowCutIirStop(state);
	}
	level_steady = state->source != LOW_CUT_SOURCE_DRY && !crossfading &&
		state->gain_dir == 0 && state->gain_pos >= state->fade_frames;

	if (b_preview_on && state->preview_pos < state->crossfade_frames)
	{
		preview_dir = 1;
	}
	else if (!b_preview_on && state->preview_pos > 0)
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
	if (segment < 1)
	{
		segment = 1;
	}

	for (n = 0; n < segment; n++)
	{
		float *frame = &fp_buffer[(size_t)n * (size_t)i_num_channels];
		float filtered[FILT_LOW_CUT_MAX_CHANNELS];
		float removed[FILT_LOW_CUT_MAX_CHANNELS];
		float weights[LOW_CUT_NUM_SOURCES] = { 0.0f, 0.0f };
		float current_weight;
		float preview_mix, iir_incoming_weight;

		if (crossfading)
		{
			state->crossfade_pos++;
			current_weight = lowCutRampWeight(state->crossfade_pos, state->crossfade_frames, crossfade_step);
			weights[state->crossfade_from] = 1.0f - current_weight;
		}
		else
		{
			state->gain_pos += state->gain_dir;
			current_weight = lowCutRampWeight(state->gain_pos, state->fade_frames, fade_step);
		}
		weights[state->source] += current_weight;

		state->preview_pos += preview_dir;
		preview_mix = lowCutRampWeight(state->preview_pos, state->crossfade_frames, crossfade_step);
		if (state->iir_crossfading)
		{
			state->iir_crossfade_pos++;
		}
		iir_incoming_weight = lowCutRampWeight(state->iir_crossfade_pos, state->crossfade_frames, crossfade_step);

		for (channel = 0; channel < i_processed_channels; channel++)
		{
			float input = frame[channel];
			float out_filtered = weights[LOW_CUT_SOURCE_DRY] * input;
			float out_removed = 0.0f;

			if (run_iir)
			{
				float iir_filtered, iir_removed;

				lowCutIirRunChannel(state, channel, (realtype)input, iir_incoming_weight, &iir_filtered, &iir_removed);
				out_filtered += weights[LOW_CUT_SOURCE_IIR] * iir_filtered;
				out_removed += weights[LOW_CUT_SOURCE_IIR] * iir_removed;
			}
			filtered[channel] = out_filtered;
			removed[channel] = out_removed;
		}

		if (state->preview_pos > 0)
		{
			lowCutLevelPreviewFrame(removed, i_processed_channels, &state->preview_level_coeffs,
				&state->preview_mean_square, &state->preview_gain, level_steady);
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
	}

	lowCutSettleRamps(state);
	return segment;
}

template <typename DesiredSourceFn>
static void lowCutProcessBuffer(LowCutAudioState *state, LowCutIirExchange *exchange, int sample_rate,
	unsigned int reset_serial, unsigned int format_serial, float *fp_buffer,
	int i_num_frames, int i_num_channels, bool b_preview_on, DesiredSourceFn desired_source)
{
	const int processed_channels = (i_num_channels < FILT_LOW_CUT_MAX_CHANNELS) ? i_num_channels : FILT_LOW_CUT_MAX_CHANNELS;
	int frame = 0;
	int desired;

	if (i_num_channels <= 0 || i_num_frames <= 0)
	{
		return;
	}

	lowCutAudioStateSetRate(state, sample_rate);
	if (format_serial != state->format_serial)
	{
		lowCutAudioStateFormatChanged(state);
		state->format_serial = format_serial;
	}
	lowCutIirPoll(state, exchange, sample_rate, reset_serial);
	desired = desired_source();

	if (state->source == LOW_CUT_SOURCE_DRY && desired == LOW_CUT_SOURCE_DRY && state->crossfade_from < 0 &&
		state->gain_pos >= state->fade_frames && state->preview_pos == 0 && !b_preview_on)
	{
		lowCutIirStop(state);
		state->gain_dir = 0;
		state->preview_was_on = false;
		return;
	}

	if (b_preview_on && !state->preview_was_on && state->preview_pos == 0)
	{
		state->preview_mean_square = 0.0f;
		state->preview_gain = 1.0f;
	}
	state->preview_was_on = b_preview_on;

	while (frame < i_num_frames)
	{
		int switches = 0;

		if (frame > 0)
		{
			lowCutIirPoll(state, exchange, sample_rate, reset_serial);
			desired = desired_source();
		}
		while (lowCutStepSource(state, desired) && ++switches < 3)
		{
			desired = desired_source();
		}

		frame += lowCutRunSegment(state, &fp_buffer[(size_t)frame * (size_t)i_num_channels], i_num_frames - frame,
			i_num_channels, processed_channels, b_preview_on);
	}
}

static int lowCutDesiredSourceFor(const LowCutAudioState *state, bool filter_on, int sample_rate, unsigned int reset_serial)
{
	if (!filter_on)
	{
		return LOW_CUT_SOURCE_DRY;
	}

	const bool keep_iir = state->source == LOW_CUT_SOURCE_IIR && state->iir_running &&
		!state->iir_keep_blocked &&
		state->iir_params[state->iir_crossfading ? (state->iir_cur ^ 1) : state->iir_cur].sample_rate == sample_rate;

	if (lowCutIirReady(state, sample_rate, reset_serial) || keep_iir)
	{
		return LOW_CUT_SOURCE_IIR;
	}
	return LOW_CUT_SOURCE_DRY;
}

static void debugVerifyLowCutResponse()
{
	const int sample_rates[] = { 44100, 48000, 96000 };
	const int *slopes = DfxDsp::LOW_CUT_SLOPES_DB;
	const size_t num_slopes = (size_t)DfxDsp::LOW_CUT_NUM_SLOPES;
	size_t rate_index, slope_index;
	int requested;

	for (requested = 0; requested <= 60; requested++)
	{
		int expected = slopes[num_slopes - 1];

		for (slope_index = num_slopes; slope_index-- > 0;)
		{
			if (slopes[slope_index] >= requested)
			{
				expected = slopes[slope_index];
			}
		}
		assert(filtLowCutSnapSlopeDb(requested) == expected);
	}

	for (rate_index = 0; rate_index < sizeof(sample_rates) / sizeof(sample_rates[0]); rate_index++)
	{
		for (slope_index = 0; slope_index < num_slopes; slope_index++)
		{
			const int sample_rate = sample_rates[rate_index];
			const int slope = slopes[slope_index];
			FiltLowCutSection expected[FILT_LOW_CUT_MAX_SECTIONS];
			LowCutIirParams params;
			int expected_sections;

			expected_sections = filtLowCutDesign(slope, FILT_LOW_CUT_Q_NEUTRAL, DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ,
				(double)sample_rate, 1, expected);
			lowCutDesignIirParams(sample_rate, slope, (float)FILT_LOW_CUT_Q_NEUTRAL, (float)DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ, 7u, &params);
			assert(expected_sections > 0);
			assert(params.num_hp_sections == expected_sections && params.num_lp_sections == expected_sections);
			assert(params.sample_rate == sample_rate && params.reset_serial == 7u);

			assert(filtLowCutCascadeResponseDb(1000.0, sample_rate, params.hp_sections, params.num_hp_sections) > -0.1);
			assert(filtLowCutCascadeResponseDb(20000.0, sample_rate, params.hp_sections, params.num_hp_sections) > -0.1);
			assert(fabs(filtLowCutCascadeResponseDb(DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ, sample_rate, params.hp_sections, params.num_hp_sections) -
				(-3.0103)) < 0.01);
			assert(fabs(filtLowCutCascadeResponseDb(DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ, sample_rate, params.lp_sections, params.num_lp_sections) -
				(-3.0103)) < 0.01);
		}
	}
}

static void debugVerifyLowCutIirHandoff()
{
	const float Q = (float)FILT_LOW_CUT_Q_NEUTRAL;
	LowCutIirExchange exchange;
	LowCutIirParams first, cutoff_moved, gentler;
	const LowCutIirParams *adopted;
	bool took_new = true;

	lowCutIirExchangeInit(&exchange);
	adopted = lowCutIirExchangeAdopt(&exchange, &took_new);
	assert(!took_new && adopted->sample_rate == 0);

	lowCutDesignIirParams(48000, 48, Q, 20.0f, 1u, &first);
	lowCutDesignIirParams(48000, 48, Q, 40.0f, 1u, &cutoff_moved);
	lowCutDesignIirParams(48000, 24, Q, 40.0f, 1u, &gentler);

	lowCutIirExchangePublish(&exchange, &first);
	adopted = lowCutIirExchangeAdopt(&exchange, &took_new);
	assert(took_new);
	assert(adopted->sample_rate == 48000 && adopted->num_hp_sections == first.num_hp_sections);
	assert(adopted->hp_sections[0].a1 == first.hp_sections[0].a1);
	assert(lowCutIirNeedsReset(adopted, NULL, false));

	lowCutIirExchangePublish(&exchange, &cutoff_moved);
	adopted = lowCutIirExchangeAdopt(&exchange, NULL);
	assert(adopted->hp_sections[0].a1 == cutoff_moved.hp_sections[0].a1);
	assert(!lowCutIirNeedsReset(adopted, &first, true));

	assert(lowCutIirExchangeAdopt(&exchange, &took_new) == adopted && !took_new);

	lowCutIirExchangePublish(&exchange, &first);
	lowCutIirExchangePublish(&exchange, &gentler);
	adopted = lowCutIirExchangeAdopt(&exchange, NULL);
	assert(adopted->num_hp_sections == gentler.num_hp_sections && adopted->hp_sections[0].a1 == gentler.hp_sections[0].a1);
	assert(lowCutIirNeedsReset(adopted, &cutoff_moved, true));

	assert(lowCutIirNeedsReset(adopted, &gentler, false));
	assert(!lowCutIirNeedsReset(adopted, &gentler, true));

	LowCutIirParams bypassed;
	lowCutDesignIirParams(48000, 48, Q, 0.0f, 1u, &bypassed);
	assert(bypassed.num_hp_sections == 0 && bypassed.num_lp_sections == 0 && bypassed.sample_rate == 48000);
	assert(lowCutIirNeedsReset(&first, &bypassed, true));

	LowCutIirParams eighteen;
	lowCutDesignIirParams(48000, 18, Q, 40.0f, 1u, &eighteen);
	assert(eighteen.num_hp_sections == gentler.num_hp_sections);
	assert(lowCutIirNeedsReset(&gentler, &eighteen, true));

	LowCutIirParams resonant;
	lowCutDesignIirParams(48000, 24, 2.0f, 40.0f, 1u, &resonant);
	assert(resonant.hp_sections[0].a1 != gentler.hp_sections[0].a1);
	assert(!lowCutIirNeedsReset(&resonant, &gentler, true));
}

static void debugVerifyLowCutPreviewLeveller()
{
	const int sample_rate = 48000;
	const int num_frames = 3 * sample_rate;
	const int measure_from = num_frames - sample_rate / 2;
	LowCutPreviewLevelCoeffs coeffs = lowCutPreviewLevelCoeffs(sample_rate);
	const float amplitudes[] = { 0.01f, 3.0f, 0.0f };
	size_t case_index;

	for (case_index = 0; case_index < sizeof(amplitudes) / sizeof(amplitudes[0]); case_index++)
	{
		float amplitude = amplitudes[case_index];
		float mean_square = 0.0f;
		float gain = 1.0f;
		double measured_sum = 0.0;
		float max_magnitude = 0.0f;
		int n;

		for (n = 0; n < num_frames; n++)
		{
			const double two_pi = 6.283185307179586;
			float value = amplitude * (float)sin(two_pi * 30.0 * (double)n / (double)sample_rate);
			float frame[2] = { value, value };

			lowCutLevelPreviewFrame(frame, 2, &coeffs, &mean_square, &gain, true);
			if (fabsf(frame[0]) > max_magnitude)
			{
				max_magnitude = fabsf(frame[0]);
			}
			if (n >= measure_from)
			{
				measured_sum += (double)frame[0] * (double)frame[0];
			}
		}

		double measured_rms = sqrt(measured_sum / (double)(num_frames - measure_from));

		assert(max_magnitude <= 1.0f);
		if (amplitude == 0.0f)
		{
			assert(max_magnitude == 0.0f);
		}
		else if (amplitude < 0.1f)
		{
			assert(measured_rms > 0.07 && measured_rms < 0.14);
		}
	}

	{
		const int silence_frames = 2 * sample_rate;
		const int tone_frames = sample_rate / 10;
		const float amplitude = 0.3f;
		float mean_square = 0.0f;
		float gain = 1.0f;
		float max_magnitude = 0.0f;
		int n;

		for (n = 0; n < silence_frames + tone_frames; n++)
		{
			const double two_pi = 6.283185307179586;
			float value = (n < silence_frames) ? 0.0f : amplitude * (float)sin(two_pi * 30.0 * (double)n / (double)sample_rate);
			float frame[2] = { value, value };

			lowCutLevelPreviewFrame(frame, 2, &coeffs, &mean_square, &gain, true);
			if (n == silence_frames - 1)
			{
				assert(gain == 1.0f);
			}
			if (fabsf(frame[0]) > max_magnitude)
			{
				max_magnitude = fabsf(frame[0]);
			}
		}
		assert(max_magnitude <= amplitude * 1.5f);
	}
}

static void debugVerifyLowCutDesiredSource()
{
	const int sample_rate = 48000;
	const int num_channels = 2;
	const int buffer_frames = 480;
	const float Q = (float)FILT_LOW_CUT_Q_NEUTRAL;
	LowCutAudioState *state = new LowCutAudioState;
	LowCutIirExchange exchange;
	LowCutIirParams set1, set2;
	std::vector<float> buffer((size_t)buffer_frames * num_channels, 0.25f);
	unsigned int reset_serial = 1u;
	unsigned int format_serial = 0u;
	auto run = [&](int buffers) {
		for (int i = 0; i < buffers; i++)
		{
			lowCutProcessBuffer(state, &exchange, sample_rate, reset_serial, format_serial, buffer.data(), buffer_frames,
				num_channels, false, [&]() { return lowCutDesiredSourceFor(state, true, sample_rate, reset_serial); });
		}
	};

	lowCutAudioStateInit(state);
	lowCutIirExchangeInit(&exchange);
	lowCutDesignIirParams(sample_rate, 24, Q, 20.0f, 1u, &set1);
	lowCutDesignIirParams(sample_rate, 24, Q, 20.0f, 2u, &set2);

	assert(lowCutDesiredSourceFor(state, false, sample_rate, reset_serial) == LOW_CUT_SOURCE_DRY);
	assert(lowCutDesiredSourceFor(state, true, sample_rate, reset_serial) == LOW_CUT_SOURCE_DRY);

	lowCutIirExchangePublish(&exchange, &set1);
	run(5);
	assert(state->source == LOW_CUT_SOURCE_IIR && state->crossfade_from < 0 && state->iir_running);
	assert(lowCutDesiredSourceFor(state, false, sample_rate, reset_serial) == LOW_CUT_SOURCE_DRY);
	assert(lowCutDesiredSourceFor(state, true, 44100, reset_serial) == LOW_CUT_SOURCE_DRY);

	reset_serial = 2u;
	assert(lowCutDesiredSourceFor(state, true, sample_rate, reset_serial) == LOW_CUT_SOURCE_IIR);
	run(3);
	assert(state->source == LOW_CUT_SOURCE_IIR && state->crossfade_from < 0);

	format_serial = 1u;
	run(5);
	assert(state->iir_keep_blocked);
	assert(lowCutDesiredSourceFor(state, true, sample_rate, reset_serial) == LOW_CUT_SOURCE_DRY);
	assert(state->source == LOW_CUT_SOURCE_DRY);
	lowCutIirExchangePublish(&exchange, &set2);
	run(5);
	assert(state->source == LOW_CUT_SOURCE_IIR && !state->iir_keep_blocked);

	delete state;
}

static void debugVerifyLowCutTransitions()
{
	enum ExpectKind { EXPECT_NONE, EXPECT_DRY, EXPECT_IIR, EXPECT_AUDIBLE };
	struct Phase {
		int first_buffer;
		int desired;
		bool preview_on;
		unsigned int reset_serial;
		unsigned int format_serial;
		int publish;
		int reference;
		ExpectKind expect;
	};
	const int sample_rate = 48000;
	const int num_channels = 2;
	const int buffer_frames = 480;
	const double amplitude = 0.5;
	const double two_pi = 6.283185307179586;
	const double sine_step = amplitude * two_pi * 100.0 / (double)sample_rate;
	const int D = LOW_CUT_SOURCE_DRY, I = LOW_CUT_SOURCE_IIR;
	const Phase script[] = {
		{   0, D, false, 1u, 0u, 0, -1, EXPECT_DRY },
		{  10, I, false, 1u, 0u, -1, -1, EXPECT_NONE },
		{  11, D, false, 1u, 0u, -1, -1, EXPECT_NONE },
		{  12, D, false, 1u, 0u, -1, -1, EXPECT_DRY },
		{  15, I, false, 1u, 0u, -1, 0, EXPECT_IIR },
		{  45, I, false, 1u, 0u, 1, -1, EXPECT_NONE },
		{  46, I, false, 1u, 0u, 2, -1, EXPECT_NONE },
		{  48, I, false, 1u, 0u, -1, 2, EXPECT_IIR },
		{  80, I, true, 1u, 0u, -1, -1, EXPECT_AUDIBLE },
		{ 100, I, false, 1u, 0u, -1, -1, EXPECT_NONE },
		{ 105, I, false, 1u, 0u, 3, -1, EXPECT_NONE },
		{ 108, I, false, 2u, 0u, -1, 0, EXPECT_IIR },
		{ 140, I, false, 3u, 1u, -1, 0, EXPECT_NONE },
		{ 142, I, false, 3u, 1u, 4, -1, EXPECT_IIR },
		{ 155, D, false, 3u, 1u, -1, -1, EXPECT_DRY },
		{ 185, D, false, 3u, 1u, -1, -1, EXPECT_NONE }
	};
	const int num_phases = (int)(sizeof(script) / sizeof(script[0])) - 1;
	LowCutAudioState *state = new LowCutAudioState;
	LowCutIirExchange exchange;
	LowCutIirParams sets[5];
	LowCutIirParams reference_params = {};
	FiltLowCutChannelState reference_state;
	std::vector<float> buffer((size_t)buffer_frames * num_channels);
	std::vector<float> input((size_t)buffer_frames);
	std::vector<float> reference((size_t)buffer_frames);
	double max_step = 0.0;
	float previous = 0.0f;
	long long t = 0;
	int phase_index, buffer_index, n;

	lowCutAudioStateInit(state);
	lowCutIirExchangeInit(&exchange);
	lowCutDesignIirParams(sample_rate, 24, (float)FILT_LOW_CUT_Q_NEUTRAL, 20.0f, 1u, &sets[0]);
	lowCutDesignIirParams(sample_rate, 24, (float)FILT_LOW_CUT_Q_NEUTRAL, 200.0f, 1u, &sets[1]);
	lowCutDesignIirParams(sample_rate, 48, (float)FILT_LOW_CUT_Q_NEUTRAL, 200.0f, 1u, &sets[2]);
	lowCutDesignIirParams(sample_rate, 24, (float)FILT_LOW_CUT_Q_NEUTRAL, 20.0f, 2u, &sets[3]);
	lowCutDesignIirParams(sample_rate, 24, (float)FILT_LOW_CUT_Q_NEUTRAL, 20.0f, 3u, &sets[4]);
	filtLowCutResetChannelState(&reference_state);

	for (phase_index = 0; phase_index < num_phases; phase_index++)
	{
		const Phase& phase = script[phase_index];
		const int last_buffer = script[phase_index + 1].first_buffer - 1;

		if (phase.publish >= 0)
		{
			lowCutIirExchangePublish(&exchange, &sets[phase.publish]);
		}
		if (phase.reference >= 0)
		{
			reference_params = sets[phase.reference];
			filtLowCutResetChannelState(&reference_state);
		}

		for (buffer_index = phase.first_buffer; buffer_index <= last_buffer; buffer_index++)
		{
			const int desired = phase.desired;

			for (n = 0; n < buffer_frames; n++, t++)
			{
				float value = (float)(amplitude * sin(two_pi * 100.0 * (double)t / (double)sample_rate));

				input[n] = value;
				buffer[(size_t)n * num_channels] = value;
				buffer[(size_t)n * num_channels + 1] = value;
				reference[n] = (float)filtLowCutProcessSample((realtype)value, reference_params.hp_sections,
					reference_params.num_hp_sections, reference_state.hp_sections);
			}

			lowCutProcessBuffer(state, &exchange, sample_rate, phase.reset_serial, phase.format_serial, buffer.data(), buffer_frames,
				num_channels, phase.preview_on, [desired]() { return desired; });

			if (buffer_index == 45 || buffer_index == 47)
			{
				assert(state->iir_crossfading && state->iir_running);
			}
			if (buffer_index == 46)
			{
				assert(!state->iir_crossfading && state->iir_taken_pending);
				assert(state->iir_taken.num_hp_sections == sets[2].num_hp_sections);
			}
			if (buffer_index == 47)
			{
				assert(!state->iir_taken_pending && state->iir_params[state->iir_cur ^ 1].num_hp_sections == sets[2].num_hp_sections);
			}
			if (buffer_index == 106 || buffer_index == 107)
			{
				assert(state->iir_taken_pending && state->iir_taken.reset_serial == 2u);
				assert(lowCutIirReady(state, sample_rate, 1u));
			}
			if (buffer_index == 140)
			{
				for (n = 0; n < buffer_frames; n++)
				{
					float fade = (n + 1 >= state->fade_frames) ? 1.0f : (float)(n + 1) / (float)state->fade_frames;

					assert(fabsf(buffer[(size_t)n * num_channels] - fade * reference[n]) < 1e-6f);
				}
				assert(state->iir_keep_blocked);
			}
			if (buffer_index == 142)
			{
				assert(state->iir_crossfading && !state->iir_keep_blocked);
			}
			if (buffer_index == 108)
			{
				assert(!state->iir_taken_pending && lowCutIirReady(state, sample_rate, 2u));
			}

			for (n = 0; n < buffer_frames; n++)
			{
				float sample = buffer[(size_t)n * num_channels];

				if (n == 0 && buffer_index == phase.first_buffer && phase_index > 0 &&
					phase.format_serial != script[phase_index - 1].format_serial)
				{
					previous = sample;
				}
				if (fabs((double)sample - (double)previous) > max_step)
				{
					max_step = fabs((double)sample - (double)previous);
				}
				previous = sample;
				assert(buffer[(size_t)n * num_channels + 1] == sample);
			}
		}

		if (phase.expect == EXPECT_DRY)
		{
			for (n = 0; n < buffer_frames; n++)
			{
				assert(buffer[(size_t)n * num_channels] == input[n]);
			}
		}
		else if (phase.expect == EXPECT_IIR)
		{
			assert(state->source == LOW_CUT_SOURCE_IIR && state->gain_pos == state->fade_frames);
			for (n = 0; n < buffer_frames; n++)
			{
				assert(fabsf(buffer[(size_t)n * num_channels] - reference[n]) < 1e-3f);
			}
		}
		else if (phase.expect == EXPECT_AUDIBLE)
		{
			double sum = 0.0;

			for (n = 0; n < buffer_frames; n++)
			{
				sum += (double)buffer[(size_t)n * num_channels] * (double)buffer[(size_t)n * num_channels];
			}
			assert(sqrt(sum / (double)buffer_frames) > 0.1);
		}
	}

	assert(max_step < 1.5 * sine_step);

	assert(state->source == LOW_CUT_SOURCE_DRY && state->crossfade_from < 0 && state->gain_pos == state->fade_frames);
	assert(!state->iir_running && state->preview_pos == 0);

	delete state;
}

DfxDspPrivate::DfxDspPrivate()
{
	dfxp_handle_ = NULL;
	slout1_ = NULL;
	midi_to_rval_qnt_handle_ = NULL;
	rval_to_midi_qnt_handle_ = NULL;

	swprintf(product_specific_.wcp_registry_product_name, PT_MAX_GENERIC_STRLEN, L"%s", DFXG_REGISTRY_DFX_PRODUCT_NAME_WIDE);
	swprintf(product_specific_.wcp_displayed_product_name, PT_MAX_GENERIC_STRLEN, L"%s", DFXG_DISPLAYED_DFX_PRODUCT_NAME_WIDE);
	product_specific_.full_version = static_cast<float>(13.028);
	product_specific_.major_version = static_cast<int>(13.028);
	vendor_specific_.vendor_code = DFXP_VENDOR_CODE_UNIVERSAL;


	// Initialize the processing mode
	if (dfxpInit(&dfxp_handle_,
		L"DFX", 23,
		14, 1, IS_FALSE,
		IS_FALSE, 0,
		IS_FALSE,
		IS_FALSE,
		IS_FALSE, slout1_) != OKAY)
	{
		//return(NOT_OKAY);
		MessageBox(NULL, L"TTEST", L"TEST", MB_OK);
	}

	// Make sure the vocal reduction is turned off 
	if (dfxpSetButtonValue(dfxp_handle_, DFX_UI_BUTTON_VOCAL_REDUCTION_ON, IS_FALSE) != OKAY)
	{
		//return(NOT_OKAY);
	}

	// From dfxg_InitStaticQnts() in dfxgQnt.cpp
	/*
	* Initalize the qnt handle for calculating the real value based
	* on a MIDI value.
	*/
	if (qntIToRInit(&(midi_to_rval_qnt_handle_), slout1_,
		MIDI_MIN_VALUE,
		MIDI_MAX_VALUE,
		DFX_UI_MIN_VALUE, DFX_UI_MAX_VALUE,
		IS_FALSE, 0,
		IS_FALSE, (realtype)0.0,
		IS_FALSE, QNT_RESPONSE_LINEAR) != OKAY)
	{
	}

	/*
	* Initialize the qnt handle for calculating the midi value
	* based on the real value.
	*/
	if (qntRToIInit(&(rval_to_midi_qnt_handle_), slout1_,
		DFX_UI_MIN_VALUE, DFX_UI_MAX_VALUE,
		MIDI_MIN_VALUE,
		MIDI_MAX_VALUE,
		IS_FALSE, 0,
		IS_FALSE) != OKAY)
	{
	}

	low_cut_cached_sample_rate_ = 44100;
	low_cut_cached_num_channels_ = 2;
	lowCutIirExchangeInit(&low_cut_iir_exchange_);
	lowCutAudioStateInit(&low_cut_audio_);

#ifdef _DEBUG
	{
		char skip_value[4] = {};
		DWORD skip_length = GetEnvironmentVariableA("FXSOUND_SKIP_DSP_SELFCHECKS", skip_value, sizeof(skip_value));

		if (!(skip_length == 1 && skip_value[0] == '1'))
		{
			debugVerifyLowCutResponse();
			debugVerifyLowCutIirHandoff();
			debugVerifyLowCutDesiredSource();
			debugVerifyLowCutTransitions();
			debugVerifyLowCutPreviewLeveller();
		}
	}
#endif

	low_cut_designer_thread_ = std::thread(&DfxDspPrivate::lowCutDesignerThreadMain, this);

	//return(OKAY);
}


DfxDspPrivate::~DfxDspPrivate()
{
	// So the thread/timer will not attempt to use this object while it's being destroyed.
	being_destroyed_ = true;

	{
		std::lock_guard<std::mutex> lock(low_cut_designer_mutex_);
		low_cut_designer_stop_ = true;
	}
	low_cut_designer_wake_.notify_one();
	if (low_cut_designer_thread_.joinable())
	{
		low_cut_designer_thread_.join();
	}

	// Free the prelst
	if (preset_list_handle_ != NULL)
	{
		if (prelstFreeUp(&(preset_list_handle_)) != OKAY)
		{
		}
	}
	// Free the midi to rval and visa versa qnt handles
	if (midi_to_rval_qnt_handle_ != NULL)
	{
		if (qntFreeUp(&(midi_to_rval_qnt_handle_)) != OKAY)
		{
		}
	}
	if (rval_to_midi_qnt_handle_ != NULL)
	{
		if (qntFreeUp(&(rval_to_midi_qnt_handle_)) != OKAY)
		{
		}
	}

	// Free dfxp handle
	dfxpFreeAll();
	free(dfxp_handle_);
	//*dfxp_handle_ = NULL;

	// Free the slout handle
	if (slout1_ != NULL)
		delete slout1_;
}

/**
dfxg_UpdateFromRegistryAllSettings()
**/
void DfxDspPrivate::processTimer()
{
	bool anything_changed = false;
	int i_eq_changed = IS_FALSE;

	if (update_from_registry_)
	{
		eqUpdateFromRegistry(&i_eq_changed);
		update_from_registry_ = false;
	}

	if (i_eq_changed)
	{
		anything_changed = true;
	}

	/* If any settings have been changed, communicate all the changes to the DSP module */
	// NOTE: I find that without this if condition and call dfxpCOmmunicateAll() repeatedly will mess up the audio.
	if (anything_changed)
	{
		dfxpCommunicateAll(dfxp_handle_);
	}
}

int DfxDspPrivate::processAudio(short int *si_input_samples, short int *si_output_samples, int i_num_sample_sets, int i_check_for_duplicate_buffers)
{
	processTimer();
	// Apply DFX processing here using data and format vars above. Format will always be 32 bit floating point.
	if (dfxpUniversalModifySamples(dfxp_handle_, si_input_samples, si_output_samples, i_num_sample_sets, i_check_for_duplicate_buffers) != OKAY)
		return(NOT_OKAY);

	applyLowCut(reinterpret_cast<float*>(si_output_samples), i_num_sample_sets);

	return OKAY;
}

int DfxDspPrivate::setSignalFormat(int i_bps, int i_nch, int i_srate, int i_valid_bits)
{
	int result = dfxpUniversalSetSignalFormat(dfxp_handle_, i_bps, i_nch, i_srate, i_valid_bits);

	if (i_srate != low_cut_cached_sample_rate_ || i_nch != low_cut_cached_num_channels_ || i_bps != low_cut_cached_bps_)
	{
		low_cut_format_serial_++;
		low_cut_reset_serial_.fetch_add(1, std::memory_order_acq_rel);
		low_cut_cached_sample_rate_ = i_srate;
		low_cut_cached_num_channels_ = i_nch;
		low_cut_cached_bps_ = i_bps;
		low_cut_request_sample_rate_ = i_srate;
		requestLowCutDesign(false);
		low_cut_designer_wake_.notify_one();
	}

	return (result == OKAY) ? OKAY : NOT_OKAY;
}

void DfxDspPrivate::requestLowCutDesign(bool wake_designer)
{
	low_cut_request_generation_.fetch_add(1, std::memory_order_acq_rel);

	if (wake_designer)
	{
		{
			std::lock_guard<std::mutex> lock(low_cut_designer_mutex_);
		}
		low_cut_designer_wake_.notify_one();
	}
}

void DfxDspPrivate::lowCutDesignerThreadMain()
{
	unsigned int handled_generation = 0;
	std::unique_lock<std::mutex> lock(low_cut_designer_mutex_, std::defer_lock);

	while (!low_cut_designer_stop_)
	{
		unsigned int generation = low_cut_request_generation_.load(std::memory_order_acquire);
		if (generation != handled_generation)
		{
			handled_generation = generation;

			int sample_rate = low_cut_request_sample_rate_;
			unsigned int reset_serial = low_cut_reset_serial_.load(std::memory_order_acquire);
			LowCutIirParams iir_params;

			lowCutDesignIirParams(sample_rate, low_cut_iir_slope_db_.load(), low_cut_iir_q_.load(),
				low_cut_cutoff_hz_.load(), reset_serial, &iir_params);
			lowCutIirExchangePublish(&low_cut_iir_exchange_, &iir_params);
		}

		lock.lock();
		low_cut_designer_wake_.wait_for(lock, std::chrono::milliseconds(200), [this, handled_generation]() {
			return low_cut_designer_stop_.load() ||
				low_cut_request_generation_.load(std::memory_order_acquire) != handled_generation;
			});
		lock.unlock();
	}
}

int DfxDspPrivate::lowCutDesiredSource(bool filter_on, unsigned int reset_serial)
{
	return lowCutDesiredSourceFor(&low_cut_audio_, filter_on, low_cut_cached_sample_rate_, reset_serial);
}

void DfxDspPrivate::applyLowCut(float *audio_buffer, int num_sample_sets)
{
	if (low_cut_cached_bps_ != 32)
	{
		return;
	}

	const int num_channels = low_cut_cached_num_channels_;
	const bool filter_on = low_cut_on_ && low_cut_power_on_;
	const bool preview_on = filter_on && low_cut_preview_on_;
	const unsigned int reset_serial = low_cut_reset_serial_.load(std::memory_order_acquire);

	lowCutProcessBuffer(&low_cut_audio_, &low_cut_iir_exchange_, low_cut_cached_sample_rate_, reset_serial,
		low_cut_format_serial_, audio_buffer, num_sample_sets, num_channels, preview_on,
		[this, filter_on, reset_serial]() {
			return lowCutDesiredSource(filter_on, reset_serial);
		});
}

void DfxDspPrivate::lowCutOn(bool on)
{
	if (on)
	{
		if (!low_cut_on_)
		{
			low_cut_reset_serial_.fetch_add(1, std::memory_order_acq_rel);
		}

		low_cut_on_ = true;
		requestLowCutDesign(true);
	}
	else
	{
		low_cut_on_ = false;
		low_cut_preview_on_ = false;
	}
}

bool DfxDspPrivate::isLowCutOn()
{
	return low_cut_on_;
}

void DfxDspPrivate::setLowCutSlope(int db_per_octave)
{
	low_cut_iir_slope_db_ = filtLowCutSnapSlopeDb(db_per_octave);
	requestLowCutDesign(true);
}

int DfxDspPrivate::getLowCutSlope()
{
	return low_cut_iir_slope_db_;
}

void DfxDspPrivate::setLowCutQ(float q)
{
	low_cut_iir_q_ = static_cast<float>(filtLowCutClampQ(q));
	requestLowCutDesign(true);
}

float DfxDspPrivate::getLowCutQ()
{
	return low_cut_iir_q_;
}

void DfxDspPrivate::lowCutPreviewOn(bool on)
{
	low_cut_preview_on_ = on && low_cut_on_;
}

bool DfxDspPrivate::isLowCutPreviewOn()
{
	return low_cut_preview_on_;
}

void DfxDspPrivate::setLowCutCutoff(float cutoff_hz)
{
	if (cutoff_hz < (float)DfxDsp::LOW_CUT_MIN_CUTOFF_HZ)
	{
		cutoff_hz = (float)DfxDsp::LOW_CUT_MIN_CUTOFF_HZ;
	}
	else if (cutoff_hz > (float)DfxDsp::LOW_CUT_MAX_CUTOFF_HZ)
	{
		cutoff_hz = (float)DfxDsp::LOW_CUT_MAX_CUTOFF_HZ;
	}

	low_cut_cutoff_hz_ = cutoff_hz;
	requestLowCutDesign(true);
}

float DfxDspPrivate::getLowCutCutoff()
{
	return low_cut_cutoff_hz_;
}

void DfxDspPrivate::powerOn(bool on)
{
	if (on)
	{
		if (dfxpSetButtonValue(dfxp_handle_, DFX_UI_BUTTON_BYPASS, 0) != OKAY)
		{
		}
	}
	else
	{
		if (dfxpSetButtonValue(dfxp_handle_, DFX_UI_BUTTON_BYPASS, 1) != OKAY)
		{
		}
	}

	if (on)
	{
		if (!low_cut_power_on_)
		{
			low_cut_reset_serial_.fetch_add(1, std::memory_order_acq_rel);
			low_cut_power_on_ = true;
			requestLowCutDesign(true);
		}
	}
	else
	{
		low_cut_power_on_ = false;
	}
}

bool DfxDspPrivate::isPowerOn()
{
	int value;
	
	dfxpGetButtonValue(dfxp_handle_, DFX_UI_BUTTON_BYPASS, &value);
	if (value != 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

float DfxDspPrivate::getEffectValue(DfxDsp::Effect effect)
{
	switch (effect)
	{
	case DfxDsp::Effect::Fidelity:
		return fidelity_.value;

	case DfxDsp::Effect::Ambience:
		return ambience_.value;

	case DfxDsp::Effect::Surround:
		return surround_.value;

	case DfxDsp::Effect::DynamicBoost:
		return dynamic_boost_.value;

	case DfxDsp::Effect::Bass:
		return bass_boost_.value;
	}

	return -1.0f;
}

void DfxDspPrivate::setEffectValue(DfxDsp::Effect effect, float value)
{
	int button;
	int knob;

	switch (effect)
	{
	case DfxDsp::Effect::Fidelity:
		button = DFX_UI_BUTTON_FIDELITY;
		knob = DFX_UI_KNOB_FIDELITY;
		fidelity_.value = (realtype)value / (realtype)10.0;
		break;

	case DfxDsp::Effect::Ambience:
		button = DFX_UI_BUTTON_AMBIENCE;
		knob = DFX_UI_KNOB_AMBIENCE;
		ambience_.value = (realtype)value / (realtype)10.0;
		break;

	case DfxDsp::Effect::Surround:
		button = DFX_UI_BUTTON_SURROUND;
		knob = DFX_UI_KNOB_SURROUND;
		surround_.value = (realtype)value / (realtype)10.0;
		break;

	case DfxDsp::Effect::DynamicBoost:
		button = DFX_UI_BUTTON_DYNAMIC_BOOST;
		knob = DFX_UI_KNOB_DYNAMIC_BOOST;
		dynamic_boost_.value = (realtype)value / (realtype)10.0;
		break;

	case DfxDsp::Effect::Bass:
		button = DFX_UI_BUTTON_BASS_BOOST;
		knob = DFX_UI_KNOB_BASS_BOOST;
		bass_boost_.value = (realtype)value / (realtype)10.0;
		break;

	default:
		return;
	}

	if (value != 0.0)
	{
		dfxpSetButtonValue(dfxp_handle_, button, 1);
	}
	else
	{
		dfxpSetButtonValue(dfxp_handle_, button, 0);
	}

	dfxpSetKnobValue(dfxp_handle_, knob, (realtype)value / (realtype)10.0, false);
}

unsigned long DfxDspPrivate::getTotalAudioProcessedTime()
{
	unsigned long value;

	dfxpGetTotalAudioProcessedTime(dfxp_handle_, &value);

	return value;
}

void DfxDspPrivate::resetTotalAudioProcessedTime()
{
	dfxpSetTotalAudioProcessedTime(dfxp_handle_, 0);
}

/*
 * 
 */
int DfxDspPrivate::dfxpFreeAll()
{
	struct dfxpHdlType *cast_handle;

	cast_handle = (struct dfxpHdlType *)(dfxp_handle_);

	if (cast_handle == NULL)
		return(OKAY);

	/* Free all the midi_to_dsp qnt handles */
	if (cast_handle->midi_to_dsp.fidelity_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.fidelity_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.spaciousness_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.spaciousness_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.ambience_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.ambience_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.dynamic_boost_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.dynamic_boost_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.bass_boost_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.bass_boost_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Fixed Aural Activation Specific */
	if (cast_handle->midi_to_dsp.aural_filter_gain_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.aural_filter_gain_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.aural_filter_a1_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.aural_filter_a1_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.aural_filter_a0_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.aural_filter_a0_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Fixed Reverb Specific */
	if (cast_handle->midi_to_dsp.room_size_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.room_size_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.damping_bandwidth_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.damping_bandwidth_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.rolloff_bandwidth_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.rolloff_bandwidth_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.motion_rate_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.motion_rate_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.motion_depth_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.motion_depth_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.screen_lex_main_knob3_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.screen_lex_main_knob3_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.screen_lex_main_knob4_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.screen_lex_main_knob4_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Fixed Optimizer Specific */
	if (cast_handle->midi_to_dsp.release_time_beta_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.release_time_beta_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.screen_opt_main_knob3_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.screen_opt_main_knob3_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Fixed Widener Specific */
	if (cast_handle->midi_to_dsp.dispersion_delay_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.dispersion_delay_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.wid_filter_gain_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.wid_filter_gain_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.wid_filter_a1_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.wid_filter_a1_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_dsp.wid_filter_a0_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.wid_filter_a0_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Delay specific */
	if (cast_handle->midi_to_dsp.dly_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_dsp.dly_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Free all the other qnt handles */
	if (cast_handle->real_to_midi_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->real_to_midi_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->midi_to_real_qnt_hdl != NULL)
	{
		if (qntFreeUp(&(cast_handle->midi_to_real_qnt_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Free the com handles */
	if (cast_handle->com_hdl_front != NULL)
	{
		if (comFreeUp(&(cast_handle->com_hdl_front)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->com_hdl_rear != NULL)
	{
		if (comFreeUp(&(cast_handle->com_hdl_rear)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->com_hdl_side != NULL)
	{
		if (comFreeUp(&(cast_handle->com_hdl_side)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->com_hdl_center != NULL)
	{
		if (comFreeUp(&(cast_handle->com_hdl_center)) != OKAY)
			return(NOT_OKAY);
	}

	if (cast_handle->com_hdl_subwoofer != NULL)
	{
		if (comFreeUp(&(cast_handle->com_hdl_subwoofer)) != OKAY)
			return(NOT_OKAY);
	}

	// Free EQ handle
	if (cast_handle->eq.graphicEq_hdl != NULL)
	{
		if (GraphicEqFreeUp(&(cast_handle->eq.graphicEq_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	// Free SurroundSyn handle
	if (cast_handle->SurroundSyn_hdl != NULL)
	{
		if (SurroundSynFreeUp(&(cast_handle->SurroundSyn_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Free the spectrum handle */
	if (cast_handle->spectrum.spectrum_hdl != NULL)
	{
		if (spectrumFreeUp(&(cast_handle->spectrum.spectrum_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Free the binauralSyn handle */
	if (cast_handle->BinauralSyn_hdl != NULL)
	{
		if (BinauralSynFreeUp(&(cast_handle->BinauralSyn_hdl)) != OKAY)
			return(NOT_OKAY);
	}

	/* Free shared memory library */
	if (cast_handle->hp_sharedUtil != NULL)
	{
		if (dfxSharedUtilFreeUp(&(cast_handle->hp_sharedUtil)) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}

void DfxDspPrivate::getSpectrumBandValues(float* rp_band_values, int i_array_size)
{
    dfxpSpectrumGetBandValues(dfxp_handle_, rp_band_values, i_array_size);
}

