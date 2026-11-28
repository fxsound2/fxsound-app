/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
    Sylvain Petitgrand (2026)

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
#pragma once
#include <algorithm>
#include <cmath>

namespace fxanalysis
{
struct FrequencySettings
{
    static constexpr double minimumHz = 30.0;
    static constexpr double maximumHz = 20000.0;
    static constexpr double offsetHz = 600.0; // 0: logarithmic; larger: closer to linear.
    static constexpr double trebleStartHz = 2300.0;
    static constexpr double trebleDisplayFraction = 0.20; // 1: disable compression.
    static constexpr double transitionOctaves = 1.50; // Measured on frequency + offset.
};
static_assert(FrequencySettings::offsetHz >= 0.0 && FrequencySettings::trebleStartHz > FrequencySettings::minimumHz
              && FrequencySettings::trebleDisplayFraction > 0.0 && FrequencySettings::trebleDisplayFraction <= 1.0
              && FrequencySettings::transitionOctaves > 0.0, "Invalid frequency display settings");

// Frequency mapping for the spectrum display. No allocation, UI, or sample processing.
// Inputs: finite maximum > minimumHz, offset >= 0, and treble fraction in (0,1].
class FrequencyScale
{
public:
    explicit FrequencyScale(double maximumHz, double offsetHz = FrequencySettings::offsetHz,
                            double trebleFraction = FrequencySettings::trebleDisplayFraction) noexcept
        : maximum_(maximumHz), offset_(offsetHz),
          logRange_(std::log1p((maximum_ - FrequencySettings::minimumHz) / (FrequencySettings::minimumHz + offset_)))
    {
        if (maximum_ <= FrequencySettings::trebleStartHz) return;
        knee_ = basePosition(FrequencySettings::trebleStartHz);
        const double target = 1.0 - trebleFraction;
        // Never expand the treble to reach its target share on a shorter range.
        if (target <= knee_) return;
        halfWidth_ = (std::min)({0.5 * FrequencySettings::transitionOctaves * std::log(2.0) / logRange_,
                                knee_ * 0.5, (1.0 - knee_) * 0.5});
        // Limit smoothing if an extreme target would require a negative slope.
        halfWidth_ = (std::min)(halfWidth_, 0.5 * knee_ * (1.0 - target) / 0.1875);
        const double atKnee = 0.1875 * halfWidth_; // Integral of smoothstep up to its midpoint.
        trebleDensity_ = (knee_ * (1.0 - target) - atKnee) / (target * (1.0 - knee_) - atKnee);
        total_ = knee_ + trebleDensity_ * (1.0 - knee_);
    }

    double position(double frequencyHz) const noexcept
    {
        if (frequencyHz <= FrequencySettings::minimumHz) return 0.0;
        if (frequencyHz >= maximum_) return 1.0;
        return warp(basePosition(frequencyHz));
    }

    // Inversion is used only when preparing band coefficients.
    double frequency(double position) const noexcept
    {
        if (position <= 0.0) return FrequencySettings::minimumHz;
        if (position >= 1.0) return maximum_;
        double u = position;
        if (trebleDensity_ != 1.0)
        {
            double low = 0.0, high = 1.0;
            for (int i = 0; i < 48; ++i)
            {
                u = (low + high) * 0.5;
                if (warp(u) < position) low = u; else high = u;
            }
            u = (low + high) * 0.5;
        }
        return FrequencySettings::minimumHz + (FrequencySettings::minimumHz + offset_) * std::expm1(u * logRange_);
    }

private:
    double basePosition(double hz) const noexcept
    {
        return std::log1p((hz - FrequencySettings::minimumHz) / (FrequencySettings::minimumHz + offset_)) / logRange_;
    }
    double warp(double u) const noexcept
    {
        if (trebleDensity_ == 1.0) return u;
        double integral = 0.0;
        if (u >= knee_ + halfWidth_) integral = u - knee_;
        else if (u > knee_ - halfWidth_)
        {
            const double t = (u - knee_ + halfWidth_) / (2.0 * halfWidth_);
            integral = 2.0 * halfWidth_ * t * t * t * (1.0 - 0.5 * t);
        }
        return (u + (trebleDensity_ - 1.0) * integral) / total_;
    }
    double maximum_, offset_, logRange_;
    double knee_ = 0.0, halfWidth_ = 0.0, trebleDensity_ = 1.0, total_ = 1.0;
};
}
