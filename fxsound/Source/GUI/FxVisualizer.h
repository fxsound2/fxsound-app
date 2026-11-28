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
#if JUCE_MAJOR_VERSION >= 8
class FxVisualizer : public Component
#else
class FxVisualizer : public Component, private Timer
#endif
{
public:
    FxVisualizer();
    ~FxVisualizer() override;

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
    void visibilityChanged() override;
    void parentHierarchyChanged() override;
    void lookAndFeelChanged() override;
    void updateAnimationState();
    void stopAnimation();
#if JUCE_MAJOR_VERSION < 8
    void timerCallback() override;
#endif

    Array<float> band_values_;
    Array<float> band_graph_;
    ColourGradient gradient_;
    bool playback_active_ = false;

#if JUCE_MAJOR_VERSION >= 8
    std::unique_ptr<juce::VBlankAttachment> vblank_listener_;
    double last_frame_time_ = 0.0;
#endif
};
