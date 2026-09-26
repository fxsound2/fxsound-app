// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>

namespace fxanalysis
{
// Display-only tuning. These values never alter the PCM or the complex spectra.
struct BarSettings
{
    static constexpr double decaySeconds = 0.070;
    static constexpr double peakRetentionSeconds = 0.030; // 0: bypass the rounded peak release.
    static constexpr float displayGain = 4.0f;
    static constexpr float directMix = 0.18f; // 0: attacks only; 1: unfiltered levels.
    static constexpr float currentChannelGain = 1.15f;
    static constexpr float oppositeChannelGain = 0.3f;
    static constexpr float floorDb = -48.0f;
    static constexpr float bassFloorLiftDb = 24.0f;
    static constexpr double floorCornerHz = 200.0;
    static constexpr float silenceThreshold = 1.0e-6f;
};
static_assert(BarSettings::directMix >= 0.0f && BarSettings::directMix <= 1.0f,
              "Direct mix must be between zero and one");
static_assert(BarSettings::decaySeconds > 0.0 && BarSettings::peakRetentionSeconds >= 0.0,
              "Decay must be positive and peak retention nonnegative");
static_assert(BarSettings::floorCornerHz > 0.0 && BarSettings::bassFloorLiftDb >= 0.0f
              && BarSettings::floorDb + BarSettings::bassFloorLiftDb < 0.0f,
              "Every display floor must remain below zero dB");

// For finite nonnegative frequencies. The high-frequency asymptote is floorDb.
inline float displayFloorDb(double frequencyHz) noexcept
{
    return BarSettings::floorDb + BarSettings::bassFloorLiftDb
        * static_cast<float>(BarSettings::floorCornerHz / (frequencyHz + BarSettings::floorCornerHz));
}

struct BarCoefficients
{
    float decay = 0.0f, peak = 0.0f, coupling = 0.0f;
    // Positive finite interval/decay, finite nonnegative peak retention.
    // Called once per format, never per bar. Optional times also serve lab experiments.
    static BarCoefficients forHop(double seconds, double decaySeconds = BarSettings::decaySeconds,
                                 double peakRetentionSeconds = BarSettings::peakRetentionSeconds) noexcept
    {
        const double d = decaySeconds, p = peakRetentionSeconds;
        const double decay = std::exp(-seconds / d);
        if (p == 0.0) return {static_cast<float>(decay), 0.0f, static_cast<float>(decay)};
        const double peak = std::exp(-seconds / p);
        // Exact coupling of an exponentially decaying input into the second pole.
        // Also supports equal time constants without division by zero.
        const double coupling = d == p ? seconds / d * decay : d / (d - p) * (decay - peak);
        return {static_cast<float>(decay), static_cast<float>(peak), static_cast<float>(coupling)};
    }
};

struct StereoBar { float left = 0.0f, right = 0.0f; };

// One instance per frequency band. Nonnegative inputs may exceed 1 after spectral contrast.
// Both release poles are advanced from the audio clock, with instant peak capture.
class BarDynamics
{
public:
    StereoBar process(float left, float right, const BarCoefficients& coefficients) noexcept
    {
        // Both separated levels use the original inputs. Remove negative levels
        // before detecting attacks, but keep headroom until the final display.
        const float separatedLeft = separate(left, right);
        const float separatedRight = separate(right, left);
        advance(separatedLeft, previous_.left, impulse_.left, held_.left, coefficients);
        advance(separatedRight, previous_.right, impulse_.right, held_.right, coefficients);
        return {display(blend(separatedLeft, held_.left)),
                display(blend(separatedRight, held_.right))};
    }
    void reset() noexcept { previous_ = {}; impulse_ = {}; held_ = {}; }

private:
    static float blend(float level, float impulse) noexcept
    {
        return BarSettings::directMix * level + (1.0f - BarSettings::directMix) * impulse;
    }
    static void advance(float input, float& previous, float& impulse, float& held,
                        const BarCoefficients& coefficients) noexcept
    {
        const float release = coefficients.peak * held + coefficients.coupling * impulse;
        impulse = (std::max)(0.0f, coefficients.decay * impulse + (std::max)(0.0f, input - previous));
        held = (std::max)(impulse, release);
        if (impulse < BarSettings::silenceThreshold) impulse = 0.0f;
        if (held < BarSettings::silenceThreshold) held = 0.0f;
        previous = input;
    }
    static float separate(float current, float opposite) noexcept
    {
        return (std::max)(0.0f, BarSettings::currentChannelGain * current
                              - BarSettings::oppositeChannelGain * opposite);
    }
    static float display(float level) noexcept
    {
        return std::clamp(BarSettings::displayGain * level, 0.0f, 1.0f);
    }
    StereoBar previous_, impulse_, held_;
};
}
