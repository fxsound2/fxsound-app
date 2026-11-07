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
#include <math.h>

#include "codedefs.h"
#include "FiltLowCut.h"

#define FILT_LOW_CUT_PI  3.14159265358979323846

#define FILT_LOW_CUT_HALF_POWER_DB  (-3.01029995663981195)

#define FILT_LOW_CUT_MAX_FREQ_RATIO  0.45

#define FILT_LOW_CUT_CALIBRATE_ITERATIONS  60

int PT_DECLSPEC filtLowCutSnapSlopeDb(int i_slope_db)
{
	static const int slopes[] = { 6, 12, 18, 24, 36, 48 };
	int i;

	for (i = 0; i < (int)(sizeof(slopes) / sizeof(slopes[0])); i++)
	{
		if (i_slope_db <= slopes[i])
		{
			return slopes[i];
		}
	}

	return FILT_LOW_CUT_SLOPE_MAX_DB;
}

double PT_DECLSPEC filtLowCutClampQ(double r_q)
{
	if (r_q != r_q)
	{
		return FILT_LOW_CUT_Q_NEUTRAL;
	}
	if (r_q < FILT_LOW_CUT_Q_MIN)
	{
		return FILT_LOW_CUT_Q_MIN;
	}
	if (r_q > FILT_LOW_CUT_Q_MAX)
	{
		return FILT_LOW_CUT_Q_MAX;
	}

	return r_q;
}

static int filtLowCutAddButterworth(int i_order, FiltLowCutPrototypeSection *sp_out, int i_count)
{
	int k;

	for (k = 1; k <= i_order / 2; k++)
	{
		sp_out[i_count].w0 = 1.0;
		sp_out[i_count].q = 1.0 / (2.0 * sin((2.0 * k - 1.0) * FILT_LOW_CUT_PI / (2.0 * i_order)));
		i_count++;
	}
	if (i_order % 2)
	{
		sp_out[i_count].w0 = 1.0;
		sp_out[i_count].q = 0.0;
		i_count++;
	}

	return i_count;
}

int PT_DECLSPEC filtLowCutPrototype(int i_order, double r_q, FiltLowCutPrototypeSection *sp_out)
{
	int count = 0;
	int top = -1;
	int i;

	if (i_order == 1 || i_order == 2 || i_order == 3 || i_order == 4 || i_order == 6 || i_order == 8)
	{
		count = filtLowCutAddButterworth(i_order, sp_out, 0);
	}

	for (i = 0; i < count; i++)
	{
		if (sp_out[i].q > 0.0 && (top < 0 || sp_out[i].q > sp_out[top].q))
		{
			top = i;
		}
	}
	if (top >= 0)
	{
		sp_out[top].q *= filtLowCutClampQ(r_q) / FILT_LOW_CUT_Q_NEUTRAL;
	}

	return count;
}

static void filtLowCutDesignSection(double r_freq_hz, double r_q, double r_sample_rate_hz, int i_high_pass_flag,
	FiltLowCutSection *sp_section)
{
	double max_freq_hz = FILT_LOW_CUT_MAX_FREQ_RATIO * r_sample_rate_hz;

	if (r_freq_hz > max_freq_hz)
	{
		r_freq_hz = max_freq_hz;
	}

	sp_section->high_pass = i_high_pass_flag ? 1 : 0;
	if (r_q <= 0.0)
	{
		double k = tan(FILT_LOW_CUT_PI * r_freq_hz / r_sample_rate_hz);

		sp_section->order = 1;
		sp_section->gain = i_high_pass_flag ? 1.0 / (1.0 + k) : k / (1.0 + k);
		sp_section->b0 = sp_section->gain;
		sp_section->b1 = i_high_pass_flag ? -sp_section->gain : sp_section->gain;
		sp_section->b2 = 0.0;
		sp_section->a1 = (k - 1.0) / (k + 1.0);
		sp_section->a2 = 0.0;
	}
	else
	{
		double w0 = 2.0 * FILT_LOW_CUT_PI * r_freq_hz / r_sample_rate_hz;
		double cos_w0 = cos(w0);
		double alpha = sin(w0) / (2.0 * r_q);
		double a0 = 1.0 + alpha;

		sp_section->order = 2;
		sp_section->gain = (i_high_pass_flag ? (1.0 + cos_w0) : (1.0 - cos_w0)) / (2.0 * a0);
		sp_section->b0 = sp_section->gain;
		sp_section->b1 = (i_high_pass_flag ? -2.0 : 2.0) * sp_section->gain;
		sp_section->b2 = sp_section->gain;
		sp_section->a1 = -2.0 * cos_w0 / a0;
		sp_section->a2 = (1.0 - alpha) / a0;
	}
}

void PT_DECLSPEC filtLowCutDesignFromPrototype(const FiltLowCutPrototypeSection *cp_proto, int i_num_sections,
	double r_scale_hz, double r_sample_rate_hz, int i_high_pass_flag, FiltLowCutSection *sp_out)
{
	int i;

	for (i = 0; i < i_num_sections; i++)
	{
		double freq_hz = i_high_pass_flag ? r_scale_hz / cp_proto[i].w0 : r_scale_hz * cp_proto[i].w0;

		filtLowCutDesignSection(freq_hz, cp_proto[i].q, r_sample_rate_hz, i_high_pass_flag, &sp_out[i]);
	}
}

int PT_DECLSPEC filtLowCutDesign(int i_slope_db, double r_q, double r_cutoff_hz, double r_sample_rate_hz,
	int i_high_pass_flag, FiltLowCutSection *sp_out)
{
	FiltLowCutPrototypeSection proto[FILT_LOW_CUT_MAX_SECTIONS];
	FiltLowCutSection trial[FILT_LOW_CUT_MAX_SECTIONS];
	double target_db, min_w0, max_w0, limit_hz, lo, hi;
	int num_sections, i, iteration;

	if (!(r_cutoff_hz > 0.0) || !(r_sample_rate_hz > 0.0))
	{
		return 0;
	}

	num_sections = filtLowCutPrototype(filtLowCutSnapSlopeDb(i_slope_db) / 6, r_q, proto);
	if (num_sections == 0)
	{
		return 0;
	}

	target_db = FILT_LOW_CUT_HALF_POWER_DB;
	min_w0 = max_w0 = proto[0].w0;
	for (i = 1; i < num_sections; i++)
	{
		min_w0 = (proto[i].w0 < min_w0) ? proto[i].w0 : min_w0;
		max_w0 = (proto[i].w0 > max_w0) ? proto[i].w0 : max_w0;
	}

	lo = r_cutoff_hz / 100.0;
	hi = r_cutoff_hz * 100.0;
	limit_hz = FILT_LOW_CUT_MAX_FREQ_RATIO * r_sample_rate_hz * (i_high_pass_flag ? min_w0 : 1.0 / max_w0);
	if (hi > limit_hz)
	{
		hi = limit_hz;
	}

	for (iteration = 0; iteration < FILT_LOW_CUT_CALIBRATE_ITERATIONS; iteration++)
	{
		double mid = sqrt(lo * hi);
		bool below_target;

		filtLowCutDesignFromPrototype(proto, num_sections, mid, r_sample_rate_hz, i_high_pass_flag, trial);
		below_target = filtLowCutCascadeResponseDb(r_cutoff_hz, r_sample_rate_hz, trial, num_sections) < target_db;
		if (below_target == (i_high_pass_flag != 0))
		{
			hi = mid;
		}
		else
		{
			lo = mid;
		}
	}

	filtLowCutDesignFromPrototype(proto, num_sections, sqrt(lo * hi), r_sample_rate_hz, i_high_pass_flag, sp_out);
	return num_sections;
}

void PT_DECLSPEC filtLowCutResetChannelState(FiltLowCutChannelState *sp_state)
{
	int i;

	for (i = 0; i < FILT_LOW_CUT_MAX_SECTIONS; i++)
	{
		sp_state->hp_sections[i].in_minus1 = 0.0;
		sp_state->hp_sections[i].in_minus2 = 0.0;
		sp_state->hp_sections[i].out_minus1 = 0.0;
		sp_state->hp_sections[i].out_minus2 = 0.0;

		sp_state->lp_sections[i].in_minus1 = 0.0;
		sp_state->lp_sections[i].in_minus2 = 0.0;
		sp_state->lp_sections[i].out_minus1 = 0.0;
		sp_state->lp_sections[i].out_minus2 = 0.0;
	}
}

static inline double filtLowCutRunSection(double d_in, const FiltLowCutSection *cp_section, FiltLowCutBiquadState *sp_state)
{
	double d_out = cp_section->b0 * d_in + cp_section->b1 * sp_state->in_minus1 + cp_section->b2 * sp_state->in_minus2 -
		cp_section->a1 * sp_state->out_minus1 - cp_section->a2 * sp_state->out_minus2;

	if (d_out < 1e-30 && d_out > -1e-30)
	{
		d_out = 0.0;
	}

	sp_state->in_minus2 = sp_state->in_minus1;
	sp_state->in_minus1 = d_in;
	sp_state->out_minus2 = sp_state->out_minus1;
	sp_state->out_minus1 = d_out;

	return d_out;
}

realtype PT_DECLSPEC filtLowCutProcessSample(realtype r_input, const FiltLowCutSection *cp_sections, int i_num_sections,
	FiltLowCutBiquadState *sp_states)
{
	double d_sample = (double)r_input;
	int i;

	for (i = 0; i < i_num_sections; i++)
	{
		d_sample = filtLowCutRunSection(d_sample, &cp_sections[i], &sp_states[i]);
	}

	return (realtype)d_sample;
}

void PT_DECLSPEC filtLowCutCascadeResponse(double r_freq_hz, double r_sample_rate_hz, const FiltLowCutSection *cp_sections,
	int i_num_sections, double *dp_re, double *dp_im)
{
	double w = 2.0 * FILT_LOW_CUT_PI * r_freq_hz / r_sample_rate_hz;
	double re = 1.0, im = 0.0;
	int i;

	for (i = 0; i < i_num_sections; i++)
	{
		const FiltLowCutSection *section = &cp_sections[i];
		double factor = section->high_pass ? 2.0 * sin(w / 2.0) : 2.0 * cos(w / 2.0);
		double num_mag = section->gain * ((section->order == 2) ? factor * factor : factor);
		double num_phase = -section->order * w / 2.0 + (section->high_pass ? section->order * FILT_LOW_CUT_PI / 2.0 : 0.0);
		double num_re = num_mag * cos(num_phase);
		double num_im = num_mag * sin(num_phase);
		double den_re = 1.0 + section->a1 * cos(w) + section->a2 * cos(2.0 * w);
		double den_im = -(section->a1 * sin(w) + section->a2 * sin(2.0 * w));
		double den_power = den_re * den_re + den_im * den_im;
		double h_re, h_im, next_re;

		if (den_power <= 0.0)
		{
			*dp_re = 0.0;
			*dp_im = 0.0;
			return;
		}
		h_re = (num_re * den_re + num_im * den_im) / den_power;
		h_im = (num_im * den_re - num_re * den_im) / den_power;
		next_re = re * h_re - im * h_im;
		im = re * h_im + im * h_re;
		re = next_re;
	}

	*dp_re = re;
	*dp_im = im;
}

double PT_DECLSPEC filtLowCutCascadeResponseDb(double r_freq_hz, double r_sample_rate_hz, const FiltLowCutSection *cp_sections,
	int i_num_sections)
{
	double total_db = 0.0;
	int i;

	for (i = 0; i < i_num_sections; i++)
	{
		double re, im, power;

		filtLowCutCascadeResponse(r_freq_hz, r_sample_rate_hz, &cp_sections[i], 1, &re, &im);
		power = re * re + im * im;
		total_db += 10.0 * log10((power > 1e-300) ? power : 1e-300);
	}

	return total_db;
}
