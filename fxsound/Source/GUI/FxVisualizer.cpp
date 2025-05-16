/*
FxSound
Copyright (C) 2025  FxSound LLC

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

    reset();

    setFramesPerSecond(30);

    setOpaque(false);
    setSize(WIDTH, HEIGHT);
}

void FxVisualizer::start()
{
    setFramesPerSecond(30);
}

void FxVisualizer::pause()
{
    setFramesPerSecond(10);
}

void FxVisualizer::reset()
{
    for (int i = 0; i < FxController::NUM_SPECTRUM_BANDS * 11; i++)
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

    float x = 55;
    for (auto i = 0; i < FxController::NUM_SPECTRUM_BANDS*NUM_BARS; i++)
    {
        float band_value = band_graph_[i];
        float height = band_value * 100.0f;

        juce::Rectangle<float> rect{ x, bounds.getHeight() / 2 - height / 2, 4, height };        
        g.fillRect(rect);

        x += 7;
    }
}

#if 0
void FxVisualizer::paint(Graphics& g)
{
    auto bounds = getLocalBounds();
    g.setFillType(FillType(Colour(0x0).withAlpha(0.2f)));
    g.fillRoundedRectangle(bounds.toFloat(), 8);

    g.setFillType(FillType(Colour(0xe33250).withAlpha(1.0f)));

    float x = 80;
    for (auto index = 0; index < FxController::NUM_SPECTRUM_BANDS; index++)
    {
        float band_value = band_values_[index];
        float height = 0;

        x += 4;

        if (band_value > 0 || band_value <= 1)
        {
            for (auto i = 1; i <= 5; i++)
            {
                height = std::sin((band_value / 5 * i) * (3.14159265 / 2)) * 100;
                juce::Rectangle<float> rect{ x, bounds.getHeight() / 2 - height / 2, 4, height };
                g.fillRect(rect);
                x += 8;
            }

            for (auto i = 4; i >= 1; i--)
            {
                height = std::sin((band_value / 5 * i) * (3.14159265 / 2)) * 100;
                juce::Rectangle<float> rect{ x, bounds.getHeight() / 2 - height / 2, 4, height };
                g.fillRect(rect);
                x += 8;
            }
        }
        else
        {
            x += 8 * 9;
        }
    }
}
#endif