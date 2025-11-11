#include "FxPresetNameEditor.h"
#include <FxTheme.h>
#include <FxModel.h>
#include <FxController.h>

PresetNameInputFilter::PresetNameInputFilter()
{
	// Define a list of reserved characters
	reservedChars.add('<');
	reservedChars.add('>');
	reservedChars.add(':');
	reservedChars.add('"');
	reservedChars.add('/');
	reservedChars.add('\\');
	reservedChars.add('|');
	reservedChars.add('?');
	reservedChars.add('*');
}

juce::String PresetNameInputFilter::filterNewText(juce::TextEditor& textEditor, const juce::String& newText)
{
	// Iterate through the new text and remove any reserved characters
	juce::String filteredText;
	for (int i = 0; i < newText.length(); ++i)
	{
		juce_wchar character = newText[i];
		if (!reservedChars.contains(character))
		{
			filteredText += character;
		}
	}
	return filteredText;
}

FxPresetNameEditor::FxPresetNameEditor()
{
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

	preset_status_ = Status::Empty;

	hint_text_.setName(L"hintLabel");
	hint_text_.setFont(theme.getNormalFont());
	hint_text_.setColour(Label::ColourIds::textColourId, Colour(FXCOLOR(HintText)).withAlpha(1.0f));
	hint_text_.setJustificationType(Justification::centredLeft);
	addAndMakeVisible(hint_text_);

	hint_text_.setText(TRANS("Enter your preset name"), NotificationType::dontSendNotification);
	preset_editor_.setDescription(TRANS("Enter your preset name"));
	preset_editor_.setName(L"presetName");
	preset_editor_.setFont(theme.getNormalFont());
	preset_editor_.setColour(TextEditor::ColourIds::backgroundColourId, Colour(FXCOLOR(DefaultFill)).withAlpha(0.0f));
	preset_editor_.setInputRestrictions(64);
	preset_editor_.setInputFilter(&preset_name_input_filter_, false);
	preset_editor_.addListener(this);
	addAndMakeVisible(preset_editor_);

	setSize(WIDTH, HEIGHT);
}

void FxPresetNameEditor::resized()
{
	auto bounds = getLocalBounds().reduced(2, 2);

	hint_text_.setBounds(bounds);
	preset_editor_.setBounds(bounds);
}

void FxPresetNameEditor::paint(Graphics& g)
{
	auto bounds = getLocalBounds();
	Colour outline_colour;

	switch (preset_status_)
	{
	case Status::Valid:
		outline_colour = Colour(FXCOLOR(ValidTextBorder)).withAlpha(1.0f);
		break;

	case Status::Empty:
	case Status::Invalid:
		outline_colour = Colour(FXCOLOR(InvalidTextBorder)).withAlpha(1.0f);
	}

	g.setColour(findColour(TextEditor::backgroundColourId));
	g.fillRect(bounds.toFloat());

	g.setColour(outline_colour);
	g.drawRect(bounds.toFloat().reduced(0.5f, 0.5f), 2.0f);

	preset_editor_.grabKeyboardFocus();
}

void FxPresetNameEditor::textEditorTextChanged(TextEditor& textEditor)
{
	auto prevStatus = preset_status_;

	preset_name_ = textEditor.getText();
	if (preset_name_.isEmpty())
	{
		preset_status_ = Status::Empty;
		hint_text_.setAlpha(1.0);
	}
	else
	{
		hint_text_.setAlpha(0.0);

		preset_status_ = Status::Valid;
		auto& model = FxModel::getModel();
		for (auto i = 0; i < model.getPresetCount(); i++)
		{
			if (model.getPreset(i).name.equalsIgnoreCase(preset_name_))
			{
				preset_status_ = Status::Invalid;
				break;
			}
		}
	}

	if (prevStatus != preset_status_)
	{
		sendLookAndFeelChange();
	}
}

void FxPresetNameEditor::lookAndFeelChanged()
{
	repaint();
}