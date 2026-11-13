/*
FxSound Linux port — linux/tray_sni.h (Phase 9 v1.1 skeleton)
Do NOT use JUCE SystemTrayIconComponent (SNI vs XEmbed differs by JUCE version;
check chosen JUCE source before using). Implement FxSystemTrayView as
StatusNotifierItem over sd-bus:
  register on org.kde.StatusNotifierWatcher, expose IconPixmap, Activate, Menu.
  com.canonical.dbusmenu: GetLayout, GetGroupProperties, Event, AboutToShow,
  LayoutUpdated so Plasma draws the menu itself.
Do NOT rely on showing a JUCE popup at passed coordinates (Wayland forbids).
Ship v1 with no tray if this slips (see DESKTOP.md).
Untested. Requires systemd-devel/dbus-devel (see BUILD.md).
*/
#pragma once

#include <string>

namespace fxsound_linux {
// v1: no-tray fallback returns false; v1.1 implements SNI registration.
bool traySniStart();
void traySniStop();
void traySniSetStatus(bool powerOn, bool processing, const std::string& presetName = "");
}  // namespace fxsound_linux
