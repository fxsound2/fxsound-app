/*
FxSound Linux port — linux/paths_posix.h (Phase 8)
Replaces GetModuleFileName + SetCurrentDirectory + %APPDATA%/%DOCS% strings.
Use File::getChildFile everywhere (see APP_LAYER.md).
  Factory (read-only): /usr/share/fxsound/Factsoft, fallback exe-adjacent Factsoft.
  User + edits: $XDG_DATA_HOME/fxsound/presets/ (default ~/.local/share/...).
  UI merges lists, factory read-only, writes go to user dir.
  Secure (.secure/wincrypt): not sensitive (hotkeys/preset/power defaults) -> plain
    file 0600 under ~/.config/FxSound/. Never /var/lib.
Untested.
*/
#pragma once
#include <string>

namespace fxsound_linux {
std::string factoryPresetDir(const std::string& exeDir);
std::string userPresetDir();
std::string autoSaveDir();
std::string statusJsonPath();  // ~/.config/FxSound/status.json (Linux also --status stdout)
}  // namespace fxsound_linux
