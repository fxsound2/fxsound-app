/*
FxSound Linux port — linux/paths_posix.cpp
*/
#include "paths_posix.h"
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace fxsound_linux {

static std::string xdg(const char* key, const char* fallback) {
  const char* v = std::getenv(key);
  if (v && *v) return v;
  const char* h = std::getenv("HOME");
  return std::string(h ? h : "/tmp") + fallback;
}

static bool checkDirHasPresets(const fs::path& p) {
  std::error_code ec;
  if (!fs::is_directory(p, ec)) return false;
  for (const auto& entry : fs::directory_iterator(p, ec)) {
    if (entry.path().extension() == ".fac") {
      return true;
    }
  }
  return false;
}

std::string factoryPresetDir(const std::string& exeDir) {
  std::error_code ec;

  // 0. Explicit override via environment variable if set
  const char* envDir = std::getenv("FXSOUND_PRESET_DIR");
  if (envDir && *envDir) {
    fs::path p(envDir);
    if (checkDirHasPresets(p)) return p.string();
  }

  // 1. Relative to executable (covers AppImage mount, extracted tarball, and prefix installs)
  if (!exeDir.empty()) {
    // Standard Unix prefix layout: e.g. /usr/bin/FxSound -> /usr/share/fxsound/Factsoft,
    // AppDir: /tmp/.mount_*/usr/bin -> /tmp/.mount_*/usr/share/fxsound/Factsoft,
    // or extracted tarball: bin/ -> share/fxsound/Factsoft
    fs::path p_share = fs::path(exeDir) / ".." / "share" / "fxsound" / "Factsoft";
    if (checkDirHasPresets(p_share)) return fs::canonical(p_share, ec).string();

    fs::path p_share_direct = fs::path(exeDir) / "share" / "fxsound" / "Factsoft";
    if (checkDirHasPresets(p_share_direct)) return fs::canonical(p_share_direct, ec).string();

    // Portable layout: Factsoft directly alongside executable
    fs::path p1 = fs::path(exeDir) / "Factsoft";
    if (checkDirHasPresets(p1)) return fs::canonical(p1, ec).string();

    // Development build tree layouts
    fs::path p2 = fs::path(exeDir) / ".." / ".." / "bin" / "x64" / "Factsoft";
    if (checkDirHasPresets(p2)) return fs::canonical(p2, ec).string();

    fs::path p3 = fs::path(exeDir) / ".." / ".." / ".." / "bin" / "x64" / "Factsoft";
    if (checkDirHasPresets(p3)) return fs::canonical(p3, ec).string();

    fs::path p4 = fs::path(exeDir) / ".." / "bin" / "x64" / "Factsoft";
    if (checkDirHasPresets(p4)) return fs::canonical(p4, ec).string();
  }

  // 2. XDG_DATA_DIRS search (e.g. /usr/share/fxsound/Factsoft, /usr/local/share/fxsound/Factsoft, etc.)
  const char* dataDirsEnv = std::getenv("XDG_DATA_DIRS");
  std::string dataDirs = (dataDirsEnv && *dataDirsEnv) ? dataDirsEnv : "/usr/local/share:/usr/share";
  std::stringstream ss(dataDirs);
  std::string item;
  while (std::getline(ss, item, ':')) {
    if (!item.empty()) {
      fs::path xdg_preset_dir = fs::path(item) / "fxsound" / "Factsoft";
      if (checkDirHasPresets(xdg_preset_dir)) {
        return fs::canonical(xdg_preset_dir, ec).string();
      }
    }
  }

  // 3. Fallbacks on current working directory
  fs::path cwd1 = fs::current_path(ec) / "Factsoft";
  if (checkDirHasPresets(cwd1)) return fs::canonical(cwd1, ec).string();

  fs::path cwd2 = fs::current_path(ec) / "share" / "fxsound" / "Factsoft";
  if (checkDirHasPresets(cwd2)) return fs::canonical(cwd2, ec).string();

  fs::path cwd3 = fs::current_path(ec) / "bin" / "x64" / "Factsoft";
  if (checkDirHasPresets(cwd3)) return fs::canonical(cwd3, ec).string();

  // 4. Default fallback path
  if (!exeDir.empty()) {
    fs::path fallback = fs::path(exeDir) / ".." / "share" / "fxsound" / "Factsoft";
    return fallback.lexically_normal().string();
  }
  return "/usr/share/fxsound/Factsoft";
}

std::string userPresetDir() { return xdg("XDG_DATA_HOME", "/.local/share") + "/fxsound/presets/"; }
std::string autoSaveDir() { return xdg("XDG_DATA_HOME", "/.local/share") + "/fxsound/AutoSave/"; }
std::string statusJsonPath() { return xdg("XDG_CONFIG_HOME", "/.config") + "/FxSound/status.json"; }

}  // namespace fxsound_linux
