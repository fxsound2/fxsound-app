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
#include "experimental/pitch/ui/PitchMotionIndicator.h"
#include "experimental/pitch/ui/HarmonicCombIndicator.h"
#include "FrequencyScale.h"

FxVisualizer::FxVisualizer()
{
    band_values_.resize(FxController::NUM_SPECTRUM_BANDS);
    band_graph_.resize(FxController::NUM_SPECTRUM_BANDS * NUM_BARS);
    auto& settings = FxController::getInstance().getSettings();
    analysis_button_.setClickingTogglesState(false);
    analysis_button_.setToggleState(settings.getBool("highres_visualizer"), dontSendNotification);
    analysis_button_.onClick = [this] { showVisualizationMenu(); };
    analysis_button_.setTitle(TRANS("Visualizations"));
    analysis_button_.setTooltip(TRANS("Choose a spectrum, history or experimental observation view."));
    addAndMakeVisible(analysis_button_);
    addChildComponent(meter_tooltip_);
    lookAndFeelChanged();

    calcGradient();
    reset();

    setOpaque(false);
    setSize(WIDTH, HEIGHT);
}

FxVisualizer::~FxVisualizer()
{
    onPreferredHeightChanged = {};
    stopAnimation();
    FxController::getInstance().setMainSpectrumRequested(false);
    FxController::getInstance().setCompactIndicatorRequested(false);
}

void FxVisualizer::start()
{
    running_ = true;
    updateAnalysisState();
    calcGradient();
    updateAnimationClock();
}

void FxVisualizer::refreshState()
{
    running_ = FxController::getInstance().isVisualizationProcessing();
    updateAnalysisState();
    updateAnimationClock();
}

void FxVisualizer::updateAnimationClock()
{
    if (!running_ || !isShowing() || !isEnabled())
    {
        stopAnimation();
        return;
    }

#if JUCE_MAJOR_VERSION >= 8
    if (vblank_listener_ == nullptr)
    {
        vblank_listener_ = std::make_unique<juce::VBlankAttachment>(
            this,
            [this](double timestamp)
            {
                constexpr double fps_interval = 1.0 / 30.0;

                if (timestamp - last_frame_time_ >= fps_interval)
                {
                    last_frame_time_ = timestamp;
                    update();
                    repaint();
                }
            });
    }
#else
    if (!isTimerRunning()) startTimerHz(30);
#endif
}

void FxVisualizer::pause()
{
    running_ = false;
    updateAnalysisState();
    calcGradient();

    reset();
    repaint();
    stopAnimation();
}

void FxVisualizer::stopAnimation()
{
#if JUCE_MAJOR_VERSION >= 8
    vblank_listener_.reset();
    last_frame_time_ = 0.0;
#else
    stopTimer();
#endif
}

#if JUCE_MAJOR_VERSION < 8
void FxVisualizer::timerCallback() { update(); repaint(); }
#endif

void FxVisualizer::reset()
{
    pending_bars_.reset();
    displayed_bars_ = {};
    analysis_generation_ = 0;
    axis_ = {};
    hoverPosition_.reset();
    if (pitch_indicator_) pitch_indicator_->clear();
    if (harmonic_indicator_) harmonic_indicator_->clear();
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
    auto& controller = FxController::getInstance();
    updateCompactIndicator(controller);

    if (showing_highres_)
    {
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
    if (controller.getAnalyzedSpectrumBands(bars, axis_))
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
        paintFrequencyAxis(g);
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
    const float center = plotTop + plotHeight * 0.5f;
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
    const float center = plotTop + plotHeight * 0.5f;
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
    constexpr int center = plotTop + plotHeight / 2;
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
    refreshState();
    calcGradient();
    repaint();
}

void FxVisualizer::resized()
{
    constexpr int buttonSize = 18;
    analysis_button_.setBounds(getWidth() - buttonSize - 5, 5, buttonSize, buttonSize);
    meter_tooltip_.setBounds(5, plotTop + plotHeight / 2 - 6, 18, 38);
    const int stripTop = HEIGHT - 8 + (showing_highres_ ? axisHeight : 0);
    if (pitch_indicator_) pitch_indicator_->setBounds(plotLeft, stripTop, plotWidth, compactHeight);
    if (harmonic_indicator_) harmonic_indicator_->setBounds(plotLeft, stripTop, plotWidth, compactHeight);
}

void FxVisualizer::visibilityChanged()
{
    refreshState();
}

void FxVisualizer::parentHierarchyChanged()
{
    refreshState();
}

void FxVisualizer::updateAnalysisState()
{
    auto& controller = FxController::getInstance();
    const bool supported = controller.canAnalyzeOutput();
    const bool muted = controller.isOutputMuted();
    const bool highres = analysis_button_.getToggleState() && supported;
    const auto compact = controller.getCompactIndicator();
    const bool visibleAndPlaying = running_ && isEnabled() && isShowing()
        && !muted && controller.isVisualizationProcessing() && supported;
    const bool requested = visibleAndPlaying && highres;
    const bool compactRequested = visibleAndPlaying && compact != FxCompactIndicator::none;
    if (supported != output_supported_)
    {
        output_supported_ = supported;
        analysis_button_.setTooltip(supported ? TRANS("Choose a spectrum, history or experimental observation view.")
            : TRANS("HighRes supports mono and stereo. Showing the classic spectrum."));
    }

    const auto outputGeneration = controller.outputObservationGeneration();
    if (highres == showing_highres_ && muted == was_muted_ && requested == analysis_requested_
        && compact == compact_mode_ && compactRequested == compact_requested_
        && outputGeneration == output_observation_generation_) return;
    // An endpoint change can leave the channel count unchanged. Clear the old
    // bar peaks and indicator holds even when HighRes remains selected.
    output_observation_generation_ = outputGeneration;
    analysis_requested_ = requested;
    compact_requested_ = compactRequested;
    compact_mode_ = compact;
    showing_highres_ = highres;
    was_muted_ = muted;
    if (!requested) controller.setMainSpectrumRequested(false);
    if (!compactRequested) controller.setCompactIndicatorRequested(false);
    reset();
    if (requested) controller.setMainSpectrumRequested(true);
    if (compactRequested) controller.setCompactIndicatorRequested(true);
    ensureCompactIndicator();
    if (pitch_indicator_) pitch_indicator_->setVisible(compact == FxCompactIndicator::pitch);
    if (harmonic_indicator_) harmonic_indicator_->setVisible(compact == FxCompactIndicator::carriers);
    meter_tooltip_.setVisible(highres && isShowing());
    updatePreferredSize();
}

int FxVisualizer::preferredHeight() const
{
    return HEIGHT + (showing_highres_ ? axisHeight : 0)
        + (compact_mode_ != FxCompactIndicator::none ? compactExtraHeight : 0);
}

void FxVisualizer::updatePreferredSize()
{
    const int height = preferredHeight();
    if (height == getHeight()) return;
    setSize(WIDTH, height);
    if (onPreferredHeightChanged) onPreferredHeightChanged();
}

void FxVisualizer::setHighResSelected(bool selected)
{
    analysis_button_.setToggleState(selected, dontSendNotification);
    FxController::getInstance().getSettings().setBool("highres_visualizer", selected);
    refreshState();
    repaint();
}

void FxVisualizer::showVisualizationMenu()
{
    const auto compact = FxController::getInstance().getCompactIndicator();
    PopupMenu menu, strip;
    menu.addItem(1, TRANS("Classic spectrum"), true, !analysis_button_.getToggleState());
    menu.addItem(2, TRANS("HighRes spectrum and frequency axis"), true, analysis_button_.getToggleState());
    menu.addSeparator();
    menu.addItem(10, TRANS("Spectrum history..."));
    menu.addItem(11, TRANS("Carrier history..."));
    menu.addItem(12, TRANS("Experimental pitch analysis..."));
    strip.addItem(20, TRANS("None"), true, compact == FxCompactIndicator::none);
    strip.addItem(21, TRANS("Pitch movement"), true, compact == FxCompactIndicator::pitch);
    strip.addItem(22, TRANS("Carriers"), true, compact == FxCompactIndicator::carriers);
    menu.addSubMenu(TRANS("Compact observation band"), strip);
    menu.showMenuAsync(PopupMenu::Options().withTargetComponent(&analysis_button_),
        [safe = Component::SafePointer<FxVisualizer>(this)](int choice)
        {
            if (safe == nullptr) return;
            auto& controller = FxController::getInstance();
            switch (choice)
            {
                case 1: safe->setHighResSelected(false); break;
                case 2: safe->setHighResSelected(true); break;
                case 10: controller.showSpectrumHistory(); break;
                case 11: controller.showCarrierHistory(); break;
                case 12: controller.showPitchExperiment(); break;
                case 20: controller.setCompactIndicator(FxCompactIndicator::none); break;
                case 21: controller.setCompactIndicator(FxCompactIndicator::pitch); break;
                case 22: controller.setCompactIndicator(FxCompactIndicator::carriers); break;
                default: break;
            }
        });
}

void FxVisualizer::ensureCompactIndicator()
{
    using namespace fxanalysis::experimental;
    if (compact_mode_ == FxCompactIndicator::pitch && !pitch_indicator_)
    {
        pitch_indicator_ = std::make_unique<PitchMotionIndicator>();
        pitch_indicator_->setCompact(true);
        pitch_indicator_->onDoubleClick = [] { FxController::getInstance().showPitchExperiment(); };
        addChildComponent(*pitch_indicator_);
    }
    if (compact_mode_ == FxCompactIndicator::carriers && !harmonic_indicator_)
    {
        harmonic_snapshot_ = std::make_unique<HarmonicCombSnapshot>();
        harmonic_indicator_ = std::make_unique<HarmonicCombIndicator>();
        harmonic_indicator_->setCompact(true);
        harmonic_indicator_->onDoubleClick = [] { FxController::getInstance().showPitchExperiment(); };
        addChildComponent(*harmonic_indicator_);
    }
    resized();
}

void FxVisualizer::updateCompactIndicator(FxController& controller)
{
    if (!compact_requested_) return;
    const auto& settings = controller.getPitchObservationSettings();
    if (compact_mode_ == FxCompactIndicator::pitch && pitch_indicator_)
    {
        fxanalysis::experimental::PitchMotionSnapshot snapshot;
        pitch_indicator_->setTrajectoryLimitSemitones(settings.integratorLimitSemitones);
        if (controller.getPitchMotion(snapshot)) pitch_indicator_->setSnapshot(snapshot);
        else pitch_indicator_->clear();
    }
    else if (compact_mode_ == FxCompactIndicator::carriers && harmonic_indicator_ && harmonic_snapshot_)
    {
        harmonic_indicator_->setSettings(settings.harmonicComb);
        if (controller.getHarmonicComb(*harmonic_snapshot_)) harmonic_indicator_->setSnapshot(*harmonic_snapshot_);
        else harmonic_indicator_->setUnavailable();
    }
}

void FxVisualizer::paintFrequencyAxis(Graphics& g)
{
    if (axis_.sampleRate <= 0.0) return;
    const double maximumHz = axis_.bandEdgesHz.back();
    if (!(maximumHz > fxanalysis::FrequencySettings::minimumHz)) return;
    const fxanalysis::FrequencyScale scale(maximumHz);
    g.setColour(Colour(FXCOLOR(GraphHigh)));
    g.setFont(10.0f);
    int previousRight = plotLeft - 8;
    for (const double hz : {50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0})
    {
        if (hz > maximumHz) continue;
        const int x = plotLeft + roundToInt(scale.position(hz) * plotWidth);
        const String text = hz < 1000.0 ? String(roundToInt(hz)) : String(hz / 1000.0, 0) + "k";
        const int width = hz < 1000.0 ? 28 : 30;
        const int left = jlimit(plotLeft, plotLeft + plotWidth - width, x - width / 2);
        if (left < previousRight + 4) continue;
        g.drawVerticalLine(x, static_cast<float>(HEIGHT - 9), static_cast<float>(HEIGHT - 6));
        g.drawText(text, left, HEIGHT - 5, width, axisHeight - 3, Justification::centred);
        previousRight = left + width;
    }
    if (hoverPosition_)
    {
        const int band = jlimit(0, fxanalysis::displayBandCount - 1,
            static_cast<int>(*hoverPosition_ * fxanalysis::displayBandCount));
        const double centerHz = axis_.bandHz[band];
        const int x = plotLeft + roundToInt(*hoverPosition_ * plotWidth);
        g.setColour(Colour(FXCOLOR(GraphHigh)).withAlpha(0.45f));
        g.drawVerticalLine(x, plotTop, plotTop + plotHeight);
        g.setColour(Colour(FXCOLOR(ControlBackground)).withAlpha(0.95f));
        const int labelLeft = jlimit(plotLeft, plotLeft + plotWidth - 150, x - 75);
        g.fillRect(labelLeft, plotTop, 150, 17);
        g.setColour(Colour(FXCOLOR(GraphHigh)));
        g.drawText(TRANS("Band centre") + ": " + String(centerHz, 1) + " Hz",
            labelLeft, plotTop, 150, 17, Justification::centred);
    }
}

void FxVisualizer::mouseMove(const MouseEvent& event)
{
    if (showing_highres_ && juce::Rectangle<int>(plotLeft, plotTop, plotWidth, plotHeight).contains(event.getPosition()))
        hoverPosition_ = jlimit(0.0, 1.0, (event.position.x - plotLeft) / static_cast<double>(plotWidth));
    else hoverPosition_.reset();
    repaint();
}

void FxVisualizer::mouseExit(const MouseEvent&) { hoverPosition_.reset(); repaint(); }

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
