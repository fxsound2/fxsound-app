/*
FxSound
Copyright (C) 2026  FxSound LLC

Contributors:
    www.theremino.com (2025)

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

class FxOutputPreferenceListModel;

class FxOutputDeviceRow : public Component
{
public:
    explicit FxOutputDeviceRow(FxOutputPreferenceListModel& model);
    void update(int index, bool is_row_selected, const DeviceConfig& device_config);

private:
    static constexpr int BUTTON_WIDTH = 18;
    static constexpr int MARGIN = 5;
    static constexpr int PRESET_LIST_WIDTH = 150;

    void paint(Graphics& g) override;
    
    FxOutputPreferenceListModel& output_preference_list_model_;
    
    std::unique_ptr<Drawable> up_image_;
    std::unique_ptr<Drawable> down_image_;
    std::unique_ptr<Drawable> up_selected_image_;
    std::unique_ptr<Drawable> down_selected_image_;
    DrawableButton up_button_;
    DrawableButton down_button_;
    Label device_name_;
    ComboBox preset_list_;

    int row_index_;
    bool is_row_selected_;
    DeviceConfig device_config_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxOutputDeviceRow)
};

class FxOutputPreferenceListModel : public ListBoxModel
{
public:
    FxOutputPreferenceListModel();
    ~FxOutputPreferenceListModel();

    int getNumRows() override;

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;

    Component* refreshComponentForRow(int rowNumber, bool isRowSelected, Component* existingComponent) override;

    void moveRowUp(int index);

    void moveRowDown(int index);

    void updateDeviceConfig(const DeviceConfig device_config);

    std::function<void()> onModelChanged;
    std::function<void(int row_index)> onRowMoved;

private:
    void persist();

    juce::Array<DeviceConfig> device_configs_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxOutputPreferenceListModel)
};


class FxOutputPreference : public Component, public KeyListener
{
public:
    FxOutputPreference();

    void update();

private:
    static constexpr int ROW_HEIGHT = 40;

    void resized() override;
    void paint(Graphics& g) override;
    bool keyPressed(const KeyPress& key, Component* originatingComponent) override;

    ListBox output_preference_list_;
    FxOutputPreferenceListModel output_preference_model_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxOutputPreference)
};

