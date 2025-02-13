/*
FxSound
Copyright (C) 2023  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <JuceHeader.h>
#include "FxWindow.h"
#include "FxLiteView.h"
#include "FxProView.h"
#include "FxPowerButton.h"

//==============================================================================
/*
	This class implements the desktop window that contains an instance of
	our MainComponent class.
*/
class FxMainWindow : public FxWindow, private Button::Listener, public FxModel::Listener
{
public:
    FxMainWindow();
    ~FxMainWindow();

    void show();
    void showLiteView();
    void showProView();
    void updateView();
    void startVisualizer();
    void pauseVisualizer();

    void setResizeImage();
    void setIcon(bool power, bool processing);

    bool keyPressed(const KeyPress& key) override;

private:
    static constexpr int BUTTON_WIDTH = 24;
    static constexpr int DONATE_BUTTON_WIDTH = 80;
    static constexpr int DONATE_BUTTON_HEIGHT = 24;

    void showMenu();
    void buttonClicked(Button* button) override;
    void mouseEnter(const MouseEvent&) override;
    void modelChanged(FxModel::Event model_event) override;
	void userTriedToCloseWindow() override;
	void closeButtonPressed() override;

	FxLiteView lite_view_;
	FxProView  pro_view_;
    FxPowerButton power_button_;
    DrawableButton menu_button_;
    DrawableButton resize_button_;
    DrawableButton minimize_button_;
    TextButton donate_button_;
    BubbleMessageComponent help_bubble_;

    std::unique_ptr<Drawable> menu_image_;
    std::unique_ptr<Drawable> menu_hover_image_;
    std::unique_ptr<Drawable> resize_image_;
    std::unique_ptr<Drawable> resize_hover_image_;
    std::unique_ptr<Drawable> minimize_image_;
    std::unique_ptr<Drawable> minimize_hover_image_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxMainWindow)
};
