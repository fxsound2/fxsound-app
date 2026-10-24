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
#include <assert.h>

#include "codedefs.h"
#include "FiltLowCut.h"
#include "FiltChain.h"

static void filtChainResetBiquad(FiltLowCutBiquadState *sp_state)
{
	sp_state->in_minus1 = 0.0;
	sp_state->in_minus2 = 0.0;
	sp_state->out_minus1 = 0.0;
	sp_state->out_minus2 = 0.0;
}

// Two identical 12 dB/oct Butterworth sections give a 24 dB/oct Linkwitz-Riley filter.
static int filtChainDesignLinkwitzRiley(double r_hz, double r_sample_rate_hz, int i_high_pass_flag,
	FiltLowCutSection *sp_out)
{
	FiltLowCutSection section;

	if (filtLowCutDesign(12, FILT_LOW_CUT_Q_NEUTRAL, r_hz, r_sample_rate_hz, i_high_pass_flag, &section) != 1)
	{
		return 0;
	}
	sp_out[0] = section;
	sp_out[1] = section;

	return FILT_CHAIN_MONO_SECTIONS;
}

int PT_DECLSPEC filtChainDesign(int i_slope_db, double r_low_cut_hz, double r_high_cut_hz, double r_mono_hz,
	double r_sample_rate_hz, FiltChainParams *sp_out)
{
	double max_hz = FILT_CHAIN_MAX_FREQ_RATIO * r_sample_rate_hz;

	sp_out->num_low_cut = 0;
	sp_out->num_low_cut_mirror = 0;
	sp_out->num_high_cut = 0;
	sp_out->num_high_cut_mirror = 0;
	sp_out->num_mono_side = 0;
	sp_out->num_mono_side_mirror = 0;

	if (r_low_cut_hz > 0.0)
	{
		sp_out->num_low_cut = filtLowCutDesign(i_slope_db, FILT_LOW_CUT_Q_NEUTRAL, r_low_cut_hz, r_sample_rate_hz, 1,
			sp_out->low_cut);
		sp_out->num_low_cut_mirror = filtLowCutDesign(i_slope_db, FILT_LOW_CUT_Q_NEUTRAL, r_low_cut_hz, r_sample_rate_hz, 0,
			sp_out->low_cut_mirror);
	}

	// At or above Nyquist there is nothing for a high cut to remove: the stage is not designed. Between the designer's
	// cap and Nyquist it is clamped to the cap.
	if (r_high_cut_hz > 0.0 && r_high_cut_hz < 0.5 * r_sample_rate_hz)
	{
		if (r_high_cut_hz > max_hz)
		{
			r_high_cut_hz = max_hz;
		}
		sp_out->num_high_cut = filtLowCutDesign(i_slope_db, FILT_LOW_CUT_Q_NEUTRAL, r_high_cut_hz, r_sample_rate_hz, 0,
			sp_out->high_cut);
		sp_out->num_high_cut_mirror = filtLowCutDesign(i_slope_db, FILT_LOW_CUT_Q_NEUTRAL, r_high_cut_hz, r_sample_rate_hz, 1,
			sp_out->high_cut_mirror);
	}

	if (r_mono_hz > 0.0)
	{
		sp_out->num_mono_side = filtChainDesignLinkwitzRiley(r_mono_hz, r_sample_rate_hz, 1, sp_out->mono_side);
		sp_out->num_mono_side_mirror = filtChainDesignLinkwitzRiley(r_mono_hz, r_sample_rate_hz, 0, sp_out->mono_side_mirror);
	}

	sp_out->audible_mask = ((sp_out->num_low_cut > 0) ? FILT_CHAIN_CHANGED_LOW_CUT : 0) |
		((sp_out->num_high_cut > 0) ? FILT_CHAIN_CHANGED_HIGH_CUT : 0) |
		((sp_out->num_mono_side > 0) ? FILT_CHAIN_CHANGED_MONO : 0);

	return sp_out->num_low_cut + sp_out->num_high_cut + sp_out->num_mono_side;
}

static void filtChainResetBiquads(FiltLowCutBiquadState *sp_states, int i_count)
{
	int i;

	for (i = 0; i < i_count; i++)
	{
		filtChainResetBiquad(&sp_states[i]);
	}
}

void PT_DECLSPEC filtChainResetChannelState(FiltChainChannelState *sp_state)
{
	filtChainResetBiquads(sp_state->low_cut, FILT_LOW_CUT_MAX_SECTIONS);
	filtChainResetBiquads(sp_state->low_cut_mirror, FILT_LOW_CUT_MAX_SECTIONS);
	filtChainResetBiquads(sp_state->high_cut, FILT_LOW_CUT_MAX_SECTIONS);
	filtChainResetBiquads(sp_state->high_cut_mirror, FILT_LOW_CUT_MAX_SECTIONS);
}

void PT_DECLSPEC filtChainResetState(FiltChainState *sp_state)
{
	int c;

	for (c = 0; c < FILT_CHAIN_MAX_CHANNELS; c++)
	{
		filtChainResetChannelState(&sp_state->channels[c]);
	}
	filtChainResetBiquads(sp_state->mono_side, FILT_CHAIN_MONO_SECTIONS);
	filtChainResetBiquads(sp_state->mono_side_mirror, FILT_CHAIN_MONO_SECTIONS);
}

static bool filtChainSameSections(const FiltLowCutSection *cp_a, int i_num_a, const FiltLowCutSection *cp_b, int i_num_b)
{
	int i;

	if (i_num_a != i_num_b)
	{
		return false;
	}
	for (i = 0; i < i_num_a; i++)
	{
		if (cp_a[i].order != cp_b[i].order)
		{
			return false;
		}
	}

	return true;
}

int PT_DECLSPEC filtChainChangedStages(const FiltChainParams *cp_a, const FiltChainParams *cp_b)
{
	int changed = 0;

	if (!filtChainSameSections(cp_a->low_cut, cp_a->num_low_cut, cp_b->low_cut, cp_b->num_low_cut) ||
		!filtChainSameSections(cp_a->low_cut_mirror, cp_a->num_low_cut_mirror, cp_b->low_cut_mirror, cp_b->num_low_cut_mirror))
	{
		changed |= FILT_CHAIN_CHANGED_LOW_CUT;
	}
	if (!filtChainSameSections(cp_a->high_cut, cp_a->num_high_cut, cp_b->high_cut, cp_b->num_high_cut) ||
		!filtChainSameSections(cp_a->high_cut_mirror, cp_a->num_high_cut_mirror, cp_b->high_cut_mirror,
			cp_b->num_high_cut_mirror))
	{
		changed |= FILT_CHAIN_CHANGED_HIGH_CUT;
	}
	if (!filtChainSameSections(cp_a->mono_side, cp_a->num_mono_side, cp_b->mono_side, cp_b->num_mono_side) ||
		!filtChainSameSections(cp_a->mono_side_mirror, cp_a->num_mono_side_mirror, cp_b->mono_side_mirror,
			cp_b->num_mono_side_mirror))
	{
		changed |= FILT_CHAIN_CHANGED_MONO;
	}

	return changed;
}

bool PT_DECLSPEC filtChainSameLayout(const FiltChainParams *cp_a, const FiltChainParams *cp_b)
{
	return filtChainChangedStages(cp_a, cp_b) == 0;
}

// Keeps (copies) or resets the histories of one cascade.
static void filtChainCarryBiquads(bool b_reset, const FiltLowCutBiquadState *cp_from, FiltLowCutBiquadState *sp_to,
	int i_count)
{
	int i;

	for (i = 0; i < i_count; i++)
	{
		if (b_reset)
		{
			filtChainResetBiquad(&sp_to[i]);
		}
		else
		{
			sp_to[i] = cp_from[i];
		}
	}
}

int PT_DECLSPEC filtChainCarryState(const FiltChainParams *cp_old, const FiltChainParams *cp_new,
	const FiltChainState *cp_old_state, FiltChainState *sp_new_state)
{
	const int changed = filtChainChangedStages(cp_old, cp_new);
	const bool low_reset = (changed & FILT_CHAIN_CHANGED_LOW_CUT) != 0;
	const bool high_reset = (changed & FILT_CHAIN_CHANGED_HIGH_CUT) != 0;
	const bool mono_reset = (changed & FILT_CHAIN_CHANGED_MONO) != 0;
	int c;

	for (c = 0; c < FILT_CHAIN_MAX_CHANNELS; c++)
	{
		const FiltChainChannelState *from = &cp_old_state->channels[c];
		FiltChainChannelState *to = &sp_new_state->channels[c];

		filtChainCarryBiquads(low_reset, from->low_cut, to->low_cut, FILT_LOW_CUT_MAX_SECTIONS);
		filtChainCarryBiquads(low_reset, from->low_cut_mirror, to->low_cut_mirror, FILT_LOW_CUT_MAX_SECTIONS);
		filtChainCarryBiquads(high_reset, from->high_cut, to->high_cut, FILT_LOW_CUT_MAX_SECTIONS);
		filtChainCarryBiquads(high_reset, from->high_cut_mirror, to->high_cut_mirror, FILT_LOW_CUT_MAX_SECTIONS);
	}
	filtChainCarryBiquads(mono_reset, cp_old_state->mono_side, sp_new_state->mono_side, FILT_CHAIN_MONO_SECTIONS);
	filtChainCarryBiquads(mono_reset, cp_old_state->mono_side_mirror, sp_new_state->mono_side_mirror,
		FILT_CHAIN_MONO_SECTIONS);

	return changed;
}

void PT_DECLSPEC filtChainProcessFrame(const float *cp_in, float *fp_out, float *fp_removed, int i_num_channels,
	int i_removed_mask, const FiltChainParams *cp_params, FiltChainState *sp_state)
{
	filtChainProcessFrameDual(cp_in, fp_out, fp_removed, i_removed_mask, NULL, 0, i_num_channels, cp_params, sp_state);
}

void PT_DECLSPEC filtChainProcessFrameDual(const float *cp_in, float *fp_out, float *fp_removed, int i_removed_mask,
	float *fp_removed_b, int i_removed_mask_b, int i_num_channels, const FiltChainParams *cp_params, FiltChainState *sp_state)
{
	const int audible = cp_params->audible_mask;
	double signal[FILT_CHAIN_MAX_CHANNELS];
	double removed[FILT_CHAIN_MAX_CHANNELS];
	double removed_b[FILT_CHAIN_MAX_CHANNELS];
	int c;

	if (fp_removed_b == NULL)
	{
		i_removed_mask_b = 0;
	}

	assert(i_num_channels >= 0 && i_num_channels <= FILT_CHAIN_MAX_CHANNELS);
	if (i_num_channels > FILT_CHAIN_MAX_CHANNELS)
	{
		i_num_channels = FILT_CHAIN_MAX_CHANNELS;
	}

	for (c = 0; c < i_num_channels; c++)
	{
		signal[c] = (double)cp_in[c];
		removed[c] = 0.0;
		removed_b[c] = 0.0;
	}

	if (i_num_channels >= 2 && cp_params->num_mono_side > 0)
	{
		double mid = 0.5 * (signal[0] + signal[1]);
		double side = 0.5 * (signal[0] - signal[1]);
		double side_hp = (double)filtLowCutProcessSample((realtype)side, cp_params->mono_side, cp_params->num_mono_side,
			sp_state->mono_side);
		double side_lp = (double)filtLowCutProcessSample((realtype)side, cp_params->mono_side_mirror,
			cp_params->num_mono_side_mirror, sp_state->mono_side_mirror);

		if (i_removed_mask & FILT_CHAIN_CHANGED_MONO)
		{
			removed[0] += side_lp;
			removed[1] -= side_lp;
		}
		if (i_removed_mask_b & FILT_CHAIN_CHANGED_MONO)
		{
			removed_b[0] += side_lp;
			removed_b[1] -= side_lp;
		}
		if (audible & FILT_CHAIN_CHANGED_MONO)
		{
			signal[0] = mid + side_hp;
			signal[1] = mid - side_hp;
		}
	}

	for (c = 0; c < i_num_channels; c++)
	{
		FiltChainChannelState *channel = &sp_state->channels[c];

		if (cp_params->num_low_cut > 0)
		{
			double filtered = (double)filtLowCutProcessSample((realtype)signal[c], cp_params->low_cut, cp_params->num_low_cut,
				channel->low_cut);
			double mirror = (double)filtLowCutProcessSample((realtype)signal[c], cp_params->low_cut_mirror,
				cp_params->num_low_cut_mirror, channel->low_cut_mirror);

			if (i_removed_mask & FILT_CHAIN_CHANGED_LOW_CUT)
			{
				removed[c] += mirror;
			}
			if (i_removed_mask_b & FILT_CHAIN_CHANGED_LOW_CUT)
			{
				removed_b[c] += mirror;
			}
			if (audible & FILT_CHAIN_CHANGED_LOW_CUT)
			{
				signal[c] = filtered;
			}
		}
		if (cp_params->num_high_cut > 0)
		{
			double filtered = (double)filtLowCutProcessSample((realtype)signal[c], cp_params->high_cut, cp_params->num_high_cut,
				channel->high_cut);
			double mirror = (double)filtLowCutProcessSample((realtype)signal[c], cp_params->high_cut_mirror,
				cp_params->num_high_cut_mirror, channel->high_cut_mirror);

			if (i_removed_mask & FILT_CHAIN_CHANGED_HIGH_CUT)
			{
				removed[c] += mirror;
			}
			if (i_removed_mask_b & FILT_CHAIN_CHANGED_HIGH_CUT)
			{
				removed_b[c] += mirror;
			}
			if (audible & FILT_CHAIN_CHANGED_HIGH_CUT)
			{
				signal[c] = filtered;
			}
		}

		fp_out[c] = (float)signal[c];
		fp_removed[c] = (float)removed[c];
		if (fp_removed_b != NULL)
		{
			fp_removed_b[c] = (float)removed_b[c];
		}
	}
}
