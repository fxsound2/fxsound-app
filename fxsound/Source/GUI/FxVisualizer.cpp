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
    if (vblank_handler_ == nullptr)
    {
        vblank_handler_ = std::make_unique<juce::VBlankAttachment>(
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
                        vblank_handler_.reset();
                    }
                }
            });
    }
}

void FxVisualizer::pause()
{
    reset();
    repaint();

    if (vblank_handler_ != nullptr)
    {
        vblank_handler_.reset();
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

    g.setFillType(FillType(Colour(0x0f0f0f).withAlpha(1.0f)));
    g.fillRoundedRectangle(bounds.toFloat(), 8);

    float alpha = 0.75;
    if (FxController::getInstance().isAudioProcessing())
    {
        alpha = 1.0;
    }
    
    ColourGradient gradient(isEnabled() ? Colour(0xd51535).withAlpha(alpha) : Colour(0xd51535).withSaturation(0.0f).withAlpha(alpha),
                            2.0f, 0.0f, 
                            isEnabled() ? Colour(0xd51535).withAlpha(alpha) : Colour(0xd51535).withSaturation(0.0f).withAlpha(alpha),
                            2.0f, 100.0f, false);
    if (isEnabled())
    {
        gradient.addColour(0.5f, Colour(0xfe566a).withAlpha(alpha));
    }
    else
    {
        gradient.addColour(0.5f, Colour(0xfe566a).withSaturation(0.0f).withAlpha(alpha));
    }
    g.setGradientFill(gradient);

    // ------------------------------------------------------ SPECTRUM AREA - LEFT AND SIZE 
    float x = 27;
    float dx = 9.1;

    for (auto i = 0; i < FxController::NUM_SPECTRUM_BANDS * NUM_BARS; i++)
    {
        float band_value = band_graph_[i] == 0.0 ? 0.01 : band_graph_[i];
        float height = band_value * 100.0f;

        juce::Rectangle<float> rect{ x, bounds.getHeight() / 2 - height / 2, 4, height };
        g.fillRect(rect);

        x += dx;
    }
}