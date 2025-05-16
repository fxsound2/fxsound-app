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
#include "FxTheme.h"

//==============================================================================
/*
*/
class FxWindow : public Component
{
public:
	FxWindow(String name = "");
	~FxWindow();

	void setContent(Component* content);
	virtual void closeButtonPressed() = 0;

	void startLogoAnimation();
	void stopLogoAnimation();
	void addToolbarButton(Button* toolbarButton);

    void enableShadow(bool enable);
    bool isShadowEnabled();

protected:
    static constexpr int SHADOW_WIDTH = 5;
	static constexpr int CLOSE_BUTTON_WIDTH = 15;

	void paint(Graphics&) override;
	void resized() override;

	class CloseButton : public Button
	{
	public:
		CloseButton() : Button("fxClose") {}
		~CloseButton() = default;

	private:
		void paintButton(Graphics& g, bool, bool) override;
	};

	class TitleBar : public Component, private Button::Listener
	{
	public:
		TitleBar(String name);
		~TitleBar() = default;

		void startLogoAnimation();
		void stopLogoAnimation();
		void addToolbarButton(Button* toolbarButton);

	private:
		static constexpr int ICON_WIDTH = 86;
		static constexpr int ICON_HEIGHT = 13;

		void paint(Graphics& g) override;
		void resized() override;

		void buttonClicked(Button* button) override;

		void mouseDown(const MouseEvent& e);
		void mouseDrag(const MouseEvent& e);
		void mouseUp(const MouseEvent&);

		String name_;
		std::unique_ptr<Drawable> icon_;
		std::unique_ptr<Drawable> animation_icon_;
		Label title_;
		std::vector<Button*> toolbar_buttons_;
		CloseButton close_button_;
		ComponentDragger dragger_;
		bool dragging_;
	};

	Component* content_;
	TitleBar title_bar_;
    
    bool draw_shadow_;
    int shadow_width_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxWindow)
};
