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

#include "FxEqualizer.h"
#include "FxController.h"
#include "FxTheme.h"

FxEqualizer::FxEqualizer()
{
    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
    auto& controller = FxController::getInstance();

    int num_bands = controller.getNumEqBands();
    labels_.resize(num_bands);
    band_boosts_.resize(num_bands);
    center_frequencies_.resize(num_bands);
    band_gain_values_.resize(num_bands);

    float min_freq, max_freq;
    for (int i = 0; i < num_bands; i++)
    {
        labels_[i].reset(new Label());
        band_boosts_[i].reset(new FxEqSlider(i, MAX_GAIN));      
        center_frequencies_[i].reset(new FxBandCenterFreqSlider(i, *labels_[i]));

        if (i == 0)
        {
            continue;
        }

        labels_[i]->setFont(theme.getNormalFont().withHeight(LABEL_HEIGHT));
        labels_[i]->setJustificationType(Justification::centredTop);
        addAndMakeVisible(labels_[i].get());

        band_boosts_[i]->setSliderStyle(Slider::LinearVertical);
        band_boosts_[i]->setRange(-MAX_GAIN, MAX_GAIN, 1.0);
        band_boosts_[i]->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        band_boosts_[i]->addListener(this);
        addAndMakeVisible(band_boosts_[i].get());
       
        center_frequencies_[i]->setRotaryParameters(3.66519f, 8.90118f, true);
        FxController::getInstance().getEqBandFrequencyRange(i, &min_freq, &max_freq);
        center_frequencies_[i]->setRange(min_freq, max_freq, (max_freq-min_freq)/100);
        center_frequencies_[i]->setFrequency(controller.getEqBandFrequency(i));
        addAndMakeVisible(center_frequencies_[i].get());

        band_gain_values_[i] = 0;
    }
    
    highlight_mode_ = false;

    setSize(WIDTH, HEIGHT);
}

void FxEqualizer::sliderValueChanged(Slider* slider)
{

}

void FxEqualizer::sliderDragStarted(Slider* slider)
{
    if (highlight_mode_)
    {
        return;
    }

    if (ModifierKeys::getCurrentModifiersRealtime().isAltDown())
    {
        auto& controller = FxController::getInstance();

        highlight_mode_ = true;

        startTimerHz(30);

        for (auto i = 1; i < band_boosts_.size(); i++)
        {
            if (band_boosts_[i].get() != slider)
            {
                band_boosts_[i]->setEnabled(false);
                band_boosts_[i]->showValue(false);

                band_gain_values_[i] = controller.getEqBandBoostCut(i);
            }
        }
    }
}

void FxEqualizer::sliderDragEnded(Slider* slider)
{
    if (highlight_mode_)
    {
        auto& controller = FxController::getInstance();
        highlight_mode_ = false;

        for (auto i = 1; i < band_boosts_.size(); i++)
        {
            if (band_boosts_[i].get() != slider)
            {
                band_boosts_[i]->setEnabled(true);
                band_boosts_[i]->showValue(true);

                controller.setEqBandBoostCut(i, band_gain_values_[i]);
                band_boosts_[i]->setValue(band_gain_values_[i], NotificationType::dontSendNotification);
            }
        }
    }
}

void FxEqualizer::timerCallback()
{
    auto& controller = FxController::getInstance();

    if (highlight_mode_)
    {
        auto done = false;
        for (auto i = 1; i < band_boosts_.size(); i++)
        {
            if (!band_boosts_[i]->isEnabled())
            {
                auto gain = controller.getEqBandBoostCut(i);
                if (gain != -(MAX_GAIN-2))
                {
                    done = true;
                    if (gain < -(MAX_GAIN-2))
                    {
                        controller.setEqBandBoostCut(i, gain+1.0f);
                        band_boosts_[i]->setValue(gain+1.0f, NotificationType::dontSendNotification);
                    }
                    else
                    {
                        controller.setEqBandBoostCut(i, gain-1.0f);
                        band_boosts_[i]->setValue(gain-1.0f, NotificationType::dontSendNotification);
                    }
                }
            }            
        }

        if (!done)
        {
            stopTimer();
        }
    }
    else
    {
        stopTimer();
    }
}

void FxEqualizer::update()
{
    auto& controller = FxController::getInstance();

    for (auto i=1; i<band_boosts_.size(); i++)
    {
        auto value = controller.getEqBandBoostCut(i);
        if (value >= -MAX_GAIN && value <= MAX_GAIN)
        {
            band_boosts_[i]->setGainValue(value);
        }

        auto freq = controller.getEqBandFrequency(i);
        center_frequencies_[i]->setFrequency(freq);
    }
}

void FxEqualizer::showValues(bool show)
{
    for (int i = 1; i < band_boosts_.size(); i++)
    {
        band_boosts_[i]->showValue(show);
    }
}

void FxEqualizer::resized()
{
    auto bounds = getLocalBounds();

    int x = X_MARGIN;
    int width = (bounds.getWidth() - X_MARGIN * 2) / (labels_.size() - 1);
    int rotary_width = jmin(ROTARY_SLIDER_HEIGHT, width);
    for (auto i=1; i<labels_.size(); i++)
    {
        band_boosts_[i]->setBounds(x+(width-FxTheme::SLIDER_THUMB_RADIUS*4)/2, Y_MARGIN, FxTheme::SLIDER_THUMB_RADIUS*4, SLIDER_HEIGHT);
        labels_[i]->setBounds(x, band_boosts_[i]->getBottom()+6, width, LABEL_HEIGHT);
        center_frequencies_[i]->setBounds(x + (width-rotary_width)/2, labels_[i]->getBottom() + 4, rotary_width, rotary_width);
        x += width;
    }
}

void FxEqualizer::paint(Graphics& g)
{
    StringArray tool_tips = { TRANS(""),
                              TRANS("Super-low Bass. Increase this for more rumble and \"thump\", decrease if there's too much boominess."),
                              TRANS("Center of your Bass sound. Increase this for a fuller low end, decrease if the bass sounds overwhelming."),
                              TRANS("The low end of your mid-range. Increase this to make vocals sound rich and warm, decrease it to help control instruments that sound loud and muffled."),
                              TRANS("A focal point of the low-mid-range. Increase this to bring out electric guitars and vocal volume, decrease it to reduce any \"boxy\" tones."),
                              TRANS("The center mid-range band. Increase this to drastically boost rhythm instruments and snare hits, reduce it to cut out \"nasal\" tones."),
                              TRANS("The high-mid-range. Increase this to get more instrumental harmonics, reduce it to improve drums that have too much \"clickiness\" or orchestral instruments that are piercing."),
                              TRANS("The lower end of the high-end range. Increase this for more vocal clarity and articulation, reduce it and move the frequency wheel up and down to find and cut out overly loud \"S\" and \"T\" sounds."),
                              TRANS("The core high-end range. Increase this to make your audio sound more like it's in an airy, large space, reduce it to help with room noises and unwanted echoing."),
                              TRANS("The highest range of average human hearing. Increase this to give your sound more of a crisp tone, with lots of overtones. Reduce it to remove hiss or painfully high sounds.") };

    String center_freq_tip = TRANS("This wheel allows you to adjust which frequencies this EQ band is affecting\r\n"
                                   "up or down to target different frequencies/pitches. The EQ slider above\r\n"
                                   "controls the volume of this EQ band. Increase or decrease to boost or cut\r\n" 
                                   "a portion of your audio's frequencies, without modifying the rest of your sound.");

    g.setFillType(FillType(Colour(0x0f0f0f).withAlpha(1.0f)));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    Path path;

    Colour line_colour = Colour(0xffe33250);
    Colour gradient_colour_1 = Colour(0xef4b65).withAlpha(0.34f);
    Colour gradient_colour_2 = Colour(0x742834).withAlpha(0.0f);

    if (!isEnabled())
    {
        line_colour = line_colour.withSaturation(0.0);
        gradient_colour_1 = gradient_colour_1.withSaturation(0.0);
        gradient_colour_2 = gradient_colour_2.withSaturation(0.0);
    }
    else
    {
        auto& controller = FxController::getInstance();

        int num_bands = controller.getNumEqBands();
        for (int i = 0; i < num_bands; i++)
        {
            if (!controller.isHelpTooltipsHidden())
            {
                band_boosts_[i]->setTooltip(tool_tips[i]);
                center_frequencies_[i]->setTooltip(center_freq_tip);
            }
            else
            {
                band_boosts_[i]->setTooltip("");
                center_frequencies_[i]->setTooltip("");
            }
        }
    }

    if (highlight_mode_)
    {
        gradient_colour_2 = gradient_colour_2.withSaturation(0.0);
    }

    for (auto i = 1; i < band_boosts_.size() - 1; i++)
    {
        auto y0 = band_boosts_[i]->getPositionOfValue(band_boosts_[i]->getValue()) + Y_MARGIN;
        auto y1 = band_boosts_[i + 1]->getPositionOfValue(band_boosts_[i + 1]->getValue()) + Y_MARGIN;

        auto x0 = band_boosts_[i]->getX() + band_boosts_[i]->getWidth() / 2;
        auto x1 = band_boosts_[i + 1]->getX() + band_boosts_[i + 1]->getWidth() / 2;

        path.addLineSegment(Line<float>(x0, y0, x1, y1), 1.0);

        if (band_boosts_[i]->isEnabled() || band_boosts_[i + 1]->isEnabled())
        {
            g.setColour(line_colour);
        }
        else
        {
            g.setColour(line_colour.withSaturation(0.0));
        }
        g.strokePath(path, PathStrokeType(1.0));
        path.clear();
    }

    for (auto i = 1; i < band_boosts_.size(); i++)
    {
        auto x = band_boosts_[i]->getX() + band_boosts_[i]->getWidth() / 2;
        auto y = band_boosts_[i]->getPositionOfValue(band_boosts_[i]->getValue()) + Y_MARGIN;

        if (i == 1)
        {
            path.startNewSubPath(Point<float>(x, band_boosts_[i]->getBottom() - FxTheme::SLIDER_THUMB_RADIUS));
        }

        path.lineTo(x, y);

        if (i == band_boosts_.size() - 1)
        {
            path.lineTo(x, band_boosts_[i]->getBottom() - FxTheme::SLIDER_THUMB_RADIUS);
        }
    }
    path.closeSubPath();

    auto gradient = ColourGradient(gradient_colour_1, 0, band_boosts_[1]->getY(), gradient_colour_2, 0, band_boosts_[1]->getBottom(), false);
    g.setFillType(FillType(gradient));
    g.fillPath(path);
}

FxEqualizer::FxEqSlider::FxEqSlider(int band, float max_gain)
{
    band_ = band;

    setMouseCursor(MouseCursor::PointingHandCursor);

    auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

    gain_label_.setFont(theme.getNormalFont().withHeight(LABEL_HEIGHT));
    gain_label_.setJustificationType(Justification::centred);
    addChildComponent(gain_label_);

    setWantsKeyboardFocus(true);
}

void FxEqualizer::FxEqSlider::setGainValue(float value)
{
    setValue(value, NotificationType::dontSendNotification);

    auto text = String::formatted("%+.0f", value);
    gain_label_.setText(text, NotificationType::dontSendNotification);

    auto y = getPositionOfValue(value) - (FxTheme::SLIDER_THUMB_RADIUS *3);
    gain_label_.setBounds(gain_label_.getBounds().withY(y));
}

void FxEqualizer::FxEqSlider::showValue(bool show)
{
    gain_label_.setVisible(show && isEnabled());
}

void FxEqualizer::FxEqSlider::enablementChanged()
{
    if (isEnabled())
    {
        setMouseCursor(MouseCursor::PointingHandCursor);
    }
    else
    {
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void FxEqualizer::FxEqSlider::resized()
{
    Slider::resized();
    gain_label_.setBounds(gain_label_.getX(), gain_label_.getY(), getWidth(), LABEL_HEIGHT);
}

void FxEqualizer::FxEqSlider::valueChanged()
{
    auto value = getValue();
    if (value != FxController::getInstance().getEqBandBoostCut(band_))
    {
        FxController::getInstance().setEqBandBoostCut(band_, value);

        auto text = String::formatted("%+.0f", value);
        gain_label_.setText(text, NotificationType::dontSendNotification);

        auto y = getPositionOfValue(value) - (FxTheme::SLIDER_THUMB_RADIUS*3);
        gain_label_.setBounds(gain_label_.getBounds().withY(y));
    }
}

bool FxEqualizer::FxEqSlider::keyPressed(const KeyPress& key)
{
    if (isEnabled())
    {
        if (key.isKeyCode(KeyPress::upKey))
        {
            setValue(getValue() + getInterval());
            return true;
        }
        else if (key.isKeyCode(KeyPress::downKey))
        {
            setValue(getValue() - getInterval());
            return true;
        }
    }

    return false;
}

FxEqualizer::FxBandCenterFreqSlider::FxBandCenterFreqSlider(int band, Label& freq_label) : Slider(Slider::SliderStyle::Rotary, Slider::TextEntryBoxPosition::NoTextBox), freq_label_(freq_label)
{
    band_ = band;

    setMouseCursor(MouseCursor::PointingHandCursor);

    setWantsKeyboardFocus(true);
}

void FxEqualizer::FxBandCenterFreqSlider::setFrequency(float value)
{
    setValue(value, NotificationType::dontSendNotification);

    String text;
    if (value > 0)
    {
        if (value > 1000)
        {
            text = String::formatted("%.2f kHz", value/1000.0f);
        }
        else
        {
            text = String::formatted("%.0f Hz", value);
        }

        freq_label_.setText(text, NotificationType::dontSendNotification);
    }
}

void FxEqualizer::FxBandCenterFreqSlider::enablementChanged()
{
    if (isEnabled())
    {
        setMouseCursor(MouseCursor::PointingHandCursor);
    }
    else
    {
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void FxEqualizer::FxBandCenterFreqSlider::valueChanged()
{
    auto freq = getValue();

    if (freq != FxController::getInstance().getEqBandFrequency(band_))
    {
        FxController::getInstance().setEqBandFrequency(band_, freq);

        String text;
        if (freq > 0)
        {
            if (freq > 1000)
            {
                text = String::formatted("%.2f kHz", freq / 1000.0f);
            }
            else
            {
                text = String::formatted("%.0f Hz", freq);
            }

            freq_label_.setText(text, NotificationType::dontSendNotification);
        }
    }
}

bool FxEqualizer::FxBandCenterFreqSlider::keyPressed(const KeyPress& key)
{
    if (isEnabled())
    {
        if (key.isKeyCode(KeyPress::upKey))
        {
            setValue(getValue() + getInterval());
            return true;
        }
        else if (key.isKeyCode(KeyPress::downKey))
        {
            setValue(getValue() - getInterval());
            return true;
        }
    }

    return false;
}
