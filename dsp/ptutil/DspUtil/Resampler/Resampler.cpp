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
#include <string.h>
#include <algorithm>

#include "Resampler.h"

namespace
{
	constexpr int kTapsPerPhase = 128;
	constexpr double kKaiserBeta = 8.96;
	constexpr double kPassbandEdge = 0.907;
	constexpr double kPi = 3.14159265358979323846;

	double besselI0(double x)
	{
		double sum = 1.0;
		double term = 1.0;

		for (int k = 1; term > 1e-12 * sum; k++)
		{
			double ratio = x / (2.0 * k);
			term *= ratio * ratio;
			sum += term;
		}

		return sum;
	}

	float dotProduct(const float *coeffs, const float *samples, int count)
	{
		float acc0 = 0.0f;
		float acc1 = 0.0f;
		float acc2 = 0.0f;
		float acc3 = 0.0f;

		for (int i = 0; i < count; i += 4)
		{
			acc0 += coeffs[i] * samples[i];
			acc1 += coeffs[i + 1] * samples[i + 1];
			acc2 += coeffs[i + 2] * samples[i + 2];
			acc3 += coeffs[i + 3] * samples[i + 3];
		}

		return (acc0 + acc1) + (acc2 + acc3);
	}
}

Resampler::Resampler(int channel_capacity, int factor_capacity, int frame_capacity)
	: max_channels_(channel_capacity), max_factor_(factor_capacity), max_frames_(frame_capacity), factor_(1), channels_(1)
{
	int max_taps = max_factor_ * kTapsPerPhase;

	prototype_.resize(max_taps);
	up_phases_.resize(max_taps);
	down_kernel_.resize(max_taps);
	up_history_.resize((kTapsPerPhase - 1) * max_channels_);
	down_history_.resize((max_taps - 1) * max_channels_);
	work_.resize(max_taps - 1 + max_frames_);
}

bool Resampler::configure(int new_factor, int new_channels)
{
	if ((new_factor < 1) || (new_factor > max_factor_) || (new_channels < 1) || (new_channels > max_channels_))
		return false;

	if ((new_factor == factor_) && (new_channels == channels_))
		return true;

	factor_ = new_factor;
	channels_ = new_channels;
	designKernel();
	reset();

	return true;
}

void Resampler::reset()
{
	std::fill(up_history_.begin(), up_history_.end(), 0.0f);
	std::fill(down_history_.begin(), down_history_.end(), 0.0f);
}

/*
 * Kaiser windowed sinc lowpass at the lower Nyquist, expressed at the higher
 * rate. With the cutoff mid transition the passband is flat to 90.7% of the
 * lower Nyquist and the stopband starts at Nyquist, about 90 dB down.
 */
void Resampler::designKernel()
{
	int taps = factor_ * kTapsPerPhase;
	double center = (taps - 1) / 2.0;
	double cutoff = (kPassbandEdge + 1.0) * 0.25 / factor_;
	double window_norm = besselI0(kKaiserBeta);
	double sum = 0.0;

	for (int n = 0; n < taps; n++)
	{
		double t = n - center;
		double sinc = (t == 0.0) ? 2.0 * cutoff : sin(2.0 * kPi * cutoff * t) / (kPi * t);
		double r = 2.0 * t / (taps - 1);
		double window = besselI0(kKaiserBeta * sqrt(1.0 - r * r)) / window_norm;

		prototype_[n] = sinc * window;
		sum += prototype_[n];
	}

	for (int n = 0; n < taps; n++)
		prototype_[n] /= sum;

	for (int q = 0; q < taps; q++)
		down_kernel_[q] = (float)prototype_[taps - 1 - q];

	for (int phase = 0; phase < factor_; phase++)
		for (int q = 0; q < kTapsPerPhase; q++)
			up_phases_[phase * kTapsPerPhase + q] = (float)(factor_ * prototype_[(kTapsPerPhase - 1 - q) * factor_ + phase]);
}

/*
 * Lays one channel out in work_ as [history][new samples], so every input
 * sample is read before any output is written and in-place calls are safe.
 */
void Resampler::loadChannel(const float *input, int ch, int frames, const float *history, int history_frames)
{
	memcpy(work_.data(), history, history_frames * sizeof(float));

	for (int i = 0; i < frames; i++)
		work_[history_frames + i] = input[i * channels_ + ch];
}

void Resampler::interpolate(const float *input, int input_frames, float *output)
{
	if (input_frames > max_frames_)
		input_frames = max_frames_;

	if (factor_ == 1)
	{
		if (input != output)
			memcpy(output, input, input_frames * channels_ * sizeof(float));
		return;
	}

	int history_frames = kTapsPerPhase - 1;

	for (int ch = 0; ch < channels_; ch++)
	{
		float *history = &up_history_[ch * history_frames];

		loadChannel(input, ch, input_frames, history, history_frames);
		memcpy(history, &work_[input_frames], history_frames * sizeof(float));

		for (int i = 0; i < input_frames; i++)
		{
			const float *samples = &work_[i];

			for (int phase = 0; phase < factor_; phase++)
				output[(i * factor_ + phase) * channels_ + ch] = dotProduct(&up_phases_[phase * kTapsPerPhase], samples, kTapsPerPhase);
		}
	}
}

void Resampler::decimate(const float *input, int input_frames, float *output)
{
	if (input_frames > max_frames_)
		input_frames = max_frames_;

	if (factor_ == 1)
	{
		if (input != output)
			memcpy(output, input, input_frames * channels_ * sizeof(float));
		return;
	}

	int taps = factor_ * kTapsPerPhase;
	int history_frames = taps - 1;
	int output_frames = input_frames / factor_;
	int used_frames = output_frames * factor_;

	for (int ch = 0; ch < channels_; ch++)
	{
		float *history = &down_history_[ch * history_frames];

		loadChannel(input, ch, used_frames, history, history_frames);
		memcpy(history, &work_[used_frames], history_frames * sizeof(float));

		for (int j = 0; j < output_frames; j++)
			output[j * channels_ + ch] = dotProduct(down_kernel_.data(), &work_[j * factor_], taps);
	}
}
