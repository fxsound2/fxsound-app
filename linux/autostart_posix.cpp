/*
FxSound Linux port — linux/autostart_posix.cpp (Phase 9). Untested.
Replaces HKCU .../Run/FxSound (FxController.cpp:2789-2818).
*/
#include "autostart_posix.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
namespace fxsound_linux {
static fs::path desktopPath() {
  const char* xdg = std::getenv("XDG_CONFIG_HOME");
  std::string base = (xdg && *xdg) ? xdg
      : std::string(std::getenv("HOME") ? std::getenv("HOME") : "/tmp") + "/.config";
  return fs::path(base) / "autostart" / "fxsound.desktop";
}
bool isLaunchOnStartup() {
  std::error_code ec;
  return fs::exists(desktopPath(), ec);
}
bool setLaunchOnStartup(bool on) {
  std::error_code ec;
  if (!on) return fs::remove(desktopPath(), ec);
  fs::create_directories(desktopPath().parent_path(), ec);
  std::ofstream f(desktopPath());
  if (!f) return false;
  f << "[Desktop Entry]\nType=Application\nName=FxSound\n"
       "Exec=fxsound --run_minimized\nX-GNOME-Autostart-enabled=true\n";
  return (bool)f;
}
}  // namespace fxsound_linux
