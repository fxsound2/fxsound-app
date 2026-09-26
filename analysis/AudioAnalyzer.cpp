// SPDX-License-Identifier: AGPL-3.0-or-later
#include "AudioAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace fxanalysis
{
namespace { constexpr double pi = 3.14159265358979323846; }

BandLevels enhanceSpectralContrast(const BandLevels& levels, float gain) noexcept
{
    if (gain == 0.0f) return levels;
    BandLevels enhanced{};
    constexpr float weights[] = {1.0f, 4.0f, 6.0f, 4.0f, 1.0f};
    for (int b = 0; b < displayBandCount; ++b)
    {
        float localMean = 0.0f;
        for (int offset = -2; offset <= 2; ++offset)
        {
            // Extend edge values, never wrap the highest frequencies to the bass.
            const int neighbour = std::clamp(b + offset, 0, displayBandCount - 1);
            localMean += weights[offset + 2] * levels[neighbour];
        }
        localMean *= 1.0f / 16.0f;
        enhanced[b] = std::max(0.0f, levels[b] + gain * (levels[b] - localMean));
    }
    return enhanced;
}

const char* describe(AnalysisResult result) noexcept
{
    switch (result)
    {
    case AnalysisResult::ok: return "OK";
    case AnalysisResult::invalidConfiguration: return "Invalid sample rate, channels, FFT or hop size";
    case AnalysisResult::notPrepared: return "Analyzer has not been prepared";
    case AnalysisResult::invalidBlock: return "Invalid PCM pointer, frame count or sample position";
    case AnalysisResult::nonFiniteInput: return "PCM contains a non-finite sample";
    }
    return "Unknown analysis status";
}

double SpectrumFrame::centreTimeSeconds() const noexcept
{
    return sampleRate > 0.0 ? (static_cast<double>(firstSample) + fftSize * 0.5) / sampleRate : 0.0;
}

AnalysisResult AudioAnalyzer::prepare(const AnalyzerConfig& config)
{
    if (!std::isfinite(config.sampleRate) || config.sampleRate < 8000.0 || config.sampleRate > 384000.0
        || (config.channels != 1 && config.channels != 2)
        || config.fftSize < 64 || config.fftSize > 32768 || (config.fftSize & (config.fftSize - 1)) != 0
        || config.hopSize < 1 || config.hopSize > config.fftSize)
        return AnalysisResult::invalidConfiguration;

    prepared_ = false;
    config_ = config;
    barCoefficients_ = BarCoefficients::forHop(config.hopSize / config.sampleRate);
    const int n = config.fftSize;
    window_.resize(n);
    ringLeft_.resize(n);
    ringRight_.resize(n);
    fftLeft_.resize(n);
    fftRight_.resize(n);
    bitReverse_.resize(n);
    roots_.resize(n / 2);
    windowSum_ = 0.0f;
    int bits = 0;
    for (int size = n; size > 1; size >>= 1) ++bits;
    for (int i = 0; i < n; ++i)
    {
        window_[i] = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * pi * i / n));
        windowSum_ += window_[i];
        int reverse = 0;
        int value = i;
        for (int b = 0; b < bits; ++b) { reverse = (reverse << 1) | (value & 1); value >>= 1; }
        bitReverse_[i] = reverse;
        if (i < n / 2)
            roots_[i] = std::polar(1.0f, static_cast<float>(-2.0 * pi * i / n));
    }
    frame_.left.resize(n / 2 + 1);
    frame_.right.resize(n / 2 + 1);
    frame_.power.resize(n / 2 + 1);
    frame_.sampleRate = config.sampleRate;
    frame_.channels = config.channels;
    frame_.fftSize = n;
    frame_.hopSize = config.hopSize;
    const double maximum = std::min(FrequencySettings::maximumHz, config.sampleRate * 0.5);
    const FrequencyScale scale(maximum);
    std::array<double, displayBandCount + 1> edges;
    for (int b = 0; b <= displayBandCount; ++b)
    {
        edges[b] = scale.frequency(b / static_cast<double>(displayBandCount));
        frame_.bandEdgesHz[b] = static_cast<float>(edges[b]);
    }
    const double binHz = config.sampleRate / n;
    for (int b = 0; b < displayBandCount; ++b)
    {
        const double centre = scale.frequency((b + 0.5) / displayBandCount);
        frame_.bandHz[b] = static_cast<float>(centre);
        frame_.bandFloorDb[b] = displayFloorDb(centre);
        // Gaussian in frequency, sized from the actual mapped band edges. Quadrature
        // combines the display bandwidth and FFT resolution without a hard switch.
        const double sigma = std::hypot(BandSettings::sigmaBandSpacing * (edges[b + 1] - edges[b]),
                                        BandSettings::minimumSigmaBins * binHz);
        const double radius = BandSettings::supportSigma * sigma;
        auto& kernel = bandKernels_[b];
        kernel.firstBin = std::max(0, static_cast<int>(std::ceil((centre - radius) / binHz)));
        const int lastBin = std::min(n / 2, static_cast<int>(std::floor((centre + radius) / binHz)));
        kernel.weights.resize(lastBin - kernel.firstBin + 1);
        double sum = 0.0;
        for (int k = kernel.firstBin; k <= lastBin; ++k)
        {
            const double distance = (k * binHz - centre) / sigma;
            const double weight = std::exp(-0.5 * distance * distance);
            kernel.weights[k - kernel.firstBin] = static_cast<float>(weight);
            sum += weight;
        }
        for (auto& weight : kernel.weights) weight = static_cast<float>(weight / sum);
    }
    reset();
    prepared_ = true;
    return AnalysisResult::ok;
}

void AudioAnalyzer::reset() noexcept
{
    havePosition_ = false;
    writeIndex_ = buffered_ = sinceLastFrame_ = 0;
    expectedSample_ = 0;
    for (auto& dynamics : barDynamics_) dynamics.reset();
}

AnalysisResult AudioAnalyzer::processBlock(const float* input, int count,
                                          std::uint64_t first, SpectrumSink& sink)
{
    if (!prepared_) return AnalysisResult::notPrepared;
    if (count < 0 || (count > 0 && input == nullptr)
        || count > std::numeric_limits<int>::max() / config_.channels
        || first > std::numeric_limits<std::uint64_t>::max() - static_cast<std::uint64_t>(count))
        return AnalysisResult::invalidBlock;
    if (count == 0) return AnalysisResult::ok;
    for (int i = 0; i < count * config_.channels; ++i)
        if (!std::isfinite(input[i])) { reset(); return AnalysisResult::nonFiniteInput; }

    if (havePosition_ && first != expectedSample_) reset();
    havePosition_ = true;
    expectedSample_ = first;
    for (int i = 0; i < count; ++i)
    {
        ringLeft_[writeIndex_] = input[i * config_.channels];
        ringRight_[writeIndex_] = input[i * config_.channels + config_.channels - 1];
        writeIndex_ = (writeIndex_ + 1) % config_.fftSize;
        ++expectedSample_;
        ++sinceLastFrame_;
        if (buffered_ < config_.fftSize) ++buffered_;
        if (buffered_ == config_.fftSize && sinceLastFrame_ >= config_.hopSize)
        {
            emit(sink);
            sinceLastFrame_ = 0;
        }
    }
    return AnalysisResult::ok;
}

void AudioAnalyzer::transform(std::vector<std::complex<float>>& data) const noexcept
{
    const int n = config_.fftSize;
    for (int i = 0; i < n; ++i)
        if (i < bitReverse_[i]) std::swap(data[i], data[bitReverse_[i]]);
    for (int size = 2; size <= n; size *= 2)
        for (int start = 0; start < n; start += size)
            for (int j = 0; j < size / 2; ++j)
            {
                const auto odd = data[start + j + size / 2] * roots_[j * (n / size)];
                const auto even = data[start + j];
                data[start + j] = even + odd;
                data[start + j + size / 2] = even - odd;
            }
}

void AudioAnalyzer::emit(SpectrumSink& sink)
{
    const int n = config_.fftSize;
    for (int i = 0; i < n; ++i)
    {
        const int index = (writeIndex_ + i) % n;
        fftLeft_[i] = ringLeft_[index] * window_[i];
        fftRight_[i] = ringRight_[index] * window_[i];
    }
    transform(fftLeft_);
    transform(fftRight_);
    for (int k = 0; k <= n / 2; ++k)
    {
        const float scale = (k == 0 || k == n / 2 ? 1.0f : 2.0f) / windowSum_;
        frame_.left[k] = fftLeft_[k] * scale;
        frame_.right[k] = fftRight_[k] * scale;
        frame_.power[k] = 0.5f * (std::norm(frame_.left[k]) + std::norm(frame_.right[k]));
    }
    // The same precomputed smooth projection is used for every band and channel.
    // Keep mean power for reference views; HighRes uses the fourth amplitude
    // moment to emphasize strong bins without hard peak picking.
    for (int b = 0; b < displayBandCount; ++b)
    {
        const auto& kernel = bandKernels_[b];
        float leftPower = 0.0f, rightPower = 0.0f;
        double leftFourth = 0.0, rightFourth = 0.0;
        for (int i = 0; i < static_cast<int>(kernel.weights.size()); ++i)
        {
            const int k = kernel.firstBin + i;
            const double lp = std::norm(frame_.left[k]), rp = std::norm(frame_.right[k]);
            leftPower += kernel.weights[i] * static_cast<float>(lp);
            rightPower += kernel.weights[i] * static_cast<float>(rp);
            leftFourth += kernel.weights[i] * lp * lp;
            rightFourth += kernel.weights[i] * rp * rp;
        }
        const float power = 0.5f * (leftPower + rightPower);
        frame_.bandDb[b] = 10.0f * std::log10(std::max(power, 1.0e-12f));
        const float floor = frame_.bandFloorDb[b];
        const auto level = [floor](double fourthMoment)
        {
            // 20 log10((sum w |X|^4)^(1/4)) = 5 log10(sum w |X|^4).
            // Double accumulation retains range when squaring the FFT powers.
            const float db = static_cast<float>(5.0 * std::log10(std::max(fourthMoment, 1.0e-24)));
            return std::clamp((db - floor) / -floor, 0.0f, 1.0f);
        };
        frame_.bandLevelLeft[b] = level(leftFourth);
        frame_.bandLevelRight[b] = level(rightFourth);
    }
    // Read the complete current frame before enhancing either channel. Keep the
    // raw projection for the lab's reference views; only HighRes bars are enhanced.
    const auto enhancedLeft = enhanceSpectralContrast(frame_.bandLevelLeft);
    const auto enhancedRight = enhanceSpectralContrast(frame_.bandLevelRight);
    for (int b = 0; b < displayBandCount; ++b)
    {
        const auto bars = barDynamics_[b].process(enhancedLeft[b], enhancedRight[b], barCoefficients_);
        frame_.barLeft[b] = bars.left;
        frame_.barRight[b] = bars.right;
    }
    frame_.firstSample = expectedSample_ - static_cast<std::uint64_t>(n);
    sink.onSpectrum(frame_);
}
}
