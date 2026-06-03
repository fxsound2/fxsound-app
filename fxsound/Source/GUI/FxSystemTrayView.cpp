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
#include "FxSystemTrayView.h"

// {A8E96325-5269-443C-A0D8-0D02562FE553}
const GUID FxSystemTrayView::trayIconGuid_ =
{ 0xa8e96325, 0x5269, 0x443c, { 0xa0, 0xd8, 0xd, 0x2, 0x56, 0x2f, 0xe5, 0x53 } };


FxSystemTrayView::FxSystemTrayView()
{
    FxModel::getModel().addListener(this);

    custom_notification_ = true;

#ifdef _WIN32
    addToDesktop(0);

    HWND hWnd = (HWND)getWindowHandle();

    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    componentWndProc_ = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)wndProc);

    taskbar_created_message_ = RegisterWindowMessage(TEXT("TaskbarCreated"));

    addIcon();
#else
    // Linux: StatusNotifierItem + com.canonical.dbusmenu (GDBus).
    // Callbacks arrive on the GLib thread; marshal to the JUCE message thread.
    // Left click toggles the main window.
    // Menu items are served via dbusmenu so Wayland panels (Waybar, KDE) render
    // the menu natively — no JUCE popup required.
    auto on_activate = []() {
        juce::MessageManager::callAsync([]() {
            auto& c = FxController::getInstance();
            if (c.isMainWindowVisible()) c.hideMainWindow();
            else c.showMainWindow();
        });
    };
    auto on_item_activated = [this](int id) {
        juce::MessageManager::callAsync([this, id]() { handleMenuActivation(id); });
    };
    tray_sni_.reset(new FxTraySNI(on_activate, on_item_activated));
    tray_sni_->setIconName("fxsound");

    // Write the embedded PNG into a temp hicolor tree so the SNI host can load
    // it by name (avoids byte-order issues in raw pixmap delivery).
    {
        juce::File icon_dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                  .getChildFile("fxsound-tray-icons")
                                  .getChildFile("hicolor")
                                  .getChildFile("32x32")
                                  .getChildFile("apps");
        icon_dir.createDirectory();
        juce::File icon_file = icon_dir.getChildFile("fxsound.png");
        if (!icon_file.existsAsFile())
            icon_file.replaceWithData(BinaryData::fxsound_png,
                                      (size_t)BinaryData::fxsound_pngSize);

        juce::File theme_root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                    .getChildFile("fxsound-tray-icons");
        tray_sni_->setIconThemePath(theme_root.getFullPathName().toStdString());
    }

    // Fallback raw RGBA pixmap for hosts that ignore IconThemePath.
    {
        juce::Image img = juce::ImageFileFormat::loadFrom(BinaryData::fxsound_png,
                                                          BinaryData::fxsound_pngSize);
        if (img.isValid())
        {
            img = img.rescaled(32, 32, juce::Graphics::highResamplingQuality);
            const int w = img.getWidth(), h = img.getHeight();
            std::vector<unsigned char> rgba;
            rgba.reserve((size_t)w * h * 4);
            juce::Image::BitmapData bd(img, juce::Image::BitmapData::readOnly);
            for (int py = 0; py < h; ++py)
                for (int px = 0; px < w; ++px)
                {
                    auto col = bd.getPixelColour(px, py);
                    rgba.push_back(col.getRed());
                    rgba.push_back(col.getGreen());
                    rgba.push_back(col.getBlue());
                    rgba.push_back(col.getAlpha());
                }
            tray_sni_->setIconPixmap(w, h, std::move(rgba));
        }
    }

    // Push the initial menu so the host has something to display immediately.
    pushDbusmenu();
#endif
}

FxSystemTrayView::~FxSystemTrayView()
{
#ifdef _WIN32
    HWND hWnd = (HWND)getWindowHandle();

    SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, NULL);

    NOTIFYICONDATA nid = { sizeof(nid) };
    nid.uFlags = NIF_GUID;
    nid.guidItem = trayIconGuid_;
    Shell_NotifyIcon(NIM_DELETE, &nid);

    removeFromDesktop();
#endif
}

void FxSystemTrayView::modelChanged(FxModel::Event model_event)
{
    if (!FxController::getInstance().isNotificationsHidden() && model_event == FxModel::Event::Notification)
    {
        showNotification();
    }
#ifndef _WIN32
    // Rebuild the dbusmenu whenever any model state that affects the menu changes.
    switch (model_event)
    {
    case FxModel::Event::PresetSelected:
    case FxModel::Event::PresetListUpdated:
    case FxModel::Event::PresetModified:
    case FxModel::Event::OutputSelected:
    case FxModel::Event::OutputListUpdated:
    case FxModel::Event::Other:
        pushDbusmenu();
        break;
    default:
        break;
    }
#endif
}

void FxSystemTrayView::setStatus(bool power, bool processing)
{
#ifdef _WIN32
    HINSTANCE hInst = GetModuleHandle(NULL);

    String param = power ? TRANS(L"on") : TRANS(L"off");

    auto& model = FxModel::getModel();
    String preset_name = model.getPreset(model.getSelectedPreset()).name;

    wchar_t tool_tip[1024];
    swprintf_s(tool_tip, String(TRANS("FxSound is %s.")).toWideCharPointer(), param.toWideCharPointer());
	wcscat_s(tool_tip, 1024, L"\n\n");
    wcscat_s(tool_tip, 1024, String(TRANS("Output: ")).toWideCharPointer());
	String output_device_name = model.getSelectedOutput().deviceFriendlyName.c_str();
	wcscat_s(tool_tip, 1024, output_device_name.toWideCharPointer());
    if (preset_name.isNotEmpty())
    {
        wcscat_s(tool_tip, 1024, L"\n");
        wcscat_s(tool_tip, 1024, String(TRANS("Preset: ")).toWideCharPointer());
        wcscat_s(tool_tip, 1024, preset_name.toWideCharPointer());
    }

    NOTIFYICONDATA nid = { sizeof(nid) };

    nid.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_GUID;
    nid.guidItem = trayIconGuid_;
    if (power)
    {
        if (processing)
        {
            if (FxTheme::getThemeMode() == FxThemeMode::Dark)
            {
                nid.hIcon = LoadIcon(hInst, L"IDI_LOGO_RED");
            }
            else
            {
                nid.hIcon = LoadIcon(hInst, L"IDI_LOGO_BLUE");
            }
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
#else
    ignoreUnused(processing);
    if (tray_sni_)
    {
        auto& model = FxModel::getModel();
        String preset_name = model.getPreset(model.getSelectedPreset()).name;
        String title = String("FxSound ") + (power ? TRANS("on") : TRANS("off"));
        if (preset_name.isNotEmpty())
            title += "\n" + TRANS("Preset: ") + preset_name;
        tray_sni_->setTitle(title.toStdString());
    }
#endif
}

Point<int> FxSystemTrayView::getSystemTrayWindowPosition(int width, int height)
{
    Point<int> pos = { 0, 0 };

#ifdef _WIN32
    NOTIFYICONIDENTIFIER icon_id = {};
    RECT rect;

    HWND hWnd = (HWND)getWindowHandle();

    icon_id.cbSize = sizeof(NOTIFYICONIDENTIFIER);
    icon_id.hWnd = hWnd;
    icon_id.guidItem = trayIconGuid_;

    if (FAILED(Shell_NotifyIconGetRect(&icon_id, &rect)))
    {
        return pos;
    }

    auto display = Desktop::getInstance().getDisplays().getPrimaryDisplay();
    if (display != nullptr)
    {
        juce::Rectangle<int> prect{ rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top };
        auto lrect = Desktop::getInstance().getDisplays().physicalToLogical(prect);

        auto area = display->userArea;

        if (lrect.getX() < area.getCentreX())
        {
            pos.x = area.getX() + 10;
        }
        else
        {
            pos.x = area.getRight() - width - 10;
        }


        if (lrect.getY() < area.getCentreY())
        {
            pos.y = area.getY() + 10;
        }
        else
        {
            pos.y = area.getBottom() - height - 10;
        }
    }
#else
    // Linux: no tray-rect query; anchor the notification to the primary
    // display's top-right corner (M5 refines against the real tray location).
    auto display = Desktop::getInstance().getDisplays().getPrimaryDisplay();
    if (display != nullptr)
    {
        auto area = display->userArea;
        pos.x = area.getRight() - width - 10;
        pos.y = area.getY() + 10;
    }
#endif

    return pos;
}

void FxSystemTrayView::addIcon()
{
#ifdef _WIN32
    NOTIFYICONDATA nid = { sizeof(nid) };

    HINSTANCE hInst = GetModuleHandle(NULL);
    HWND hWnd = (HWND)getWindowHandle();

    if (FxModel::getModel().getPowerState())
    {
        if (FxController::getInstance().isAudioProcessing())
        {
            if (FxTheme::getThemeMode() == FxThemeMode::Dark)
            {
                nid.hIcon = LoadIcon(hInst, L"IDI_LOGO_RED");
            }
            else
            {
                nid.hIcon = LoadIcon(hInst, L"IDI_LOGO_BLUE");
            }
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

    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
    nid.guidItem = trayIconGuid_;
    nid.uCallbackMessage = WMAPP_FXTRAYICON;
    nid.hWnd = hWnd;
    lstrcpy(nid.szTip, L"FxSound");
    Shell_NotifyIcon(NIM_ADD, &nid);

    // NOTIFYICON_VERSION_4 is prefered
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIcon(NIM_SETVERSION, &nid);

    setVisible(true);
#endif
}

void FxSystemTrayView::showContextMenu(int x, int y)
{
    PopupMenu context_menu;

    PopupMenu preset_menu;
    PopupMenu theme_menu;

    FxController::getInstance().checkDeviceChanges();

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

    auto darkModeClicked = [this]() {
        FxController::getInstance().setThemeMode(FxThemeMode::Dark);
        };

    auto lightModeClicked = [this]() {
        FxController::getInstance().setThemeMode(FxThemeMode::Light);
        };

    auto alwaysOnTopClicked = []() {
        auto& controller = FxController::getInstance();
        controller.setAlwaysOnTop(!controller.isAlwaysOnTop());
    };

    auto donateClicked = []() {
        URL url("https://www.paypal.com/donate/?hosted_button_id=JVNQGYXCQ2GPG");
        url.launchInDefaultBrowser();
    };

    auto exitClicked = []() {
        FxController::getInstance().exit();
    };

    theme_menu.addItem(TRANS("Dark"), true, FxTheme::getThemeMode() == FxThemeMode::Dark, darkModeClicked);
    theme_menu.addItem(TRANS("Light"), true, FxTheme::getThemeMode() == FxThemeMode::Light, lightModeClicked);

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
    if (FxModel::getModel().getPowerState())
    {
        context_menu.addSubMenu(TRANS("Preset Select"), preset_menu);
    }
    addOutputDeviceMenu(&context_menu);
    context_menu.addItem(settings);
    context_menu.addSubMenu(TRANS("Theme"), theme_menu);
    context_menu.addItem(TRANS("Always On Top"), true, FxController::getInstance().getMainWindow()->isAlwaysOnTop(), alwaysOnTopClicked);
    context_menu.addItem(donate);
    context_menu.addItem(exit);

#ifdef _WIN32
    HWND hWnd = (HWND)getWindowHandle();

    SetFocus(hWnd);
    SetForegroundWindow(hWnd);
    context_menu.show();
#else
    // Use the screen coordinates from the SNI host so the popup appears near
    // the tray icon. If the host sent (0,0) fall back to the cursor position.
    if (x != 0 || y != 0)
    {
        juce::Rectangle<int> anchor(x, y, 1, 1);
        context_menu.showMenuAsync(PopupMenu::Options()
            .withTargetScreenArea(anchor)
            .withMinimumWidth(180));
    }
    else
    {
        context_menu.showMenuAsync(PopupMenu::Options()
            .withMinimumWidth(180));
    }
#endif
}

void FxSystemTrayView::addOutputDeviceMenu(PopupMenu* context_menu)
{
    PopupMenu output_menu;
    PopupMenu* menu;

    auto output_devices = FxModel::getModel().getOutputDevices();
    int num_outputs = output_devices.size();
    if (num_outputs > 5)
    {
        menu = &output_menu;
    }
    else
    {
        menu = context_menu;
        menu->addSeparator();
        menu->addSectionHeader(TRANS("Playback Device Select"));
    }

    auto id = OUTPUT_MENU_ID_START;
    for (auto device : output_devices)
    {
        PopupMenu::Item menu_item(getTruncatedText(device.deviceFriendlyName.c_str(), 30));
        menu_item.setID(id);
        if (device.deviceNumChannel < 2)
        {
            menu_item.setEnabled(false);
        }

        if (id - OUTPUT_MENU_ID_START == FxModel::getModel().getSelectedOutputIndex())
        {
            menu_item.setTicked(true);
        }

        auto handler = [output = id]() {
            FxController::getInstance().setOutput(output - OUTPUT_MENU_ID_START);
            };
        menu_item.setAction(handler);

        menu->addItem(menu_item);
        id++;
    }

    if (num_outputs > 5)
    {
        context_menu->addSubMenu(TRANS("Playback Device Select"), output_menu);
    }
    else
    {
        menu->addSeparator();
    }
}

void FxSystemTrayView::showNotification()
{
    String message;
    std::pair<String, String> link;
    FxModel::getModel().popMessage(message, link);

    if (message.isNotEmpty())
    {
        if (custom_notification_ || link.first.isNotEmpty())
        {
#ifdef _WIN32
            QUERY_USER_NOTIFICATION_STATE quns;
            if (FAILED(SHQueryUserNotificationState(&quns)) || quns != QUNS_ACCEPTS_NOTIFICATIONS)
            {
                return;
            }
#endif

            notification_.setMessage(message, link);
            Point<int> pos = getSystemTrayWindowPosition(notification_.getWidth(), notification_.getHeight());
            notification_.setBounds(pos.x, pos.y, notification_.getWidth(), notification_.getHeight());
            notification_.showMessage();
        }
        else
        {
#ifdef _WIN32
            NOTIFYICONDATA nid = { sizeof(nid) };

            nid.uFlags = NIF_INFO | NIF_GUID | NIF_REALTIME;
            nid.guidItem = trayIconGuid_;
            nid.dwInfoFlags = NIIF_NOSOUND | NIIF_RESPECT_QUIET_TIME;

            String title = L"FxSound";
            title.copyToUTF16(nid.szInfoTitle, sizeof(nid.szInfoTitle) - 1);
            message.copyToUTF16(nid.szInfo, sizeof(nid.szInfo) - 1);

            Shell_NotifyIcon(NIM_MODIFY, &nid);
#endif
        }
    }
}

String FxSystemTrayView::getTruncatedText(const String& text, int max_length)
{
    if (text.length() <= max_length)
        return text;
    else
    {
        const String ellipsis = "...";
        int trunc_length = (text.length() - max_length) + ellipsis.length();
        return text.dropLastCharacters(trunc_length) + ellipsis;
    }
}

// ---------------------------------------------------------------------------
// Linux-only: dbusmenu builder + item activation handler
// ---------------------------------------------------------------------------
#ifndef _WIN32

// Item ID constants — must match handleMenuActivation switch.
static constexpr int kIdOpen       =  1;
static constexpr int kIdPower      =  2;
static constexpr int kIdSep1       =  3;
static constexpr int kIdPresets    =  4;
static constexpr int kIdOutputs    =  5;
static constexpr int kIdSep2       =  6;
static constexpr int kIdSettings   =  7;
static constexpr int kIdTheme      =  8;
static constexpr int kIdThemeDark  =  9;
static constexpr int kIdThemeLight = 10;
static constexpr int kIdAlwaysTop  = 11;
static constexpr int kIdSep3       = 12;
static constexpr int kIdDonate     = 13;
static constexpr int kIdExit       = 14;
static constexpr int kPresetBase   = 101; // 101..199
static constexpr int kOutputBase   = 201; // 201..299

MenuItem FxSystemTrayView::buildDbusmenu()
{
    auto& model = FxModel::getModel();
    auto& ctrl  = FxController::getInstance();

    bool power          = model.getPowerState();
    int  selPreset      = model.getSelectedPreset();
    bool presetModified = model.isPresetModified();
    int  selOutput      = model.getSelectedOutputIndex();
    bool aot            = ctrl.isAlwaysOnTop();
    bool isDark         = FxTheme::getThemeMode() == FxThemeMode::Dark;

    MenuItem root;
    root.id = 0;

    // Open
    MenuItem open; open.id = kIdOpen; open.label = TRANS("Open").toStdString();
    root.children.push_back(open);

    // Power toggle
    MenuItem pwr;
    pwr.id    = kIdPower;
    pwr.label = power ? TRANS("Turn Off").toStdString() : TRANS("Turn On").toStdString();
    root.children.push_back(pwr);

    root.children.push_back(MenuItem::separator(kIdSep1));

    // Presets submenu
    {
        MenuItem presets;
        presets.id               = kIdPresets;
        presets.label            = TRANS("Preset Select").toStdString();
        presets.children_display = "submenu";
        presets.enabled          = power;

        int count = model.getPresetCount();
        FxModel::PresetType last_type = FxModel::PresetType::AppPreset;
        static constexpr int kSepBase = 150;
        int sep_id = kSepBase;
        for (int i = 0; i < count; ++i)
        {
            auto preset = model.getPreset(i);
            if (i > 0 && preset.type != last_type)
            {
                presets.children.push_back(MenuItem::separator(sep_id++));
                last_type = preset.type;
            }
            MenuItem pi;
            pi.id          = kPresetBase + i;
            pi.label       = (i == selPreset && presetModified)
                             ? (preset.name + " *").toStdString()
                             : preset.name.toStdString();
            pi.toggle_type  = "radio";
            pi.toggle_state = (i == selPreset) ? 1 : 0;
            presets.children.push_back(pi);
        }
        root.children.push_back(presets);
    }

    // Outputs submenu
    {
        MenuItem outputs;
        outputs.id               = kIdOutputs;
        outputs.label            = TRANS("Playback Device Select").toStdString();
        outputs.children_display = "submenu";

        auto devices = model.getOutputDevices();
        for (int i = 0; i < (int)devices.size(); ++i)
        {
            MenuItem oi;
            oi.id          = kOutputBase + i;
            oi.label       = getTruncatedText(
                                 String(devices[i].deviceFriendlyName.c_str()), 35)
                                 .toStdString();
            oi.enabled     = (devices[i].deviceNumChannel >= 2);
            oi.toggle_type  = "radio";
            oi.toggle_state = (i == selOutput) ? 1 : 0;
            outputs.children.push_back(oi);
        }
        root.children.push_back(outputs);
    }

    root.children.push_back(MenuItem::separator(kIdSep2));

    // Settings
    MenuItem settings;
    settings.id = kIdSettings; settings.label = TRANS("Settings").toStdString();
    root.children.push_back(settings);

    // Theme submenu
    {
        MenuItem theme;
        theme.id               = kIdTheme;
        theme.label            = TRANS("Theme").toStdString();
        theme.children_display = "submenu";

        MenuItem dark;
        dark.id = kIdThemeDark; dark.label = TRANS("Dark").toStdString();
        dark.toggle_type = "radio"; dark.toggle_state = isDark ? 1 : 0;
        theme.children.push_back(dark);

        MenuItem light;
        light.id = kIdThemeLight; light.label = TRANS("Light").toStdString();
        light.toggle_type = "radio"; light.toggle_state = isDark ? 0 : 1;
        theme.children.push_back(light);

        root.children.push_back(theme);
    }

    // Always On Top
    MenuItem aotItem;
    aotItem.id          = kIdAlwaysTop;
    aotItem.label       = TRANS("Always On Top").toStdString();
    aotItem.toggle_type  = "checkmark";
    aotItem.toggle_state = aot ? 1 : 0;
    root.children.push_back(aotItem);

    root.children.push_back(MenuItem::separator(kIdSep3));

    // Donate
    MenuItem donate;
    donate.id = kIdDonate; donate.label = TRANS("Donate").toStdString();
    root.children.push_back(donate);

    // Exit
    MenuItem exitItem;
    exitItem.id = kIdExit; exitItem.label = TRANS("Exit").toStdString();
    root.children.push_back(exitItem);

    return root;
}

void FxSystemTrayView::pushDbusmenu()
{
    if (tray_sni_)
        tray_sni_->updateMenu(buildDbusmenu());
}

void FxSystemTrayView::handleMenuActivation(int id)
{
    auto& ctrl  = FxController::getInstance();
    auto& model = FxModel::getModel();

    if (id == kIdOpen)
    {
        ctrl.showMainWindow();
    }
    else if (id == kIdPower)
    {
        ctrl.setPowerState(!model.getPowerState());
        pushDbusmenu(); // reflect new state immediately
    }
    else if (id == kIdSettings)
    {
        FxSettingsDialog dlg;
        dlg.runModalLoop();
    }
    else if (id == kIdThemeDark)
    {
        ctrl.setThemeMode(FxThemeMode::Dark);
        pushDbusmenu();
    }
    else if (id == kIdThemeLight)
    {
        ctrl.setThemeMode(FxThemeMode::Light);
        pushDbusmenu();
    }
    else if (id == kIdAlwaysTop)
    {
        ctrl.setAlwaysOnTop(!ctrl.isAlwaysOnTop());
        pushDbusmenu();
    }
    else if (id == kIdDonate)
    {
        URL("https://www.paypal.com/donate/?hosted_button_id=JVNQGYXCQ2GPG")
            .launchInDefaultBrowser();
    }
    else if (id == kIdExit)
    {
        ctrl.exit();
    }
    else if (id >= kPresetBase && id < kPresetBase + 99)
    {
        ctrl.setPreset(id - kPresetBase);
    }
    else if (id >= kOutputBase && id < kOutputBase + 99)
    {
        ctrl.setOutput(id - kOutputBase);
    }
}

#endif // !_WIN32

// ---------------------------------------------------------------------------
#ifdef _WIN32
LRESULT CALLBACK FxSystemTrayView::wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto tray_view = reinterpret_cast<FxSystemTrayView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (message == tray_view->taskbar_created_message_)
    {
        tray_view->addIcon();
    }
    else if (message == WMAPP_FXTRAYICON)
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
#endif // _WIN32