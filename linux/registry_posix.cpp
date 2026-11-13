/*
FxSound Linux port — linux/registry_posix.cpp (Phase 5)
INI-ish key=value file, no Win32. Logs defaults for startup-only keys.
Untested.
*/
#include "registry_posix.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <unistd.h>

namespace fs = std::filesystem;
namespace fxsound_linux {
namespace {

std::mutex g_regMutex;
std::map<std::wstring, std::wstring> g_cache;
bool g_loaded = false;

void ensureLoadedLocked() {
  if (g_loaded) return;
  g_loaded = true;
  std::ifstream f(registryPath());
  if (!f.is_open()) return;

  std::string line;
  auto w = [](const std::string& s) { return fs::u8path(s).wstring(); };
  while (std::getline(f, line)) {
    if (line.empty() || line[0] == '#') continue;
    auto p = line.find('=');
    if (p == std::string::npos) continue;
    g_cache[w(line.substr(0, p))] = w(line.substr(p + 1));
  }
}

void saveLocked() {
  std::error_code ec;
  fs::create_directories(fs::path(registryPath()).parent_path(), ec);
  std::string tmpPath = registryPath() + ".tmp." + std::to_string(getpid());
  {
    std::ofstream f(tmpPath, std::ios::trunc);
    if (!f.is_open()) return;
    auto u = [](const std::wstring& ws) { return fs::path(ws).u8string(); };
    for (auto& [k, v] : g_cache) f << u(k) << "=" << u(v) << "\n";
  }
  fs::rename(tmpPath, registryPath(), ec);
}

}  // namespace

std::string configDir() {
  const char* xdg = std::getenv("XDG_CONFIG_HOME");
  std::string base = (xdg && *xdg) ? xdg : std::string(std::getenv("HOME") ? std::getenv("HOME") : "/tmp") + "/.config";
  return base + "/FxSound";
}
std::string registryPath() { return configDir() + "/linux_registry.ini"; }

bool regReadString(const std::wstring& subkey, const std::wstring& value, std::wstring& out) {
  std::lock_guard<std::mutex> lock(g_regMutex);
  ensureLoadedLocked();
  auto it = g_cache.find(subkey + L"|" + value);
  if (it == g_cache.end()) { return false; }
  out = it->second;
  return true;
}

bool regWriteString(const std::wstring& subkey, const std::wstring& value, const std::wstring& data) {
  std::lock_guard<std::mutex> lock(g_regMutex);
  ensureLoadedLocked();
  std::wstring key = subkey + L"|" + value;
  auto it = g_cache.find(key);
  if (it != g_cache.end() && it->second == data) {
    return true;  // No change
  }
  g_cache[key] = data;
  saveLocked();
  return true;
}

bool regReadDword(const std::wstring& subkey, const std::wstring& value, uint32_t& out) {
  std::wstring s; if (!regReadString(subkey, value, s)) return false;
  try { out = (uint32_t)std::stoul(s); return true; } catch (...) { return false; }
}
bool regWriteDword(const std::wstring& subkey, const std::wstring& value, uint32_t data) {
  return regWriteString(subkey, value, std::to_wstring(data));
}

std::wstring readDfxGuid() {
  std::wstring v; if (regReadString(L"devices", L"dfx_guid", v) && !v.empty()) return v;
  return L"fxsound_audio_enhancer";
}
uint32_t readDefaultBufferSizeMs() {
  uint32_t v = 0; if (regReadDword(L"devices", L"default_buffer_size", v) && v) return v;
  std::cerr << "[linux-reg] default_buffer_size default 60ms\n"; return 60;
}
uint32_t readUserBufferSizeMs(uint32_t fallback) {
  uint32_t v = 0; if (regReadDword(L"devices", L"user_buffer_size", v) && v) return v; return fallback;
}
void writeUserBufferSizeMs(uint32_t ms) { regWriteDword(L"devices", L"user_buffer_size", ms); }
std::wstring readUserSelectedPlayback() {
  std::wstring v; regReadString(L"devices", L"user_selected_playback", v); return v;
}
void writeUserSelectedPlayback(const std::wstring& n) { regWriteString(L"devices", L"user_selected_playback", n); }
}  // namespace fxsound_linux
