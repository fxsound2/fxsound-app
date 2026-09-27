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
    analysis_button_.setClickingTogglesState(true);
    analysis_button_.setTooltip(TRANS("HighRes: left above, right below. Switch off for the original spectrum."));
    analysis_button_.setColour(TextButton::buttonColourId, Colour(0xff303540));
    analysis_button_.setColour(TextButton::buttonOnColourId, Colour(0xffbc254b));
    analysis_button_.setColour(TextButton::textColourOffId, Colour(0xffb8bcc4));
    analysis_button_.setColour(TextButton::textColourOnId, Colours::white);
    analysis_button_.onClick = [this]
    {
        FxController::getInstance().setAnalysisEnabled(analysis_button_.getToggleState());
        reset();
        repaint();
    };
    analysis_button_.setToggleState(true, NotificationType::dontSendNotification);
    FxController::getInstance().setAnalysisEnabled(analysis_button_.getToggleState());
    addAndMakeVisible(analysis_button_);
    analysis_button_.setBounds(WIDTH - 94, 3, 86, 20);

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
    was_showing_ = false;
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
                    was_showing_ = false;
                    pending_bars_.reset();
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
                        reset();
                        repaint();
                        vblank_listener_.reset();
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
    was_showing_ = false;
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
    left_clip_.reset();
    right_clip_.reset();
    FxController::getInstance().consumeOutputClipEvents();
    FxController::getInstance().consumeLimiterActivity();
    for (int i = 0; i < FxController::NUM_SPECTRUM_BANDS * NUM_BARS; i++)
    {
        band_graph_.set(i, 0);
    }
}

void FxVisualizer::update()
{
    if (!isEnabled() || !isShowing())
    {
        was_showing_ = false;
        pending_bars_.reset();
        return;
    }
    if (!was_showing_)
    {
        pending_bars_.reset();
        FxController::getInstance().discardAnalyzedSpectrumPeaks();
        was_showing_ = true;
    }

    if (analysis_button_.getToggleState())
    {
        const auto clips = FxController::getInstance().consumeOutputClipEvents();
        const auto limiting = FxController::getInstance().consumeLimiterActivity();
        const auto now = FxClipIndicator::Clock::now();
        left_clip_.update(clips.left, now);
        right_clip_.update(clips.right, now);
        left_clip_.updateLimiter(limiting.leftDb, now);
        right_clip_.updateLimiter(limiting.rightDb, now);
        analysis_button_.setTooltip(TRANS("Limiter gain reduction: L") + " " + String(left_clip_.reductionDb(now), 1)
            + " dB / " + TRANS("R") + " " + String(right_clip_.reductionDb(now), 1)
            + " dB. " + TRANS("Amber from 0.5 dB, red at 6 dB. Held for 500 ms.")
            + " " + (left_clip_.isClipped(now) || right_clip_.isClipped(now)
                ? TRANS("Output sample peak near full scale detected.") : TRANS("No output sample peak near full scale.")));
        fxanalysis::BarSnapshot bars;
        if (FxController::getInstance().getAnalyzedSpectrumBands(bars))
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
        return;
    }

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
    auto bounds = getLocalBounds();

    g.setFillType(FillType(Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f)));
    g.fillRoundedRectangle(bounds.toFloat(), 8);

    g.setGradientFill(gradient_);

    // Keep maxima through coalesced UI updates. An exposure repaint without
    // new data reuses the last drawn values and never consumes the audio stream.
    if (analysis_button_.getToggleState() && pending_bars_.hasPending()
        && g.getClipBounds().contains(juce::Rectangle<int>(27, 10, 910, 100)))
        displayed_bars_ = pending_bars_.consume();

    // ------------------------------------------------------ SPECTRUM AREA - LEFT AND SIZE 
    Path barsPath;

    float x = 27;
    float dx = 9.1;

    for (auto i = 0; i < FxController::NUM_SPECTRUM_BANDS * NUM_BARS; i++)
    {
        if (analysis_button_.getToggleState())
        {
            const float centre = bounds.getHeight() * 0.5f;
            const float leftHeight = displayed_bars_.left[i] * 50.0f;
            const float rightHeight = displayed_bars_.right[i] * 50.0f;
            if (leftHeight > 0.0f) barsPath.addRectangle(x, centre - leftHeight, 4.0f, leftHeight);
            if (rightHeight > 0.0f) barsPath.addRectangle(x, centre, 4.0f, rightHeight);
            x += dx;
            continue;
        }
        float band_value = band_graph_[i] == 0.0 ? 0.01 : band_graph_[i];
        float height = band_value * 100.0f;

        barsPath.addRectangle(x, bounds.getHeight() / 2.0f - height / 2.0f, 4.0f, height);
        x += dx;
    }

    g.fillPath(barsPath);
    if (analysis_button_.getToggleState())
    {
        g.setColour(Colour(0xffb8bcc4));
        g.setFont(11.0f);
        g.drawText(TRANS("L"), 5, bounds.getHeight() / 2 - 23, 18, 16, Justification::centred);
        g.drawText(TRANS("R"), 5, bounds.getHeight() / 2 + 7, 18, 16, Justification::centred);
        const auto now = FxClipIndicator::Clock::now();
        const Colour clipOn(0xffff3045), clipOff(0xff482530);
        const Colour limiterOn(0xffffb020);
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
        const float centre = bounds.getHeight() * 0.5f;
        g.setColour(indicatorColour(left_clip_));
        g.fillEllipse(10.0f, centre - 6.0f, 8.0f, 8.0f);
        g.setColour(indicatorColour(right_clip_));
        g.fillEllipse(10.0f, centre + 24.0f, 8.0f, 8.0f);
    }
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

void FxVisualizer::lookAndFeelChanged()
{
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
