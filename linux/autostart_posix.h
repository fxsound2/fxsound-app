/*
FxSound Linux port — linux/autostart_posix.h (Phase 9)
Plasma 6 / Wayland. JUCE runs under XWayland — check `echo $XDG_SESSION_TYPE`.
Autostart: write/remove ~/.config/autostart/fxsound.desktop
  Exec=fxsound --run_minimized
in setLaunchOnStartup/isLaunchOnStartup (replaces HKCU Run key).
Untested.
*/
#pragma once

namespace fxsound_linux {
bool isLaunchOnStartup();
bool setLaunchOnStartup(bool on);
}  // namespace fxsound_linux
