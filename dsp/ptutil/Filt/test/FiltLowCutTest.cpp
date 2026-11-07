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

#include "codedefs.h"
#include "FiltLowCut.h"

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

static const int kSlopes[] = { 6, 12, 18, 24, 36, 48 };
static const double kPi = 3.14159265358979323846;
static const double kTargetDb = -3.0103;

#define COUNT_OF(a) ((int)(sizeof(a) / sizeof((a)[0])))

static void testSnapAndClamp()
{
	CHECK(filtLowCutSnapSlopeDb(6) == 6, "6 -> %d", filtLowCutSnapSlopeDb(6));
	CHECK(filtLowCutSnapSlopeDb(18) == 18, "18 -> %d", filtLowCutSnapSlopeDb(18));
	CHECK(filtLowCutSnapSlopeDb(36) == 36, "36 -> %d", filtLowCutSnapSlopeDb(36));
	CHECK(filtLowCutSnapSlopeDb(48) == 48, "48 -> %d", filtLowCutSnapSlopeDb(48));
	CHECK(filtLowCutSnapSlopeDb(7) == 12, "7 -> %d", filtLowCutSnapSlopeDb(7));
	CHECK(filtLowCutSnapSlopeDb(30) == 36, "30 -> %d", filtLowCutSnapSlopeDb(30));
	CHECK(filtLowCutSnapSlopeDb(40) == 48, "40 -> %d", filtLowCutSnapSlopeDb(40));
	CHECK(filtLowCutSnapSlopeDb(96) == 48, "96 -> %d", filtLowCutSnapSlopeDb(96));
	CHECK(filtLowCutSnapSlopeDb(0) == 6, "0 -> %d", filtLowCutSnapSlopeDb(0));
	CHECK(filtLowCutSnapSlopeDb(-5) == 6, "-5 -> %d", filtLowCutSnapSlopeDb(-5));

	CHECK(filtLowCutClampQ(0.1) == FILT_LOW_CUT_Q_MIN, "Q 0.1 -> %f", filtLowCutClampQ(0.1));
	CHECK(filtLowCutClampQ(5.0) == FILT_LOW_CUT_Q_MAX, "Q 5.0 -> %f", filtLowCutClampQ(5.0));
	CHECK(filtLowCutClampQ(0.71) == 0.71, "Q 0.71 kept");
	CHECK(filtLowCutClampQ(sqrt(-1.0)) == FILT_LOW_CUT_Q_NEUTRAL, "Q NaN -> neutral");
}

static void testPrototypes()
{
	FiltLowCutPrototypeSection proto[FILT_LOW_CUT_MAX_SECTIONS];
	int n;

	n = filtLowCutPrototype(4, FILT_LOW_CUT_Q_NEUTRAL, proto);
	CHECK(n == 2 && fabs(proto[0].q - 1.30656) < 1e-4 && fabs(proto[1].q - 0.54120) < 1e-4,
		"BW4 Qs %f %f", proto[0].q, proto[1].q);

	n = filtLowCutPrototype(2, 2.0, proto);
	CHECK(n == 1 && fabs(proto[0].q - 0.70710678 * 2.0 / 0.71) < 1e-6, "BW2 Q at 2.0: %f", proto[0].q);

	n = filtLowCutPrototype(6, 2.0, proto);
	CHECK(n == 3 && fabs(proto[0].q - 1.93185 * 2.0 / 0.71) < 1e-4 && fabs(proto[1].q - 0.70711) < 1e-4 &&
		fabs(proto[2].q - 0.51764) < 1e-4, "BW6 Qs at 2.0: %f %f %f", proto[0].q, proto[1].q, proto[2].q);

	n = filtLowCutPrototype(1, 2.0, proto);
	CHECK(n == 1 && proto[0].q == 0.0, "BW1 is first-order");

	CHECK(filtLowCutPrototype(5, 0.71, proto) == 0, "order 5 unsupported");
	CHECK(filtLowCutPrototype(16, 0.71, proto) == 0, "order 16 unsupported");
}

static void testSectionLayout()
{
	struct LayoutCase {
		int slope_db;
		int count;
		int orders[FILT_LOW_CUT_MAX_SECTIONS];
	};
	const LayoutCase cases[] = {
		{ 6, 1, { 1 } }, { 12, 1, { 2 } }, { 18, 2, { 2, 1 } }, { 24, 2, { 2, 2 } }, { 36, 3, { 2, 2, 2 } }, { 48, 4, { 2, 2, 2, 2 } }
	};
	FiltLowCutSection sections[FILT_LOW_CUT_MAX_SECTIONS];
	int c, hp, i;

	for (c = 0; c < COUNT_OF(cases); c++)
	{
		for (hp = 0; hp <= 1; hp++)
		{
			int n = filtLowCutDesign(cases[c].slope_db, FILT_LOW_CUT_Q_NEUTRAL, 20.0, 48000.0, hp, sections);

			CHECK(n == cases[c].count, "slope %d hp %d: %d sections, expected %d", cases[c].slope_db, hp, n, cases[c].count);
			for (i = 0; i < n && i < cases[c].count; i++)
			{
				CHECK(sections[i].order == cases[c].orders[i] && sections[i].high_pass == hp,
					"slope %d hp %d section %d: order %d high_pass %d", cases[c].slope_db, hp, i,
					sections[i].order, sections[i].high_pass);
			}
		}
	}

	CHECK(filtLowCutDesign(24, 2.0, 0.0, 48000.0, 1, sections) == 0, "0Hz high-pass");
	CHECK(filtLowCutDesign(24, 2.0, 0.0, 48000.0, 0, sections) == 0, "0Hz low-pass");

	CHECK(filtLowCutDesign(30, 0.71, 20.0, 48000.0, 1, sections) == 3, "30 designs as 36");
}

static void testCutoffAndPassband()
{
	const double sample_rates[] = { 22050.0, 44100.0, 48000.0, 96000.0, 192000.0 };
	const double cutoffs[] = { 5.0, 20.0, 200.0 };
	const double qs[] = { 0.5, 0.71, 1.0, 2.0 };
	FiltLowCutSection sections[FILT_LOW_CUT_MAX_SECTIONS];
	int r, c, s, q, hp;

	for (r = 0; r < COUNT_OF(sample_rates); r++)
	{
		for (c = 0; c < COUNT_OF(cutoffs); c++)
		{
			for (s = 0; s < COUNT_OF(kSlopes); s++)
			{
				double fs = sample_rates[r];
				double fc = cutoffs[c];

				for (q = 0; q < COUNT_OF(qs); q++)
				{
					for (hp = 0; hp <= 1; hp++)
					{
						int n = filtLowCutDesign(kSlopes[s], qs[q], fc, fs, hp, sections);
						double at_cutoff = filtLowCutCascadeResponseDb(fc, fs, sections, n);

						CHECK(fabs(at_cutoff - kTargetDb) < 0.01, "%d dB/oct Q %.2f hp %d fc %.0f fs %.0f: %.4f dB at cutoff",
							kSlopes[s], qs[q], hp, fc, fs, at_cutoff);

						{
							double below = filtLowCutCascadeResponseDb(fc / 1.5, fs, sections, n);

							CHECK(hp ? (below < kTargetDb) : (below > kTargetDb),
								"%d dB/oct Q %.2f hp %d fc %.0f fs %.0f: %.4f dB at fc/1.5",
								kSlopes[s], qs[q], hp, fc, fs, below);

							if (fc * 1.5 <= 0.45 * fs)
							{
								double above = filtLowCutCascadeResponseDb(fc * 1.5, fs, sections, n);

								CHECK(hp ? (above > kTargetDb) : (above < kTargetDb),
									"%d dB/oct Q %.2f hp %d fc %.0f fs %.0f: %.4f dB at fc*1.5",
									kSlopes[s], qs[q], hp, fc, fs, above);
							}
						}
					}
				}

				{
					double passband_hz = (fc * 20.0 > 1000.0) ? fc * 20.0 : 1000.0;
					int n = filtLowCutDesign(kSlopes[s], FILT_LOW_CUT_Q_NEUTRAL, fc, fs, 1, sections);
					double passband = filtLowCutCascadeResponseDb(passband_hz, fs, sections, n);

					CHECK(fabs(passband) < 0.1, "%d dB/oct fc %.0f fs %.0f: passband %.4f dB at %.0f Hz",
						kSlopes[s], fc, fs, passband, passband_hz);
				}
			}
		}
	}
}

static void testSlope()
{
	const double sample_rates[] = { 44100.0, 48000.0, 96000.0 };
	const double cutoffs[] = { 20.0, 200.0 };
	FiltLowCutSection sections[FILT_LOW_CUT_MAX_SECTIONS];
	int r, c, s;

	for (r = 0; r < COUNT_OF(sample_rates); r++)
	{
		for (c = 0; c < COUNT_OF(cutoffs); c++)
		{
			for (s = 0; s < COUNT_OF(kSlopes); s++)
			{
				double fs = sample_rates[r];
				double fc = cutoffs[c];
				int n = filtLowCutDesign(kSlopes[s], FILT_LOW_CUT_Q_NEUTRAL, fc, fs, 1, sections);
				double octave_db = filtLowCutCascadeResponseDb(fc / 32.0, fs, sections, n) - filtLowCutCascadeResponseDb(fc / 64.0, fs, sections, n);

				CHECK(fabs(octave_db - kSlopes[s]) < 1.0, "%d dB/oct fc %.0f fs %.0f: %.2f dB per octave far below the cutoff",
					kSlopes[s], fc, fs, octave_db);
			}
		}
	}
}

static void testButterworthShape()
{
	FiltLowCutSection sections[FILT_LOW_CUT_MAX_SECTIONS];
	const double fs = 48000.0;
	const double fc = 20.0;
	double peak = -1000.0, lowest = 1000.0;
	int n, i, s;

	n = filtLowCutDesign(12, 2.0, fc, fs, 1, sections);
	for (i = 0; i <= 400; i++)
	{
		double db = filtLowCutCascadeResponseDb(fc * pow(10.0, i / 400.0), fs, sections, n);
		peak = (db > peak) ? db : peak;
	}
	CHECK(fabs(peak - 6.3) < 0.3, "12 dB/oct Q 2.0 peak %.3f dB", peak);

	for (s = 0; s < COUNT_OF(kSlopes); s++)
	{
		double from_hz = (kSlopes[s] <= 12) ? 20.0 * fc : 2.0 * fc;

		peak = -1000.0;
		lowest = 1000.0;
		n = filtLowCutDesign(kSlopes[s], FILT_LOW_CUT_Q_NEUTRAL, fc, fs, 1, sections);
		for (i = 0; i <= 400; i++)
		{
			double db = filtLowCutCascadeResponseDb(from_hz * pow(1000.0 / from_hz, i / 400.0), fs, sections, n);
			peak = (db > peak) ? db : peak;
			lowest = (db < lowest) ? db : lowest;
		}
		CHECK(peak < 0.01 && lowest > -0.1, "%d dB/oct neutral: %.4f..%.4f dB from %.0f Hz to 1kHz", kSlopes[s], lowest, peak, from_hz);
	}

	for (s = 0; s < COUNT_OF(kSlopes); s++)
	{
		peak = -1000.0;
		n = filtLowCutDesign(kSlopes[s], 2.0, fc, fs, 1, sections);
		for (i = 0; i <= 800; i++)
		{
			double db = filtLowCutCascadeResponseDb(fc * pow(100.0, i / 800.0), fs, sections, n);
			peak = (db > peak) ? db : peak;
		}
		CHECK(peak < 6.5, "%d dB/oct Q 2.0 peak %.3f dB", kSlopes[s], peak);
	}
}

static void testTimeDomain()
{
	struct ToneCase {
		int sample_rate;
		int slope_db;
		double cutoff_hz;
		double tone_hz;
	};
	const ToneCase cases[] = {
		{ 48000, 24, 20.0, 30.0 },
		{ 48000, 18, 20.0, 30.0 },
		{ 48000, 36, 20.0, 30.0 },
		{ 44100, 6, 200.0, 100.0 },
		{ 96000, 48, 5.0, 1000.0 }
	};
	int c;

	for (c = 0; c < COUNT_OF(cases); c++)
	{
		const ToneCase& tone = cases[c];
		const int num_frames = 2 * tone.sample_rate;
		const int measure_from = num_frames - tone.sample_rate / 2;
		FiltLowCutSection hp_sections[FILT_LOW_CUT_MAX_SECTIONS];
		FiltLowCutSection lp_sections[FILT_LOW_CUT_MAX_SECTIONS];
		FiltLowCutChannelState state;
		int num_hp = filtLowCutDesign(tone.slope_db, FILT_LOW_CUT_Q_NEUTRAL, tone.cutoff_hz, tone.sample_rate, 1, hp_sections);
		int num_lp = filtLowCutDesign(tone.slope_db, FILT_LOW_CUT_Q_NEUTRAL, tone.cutoff_hz, tone.sample_rate, 0, lp_sections);
		double input_sum = 0.0, filtered_sum = 0.0, removed_sum = 0.0;
		int n;

		filtLowCutResetChannelState(&state);
		for (n = 0; n < num_frames; n++)
		{
			realtype input = (realtype)(0.5 * sin(2.0 * kPi * tone.tone_hz * (double)n / (double)tone.sample_rate));
			realtype filtered = filtLowCutProcessSample(input, hp_sections, num_hp, state.hp_sections);
			realtype removed = filtLowCutProcessSample(input, lp_sections, num_lp, state.lp_sections);

			if (n >= measure_from)
			{
				input_sum += (double)input * (double)input;
				filtered_sum += (double)filtered * (double)filtered;
				removed_sum += (double)removed * (double)removed;
			}
		}

		{
			double filtered_db = 10.0 * log10(filtered_sum / input_sum);
			double removed_db = 10.0 * log10((removed_sum + 1e-30) / input_sum);
			double expected_filtered_db = filtLowCutCascadeResponseDb(tone.tone_hz, tone.sample_rate, hp_sections, num_hp);
			double expected_removed_db = filtLowCutCascadeResponseDb(tone.tone_hz, tone.sample_rate, lp_sections, num_lp);

			CHECK(fabs(filtered_db - expected_filtered_db) < 0.05, "tone case %d: filtered %.4f dB, expected %.4f dB", c, filtered_db, expected_filtered_db);
			if (expected_removed_db > -40.0)
			{
				CHECK(fabs(removed_db - expected_removed_db) < 0.05, "tone case %d: removed %.4f dB, expected %.4f dB", c, removed_db, expected_removed_db);
			}
			else
			{
				CHECK(removed_db < -60.0, "tone case %d: removed %.4f dB, expected below -60 dB", c, removed_db);
			}
		}
	}
}

int main()
{
	testSnapAndClamp();
	testPrototypes();
	testSectionLayout();
	testCutoffAndPassband();
	testSlope();
	testButterworthShape();
	testTimeDomain();

	printf("%d checks, %d failures\n", g_checks, g_failures);
	return (g_failures == 0) ? 0 : 1;
}
