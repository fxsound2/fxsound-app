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

    start();

    setOpaque(false);
    setSize(WIDTH, HEIGHT);
}

void FxVisualizer::start()
{    
    calcGradient();

    if (vblank_listener_ == nullptr)
    {
        vblank_listener_ = std::make_unique<juce::VBlankAttachment>(
            this,
            [this](double timestamp)
            {
                if (!isShowing())
                    return;

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
}

void FxVisualizer::pause()
{
    calcGradient();

    reset();
    repaint();

    if (vblank_listener_ != nullptr)
    {
        vblank_listener_.reset();
    }
}

void FxVisualizer::reset()
{
    for (int i = 0; i < FxController::NUM_SPECTRUM_BANDS * NUM_BARS; i++)
    {
        band_graph_.set(i, 0);
    }
}

void FxVisualizer::update()
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
    auto bounds = getLocalBounds();

    g.setFillType(FillType(Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f)));
    g.fillRoundedRectangle(bounds.toFloat(), 8);

    g.setGradientFill(gradient_);

    // ------------------------------------------------------ SPECTRUM AREA - LEFT AND SIZE 
    Path barsPath;

    float x = 27;
    float dx = 9.1;

    for (auto i = 0; i < FxController::NUM_SPECTRUM_BANDS * NUM_BARS; i++)
    {
        float band_value = band_graph_[i] == 0.0 ? 0.01 : band_graph_[i];
        float height = band_value * 100.0f;

        barsPath.addRectangle(x, bounds.getHeight() / 2.0f - height / 2.0f, 4.0f, height);
        x += dx;
    }

    g.fillPath(barsPath);
}

void FxVisualizer::enablementChanged()
{
    calcGradient();
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