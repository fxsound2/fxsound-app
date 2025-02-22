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

// {A8E96325-5269-443C-A0D8-0D02562FE553}
const GUID FxSystemTrayView::trayIconGuid_ =
{ 0xa8e96325, 0x5269, 0x443c, { 0xa0, 0xd8, 0xd, 0x2, 0x56, 0x2f, 0xe5, 0x53 } };


FxSystemTrayView::FxSystemTrayView()
{
    FxModel::getModel().addListener(this);

    custom_notification_ = true;

    addToDesktop(0);

    NOTIFYICONDATA nid = { sizeof(nid) };

    HINSTANCE hInst = GetModuleHandle(NULL);
    HWND hWnd = (HWND)getWindowHandle();

    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    componentWndProc_ = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)wndProc);

    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
    nid.guidItem = trayIconGuid_;
    nid.uCallbackMessage = WMAPP_FXTRAYICON;
    nid.hIcon = LoadIcon(hInst, L"IDI_LOGO_WHITE");    
    nid.hWnd = hWnd;
    lstrcpy(nid.szTip, L"FxSound");
    Shell_NotifyIcon(NIM_ADD, &nid);

    // NOTIFYICON_VERSION_4 is prefered
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIcon(NIM_SETVERSION, &nid);

    setVisible(true);
}

FxSystemTrayView::~FxSystemTrayView()
{
    HWND hWnd = (HWND)getWindowHandle();

    SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, NULL);

    NOTIFYICONDATA nid = { sizeof(nid) };
    nid.uFlags = NIF_GUID;
    nid.guidItem = trayIconGuid_;
    Shell_NotifyIcon(NIM_DELETE, &nid);

    removeFromDesktop();
}

void FxSystemTrayView::modelChanged(FxModel::Event model_event)
{
    if (!FxController::getInstance().isNotificationsHidden() && model_event == FxModel::Event::Notification)
    {
        showNotification();
    }
}

void FxSystemTrayView::setStatus(bool power, bool processing)
{
    HINSTANCE hInst = GetModuleHandle(NULL);

    String param = power ? TRANS(L"on") : TRANS(L"off");

    wchar_t tool_tip[1024];
    swprintf_s(tool_tip, String(TRANS("FxSound is %s.")).toWideCharPointer(), param.toWideCharPointer());

    NOTIFYICONDATA nid = { sizeof(nid) };

    nid.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_GUID;
    nid.guidItem = trayIconGuid_;
    if (power)
    {
        if (processing)
        {
            nid.hIcon = LoadIcon(hInst, L"IDI_LOGO_RED");
        }
        else
        {
            nid.hIcon = LoadIcon(hInst, L"IDI_LOGO_WHITE");
        }
    }
    else
    {
        nid.hIcon = LoadIcon(hInst, L"IDI_LOGO_GRAY");
    }

    if (nid.hIcon == NULL)
    {
        return;
    }

    lstrcpy(nid.szTip, tool_tip);

    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void FxSystemTrayView::showContextMenu()
{    
    PopupMenu context_menu;

    PopupMenu preset_menu;

    auto id = PRESET_MENU_ID_START;
    auto count = FxModel::getModel().getPresetCount();
    auto preset_type = FxModel::PresetType::AppPreset;
    for (auto i = 0; i < count; i++)
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

        auto handler = [preset = id]() {
            FxController::getInstance().setPreset(preset - PRESET_MENU_ID_START);
        };
        menu_item.setAction(handler);

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
        if (name.endsWith("[Mono]"))
        {
            menu_item.setEnabled(false);
        }

        if (id - OUTPUT_MENU_ID_START == FxModel::getModel().getSelectedOutput())
        {
            menu_item.setTicked(true);
        }

        auto handler = [output = id]() {
            FxController::getInstance().setOutput(output - OUTPUT_MENU_ID_START);
        };
        menu_item.setAction(handler);

        output_menu.addItem(menu_item);
        id++;
    }

    auto openClicked = []() {
        FxController::getInstance().showMainWindow();
    };

    auto powerClicked = []() {
        FxController::getInstance().setPowerState(!FxModel::getModel().getPowerState());
    };

    auto settingsClicked = []() {
        FxSettingsDialog settings_dialog;
        settings_dialog.runModalLoop();
    };

    auto donateClicked = []() {
        URL url("https://www.paypal.com/donate/?hosted_button_id=JVNQGYXCQ2GPG");
        url.launchInDefaultBrowser();
    };

    auto exitClicked = []() {
        FxController::getInstance().exit();
    };

    PopupMenu::Item open(TRANS("Open"));
    PopupMenu::Item power;
    PopupMenu::Item settings(TRANS("Settings"));
    PopupMenu::Item donate(TRANS("Donate"));
    PopupMenu::Item exit(TRANS("Exit"));

    open.setID(MENU_ID_OPEN);
    open.setAction(openClicked);
    power.text = FxModel::getModel().getPowerState() ? String(TRANS("Turn Off")) : String(TRANS("Turn On"));    
    power.setID(MENU_ID_POWER);
    power.setAction(powerClicked);
    settings.setID(MENU_ID_SETTINGS);
    settings.setAction(settingsClicked);
    donate.setID(MENU_ID_DONATE);
    donate.setAction(donateClicked);
    exit.setID(MENU_ID_EXIT);
    exit.setAction(exitClicked);

    context_menu.addItem(open);
    context_menu.addItem(power);
    context_menu.addSubMenu(TRANS("Preset Select"), preset_menu, FxModel::getModel().getPowerState());
    context_menu.addSubMenu(TRANS("Playback Device Select"), output_menu);
    context_menu.addItem(settings);
    context_menu.addItem(donate);
    context_menu.addItem(exit);

    HWND hWnd = (HWND)getWindowHandle();

    SetFocus(hWnd);
    SetForegroundWindow(hWnd);

    context_menu.show();
}

void FxSystemTrayView::showNotification()
{
    String message;
    std::pair<String, String> link;
    FxModel::getModel().popMessage(message, link);

    HWND hWnd = (HWND)getWindowHandle();

    if (message.isNotEmpty())
    {
        if (custom_notification_ || link.first.isNotEmpty())
        {
            NOTIFYICONIDENTIFIER icon_id = {};
            RECT rect;

            icon_id.cbSize = sizeof(NOTIFYICONIDENTIFIER);
            icon_id.hWnd = hWnd;
            icon_id.guidItem = trayIconGuid_;

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
            NOTIFYICONDATA nid = { sizeof(nid) };

            nid.uFlags = NIF_INFO | NIF_GUID | NIF_REALTIME;
            nid.guidItem = trayIconGuid_;
            nid.dwInfoFlags = NIIF_NOSOUND | NIIF_RESPECT_QUIET_TIME;

            String title = L"FxSound";
            title.copyToUTF16(nid.szInfoTitle, sizeof(nid.szInfoTitle) - 1);
            message.copyToUTF16(nid.szInfo, sizeof(nid.szInfo) - 1);

            Shell_NotifyIcon(NIM_MODIFY, &nid);
        }
    }
}

LRESULT CALLBACK FxSystemTrayView::wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto tray_view = reinterpret_cast<FxSystemTrayView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (message == WMAPP_FXTRAYICON)
    {
        switch (LOWORD(lParam))
        {
        case NIN_SELECT:
            if (FxController::getInstance().isMainWindowVisible())
            {
                FxController::getInstance().hideMainWindow();
            }
            else
            {
                FxController::getInstance().showMainWindow();
            }
            break;

        case NIN_BALLOONTIMEOUT:
        case NIN_BALLOONUSERCLICK:
        {
            NOTIFYICONDATA nid = { sizeof(nid) };
            nid.uFlags = NIF_SHOWTIP | NIF_GUID;
            nid.guidItem = trayIconGuid_;
            Shell_NotifyIcon(NIM_MODIFY, &nid);
        }
        break;

        case WM_CONTEXTMENU:
            tray_view->showContextMenu();
            break;
        }
    }
    else
    {
        return CallWindowProc(tray_view->componentWndProc_, hwnd, message, wParam, lParam);
    }

    return 0;
}