/*
FxSound — Linux build: StatusNotifierItem tray icon.

JUCE's SystemTrayIconComponent only implements the legacy XEmbed tray, which
does not work on Wayland / modern desktops (it shows as a floating window).
This provides a real org.kde.StatusNotifierItem over DBus (GDBus). The item
exposes the icon and a full com.canonical.dbusmenu so that Wayland panels
(Waybar, KDE Plasma, etc.) render the context menu natively.
*/

#ifndef FX_TRAY_SNI_H
#define FX_TRAY_SNI_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// MenuItem — tree node for the dbusmenu layout.
// Build a tree rooted at id=0 and pass it to FxTraySNI::updateMenu().
// ---------------------------------------------------------------------------
struct MenuItem
{
    int         id               = 0;
    std::string label;
    std::string type             = "standard"; // "standard" | "separator"
    bool        enabled          = true;
    bool        visible          = true;
    std::string toggle_type;     // "" | "checkmark" | "radio"
    int         toggle_state     = -1; // -1=none, 0=off, 1=on
    std::string children_display;      // "submenu" if has children
    std::vector<MenuItem> children;

    static MenuItem separator(int sid)
    {
        MenuItem s; s.id = sid; s.type = "separator"; return s;
    }
};

// ---------------------------------------------------------------------------
class FxTraySNI
{
public:
    // on_activate       : left-click on the tray icon
    // on_item_activated : dbusmenu item clicked (id matches MenuItem::id)
    // Both callbacks are invoked on the DBus/GLib thread; marshal to the
    // JUCE message thread with juce::MessageManager::callAsync.
    FxTraySNI(std::function<void()>    on_activate,
              std::function<void(int)> on_item_activated);
    ~FxTraySNI();

    // True if the SNI was successfully registered with a watcher.
    bool isActive() const;

    // Push a new menu tree; thread-safe, signals LayoutUpdated to the host.
    void updateMenu(MenuItem root);

    // Icon helpers (same semantics as before).
    void setIconName(const std::string& icon_name);
    void setIconThemePath(const std::string& path);
    void setIconPixmap(int width, int height, std::vector<unsigned char> rgba);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // FX_TRAY_SNI_H
