// SPDX-License-Identifier: AGPL-3.0-or-later
#include "WaterfallRaster.h"
#include <algorithm>
#include <cmath>

namespace fxanalysis
{
namespace
{
std::uint32_t shadeColour(std::uint32_t base, double light)
{
    if (light == 1.0) return base;
    std::uint32_t shaded = 0xff000000;
    for (int shift = 0; shift <= 16; shift += 8)
    {
        const double c = ((base >> shift) & 255) / 255.0;
        double linear = c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
        linear = (std::min)(1.0, linear * light);
        const double encoded = linear <= 0.0031308 ? 12.92 * linear : 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
        shaded |= static_cast<std::uint32_t>(std::lround(255 * encoded)) << shift;
    }
    return shaded;
}

constexpr float opaqueCarrierLevel = 0.22f;
constexpr int carrierShadowOpacity = 96;

std::uint32_t premultiply(std::uint32_t colour, std::uint32_t alpha)
{
    std::uint32_t result = alpha << 24;
    for (int shift = 0; shift <= 16; shift += 8)
        result |= ((((colour >> shift) & 255) * alpha + 127) / 255) << shift;
    return result;
}
}

std::uint32_t WaterfallRaster::colour(float value) noexcept
{
    // Dark silence, violet/blue, cyan/green, yellow/orange, red and pale peaks.
    constexpr std::uint32_t stops[] = {background, 0xff27216c, 0xff174ccc,
        0xff00a8aa, 0xff1dc64b, 0xffd8e52c, 0xffffb020, 0xffed3830, 0xffffe5d5};
    constexpr int intervalCount = static_cast<int>(std::size(stops)) - 1;
    const float position = std::clamp(value, 0.0f, 1.0f) * static_cast<float>(intervalCount);
    const int a = (std::min)(intervalCount - 1, static_cast<int>(position));
    const float t = position - a;
    std::uint32_t result = 0xff000000;
    for (int shift = 0; shift <= 16; shift += 8)
    {
        const float first = static_cast<float>((stops[a] >> shift) & 255);
        const float last = static_cast<float>((stops[a + 1] >> shift) & 255);
        result |= static_cast<std::uint32_t>(std::lround(first + t * (last - first))) << shift;
    }
    return result;
}

WaterfallRaster::WaterfallRaster(std::optional<std::uint32_t> overlayColour)
    : renderHistory_(maximumCorrectedRows), corrected_(maximumCorrectedRows),
      overlayColour_(overlayColour),
      reliefExtent_(overlayColour ? WaterfallStyle::carrierExtent : WaterfallStyle::spectrumExtent),
      frequencyRadius_(static_cast<int>(reliefExtent_ * (overlayColour
          ? WaterfallStyle::carrierFrequencyRadius : WaterfallStyle::frequencyRadius))),
      frequencyRadiusFraction_(static_cast<float>(reliefExtent_ * (overlayColour
          ? WaterfallStyle::carrierFrequencyRadius : WaterfallStyle::frequencyRadius) - frequencyRadius_)),
      table_((maximumLight * lightSteps + 1) * levelSteps)
{
    buildHorizontalInterpolationTable();
    buildShadedColourTable();
    buildHighlightTable();
}

void WaterfallRaster::buildHorizontalInterpolationTable()
{
    for (int x = 0; x < WaterfallSettings::width; ++x)
    {
        const double position = std::clamp((x + 0.5) / WaterfallSettings::horizontalScale - 0.5,
                                          0.0, static_cast<double>(displayBandCount - 1));
        const int first = static_cast<int>(position);
        const double t = position - first;
        coefficients_[x] = {first, (std::min)(first + 1, displayBandCount - 1),
                            static_cast<float>(t * t * (3.0 - 2.0 * t))};
    }
}

void WaterfallRaster::buildShadedColourTable()
{
    // Give the lobe a solid core at moderate levels and a soft fade at its edges.
    std::array<std::uint32_t, levelSteps> overlayOpacity{};
    if (overlayColour_)
        for (int p = 0; p < levelSteps; ++p)
        {
            const float t = (std::min)(1.0f, p / (opaqueCarrierLevel * lastLevel));
            const float coverage = t * t * (3.0f - 2.0f * t);
            overlayOpacity[p] = static_cast<std::uint32_t>(std::lround(coverage * 255.0f));
        }

    // Expensive transfer functions run once, never in the per-pixel loop.
    for (int light = 0; light <= maximumLight * lightSteps; ++light)
    {
        const double gain = static_cast<double>(light) / lightSteps;
        if (overlayColour_)
        {
            const auto gold = shadeColour(*overlayColour_, gain);
            for (int p = 0; p < levelSteps; ++p)
                table_[light * levelSteps + p] = premultiply(gold, overlayOpacity[p]);
        }
        else
        {
            // Scale both extremes around neutral after the light has been clamped.
            const double spectrumGain = 1.0 + WaterfallStyle::spectrumLightAmplitude * (gain - 1.0);
            for (int p = 0; p < levelSteps; ++p)
                table_[light * levelSteps + p] = shadeColour(colour(p / static_cast<float>(lastLevel)), spectrumGain);
        }
    }
}

void WaterfallRaster::buildHighlightTable()
{
    for (int i = 0; i <= highlightIntervals; ++i)
    {
        const float slope = maximumReliefSlope * i / highlightIntervals;
        highlights_[i] = slope * std::exp(-slope / WaterfallStyle::specularPeakSlope);
    }
}

float WaterfallRaster::highlightResponse(float slope) const noexcept
{
    const float position = std::clamp(slope * (highlightIntervals / maximumReliefSlope), 0.0f, static_cast<float>(highlightIntervals));
    const int first = (std::min)(highlightIntervals - 1, static_cast<int>(position));
    const float t = position - first;
    return highlights_[first] + t * (highlights_[first + 1] - highlights_[first]);
}
void WaterfallRaster::reset() noexcept
{
    haveFrame_ = haveLine_ = false;
    previous_.fill(0); temporal_.fill(0);
    historyHead_ = retainedRenderRows_ = correctedCount_ = 0;
}

void WaterfallRaster::cropHistory(int visibleRows) noexcept
{
    retainedRenderRows_ = (std::min)(retainedRenderRows_, (std::max)(0, visibleRows));
    correctedCount_ = (std::min)(correctedCount_, retainedRenderRows_);
}
WaterfallRaster::Levels WaterfallRaster::interpolate(const BandLevels& bands) const noexcept
{
    Levels result{};
    for (int x = 0; x < WaterfallSettings::width; ++x)
    {
        const auto& c = coefficients_[x];
        result[x] = (1.0f - c.weight) * bands[c.first] + c.weight * bands[c.second];
    }
    return result;
}

bool WaterfallRaster::process(const WaterfallFrame& frame, bool shading, Rows& rows) noexcept
{
    Levels current{};
    if (!readLevels(frame, current)) return false;
    const double hopSeconds = frame.hopSize / frame.sampleRate;
    const bool restart = requiresRestart(frame, shading, hopSeconds);
    if (frame.hasProfile && (restart || displayTimingPercent_ != frame.displayTimingPercent))
    {
        displayTimingPercent_ = frame.displayTimingPercent;
        const auto timing = DisplayTiming::atPercent(displayTimingPercent_);
        carrierLevelRetention_ = static_cast<float>(DisplayTiming::retention(hopSeconds, timing.carrierLevelMs));
    }
    if (restart) startObservation(frame, current, hopSeconds);
    else if (frame.hasProfile)
    {
        // FFT bands already carry the bar envelope. For carrier lobes, smooth
        // only where both observations contain a marker: onset and silence stay exact.
        for (int x = 0; x < WaterfallSettings::width; ++x)
            if (current[x] > 0.0f && previous_[x] > 0.0f)
                current[x] += carrierLevelRetention_ * (previous_[x] - current[x]);
    }
    const auto midpoint = interpolateTemporalMidpoint(previous_, current);
    if (!shading) historyLimit_ = WaterfallSettings::linesPerFrame;
    RenderRow row;
    renderRow(midpoint, row); insertRow(row);
    renderRow(current, row); insertRow(row);
    composeHistory(shading);
    rows.current = corrected_[0]; rows.midpoint = corrected_[1];
    rows.reset = restart;
    rememberObservation(frame, current, shading);
    return true;
}

// Validate the selected input before advancing the display history.
bool WaterfallRaster::readLevels(const WaterfallFrame& f, Levels& current) const noexcept
{
    if (!std::isfinite(f.time) || f.time < 0 || !std::isfinite(f.sampleRate)
        || f.sampleRate < AnalysisLimits::minimumSampleRate || f.sampleRate > AnalysisLimits::maximumSampleRate || f.hopSize <= 0
        || (f.channels != 1 && f.channels != 2)) return false;
    for (int b = 0; b < displayBandCount; ++b)
        if (!std::isfinite(f.frequencies[b]) || f.frequencies[b] <= 0
            || (b > 0 && f.frequencies[b] <= f.frequencies[b - 1])) return false;
    if (f.hasProfile)
    {
        for (float level : f.profile)
            if (!std::isfinite(level) || level < 0 || level > 1) return false;
        current = f.profile;
        return true;
    }
    BandLevels combined{};
    for (int b = 0; b < displayBandCount; ++b)
    {
        if (!std::isfinite(f.levels.left[b]) || !std::isfinite(f.levels.right[b])
            || f.levels.left[b] < 0 || f.levels.left[b] > 1 || f.levels.right[b] < 0 || f.levels.right[b] > 1) return false;
        combined[b] = (std::max)(f.levels.left[b], f.levels.right[b]);
    }
    current = interpolate(combined);
    return true;
}

bool WaterfallRaster::requiresRestart(const WaterfallFrame& f, bool shading, double hopSeconds) const noexcept
{
    return !haveFrame_ || shading != shading_ || f.discontinuity
        || f.generation != generation_ || f.revision != revision_
        || f.hasProfile != hasProfile_ || (f.hasProfile && f.sourceRevision != sourceRevision_)
        || f.sampleRate != sampleRate_ || f.hopSize != hopSize_ || f.channels != channels_
        || f.frequencies != frequencies_ || std::abs(f.time - lastTime_ - hopSeconds) > hopSeconds * continuityToleranceHops;
}

void WaterfallRaster::startObservation(const WaterfallFrame& f, const Levels& current, double hopSeconds) noexcept
{
    reset(); previous_ = current;
    sampleRate_ = f.sampleRate; hopSize_ = f.hopSize; channels_ = f.channels;
    frequencies_ = f.frequencies;
    rowSeconds_ = hopSeconds / WaterfallSettings::linesPerFrame;
    const double shadowSeconds = overlayColour_ ? WaterfallStyle::carrierShadowSeconds : WaterfallStyle::spectrumShadowSeconds;
    temporalRetention_ = static_cast<float>(std::exp(-rowSeconds_ / WaterfallStyle::temporalSeconds));
    shadowRetention_ = static_cast<float>(std::exp(-rowSeconds_ / shadowSeconds));
    // Contributions below the visible margin cannot become visible again under max/decay.
    const double threshold = (std::max)(WaterfallStyle::shadowReferenceSlope, WaterfallStyle::shadowSilenceThreshold);
    const double reach = shadowSeconds * std::log((std::max)(static_cast<double>(maximumReliefSlope), threshold) / threshold) / rowSeconds_;
    const int travel = static_cast<int>(std::ceil((std::min)(static_cast<double>(WaterfallSettings::width - 1), reach)));
    historyLimit_ = travel + WaterfallSettings::linesPerFrame;
}

WaterfallRaster::Levels WaterfallRaster::interpolateTemporalMidpoint(const Levels& previous,
    const Levels& current) noexcept
{
    Levels midpoint{};
    for (int x = 0; x < WaterfallSettings::width; ++x) midpoint[x] = 0.5f * (previous[x] + current[x]);
    return midpoint;
}

void WaterfallRaster::rememberObservation(const WaterfallFrame& f, const Levels& current, bool shading) noexcept
{
    previous_ = current; lastTime_ = f.time; generation_ = f.generation; revision_ = f.revision;
    haveFrame_ = true; shading_ = shading; hasProfile_ = f.hasProfile; sourceRevision_ = f.sourceRevision;
}

// Older observations lie below the current row: screen-Y gradient is -dt.
float WaterfallRaster::updateReliefSlope(const Levels& levels, int x) noexcept
{
    const int left = (std::max)(0, x - frequencyRadius_);
    const int right = (std::min)(WaterfallSettings::width - 1, x + frequencyRadius_);
    const float inner = levels[right] - levels[left];
    const float outer = levels[(std::min)(right + 1, WaterfallSettings::width - 1)] - levels[(std::max)(0, left - 1)];
    const float dx = 0.5f * (inner + frequencyRadiusFraction_ * (outer - inner));
    const float dt = levels[x] - temporal_[x];
    temporal_[x] = levels[x] + temporalRetention_ * (temporal_[x] - levels[x]);
    // Newer samples are above older rows: screen Y runs opposite to time.
    const float slope = WaterfallStyle::frequencyRelief * dx - WaterfallStyle::timeRelief * dt;
    return slope;
}

float WaterfallRaster::reliefLight(float localLight, float effectiveShadow, bool shading) noexcept
{
    return shading ? std::clamp(localLight - WaterfallStyle::shadow * effectiveShadow,
        WaterfallStyle::minimumLight, static_cast<float>(maximumLight)) : 1.0f;
}

std::uint32_t WaterfallRaster::lookupShadedColour(int paletteIndex, float light) const noexcept
{
    if (paletteIndex == levelSteps) return overlayColour_ ? 0u : background;
    return table_[static_cast<int>(std::lround(light * lightSteps)) * levelSteps + paletteIndex];
}

void WaterfallRaster::renderRow(const Levels& levels, RenderRow& row) noexcept
{
    if (!haveLine_) { temporal_ = levels; haveLine_ = true; }
    for (int x = 0; x < WaterfallSettings::width; ++x)
    {
        const float slope = updateReliefSlope(levels, x);
        row.localLight[x] = 1.0f + WaterfallStyle::highlight * highlightResponse(slope);
        row.shadowSeed[x] = (std::max)(0.0f, -slope);
        row.palette[x] = static_cast<std::uint16_t>(levels[x] == 0 ? levelSteps : std::lround(levels[x] * lastLevel));
    }
}

void WaterfallRaster::insertRow(const RenderRow& row) noexcept
{
    historyHead_ = (historyHead_ + maximumCorrectedRows - 1) % maximumCorrectedRows;
    renderHistory_[historyHead_] = row;
    retainedRenderRows_ = (std::min)(historyLimit_, retainedRenderRows_ + 1);
}

void WaterfallRaster::composeHistory(bool shading) noexcept
{
    Levels shadow{};
    correctedCount_ = retainedRenderRows_;
    // Y is screen Y, newest first. The previous pass is the row ABOVE this one.
    for (int y = 0; y < correctedCount_; ++y)
    {
        const auto& row = renderHistory_[(historyHead_ + y) % maximumCorrectedRows];
        auto& pixels = corrected_[y];
        // Descending X preserves the above-left sample until it has been read.
        for (int x = WaterfallSettings::width - 1; x >= 0; --x)
        {
            shadow[x] = (std::max)(row.shadowSeed[x], x ? shadowRetention_ * shadow[x - 1] : 0.0f);
            if (shadow[x] < WaterfallStyle::shadowSilenceThreshold) shadow[x] = 0;
            const float effectiveShadow = shading
                ? std::clamp(shadow[x] - WaterfallStyle::shadowReferenceSlope, 0.0f, 1.0f) : 0.0f;
            auto pixel = lookupShadedColour(row.palette[x], reliefLight(row.localLight[x], effectiveShadow, shading));
            if (overlayColour_)
            {
                // Composite the translucent lobe over a black cast shadow.
                // Its premultiplied RGB is unchanged by the shadow behind it.
                const auto shade = static_cast<std::uint32_t>(carrierShadowOpacity * effectiveShadow + 0.5f);
                const auto extraAlpha = ((255 - (pixel >> 24)) * shade + 127) / 255;
                pixel += extraAlpha << 24;
            }
            pixels[x] = pixel;
        }
    }
}
}
