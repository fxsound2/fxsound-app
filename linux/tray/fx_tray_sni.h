/*
FxSound — Linux build: StatusNotifierItem tray icon.

JUCE's SystemTrayIconComponent only implements the legacy XEmbed tray, which
does not work on Wayland / modern desktops (it shows as a floating window).
This provides a real org.kde.StatusNotifierItem over DBus (GDBus). The item
exposes the icon and routes clicks back to the app; the context menu itself is
rendered by JUCE's PopupMenu (so we don't implement com.canonical.dbusmenu).
*/

#ifndef FX_TRAY_SNI_H
#define FX_TRAY_SNI_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

class FxTraySNI
{
public:
    // Callbacks are invoked on the DBus thread; the caller must marshal to the
    // UI thread (e.g. juce::MessageManager::callAsync).
    FxTraySNI(std::function<void()> on_activate,
              std::function<void()> on_context_menu);
    ~FxTraySNI();

    // True if registration with a StatusNotifierWatcher succeeded.
    bool isActive() const;

    // Update the icon by freedesktop icon-theme name (e.g. "fxsound").
    void setIconName(const std::string& icon_name);

    // Point the tray at an extra icon-theme search directory.
    // The tray looks here first before the system theme for IconName.
    void setIconThemePath(const std::string& path);

    // Provide a raw icon fallback (byte order: RGBA, 4 bytes per pixel).
    void setIconPixmap(int width, int height, std::vector<unsigned char> rgba);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // FX_TRAY_SNI_H
