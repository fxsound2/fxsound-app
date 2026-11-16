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
#include "FxDrawable.h"

//==============================================================================
// JUCE version compatibility.
// Older JUCE (6.x / 8.x) is still used for builds that must support Windows 7, so
// API changes from JUCE 9 are handled behind these checks rather than replaced outright.

// JUCE 9.0.5: Component::addToDesktop() is no longer virtual; override createDesktopWindow() instead.
// (Drawable compatibility for JUCE 9 is in FxDrawable.h.)
#define FX_JUCE_HAS_CREATE_DESKTOP_WINDOW (JUCE_VERSION >= 0x090005)

//==============================================================================
/*
*/
class FxWindow : public Component
{
public:
	FxWindow(String name = "");
	~FxWindow();

#if ! FX_JUCE_HAS_CREATE_DESKTOP_WINDOW
	void addToDesktop(int window_style_flags, void* native_window_to_attach_to = nullptr) override;
#endif
	void setContent(Component* content);
	virtual void closeButtonPressed() = 0;

	void startLogoAnimation();
	void stopLogoAnimation();
	void addToolbarButton(Button* toolbarButton, bool right_aligned = true);

    void enableShadow(bool enable);
    bool isShadowEnabled();

protected:
    static constexpr int SHADOW_WIDTH = 5;
	static constexpr int CLOSE_BUTTON_WIDTH = 15;

	void paint(Graphics&) override;
	void resized() override;

#if FX_JUCE_HAS_CREATE_DESKTOP_WINDOW
	void createDesktopWindow(const DesktopWindowOptions& options) override;
#endif

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
		void addToolbarButton(Button* toolbarButton, bool right_aligned=true);

	private:
		static constexpr int ICON_WIDTH = 106;
		static constexpr int ICON_HEIGHT = 15;

		void paint(Graphics& g) override;
		void resized() override;
		void lookAndFeelChanged() override;

		void buttonClicked(Button* button) override;

		void mouseDown(const MouseEvent& e);
		void mouseDrag(const MouseEvent& e);
		void mouseUp(const MouseEvent&);

		void updateLogo();

		String name_;
		FxDrawable icon_;
		FxDrawable animation_icon_;
		Label title_;
		std::vector<std::pair<Button*, bool>> toolbar_buttons_;
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
