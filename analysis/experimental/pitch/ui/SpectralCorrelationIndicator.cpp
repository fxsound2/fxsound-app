// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SpectralCorrelationIndicator.h"
#include <algorithm>
#include <cmath>

namespace fxanalysis::experimental
{
using namespace juce;
namespace
{
constexpr juce::uint32 backgroundColour = 0xff101821;
constexpr juce::uint32 plotColour = 0xff172430;
constexpr juce::uint32 axisColour = 0xffb4c3d2;
constexpr juce::uint32 gridColour = 0xff344453;
constexpr std::array<juce::uint32, spectralCorrelationLagCount> lagColours{{0xff59d9da, 0xffffbe5c, 0xffb59cfa}};
constexpr double maximumShift = pitchHistogramMinimumSemitones
    + (pitchHistogramBinCount - 1) * pitchHistogramStepSemitones;
constexpr double minimumHistoryStepSeconds = 0.020;

String signedValue(double value, int decimals = 2)
{
    if (!std::isfinite(value)) return "--";
    return (value > 0.0 ? String("+") : String()) + String(value, decimals);
}
void drawShiftAxis(Graphics& g, Rectangle<float> plot)
{
    g.setColour(Colour(axisColour));
    g.setFont(10.5f);
    for (const double shift : {pitchHistogramMinimumSemitones, -6.0, 0.0, 6.0, maximumShift})
    {
        const float x = plot.getX() + static_cast<float>((shift - pitchHistogramMinimumSemitones)
            / (maximumShift - pitchHistogramMinimumSemitones)) * plot.getWidth();
        g.drawText(signedValue(shift, 0), roundToInt(x) - 18, roundToInt(plot.getBottom()) + 2,
                   36, 16, Justification::centred);
    }
}
}

SpectralCorrelationIndicator::SpectralCorrelationIndicator()
{
    setOpaque(true);
    setTooltip("Cosine similarity of spectral modules on a log-frequency grid. Colors identify actual comparison lags. Peak contrast and similarity are numerical diagnostics, not musical confidence.");
}
void SpectralCorrelationIndicator::clear()
{
    snapshot_ = {};
    historyBegin_ = historyCount_ = 0;
    lastInputTime_ = 0.0;
    haveInputTime_ = false;
    repaint();
}
void SpectralCorrelationIndicator::setTrajectoryLimitSemitones(double limit)
{
    if (!std::isfinite(limit) || limit <= 0.0) return;
    trajectoryLimitSemitones_ = limit;
    repaint();
}
void SpectralCorrelationIndicator::setSnapshot(const SpectralCorrelationSnapshot& snapshot)
{
    const bool anyEstimate = std::any_of(snapshot.lags.begin(), snapshot.lags.end(),
        [](const SpectralCorrelationLagSnapshot& lag) { return lag.hasEstimate; });
    if (!anyEstimate || !std::isfinite(snapshot.centreTimeSeconds))
    {
        clear();
        snapshot_ = snapshot;
        return;
    }
    const double time = snapshot.centreTimeSeconds;
    if (haveInputTime_ && (time < lastInputTime_ || time - lastInputTime_ > 0.5))
        clear();
    snapshot_ = snapshot;
    if (haveInputTime_ && time <= lastInputTime_)
    {
        repaint();
        return;
    }
    lastInputTime_ = time;
    haveInputTime_ = true;
    while (historyCount_ > 0 && time - history_[historyBegin_].time > historySeconds)
    {
        historyBegin_ = (historyBegin_ + 1) % historyCapacity;
        --historyCount_;
    }
    const int previous = (historyBegin_ + historyCount_ - 1 + historyCapacity) % historyCapacity;
    if (historyCount_ == 0 || time - history_[previous].time >= minimumHistoryStepSeconds)
    {
        if (historyCount_ == historyCapacity)
        {
            historyBegin_ = (historyBegin_ + 1) % historyCapacity;
            --historyCount_;
        }
        auto& point = history_[(historyBegin_ + historyCount_) % historyCapacity];
        point = {};
        point.time = time;
        for (int lag = 0; lag < spectralCorrelationLagCount; ++lag)
        {
            point.valid[lag] = snapshot.lags[lag].hasEstimate
                && std::isfinite(snapshot.lags[lag].displacementSemitones);
            point.displacement[lag] = snapshot.lags[lag].displacementSemitones;
        }
        ++historyCount_;
    }
    repaint();
}

void SpectralCorrelationIndicator::drawCorrelation(Graphics& g, Rectangle<float> plot)
{
    g.setColour(Colour(plotColour));
    g.fillRect(plot);
    g.setColour(Colour(gridColour));
    g.drawHorizontalLine(roundToInt(plot.getCentreY()), plot.getX(), plot.getRight());
    g.drawHorizontalLine(roundToInt(plot.getBottom()), plot.getX(), plot.getRight());
    g.drawVerticalLine(roundToInt(plot.getCentreX()), plot.getY(), plot.getBottom());
    g.setColour(Colour(axisColour));
    g.setFont(10.5f);
    g.drawText("1", 0, roundToInt(plot.getY()) - 5, roundToInt(plot.getX()) - 5, 15, Justification::centredRight);
    g.drawText("0", 0, roundToInt(plot.getBottom()) - 9, roundToInt(plot.getX()) - 5, 15, Justification::centredRight);
    for (int lag = 0; lag < spectralCorrelationLagCount; ++lag)
    {
        const auto& result = snapshot_.lags[lag];
        if (!result.hasEstimate) continue;
        Path path;
        bool started = false;
        for (int index = 0; index < pitchHistogramBinCount; ++index)
        {
            const float score = result.correlation[index];
            if (!std::isfinite(score)) { started = false; continue; }
            const float x = plot.getX() + index * plot.getWidth() / (pitchHistogramBinCount - 1);
            const float y = plot.getBottom() - std::clamp(score, 0.0f, 1.0f) * plot.getHeight();
            if (started) path.lineTo(x, y); else { path.startNewSubPath(x, y); started = true; }
        }
        g.setColour(Colour(lagColours[lag]));
        g.strokePath(path, PathStrokeType(1.5f));
        if (std::isfinite(result.displacementSemitones) && std::isfinite(result.peakCorrelation))
        {
            const float x = plot.getX() + static_cast<float>((std::clamp(result.displacementSemitones,
                pitchHistogramMinimumSemitones, maximumShift) - pitchHistogramMinimumSemitones)
                / (maximumShift - pitchHistogramMinimumSemitones)) * plot.getWidth();
            const float y = plot.getBottom() - static_cast<float>(std::clamp(result.peakCorrelation, 0.0, 1.0)) * plot.getHeight();
            g.fillEllipse(x - 2.5f, y - 2.5f, 5.0f, 5.0f);
        }
    }
    drawShiftAxis(g, plot);
}

void SpectralCorrelationIndicator::drawHistory(Graphics& g, Rectangle<float> plot)
{
    double maximumHistoryShift = 0.0;
    for (int i = 0; i < historyCount_; ++i)
    {
        const auto& point = history_[(historyBegin_ + i) % historyCapacity];
        for (int lag = 0; lag < spectralCorrelationLagCount; ++lag)
            if (point.valid[lag] && std::isfinite(point.displacement[lag]))
                maximumHistoryShift = (std::max)(maximumHistoryShift, std::abs(point.displacement[lag]));
    }
    const double historyLimit = std::clamp(0.25 * std::ceil(1.15 * maximumHistoryShift / 0.25), 0.25, maximumShift);
    g.setColour(Colour(plotColour));
    g.fillRect(plot);
    g.setColour(Colour(gridColour));
    g.drawHorizontalLine(roundToInt(plot.getCentreY()), plot.getX(), plot.getRight());
    g.drawVerticalLine(roundToInt(plot.getCentreX()), plot.getY(), plot.getBottom());
    g.setColour(Colour(axisColour));
    g.setFont(10.5f);
    for (const double shift : {-historyLimit, 0.0, historyLimit})
    {
        const float y = plot.getCentreY() - static_cast<float>(shift / historyLimit) * 0.5f * plot.getHeight();
        g.drawText(signedValue(shift, historyLimit < 1.0 ? 2 : 1), 0, roundToInt(y) - 7,
                   roundToInt(plot.getX()) - 5, 15, Justification::centredRight);
    }
    {
        const Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(plot.toNearestInt());
        for (int lag = 0; lag < spectralCorrelationLagCount; ++lag)
        {
            Path path;
            bool started = false, haveLastPoint = false;
            float lastX = 0.0f, lastY = 0.0f;
            for (int i = 0; i < historyCount_; ++i)
            {
                const auto& point = history_[(historyBegin_ + i) % historyCapacity];
                if (!point.valid[lag]) { started = false; continue; }
                const float x = plot.getRight() - static_cast<float>((lastInputTime_ - point.time) / historySeconds) * plot.getWidth();
                const float y = plot.getCentreY() - static_cast<float>(std::clamp(point.displacement[lag],
                    -historyLimit, historyLimit) / historyLimit) * 0.5f * plot.getHeight();
                if (started) path.lineTo(x, y); else { path.startNewSubPath(x, y); started = true; }
                haveLastPoint = true;
                lastX = x; lastY = y;
            }
            g.setColour(Colour(lagColours[lag]));
            g.strokePath(path, PathStrokeType(1.5f));
            if (haveLastPoint) g.fillEllipse(lastX - 2.0f, lastY - 2.0f, 4.0f, 4.0f);
        }
    }
    g.setColour(Colour(axisColour));
    g.drawText("auto +/-" + String(historyLimit, 2) + " st", roundToInt(plot.getRight()) - 138,
               roundToInt(plot.getY()) - 16, 138, 14, Justification::centredRight);
    g.drawText("-10 s", roundToInt(plot.getX()), roundToInt(plot.getBottom()) + 2, 50, 16, Justification::centredLeft);
    g.drawText("-5 s", roundToInt(plot.getCentreX()) - 25, roundToInt(plot.getBottom()) + 2, 50, 16, Justification::centred);
    g.drawText("0 s", roundToInt(plot.getRight()) - 50, roundToInt(plot.getBottom()) + 2, 50, 16, Justification::centredRight);
}

void SpectralCorrelationIndicator::drawIntegral(Graphics& g, Rectangle<float> area)
{
    g.setColour(Colour(axisColour));
    g.setFont(11.0f);
    g.drawText("Correlation integral " + signedValue(snapshot_.integratedSemitones) + " st  |  combined rate "
        + signedValue(snapshot_.combinedSemitonesPerSecond) + " st/s", area.toNearestInt().removeFromTop(17), Justification::centredLeft);
    const Rectangle<float> track(area.getX(), area.getY() + 21.0f, area.getWidth(), 4.0f);
    g.setColour(Colour(gridColour));
    g.fillRect(track);
    const double integral = std::isfinite(snapshot_.integratedSemitones) ? snapshot_.integratedSemitones : 0.0;
    const float offset = static_cast<float>(std::clamp(integral / trajectoryLimitSemitones_, -1.0, 1.0)) * track.getWidth() * 0.5f;
    g.setColour(Colour(lagColours[0]));
    g.fillRect((std::min)(track.getCentreX(), track.getCentreX() + offset), track.getY(), std::abs(offset), track.getHeight());
    g.setColour(Colour(axisColour));
    g.drawVerticalLine(roundToInt(track.getCentreX()), track.getY() - 1.0f, track.getBottom() + 1.0f);
}

void SpectralCorrelationIndicator::paint(Graphics& g)
{
    g.fillAll(Colour(backgroundColour));
    if (getWidth() < 140 || getHeight() < 160) return;
    g.setColour(Colour(axisColour));
    g.setFont(13.0f);
    g.drawText("Module correlation vs shift [st] - similarity 0..1", 40, 0, getWidth() - 52, 19, Justification::centredLeft);
    const int legendWidth = (getWidth() - 52) / spectralCorrelationLagCount;
    String contrasts = "Peak history [st] | peak contrast ";
    bool anyEstimate = false;
    for (int lag = 0; lag < spectralCorrelationLagCount; ++lag)
    {
        const auto& result = snapshot_.lags[lag];
        anyEstimate = anyEstimate || result.hasEstimate;
        const String label = result.hasEstimate
            ? String(result.comparisonSeconds * 1000.0, 1) + " ms: " + signedValue(result.displacementSemitones)
                + " st  R " + String(result.peakCorrelation, 2) + (result.atSearchBoundary ? "*" : "")
            : String(spectralCorrelationRequestedLags[lag] * 1000.0, 0) + " ms: waiting";
        g.setColour(Colour(lagColours[lag]));
        g.setFont(10.5f);
        g.drawFittedText(label, 40 + lag * legendWidth, 20, legendWidth - 5, 18, Justification::centredLeft, 1);
        if (lag > 0) contrasts += " / ";
        contrasts += result.hasEstimate ? String(result.prominence, 2) : String("--");
    }
    const float chartHeight = (std::max)(20.0f, (getHeight() - 138.0f) * 0.5f);
    const Rectangle<float> correlationPlot(40.0f, 44.0f, static_cast<float>(getWidth() - 52), chartHeight);
    drawCorrelation(g, correlationPlot);
    if (!anyEstimate)
    {
        g.setColour(Colour(axisColour));
        g.setFont(11.0f);
        g.drawText(String(describe(snapshot_.status)), correlationPlot.toNearestInt(), Justification::centred);
    }
    g.setColour(Colour(axisColour));
    g.setFont(10.5f);
    g.drawFittedText(contrasts + "  (* search limit)", 40, roundToInt(correlationPlot.getBottom()) + 22,
                     getWidth() - 192, 18, Justification::centredLeft, 1);
    const Rectangle<float> historyPlot(40.0f, correlationPlot.getBottom() + 42.0f,
                                       static_cast<float>(getWidth() - 52), chartHeight);
    drawHistory(g, historyPlot);
    drawIntegral(g, Rectangle<float>(40.0f, static_cast<float>(getHeight() - 32),
                                      static_cast<float>(getWidth() - 52), 28.0f));
}
}
