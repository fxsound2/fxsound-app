// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../AudioAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

namespace fxanalysis::test
{
using Check = std::function<void(const char*, bool)>;
inline constexpr double pi = 3.14159265358979323846;
struct Frames final : SpectrumSink
{
    std::vector<SpectrumFrame> frames;
    void onSpectrum(const SpectrumFrame& f) override { frames.push_back(f); }
};
inline std::vector<float> tone(int count, double frequency, float rightGain = 1.0f, double sampleRate = 48000.0)
{
    std::vector<float> pcm(count * 2);
    for (int i = 0; i < count; ++i)
    {
        pcm[i * 2] = 0.25f * static_cast<float>(std::sin(2.0 * pi * frequency * i / sampleRate));
        pcm[i * 2 + 1] = pcm[i * 2] * rightGain;
    }
    return pcm;
}

struct NumericalMetrics
{
    double dftError, sweepStepDb, highResSweepStepDb, sweepStepHz, minimumSweepPeakDb;
};
void runMeterTests(const Check& check);
void runDynamicsTests(const Check& check);
void runStreamTests(const Check& check);
void runMicrophoneInputTests(const Check& check);
}
