// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <array>
#include <complex>
#include <cstdint>
#include <vector>
#include "BarDynamics.h"
#include "FrequencyScale.h"

namespace fxanalysis
{
constexpr int displayBandCount = 100;

struct BandSettings
{
    static constexpr double sigmaBandSpacing = 0.60;
    static constexpr double minimumSigmaBins = 0.75;
    static constexpr double supportSigma = 4.0;
    static constexpr float contrastGain = 0.75f; // 0: bypass; positive: accentuate local peaks.
};

using BandLevels = std::array<float, displayBandCount>;
// Finite nonnegative heights. Gain is finite; zero gives an exact bypass.
// Uses a five-band local mean; returns nonnegative levels, retaining headroom above 1.
BandLevels enhanceSpectralContrast(const BandLevels& levels, float gain = BandSettings::contrastGain) noexcept;

struct AnalyzerConfig
{
    double sampleRate = 48000.0;
    int channels = 2;
    int fftSize = 2048;
    int hopSize = 512;
};

enum class AnalysisResult { ok, invalidConfiguration, notPrepared, invalidBlock, nonFiniteInput };
const char* describe(AnalysisResult result) noexcept;

struct SpectrumFrame
{
    std::uint64_t firstSample = 0; // Start of the observation window, not delivery time.
    double sampleRate = 0.0;
    int fftSize = 0;
    int hopSize = 0;
    int channels = 0;
    std::vector<std::complex<float>> left;
    std::vector<std::complex<float>> right;
    std::vector<float> power; // Mean channel power; no L+R phase cancellation.
    std::array<float, displayBandCount> bandDb{};
    std::array<float, displayBandCount> bandHz{};
    std::array<float, displayBandCount + 1> bandEdgesHz{};
    BandLevels bandFloorDb{}; // Per-band display floor; raw bandDb remains unmodified.
    BandLevels bandLevelLeft{}, bandLevelRight{}; // Order-4 amplitude levels [0,1], before spectral contrast.
    std::array<float, displayBandCount> barLeft{}, barRight{}; // Attack/direct blend with stereo contrast [0,1].
    double centreTimeSeconds() const noexcept;
};

class SpectrumSink
{
public:
    virtual ~SpectrumSink() = default;
    // Synchronous, on the caller's thread. The reference is valid only during this call.
    // Copy what is needed by another thread. Sinks must not throw.
    virtual void onSpectrum(const SpectrumFrame& frame) = 0;
};

// Pure, single-threaded C++17 calculation. No windows, files, devices, threads or globals.
// prepare() owns allocations; processBlock() never changes the supplied PCM.
// The caller supplies continuity in sample units and owns scheduling / diagnostics.
class AudioAnalyzer
{
public:
    AnalysisResult prepare(const AnalyzerConfig& config);
    void reset() noexcept;
    AnalysisResult processBlock(const float* interleaved, int frameCount,
                                std::uint64_t firstSample, SpectrumSink& sink);
    const AnalyzerConfig& config() const noexcept { return config_; }

private:
    struct BandKernel
    {
        int firstBin = 0;
        std::vector<float> weights; // Non-negative, sum to one; prepared once per format.
    };
    void transform(std::vector<std::complex<float>>& data) const noexcept;
    void emit(SpectrumSink& sink);
    AnalyzerConfig config_;
    bool prepared_ = false;
    bool havePosition_ = false;
    std::uint64_t expectedSample_ = 0;
    int writeIndex_ = 0;
    int buffered_ = 0;
    int sinceLastFrame_ = 0;
    float windowSum_ = 0.0f;
    BarCoefficients barCoefficients_;
    std::array<BarDynamics, displayBandCount> barDynamics_;
    std::array<BandKernel, displayBandCount> bandKernels_;
    std::vector<float> window_, ringLeft_, ringRight_;
    std::vector<int> bitReverse_;
    std::vector<std::complex<float>> roots_, fftLeft_, fftRight_;
    SpectrumFrame frame_;
};
}
