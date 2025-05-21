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
#include "FxWindow.h"
#include "FxHyperlink.h"
#include "FxController.h"
#include "FxModel.h"
#include "FxTheme.h"
#include "FxHotkeyLabel.h"
#include "FxLanguage.h"

//==============================================================================
/*
*/

class FxVolumeSlider : public Slider
{
public:
	FxVolumeSlider();
	~FxVolumeSlider() = default;

	void setVolumeValue(float value);
	void showValue(bool show);

private:
	static constexpr int LABEL_HEIGHT = 12;

	void resized() override;
	void valueChanged() override;
	void enablementChanged() override;
	bool keyPressed(const KeyPress& key) override;

	Label value_label_;
};

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

	class SettingsPane : public Component
	{
	public:    
		SettingsPane(String name);
		~SettingsPane() = default;

	protected:
        void paint(Graphics& g) override;

		static constexpr int X_MARGIN = 25;
		static constexpr int Y_MARGIN = 5;
		static constexpr int TITLE_HEIGHT = 24;

        Label title_;

		String name_;
	};

	class AudioSettingsPane : public SettingsPane, public FxModel::Listener
	{
	public:
		AudioSettingsPane();
		~AudioSettingsPane();

		void resized() override;
		void paint(Graphics& g) override;

	private:
		static constexpr int ENDPOINT_Y = 50;
		static constexpr int TOGGLE_BUTTON_HEIGHT = 30;
		static constexpr int ENDPOINT_LABEL_WIDTH = 120;
		static constexpr int ENDPOINT_LIST_HEIGHT = 30;
		static constexpr int SLIDER_WIDTH = 200;
		static constexpr int SLIDER_HEIGHT = 18;

		void setText();
		void modelChanged(FxModel::Event model_event);
		void updateEndpointList();

		void mouseEnter(const MouseEvent& mouse_event) override;
		void mouseExit(const MouseEvent& mouse_event) override;

		ComboBox preferred_endpoint_;
		Label endpoint_title_;
		ToggleButton volume_normalizer_toggle_;
		FxVolumeSlider volume_;
	};

	class GeneralSettingsPane : public SettingsPane
	{
	public:
		GeneralSettingsPane();
		~GeneralSettingsPane();

		void resized() override;
		void paint(Graphics& g) override;

	private:
		static constexpr int LANGUAGE_SWITCH_Y = 50;
		static constexpr int TOGGLE_BUTTON_HEIGHT = 30;
		static constexpr int HOTKEY_LABEL_X = X_MARGIN + 25;
		static constexpr int HOTKEY_LABEL_HEIGHT = 20;
		static constexpr int LANGUAGE_LABEL_HEIGHT = 24;
		static constexpr int LANGUAGE_LIST_WIDTH = 120;
		static constexpr int LANGUAGE_LIST_HEIGHT = 30;
		static constexpr int BUTTON_WIDTH = 220;
		static constexpr int BUTTON_HEIGHT = 24;
		static constexpr int MAX_BUTTON_WIDTH = 315;

        void setText();
		void resizeResetButton(int x, int y);

        ToggleButton launch_toggle_;
        ToggleButton hide_help_tips_toggle_;
		ToggleButton hide_notifications_toggle_;
		ToggleButton hotkeys_toggle_;
		TextButton reset_presets_button_;
		OwnedArray<FxHotkeyLabel> hotkey_labels_;
		FxLanguage language_switch_;
	};

	class HelpSettingsPane : public SettingsPane, public ToggleButton::Listener
	{
	public:
		HelpSettingsPane();
		~HelpSettingsPane() = default;

		void resized() override;
		void paint(Graphics& g) override;

		void buttonClicked(Button*) override {}
		void buttonStateChanged(Button* button) override;

	private:
		static constexpr int TEXT_Y = 50;
		static constexpr int TITLE_HEIGHT = 24;
		static constexpr int TEXT_HEIGHT = 20;
		static constexpr int HYPERLINK_HEIGHT = 24;
		static constexpr int TOGGLE_BUTTON_HEIGHT = 24;
		static constexpr int BUTTON_WIDTH = 220;

        void setText();
        
		Label version_title_;
		Label support_title_;
		Label maintenance_title_;
		Label version_text_;
		FxHyperlink changelog_link_;
		FxHyperlink quicktour_link_;
		FxHyperlink submitlogs_link_;
		FxHyperlink helpcenter_link_;
		FxHyperlink feedback_link_;
		TextButton updates_button_;
		ToggleButton debug_log_toggle_;
	};

	class SettingsComponent : public Component, public Button::Listener
	{
	public:
        static constexpr int WIDTH = 600;
        static constexpr int HEIGHT = 510;

		SettingsComponent();
        ~SettingsComponent() = default;

		void resized() override;
        void paint(Graphics& g) override;

		void buttonClicked(Button* button) override;

	private:
		static constexpr int BUTTON_X = 20;
		static constexpr int BUTTON_Y = 50;
		static constexpr int BUTTON_WIDTH = 150;
		static constexpr int BUTTON_HEIGHT = 40;
        static constexpr int DONATE_BUTTON_WIDTH = 100;
        static constexpr int DONATE_BUTTON_HEIGHT = 30;
		static constexpr int SEPARATOR_X = 152;

		std::unique_ptr<SettingsButton> audio_button_;
		std::unique_ptr<SettingsButton> general_button_;
		std::unique_ptr<SettingsButton> help_button_;
        TextButton donate_button_;

		AudioSettingsPane audio_settings_pane_;
		GeneralSettingsPane general_settings_pane_;
		HelpSettingsPane help_settings_pane_; 
	};

	SettingsComponent settings_content_;
	TooltipWindow tooltip_window_;
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxSettingsDialog)
};
