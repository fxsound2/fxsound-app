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
#include <stdio.h>
#include <stdlib.h>

#include "codedefs.h"
#include "FiltLowCut.h"
#include "FiltChain.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, ...) \
	do { \
		g_checks++; \
		if (!(cond)) { \
			g_failures++; \
			printf("FAIL line %d: ", __LINE__); \
			printf(__VA_ARGS__); \
			printf("\n"); \
		} \
	} while (0)

static const double kPi = 3.14159265358979323846;

static double randomSample()
{
	return ((double)rand() / (double)RAND_MAX) * 2.0 - 1.0;
}

static double powerDb(double rms_out, double rms_in)
{
	return 10.0 * log10((rms_out * rms_out + 1e-300) / (rms_in * rms_in));
}

// Runs a chain over num_frames frames. in/out/removed hold num_frames * num_channels floats.
static void runChain(const FiltChainParams& params, int num_channels, int removed_mask, const float *in, float *out,
	float *removed, int num_frames)
{
	FiltChainState state;
	int n;

	filtChainResetState(&state);
	for (n = 0; n < num_frames; n++)
	{
		filtChainProcessFrame(in + n * num_channels, out + n * num_channels, removed + n * num_channels, num_channels,
			removed_mask, &params, &state);
	}
}

static double rmsFrom(const float *buf, int num_channels, int num_frames, int from_frame)
{
	double sum = 0.0;
	int count = 0;
	int i;

	for (i = from_frame * num_channels; i < num_frames * num_channels; i++)
	{
		sum += (double)buf[i] * (double)buf[i];
		count++;
	}

	return sqrt(sum / (double)count);
}

static void testDesignOffStages()
{
	FiltChainParams p;
	FiltLowCutSection tmp[FILT_LOW_CUT_MAX_SECTIONS];
	int n;

	n = filtChainDesign(24, 0.0, 0.0, 0.0, 48000.0, &p);
	CHECK(n == 0 && p.num_low_cut == 0 && p.num_high_cut == 0 && p.num_mono_side == 0, "all off: %d %d %d %d",
		n, p.num_low_cut, p.num_high_cut, p.num_mono_side);

	n = filtChainDesign(24, 20.0, 0.0, 0.0, 48000.0, &p);
	CHECK(p.num_low_cut == filtLowCutDesign(24, 0.71, 20.0, 48000.0, 1, tmp) && p.num_low_cut > 0 && p.num_high_cut == 0 &&
		p.num_mono_side == 0 && n == p.num_low_cut, "low cut only: %d %d %d, total %d", p.num_low_cut, p.num_high_cut,
		p.num_mono_side, n);

	n = filtChainDesign(24, 0.0, 18000.0, 0.0, 48000.0, &p);
	CHECK(p.num_low_cut == 0 && p.num_high_cut > 0 && p.num_mono_side == 0 && n == p.num_high_cut,
		"high cut only: %d %d %d", p.num_low_cut, p.num_high_cut, p.num_mono_side);

	n = filtChainDesign(24, 0.0, 0.0, 150.0, 48000.0, &p);
	CHECK(p.num_low_cut == 0 && p.num_high_cut == 0 && p.num_mono_side == 2 && n == 2, "mono only: %d %d %d",
		p.num_low_cut, p.num_high_cut, p.num_mono_side);
	CHECK(p.mono_side[0].order == 2 && p.mono_side[1].order == 2 && p.mono_side[0].high_pass == 1 &&
		p.mono_side[1].high_pass == 1, "mono sections are second-order high-pass");
	CHECK(p.num_mono_side_mirror == 2 && p.mono_side_mirror[0].order == 2 && p.mono_side_mirror[1].order == 2 &&
		p.mono_side_mirror[0].high_pass == 0 && p.mono_side_mirror[1].high_pass == 0,
		"mono mirror sections are second-order low-pass");

	// Mirrors: same slope and cutoff, opposite direction.
	filtChainDesign(48, 30.0, 16000.0, 0.0, 48000.0, &p);
	CHECK(p.num_low_cut_mirror == p.num_low_cut && p.low_cut_mirror[0].high_pass == 0 && p.low_cut[0].high_pass == 1,
		"low cut mirror is a low-pass with as many sections (%d vs %d)", p.num_low_cut_mirror, p.num_low_cut);
	CHECK(p.num_high_cut_mirror == p.num_high_cut && p.high_cut_mirror[0].high_pass == 1 && p.high_cut[0].high_pass == 0,
		"high cut mirror is a high-pass with as many sections (%d vs %d)", p.num_high_cut_mirror, p.num_high_cut);
	filtChainDesign(24, 0.0, 0.0, 0.0, 48000.0, &p);
	CHECK(p.num_low_cut_mirror == 0 && p.num_high_cut_mirror == 0 && p.num_mono_side_mirror == 0, "off stages have no mirrors");

	// By default every designed stage is audible; stages that are Off have no bit.
	filtChainDesign(24, 0.0, 0.0, 150.0, 48000.0, &p);
	CHECK(p.audible_mask == FILT_CHAIN_CHANGED_MONO, "mono only: audible mask %d", p.audible_mask);
	filtChainDesign(24, 20.0, 18000.0, 150.0, 48000.0, &p);
	CHECK(p.audible_mask == FILT_CHAIN_STAGES_ALL, "all designed: audible mask %d", p.audible_mask);
	filtChainDesign(24, 0.0, 0.0, 0.0, 48000.0, &p);
	CHECK(p.audible_mask == 0, "all off: audible mask %d", p.audible_mask);
}

static void testMonoLinkwitzRiley()
{
	FiltChainParams p;
	double at_corner, at_1500;

	filtChainDesign(24, 0.0, 0.0, 150.0, 48000.0, &p);
	at_corner = filtLowCutCascadeResponseDb(150.0, 48000.0, p.mono_side, 2);
	at_1500 = filtLowCutCascadeResponseDb(1500.0, 48000.0, p.mono_side, 2);
	CHECK(fabs(at_corner + 6.02) < 0.05, "mono corner %.4f dB", at_corner);
	CHECK(at_1500 > -0.1, "mono at 1500 Hz %.4f dB", at_1500);
}

static void testMonoKeepsMidExact()
{
	const int frames = 4800;
	static float in[4800 * 2], out[4800 * 2], removed[4800 * 2];
	FiltChainParams p;
	double max_diff = 0.0, max_removed = 0.0;
	int n;

	for (n = 0; n < frames; n++)
	{
		in[n * 2] = in[n * 2 + 1] = (float)randomSample();
	}
	filtChainDesign(24, 0.0, 0.0, 150.0, 48000.0, &p);
	runChain(p, 2, FILT_CHAIN_CHANGED_MONO, in, out, removed, frames);
	for (n = 0; n < frames * 2; n++)
	{
		max_diff = fmax(max_diff, fabs((double)out[n] - (double)in[n]));
		max_removed = fmax(max_removed, fabs((double)removed[n]));
	}
	CHECK(max_diff < 1e-6, "mid changed by %g", max_diff);
	CHECK(max_removed == 0.0, "removed of L == R input not exactly zero: %g", max_removed);
}

static void testMonoFoldsSideBass()
{
	const int frames = 48000 * 2;
	static float in[48000 * 2 * 2], out[48000 * 2 * 2], removed[48000 * 2 * 2];
	const double tones[] = { 40.0, 1000.0 };
	FiltChainParams p;
	int t, n;

	filtChainDesign(24, 0.0, 0.0, 150.0, 48000.0, &p);
	for (t = 0; t < 2; t++)
	{
		double db;

		for (n = 0; n < frames; n++)
		{
			float s = (float)(0.5 * sin(2.0 * kPi * tones[t] * (double)n / 48000.0));

			in[n * 2] = s;
			in[n * 2 + 1] = -s;
		}
		runChain(p, 2, 0, in, out, removed, frames);
		db = powerDb(rmsFrom(out, 2, frames, 48000), rmsFrom(in, 2, frames, 48000));
		if (t == 0)
		{
			CHECK(db <= -20.0, "40 Hz side: %.2f dB", db);
		}
		else
		{
			CHECK(fabs(db) < 0.5, "1 kHz side: %.2f dB", db);
		}
	}
}

static void testHighCutResponse()
{
	const int frames = 48000 * 2;
	static float in[48000 * 2], out[48000 * 2], removed[48000 * 2];
	FiltChainParams p;
	double at_cut, db;
	int n;

	filtChainDesign(24, 0.0, 18000.0, 0.0, 48000.0, &p);
	at_cut = filtLowCutCascadeResponseDb(18000.0, 48000.0, p.high_cut, p.num_high_cut);
	CHECK(fabs(at_cut + 3.01) < 0.1, "high cut at corner %.4f dB", at_cut);

	for (n = 0; n < frames; n++)
	{
		in[n] = (float)(0.5 * sin(2.0 * kPi * 1000.0 * (double)n / 48000.0));
	}
	runChain(p, 1, 0, in, out, removed, frames);
	db = powerDb(rmsFrom(out, 1, frames, 48000), rmsFrom(in, 1, frames, 48000));
	CHECK(fabs(db) < 0.1, "1 kHz through high cut %.4f dB", db);
}

static void testHighCutClampAt44k()
{
	const int frames = 44100 * 2;
	static float in[44100 * 2], out[44100 * 2], removed[44100 * 2];
	FiltChainParams p;
	double db, at_20k, tail = 0.0;
	bool finite = true;
	int n;

	filtChainDesign(24, 0.0, 20000.0, 0.0, 44100.0, &p);
	CHECK(p.num_high_cut > 0, "20 kHz at 44.1 kHz designs sections");

	in[0] = 1.0f;
	runChain(p, 1, 0, in, out, removed, 44100);
	for (n = 0; n < 44100; n++)
	{
		if (!(fabs((double)out[n]) < 1e10))
		{
			finite = false;
		}
		if (n >= 4410)
		{
			tail = fmax(tail, fabs((double)out[n]));
		}
	}
	CHECK(finite, "impulse response finite");
	CHECK(tail < 1e-6, "impulse tail %g", tail);

	for (n = 0; n < frames; n++)
	{
		in[n] = (float)(0.5 * sin(2.0 * kPi * 1000.0 * (double)n / 44100.0));
	}
	runChain(p, 1, 0, in, out, removed, frames);
	db = powerDb(rmsFrom(out, 1, frames, 44100), rmsFrom(in, 1, frames, 44100));
	CHECK(fabs(db) < 0.1, "1 kHz at 44.1k %.4f dB", db);

	at_20k = filtLowCutCascadeResponseDb(20000.0, 44100.0, p.high_cut, p.num_high_cut);
	CHECK(at_20k < -3.0, "20 kHz response %.4f dB", at_20k);
}

// A high cut at or above Nyquist has nothing to remove, so it is not designed (clamping it to 0.45 x rate would cut
// audible content at low device rates: 18 kHz at 32 kHz would become 14.4 kHz). Just below Nyquist it is still clamped.
static void testHighCutAboveNyquistBypassed()
{
	FiltChainParams p;

	filtChainDesign(48, 0.0, 18000.0, 0.0, 32000.0, &p);
	CHECK(p.num_high_cut == 0 && p.num_high_cut_mirror == 0, "18 kHz at 32 kHz: %d sections", p.num_high_cut);
	CHECK((p.audible_mask & FILT_CHAIN_CHANGED_HIGH_CUT) == 0, "18 kHz at 32 kHz audible mask %d", p.audible_mask);

	filtChainDesign(48, 0.0, 16000.0, 0.0, 32000.0, &p);
	CHECK(p.num_high_cut == 0, "16 kHz (Nyquist) at 32 kHz: %d sections", p.num_high_cut);

	filtChainDesign(48, 0.0, 15000.0, 0.0, 32000.0, &p);
	CHECK(p.num_high_cut > 0 && p.num_high_cut_mirror > 0, "15 kHz at 32 kHz designs sections");

	filtChainDesign(48, 0.0, 20000.0, 0.0, 44100.0, &p);
	CHECK(p.num_high_cut > 0, "20 kHz at 44.1 kHz still designs sections");

	filtChainDesign(48, 0.0, 20000.0, 0.0, 22050.0, &p);
	CHECK(p.num_high_cut == 0, "20 kHz at 22.05 kHz: %d sections", p.num_high_cut);
}

// Runs a standalone cascade over one channel of an interleaved buffer.
static void runCascade(const FiltLowCutSection *sections, int num_sections, const float *in, int num_channels, int channel,
	float *out, int num_frames)
{
	FiltLowCutBiquadState state[FILT_LOW_CUT_MAX_SECTIONS] = {};
	int n;

	for (n = 0; n < num_frames; n++)
	{
		out[n] = (float)filtLowCutProcessSample((realtype)in[n * num_channels + channel], sections, num_sections, state);
	}
}

// Each stage's removed signal is its mirror cascade run on the stage's input.
static void testRemovedMatchesMirror()
{
	const int channels = 8;
	const int frames = 4800;
	static float in[4800 * 8], out[4800 * 8], removed[4800 * 8], expected[4800], side[4800];
	FiltChainParams p;
	double max_err, max_removed;
	int n, c;

	for (n = 0; n < frames * channels; n++)
	{
		in[n] = (float)randomSample();
	}

	// Low cut and high cut alone: the stage input is the channel input.
	{
		const int stages[] = { FILT_CHAIN_CHANGED_LOW_CUT, FILT_CHAIN_CHANGED_HIGH_CUT };
		int s;

		for (s = 0; s < 2; s++)
		{
			const bool low = stages[s] == FILT_CHAIN_CHANGED_LOW_CUT;

			filtChainDesign(48, low ? 30.0 : 0.0, low ? 0.0 : 16000.0, 0.0, 48000.0, &p);
			runChain(p, channels, stages[s], in, out, removed, frames);
			max_err = 0.0;
			max_removed = 0.0;
			for (c = 0; c < channels; c++)
			{
				runCascade(low ? p.low_cut_mirror : p.high_cut_mirror, low ? p.num_low_cut_mirror : p.num_high_cut_mirror, in,
					channels, c, expected, frames);
				for (n = 0; n < frames; n++)
				{
					max_err = fmax(max_err, fabs((double)removed[n * channels + c] - (double)expected[n]));
					max_removed = fmax(max_removed, fabs((double)removed[n * channels + c]));
				}
			}
			CHECK(max_err < 1e-6, "stage %d: removed differs from its mirror cascade by %g", stages[s], max_err);
			CHECK(max_removed > 1e-3, "stage %d removed something (%g)", stages[s], max_removed);
		}
	}

	// High cut after an audible low cut: its stage input is the low cut output.
	filtChainDesign(24, 100.0, 14000.0, 0.0, 48000.0, &p);
	runChain(p, channels, FILT_CHAIN_CHANGED_HIGH_CUT, in, out, removed, frames);
	{
		static float after_low[4800];

		max_err = 0.0;
		for (c = 0; c < channels; c++)
		{
			runCascade(p.low_cut, p.num_low_cut, in, channels, c, after_low, frames);
			runCascade(p.high_cut_mirror, p.num_high_cut_mirror, after_low, 1, 0, expected, frames);
			for (n = 0; n < frames; n++)
			{
				max_err = fmax(max_err, fabs((double)removed[n * channels + c] - (double)expected[n]));
			}
		}
		CHECK(max_err < 1e-6, "high cut after low cut: removed differs from its mirror on the stage input by %g", max_err);
	}

	// Stereo to mono: +side low-pass on channel 0, -side low-pass on channel 1, nothing on the others.
	filtChainDesign(24, 0.0, 0.0, 150.0, 48000.0, &p);
	runChain(p, channels, FILT_CHAIN_CHANGED_MONO, in, out, removed, frames);
	for (n = 0; n < frames; n++)
	{
		side[n] = (float)(0.5 * ((double)in[n * channels] - (double)in[n * channels + 1]));
	}
	runCascade(p.mono_side_mirror, p.num_mono_side_mirror, side, 1, 0, expected, frames);
	max_err = 0.0;
	max_removed = 0.0;
	for (n = 0; n < frames; n++)
	{
		max_err = fmax(max_err, fabs((double)removed[n * channels] - (double)expected[n]));
		max_err = fmax(max_err, fabs((double)removed[n * channels + 1] + (double)expected[n]));
		max_removed = fmax(max_removed, fabs((double)removed[n * channels]));
		for (c = 2; c < channels; c++)
		{
			max_err = fmax(max_err, fabs((double)removed[n * channels + c]));
		}
	}
	CHECK(max_err < 1e-6, "mono: removed differs from +/- side low-pass by %g", max_err);
	CHECK(max_removed > 1e-3, "mono removed something (%g)", max_removed);

	// An empty removed mask plays nothing.
	filtChainDesign(48, 20.0, 16000.0, 150.0, 48000.0, &p);
	runChain(p, channels, 0, in, out, removed, frames);
	max_removed = 0.0;
	for (n = 0; n < frames * channels; n++)
	{
		max_removed = fmax(max_removed, fabs((double)removed[n]));
	}
	CHECK(max_removed == 0.0, "empty mask removed is zero (%g)", max_removed);
}

// The mirrors fall at the full slope away from the cutoff (in - HP(in) would only fall at 6 dB/oct).
static void testMirrorResponse()
{
	FiltChainParams p;
	double db;

	filtChainDesign(48, 30.0, 0.0, 0.0, 48000.0, &p);
	db = filtLowCutCascadeResponseDb(300.0, 48000.0, p.low_cut_mirror, p.num_low_cut_mirror);
	CHECK(db < -100.0, "low cut 30 Hz / 48 dB removed at 300 Hz: %.2f dB", db);
	db = filtLowCutCascadeResponseDb(15.0, 48000.0, p.low_cut_mirror, p.num_low_cut_mirror);
	CHECK(fabs(db) < 1.0, "low cut 30 Hz / 48 dB removed at 15 Hz: %.2f dB", db);

	filtChainDesign(24, 30.0, 0.0, 0.0, 48000.0, &p);
	db = filtLowCutCascadeResponseDb(120.0, 48000.0, p.low_cut_mirror, p.num_low_cut_mirror);
	CHECK(db < -40.0, "low cut 30 Hz / 24 dB removed at 120 Hz: %.2f dB", db);

	filtChainDesign(24, 0.0, 16000.0, 0.0, 48000.0, &p);
	db = filtLowCutCascadeResponseDb(4000.0, 48000.0, p.high_cut_mirror, p.num_high_cut_mirror);
	CHECK(db < -40.0, "high cut 16 kHz / 24 dB removed at 4 kHz: %.2f dB", db);

	filtChainDesign(24, 0.0, 0.0, 150.0, 48000.0, &p);
	db = filtLowCutCascadeResponseDb(150.0, 48000.0, p.mono_side_mirror, p.num_mono_side_mirror);
	CHECK(fabs(db + 6.02) < 0.05, "mono removed at its corner: %.4f dB", db);
}

// A stage that is designed but not audible runs (so it keeps its history and can report what it would remove) but
// passes its input through exactly. Its removed signal equals that of an otherwise identical audible run.
static void testInaudibleStagePassesThrough()
{
	const int channels = 2;
	const int frames = 4800;
	static float in[4800 * 2], out[4800 * 2], removed[4800 * 2], audible_out[4800 * 2], audible_removed[4800 * 2];
	const int stages[] = { FILT_CHAIN_CHANGED_MONO, FILT_CHAIN_CHANGED_LOW_CUT, FILT_CHAIN_CHANGED_HIGH_CUT };
	FiltChainParams audible, inaudible;
	int s, n;

	for (n = 0; n < frames * channels; n++)
	{
		in[n] = (float)randomSample();
	}

	for (s = 0; s < 3; s++)
	{
		const int stage = stages[s];
		double max_pass = 0.0, max_removed_diff = 0.0, max_removed = 0.0;

		filtChainDesign(48, (stage == FILT_CHAIN_CHANGED_LOW_CUT) ? 80.0 : 0.0,
			(stage == FILT_CHAIN_CHANGED_HIGH_CUT) ? 14000.0 : 0.0, (stage == FILT_CHAIN_CHANGED_MONO) ? 150.0 : 0.0,
			48000.0, &audible);
		CHECK(audible.audible_mask == stage, "stage %d designed audible (%d)", stage, audible.audible_mask);
		inaudible = audible;
		inaudible.audible_mask = 0;
		CHECK(filtChainChangedStages(&audible, &inaudible) == 0, "stage %d: audible mask is not part of the layout", stage);

		runChain(inaudible, channels, stage, in, out, removed, frames);
		runChain(audible, channels, stage, in, audible_out, audible_removed, frames);
		for (n = 0; n < frames * channels; n++)
		{
			max_pass = fmax(max_pass, fabs((double)out[n] - (double)in[n]));
			max_removed_diff = fmax(max_removed_diff, fabs((double)removed[n] - (double)audible_removed[n]));
			max_removed = fmax(max_removed, fabs((double)removed[n]));
		}
		CHECK(max_pass == 0.0, "stage %d inaudible: out differs from in by %g", stage, max_pass);
		CHECK(max_removed_diff < 1e-6, "stage %d inaudible: removed differs from audible run by %g", stage, max_removed_diff);
		CHECK(max_removed > 1e-3, "stage %d inaudible still reports removed (%g)", stage, max_removed);
	}
}

// The removed signal of a mask is the sum of each masked stage's removed signal, audible or not.
static void testRemovedUnion()
{
	const int channels = 2;
	const int frames = 4800;
	static float in[4800 * 2], out[4800 * 2], low[4800 * 2], high[4800 * 2], both[4800 * 2];
	const int audible_masks[] = { FILT_CHAIN_STAGES_ALL, 0, FILT_CHAIN_CHANGED_MONO };
	FiltChainParams p;
	int m, n;

	for (n = 0; n < frames * channels; n++)
	{
		in[n] = (float)randomSample();
	}

	for (m = 0; m < 3; m++)
	{
		double max_err = 0.0, max_both = 0.0;

		filtChainDesign(24, 100.0, 14000.0, 150.0, 48000.0, &p);
		p.audible_mask = audible_masks[m];
		runChain(p, channels, FILT_CHAIN_CHANGED_LOW_CUT, in, out, low, frames);
		runChain(p, channels, FILT_CHAIN_CHANGED_HIGH_CUT, in, out, high, frames);
		runChain(p, channels, FILT_CHAIN_CHANGED_LOW_CUT | FILT_CHAIN_CHANGED_HIGH_CUT, in, out, both, frames);
		for (n = 0; n < frames * channels; n++)
		{
			max_err = fmax(max_err, fabs((double)both[n] - (double)low[n] - (double)high[n]));
			max_both = fmax(max_both, fabs((double)both[n]));
		}
		CHECK(max_err < 1e-6, "audible %d: removed(low|high) - removed(low) - removed(high) = %g", audible_masks[m], max_err);
		CHECK(max_both > 1e-3, "audible %d: union removed something (%g)", audible_masks[m], max_both);
	}
}

// filtChainProcessFrameDual's second removed output (another mask, same frame, same filter state) equals a separate
// run's removed for that mask, and neither the first removed output nor the filtered output changes.
static void testSecondRemovedOutput()
{
	const int channels = 2;
	const int frames = 4800;
	static float in[4800 * 2], out[4800 * 2], ref_out[4800 * 2], ref_removed[4800 * 2],
		ref_removed_b[4800 * 2], dual_out[4800 * 2], dual_removed[4800 * 2], dual_removed_b[4800 * 2];
	const int mask_pairs[][2] = {
		{ FILT_CHAIN_CHANGED_LOW_CUT, FILT_CHAIN_CHANGED_HIGH_CUT },
		{ FILT_CHAIN_CHANGED_HIGH_CUT, FILT_CHAIN_CHANGED_LOW_CUT | FILT_CHAIN_CHANGED_HIGH_CUT },
		{ FILT_CHAIN_CHANGED_LOW_CUT | FILT_CHAIN_CHANGED_HIGH_CUT, FILT_CHAIN_STAGES_ALL },
		{ FILT_CHAIN_CHANGED_MONO, FILT_CHAIN_CHANGED_LOW_CUT },
		{ FILT_CHAIN_CHANGED_LOW_CUT, 0 }
	};
	const int audible_masks[] = { FILT_CHAIN_STAGES_ALL, FILT_CHAIN_CHANGED_LOW_CUT };
	FiltChainParams p;
	int pair, a, n;

	for (n = 0; n < frames * channels; n++)
	{
		in[n] = (float)randomSample();
	}

	for (a = 0; a < 2; a++)
	{
		for (pair = 0; pair < (int)(sizeof(mask_pairs) / sizeof(mask_pairs[0])); pair++)
		{
			const int mask_a = mask_pairs[pair][0];
			const int mask_b = mask_pairs[pair][1];
			FiltChainState state;
			double max_b = 0.0, max_a = 0.0, max_out = 0.0, max_b_seen = 0.0;

			filtChainDesign(24, 100.0, 14000.0, 150.0, 48000.0, &p);
			p.audible_mask = audible_masks[a];
			runChain(p, channels, mask_a, in, ref_out, ref_removed, frames);
			runChain(p, channels, mask_b, in, out, ref_removed_b, frames);
			filtChainResetState(&state);
			for (n = 0; n < frames; n++)
			{
				filtChainProcessFrameDual(in + n * channels, dual_out + n * channels, dual_removed + n * channels, mask_a,
					dual_removed_b + n * channels, mask_b, channels, &p, &state);
			}
			for (n = 0; n < frames * channels; n++)
			{
				max_b = fmax(max_b, fabs((double)dual_removed_b[n] - (double)ref_removed_b[n]));
				max_b_seen = fmax(max_b_seen, fabs((double)dual_removed_b[n]));
				max_a = fmax(max_a, fabs((double)dual_removed[n] - (double)ref_removed[n]));
				max_out = fmax(max_out, fabs((double)dual_out[n] - (double)ref_out[n]));
			}
			CHECK(max_b < 1e-6, "audible %d masks %d/%d: second removed differs from separate run by %g", audible_masks[a],
				mask_a, mask_b, max_b);
			CHECK(mask_b == 0 || max_b_seen > 1e-3, "masks %d/%d: second removed is not silent (%g)", mask_a, mask_b,
				max_b_seen);
			CHECK(max_a == 0.0, "audible %d masks %d/%d: first removed changed by %g", audible_masks[a], mask_a, mask_b, max_a);
			CHECK(max_out == 0.0, "audible %d masks %d/%d: filtered output changed by %g", audible_masks[a], mask_a, mask_b,
				max_out);
		}

		// A null second output leaves the first removed output and the filtered output exactly as a run that does
		// compute the second one (both against the Dual path with a non-null second output).
		{
			FiltChainState state;
			double max_a = 0.0;

			filtChainDesign(24, 100.0, 14000.0, 150.0, 48000.0, &p);
			p.audible_mask = audible_masks[a];
			filtChainResetState(&state);
			for (n = 0; n < frames; n++)
			{
				filtChainProcessFrameDual(in + n * channels, ref_out + n * channels, ref_removed + n * channels,
					FILT_CHAIN_CHANGED_LOW_CUT, ref_removed_b + n * channels, FILT_CHAIN_CHANGED_HIGH_CUT, channels, &p, &state);
			}
			filtChainResetState(&state);
			for (n = 0; n < frames; n++)
			{
				filtChainProcessFrameDual(in + n * channels, dual_out + n * channels, dual_removed + n * channels,
					FILT_CHAIN_CHANGED_LOW_CUT, NULL, FILT_CHAIN_CHANGED_HIGH_CUT, channels, &p, &state);
			}
			for (n = 0; n < frames * channels; n++)
			{
				max_a = fmax(max_a, fabs((double)dual_removed[n] - (double)ref_removed[n]) +
					fabs((double)dual_out[n] - (double)ref_out[n]));
			}
			CHECK(max_a == 0.0, "audible %d: null second output changed the first by %g", audible_masks[a], max_a);
		}
	}
}

static void testMonoChannelCounts()
{
	const int frames = 4800;
	static float in[4800 * 6], out_a[4800 * 6], out_b[4800 * 6], removed[4800 * 6];
	FiltChainParams with_mono, without_mono;
	double max_diff = 0.0;
	int n, c;

	filtChainDesign(24, 20.0, 16000.0, 150.0, 48000.0, &with_mono);
	filtChainDesign(24, 20.0, 16000.0, 0.0, 48000.0, &without_mono);

	for (n = 0; n < frames * 6; n++)
	{
		in[n] = (float)randomSample();
	}

	runChain(with_mono, 1, 0, in, out_a, removed, frames);
	runChain(without_mono, 1, 0, in, out_b, removed, frames);
	for (n = 0; n < frames; n++)
	{
		max_diff = fmax(max_diff, fabs((double)out_a[n] - (double)out_b[n]));
	}
	CHECK(max_diff == 0.0, "1 ch: mono skipped, diff %g", max_diff);

	runChain(with_mono, 6, 0, in, out_a, removed, frames);
	runChain(without_mono, 6, 0, in, out_b, removed, frames);
	max_diff = 0.0;
	for (n = 0; n < frames; n++)
	{
		for (c = 2; c < 6; c++)
		{
			max_diff = fmax(max_diff, fabs((double)out_a[n * 6 + c] - (double)out_b[n * 6 + c]));
		}
	}
	CHECK(max_diff == 0.0, "6 ch: channels 2-5 unaffected, diff %g", max_diff);
}

static bool biquadIsZero(const FiltLowCutBiquadState& b)
{
	return b.in_minus1 == 0.0 && b.in_minus2 == 0.0 && b.out_minus1 == 0.0 && b.out_minus2 == 0.0;
}

static bool biquadEqual(const FiltLowCutBiquadState& a, const FiltLowCutBiquadState& b)
{
	return a.in_minus1 == b.in_minus1 && a.in_minus2 == b.in_minus2 && a.out_minus1 == b.out_minus1 &&
		a.out_minus2 == b.out_minus2;
}

// Toggling one stage must keep the history of the stages whose layout did not change.
static void testCarryStateKeepsUnchangedStages()
{
	const double fs = 48000.0;
	const int warm_frames = 48000;
	const int check_frames = 9600;
	const int settle_frames = 64;
	FiltChainParams low_only, low_high, low_high_mono, gentler_high;
	FiltChainState old_state, new_state, reference_state;
	double max_diff = 0.0;
	int n, c, i, changed;

	filtChainDesign(48, 40.0, 0.0, 0.0, fs, &low_only);
	filtChainDesign(48, 40.0, 16000.0, 0.0, fs, &low_high);
	filtChainDesign(48, 40.0, 16000.0, 150.0, fs, &low_high_mono);
	filtChainDesign(24, 40.0, 16000.0, 150.0, fs, &gentler_high);

	CHECK(filtChainChangedStages(&low_only, &low_only) == 0, "same params: nothing changed");
	CHECK(filtChainChangedStages(&low_only, &low_high) == FILT_CHAIN_CHANGED_HIGH_CUT, "adding high cut changes only it");
	CHECK(filtChainChangedStages(&low_high, &low_high_mono) == FILT_CHAIN_CHANGED_MONO, "adding mono changes only it");
	CHECK(filtChainChangedStages(&low_high_mono, &gentler_high) ==
		(FILT_CHAIN_CHANGED_LOW_CUT | FILT_CHAIN_CHANGED_HIGH_CUT), "slope changes low and high cut, not mono");
	CHECK(filtChainSameLayout(&low_only, &low_only) && !filtChainSameLayout(&low_only, &low_high), "same layout wrapper");

	// Warm the low-cut-only chain on a 50 Hz sine, stereo. The reference runs low + high cut from the start: the
	// low cut sits before the high cut, so its history only depends on the input and must match after the carry.
	filtChainResetState(&old_state);
	filtChainResetState(&reference_state);
	for (n = 0; n < warm_frames; n++)
	{
		float frame[2], out[2], removed[2];

		frame[0] = (float)(0.5 * sin(2.0 * kPi * 50.0 * n / fs));
		frame[1] = frame[0];
		filtChainProcessFrame(frame, out, removed, 2, 0, &low_only, &old_state);
		filtChainProcessFrame(frame, out, removed, 2, 0, &low_high, &reference_state);
	}

	// Poison the stages the new set adds so the reset is observable.
	for (c = 0; c < FILT_CHAIN_MAX_CHANNELS; c++)
	{
		for (i = 0; i < FILT_LOW_CUT_MAX_SECTIONS; i++)
		{
			old_state.channels[c].high_cut[i].out_minus1 = 123.0;
			old_state.channels[c].high_cut_mirror[i].out_minus1 = 124.0;
		}
	}
	old_state.mono_side[0].in_minus1 = 456.0;
	for (c = 0; c < FILT_CHAIN_MAX_CHANNELS; c++)
	{
		for (i = 0; i < FILT_LOW_CUT_MAX_SECTIONS; i++)
		{
			new_state.channels[c].low_cut[i].out_minus1 = -7.0;
			new_state.channels[c].low_cut_mirror[i].out_minus1 = -7.0;
			new_state.channels[c].high_cut[i].out_minus1 = -7.0;
			new_state.channels[c].high_cut_mirror[i].out_minus1 = -7.0;
		}
	}

	changed = filtChainCarryState(&low_only, &low_high, &old_state, &new_state);
	CHECK(changed == FILT_CHAIN_CHANGED_HIGH_CUT, "carry reports high cut changed (%d)", changed);
	{
		bool low_kept = true, high_reset = true;

		for (c = 0; c < FILT_CHAIN_MAX_CHANNELS; c++)
		{
			for (i = 0; i < FILT_LOW_CUT_MAX_SECTIONS; i++)
			{
				low_kept = low_kept && biquadEqual(new_state.channels[c].low_cut[i], old_state.channels[c].low_cut[i]) &&
					biquadEqual(new_state.channels[c].low_cut_mirror[i], old_state.channels[c].low_cut_mirror[i]);
				high_reset = high_reset && biquadIsZero(new_state.channels[c].high_cut[i]) &&
					biquadIsZero(new_state.channels[c].high_cut_mirror[i]);
			}
		}
		CHECK(low_kept, "low cut history carried over");
		CHECK(high_reset, "high cut history reset");
		CHECK(biquadEqual(new_state.mono_side[0], old_state.mono_side[0]), "unchanged (off) mono stage carried");
	}

	// Continuing with the new set must track the steady low + high reference: no restart thump from the low cut.
	for (n = 0; n < check_frames; n++)
	{
		const int t = warm_frames + n;
		float frame[2], out[2], ref[2], removed[2];

		frame[0] = (float)(0.5 * sin(2.0 * kPi * 50.0 * t / fs));
		frame[1] = frame[0];
		filtChainProcessFrame(frame, out, removed, 2, 0, &low_high, &new_state);
		filtChainProcessFrame(frame, ref, removed, 2, 0, &low_high, &reference_state);
		if (n >= settle_frames)
		{
			max_diff = fmax(max_diff, fabs((double)out[0] - (double)ref[0]));
		}
	}
	CHECK(max_diff < 1e-5, "carried low cut tracks reference after adding high cut (diff %g)", max_diff);

	// Changing the slope resets the low and high cut but keeps the mono side; aliasing old and new is allowed.
	new_state.mono_side[1].out_minus2 = 0.25;
	new_state.mono_side_mirror[0].in_minus1 = -0.5;
	{
		FiltLowCutBiquadState mono_before = new_state.mono_side[1];
		FiltLowCutBiquadState mono_mirror_before = new_state.mono_side_mirror[0];

		changed = filtChainCarryState(&low_high_mono, &gentler_high, &new_state, &new_state);
		CHECK(changed == (FILT_CHAIN_CHANGED_LOW_CUT | FILT_CHAIN_CHANGED_HIGH_CUT), "slope change bits (%d)", changed);
		CHECK(biquadEqual(new_state.mono_side[1], mono_before), "mono history kept on slope change");
		CHECK(biquadEqual(new_state.mono_side_mirror[0], mono_mirror_before), "mono mirror history kept on slope change");
		CHECK(biquadIsZero(new_state.channels[0].low_cut[0]) && biquadIsZero(new_state.channels[1].high_cut[0]) &&
			biquadIsZero(new_state.channels[0].low_cut_mirror[0]) && biquadIsZero(new_state.channels[1].high_cut_mirror[0]),
			"low and high cut reset on slope change");
	}
}

int main()
{
	srand(12345);
	testDesignOffStages();
	testMonoLinkwitzRiley();
	testMonoKeepsMidExact();
	testMonoFoldsSideBass();
	testHighCutResponse();
	testHighCutClampAt44k();
	testHighCutAboveNyquistBypassed();
	testRemovedMatchesMirror();
	testMirrorResponse();
	testInaudibleStagePassesThrough();
	testRemovedUnion();
	testSecondRemovedOutput();
	testMonoChannelCounts();
	testCarryStateKeepsUnchangedStages();

	printf("%d checks, %d failures\n", g_checks, g_failures);
	return (g_failures == 0) ? 0 : 1;
}
