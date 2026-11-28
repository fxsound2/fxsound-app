// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PitchMotionIndicator.h"
#include <algorithm>
#include <cmath>

namespace fxanalysis::experimental
{
using namespace juce;
namespace
{
constexpr juce::uint32 backgroundColour = 0xff101821;
constexpr juce::uint32 plotColour = 0xff172430;
constexpr juce::uint32 axisColour = 0xffa9b9ca;
constexpr juce::uint32 gridColour = 0xff30404f;
constexpr juce::uint32 lowerColour = 0xffed7690;
constexpr juce::uint32 higherColour = 0xff59d9da;
constexpr juce::uint32 stationaryColour = 0xff8ea0b7;
constexpr float histogramDisplayGain = 4.0f;
constexpr double maximumDisplacement = pitchHistogramMinimumSemitones
    + (pitchHistogramBinCount - 1) * pitchHistogramStepSemitones;

String signedValue(double value, int decimals = 2)
{
    if (!std::isfinite(value)) return "--";
    return (value > 0.0 ? String("+") : String()) + String(value, decimals);
}
String percentage(double mass)
{
    return std::isfinite(mass) ? String(100.0 * mass, 1) + "%" : String("--");
}
String statusText(PitchMotionResult status)
{
    switch (status)
    {
    case PitchMotionResult::ok: return "";
    case PitchMotionResult::warmingUp: return "Collecting comparison frames";
    case PitchMotionResult::noSignal: return "No usable spectral peaks";
    case PitchMotionResult::notPrepared: return "Waiting for audio";
    case PitchMotionResult::invalidConfiguration: return "Invalid experimental settings";
    case PitchMotionResult::invalidFrame: return "Audio discontinuity or invalid frame";
    case PitchMotionResult::nonFiniteInput: return "Invalid spectral values";
    case PitchMotionResult::numericalFailure: return "Transport calculation unavailable";
    }
    return "Waiting for audio";
}
}

PitchMotionIndicator::PitchMotionIndicator()
{
    setOpaque(true);
    setTooltip("Weighted spectral movement: left = lower, right = higher. Histogram display gain x4; 25% mass fills the plot.");
}
void PitchMotionIndicator::setSnapshot(const PitchMotionSnapshot& snapshot)
{
    snapshot_ = snapshot;
    repaint();
}
void PitchMotionIndicator::clear()
{
    snapshot_ = {};
    repaint();
}
void PitchMotionIndicator::setCompact(bool compact)
{
    if (compact_ == compact) return;
    compact_ = compact;
    setTooltip(compact ? "Weighted spectral movement: left = lower, right = higher. Display gain x4. Double-click to open the experiment."
                       : "Weighted spectral movement: left = lower, right = higher. Histogram display gain x4; 25% mass fills the plot.");
    repaint();
}
void PitchMotionIndicator::setShowSmoothed(bool smoothed)
{
    if (showSmoothed_ == smoothed) return;
    showSmoothed_ = smoothed;
    repaint();
}
void PitchMotionIndicator::setTrajectoryLimitSemitones(double limit)
{
    if (!std::isfinite(limit) || limit <= 0.0) return;
    trajectoryLimitSemitones_ = limit;
    repaint();
}
void PitchMotionIndicator::mouseDoubleClick(const MouseEvent&)
{
    if (onDoubleClick) onDoubleClick();
}

void PitchMotionIndicator::drawHistogram(Graphics& g, Rectangle<float> plot)
{
    const auto& histogram = showSmoothed_ ? snapshot_.smoothedHistogram : snapshot_.rawHistogram;
    // Same fixed x4 display gain for raw, smoothed and compact views.
    // Numerical masses and diagnostics remain unchanged; 25% saturates visually.
    // Never renormalize the current and remembered histograms to independent peaks.
    const float heightScale = histogramDisplayGain * plot.getHeight();
    const float cellWidth = plot.getWidth() / static_cast<float>(pitchHistogramBinCount);
    const auto xForDisplacement = [&](double displacement)
    {
        const double bin = (displacement - pitchHistogramMinimumSemitones) / pitchHistogramStepSemitones;
        return plot.getX() + (static_cast<float>(bin) + 0.5f) * cellWidth;
    };
    g.setColour(Colour(plotColour));
    g.fillRect(plot);
    if (!compact_)
    {
        g.setColour(Colour(gridColour));
        g.drawHorizontalLine(roundToInt(plot.getCentreY()), plot.getX(), plot.getRight());
        g.drawHorizontalLine(roundToInt(plot.getBottom()), plot.getX(), plot.getRight());
        for (const double displacement : {pitchHistogramMinimumSemitones, -6.0, 0.0, 6.0, maximumDisplacement})
            g.drawVerticalLine(roundToInt(xForDisplacement(displacement)), plot.getY(), plot.getBottom());
    }
    for (int i = 0; i < pitchHistogramBinCount; ++i)
    {
        const float mass = histogram[static_cast<std::size_t>(i)];
        if (!std::isfinite(mass) || mass <= 0.0f) continue;
        const double displacement = pitchHistogramMinimumSemitones + i * pitchHistogramStepSemitones;
        const float height = (std::min)(plot.getHeight(), mass * heightScale);
        const float gap = compact_ ? 0.0f : (std::min)(1.0f, cellWidth * 0.15f);
        g.setColour(Colour(displacement < -0.5 * pitchHistogramStepSemitones ? lowerColour
            : displacement > 0.5 * pitchHistogramStepSemitones ? higherColour : stationaryColour));
        g.fillRect(plot.getX() + i * cellWidth + gap * 0.5f, plot.getBottom() - height,
                   (std::max)(0.25f, cellWidth - gap), height);
    }
    g.setColour(Colour(axisColour).withAlpha(compact_ ? 0.6f : 0.85f));
    g.drawVerticalLine(roundToInt(xForDisplacement(0.0)), plot.getY(), plot.getBottom());
    if (compact_) return;

    g.setFont(11.0f);
    for (const double displacement : {pitchHistogramMinimumSemitones, -6.0, 0.0, 6.0, maximumDisplacement})
        g.drawText(signedValue(displacement, 0), roundToInt(xForDisplacement(displacement)) - 22,
                   roundToInt(plot.getBottom()) + 3, 44, 17, Justification::centred);
    g.setColour(Colour(axisColour));
    g.drawText("25%", 0, roundToInt(plot.getY()) - 6,
               roundToInt(plot.getX()) - 5, 16, Justification::centredRight);
    g.drawText("0", 0, roundToInt(plot.getBottom()) - 10,
               roundToInt(plot.getX()) - 5, 16, Justification::centredRight);
    const String interval = snapshot_.comparisonSeconds > 0.0
        ? String(snapshot_.comparisonSeconds * 1000.0, 1) + " ms" : String("fixed lag");
    const int labelY = roundToInt(plot.getBottom()) + 22;
    g.drawText("Lower", roundToInt(plot.getX()), labelY, 64, 18, Justification::centredLeft);
    g.drawText("Higher", roundToInt(plot.getRight()) - 64, labelY, 64, 18, Justification::centredRight);
    g.drawText("Movement over " + interval + " [semitones]",
        roundToInt(plot.getX()) + 68, labelY, roundToInt(plot.getWidth()) - 136, 18, Justification::centred);
}

void PitchMotionIndicator::drawTrajectory(Graphics& g, Rectangle<float> area)
{
    g.setColour(Colour(axisColour));
    g.setFont(11.5f);
    g.drawText("Accumulated net movement " + signedValue(snapshot_.integratedSemitones)
        + " st  |  display +/-" + String(trajectoryLimitSemitones_, 1) + " st", area.toNearestInt().removeFromTop(18), Justification::centredLeft);
    const Rectangle<float> track(area.getX(), area.getY() + 22.0f, area.getWidth(), 6.0f);
    g.setColour(Colour(gridColour));
    g.fillRect(track);
    const double integrated = std::isfinite(snapshot_.integratedSemitones) ? snapshot_.integratedSemitones : 0.0;
    const float offset = static_cast<float>(std::clamp(integrated / trajectoryLimitSemitones_, -1.0, 1.0))
        * track.getWidth() * 0.5f;
    g.setColour(Colour(offset < 0.0f ? lowerColour : higherColour));
    g.fillRect((std::min)(track.getCentreX(), track.getCentreX() + offset), track.getY(), std::abs(offset), track.getHeight());
    g.setColour(Colour(axisColour));
    g.drawVerticalLine(roundToInt(track.getCentreX()), track.getY() - 1.0f, track.getBottom() + 1.0f);
}

void PitchMotionIndicator::paint(Graphics& g)
{
    g.fillAll(Colour(backgroundColour));
    if (getWidth() <= 0 || getHeight() <= 0) return;
    if (compact_)
    {
        drawHistogram(g, getLocalBounds().toFloat());
        return;
    }
    g.setColour(Colour(axisColour));
    g.setFont(13.0f);
    const String title = showSmoothed_ ? "Smoothed weighted movement histogram (x4)" : "Instantaneous weighted movement histogram (x4)";
    g.drawText(title, 48, 3, getWidth() - 64, 22, Justification::centredLeft);
    const Rectangle<float> plot(48.0f, 35.0f, (std::max)(1.0f, getWidth() - 65.0f),
                                (std::max)(1.0f, getHeight() - 199.0f));
    drawHistogram(g, plot);
    const int diagnosticTop = roundToInt(plot.getBottom()) + 44;
    const double lowerTail = showSmoothed_ ? snapshot_.smoothedLowerTailMass : snapshot_.rawLowerTailMass;
    const double upperTail = showSmoothed_ ? snapshot_.smoothedUpperTailMass : snapshot_.rawUpperTailMass;
    g.setFont(11.5f);
    const bool hasInRangeMode = std::any_of(snapshot_.rawHistogram.begin(), snapshot_.rawHistogram.end(),
        [](float mass) { return std::isfinite(mass) && mass > 0.0f; });
    const String modeText = hasInRangeMode ? signedValue(snapshot_.modeSemitones) + " st" : String("--");
    const String diagnostics = snapshot_.hasEstimate
        ? "Instantaneous: mode " + modeText + "  |  net " + signedValue(snapshot_.netDisplacementSemitones)
            + " st  |  transported " + percentage(snapshot_.matchedMass)
        : statusText(snapshot_.status);
    g.drawFittedText(diagnostics, 48, diagnosticTop, getWidth() - 65, 18, Justification::centredLeft, 1);
    const String marginals = "Soft marginal deficit old/new " + percentage(snapshot_.unmatchedSourceMass)
        + " / " + percentage(snapshot_.unmatchedTargetMass) + "  |  excess "
        + percentage(snapshot_.excessSourceMass) + " / " + percentage(snapshot_.excessTargetMass);
    g.drawFittedText(marginals, 48, diagnosticTop + 19, getWidth() - 65, 18, Justification::centredLeft, 1);
    const String tails = "Outside range: lower " + percentage(lowerTail) + " / higher " + percentage(upperTail);
    const String solver = snapshot_.hasEstimate
        ? String(snapshot_.solverConverged ? "Solver converged" : "Approximate transport")
            + " (residual " + String::formatted("%.2g", snapshot_.solverResidual) + ")  |  "
        : String();
    g.drawFittedText(solver + tails, 48, diagnosticTop + 38, getWidth() - 65, 18, Justification::centredLeft, 1);
    drawTrajectory(g, Rectangle<float>(48.0f, static_cast<float>(getHeight() - 53),
                                       static_cast<float>((std::max)(1, getWidth() - 65)), 31.0f));
    g.setFont(10.5f);
    g.drawText("Relative spectral motion; the integral is not an absolute musical pitch.",
               48, getHeight() - 20, getWidth() - 65, 16, Justification::centredLeft);
}
}
