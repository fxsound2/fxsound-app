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
#ifndef _FILT_CHAIN_H_
#define _FILT_CHAIN_H_

#include "codedefs.h"
#include "FiltLowCut.h"

#define FILT_CHAIN_MAX_CHANNELS  FILT_LOW_CUT_MAX_CHANNELS
#define FILT_CHAIN_MONO_SECTIONS  2

#define FILT_CHAIN_MAX_FREQ_RATIO  0.45

// One bit per stage. Used for the stages whose section layout differs (filtChainChangedStages /
// filtChainCarryState), for FiltChainParams::audible_mask and for the removed mask of filtChainProcessFrame.
#define FILT_CHAIN_CHANGED_LOW_CUT   0x1
#define FILT_CHAIN_CHANGED_HIGH_CUT  0x2
#define FILT_CHAIN_CHANGED_MONO      0x4
#define FILT_CHAIN_STAGES_ALL        (FILT_CHAIN_CHANGED_LOW_CUT | FILT_CHAIN_CHANGED_HIGH_CUT | FILT_CHAIN_CHANGED_MONO)

// Each stage has a main cascade (what is heard) and a mirror cascade (what the stage removes, played by the preview):
// low cut = high-pass / low-pass mirror, high cut = low-pass / high-pass mirror, both at the same cutoff, slope and Q;
// stereo to mono = side high-pass / side low-pass mirror (both LR4).
struct FiltChainParams {
	FiltLowCutSection low_cut[FILT_LOW_CUT_MAX_SECTIONS];
	int num_low_cut;
	FiltLowCutSection low_cut_mirror[FILT_LOW_CUT_MAX_SECTIONS];
	int num_low_cut_mirror;
	FiltLowCutSection high_cut[FILT_LOW_CUT_MAX_SECTIONS];
	int num_high_cut;
	FiltLowCutSection high_cut_mirror[FILT_LOW_CUT_MAX_SECTIONS];
	int num_high_cut_mirror;
	FiltLowCutSection mono_side[FILT_CHAIN_MONO_SECTIONS];
	int num_mono_side;
	FiltLowCutSection mono_side_mirror[FILT_CHAIN_MONO_SECTIONS];
	int num_mono_side_mirror;
	// Stages (FILT_CHAIN_CHANGED_* bits) whose output is heard. A designed stage outside this mask still runs, so it
	// keeps its history and can report what it would remove, but passes its input through. Not part of the layout:
	// changing only this mask keeps every stage's history (filtChainCarryState).
	int audible_mask;
};

struct FiltChainChannelState {
	FiltLowCutBiquadState low_cut[FILT_LOW_CUT_MAX_SECTIONS];
	FiltLowCutBiquadState low_cut_mirror[FILT_LOW_CUT_MAX_SECTIONS];
	FiltLowCutBiquadState high_cut[FILT_LOW_CUT_MAX_SECTIONS];
	FiltLowCutBiquadState high_cut_mirror[FILT_LOW_CUT_MAX_SECTIONS];
};

struct FiltChainState {
	FiltChainChannelState channels[FILT_CHAIN_MAX_CHANNELS];
	FiltLowCutBiquadState mono_side[FILT_CHAIN_MONO_SECTIONS];
	FiltLowCutBiquadState mono_side_mirror[FILT_CHAIN_MONO_SECTIONS];
};

// Designs every stage whose frequency is above 0 (high cut clamped to FILT_CHAIN_MAX_FREQ_RATIO), main and mirror
// cascades, and sets audible_mask to the designed stages; callers may narrow audible_mask afterwards.
int PT_DECLSPEC filtChainDesign(int i_slope_db, double r_low_cut_hz, double r_high_cut_hz, double r_mono_hz,
	double r_sample_rate_hz, FiltChainParams *sp_out);

void PT_DECLSPEC filtChainResetState(FiltChainState *sp_state);

void PT_DECLSPEC filtChainResetChannelState(FiltChainChannelState *sp_state);

bool PT_DECLSPEC filtChainSameLayout(const FiltChainParams *cp_a, const FiltChainParams *cp_b);

// Stages whose section layout (main or mirror cascade) differs.
int PT_DECLSPEC filtChainChangedStages(const FiltChainParams *cp_a, const FiltChainParams *cp_b);

// Builds the state for cp_new from the state that ran cp_old: each stage whose section layout is unchanged keeps
// its history (main and mirror cascades), each stage whose layout changed starts from zero. Allocation and lock free.
// cp_old_state and sp_new_state may be the same. Returns the FILT_CHAIN_CHANGED_* bits.
int PT_DECLSPEC filtChainCarryState(const FiltChainParams *cp_old, const FiltChainParams *cp_new,
	const FiltChainState *cp_old_state, FiltChainState *sp_new_state);

// Runs one frame through mono -> low cut -> high cut. Each designed stage runs its main and mirror cascades on its
// input; its output is the main cascade's when the stage is in cp_params->audible_mask, otherwise its unchanged input.
// fp_removed is the sum of the mirror outputs of the stages in i_removed_mask (stereo to mono: +side low-pass on
// channel 0, -side low-pass on channel 1, nothing elsewhere). Allocation and lock free.
void PT_DECLSPEC filtChainProcessFrame(const float *cp_in, float *fp_out, float *fp_removed, int i_num_channels,
	int i_removed_mask, const FiltChainParams *cp_params, FiltChainState *sp_state);

// As filtChainProcessFrame, plus a second removed output: fp_removed_b gets the sum of the mirror outputs of the stages
// in i_removed_mask_b, from the same mirror cascades in the same call (the chain runs once, its state advances once).
// fp_removed_b may be NULL (i_removed_mask_b is then ignored). Used to crossfade between two preview selections.
void PT_DECLSPEC filtChainProcessFrameDual(const float *cp_in, float *fp_out, float *fp_removed, int i_removed_mask,
	float *fp_removed_b, int i_removed_mask_b, int i_num_channels, const FiltChainParams *cp_params, FiltChainState *sp_state);

#endif
