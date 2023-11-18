/*
FxSound
Copyright (C) 2023  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <JuceHeader.h>
#include "FxSystemTrayView.h"

FxSystemTrayView::FxSystemTrayView() : left_click_(false)
{
    FxModel::getModel().addListener(this);
    auto os = SystemStats::getOperatingSystemType();
    if (os == SystemStats::OperatingSystemType::Windows7 ||
        os == SystemStats::OperatingSystemType::Windows8_0 ||
        os == SystemStats::OperatingSystemType::Windows8_1)
    {
        custom_notification_ = true;
    }
    else
    {
        custom_notification_ = false;
    }
}

FxSystemTrayView::~FxSystemTrayView()
{
    removeFromDesktop();
}

void FxSystemTrayView::mouseDown(const MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        context_menu_.clear();

        PopupMenu preset_menu;


        auto id = PRESET_MENU_ID_START;
        auto count = FxModel::getModel().getPresetCount();
        auto preset_type = FxModel::PresetType::AppPreset;
        for (auto i=0; i<count; i++)
        {
            auto preset = FxModel::getModel().getPreset(i);
            PopupMenu::Item menu_item(preset.name);
            menu_item.setID(id);
            if (id - PRESET_MENU_ID_START == FxModel::getModel().getSelectedPreset())
            {
                menu_item.setTicked(true);
                if (FxModel::getModel().isPresetModified())
                {
                    menu_item.text = preset.name + L" *";
                }
            }

            if (preset_type != preset.type)
            {
                preset_menu.addSeparator();
                preset_type = preset.type;
            }

            preset_menu.addItem(menu_item);
            id++;
        }

        PopupMenu output_menu;

        StringArray output_names = FxModel::getModel().getOutputNames();

        id = OUTPUT_MENU_ID_START;
        for (auto name : output_names)
        {
            PopupMenu::Item menu_item(name);
            menu_item.setID(id);
            if (id - OUTPUT_MENU_ID_START == FxModel::getModel().getSelectedOutput())
            {
                menu_item.setTicked(true);
            }
            output_menu.addItem(menu_item);
            id++;
        }

        PopupMenu::Item open(TRANS("Open"));
        PopupMenu::Item power;
        PopupMenu::Item settings(TRANS("Settings"));
        PopupMenu::Item donate(TRANS("Donate"));
        PopupMenu::Item exit(TRANS("Exit"));

        open.setID(MENU_ID_OPEN);
        power.text = FxModel::getModel().getPowerState() ? String(TRANS("Turn Off")) : String(TRANS("Turn On"));
        power.setID(MENU_ID_POWER);
        settings.setID(MENU_ID_SETTINGS);
        donate.setID(MENU_ID_DONATE);
        exit.setID(MENU_ID_EXIT);
        context_menu_.addItem(open);
        context_menu_.addItem(power);
        context_menu_.addSubMenu(TRANS("Preset Select"), preset_menu, FxModel::getModel().getPowerState());
        context_menu_.addSubMenu(TRANS("Playback Device Select"), output_menu);
        context_menu_.addItem(settings);
        context_menu_.addItem(donate);
        context_menu_.addItem(exit);
    }

    if (event.mods.isLeftButtonDown())
    {
        left_click_ = true;
        context_menu_.clear();
    }

    if (event.mods.isMiddleButtonDown())
    {
        context_menu_.clear();
    }
}

void FxSystemTrayView::mouseUp(const MouseEvent&)
{
    if (left_click_)
    {
        if (FxController::getInstance().isMainWindowVisible())
        {
            FxController::getInstance().hideMainWindow();
        }
        else
        {
            FxController::getInstance().showMainWindow();
        }
        
        left_click_ = false;
        return;
    }

    if (context_menu_.getNumItems() > 0)
    {
        int id = context_menu_.show();

        if (id == MENU_ID_OPEN)
        {
            FxController::getInstance().showMainWindow();
            return;
        }

        if (id == MENU_ID_POWER)
        {
            FxController::getInstance().setPowerState(!FxModel::getModel().getPowerState());
            return;
        }

        if (id == MENU_ID_DONATE)
        {
            URL url("https://www.paypal.com/donate/?hosted_button_id=JVNQGYXCQ2GPG");
            url.launchInDefaultBrowser();
            return;
        }

        if (id == MENU_ID_EXIT)
        {
            if (FxController::getInstance().exit())
            {
                removeFromDesktop();
            }
            return;
        }

        if (id == MENU_ID_SETTINGS)
        {
            FxSettingsDialog settings_dialog;
            settings_dialog.runModalLoop();            
            return;
        }

        auto num_presets = FxModel::getModel().getPresetCount();
        if (id >= PRESET_MENU_ID_START && id <= PRESET_MENU_ID_START + num_presets)
        {
            FxController::getInstance().setPreset(id - PRESET_MENU_ID_START);
            return;
        }

        auto num_outputs = FxModel::getModel().getOutputNames().size();
        if (id >= OUTPUT_MENU_ID_START && id <= OUTPUT_MENU_ID_START + num_outputs)
        {
            FxController::getInstance().setOutput(id - OUTPUT_MENU_ID_START);
            return;
        }
    }
}

void FxSystemTrayView::modelChanged(FxModel::Event model_event)
{
    if (model_event == FxModel::Event::Notification)
    {
        showNotification();
    }
}

void FxSystemTrayView::showNotification()
{
    String message;
    std::pair<String, String> link;
    FxModel::getModel().popMessage(message, link);
    NOTIFYICONDATA* icon_data = (NOTIFYICONDATA*)getNativeHandle();

    if (message.isNotEmpty() && icon_data != nullptr)
    {
        if (custom_notification_ || link.first.isNotEmpty())
        {
            NOTIFYICONIDENTIFIER icon_id = {};
            RECT rect;

            icon_id.cbSize = sizeof(NOTIFYICONIDENTIFIER);
            icon_id.hWnd = icon_data->hWnd;
            icon_id.uID = icon_data->uID;

            if (FAILED(Shell_NotifyIconGetRect(&icon_id, &rect)))
            {
                return;
            }

            QUERY_USER_NOTIFICATION_STATE quns;
            if (FAILED(SHQueryUserNotificationState(&quns)) || quns != QUNS_ACCEPTS_NOTIFICATIONS)
            {
                return;
            }

            notification_.setMessage(message, link);

            auto display = Desktop::getInstance().getDisplays().getPrimaryDisplay();
            if (display != nullptr)
            {
                auto area = display->userArea;
                Point<int> pos = {};
                if (rect.left < area.getCentreX())
                {
                    pos.x = area.getX() + 20;
                }
                else
                {
                    pos.x = area.getRight() - notification_.getWidth() - 20;
                }
                if (rect.top < area.getCentreY())
                {
                    pos.y = area.getY() + 20;
                }
                else
                {
                    pos.y = area.getBottom() - notification_.getHeight() - 20;
                }
                notification_.setBounds(pos.x, pos.y, notification_.getWidth(), notification_.getHeight());
                notification_.showMessage();
            }            
        }
        else
        {
            icon_data->uFlags = NIF_INFO | NIF_REALTIME;
            icon_data->dwInfoFlags = NIIF_NOSOUND | NIIF_RESPECT_QUIET_TIME;

            String title = L"FxSound";
            title.copyToUTF16(icon_data->szInfoTitle, sizeof(icon_data->szInfoTitle) - 1);
            message.copyToUTF16(icon_data->szInfo, sizeof(icon_data->szInfo) - 1);

            Shell_NotifyIcon(NIM_MODIFY, icon_data);
        }
    }
}