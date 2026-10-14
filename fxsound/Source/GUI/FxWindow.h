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
	void addToolbarButton(Button* toolbarButton, bool right_aligned = true);

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
		// Drawable is not a Component in newer JUCE versions, so wrap it to keep
		// the bounds/alpha/animator behaviour the title bar relies on.
		class LogoIcon : public Component
		{
		public:
			explicit LogoIcon(std::unique_ptr<Drawable> drawable) : drawable_(std::move(drawable))
			{
				setInterceptsMouseClicks(false, false);
			}

			int getIntrinsicWidth() const { return drawable_ ? juce::roundToInt(drawable_->getDrawableBounds().getWidth()) : 0; }
			int getIntrinsicHeight() const { return drawable_ ? juce::roundToInt(drawable_->getDrawableBounds().getHeight()) : 0; }

			void paint(Graphics& g) override
			{
				if (drawable_ != nullptr)
				{
					drawable_->drawWithin(g, getLocalBounds().toFloat(),
						RectanglePlacement::xLeft | RectanglePlacement::yMid, 1.0f);
				}
			}

		private:
			std::unique_ptr<Drawable> drawable_;
		};

		std::unique_ptr<LogoIcon> icon_;
		std::unique_ptr<LogoIcon> animation_icon_;
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
