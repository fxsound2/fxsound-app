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

#include <JuceHeader.h>
#include "FxMainWindow.h"
#include "FxController.h"
#include "FxSettingsDialog.h"
#include "FxPresetExportDialog.h"
#include "FxPresetImportDialog.h"

class PresetNameInputFilter : public juce::TextEditor::InputFilter
{
public:
	PresetNameInputFilter()
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

	juce::String filterNewText(juce::TextEditor& textEditor, const juce::String& newText) override
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

private:
	juce::Array<juce::juce_wchar> reservedChars;
};

class FxPresetNameEditor : public PopupMenu::CustomComponent, private TextEditor::Listener
{
public:
	enum class Status { Empty = 0, Valid, Invalid };
	enum class Action { Save = 1, Rename };

	FxPresetNameEditor(Action action) : PopupMenu::CustomComponent(false)
	{
		auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

		preset_status_ = Status::Empty;

		hint_text_.setName(L"hintLabel");
		hint_text_.setFont(theme.getNormalFont());
		hint_text_.setColour(Label::ColourIds::textColourId, Colour(0x7F7F7F).withAlpha(1.0f));
		hint_text_.setJustificationType(Justification::centredLeft);
		addAndMakeVisible(hint_text_);

		if (action == Action::Save)
		{
			hint_text_.setText(TRANS("Enter your preset name"), NotificationType::dontSendNotification);
			preset_editor_.setDescription(TRANS("Enter your preset name"));
		}
		else
		{
			hint_text_.setText(TRANS("Enter new preset name"), NotificationType::dontSendNotification);
			preset_editor_.setDescription(TRANS("Enter new preset name"));
		}
		preset_editor_.setName(L"presetName");
		preset_editor_.setFont(theme.getNormalFont());
		preset_editor_.setColour(TextEditor::ColourIds::backgroundColourId, Colour(0x000000).withAlpha(0.0f));
		preset_editor_.setInputRestrictions(64);
		preset_editor_.setInputFilter(&preset_name_input_filter_, false);
		preset_editor_.addListener(this);
		addAndMakeVisible(preset_editor_);

		preset_editor_.onEscapeKey = [this]() {
			preset_status_ = Status::Empty;
			triggerMenuItem();
		};

		preset_editor_.onReturnKey = [this, action]() {
			if (preset_status_ == Status::Valid)
			{
				if (action == Action::Save)
				{
					FxController::getInstance().savePreset(preset_name_);
					triggerMenuItem();
				}
				else
				{
					FxController::getInstance().renamePreset(preset_name_);
					triggerMenuItem();
				}
			}
		};
	}
	~FxPresetNameEditor() = default;

	Status getStatus() { return preset_status_; }
	String getPresetName() { return preset_name_; }

	void getIdealSize(int& ideal_width, int& ideal_height)
	{
		ideal_width = WIDTH;
		ideal_height = HEIGHT;
	}

private:
	static constexpr int WIDTH = 200;
	static constexpr int HEIGHT = 30;

	void resized() override
	{
		auto bounds = getLocalBounds().reduced(2, 2);

		hint_text_.setBounds(bounds);
		preset_editor_.setBounds(bounds);
	}

	void paint(Graphics& g) override
	{
		auto bounds = getLocalBounds();
		Colour outline_colour;

		switch (preset_status_)
		{
		case Status::Valid:
			outline_colour = Colour(0xff009cdd);
			break;

		case Status::Empty:
		case Status::Invalid:
			outline_colour = Colour(0xffd51535);
		}

		g.setColour(findColour(TextEditor::backgroundColourId));
		g.fillRect(bounds.toFloat());

		g.setColour(outline_colour);
		g.drawRect(bounds.toFloat().reduced(0.5f, 0.5f), 2.0f);

		preset_editor_.grabKeyboardFocus();
	}

	void textEditorTextChanged(TextEditor& textEditor) override
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

	void lookAndFeelChanged() override
	{
		repaint();
	}

	TextEditor preset_editor_;
	PresetNameInputFilter preset_name_input_filter_;
	Label hint_text_;
	Status preset_status_;
	String preset_name_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPresetNameEditor)
};

//==============================================================================
FxMainWindow::FxMainWindow() : power_button_(L"powerButton"), menu_button_(L"menuButton", DrawableButton::ButtonStyle::ImageFitted), resize_button_(L"resizeButton", DrawableButton::ButtonStyle::ImageFitted), donate_button_(TRANS("Donate")), minimize_button_(L"minimizeButton", DrawableButton::ButtonStyle::ImageFitted)
{
	setName("FxSound");
	setOpaque(false);
    enableShadow(false);

	setWantsKeyboardFocus(true);

	FxModel::getModel().addListener(this);

	power_button_.setPowerState(FxModel::getModel().getPowerState());

	power_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	power_button_.setSize(BUTTON_WIDTH, BUTTON_WIDTH);
	power_button_.setImageWidth(BUTTON_WIDTH);
	power_button_.setHelpText(TRANS("Power Button"));
	power_button_.setWantsKeyboardFocus(true);
	power_button_.addListener(this);

	menu_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	menu_button_.setSize(BUTTON_WIDTH+2, BUTTON_WIDTH+6);
	menu_button_.setHelpText(TRANS("Menu Button"));
	menu_image_ = Drawable::createFromImageData(BinaryData::menu_svg, BinaryData::menu_svgSize);
	menu_hover_image_ = Drawable::createFromImageData(BinaryData::menu_hover_svg, BinaryData::menu_hover_svgSize);
	menu_button_.setImages(menu_image_.get(), menu_hover_image_.get());
	menu_button_.setWantsKeyboardFocus(true);
	menu_button_.addListener(this);

	resize_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	setResizeImage();
	resize_button_.setHelpText(TRANS("Resize Button"));
	resize_button_.setWantsKeyboardFocus(true);
	resize_button_.addListener(this);

	auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());
	donate_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	donate_button_.setName(L"Donate");
	donate_button_.setSize(DONATE_BUTTON_WIDTH, DONATE_BUTTON_HEIGHT);
	donate_button_.onClick = [this]() {
		URL url("https://www.paypal.com/donate/?hosted_button_id=JVNQGYXCQ2GPG");
		url.launchInDefaultBrowser();
	};

	minimize_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	minimize_button_.setSize(BUTTON_WIDTH + 2, BUTTON_WIDTH + 6);
	minimize_button_.setHelpText(TRANS("Minimize Button"));
	minimize_image_ = Drawable::createFromImageData(BinaryData::min_window_svg, BinaryData::min_window_svgSize);
	minimize_hover_image_ = Drawable::createFromImageData(BinaryData::min_window_hover_svg, BinaryData::min_window_hover_svgSize);
	minimize_button_.setImages(minimize_image_.get(), minimize_hover_image_.get());
	minimize_button_.setWantsKeyboardFocus(true);
	minimize_button_.addListener(this);

	help_bubble_.addToDesktop(0);
	help_bubble_.setColour(BubbleComponent::ColourIds::backgroundColourId, Colour(0x0).withAlpha(1.0f));
	help_bubble_.setColour(BubbleComponent::ColourIds::outlineColourId, theme.findColour(TextEditor::textColourId));
	help_bubble_.setAlwaysOnTop(true);

	addToolbarButton(&minimize_button_);
	addToolbarButton(&resize_button_);
	addToolbarButton(&menu_button_);
	addToolbarButton(&power_button_);
	addToolbarButton(&donate_button_);
}

FxMainWindow::~FxMainWindow()
{
}

void FxMainWindow::show()
{
	if (!isVisible())
	{
		setVisible(true);
	}

	addToDesktop(ComponentPeer::windowAppearsOnTaskbar);
	toFront(true);

	// Bring window to the top
	auto* peer = getPeer();
	if (peer)
	{
		HWND hwnd = (HWND)peer->getNativeHandle();
		if (IsIconic(hwnd)) // If minimized, restore first
			ShowWindow(hwnd, SW_RESTORE);

		SetForegroundWindow(hwnd); // Bring to front
		SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	}
}

void FxMainWindow::showLiteView()
{
	setContent(&lite_view_);
    setResizeImage();
	setAlwaysOnTop(true);

	auto bounds = getBounds();
	auto pos = FxController::getInstance().getSystemTrayWindowPosition(bounds.getWidth(), bounds.getHeight());
	bounds.setPosition(pos);
	setBounds(bounds);
}

void FxMainWindow::showProView()
{
    pro_view_.update();
	setContent(&pro_view_);
    setResizeImage();
	setAlwaysOnTop(false);

	auto display = Desktop::getInstance().getDisplays().getPrimaryDisplay();
	if (display != nullptr)
	{
		auto bounds = getBounds();
		if (display->userArea.getX() > bounds.getX())
		{
			bounds.setX(display->userArea.getX() + 10);
		}
		else if (display->userArea.getRight() < bounds.getRight())
		{
			bounds.setX(display->userArea.getRight() - bounds.getWidth() - 10);
		}

		if (display->userArea.getY() > bounds.getY())
		{
			bounds.setY(display->userArea.getY() + 10);
		}
		else if (display->userArea.getBottom() < bounds.getBottom())
		{
			bounds.setY(display->userArea.getBottom() - bounds.getHeight() - 10);
		}

		setBounds(bounds);
	}
}

void FxMainWindow::updateView()
{
    pro_view_.update();
}

void FxMainWindow::startVisualizer()
{
    pro_view_.startVisualizer();
}

void FxMainWindow::pauseVisualizer()
{
    pro_view_.pauseVisualizer();
}

void FxMainWindow::setResizeImage()
{
	if (FxController::getInstance().getCurrentView() == ViewType::Pro)
	{
		resize_image_ = Drawable::createFromImageData(BinaryData::minimize_svg, BinaryData::minimize_svgSize);
		resize_hover_image_ = Drawable::createFromImageData(BinaryData::minimize_hover_svg, BinaryData::minimize_hover_svgSize);
		resize_button_.setSize(BUTTON_WIDTH+2, BUTTON_WIDTH+2);
	}
	else
	{
		resize_image_ = Drawable::createFromImageData(BinaryData::maximize_svg, BinaryData::maximize_svgSize);
		resize_hover_image_ = Drawable::createFromImageData(BinaryData::maximize_hover_svg, BinaryData::maximize_hover_svgSize);
		resize_button_.setSize(BUTTON_WIDTH, BUTTON_WIDTH); 	
	}

	resize_button_.setImages(resize_image_.get(), resize_hover_image_.get());
}

void FxMainWindow::setIcon(bool power, bool processing)
{
	HINSTANCE hInst = GetModuleHandle(NULL);
	HWND hWnd = (HWND)getWindowHandle();

	HICON curr_icon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
	HICON icon = NULL;

	if (power)
	{
		if (processing)
		{
			icon = LoadIcon(hInst, L"IDI_LOGO_RED");
		}
		else
		{
			icon = LoadIcon(hInst, L"IDI_LOGO_WHITE");
		}
	}
	else
	{
		icon = LoadIcon(hInst, L"IDI_LOGO_GRAY");
	}

	if (icon != NULL)
	{
		SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
	}

	if (curr_icon != NULL)
	{
		DestroyIcon(curr_icon);
	}
}

bool FxMainWindow::keyPressed(const KeyPress& key)
{
	if (key.getModifiers().isAltDown() && key.getKeyCode() == juce::KeyPress::returnKey)
	{
		FxController::getInstance().setMenuClicked(true);
		showMenu();
		return true;
	}

	return Component::keyPressed(key);
}

void FxMainWindow::visibilityChanged()
{
	auto* peer = getPeer();
	if (peer)
	{
		HWND hwnd = (HWND)peer->getNativeHandle();
		LONG style = GetWindowLong(hwnd, GWL_STYLE);
		if ((style & WS_MINIMIZEBOX) == 0)
		{
			SetWindowLong(hwnd, GWL_STYLE, style | WS_MINIMIZEBOX);
		}
	}
}

void FxMainWindow::showMenu()
{
	auto settingsClicked = [this]() {

		FxSettingsDialog settings_dialog;
		settings_dialog.runModalLoop();
	};

	auto overwriteClicked = [this]() {
		FxController::getInstance().savePreset();
	};

	auto undoClicked = []() {
		FxController::getInstance().undoPreset();
	};

	auto deleteClicked = []() {
		FxController::getInstance().deletePreset();
	};

	auto exportClicked = [this]() {
		FxPresetExportDialog preset_export_dialog;
		preset_export_dialog.runModalLoop();
	};

	auto importClicked = [this]() {
		FxPresetImportDialog preset_import_dialog;
		preset_import_dialog.runModalLoop();
	};

	auto downloadClicked = []() {
		URL url("https://www.fxsound.com/presets");
		url.launchInDefaultBrowser();
	};

	auto donateClicked = []() {
		URL url("https://www.paypal.com/donate/?hosted_button_id=JVNQGYXCQ2GPG");
		url.launchInDefaultBrowser();
	};

	PopupMenu popup_menu;
	PopupMenu save_menu;
	PopupMenu rename_menu;

	save_menu.addCustomItem(1, std::make_unique<FxPresetNameEditor>(FxPresetNameEditor::Action::Save), nullptr);
	rename_menu.addCustomItem(2, std::make_unique<FxPresetNameEditor>(FxPresetNameEditor::Action::Rename), nullptr);

	popup_menu.addItem(TRANS("Settings"), settingsClicked);
	popup_menu.addSeparator();

	bool user_preset = false;
	String overwrite_menu_name = TRANS("Overwrite Existing Preset");

	auto& model = FxModel::getModel();
	auto preset = model.getPreset(model.getSelectedPreset());
	if (preset.type == FxModel::PresetType::UserPreset)
	{
		user_preset = true;
	}
	if (model.isPresetModified() && user_preset)
	{
		overwrite_menu_name = overwrite_menu_name + L" - " + preset.name;
	}

	auto power_state = model.getPowerState();
	popup_menu.addSubMenu(TRANS("Save New Preset"), save_menu, model.isPresetModified() && model.getUserPresetCount() < FxController::getInstance().getMaxUserPresets() && power_state);
	popup_menu.addItem(overwrite_menu_name, model.isPresetModified() && user_preset && power_state, false, overwriteClicked);
	popup_menu.addItem(TRANS("Undo Preset Changes"), model.isPresetModified() && power_state, false, undoClicked);
	popup_menu.addSubMenu(TRANS("Rename Preset"), rename_menu, !model.isPresetModified() && user_preset && power_state);
	popup_menu.addItem(TRANS("Delete Preset"), !model.isPresetModified() && user_preset && power_state, false, deleteClicked);
	popup_menu.addSeparator();
	popup_menu.addItem(TRANS("Export Presets"), !model.isPresetModified() && power_state, false, exportClicked);
	popup_menu.addItem(TRANS("Import Presets"), !model.isPresetModified() && power_state, false, importClicked);
	popup_menu.addSeparator();
	popup_menu.addItem(TRANS("Download Bonus Presets"), downloadClicked);
	popup_menu.addSeparator();
	popup_menu.addItem(TRANS("Donate"), donateClicked);

	popup_menu.showAt(&menu_button_);
}


void FxMainWindow::buttonClicked(Button* button)
{
	if (button == &power_button_)
	{
		auto power_state = !FxModel::getModel().getPowerState();

		FxController::getInstance().setPowerState(power_state);

		power_button_.setPowerState(FxModel::getModel().getPowerState());

		repaint();
	}
	else if (button == &menu_button_)
	{
		FxController::getInstance().setMenuClicked(true);
		showMenu();
	}
	else if (button == &resize_button_)
	{
		FxController::getInstance().switchView();
		setResizeImage();
	}
	else if (button == &minimize_button_)
	{
		if (isOnDesktop())
		{
			ShowWindow((HWND)getWindowHandle(), SW_MINIMIZE);
		}
	}
}

void FxMainWindow::mouseEnter(const MouseEvent&)
{
	if (menu_button_.isMouseOver(true) && !FxModel::getModel().isMenuClicked())
	{
		auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

		AttributedString text(TRANS("Click here to save new presets, overwrite old ones, or reset your settings."));
		text.setColour(theme.findColour(TextEditor::textColourId));
		text.setJustification(Justification::centred);
		text.setFont(theme.getSmallFont());

		help_bubble_.showAt(&menu_button_, text, 0, true, false);
	}
}

void FxMainWindow::modelChanged(FxModel::Event model_event)
{
	power_button_.setPowerState(FxModel::getModel().getPowerState());
}

void FxMainWindow::userTriedToCloseWindow()
{
	FxController::getInstance().hideMainWindow();
}

void FxMainWindow::closeButtonPressed()
{
	FxController::getInstance().hideMainWindow();
}