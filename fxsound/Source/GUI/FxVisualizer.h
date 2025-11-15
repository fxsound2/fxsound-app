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

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class FxVisualizer : public Component
{
public:
    FxVisualizer();

    void start();
    void pause();

    void reset();
    void update();

    void calcGradient();

private:
    static constexpr int WIDTH = 960;
    static constexpr int HEIGHT = 120;
    static constexpr int NUM_BARS = 10;

    void paint(Graphics& g) override;
    void enablementChanged() override;
	void lookAndFeelChanged() override;

    Array<float> band_values_;
    Array<float> band_graph_;
    ColourGradient gradient_;

    std::unique_ptr<juce::VBlankAttachment> vblank_listener_;
};