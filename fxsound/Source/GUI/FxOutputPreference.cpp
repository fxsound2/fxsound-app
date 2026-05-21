/*
FxSound
Copyright (C) 2026  FxSound LLC

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

#include "FxController.h"
#include "FxOutputPreference.h"

FxOutputDeviceRow::FxOutputDeviceRow(FxOutputPreferenceListModel& model) : up_button_("up", DrawableButton::ImageFitted), down_button_("down", DrawableButton::ImageFitted), output_preference_list_model_(model)
{
    setInterceptsMouseClicks(false, true);

    up_image_ = Drawable::createFromImageData(FXIMAGE(ArrowUp), FXIMAGESIZE(ArrowUp));
    down_image_ = Drawable::createFromImageData(FXIMAGE(ArrowDown), FXIMAGESIZE(ArrowDown));
    up_selected_image_ = Drawable::createFromImageData(FXIMAGE(ArrowUpSelected), FXIMAGESIZE(ArrowUpSelected));
    down_selected_image_ = Drawable::createFromImageData(FXIMAGE(ArrowDownSelected), FXIMAGESIZE(ArrowDownSelected));

    up_button_.setMouseCursor(MouseCursor::PointingHandCursor);
    up_button_.setSize(BUTTON_WIDTH, BUTTON_WIDTH);
    up_button_.setImages(up_image_.get(), up_selected_image_.get(), up_image_.get());
    up_button_.setWantsKeyboardFocus(true);
    up_button_.onClick = [this]() {
        output_preference_list_model_.moveRowUp(row_index_);
        };

    down_button_.setMouseCursor(MouseCursor::PointingHandCursor);
    down_button_.setSize(BUTTON_WIDTH, BUTTON_WIDTH);
    down_button_.setImages(down_image_.get(), down_selected_image_.get(), down_image_.get());
    down_button_.setWantsKeyboardFocus(true);
    down_button_.onClick = [this]() {
        output_preference_list_model_.moveRowDown(row_index_);
        };

    preset_list_.setColour(ComboBox::ColourIds::backgroundColourId, Colour(FXCOLOR(WidgetBackground)).withAlpha(1.0f));
    preset_list_.setColour(ComboBox::ColourIds::outlineColourId, Colour(FXCOLOR(RowOutline)).withAlpha(1.0f));
    preset_list_.setColour(ComboBox::ColourIds::focusedOutlineColourId, Colour(FXCOLOR(SelectedRowOutline)).withAlpha(1.0f));
    preset_list_.setWantsKeyboardFocus(true);
    preset_list_.setJustificationType(Justification::centredLeft);
    preset_list_.setTextWhenNothingSelected(TRANS("Select preset"));

    preset_list_.onChange = [this]() {
        auto index = preset_list_.getSelectedItemIndex();
        if (index >= 0)
        {
            auto preset = preset_list_.getItemText(index);
            device_config_.preset = preset;
            output_preference_list_model_.updateDeviceConfig(device_config_);
            if (device_config_.device_name == FxController::getInstance().getOutputName())
            {
                FxController::getInstance().setPreset(preset);
            }
        }
    };

    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());
    device_name_.setInterceptsMouseClicks(false, false);
    device_name_.setFont(theme.getNormalFont());
    device_name_.setMinimumHorizontalScale(1.0f);

    addAndMakeVisible(up_button_);
    addAndMakeVisible(down_button_);
    addAndMakeVisible(device_name_);
    addAndMakeVisible(preset_list_);

    row_index_ = -1;
    is_row_selected_ = false;
}

void FxOutputDeviceRow::update(int index, bool is_row_selected, const DeviceConfig& device_config)
{
    if (index < 0)
        return;

    row_index_ = index;

    device_config_ = device_config;

    up_button_.setVisible(row_index_ > 0);
    down_button_.setVisible(row_index_ < output_preference_list_model_.getNumRows() - 1);

    auto bounds = getLocalBounds().reduced(2);

    auto y = (getHeight() - BUTTON_WIDTH) / 2;
    int x;

    if (row_index_ < (output_preference_list_model_.getNumRows() - 1))
    {
        x = MARGIN;
    }
    else
    {
        x = MARGIN + BUTTON_WIDTH / 2;
    }
    up_button_.setBounds(x, y, BUTTON_WIDTH, BUTTON_WIDTH);

    if (row_index_ != 0)
    {
        x = up_button_.getRight();
    }
    else
    {
        x = MARGIN + BUTTON_WIDTH / 2;
    }
    down_button_.setBounds(x, y, BUTTON_WIDTH, BUTTON_WIDTH);

    preset_list_.setBounds((bounds.getWidth() - PRESET_LIST_WIDTH - MARGIN), bounds.getY(), PRESET_LIST_WIDTH, bounds.getHeight());
    
    x = MARGIN * 2 + BUTTON_WIDTH * 2;
    int width = bounds.getWidth() - x - PRESET_LIST_WIDTH - MARGIN * 2;
    device_name_.setBounds(x, bounds.getY(), width, bounds.getHeight());

    device_name_.setText(String::formatted("%d. ", row_index_ + 1) +  device_config.device_name, NotificationType::dontSendNotification);
    device_name_.setEnabled(FxController::getInstance().isOutputDeviceConnected(device_config.device_name));

    if (is_row_selected != is_row_selected_)
    {
        is_row_selected_ = is_row_selected;
        if (is_row_selected_)
        {
            up_button_.setImages(up_selected_image_.get(), up_selected_image_.get(), up_selected_image_.get());
            down_button_.setImages(down_selected_image_.get(), down_selected_image_.get(), down_selected_image_.get());
            preset_list_.setColour(ComboBox::ColourIds::outlineColourId, Colour(FXCOLOR(SelectedRowOutline)).withAlpha(1.0f));
        }
        else
        {
            up_button_.setImages(up_image_.get(), up_selected_image_.get(), up_image_.get());
            down_button_.setImages(down_image_.get(), down_selected_image_.get(), down_image_.get());
            preset_list_.setColour(ComboBox::ColourIds::outlineColourId, Colour(FXCOLOR(RowOutline)).withAlpha(0.5f));
        }
        repaint();
    }

    if (preset_list_.getNumItems() == 0)
    {
        auto& model = FxModel::getModel();
        for (auto i = 0; i < model.getPresetCount(); i++)
            preset_list_.addItem(FxModel::getModel().getPreset(i).name, i + 1);
    }

    int selected_id = 0;
    auto& model = FxModel::getModel();
    for (auto i = 0; i < model.getPresetCount(); i++)
    {
        if (FxModel::getModel().getPreset(i).name == device_config.preset)
        {
            selected_id = i + 1;
            break;
        }
    }
    if (selected_id != 0)
        preset_list_.setSelectedId(selected_id, NotificationType::dontSendNotification);
}

void FxOutputDeviceRow::paint(Graphics& g)
{
    if (is_row_selected_)
    {
        g.setColour(Colour(FXCOLOR(SelectedRowOutline)).withAlpha(1.0f));
        g.drawLine(device_name_.getX(), device_name_.getBottom() - 0.5f, device_name_.getRight(), device_name_.getBottom() - 1.0f, 1.0);
    }
    else
    {
        g.setColour(Colour(FXCOLOR(RowOutline)).withAlpha(1.0f));
        g.drawLine(device_name_.getX(), device_name_.getBottom() - 0.5f, device_name_.getRight(), device_name_.getBottom() - 0.5f, 0.5);
    }    
}

FxOutputPreferenceListModel::FxOutputPreferenceListModel()
{
    DeviceConfig::onDeviceConfigsUpdate = [this]() {
        device_configs_ = FxController::getInstance().getDeviceConfigs();
        if (onModelChanged)
            onModelChanged();
    };

    device_configs_ = FxController::getInstance().getDeviceConfigs();
}

FxOutputPreferenceListModel::~FxOutputPreferenceListModel()
{
    DeviceConfig::onDeviceConfigsUpdate = nullptr;
}

int FxOutputPreferenceListModel::getNumRows()
{
    return device_configs_.size();
}

void FxOutputPreferenceListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
}

Component* FxOutputPreferenceListModel::refreshComponentForRow(int rowNumber, bool isRowSelected, Component* existingComponent)
{
    auto* row = dynamic_cast<FxOutputDeviceRow*>(existingComponent);

    if (row == nullptr)
        row = new FxOutputDeviceRow(*this);

    row->update(rowNumber, isRowSelected, device_configs_[rowNumber]);

    return row;
}

void FxOutputPreferenceListModel::moveRowUp(int index)
{
    if (index <= 0)
        return;

    device_configs_.swap(index, index - 1);
    persist();

    if (onRowMoved)
        onRowMoved(index - 1);
}

void FxOutputPreferenceListModel::moveRowDown(int index)
{
    if (index >= device_configs_.size() - 1)
        return;

    device_configs_.swap(index, index + 1);
    persist();

    if (onRowMoved)
        onRowMoved(index + 1);
}

void FxOutputPreferenceListModel::updateDeviceConfig(const DeviceConfig device_config)
{
    for (auto i = 0; i < device_configs_.size(); i++)
    {
        if (device_configs_[i].device_name == device_config.device_name)
        {
            device_configs_.getReference(i).preset = device_config.preset;
            break;
        }
    }

    persist();
}

void FxOutputPreferenceListModel::persist()
{
    FxController::getInstance().saveDeviceConfigs(device_configs_);
}

FxOutputPreference::FxOutputPreference()
{
    output_preference_model_.onModelChanged = [this]() {
        output_preference_list_.updateContent();
        output_preference_list_.repaintRow(-1);
    };

    output_preference_model_.onRowMoved = [this](int row_index) {
        output_preference_list_.updateContent();
        output_preference_list_.selectRow(row_index);
    };

    output_preference_list_.setModel(&output_preference_model_);
    output_preference_list_.addKeyListener(this);
    output_preference_list_.setTooltip(TRANS("Use Shift+Up or Shift+Down to change the device priority"));

    addAndMakeVisible(output_preference_list_);

    output_preference_list_.setRowHeight(ROW_HEIGHT);
    output_preference_list_.setMultipleSelectionEnabled(false);

    output_preference_list_.repaint();
}

void FxOutputPreference::update()
{
    output_preference_list_.updateContent();
    output_preference_list_.resized();
    output_preference_list_.repaint();
}

bool FxOutputPreference::keyPressed(const KeyPress& key, Component*)
{
    auto selected_row = output_preference_list_.getSelectedRow();
    if (selected_row < 0)
        return false;

    if (key == KeyPress(KeyPress::upKey, ModifierKeys::shiftModifier, 0))
    {
        output_preference_model_.moveRowUp(selected_row);
        output_preference_list_.grabKeyboardFocus();
        return true;
    }

    if (key == KeyPress(KeyPress::downKey, ModifierKeys::shiftModifier, 0))
    {
        output_preference_model_.moveRowDown(selected_row);
        output_preference_list_.grabKeyboardFocus();
        return true;
    }

    return false;
}

void FxOutputPreference::resized()
{
    output_preference_list_.setBounds(getLocalBounds().reduced(5, 10));
    output_preference_list_.updateContent();
}

void FxOutputPreference::paint(Graphics& g)
{
    g.setFillType(FillType(Colour(FXCOLOR(WidgetBackground)).withAlpha(1.0f)));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
}
