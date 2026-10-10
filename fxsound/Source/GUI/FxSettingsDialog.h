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

#include <vector>
#include <JuceHeader.h>
#include "FxWindow.h"
#include "FxSettingsPane.h"

//==============================================================================
/*
*/

class FxSettingsDialog : public FxWindow
{
public:
    FxSettingsDialog();
    ~FxSettingsDialog() = default;

	void closeButtonPressed() override;

	void paint(Graphics& g) override;

	bool keyPressed(const KeyPress& key) override;	

private:
	static constexpr int SEPARATOR_X = 152;

	class SettingsButton : public Button
	{
	public:
		SettingsButton(String button_name) : Button(button_name)
		{
			setMouseCursor(MouseCursor::PointingHandCursor);
		}
		~SettingsButton() = default;

		void setImage(const Drawable* image)
		{
			image_ = image->createCopy();
		}

		void paintButton(Graphics &, bool, bool) override {}
		void paint(Graphics& g) override;

	private:
		std::unique_ptr<Drawable> image_;
	};

	class SettingsComponent : public Component, public Button::Listener
	{
	public:
        static constexpr int WIDTH = 600;
        static constexpr int HEIGHT = 510;

		SettingsComponent();
        ~SettingsComponent() = default;

		void resized() override;

		void buttonClicked(Button* button) override;

	private:
		static constexpr int BUTTON_X = 20;
		static constexpr int BUTTON_Y = 50;
		static constexpr int BUTTON_WIDTH = 150;
		static constexpr int BUTTON_HEIGHT = 40;
		static constexpr int SEPARATOR_X = 152;

		struct PaneEntry
		{
			std::unique_ptr<SettingsButton> button;
			std::unique_ptr<FxSettingsPane> pane;
		};

		void addPane(const String& name, const void* icon_data, int icon_data_size, std::unique_ptr<FxSettingsPane> pane);

		std::vector<PaneEntry> panes_;
	};

	SettingsComponent settings_content_;
	TooltipWindow tooltip_window_;
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxSettingsDialog)
};
