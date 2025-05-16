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

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class FxEqualizer : public Component, Slider::Listener, Timer
{
public:
	FxEqualizer();
	~FxEqualizer() = default;

    void sliderValueChanged(Slider*slider) override;
    void sliderDragStarted(Slider* slider) override;
    void sliderDragEnded(Slider* slider) override;

    void timerCallback() override;

	void update();
	void showValues(bool show);

private:
    class FxEqSlider : public Slider
    {
    public:
        FxEqSlider(int band, float max_gain);
        ~FxEqSlider() = default;

        void enablementChanged() override;

        void setGainValue(float value);
        void showValue(bool show);

    private:
        static constexpr int LABEL_HEIGHT = 12;

        void resized() override;
        void valueChanged() override;
        bool keyPressed(const KeyPress& key) override;

        Label gain_label_;

        int band_;
    };

    class FxBandCenterFreqSlider : public Slider
    {
    public:
        FxBandCenterFreqSlider(int band, Label& freq_label);
        ~FxBandCenterFreqSlider() = default;

        void setFrequency(float value);

        void enablementChanged() override;

    private:
        void valueChanged() override;
        bool keyPressed(const KeyPress& key) override;
        
        Label& freq_label_;

        int band_;
    };

	static constexpr int WIDTH = 696;
	static constexpr int HEIGHT = 242;
	static constexpr int SLIDER_HEIGHT = 170;
	static constexpr int LABEL_HEIGHT = 12;
    static constexpr int ROTARY_SLIDER_HEIGHT = 36;
	static constexpr int X_MARGIN = 16;
	static constexpr int Y_MARGIN = 8;

	static constexpr float MAX_GAIN = 12.0f;

	void resized() override;
	void paint(Graphics& g) override;

	std::vector<std::unique_ptr<Label>> labels_;
    std::vector<std::unique_ptr<FxEqSlider>> band_boosts_;
    std::vector<std::unique_ptr<FxBandCenterFreqSlider>> center_frequencies_;
    std::vector<float> band_gain_values_;

    bool highlight_mode_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxEqualizer)
};
