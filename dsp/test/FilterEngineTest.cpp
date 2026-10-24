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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "FilterEngine.h"

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

static void testFilterResponse()
{
	const int sample_rates[] = { 44100, 48000, 96000 };
	const int *slopes = DfxDsp::CUT_SLOPES_DB;
	const size_t num_slopes = (size_t)DfxDsp::CUT_NUM_SLOPES;
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
		CHECK((filtLowCutSnapSlopeDb(requested) == expected), "testFilterResponse");
	}

	for (rate_index = 0; rate_index < sizeof(sample_rates) / sizeof(sample_rates[0]); rate_index++)
	{
		for (slope_index = 0; slope_index < num_slopes; slope_index++)
		{
			const int sample_rate = sample_rates[rate_index];
			const int slope = slopes[slope_index];
			FiltLowCutSection expected[FILT_LOW_CUT_MAX_SECTIONS];
			FilterIirParams params;
			int expected_sections;

			expected_sections = filtLowCutDesign(slope, FILT_LOW_CUT_Q_NEUTRAL, DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ,
				(double)sample_rate, 1, expected);
			filterDesignIirParams(sample_rate, slope, (float)DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ, 0.0f, 0.0f, 7u, &params);
			CHECK((expected_sections > 0), "testFilterResponse");
			CHECK((params.chain.num_low_cut == expected_sections), "testFilterResponse");
			CHECK((params.chain.num_high_cut == 0 && params.chain.num_mono_side == 0), "testFilterResponse");
			CHECK((params.sample_rate == sample_rate && params.reset_serial == 7u), "testFilterResponse");

			CHECK((filtLowCutCascadeResponseDb(1000.0, sample_rate, params.chain.low_cut, params.chain.num_low_cut) > -0.1), "testFilterResponse");
			CHECK((filtLowCutCascadeResponseDb(20000.0, sample_rate, params.chain.low_cut, params.chain.num_low_cut) > -0.1), "testFilterResponse");
			CHECK((fabs(filtLowCutCascadeResponseDb(DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ, sample_rate, params.chain.low_cut,
				params.chain.num_low_cut) - (-3.0103)) < 0.01), "testFilterResponse");
		}
	}
}

static void testFilterIirHandoff()
{
	FilterIirExchange exchange;
	FilterIirParams first, cutoff_moved, gentler;
	const FilterIirParams *adopted;
	bool took_new = true;

	filterIirExchangeInit(&exchange);
	adopted = filterIirExchangeAdopt(&exchange, &took_new);
	CHECK((!took_new && adopted->sample_rate == 0), "testFilterIirHandoff");

	filterDesignIirParams(48000, 48, 20.0f, 0.0f, 0.0f, 1u, &first);
	filterDesignIirParams(48000, 48, 40.0f, 0.0f, 0.0f, 1u, &cutoff_moved);
	filterDesignIirParams(48000, 24, 40.0f, 0.0f, 0.0f, 1u, &gentler);

	filterIirExchangePublish(&exchange, &first);
	adopted = filterIirExchangeAdopt(&exchange, &took_new);
	CHECK((took_new), "testFilterIirHandoff");
	CHECK((adopted->sample_rate == 48000 && adopted->chain.num_low_cut == first.chain.num_low_cut), "testFilterIirHandoff");
	CHECK((adopted->chain.low_cut[0].a1 == first.chain.low_cut[0].a1), "testFilterIirHandoff");

	filterIirExchangePublish(&exchange, &cutoff_moved);
	adopted = filterIirExchangeAdopt(&exchange, NULL);
	CHECK((adopted->chain.low_cut[0].a1 == cutoff_moved.chain.low_cut[0].a1), "testFilterIirHandoff");
	CHECK((filtChainChangedStages(&adopted->chain, &first.chain) == 0), "testFilterIirHandoff");

	CHECK((filterIirExchangeAdopt(&exchange, &took_new) == adopted && !took_new), "testFilterIirHandoff");

	filterIirExchangePublish(&exchange, &first);
	filterIirExchangePublish(&exchange, &gentler);
	adopted = filterIirExchangeAdopt(&exchange, NULL);
	CHECK((adopted->chain.num_low_cut == gentler.chain.num_low_cut && adopted->chain.low_cut[0].a1 == gentler.chain.low_cut[0].a1), "testFilterIirHandoff");
	CHECK((filtChainChangedStages(&adopted->chain, &cutoff_moved.chain) == FILT_CHAIN_CHANGED_LOW_CUT), "testFilterIirHandoff");
	CHECK((filtChainChangedStages(&adopted->chain, &gentler.chain) == 0), "testFilterIirHandoff");

	FilterIirParams bypassed;
	filterDesignIirParams(48000, 48, 0.0f, 0.0f, 0.0f, 1u, &bypassed);
	CHECK((bypassed.chain.num_low_cut == 0 && bypassed.chain.num_high_cut == 0 && bypassed.chain.num_mono_side == 0), "testFilterIirHandoff");
	CHECK((bypassed.sample_rate == 48000), "testFilterIirHandoff");
	CHECK((filtChainChangedStages(&first.chain, &bypassed.chain) == FILT_CHAIN_CHANGED_LOW_CUT), "testFilterIirHandoff");

	FilterIirParams eighteen;
	filterDesignIirParams(48000, 18, 40.0f, 0.0f, 0.0f, 1u, &eighteen);
	CHECK((eighteen.chain.num_low_cut == gentler.chain.num_low_cut), "testFilterIirHandoff");
	CHECK((filtChainChangedStages(&gentler.chain, &eighteen.chain) == FILT_CHAIN_CHANGED_LOW_CUT), "testFilterIirHandoff");

	// Adding a high cut or mono stage changes only that stage's layout; moving its frequency changes nothing.
	FilterIirParams with_high, high_moved, with_mono;
	filterDesignIirParams(48000, 24, 40.0f, 16000.0f, 0.0f, 1u, &with_high);
	filterDesignIirParams(48000, 24, 40.0f, 12000.0f, 0.0f, 1u, &high_moved);
	filterDesignIirParams(48000, 24, 40.0f, 16000.0f, 150.0f, 1u, &with_mono);
	CHECK((with_high.chain.num_high_cut > 0 && with_mono.chain.num_mono_side == FILT_CHAIN_MONO_SECTIONS), "testFilterIirHandoff");
	CHECK((filtChainChangedStages(&with_high.chain, &gentler.chain) == FILT_CHAIN_CHANGED_HIGH_CUT), "testFilterIirHandoff");
	CHECK((with_high.chain.high_cut[0].a1 != high_moved.chain.high_cut[0].a1), "testFilterIirHandoff");
	CHECK((filtChainChangedStages(&high_moved.chain, &with_high.chain) == 0), "testFilterIirHandoff");
	CHECK((filtChainChangedStages(&with_mono.chain, &with_high.chain) == FILT_CHAIN_CHANGED_MONO), "testFilterIirHandoff");
}

static void testFilterPreviewLeveller()
{
	const int sample_rate = 48000;
	const int num_frames = 3 * sample_rate;
	const int measure_from = num_frames - sample_rate / 2;
	FilterPreviewLevelCoeffs coeffs = filterPreviewLevelCoeffs(sample_rate);
	const float amplitudes[] = { 0.01f, 3.0f, 0.0f };
	size_t case_index;

	for (case_index = 0; case_index < sizeof(amplitudes) / sizeof(amplitudes[0]); case_index++)
	{
		float amplitude = amplitudes[case_index];
		FilterPreviewLeveller leveller = { 0.0f, 1.0f, 0.0f, 0 };
		double measured_sum = 0.0;
		float max_magnitude = 0.0f;
		int n;

		for (n = 0; n < num_frames; n++)
		{
			const double two_pi = 6.283185307179586;
			float value = amplitude * (float)sin(two_pi * 30.0 * (double)n / (double)sample_rate);
			float frame[2] = { value, value };

			filterPreviewLevelFrame(frame, 2, &coeffs, &leveller, true);
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

		CHECK((max_magnitude <= 1.0f), "testFilterPreviewLeveller");
		if (amplitude == 0.0f)
		{
			CHECK((max_magnitude == 0.0f), "testFilterPreviewLeveller");
		}
		else if (amplitude < 0.1f)
		{
			CHECK((measured_rms > 0.07 && measured_rms < 0.14), "testFilterPreviewLeveller");
		}
	}

	{
		const int silence_frames = 2 * sample_rate;
		const int tone_frames = sample_rate / 10;
		const float amplitude = 0.3f;
		FilterPreviewLeveller leveller = { 0.0f, 1.0f, 0.0f, 0 };
		float max_magnitude = 0.0f;
		int n;

		for (n = 0; n < silence_frames + tone_frames; n++)
		{
			const double two_pi = 6.283185307179586;
			float value = (n < silence_frames) ? 0.0f : amplitude * (float)sin(two_pi * 30.0 * (double)n / (double)sample_rate);
			float frame[2] = { value, value };

			filterPreviewLevelFrame(frame, 2, &coeffs, &leveller, true);
			if (n == silence_frames - 1)
			{
				CHECK((leveller.gain == 1.0f), "testFilterPreviewLeveller");
			}
			if (fabsf(frame[0]) > max_magnitude)
			{
				max_magnitude = fabsf(frame[0]);
			}
		}
		CHECK((max_magnitude <= amplitude * 1.5f), "testFilterPreviewLeveller");
	}
}

// Reset gives the leveller's starting state, and a rate change keeps the remaining peak hold at the same fraction of
// the hold time (a finished or unknown hold maps to the whole new hold, or stays 0).
static void testFilterPreviewLevellerResetAndRescale()
{
	FilterPreviewLeveller leveller = { 0.5f, 7.0f, 0.3f, 12 };

	filterPreviewLevellerReset(&leveller);
	CHECK((leveller.mean_square == 0.0f && leveller.gain == 1.0f && leveller.peak_env == 0.0f && leveller.peak_hold == 0),
		"testFilterPreviewLevellerResetAndRescale");

	filterPreviewLevellerRescaleHold(&leveller, 2205, 2400);
	CHECK((leveller.peak_hold == 0), "testFilterPreviewLevellerResetAndRescale: 0 -> %d", leveller.peak_hold);
	leveller.peak_hold = 1000;
	filterPreviewLevellerRescaleHold(&leveller, 2205, 4410);
	CHECK((leveller.peak_hold == 2000), "testFilterPreviewLevellerResetAndRescale: 1000 -> %d", leveller.peak_hold);
	leveller.peak_hold = 1000;
	filterPreviewLevellerRescaleHold(&leveller, 2400, 2205);
	CHECK((leveller.peak_hold == 918), "testFilterPreviewLevellerResetAndRescale: 1000 -> %d", leveller.peak_hold);
	leveller.peak_hold = 2205;
	filterPreviewLevellerRescaleHold(&leveller, 2205, 2400);
	CHECK((leveller.peak_hold == 2400), "testFilterPreviewLevellerResetAndRescale: full -> %d", leveller.peak_hold);
	leveller.peak_hold = 5;
	filterPreviewLevellerRescaleHold(&leveller, 0, 2400);
	CHECK((leveller.peak_hold == 2400), "testFilterPreviewLevellerResetAndRescale: no old hold -> %d", leveller.peak_hold);
}

// With the peak cap engaged on a steady low tone, the leveller must stay linear: the cap's peak follower holds over
// the gaps between the wave's peaks instead of sagging and re-clipping each crest (flat-topping on the bass previews).
// The rms gain is frozen at 10 (b_update false), the tone is 0.05, so gain x peak = 0.5 > the cap and the applied gain
// is cap / 0.05 = 5. After settling, the output must equal the input times one constant: residual below -60 dB.
// 20 and 30 Hz have a whole number of samples per period at 48 kHz; 27.3 Hz does not (its sampled peaks differ
// slightly from cycle to cycle).
static void testFilterPreviewPeakCapLinear()
{
	const int sample_rate = 48000;
	const int settle_frames = sample_rate / 5;
	const int measure_frames = sample_rate;
	const double frequencies[] = { 20.0, 27.3, 30.0 };
	const double amplitude = 0.05;
	const double two_pi = 6.283185307179586;
	FilterPreviewLevelCoeffs coeffs = filterPreviewLevelCoeffs(sample_rate);
	size_t index;

	for (index = 0; index < sizeof(frequencies) / sizeof(frequencies[0]); index++)
	{
		FilterPreviewLeveller leveller = { 0.0f, 10.0f, 0.0f, 0 };
		double in_out = 0.0, in_in = 0.0, out_out = 0.0;
		std::vector<float> input((size_t)measure_frames), output((size_t)measure_frames);
		int n;

		for (n = 0; n < settle_frames + measure_frames; n++)
		{
			const float value = (float)(amplitude * sin(two_pi * frequencies[index] * (double)n / (double)sample_rate));
			float frame[2] = { value, value };

			filterPreviewLevelFrame(frame, 2, &coeffs, &leveller, false);
			CHECK((leveller.gain == 10.0f), "testFilterPreviewPeakCapLinear");
			if (n >= settle_frames)
			{
				input[(size_t)(n - settle_frames)] = value;
				output[(size_t)(n - settle_frames)] = frame[0];
			}
		}

		for (n = 0; n < measure_frames; n++)
		{
			in_out += (double)input[(size_t)n] * (double)output[(size_t)n];
			in_in += (double)input[(size_t)n] * (double)input[(size_t)n];
			out_out += (double)output[(size_t)n] * (double)output[(size_t)n];
		}

		// Best-fit scale and the residual energy left after it, relative to the output.
		const double scale = in_out / in_in;
		const double residual = out_out - scale * in_out;
		const double residual_db = 10.0 * log10((residual > 1e-30 ? residual : 1e-30) / out_out);

		CHECK((scale > 4.9 && scale < 5.1), "testFilterPreviewPeakCapLinear");
		CHECK((residual_db < -60.0), "testFilterPreviewPeakCapLinear");
	}
}

static void testFilterDesiredSource()
{
	const int sample_rate = 48000;
	const int num_channels = 2;
	const int buffer_frames = 480;
	FilterAudioState *state = new FilterAudioState;
	FilterIirExchange exchange;
	FilterIirParams set1, set2;
	std::vector<float> buffer((size_t)buffer_frames * num_channels, 0.25f);
	unsigned int reset_serial = 1u;
	unsigned int format_serial = 0u;
	auto run = [&](int buffers) {
		for (int i = 0; i < buffers; i++)
		{
			filterProcessBuffer(state, &exchange, sample_rate, reset_serial, format_serial, buffer.data(), buffer_frames,
				num_channels, 0, [&]() { return filterDesiredSourceFor(state, true, sample_rate, reset_serial); });
		}
	};

	filterAudioStateInit(state);
	filterIirExchangeInit(&exchange);
	filterDesignIirParams(sample_rate, 24, 20.0f, 0.0f, 0.0f, 1u, &set1);
	filterDesignIirParams(sample_rate, 24, 20.0f, 0.0f, 0.0f, 2u, &set2);

	CHECK((filterDesiredSourceFor(state, false, sample_rate, reset_serial) == FILTER_SOURCE_DRY), "testFilterDesiredSource");
	CHECK((filterDesiredSourceFor(state, true, sample_rate, reset_serial) == FILTER_SOURCE_DRY), "testFilterDesiredSource");

	filterIirExchangePublish(&exchange, &set1);
	run(5);
	CHECK((state->source == FILTER_SOURCE_IIR && state->crossfade_from < 0 && state->iir_running), "testFilterDesiredSource");
	CHECK((filterDesiredSourceFor(state, false, sample_rate, reset_serial) == FILTER_SOURCE_DRY), "testFilterDesiredSource");
	CHECK((filterDesiredSourceFor(state, true, 44100, reset_serial) == FILTER_SOURCE_DRY), "testFilterDesiredSource");

	reset_serial = 2u;
	CHECK((filterDesiredSourceFor(state, true, sample_rate, reset_serial) == FILTER_SOURCE_IIR), "testFilterDesiredSource");
	run(3);
	CHECK((state->source == FILTER_SOURCE_IIR && state->crossfade_from < 0), "testFilterDesiredSource");

	format_serial = 1u;
	run(5);
	CHECK((state->iir_keep_blocked), "testFilterDesiredSource");
	CHECK((filterDesiredSourceFor(state, true, sample_rate, reset_serial) == FILTER_SOURCE_DRY), "testFilterDesiredSource");
	CHECK((state->source == FILTER_SOURCE_DRY), "testFilterDesiredSource");
	filterIirExchangePublish(&exchange, &set2);
	run(5);
	CHECK((state->source == FILTER_SOURCE_IIR && !state->iir_keep_blocked), "testFilterDesiredSource");

	delete state;
}

static void testFilterTransitions()
{
	enum ExpectKind { EXPECT_NONE, EXPECT_DRY, EXPECT_IIR, EXPECT_AUDIBLE };
	struct Phase {
		int first_buffer;
		int desired;
		int preview_mask;
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
	const int D = FILTER_SOURCE_DRY, I = FILTER_SOURCE_IIR;
	const Phase script[] = {
		{   0, D, 0, 1u, 0u, 0, -1, EXPECT_DRY },
		{  10, I, 0, 1u, 0u, -1, -1, EXPECT_NONE },
		{  11, D, 0, 1u, 0u, -1, -1, EXPECT_NONE },
		{  12, D, 0, 1u, 0u, -1, -1, EXPECT_DRY },
		{  15, I, 0, 1u, 0u, -1, 0, EXPECT_IIR },
		{  45, I, 0, 1u, 0u, 1, -1, EXPECT_NONE },
		{  46, I, 0, 1u, 0u, 2, -1, EXPECT_NONE },
		{  48, I, 0, 1u, 0u, -1, 2, EXPECT_IIR },
		{  80, I, FILT_CHAIN_CHANGED_LOW_CUT, 1u, 0u, -1, -1, EXPECT_AUDIBLE },
		{ 100, I, 0, 1u, 0u, -1, -1, EXPECT_NONE },
		{ 105, I, 0, 1u, 0u, 3, -1, EXPECT_NONE },
		{ 108, I, 0, 2u, 0u, -1, 0, EXPECT_IIR },
		{ 140, I, 0, 3u, 1u, -1, 0, EXPECT_NONE },
		{ 142, I, 0, 3u, 1u, 4, -1, EXPECT_IIR },
		{ 155, D, 0, 3u, 1u, -1, -1, EXPECT_DRY },
		{ 185, D, 0, 3u, 1u, -1, -1, EXPECT_NONE }
	};
	const int num_phases = (int)(sizeof(script) / sizeof(script[0])) - 1;
	FilterAudioState *state = new FilterAudioState;
	FilterIirExchange exchange;
	FilterIirParams sets[5];
	FilterIirParams reference_params = {};
	FiltChainState reference_state;
	std::vector<float> buffer((size_t)buffer_frames * num_channels);
	std::vector<float> input((size_t)buffer_frames);
	std::vector<float> reference((size_t)buffer_frames);
	double max_step = 0.0;
	float previous = 0.0f;
	long long t = 0;
	int phase_index, buffer_index, n;

	filterAudioStateInit(state);
	filterIirExchangeInit(&exchange);
	filterDesignIirParams(sample_rate, 24, 20.0f, 0.0f, 0.0f, 1u, &sets[0]);
	filterDesignIirParams(sample_rate, 24, 200.0f, 0.0f, 0.0f, 1u, &sets[1]);
	filterDesignIirParams(sample_rate, 48, 200.0f, 0.0f, 0.0f, 1u, &sets[2]);
	filterDesignIirParams(sample_rate, 24, 20.0f, 0.0f, 0.0f, 2u, &sets[3]);
	filterDesignIirParams(sample_rate, 24, 20.0f, 0.0f, 0.0f, 3u, &sets[4]);
	filtChainResetState(&reference_state);

	for (phase_index = 0; phase_index < num_phases; phase_index++)
	{
		const Phase& phase = script[phase_index];
		const int last_buffer = script[phase_index + 1].first_buffer - 1;

		if (phase.publish >= 0)
		{
			filterIirExchangePublish(&exchange, &sets[phase.publish]);
		}
		if (phase.reference >= 0)
		{
			reference_params = sets[phase.reference];
			filtChainResetState(&reference_state);
		}

		for (buffer_index = phase.first_buffer; buffer_index <= last_buffer; buffer_index++)
		{
			const int desired = phase.desired;

			for (n = 0; n < buffer_frames; n++, t++)
			{
				float value = (float)(amplitude * sin(two_pi * 100.0 * (double)t / (double)sample_rate));
				float unused_removed;

				input[n] = value;
				buffer[(size_t)n * num_channels] = value;
				buffer[(size_t)n * num_channels + 1] = value;
				filtChainProcessFrame(&value, &reference[n], &unused_removed, 1, 0, &reference_params.chain,
					&reference_state);
			}

			filterProcessBuffer(state, &exchange, sample_rate, phase.reset_serial, phase.format_serial, buffer.data(), buffer_frames,
				num_channels, phase.preview_mask, [desired]() { return desired; });

			if (buffer_index == 45 || buffer_index == 47)
			{
				CHECK((state->iir_crossfading && state->iir_running), "testFilterTransitions");
			}
			if (buffer_index == 46)
			{
				CHECK((!state->iir_crossfading && state->iir_taken_pending), "testFilterTransitions");
				CHECK((state->iir_taken.chain.num_low_cut == sets[2].chain.num_low_cut), "testFilterTransitions");
			}
			if (buffer_index == 47)
			{
				CHECK((!state->iir_taken_pending && state->iir_params[state->iir_cur ^ 1].chain.num_low_cut == sets[2].chain.num_low_cut), "testFilterTransitions");
			}
			if (buffer_index == 106 || buffer_index == 107)
			{
				CHECK((state->iir_taken_pending && state->iir_taken.reset_serial == 2u), "testFilterTransitions");
				CHECK((filterIirReady(state, sample_rate, 1u)), "testFilterTransitions");
			}
			if (buffer_index == 140)
			{
				for (n = 0; n < buffer_frames; n++)
				{
					float fade = (n + 1 >= state->fade_frames) ? 1.0f : (float)(n + 1) / (float)state->fade_frames;

					CHECK((fabsf(buffer[(size_t)n * num_channels] - fade * reference[n]) < 1e-6f), "testFilterTransitions");
				}
				CHECK((state->iir_keep_blocked), "testFilterTransitions");
			}
			if (buffer_index == 142)
			{
				CHECK((state->iir_crossfading && !state->iir_keep_blocked), "testFilterTransitions");
			}
			if (buffer_index == 108)
			{
				CHECK((!state->iir_taken_pending && filterIirReady(state, sample_rate, 2u)), "testFilterTransitions");
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
				CHECK((buffer[(size_t)n * num_channels + 1] == sample), "testFilterTransitions");
			}
		}

		if (phase.expect == EXPECT_DRY)
		{
			for (n = 0; n < buffer_frames; n++)
			{
				CHECK((buffer[(size_t)n * num_channels] == input[n]), "testFilterTransitions");
			}
		}
		else if (phase.expect == EXPECT_IIR)
		{
			CHECK((state->source == FILTER_SOURCE_IIR && state->gain_pos == state->fade_frames), "testFilterTransitions");
			for (n = 0; n < buffer_frames; n++)
			{
				CHECK((fabsf(buffer[(size_t)n * num_channels] - reference[n]) < 1e-3f), "testFilterTransitions");
			}
		}
		else if (phase.expect == EXPECT_AUDIBLE)
		{
			double sum = 0.0;

			for (n = 0; n < buffer_frames; n++)
			{
				sum += (double)buffer[(size_t)n * num_channels] * (double)buffer[(size_t)n * num_channels];
			}
			CHECK((sqrt(sum / (double)buffer_frames) > 0.1), "testFilterTransitions");
		}
	}

	CHECK((max_step < 1.5 * sine_step), "testFilterTransitions");

	CHECK((state->source == FILTER_SOURCE_DRY && state->crossfade_from < 0 && state->gain_pos == state->fade_frames), "testFilterTransitions");
	CHECK((!state->iir_running && state->preview_pos == 0 && state->preview_mask == 0), "testFilterTransitions");

	delete state;
}

static void testFilterChainSteady()
{
	const int sample_rate = 48000;
	const int num_channels = 2;
	const int buffer_frames = 480;
	const int num_buffers = 40;
	const double two_pi = 6.283185307179586;
	FilterAudioState *state = new FilterAudioState;
	FilterIirExchange exchange;
	FilterIirParams params;
	FiltChainState reference_state;
	std::vector<float> buffer((size_t)buffer_frames * num_channels);
	std::vector<float> reference((size_t)buffer_frames * num_channels);
	bool steady = false;
	int checked = 0;
	long long t = 0;
	int buffer_index, n, channel;

	filterAudioStateInit(state);
	filterIirExchangeInit(&exchange);
	filterDesignIirParams(sample_rate, 24, 100.0f, 16000.0f, 150.0f, 1u, &params);
	CHECK((params.chain.num_low_cut > 0 && params.chain.num_high_cut > 0 &&
		params.chain.num_mono_side == FILT_CHAIN_MONO_SECTIONS), "testFilterChainSteady");
	filterIirExchangePublish(&exchange, &params);

	// The set is adopted before the first buffer's dry-to-IIR crossfade, so the engine starts its IIR with fresh
	// state on frame 0 of buffer 0; the reference starts there too.
	filtChainResetState(&reference_state);

	for (buffer_index = 0; buffer_index < num_buffers; buffer_index++)
	{
		for (n = 0; n < buffer_frames; n++, t++)
		{
			const double time = (double)t / (double)sample_rate;
			float *frame = &buffer[(size_t)n * num_channels];
			float unused_removed[2];

			frame[0] = (float)(0.3 * sin(two_pi * 60.0 * time) + 0.2 * sin(two_pi * 1000.0 * time) +
				0.1 * sin(two_pi * 18000.0 * time));
			frame[1] = (float)(-0.25 * sin(two_pi * 80.0 * time) + 0.2 * sin(two_pi * 3000.0 * time));
			filtChainProcessFrame(frame, &reference[(size_t)n * num_channels], unused_removed, num_channels,
				0, &params.chain, &reference_state);
		}

		filterProcessBuffer(state, &exchange, sample_rate, 1u, 0u, buffer.data(), buffer_frames, num_channels,
			0, []() { return FILTER_SOURCE_IIR; });
		CHECK((state->iir_running), "testFilterChainSteady");

		if (steady)
		{
			for (n = 0; n < buffer_frames; n++)
			{
				for (channel = 0; channel < num_channels; channel++)
				{
					const size_t index = (size_t)n * num_channels + channel;

					CHECK((fabsf(buffer[index] - reference[index]) < 1e-5f), "testFilterChainSteady");
				}
			}
			checked++;
		}
		steady = state->source == FILTER_SOURCE_IIR && state->crossfade_from < 0 &&
			state->gain_pos >= state->fade_frames && state->preview_pos == 0;
	}

	CHECK((steady && checked >= num_buffers - 3), "testFilterChainSteady");

	delete state;
}

static void testFilterToggleKeepsLowCut()
{
	// A steady 40 Hz / 48 dB low cut on a 50 Hz sine; then the high cut is switched on. Only the high cut may start
	// from zero: the low cut keeps its history, so the output stays on the low-cut-only reference through and after
	// the crossfade. Restarting the low cut here would ring for tens of milliseconds (an audible bass thump).
	const int sample_rate = 48000;
	const int num_channels = 2;
	const int buffer_frames = 480;
	const int publish_buffer = 150;
	const int num_buffers = 200;
	const double amplitude = 0.5;
	const double two_pi = 6.283185307179586;
	// The added 16 kHz high cut only adds a few samples of delay at 50 Hz (about 0.005 at this level).
	const float tolerance = 0.01f;
	FilterAudioState *state = new FilterAudioState;
	FilterIirExchange exchange;
	FilterIirParams low_only, low_high;
	FiltChainState reference_state;
	std::vector<float> buffer((size_t)buffer_frames * num_channels);
	std::vector<float> reference((size_t)buffer_frames);
	bool steady = false;
	bool crossfade_seen = false;
	float max_diff_after_publish = 0.0f;
	long long t = 0;
	int buffer_index, n;

	filterAudioStateInit(state);
	filterIirExchangeInit(&exchange);
	filterDesignIirParams(sample_rate, 48, 40.0f, 0.0f, 0.0f, 1u, &low_only);
	filterDesignIirParams(sample_rate, 48, 40.0f, 16000.0f, 0.0f, 1u, &low_high);
	CHECK((low_only.chain.num_low_cut > 0 && low_only.chain.num_high_cut == 0), "testFilterToggleKeepsLowCut");
	CHECK((low_high.chain.num_high_cut > 0), "testFilterToggleKeepsLowCut");
	CHECK((filtChainChangedStages(&low_only.chain, &low_high.chain) == FILT_CHAIN_CHANGED_HIGH_CUT), "testFilterToggleKeepsLowCut");
	filterIirExchangePublish(&exchange, &low_only);
	filtChainResetState(&reference_state);

	for (buffer_index = 0; buffer_index < num_buffers; buffer_index++)
	{
		if (buffer_index == publish_buffer)
		{
			CHECK((steady), "testFilterToggleKeepsLowCut");
			filterIirExchangePublish(&exchange, &low_high);
		}

		for (n = 0; n < buffer_frames; n++, t++)
		{
			float value = (float)(amplitude * sin(two_pi * 50.0 * (double)t / (double)sample_rate));
			float unused_removed;

			buffer[(size_t)n * num_channels] = value;
			buffer[(size_t)n * num_channels + 1] = value;
			filtChainProcessFrame(&value, &reference[n], &unused_removed, 1, 0, &low_only.chain,
				&reference_state);
		}

		filterProcessBuffer(state, &exchange, sample_rate, 1u, 0u, buffer.data(), buffer_frames, num_channels,
			0, []() { return FILTER_SOURCE_IIR; });
		if (state->iir_crossfading)
		{
			crossfade_seen = true;
		}

		if (steady)
		{
			for (n = 0; n < buffer_frames; n++)
			{
				const float diff = fabsf(buffer[(size_t)n * num_channels] - reference[n]);

				CHECK((buffer[(size_t)n * num_channels + 1] == buffer[(size_t)n * num_channels]), "testFilterToggleKeepsLowCut");
				if (buffer_index < publish_buffer)
				{
					CHECK((diff < 1e-5f), "testFilterToggleKeepsLowCut");
				}
				else if (diff > max_diff_after_publish)
				{
					max_diff_after_publish = diff;
				}
			}
		}
		steady = state->source == FILTER_SOURCE_IIR && state->crossfade_from < 0 &&
			state->gain_pos >= state->fade_frames && state->preview_pos == 0;
	}

	CHECK((crossfade_seen && !state->iir_crossfading), "testFilterToggleKeepsLowCut");
	CHECK((state->iir_params[state->iir_cur].chain.num_high_cut == low_high.chain.num_high_cut), "testFilterToggleKeepsLowCut");
	CHECK((max_diff_after_publish > 0.0f && max_diff_after_publish < tolerance), "testFilterToggleKeepsLowCut");

	delete state;
}

// Switching the preview between two non-empty selections crossfades the removed signal directly: the preview stays fully
// in (preview_pos never moves, so the normal programme is never heard), the leveller keeps its gain (no reset to 1),
// and the output has no step larger than the steady previews' own. Low cut 100 Hz, high cut 16 kHz, on a quiet 100 Hz +
// 17 kHz input (quiet so the leveller's gain is well above 1). After the low cut preview: the high cut; then on
// consecutive buffers (each half of the 960-frame crossfade) low cut, low | high (a third selection mid-crossfade: it
// waits for the crossfade to end, then starts its own), low | high, high cut (again a third selection: waits); later
// low cut then high cut again (the selection being faded out, requested mid-crossfade: the crossfade reverses).
static void testFilterPreviewSwitch()
{
	struct Step {
		int first_buffer;
		int mask;
	};
	const int sample_rate = 48000;
	const int num_channels = 2;
	const int buffer_frames = 480;
	const int low = FILT_CHAIN_CHANGED_LOW_CUT, high = FILT_CHAIN_CHANGED_HIGH_CUT;
	const Step script[] = {
		{ 0, 0 },
		{ 10, low },
		{ 60, high },
		{ 110, low },
		{ 111, low | high },
		{ 112, low | high },
		{ 113, high },
		{ 140, low },
		{ 141, high },
		{ 170, -1 }
	};
	const int steady_from = 30;
	const double amplitude = 0.02;
	const double two_pi = 6.283185307179586;
	FilterAudioState *state = new FilterAudioState;
	FilterIirExchange exchange;
	FilterIirParams params;
	std::vector<float> buffer((size_t)buffer_frames * num_channels);
	double steady_low_step = 0.0, steady_high_step = 0.0, switch_step = 0.0;
	float previous = 0.0f;
	float low_gain = 0.0f, min_switch_gain = 1e30f;
	bool high_switch_seen = false, wait_seen = false, reverse_seen = false;
	int last_switch_pos = 0;
	long long t = 0;
	int step_index, buffer_index, n;

	filterAudioStateInit(state);
	filterIirExchangeInit(&exchange);
	filterDesignIirParams(sample_rate, 24, 100.0f, 16000.0f, 150.0f, 1u, &params);
	filterIirExchangePublish(&exchange, &params);

	for (step_index = 0; script[step_index].mask >= 0; step_index++)
	{
		const int requested = script[step_index].mask;

		for (buffer_index = script[step_index].first_buffer; buffer_index < script[step_index + 1].first_buffer; buffer_index++)
		{
			const int mask_before = state->preview_mask;
			const int incoming_before = state->preview_mask_incoming;
			const int switch_pos_before = state->preview_switch_pos;
			double buffer_step = 0.0;

			for (n = 0; n < buffer_frames; n++, t++)
			{
				const double time = (double)t / (double)sample_rate;
				const float value = (float)(amplitude * (sin(two_pi * 100.0 * time) + sin(two_pi * 17000.0 * time)));

				buffer[(size_t)n * num_channels] = value;
				buffer[(size_t)n * num_channels + 1] = value;
			}

			filterProcessBuffer(state, &exchange, sample_rate, 1u, 0u, buffer.data(), buffer_frames, num_channels, requested,
				[]() { return FILTER_SOURCE_IIR; });

			for (n = 0; n < buffer_frames; n++)
			{
				const float sample = buffer[(size_t)n * num_channels];

				if (buffer_index > 0 && fabs((double)sample - (double)previous) > buffer_step)
				{
					buffer_step = fabs((double)sample - (double)previous);
				}
				previous = sample;
			}

			if (buffer_index < script[2].first_buffer)
			{
				if (buffer_index >= script[1].first_buffer + steady_from)
				{
					CHECK((state->preview_mask == low && state->preview_mask_incoming == 0), "testFilterPreviewSwitch");
					CHECK((state->preview_pos == state->crossfade_frames), "testFilterPreviewSwitch");
					steady_low_step = (buffer_step > steady_low_step) ? buffer_step : steady_low_step;
					low_gain = state->preview_leveller.gain;
				}
				continue;
			}

			// From the first switch on the preview never leaves: only the removed signal crossfades.
			CHECK((state->preview_pos == state->crossfade_frames), "testFilterPreviewSwitch");
			if (state->preview_leveller.gain < min_switch_gain)
			{
				min_switch_gain = state->preview_leveller.gain;
			}

			if (buffer_index == script[2].first_buffer)
			{
				// Half of the 960-frame crossfade done in this 480-frame buffer.
				CHECK((state->preview_mask == low && state->preview_mask_incoming == high), "testFilterPreviewSwitch");
				CHECK((state->preview_switch_pos == buffer_frames), "testFilterPreviewSwitch");
				high_switch_seen = true;
			}
			else if (state->preview_mask_incoming != 0)
			{
				CHECK((state->preview_switch_pos > 0 && state->preview_switch_pos < state->crossfade_frames), "testFilterPreviewSwitch");
				if (incoming_before == state->preview_mask_incoming)
				{
					CHECK((state->preview_switch_pos > last_switch_pos), "testFilterPreviewSwitch");
				}
			}
			if (buffer_index == script[4].first_buffer)
			{
				// low | high requested while high -> low is half way: it waits, high -> low completes.
				CHECK((mask_before == high && incoming_before == low && switch_pos_before == buffer_frames), "testFilterPreviewSwitch");
				CHECK((state->preview_mask == low && state->preview_mask_incoming == 0), "testFilterPreviewSwitch");
				wait_seen = true;
			}
			if (buffer_index == script[5].first_buffer)
			{
				CHECK((state->preview_mask == low && state->preview_mask_incoming == (low | high)), "testFilterPreviewSwitch");
				CHECK((state->preview_switch_pos == buffer_frames), "testFilterPreviewSwitch");
			}
			if (buffer_index == script[6].first_buffer)
			{
				// high requested while low -> low | high is half way: neither end, so it waits again.
				CHECK((mask_before == low && incoming_before == (low | high)), "testFilterPreviewSwitch");
				CHECK((state->preview_mask == (low | high) && state->preview_mask_incoming == 0), "testFilterPreviewSwitch");
			}
			if (buffer_index == script[6].first_buffer + 1)
			{
				CHECK((state->preview_mask == (low | high) && state->preview_mask_incoming == high), "testFilterPreviewSwitch");
			}
			if (buffer_index == script[8].first_buffer)
			{
				// high requested while high -> low is half way: the crossfade runs back from where it is and ends on
				// high (waiting would have ended on low).
				CHECK((mask_before == high && incoming_before == low && switch_pos_before == buffer_frames), "testFilterPreviewSwitch");
				CHECK((state->preview_mask == high && state->preview_mask_incoming == 0), "testFilterPreviewSwitch");
				reverse_seen = true;
			}
			last_switch_pos = state->preview_switch_pos;

			if (buffer_index >= script[2].first_buffer + steady_from && buffer_index < script[3].first_buffer)
			{
				CHECK((state->preview_mask == high && state->preview_mask_incoming == 0), "testFilterPreviewSwitch");
				steady_high_step = (buffer_step > steady_high_step) ? buffer_step : steady_high_step;
			}
			else if (buffer_step > switch_step)
			{
				switch_step = buffer_step;
			}
		}
	}

	CHECK((high_switch_seen && wait_seen && reverse_seen), "testFilterPreviewSwitch");
	CHECK((state->preview_mask == high && state->preview_mask_incoming == 0 && state->preview_pos == state->crossfade_frames), "testFilterPreviewSwitch");
	// The leveller runs through the switches: its gain is well above 1 and never reset to 1.
	CHECK((low_gain > 2.0f && min_switch_gain > 2.0f), "testFilterPreviewSwitch");
	CHECK((steady_low_step > 0.0 && steady_high_step > 0.0), "testFilterPreviewSwitch");
	CHECK((switch_step < 2.0 * ((steady_low_step > steady_high_step) ? steady_low_step : steady_high_step)), "testFilterPreviewSwitch");

	delete state;
}

static FilterSettings debugFilterSettings(bool filters_on, bool low_on, float low_hz, bool high_on,
	float high_hz, bool mono_on, float mono_hz)
{
	FilterSettings settings;

	settings.filters_on = filters_on;
	settings.low_on = low_on;
	settings.low_hz = low_hz;
	settings.high_on = high_on;
	settings.high_hz = high_hz;
	settings.mono_on = mono_on;
	settings.mono_hz = mono_hz;
	return settings;
}

// Switching the preview from a quiet removed signal to a loud one must not burst. The leveller keeps its gain across
// a switch (no reset), so after a long quiet preview (high gain) it has to pull the gain down as fast as the loud
// signal fades in. Low cut 40 Hz and high cut 5 kHz on 0.004 at 30 Hz (removed by the low cut: quiet, leveller gain
// high) plus 0.15 at 8 kHz (removed by the high cut: loud, levelled gain 1). Low cut preview for 3 s, then the high cut
// preview. The peak after the switch must stay at the leveller's peak cap (DFXG_FILTER_PREVIEW_PEAK_CAP) and far
// below the limiter; without the cap the kept gain (about 37) bursts to full scale while the rms detector catches up.
static void testFilterPreviewSwitchNoBurst()
{
	const int sample_rate = 48000;
	const int num_channels = 2;
	const int buffer_frames = 480;
	const int low_buffers = 300;
	const int high_buffers = 100;
	const int steady_from = low_buffers + 60;
	const double two_pi = 6.283185307179586;
	FilterAudioState *state = new FilterAudioState;
	FilterIirExchange exchange;
	FilterIirParams params;
	std::vector<float> buffer((size_t)buffer_frames * num_channels);
	float low_gain = 0.0f, switch_peak = 0.0f, steady_peak = 0.0f;
	long long t = 0;
	int buffer_index, n;

	filterAudioStateInit(state);
	filterIirExchangeInit(&exchange);
	filterDesignIirParams(sample_rate, 24, 40.0f, 5000.0f, 0.0f, 1u, &params);
	filterIirExchangePublish(&exchange, &params);

	for (buffer_index = 0; buffer_index < low_buffers + high_buffers; buffer_index++)
	{
		const int mask = (buffer_index < low_buffers) ? FILT_CHAIN_CHANGED_LOW_CUT : FILT_CHAIN_CHANGED_HIGH_CUT;

		for (n = 0; n < buffer_frames; n++, t++)
		{
			const double time = (double)t / (double)sample_rate;
			const float value = (float)(0.004 * sin(two_pi * 30.0 * time) + 0.15 * sin(two_pi * 8000.0 * time));

			buffer[(size_t)n * num_channels] = value;
			buffer[(size_t)n * num_channels + 1] = value;
		}

		filterProcessBuffer(state, &exchange, sample_rate, 1u, 0u, buffer.data(), buffer_frames, num_channels, mask,
			[]() { return FILTER_SOURCE_IIR; });

		if (buffer_index == low_buffers - 1)
		{
			CHECK((state->preview_mask == FILT_CHAIN_CHANGED_LOW_CUT && state->preview_pos == state->crossfade_frames), "testFilterPreviewSwitchNoBurst");
			low_gain = state->preview_leveller.gain;
		}
		for (n = 0; n < buffer_frames * num_channels; n++)
		{
			const float magnitude = fabsf(buffer[(size_t)n]);

			if (buffer_index >= low_buffers && magnitude > switch_peak)
			{
				switch_peak = magnitude;
			}
			if (buffer_index >= steady_from && magnitude > steady_peak)
			{
				steady_peak = magnitude;
			}
		}
	}

	// The case only means something if the quiet preview had a high gain and the loud one is levelled near unity.
	CHECK((low_gain > 20.0f), "testFilterPreviewSwitchNoBurst");
	CHECK((steady_peak > 0.05f && steady_peak < 0.3f), "testFilterPreviewSwitchNoBurst");
	CHECK((switch_peak <= DFXG_FILTER_PREVIEW_PEAK_CAP + 1e-3f && switch_peak < 0.45f), "testFilterPreviewSwitchNoBurst");

	delete state;
}

// Changing the preview selection while a preview plays must not let the normal programme through. A steady IIR with a
// 30 Hz low cut, 16 kHz high cut and 120 Hz mono bass on a 1 kHz sine (identical channels): no stage removes any of
// the sine, so every preview plays near silence, and the normal programme is the full 0.5 sine. Preview the low cut,
// then switch to the high cut, low | high, the global preview, then quick switches inside a crossfade (low, mono, high
// on consecutive buffers). Once the first preview is fully in, the output must stay near silent through every switch.
static void testFilterPreviewNoLeak()
{
	struct Step {
		int first_buffer;
		int selection;
	};
	const int sample_rate = 48000;
	const int num_channels = 2;
	const int buffer_frames = 480;
	const double amplitude = 0.5;
	const double two_pi = 6.283185307179586;
	const float leak_bound = 0.02f;
	const int low = DfxDsp::PreviewLowCut, high = DfxDsp::PreviewHighCut, mono = DfxDsp::PreviewMonoBass;
	const Step script[] = {
		{ 0, DfxDsp::PreviewNone },
		{ 10, low },
		{ 40, high },
		{ 60, low | high },
		{ 80, DfxDsp::PreviewAll },
		{ 100, high },
		{ 120, low },
		{ 121, mono },
		{ 122, high },
		{ 150, DfxDsp::PreviewNone },
		{ 180, -1 }
	};
	const FilterSettings settings = debugFilterSettings(true, true, 30.0f, true, 16000.0f, true, 120.0f);
	FilterAudioState *state = new FilterAudioState;
	FilterIirExchange exchange;
	FilterIirParams params;
	std::vector<float> buffer((size_t)buffer_frames * num_channels);
	bool measuring = false;
	float peak = 0.0f, peak_after_stop = 0.0f;
	long long t = 0;
	int step_index, buffer_index, n;

	CHECK((filterPreviewMaskFor(DfxDsp::PreviewAll, settings) == FILT_CHAIN_STAGES_ALL), "testFilterPreviewNoLeak");
	filterAudioStateInit(state);
	filterIirExchangeInit(&exchange);
	filterDesignParams(sample_rate, 24, settings, 1u, &params);
	CHECK((params.chain.num_low_cut > 0 && params.chain.num_high_cut > 0 && params.chain.num_mono_side > 0), "testFilterPreviewNoLeak");
	filterIirExchangePublish(&exchange, &params);

	for (step_index = 0; script[step_index].selection >= 0; step_index++)
	{
		const int mask = filterPreviewMaskFor(script[step_index].selection, settings);

		for (buffer_index = script[step_index].first_buffer; buffer_index < script[step_index + 1].first_buffer; buffer_index++)
		{
			for (n = 0; n < buffer_frames; n++, t++)
			{
				const float value = (float)(amplitude * sin(two_pi * 1000.0 * (double)t / (double)sample_rate));

				buffer[(size_t)n * num_channels] = value;
				buffer[(size_t)n * num_channels + 1] = value;
			}

			filterProcessBuffer(state, &exchange, sample_rate, 1u, 0u, buffer.data(), buffer_frames, num_channels, mask,
				[]() { return FILTER_SOURCE_IIR; });
			if (script[step_index].selection == DfxDsp::PreviewNone)
			{
				measuring = false;
			}

			for (n = 0; n < buffer_frames * num_channels; n++)
			{
				const float magnitude = fabsf(buffer[(size_t)n]);

				if (measuring && magnitude > peak)
				{
					peak = magnitude;
				}
				if (script[step_index].selection == DfxDsp::PreviewNone && buffer_index >= 170 && magnitude > peak_after_stop)
				{
					peak_after_stop = magnitude;
				}
			}
			if (script[step_index].selection != DfxDsp::PreviewNone && state->preview_pos == state->crossfade_frames)
			{
				measuring = true;
			}
		}
	}

	// Every preview played near silence: the 1 kHz programme never leaked through a switch.
	CHECK((peak < leak_bound), "testFilterPreviewNoLeak");
	// Stopping the preview still ramps back to the normal programme.
	CHECK((state->preview_pos == 0 && state->preview_mask == 0), "testFilterPreviewNoLeak");
	CHECK((peak_after_stop > 0.45f), "testFilterPreviewNoLeak");

	delete state;
}

// More channels than the chain filters (FILT_LOW_CUT_MAX_CHANNELS): the extra channels are not filtered, but they
// follow the preview mix, so with a preview fully in they are silent rather than playing the normal programme, and
// once the preview stops they come back unchanged.
static void testFilterExtraChannelsFollowPreview()
{
	const int sample_rate = 48000;
	const int num_channels = FILT_LOW_CUT_MAX_CHANNELS + 2;
	const int buffer_frames = 480;
	const float value = 0.5f;
	const FilterSettings settings = debugFilterSettings(true, true, 30.0f, true, 16000.0f, false, 0.0f);
	FilterAudioState *state = new FilterAudioState;
	FilterIirExchange exchange;
	FilterIirParams params;
	std::vector<float> buffer((size_t)buffer_frames * num_channels);
	float extra_peak_in_preview = 0.0f, extra_min_after = 1.0f, extra_max_after = 0.0f;
	int buffer_index, n, channel;

	filterAudioStateInit(state);
	filterIirExchangeInit(&exchange);
	filterDesignParams(sample_rate, 24, settings, 1u, &params);
	filterIirExchangePublish(&exchange, &params);

	for (buffer_index = 0; buffer_index < 60; buffer_index++)
	{
		const int mask = (buffer_index >= 10 && buffer_index < 30) ? DfxDsp::PreviewLowCut : 0;

		for (n = 0; n < buffer_frames * num_channels; n++)
		{
			buffer[(size_t)n] = value;
		}
		filterProcessBuffer(state, &exchange, sample_rate, 1u, 0u, buffer.data(), buffer_frames, num_channels, mask,
			[]() { return FILTER_SOURCE_IIR; });

		for (n = 0; n < buffer_frames; n++)
		{
			for (channel = FILT_LOW_CUT_MAX_CHANNELS; channel < num_channels; channel++)
			{
				const float sample = buffer[(size_t)n * num_channels + channel];

				if (buffer_index >= 15 && buffer_index < 30 && fabsf(sample) > extra_peak_in_preview)
				{
					extra_peak_in_preview = fabsf(sample);
				}
				if (buffer_index >= 40)
				{
					extra_min_after = (sample < extra_min_after) ? sample : extra_min_after;
					extra_max_after = (sample > extra_max_after) ? sample : extra_max_after;
				}
			}
		}
	}

	CHECK((extra_peak_in_preview < 1e-6f), "testFilterExtraChannelsFollowPreview: peak %g", extra_peak_in_preview);
	CHECK((extra_min_after == value && extra_max_after == value), "testFilterExtraChannelsFollowPreview");

	delete state;
}

// Switching the high cut on while the low cut plays: the high cut was designed all along (its slider is not at Off),
// just not heard, so only the audible mask changes. The engine crossfades the two parameter sets with every stage's
// history kept: the output moves from the low-only reference to the low + high reference and then tracks it exactly.
// A restarted stage would leave the references (the low cut ringing on the 50 Hz tone would be an audible thump).
static void testFilterAudibleToggle()
{
	const int sample_rate = 48000;
	const int num_channels = 2;
	const int buffer_frames = 480;
	const int publish_buffer = 150;
	const int num_buffers = 200;
	const double two_pi = 6.283185307179586;
	FilterAudioState *state = new FilterAudioState;
	FilterIirExchange exchange;
	FilterIirParams low_heard, both_heard;
	FiltChainState low_state, both_state;
	FilterSettings settings;
	std::vector<float> buffer((size_t)buffer_frames * num_channels);
	std::vector<float> low_reference((size_t)buffer_frames);
	std::vector<float> both_reference((size_t)buffer_frames);
	bool steady = false, crossfade_seen = false, crossfade_done = false;
	float max_outside = 0.0f, max_after = 0.0f, max_reference_gap = 0.0f;
	long long t = 0;
	int buffer_index, n;

	settings = debugFilterSettings(true, true, 40.0f, false, 16000.0f, false, 0.0f);
	filterDesignParams(sample_rate, 48, settings, 1u, &low_heard);
	settings.high_on = true;
	filterDesignParams(sample_rate, 48, settings, 1u, &both_heard);
	CHECK((low_heard.chain.audible_mask == FILT_CHAIN_CHANGED_LOW_CUT), "testFilterAudibleToggle");
	CHECK((both_heard.chain.audible_mask == (FILT_CHAIN_CHANGED_LOW_CUT | FILT_CHAIN_CHANGED_HIGH_CUT)), "testFilterAudibleToggle");
	CHECK((filtChainChangedStages(&low_heard.chain, &both_heard.chain) == 0), "testFilterAudibleToggle");

	filterAudioStateInit(state);
	filterIirExchangeInit(&exchange);
	filterIirExchangePublish(&exchange, &low_heard);
	filtChainResetState(&low_state);
	filtChainResetState(&both_state);

	for (buffer_index = 0; buffer_index < num_buffers; buffer_index++)
	{
		const bool done_before = crossfade_done;

		if (buffer_index == publish_buffer)
		{
			CHECK((steady), "testFilterAudibleToggle");
			filterIirExchangePublish(&exchange, &both_heard);
		}

		for (n = 0; n < buffer_frames; n++, t++)
		{
			const double time = (double)t / (double)sample_rate;
			float value = (float)(0.4 * sin(two_pi * 50.0 * time) + 0.2 * sin(two_pi * 18000.0 * time));
			float unused_removed;

			buffer[(size_t)n * num_channels] = value;
			buffer[(size_t)n * num_channels + 1] = value;
			filtChainProcessFrame(&value, &low_reference[n], &unused_removed, 1, 0, &low_heard.chain, &low_state);
			filtChainProcessFrame(&value, &both_reference[n], &unused_removed, 1, 0, &both_heard.chain, &both_state);
		}

		filterProcessBuffer(state, &exchange, sample_rate, 1u, 0u, buffer.data(), buffer_frames, num_channels, 0,
			[]() { return FILTER_SOURCE_IIR; });
		if (state->iir_crossfading)
		{
			crossfade_seen = true;
		}
		crossfade_done = crossfade_seen && !state->iir_crossfading;

		if (steady)
		{
			for (n = 0; n < buffer_frames; n++)
			{
				const float out = buffer[(size_t)n * num_channels];
				const float lo = fminf(low_reference[n], both_reference[n]);
				const float hi = fmaxf(low_reference[n], both_reference[n]);

				CHECK((buffer[(size_t)n * num_channels + 1] == out), "testFilterAudibleToggle");
				max_reference_gap = fmaxf(max_reference_gap, hi - lo);
				if (buffer_index < publish_buffer)
				{
					CHECK((fabsf(out - low_reference[n]) < 1e-5f), "testFilterAudibleToggle");
					continue;
				}
				max_outside = fmaxf(max_outside, fmaxf(lo - out, out - hi));
				if (done_before)
				{
					max_after = fmaxf(max_after, fabsf(out - both_reference[n]));
				}
			}
		}
		steady = state->source == FILTER_SOURCE_IIR && state->crossfade_from < 0 &&
			state->gain_pos >= state->fade_frames && state->preview_pos == 0;
	}

	// The 18 kHz tone makes the two references clearly different, so staying between them means something.
	CHECK((max_reference_gap > 0.1f), "testFilterAudibleToggle");
	CHECK((crossfade_seen && crossfade_done), "testFilterAudibleToggle");
	CHECK((max_outside < 1e-5f), "testFilterAudibleToggle");
	CHECK((max_after < 1e-5f), "testFilterAudibleToggle");

	delete state;
}

static void testFilterPreviewAvailability()
{
	const float low_hz = (float)DfxDsp::LOW_CUT_DEFAULT_CUTOFF_HZ;
	const float high_off = (float)DfxDsp::HIGH_CUT_OFF_HZ;
	const float high_max = (float)DfxDsp::HIGH_CUT_MAX_CUTOFF_HZ;
	const float mono_hz = (float)DfxDsp::MONO_BASS_DEFAULT_CUTOFF_HZ;
	const int low = DfxDsp::PreviewLowCut, high = DfxDsp::PreviewHighCut, mono = DfxDsp::PreviewMonoBass;
	const int all = DfxDsp::PreviewAll, none = DfxDsp::PreviewNone;
	// Global on. Every slider set and every own switch on / every own switch off / every slider at Off.
	{
		const FilterSettings set_up = debugFilterSettings(true, true, low_hz, true, high_max, true, mono_hz);
		const FilterSettings switched_off = debugFilterSettings(true, false, low_hz, false, high_max, false, mono_hz);
		const FilterSettings sliders_off = debugFilterSettings(true, true, 0.0f, true, high_off, true, 0.0f);

		// Individual previews are available whatever their own switch says, and a slider at Off plays silence.
		for (int preview : { low, high, mono, low | high, low | high | mono })
		{
			CHECK((filterPreviewAvailableFor(preview, set_up)), "testFilterPreviewAvailability");
			CHECK((filterPreviewAvailableFor(preview, switched_off)), "testFilterPreviewAvailability");
			CHECK((filterPreviewAvailableFor(preview, sliders_off)), "testFilterPreviewAvailability");
		}
		CHECK((filterPreviewAvailableFor(none, set_up) && filterPreviewAvailableFor(none, sliders_off)), "testFilterPreviewAvailability");
		CHECK((!filterPreviewAvailableFor(1 << 8, set_up)), "testFilterPreviewAvailability");

		// The global preview needs a filter with its own switch on (slider at Off is silence).
		CHECK((filterPreviewAvailableFor(all, set_up)), "testFilterPreviewAvailability");
		CHECK((!filterPreviewAvailableFor(all, switched_off)), "testFilterPreviewAvailability");
		CHECK((filterPreviewAvailableFor(all, sliders_off)), "testFilterPreviewAvailability");
		CHECK((filterPreviewAvailableFor(all, debugFilterSettings(true, false, low_hz, false, high_max, true, mono_hz))), "testFilterPreviewAvailability");
		CHECK((filterPreviewAvailableFor(all, debugFilterSettings(true, true, 0.0f, false, high_max, false, mono_hz))), "testFilterPreviewAvailability");

		// Individual flags play their own stages (a union sums them), at Off too; the global preview plays the
		// switched-on stages.
		CHECK((filterPreviewMaskFor(low | high, switched_off) == (FILT_CHAIN_CHANGED_LOW_CUT | FILT_CHAIN_CHANGED_HIGH_CUT)), "testFilterPreviewAvailability");
		CHECK((filterPreviewMaskFor(high, sliders_off) == FILT_CHAIN_CHANGED_HIGH_CUT), "testFilterPreviewAvailability");
		CHECK((filterPreviewMaskFor(all, debugFilterSettings(true, true, low_hz, false, high_max, true, 0.0f)) ==
			(FILT_CHAIN_CHANGED_LOW_CUT | FILT_CHAIN_CHANGED_MONO)), "testFilterPreviewAvailability");
		CHECK((filterPreviewMaskFor(all, set_up) == FILT_CHAIN_STAGES_ALL), "testFilterPreviewAvailability");
		CHECK((filterPreviewMaskFor(none, set_up) == 0), "testFilterPreviewAvailability");

		// Heard: the own-on stages whose slider is not at Off.
		CHECK((filterAudibleMaskFor(set_up) == FILT_CHAIN_STAGES_ALL), "testFilterPreviewAvailability");
		CHECK((filterAudibleMaskFor(switched_off) == 0), "testFilterPreviewAvailability");
		CHECK((filterAudibleMaskFor(sliders_off) == 0), "testFilterPreviewAvailability");

		// The IIR runs when something is heard or previewed; a preview at Off still runs it, to play silence.
		CHECK((filterIirNeededFor(none, set_up)), "testFilterPreviewAvailability");
		CHECK((!filterIirNeededFor(none, switched_off)), "testFilterPreviewAvailability");
		CHECK((filterIirNeededFor(low, switched_off)), "testFilterPreviewAvailability");
		CHECK((!filterIirNeededFor(all, switched_off)), "testFilterPreviewAvailability");
		CHECK((filterIirNeededFor(low, sliders_off)), "testFilterPreviewAvailability");
	}

	// Global off: nothing is heard and every preview is unavailable, so the chain does not run.
	{
		const FilterSettings set_up = debugFilterSettings(false, true, low_hz, true, high_max, true, mono_hz);

		for (int preview : { low, high, mono, low | high, all })
		{
			CHECK((!filterPreviewAvailableFor(preview, set_up)), "testFilterPreviewAvailability");
			CHECK((filterPreviewMaskFor(preview, set_up) == 0), "testFilterPreviewAvailability");
			CHECK((filterPreviewSelectionFor(preview, set_up) == none), "testFilterPreviewAvailability");
			CHECK((!filterIirNeededFor(preview, set_up)), "testFilterPreviewAvailability");
		}
		CHECK((filterPreviewAvailableFor(none, set_up)), "testFilterPreviewAvailability");
		CHECK((filterAudibleMaskFor(set_up) == 0), "testFilterPreviewAvailability");
	}

	// The selection kept for a request: global wins over individual flags and drops when no filter is switched on;
	// individual flags are kept even with the slider at Off.
	{
		const FilterSettings high_at_off = debugFilterSettings(true, true, low_hz, true, high_off, false, mono_hz);

		CHECK((filterPreviewSelectionFor(low | high, high_at_off) == (low | high)), "testFilterPreviewAvailability");
		CHECK((filterPreviewSelectionFor(low | mono, high_at_off) == (low | mono)), "testFilterPreviewAvailability");
		CHECK((filterPreviewSelectionFor(all | low, high_at_off) == all), "testFilterPreviewAvailability");
		CHECK((filterPreviewSelectionFor(all, debugFilterSettings(true, false, low_hz, false, high_off, false, mono_hz)) == none), "testFilterPreviewAvailability");
		CHECK((filterPreviewSelectionFor(none, high_at_off) == none), "testFilterPreviewAvailability");
	}

	// Button presses: individual buttons toggle and combine; the global button replaces them and they replace it.
	CHECK((DfxDsp::toggledFilterPreview(none, DfxDsp::PreviewLowCut) == low), "testFilterPreviewAvailability");
	CHECK((DfxDsp::toggledFilterPreview(low, DfxDsp::PreviewHighCut) == (low | high)), "testFilterPreviewAvailability");
	CHECK((DfxDsp::toggledFilterPreview(low | high, DfxDsp::PreviewLowCut) == high), "testFilterPreviewAvailability");
	CHECK((DfxDsp::toggledFilterPreview(high, DfxDsp::PreviewHighCut) == none), "testFilterPreviewAvailability");
	CHECK((DfxDsp::toggledFilterPreview(low | high, DfxDsp::PreviewAll) == all), "testFilterPreviewAvailability");
	CHECK((DfxDsp::toggledFilterPreview(all, DfxDsp::PreviewMonoBass) == mono), "testFilterPreviewAvailability");
	CHECK((DfxDsp::toggledFilterPreview(all, DfxDsp::PreviewAll) == none), "testFilterPreviewAvailability");

	// The designer designs every stage whose slider is not at Off; the switches only set what is heard.
	{
		FilterIirParams params;

		filterDesignParams(48000, 24, debugFilterSettings(false, true, low_hz, false, 16000.0f, false, mono_hz), 1u, &params);
		CHECK((params.chain.num_low_cut > 0 && params.chain.num_high_cut > 0 &&
			params.chain.num_mono_side == FILT_CHAIN_MONO_SECTIONS), "testFilterPreviewAvailability");
		CHECK((params.chain.audible_mask == 0), "testFilterPreviewAvailability");
		filterDesignParams(48000, 24, debugFilterSettings(true, true, low_hz, false, 16000.0f, false, mono_hz), 1u, &params);
		CHECK((params.chain.audible_mask == FILT_CHAIN_CHANGED_LOW_CUT), "testFilterPreviewAvailability");
		filterDesignParams(48000, 24, debugFilterSettings(true, true, 0.0f, true, high_off, true, 0.0f), 1u, &params);
		CHECK((params.chain.num_low_cut == 0 && params.chain.num_high_cut == 0 && params.chain.num_mono_side == 0), "testFilterPreviewAvailability");
		CHECK((params.chain.audible_mask == 0), "testFilterPreviewAvailability");
	}
}

static bool settingsEqual(const FilterSettings &a, const FilterSettings &b)
{
	return a.filters_on == b.filters_on && a.low_on == b.low_on && a.low_hz == b.low_hz && a.high_on == b.high_on &&
		a.high_hz == b.high_hz && a.mono_on == b.mono_on && a.mono_hz == b.mono_hz;
}

// The per-filter setters' shared rules: a stage bit picks the one filter whose switch and cutoff change, and
// switching a filter off drops its own preview flag only.
static void testSetterRules()
{
	const FilterSettings s = debugFilterSettings(true, true, 40.0f, true, 12000.0f, false, 125.0f);
	const int selection = DfxDsp::PreviewLowCut | DfxDsp::PreviewHighCut;
	FilterSettings next, expected;

	next = filterSettingsWithStage(s, FILT_CHAIN_CHANGED_HIGH_CUT, false, 16000.0f);
	expected = s;
	expected.high_on = false;
	expected.high_hz = 16000.0f;
	CHECK((settingsEqual(next, expected)), "testSetterRules: high cut");

	next = filterSettingsWithStage(s, FILT_CHAIN_CHANGED_LOW_CUT, false, 80.0f);
	expected = s;
	expected.low_on = false;
	expected.low_hz = 80.0f;
	CHECK((settingsEqual(next, expected)), "testSetterRules: low cut");

	next = filterSettingsWithStage(s, FILT_CHAIN_CHANGED_MONO, true, 200.0f);
	expected = s;
	expected.mono_on = true;
	expected.mono_hz = 200.0f;
	CHECK((settingsEqual(next, expected)), "testSetterRules: mono");

	CHECK((filterStageOn(s, FILT_CHAIN_CHANGED_LOW_CUT) && filterStageHz(s, FILT_CHAIN_CHANGED_LOW_CUT) == 40.0f),
		"testSetterRules: low cut accessors");
	CHECK((filterStageOn(s, FILT_CHAIN_CHANGED_HIGH_CUT) && filterStageHz(s, FILT_CHAIN_CHANGED_HIGH_CUT) == 12000.0f),
		"testSetterRules: high cut accessors");
	CHECK((!filterStageOn(s, FILT_CHAIN_CHANGED_MONO) && filterStageHz(s, FILT_CHAIN_CHANGED_MONO) == 125.0f),
		"testSetterRules: mono accessors");

	next = filterSettingsWithStage(s, FILT_CHAIN_CHANGED_HIGH_CUT, false, s.high_hz);
	CHECK((filterPreviewSelectionFor(selection & ~DfxDsp::PreviewHighCut, next) == DfxDsp::PreviewLowCut),
		"testSetterRules: switching high cut off keeps the low cut preview");

	// Switching a filter off drops its own preview flag only; switching it on keeps the selection.
	CHECK((filterPreviewAfterSwitch(selection, FILT_CHAIN_CHANGED_HIGH_CUT, false) == DfxDsp::PreviewLowCut),
		"testSetterRules: high cut off drops its preview");
	CHECK((filterPreviewAfterSwitch(selection, FILT_CHAIN_CHANGED_MONO, false) == selection),
		"testSetterRules: mono off keeps the other previews");
	CHECK((filterPreviewAfterSwitch(selection, FILT_CHAIN_CHANGED_LOW_CUT, true) == selection),
		"testSetterRules: switching on keeps the selection");
	CHECK((filterPreviewAfterSwitch(DfxDsp::PreviewAll, FILT_CHAIN_CHANGED_LOW_CUT, false) == DfxDsp::PreviewAll),
		"testSetterRules: switching off keeps the global preview");
}

// A change resets the IIR chain (a fresh design, no stale history) exactly when it goes from not needed (nothing heard,
// nothing previewed) to needed.
static void testChangeNeedsReset()
{
	const FilterSettings global_off = debugFilterSettings(false, true, 40.0f, true, 16000.0f, false, 125.0f);
	const FilterSettings heard = debugFilterSettings(true, true, 40.0f, true, 16000.0f, false, 125.0f);
	const FilterSettings heard_moved = debugFilterSettings(true, true, 60.0f, true, 16000.0f, false, 125.0f);
	const FilterSettings all_switched_off = debugFilterSettings(true, false, 40.0f, false, 16000.0f, false, 125.0f);
	const int none = DfxDsp::PreviewNone, low = DfxDsp::PreviewLowCut;

	CHECK((filterChangeNeedsReset(none, global_off, none, heard)), "testChangeNeedsReset: global on");
	CHECK((filterChangeNeedsReset(none, all_switched_off, none, heard)), "testChangeNeedsReset: a switch on");
	CHECK((filterChangeNeedsReset(none, all_switched_off, low, all_switched_off)), "testChangeNeedsReset: preview starts");
	CHECK((!filterChangeNeedsReset(none, heard, none, heard_moved)), "testChangeNeedsReset: cutoff while heard");
	CHECK((!filterChangeNeedsReset(low, all_switched_off, none, heard)), "testChangeNeedsReset: preview to heard");
	CHECK((!filterChangeNeedsReset(none, heard, none, global_off)), "testChangeNeedsReset: global off");
	CHECK((!filterChangeNeedsReset(none, global_off, none, all_switched_off)),
		"testChangeNeedsReset: global on with nothing switched on");
	CHECK((!filterChangeNeedsReset(none, global_off, low, global_off)), "testChangeNeedsReset: preview with global off");
}

// FNV-1a 64-bit hash of the engine's output for a fixed script, recorded on the code before the refactor. Any change
// to the audio (a swapped argument, a lost constant, a different rounding) changes it.
static const uint64_t kGoldenHash = 0xc5553290315ddb45ull;

static uint64_t goldenHashBytes(uint64_t hash, const void *data, size_t size)
{
	const unsigned char *bytes = (const unsigned char *)data;
	size_t i;

	for (i = 0; i < size; i++)
	{
		hash ^= bytes[i];
		hash *= 0x100000001b3ull;
	}
	return hash;
}

struct GoldenPhase {
	FilterSettings settings;
	int selection;
};

// Runs a script of settings and preview selections through the engine (48 kHz, 2 channels, 20 buffers of 480 frames
// per phase) and hashes its output. The input is 50 Hz + 1 kHz + 17 kHz on both channels; with stereo_bass, the left
// channel also carries a 60 Hz tone, so the side signal has bass for the stereo to mono stage to fold.
static uint64_t goldenHashFor(const GoldenPhase *phases, size_t num_phases, int slope_db, bool stereo_bass)
{
	const int sample_rate = 48000;
	const int num_channels = 2;
	const int buffer_frames = 480;
	const int buffers_per_phase = 20;
	const unsigned int reset_serial = 1u;
	const double two_pi = 6.283185307179586;
	FilterAudioState *state = new FilterAudioState;
	FilterIirExchange exchange;
	std::vector<float> buffer((size_t)buffer_frames * num_channels);
	uint64_t hash = 0xcbf29ce484222325ull;
	long long t = 0;
	size_t phase_index;
	int buffer_index, n, channel;

	filterAudioStateInit(state);
	filterIirExchangeInit(&exchange);

	for (phase_index = 0; phase_index < num_phases; phase_index++)
	{
		const FilterSettings &settings = phases[phase_index].settings;
		const int preview_mask = filterPreviewMaskFor(phases[phase_index].selection, settings);
		const bool filter_on = filterAudibleMaskFor(settings) != 0 || preview_mask != 0;
		FilterIirParams params;

		filterDesignParams(sample_rate, slope_db, settings, reset_serial, &params);
		filterIirExchangePublish(&exchange, &params);

		for (buffer_index = 0; buffer_index < buffers_per_phase; buffer_index++)
		{
			for (n = 0; n < buffer_frames; n++, t++)
			{
				const double time = (double)t / (double)sample_rate;
				const float value = (float)(0.2 * sin(two_pi * 50.0 * time) + 0.2 * sin(two_pi * 1000.0 * time) +
					0.2 * sin(two_pi * 17000.0 * time));

				for (channel = 0; channel < num_channels; channel++)
				{
					buffer[(size_t)n * num_channels + channel] = value;
				}
				if (stereo_bass)
				{
					buffer[(size_t)n * num_channels] += (float)(0.3 * sin(two_pi * 60.0 * time));
				}
			}

			filterProcessBuffer(state, &exchange, sample_rate, reset_serial, 0u, buffer.data(), buffer_frames,
				num_channels, preview_mask, [state, filter_on, sample_rate, reset_serial]() {
					return filterDesiredSourceFor(state, filter_on, sample_rate, reset_serial);
				});
			hash = goldenHashBytes(hash, buffer.data(), buffer.size() * sizeof(float));
		}
	}

	delete state;
	return hash;
}

static void testGoldenOutputUnchanged()
{
	const float high_off = (float)DfxDsp::HIGH_CUT_OFF_HZ;
	const GoldenPhase phases[] = {
		{ { true, true, 40.0f, false, high_off, false, 0.0f }, DfxDsp::PreviewNone },
		{ { true, true, 40.0f, true, 16000.0f, false, 0.0f }, DfxDsp::PreviewNone },
		{ { true, true, 40.0f, true, 16000.0f, true, 125.0f }, DfxDsp::PreviewNone },
		{ { true, true, 40.0f, true, 16000.0f, true, 125.0f }, DfxDsp::PreviewLowCut },
		{ { true, true, 40.0f, true, 16000.0f, true, 125.0f }, DfxDsp::PreviewHighCut },
		{ { true, true, 40.0f, true, 16000.0f, true, 125.0f }, DfxDsp::PreviewAll },
		{ { true, true, 40.0f, true, 16000.0f, true, 125.0f }, DfxDsp::PreviewNone },
		{ { false, true, 40.0f, true, 16000.0f, true, 125.0f }, DfxDsp::PreviewNone },
	};
	const uint64_t hash = goldenHashFor(phases, sizeof(phases) / sizeof(phases[0]), 24, false);

	printf("golden hash 0x%016llxull\n", (unsigned long long)hash);
	CHECK(hash == kGoldenHash, "testGoldenOutputUnchanged: hash 0x%016llx", (unsigned long long)hash);
}

// The same check with the steepest slope (48 dB/oct; the first script covers 24) and a left-only bass tone, so the
// stereo to mono stage and its preview change the output too (the first script's input is identical on both channels,
// which the mono stage passes untouched).
static const uint64_t kGoldenStereoHash = 0x79b62517e1f19157ull;

static void testGoldenStereoOutputUnchanged()
{
	const int low = DfxDsp::PreviewLowCut, high = DfxDsp::PreviewHighCut, mono = DfxDsp::PreviewMonoBass;
	const GoldenPhase phases[] = {
		{ { true, false, 40.0f, false, 16000.0f, true, 125.0f }, DfxDsp::PreviewNone },
		{ { true, true, 40.0f, true, 16000.0f, true, 125.0f }, DfxDsp::PreviewNone },
		{ { true, true, 40.0f, true, 16000.0f, true, 125.0f }, mono },
		{ { true, true, 40.0f, true, 16000.0f, true, 125.0f }, low | mono },
		{ { true, true, 40.0f, true, 16000.0f, true, 125.0f }, high },
		{ { true, true, 40.0f, true, 16000.0f, true, 125.0f }, DfxDsp::PreviewAll },
		{ { true, true, 40.0f, true, 16000.0f, false, 125.0f }, DfxDsp::PreviewNone },
		{ { true, true, 40.0f, true, 16000.0f, true, 80.0f }, DfxDsp::PreviewNone },
		{ { false, true, 40.0f, true, 16000.0f, true, 80.0f }, DfxDsp::PreviewNone },
	};
	const uint64_t hash = goldenHashFor(phases, sizeof(phases) / sizeof(phases[0]), 48, true);

	printf("golden stereo hash 0x%016llxull\n", (unsigned long long)hash);
	CHECK(hash == kGoldenStereoHash, "testGoldenStereoOutputUnchanged: hash 0x%016llx", (unsigned long long)hash);
}

int main()
{
	testFilterResponse();
	testFilterIirHandoff();
	testFilterPreviewLeveller();
	testFilterPreviewPeakCapLinear();
	testFilterPreviewLevellerResetAndRescale();
	testFilterDesiredSource();
	testFilterTransitions();
	testFilterChainSteady();
	testFilterToggleKeepsLowCut();
	testFilterPreviewSwitch();
	testFilterPreviewSwitchNoBurst();
	testFilterPreviewNoLeak();
	testFilterExtraChannelsFollowPreview();
	testFilterAudibleToggle();
	testFilterPreviewAvailability();
	testSetterRules();
	testChangeNeedsReset();
	testGoldenOutputUnchanged();
	testGoldenStereoOutputUnchanged();

	printf("%d checks, %d failures\n", g_checks, g_failures);
	return (g_failures == 0) ? 0 : 1;
}
