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
#include "FxView.h"

//==============================================================================
/*
*/
class FxLiteView : public FxView
{
public:
    FxLiteView();
    ~FxLiteView() = default;

    void resized() override;
	void paint(Graphics& g) override;

private:
	static constexpr int PRESET_LIST_X = 40;
	static constexpr int OUTPUT_LIST_X = 285;
	static constexpr int LIST_Y = 42;
    static constexpr int BACKGROUND_WIDTH = LIST_WIDTH*2 + 20*3;
    static constexpr int BACKGROUND_HEIGHT = LIST_HEIGHT + 40;
    static constexpr int WIDTH = BACKGROUND_WIDTH + 40;
    static constexpr int HEIGHT = BACKGROUND_HEIGHT + 22;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxLiteView)
};
