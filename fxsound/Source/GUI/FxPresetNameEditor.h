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

class PresetNameInputFilter : public juce::TextEditor::InputFilter
{
public:
	PresetNameInputFilter();

	juce::String filterNewText(juce::TextEditor& textEditor, const juce::String& newText) override;

private:
	juce::Array<juce::juce_wchar> reservedChars;
};

class FxPresetNameEditor : public Component, private TextEditor::Listener
{
public:
	enum class Status { Empty = 0, Valid, Invalid };

	static constexpr int WIDTH = 200;
	static constexpr int HEIGHT = 30;

	FxPresetNameEditor();
	~FxPresetNameEditor() = default;

	Status getStatus() { return preset_status_; }
	String getPresetName() { return preset_name_; }

private:
	void resized() override;
	void paint(Graphics& g) override;
	void textEditorTextChanged(TextEditor& textEditor) override;
	void lookAndFeelChanged() override;

	TextEditor preset_editor_;
	PresetNameInputFilter preset_name_input_filter_;
	Label hint_text_;
	Status preset_status_;
	String preset_name_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPresetNameEditor)
};

