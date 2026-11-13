/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
    www.theremino.com (2025)

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

#include "FxVisualizer.h"
#include "FxController.h"
#include "FxTheme.h"

FxVisualizer::FxVisualizer()
{
    band_values_.resize(FxController::NUM_SPECTRUM_BANDS);
    band_graph_.resize(FxController::NUM_SPECTRUM_BANDS * NUM_BARS);
    auto& settings = FxController::getInstance().getSettings();
    analysis_button_.setClickingTogglesState(true);
    analysis_button_.setToggleState(settings.getBool("highres_visualizer"), dontSendNotification);
    analysis_button_.onClick = [this]
    {
        FxController::getInstance().getSettings().setBool("highres_visualizer", analysis_button_.getToggleState());
        updateAnalysisState();
        repaint();
    };
    analysis_button_.setTitle(TRANS("Switch spectrum visualizer"));
    analysis_button_.setTooltip(TRANS("Switch between the classic and HighRes spectrum."));
    addAndMakeVisible(analysis_button_);
    addChildComponent(meter_tooltip_);
    lookAndFeelChanged();

#if JUCE_MAJOR_VERSION >= 8
    start();
#else
    calcGradient();
    reset();
    setFramesPerSecond(10);
#endif

    setOpaque(false);
    setSize(WIDTH, HEIGHT);
}

void FxVisualizer::start()
{
    running_ = true;
    updateAnalysisState();
    calcGradient();

#if JUCE_MAJOR_VERSION >= 8
    if (vblank_listener_ == nullptr)
    {
        vblank_listener_ = std::make_unique<juce::VBlankAttachment>(
            this,
            [this](double timestamp)
            {
                if (!isShowing())
                {
                    updateAnalysisState();
                    return;
                }

                static double last_frame_time = 0.0;
                constexpr double fps_interval = 1.0 / 30.0;

                if (timestamp - last_frame_time >= fps_interval)
                {
                    last_frame_time = timestamp;
                    if (FxController::getInstance().isAudioProcessing())
                    {
                        update();
                        repaint();
                    }
                    else
                    {
                        pause();
                    }
                }
            });
    }
#else
    setFramesPerSecond(30);
#endif
}

void FxVisualizer::pause()
{
    running_ = false;
    updateAnalysisState();
    calcGradient();

#if JUCE_MAJOR_VERSION >= 8
    reset();
    repaint();

    if (vblank_listener_ != nullptr)
    {
        vblank_listener_.reset();
    }
#else
    setFramesPerSecond(10);
#endif
}

void FxVisualizer::reset()
{
    pending_bars_.reset();
    displayed_bars_ = {};
    analysis_generation_ = 0;
    FxController::getInstance().discardAnalyzedSpectrumPeaks();
    resetMeters();
    for (int i = 0; i < FxController::NUM_SPECTRUM_BANDS * NUM_BARS; i++)
    {
        band_graph_.set(i, 0);
    }
}

void FxVisualizer::update()
{
    updateAnalysisState();
    if (!isEnabled() || !isShowing()) return;

    if (showing_highres_)
    {
        auto& controller = FxController::getInstance();
        if (controller.isOutputMuted()) return;
        updateOutputIndicators(controller);
        updateHighResBars(controller);
        return;
    }
    updateClassicSpectrum();
}

void FxVisualizer::updateOutputIndicators(FxController& controller)
{
    const auto clips = controller.consumeOutputClipEvents();
    const auto limiting = controller.consumeLimiterActivity();
    const auto now = FxClipIndicator::Clock::now();
    left_clip_.update(clips.left, now);
    right_clip_.update(clips.right, now);
    left_clip_.updateLimiter(limiting.leftDb, now);
    right_clip_.updateLimiter(limiting.rightDb, now);
    updateMeterTooltip();
}

void FxVisualizer::updateHighResBars(FxController& controller)
{
    fxanalysis::BarSnapshot bars;
    if (controller.getAnalyzedSpectrumBands(bars))
    {
        if (analysis_generation_ != bars.generation) pending_bars_.reset();
        analysis_generation_ = bars.generation;
        pending_bars_.add(bars.levels.left, bars.levels.right);
    }
    else
    {
        pending_bars_.reset();
        displayed_bars_ = {};
    }
}

void FxVisualizer::updateClassicSpectrum()
{
    FxController::getInstance().getSpectrumBandValues(band_values_);

    for (int i = 0; i < FxController::NUM_SPECTRUM_BANDS; i++)
    {
        if (band_values_[i] < 0 || band_values_[i] > 1)
        {
            band_values_.set(i, 0);
        }

        for (int j = 0; j < NUM_BARS / 2; j++)
        {
            band_graph_.set(i*NUM_BARS + j, band_graph_[i*NUM_BARS + j + 1]);
            band_graph_.set(i*NUM_BARS + (NUM_BARS - 1) - j, band_graph_[i*NUM_BARS + j + 1]);
        }

        band_graph_.set(i*NUM_BARS + NUM_BARS / 2, band_values_[i]);
    }
}

void FxVisualizer::paint(Graphics& g)
{
    g.setColour(Colour(FXCOLOR(ControlBackground)));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8);
    g.setGradientFill(gradient_);
    if (showing_highres_)
    {
        paintHighResSpectrum(g);
        paintOutputIndicators(g);
    }
    else
    {
        paintClassicSpectrum(g);
    }
}

void FxVisualizer::paintHighResSpectrum(Graphics& g)
{
    // Keep short attacks visible when several UI updates share one repaint.
    // A partial repaint keeps the previous heights until the full plot is redrawn.
    if (pending_bars_.hasPending() && g.getClipBounds().contains(juce::Rectangle<int>(plotLeft, plotTop, plotWidth, plotHeight)))
        displayed_bars_ = pending_bars_.consume();

    Path bars;
    const float center = getHeight() * 0.5f;
    const float step = static_cast<float>(plotWidth) / fxanalysis::displayBandCount;
    float x = plotLeft;
    for (int band = 0; band < fxanalysis::displayBandCount; ++band, x += step)
    {
        const float left = displayed_bars_.left[band] * (plotHeight * 0.5f);
        const float right = displayed_bars_.right[band] * (plotHeight * 0.5f);
        if (left > 0.0f) bars.addRectangle(x, center - left, barWidth, left);
        if (right > 0.0f) bars.addRectangle(x, center, barWidth, right);
    }
    g.fillPath(bars);
}

void FxVisualizer::paintClassicSpectrum(Graphics& g)
{
    Path bars;
    const float center = getHeight() * 0.5f;
    const float step = static_cast<float>(plotWidth) / band_graph_.size();
    float x = plotLeft;
    for (const float value : band_graph_)
    {
        const float height = (value == 0.0f ? 0.01f : value) * plotHeight;
        bars.addRectangle(x, center - height * 0.5f, barWidth, height);
        x += step;
    }
    g.fillPath(bars);
}

void FxVisualizer::paintOutputIndicators(Graphics& g)
{
    const auto center = getHeight() / 2;
    g.setColour(Colour(FXCOLOR(GraphHigh)));
    g.setFont(11.0f);
    g.drawText(TRANS("L"), 5, center - 23, 18, 16, Justification::centred);
    g.drawText(TRANS("R"), 5, center + 7, 18, 16, Justification::centred);

    const auto now = FxClipIndicator::Clock::now();
    const Colour clipOn(FXCOLOR(PeakWarning)), clipOff(FXCOLOR(PeakInactive));
    const Colour limiterOn(FXCOLOR(LimiterWarning));
    const auto indicatorColour = [&](const FxClipIndicator& indicator)
    {
        if (indicator.isClipped(now)) return clipOn;
        const float reduction = indicator.reductionDb(now);
        if (reduction < FxClipIndicator::activityThresholdDb) return clipOff;
        const float severity = jlimit(0.0f, 1.0f,
            (reduction - FxClipIndicator::activityThresholdDb)
            / (FxClipIndicator::strongReductionDb - FxClipIndicator::activityThresholdDb));
        return limiterOn.interpolatedWith(clipOn, severity);
    };
    g.setColour(indicatorColour(left_clip_));
    g.fillEllipse(10.0f, center - 6.0f, 8.0f, 8.0f);
    g.setColour(indicatorColour(right_clip_));
    g.fillEllipse(10.0f, center + 24.0f, 8.0f, 8.0f);
}

void FxVisualizer::enablementChanged()
{
    if (isEnabled())
    {
        start();
    }
    else
    {
        pause();
        reset();
    }
}

void FxVisualizer::resized()
{
    constexpr int buttonSize = 18;
    analysis_button_.setBounds(getWidth() - buttonSize - 5, 5, buttonSize, buttonSize);
    meter_tooltip_.setBounds(5, getHeight() / 2 - 6, 18, 38);
}

void FxVisualizer::visibilityChanged()
{
    updateAnalysisState();
}

void FxVisualizer::parentHierarchyChanged()
{
    updateAnalysisState();
}

void FxVisualizer::updateAnalysisState()
{
    auto& controller = FxController::getInstance();
    const bool supported = controller.canAnalyzeOutput();
    const bool muted = controller.isOutputMuted();
    const bool highres = isEnabled() && isShowing()
        && analysis_button_.getToggleState() && supported;
    const bool requested = running_ && highres && !muted && controller.isAudioProcessing();
    if (supported != output_supported_)
    {
        output_supported_ = supported;
        analysis_button_.setTooltip(supported ? TRANS("Switch between the classic and HighRes spectrum.")
            : TRANS("HighRes supports mono and stereo. Showing the classic spectrum."));
    }

    if (highres == showing_highres_ && muted == was_muted_ && requested == analysis_requested_) return;
    analysis_requested_ = requested;
    showing_highres_ = highres;
    was_muted_ = muted;
    if (!requested) controller.setAnalysisEnabled(false);
    reset();
    if (requested) controller.setAnalysisEnabled(true);
    meter_tooltip_.setVisible(highres);
}

void FxVisualizer::resetMeters()
{
    left_clip_.reset();
    right_clip_.reset();
    FxController::getInstance().consumeOutputClipEvents();
    FxController::getInstance().consumeLimiterActivity();
    updateMeterTooltip();
}

void FxVisualizer::updateMeterTooltip()
{
    const auto now = FxClipIndicator::Clock::now();
    const int left = roundToInt(left_clip_.reductionDb(now) * 10.0f);
    const int right = roundToInt(right_clip_.reductionDb(now) * 10.0f);
    const bool clipped = left_clip_.isClipped(now) || right_clip_.isClipped(now);
    if (left == tooltip_left_ && right == tooltip_right_ && clipped == tooltip_clipped_) return;
    tooltip_left_ = left;
    tooltip_right_ = right;
    tooltip_clipped_ = clipped;
    meter_tooltip_.setTooltip(TRANS("Estimated front limiter activity: L") + " " + String(left / 10.0f, 1)
        + " dB / " + TRANS("R") + " " + String(right / 10.0f, 1) + " dB. "
        + (clipped ? TRANS("Output sample peak near full scale detected.")
                   : TRANS("No output sample peak near full scale.")));
}

void FxVisualizer::lookAndFeelChanged()
{
    const auto image = Drawable::createFromImageData(FXIMAGE(FlipButton), FXIMAGESIZE(FlipButton));
    const auto hover = Drawable::createFromImageData(FXIMAGE(FlipButtonHover), FXIMAGESIZE(FlipButtonHover));
    analysis_button_.setImages(image.get(), hover.get());
    calcGradient();
    repaint();
}

void FxVisualizer::calcGradient()
{
    float alpha = 0.75;
    if (FxController::getInstance().isAudioProcessing())
    {
        alpha = 1.0;
    }

    gradient_ = ColourGradient(isEnabled() ? Colour(FXCOLOR(GraphHigh)).withAlpha(alpha) : Colour(FXCOLOR(GraphHigh)).withSaturation(0.0f).withAlpha(alpha),
        2.0f, 0.0f,
        isEnabled() ? Colour(FXCOLOR(GraphHigh)).withAlpha(alpha) : Colour(FXCOLOR(GraphHigh)).withSaturation(0.0f).withAlpha(alpha),
        2.0f, 100.0f, false);
    if (isEnabled())
    {
        gradient_.addColour(0.5f, Colour(FXCOLOR(GraphLow)).withAlpha(alpha));
    }
    else
    {
        gradient_.addColour(0.5f, Colour(FXCOLOR(GraphLow)).withSaturation(0.0f).withAlpha(alpha));
    }
}
