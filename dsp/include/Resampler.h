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

#ifndef _RESAMPLER_H_
#define _RESAMPLER_H_

#include <vector>

/*
 * Integer factor polyphase FIR resampler for interleaved float audio, with
 * separate histories for interpolation and decimation. All memory is
 * allocated by the constructor, and input and output may be the same buffer.
 */
class Resampler
{
public:
	Resampler(int channel_capacity, int factor_capacity, int frame_capacity);

	bool configure(int new_factor, int new_channels);
	void reset();

	void interpolate(const float *input, int input_frames, float *output);
	void decimate(const float *input, int input_frames, float *output);

	int getFactor() const { return factor_; }
	int getChannels() const { return channels_; }
	int getMaxChannels() const { return max_channels_; }
	int getMaxFactor() const { return max_factor_; }
	int getMaxFrames() const { return max_frames_; }

private:
	void designKernel();
	void loadChannel(const float *input, int ch, int frames, const float *history, int history_frames);

	int max_channels_;
	int max_factor_;
	int max_frames_;
	int factor_;
	int channels_;
	std::vector<double> prototype_;
	std::vector<float> up_phases_;
	std::vector<float> down_kernel_;
	std::vector<float> up_history_;
	std::vector<float> down_history_;
	std::vector<float> work_;
};

#endif
