// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "PitchMotion.h"
#include "HarmonicCombSnapshot.h"
#include "../../BarDynamics.h"
#include <algorithm>
#include <cmath>

namespace fxanalysis::experimental
{

// Fixed linear amplitude reference, anchored at 1 kHz. Reuse the same frequency
// floor as the 100 bars and History, without their logarithmic height/dynamics.
// Used for pursuit ranking and rendering. Subtraction always uses the original
// unweighted model coefficients; gain/linear-dB clipping never enters the fit.
inline constexpr double referenceAnchorHz = 1000.0;
inline constexpr double amplitudeDecibelFactor = 20.0;
inline double harmonicReferenceGain(double hz, bool highResReference) noexcept
{
    return highResReference ? std::pow(10.0,
        (displayFloorDb(referenceAnchorHz) - displayFloorDb(hz)) / amplitudeDecibelFactor) : 1.0;
}

// A plateau is represented once, at its first sample. Positive endpoints are
// maxima only when the inward neighbor is lower; a wholly flat curve has none.
inline bool harmonicLocalMaximum(const std::array<float, harmonicProfileCapacity>& values,
    int count, int index) noexcept
{
    if (count < 2 || count > harmonicProfileCapacity || index < 0 || index >= count
        || !std::isfinite(values[index]) || values[index] <= 0.0f
        || (index > 0 && !(values[index] > values[index - 1]))) return false;
    int next = index + 1;
    while (next < count && values[next] == values[index]) ++next;
    return next < count ? values[index] > values[next] : index > 0;
}

inline double harmonicDisplayHeight(double amplitude, double gain, bool logarithmic,
    double rangeDb) noexcept
{
    if (!std::isfinite(amplitude) || amplitude <= 0.0 || !std::isfinite(gain) || gain <= 0.0)
        return 0.0;
    if (!logarithmic) return std::clamp(amplitude * gain, 0.0, 1.0);
    if (!std::isfinite(rangeDb) || rangeDb <= 0.0) return 0.0;
    return std::clamp(1.0 + amplitudeDecibelFactor * (std::log10(amplitude) + std::log10(gain)) / rangeDb, 0.0, 1.0);
}

// One frame -> bounded greedy selection -> joint nonnegative fit of <=4 combs.
// The previous one-family partial cancellation remains an explicit comparison mode.
// Pure bounded worker calculation; no allocations in prepare/process/reset.
// Magnitude templates use the actual periodic-Hann FFT response, main lobe only.
// Positive-frequency magnitude addition is approximate for overlapping partials.
class HarmonicCombProfile
{
public:
    PitchMotionResult prepare(const PitchMotionSettings& settings = {}) noexcept;
    void reset() noexcept;
    PitchMotionResult process(const SpectrumFrame& frame) noexcept;
    const HarmonicCombSnapshot& snapshot() const noexcept { return snapshot_; }
private:
    void beginSnapshot() noexcept;
    static constexpr double hannHalfWidthBins = 2.0;
    static constexpr double bassWideningCornerHz = 300.0;
    static constexpr double bassWideningBoost = 0.5;
    static constexpr int maximumBinsPerHarmonic =
        static_cast<int>(2 * hannHalfWidthBins * (1 + harmonicMaximumWideningPercent / 100.0)) + 1;
    static constexpr int maximumBinsPerPartial = static_cast<int>(2 * hannHalfWidthBins
        * (1 + harmonicMaximumWideningPercent / 100.0 * (1 + bassWideningBoost))) + 1;
    // Only the fundamental needs the extra bin; higher teeth keep their original bound.
    static constexpr int templateCapacity = maximumBinsPerPartial
        + (harmonicMaximumHarmonics - 1) * maximumBinsPerHarmonic;
    struct Template
    {
        std::array<std::uint16_t, templateCapacity> bins{};
        std::array<float, templateCapacity> values{};
        int count = 0;
        double normSquared = 0.0;
        double wideningPercent = 0.0;
    };
    struct Partial
    {
        std::array<int, maximumBinsPerPartial> bins{};
        std::array<double, maximumBinsPerPartial> values{};
        int count = 0;
        double amplitude = 0.0, normSquared = 0.0;
    };
    static double lobe(double offset, int fftSize) noexcept;
    static double partialWideningScale(double frequency, int harmonic, double wideningPercent) noexcept;
    static Partial partial(double frequency, double binHz, int fftSize,
        double wideningPercent = 0.0, int harmonic = 1) noexcept;
    void buildTemplate(const SpectrumFrame& frame, double frequency, Template& shape,
        bool refineSupport = false) const noexcept;
    static double overlap(const Template& a, const Template& b) noexcept;
    bool similarToSelected(const Template& shape, double frequency, int count) const noexcept;
    void jointFit(int count, double inputEnergy, std::array<double, harmonicMaximumComponents>& amplitudes) noexcept;
    void extractComponents(const SpectrumFrame& frame, double inputEnergy) noexcept;
    double reconstruct(const SpectrumFrame& frame, int count,
        const std::array<double, harmonicMaximumComponents>& amplitudes) noexcept;
    void prepareTemplates(const SpectrumFrame& frame) noexcept;
    double amplitude(const Template& shape, const std::array<double, AnalysisLimits::maximumFftSize / 2 + 1>& data) const noexcept;
    void subtractFamily(const SpectrumFrame& frame, int selected) noexcept;
    double bassSupport(const SpectrumFrame& frame, double frequency, double projection) const noexcept;
    double selectionGain(const SpectrumFrame& frame, double frequency, const Template& shape) const noexcept;
    PitchMotionResult fail(PitchMotionResult result) noexcept;
    PitchMotionSettings settings_;
    HarmonicCombSnapshot snapshot_;
    bool prepared_ = false, cacheValid_ = false, haveTime_ = false;
    double cachedRate_ = 0.0, lastTime_ = 0.0;
    int cachedFftSize_ = 0, count_ = 0;
    std::uint64_t lastFirstSample_ = 0;
    std::uint64_t generation_ = 0;
    int lastHop_ = 0, lastChannels_ = 0;
    std::array<float, harmonicProfileCapacity> frequencies_{};
    std::array<Template, harmonicProfileCapacity> templates_; // Each Template initializes its own fields.
    std::array<Template, harmonicMaximumComponents> selectedTemplates_{};
    std::array<double, harmonicMaximumComponents> selectedFrequencies_{};
    std::array<double, harmonicProfileCapacity> supportWeights_{};
    std::array<double, AnalysisLimits::maximumFftSize / 2 + 1> input_{}, residual_{}, fitted_{};
};

// Display-only stamps: no identities, prediction or change to fitted amplitudes.
// Fixed capacity; repeated reads of one publication never refresh a stamp.
class HarmonicMarkerTrail
{
public:
    // Storage budget per simultaneous component, with oldest-stamp eviction at capacity.
    static constexpr int stampsPerComponent = 96;
    static constexpr int capacity = stampsPerComponent * harmonicMaximumComponents;
    static constexpr int visibleHalfLives = 5;
    static constexpr double centsPerOctave = 100.0 * semitonesPerOctave;
    static constexpr double logAmplitudeCellWidth = 0.02;
    struct Marker { double frequencyHz = 0.0, amplitude = 0.0, time = 0.0; };
    void clear() noexcept { count_ = 0; haveFrame_ = false; }
    void expire(double now, double halfLife) noexcept
    {
        if (!std::isfinite(now) || !std::isfinite(halfLife) || halfLife <= 0.0) { clear(); return; }
        int kept = 0;
        for (int i = 0; i < count_; ++i)
            if (now >= markers_[i].time && now - markers_[i].time < visibleHalfLives * halfLife)
                markers_[kept++] = markers_[i];
        count_ = kept;
    }
    void observe(const HarmonicCombSnapshot& snapshot, double now) noexcept
    {
        expire(now, snapshot.trailHalfLifeSeconds);
        if (snapshot.trailHalfLifeSeconds <= 0.0 || !std::isfinite(now)) return;
        if (!std::isfinite(snapshot.centreTimeSeconds)) { clear(); return; }
        if (haveFrame_ && (snapshot.generation != generation_ || snapshot.centreTimeSeconds < sourceTime_)) clear();
        if (haveFrame_ && snapshot.centreTimeSeconds == sourceTime_) return;
        generation_ = snapshot.generation; sourceTime_ = snapshot.centreTimeSeconds; haveFrame_ = true;
        if (!snapshot.hasEstimate) return;
        for (int i = 0; i < std::clamp(snapshot.componentCount, 0, harmonicMaximumComponents); ++i)
        {
            const auto& c = snapshot.components[i];
            if (!std::isfinite(c.frequencyHz) || c.frequencyHz <= 0.0
                || !std::isfinite(c.displayAmplitude) || c.displayAmplitude <= 0.0) continue;
            // Same 1-cent / 2%-amplitude display cell, independent of extraction rank.
            int slot = 0;
            for (; slot < count_; ++slot)
                if (std::llround(centsPerOctave * std::log2(markers_[slot].frequencyHz)) == std::llround(centsPerOctave * std::log2(c.frequencyHz))
                    && std::llround(std::log(markers_[slot].amplitude) / logAmplitudeCellWidth) == std::llround(std::log(c.displayAmplitude) / logAmplitudeCellWidth)) break;
            if (slot == count_)
            {
                if (count_ < capacity) ++count_;
                else
                {
                    slot = 0;
                    for (int j = 1; j < count_; ++j) if (markers_[j].time < markers_[slot].time) slot = j;
                }
            }
            markers_[slot] = {c.frequencyHz, c.displayAmplitude, now};
        }
    }
    static double opacity(const Marker& marker, double now, double halfLife) noexcept
    { return halfLife > 0.0 ? std::exp2(-(std::max)(0.0, now - marker.time) / halfLife) : 0.0; }
    int count() const noexcept { return count_; }
    const Marker& marker(int index) const noexcept { return markers_[index]; }
private:
    std::array<Marker, capacity> markers_{};
    int count_ = 0;
    bool haveFrame_ = false;
    double sourceTime_ = 0.0;
    std::uint64_t generation_ = 0;
};
}
