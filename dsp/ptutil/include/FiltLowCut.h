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
#ifndef _FILT_LOW_CUT_H_
#define _FILT_LOW_CUT_H_

#include "codedefs.h"

#define FILT_LOW_CUT_MAX_CHANNELS  8
#define FILT_LOW_CUT_MAX_SECTIONS  4

#define FILT_LOW_CUT_Q_MIN      0.5
#define FILT_LOW_CUT_Q_MAX      2.0
#define FILT_LOW_CUT_Q_NEUTRAL  0.71

#define FILT_LOW_CUT_SLOPE_MIN_DB  6
#define FILT_LOW_CUT_SLOPE_MAX_DB  48

struct FiltLowCutSection {
	double b0;
	double b1;
	double b2;
	double a1;
	double a2;
	double gain;
	int order;
	int high_pass;
};

struct FiltLowCutBiquadState {
	double in_minus1;
	double in_minus2;
	double out_minus1;
	double out_minus2;
};

struct FiltLowCutChannelState {
	FiltLowCutBiquadState hp_sections[FILT_LOW_CUT_MAX_SECTIONS];
	FiltLowCutBiquadState lp_sections[FILT_LOW_CUT_MAX_SECTIONS];
};

struct FiltLowCutPrototypeSection {
	double w0;
	double q;
};

int PT_DECLSPEC filtLowCutSnapSlopeDb(int i_slope_db);
double PT_DECLSPEC filtLowCutClampQ(double r_q);

int PT_DECLSPEC filtLowCutPrototype(int i_order, double r_q, FiltLowCutPrototypeSection *sp_out);

void PT_DECLSPEC filtLowCutDesignFromPrototype(const FiltLowCutPrototypeSection *cp_proto, int i_num_sections,
	double r_scale_hz, double r_sample_rate_hz, int i_high_pass_flag, FiltLowCutSection *sp_out);

int PT_DECLSPEC filtLowCutDesign(int i_slope_db, double r_q, double r_cutoff_hz, double r_sample_rate_hz,
	int i_high_pass_flag, FiltLowCutSection *sp_out);

void PT_DECLSPEC filtLowCutResetChannelState(FiltLowCutChannelState *sp_state);

realtype PT_DECLSPEC filtLowCutProcessSample(realtype r_input, const FiltLowCutSection *cp_sections, int i_num_sections,
	FiltLowCutBiquadState *sp_states);

void PT_DECLSPEC filtLowCutCascadeResponse(double r_freq_hz, double r_sample_rate_hz, const FiltLowCutSection *cp_sections,
	int i_num_sections, double *dp_re, double *dp_im);
double PT_DECLSPEC filtLowCutCascadeResponseDb(double r_freq_hz, double r_sample_rate_hz, const FiltLowCutSection *cp_sections,
	int i_num_sections);

#endif
