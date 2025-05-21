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

class FxHotkeyEditor : public Label
{
public:
	FxHotkeyEditor(const String& command);

private:
	static constexpr int HOTKEY_EDITOR_WIDTH = 120;
	static constexpr int HOTKEY_EDITOR_HEIGHT = 20;

	bool keyPressed(const KeyPress& key) override;
	void focusGained(FocusChangeType cause) override;
	void focusLost(FocusChangeType cause) override;
	void paint(juce::Graphics& g) override;

	void setKeyText();

	String key_text_;
	String command_;
	int mod_;
	int vk_;
};

class FxHotkeyLabel : public Component
{
public:
	FxHotkeyLabel(const String& name, const String& command);

private:
	static constexpr int HOTKEY_LABEL_WIDTH = 170;

	void resized() override;
	void paint(Graphics& g) override;

	String name_;
	Label label_;
	FxHotkeyEditor hotkey_editor_;
};
