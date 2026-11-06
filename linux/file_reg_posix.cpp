/*
FxSound Linux port — linux/file_reg_posix.cpp (Phase 5)
Minimal POSIX reimplementation of file and reg APIs needed for DfxDsp offline
golden (dsp_cli preset .fac via Valsfile). Windows FileGeneral uses CreateFile
and Win32 registry uses RegCreateKeyExW - excluded on Linux (see CMake).
*/
#include "ptLong.h"   /* FxSound Linux port: fixed-width DSP pt_long (see ptLong.h) */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <filesystem>
#include <iostream>

#include "File.h"
#include "reg.h"
#include "slout.h"

namespace fs = std::filesystem;

namespace {
std::string toUtf8(const wchar_t* ws) {
  if (!ws) return {};
  char buf[4096] = {0};
  if (wcstombs(buf, ws, sizeof(buf) - 1) == (size_t)-1) return {};
  return buf;
}
}  // namespace

// ---- file* (POSIX, UTF-8 paths) ----
FILE* fileOpen_Wide(wchar_t* wcp_path, wchar_t* wcp_mode, CSlout* /*hp_slout*/) {
  std::string p = toUtf8(wcp_path);
  std::string m = toUtf8(wcp_mode);
  if (p.empty() || m.empty()) return nullptr;
  return fopen(p.c_str(), m.c_str());
}

int fileExist_Wide(wchar_t* wcp_name, int* ip_exist) {
  if (!wcp_name || !ip_exist) return 1;  // NOT_OKAY
  std::error_code ec;
  *ip_exist = fs::exists(toUtf8(wcp_name), ec) ? 1 : 0;
  return 0;  // OKAY
}

int fileCreateDirectoryAndParents_Wide(wchar_t* wcp_path, int* ip_success, CSlout* /*hp_slout*/) {
  if (!wcp_path || !ip_success) return 1;
  std::error_code ec;
  fs::create_directories(toUtf8(wcp_path), ec);
  *ip_success = ec ? 0 : 1;
  return 0;
}

int fileSplitFullpath_Wide(wchar_t* wcp_full, wchar_t* wcp_dir, wchar_t* wcp_file, CSlout* /*s*/) {
  if (!wcp_full) return 1;
  fs::path p(toUtf8(wcp_full));
  std::string dir = p.parent_path().string();
  std::string file = p.filename().string();
  if (wcp_dir) {
    char tmp[4096] = {0};
    strncpy(tmp, dir.c_str(), sizeof(tmp) - 1);
    mbstowcs(wcp_dir, tmp, 1023);
  }
  if (wcp_file) {
    char tmp[4096] = {0};
    strncpy(tmp, file.c_str(), sizeof(tmp) - 1);
    mbstowcs(wcp_file, tmp, 1023);
  }
  return 0;
}

int fileGetModifiedDate_Wide(wchar_t* wcp_path, FILETIME* ft, CSlout* /*s*/) {
  if (!wcp_path || !ft) return 1;
  std::error_code ec;
  auto t = fs::last_write_time(toUtf8(wcp_path), ec);
  if (ec) return 1;
  auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
      t - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
  std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
  struct tm tmv = {};
  gmtime_r(&tt, &tmv);
  SYSTEMTIME st = {};
  st.wYear = (WORD)(tmv.tm_year + 1900); st.wMonth = (WORD)(tmv.tm_mon + 1);
  st.wDay = (WORD)tmv.tm_mday; st.wHour = (WORD)tmv.tm_hour;
  st.wMinute = (WORD)tmv.tm_min; st.wSecond = (WORD)tmv.tm_sec;
  SYSTEMTIME local = st;
  return SystemTimeToFileTime(&local, ft) ? 0 : 1;
}

// ---- reg* (in-memory + ini file via registry_posix where needed) ----
// DSP setEffectValue writes via reg* then processTimer reads back on first
// processAudio (update_from_registry_). Stubs must persist within process,
// otherwise effects never apply (param_effect.py FAIL). In-memory map covers
// dsp_cli offline golden; full persistence in Phase 8 via registry_posix.
#include <map>
#include <mutex>
namespace {
std::map<std::wstring, std::wstring> g_regStr;
std::mutex g_regMu;
std::wstring regKey(int root, const wchar_t* path, const wchar_t* key = nullptr) {
  std::wstring k = std::to_wstring(root) + L"|" + (path ? path : L"");
  if (key) { k += L"|"; k += key; }
  return k;
}
}  // namespace
int regCreateKey_Wide(int root, wchar_t* path, wchar_t* value) {
  if (!path) return 1;
  std::lock_guard<std::mutex> l(g_regMu);
  g_regStr[regKey(root, path)] = value ? value : L"";
  return 0;
}
int regReadKey_Wide(int root, wchar_t* path, int* key_exists, wchar_t* value, pt_ulong len) {
  std::lock_guard<std::mutex> l(g_regMu);
  auto it = g_regStr.find(regKey(root, path));
  if (it == g_regStr.end()) {
    if (key_exists) *key_exists = 0;
    if (value && len > 0) value[0] = 0;
    return 0;
  }
  if (key_exists) *key_exists = 1;
  if (value && len > 0) {
    wcsncpy(value, it->second.c_str(), (size_t)len - 1);
    value[len - 1] = 0;
  }
  return 0;
}
int regRemoveKey_Wide(int /*root*/, wchar_t* /*path*/) { return 0; }
int regReadTopDir_Wide(wchar_t* /*out*/, int /*a*/, int /*b*/, int /*c*/, CSlout* /*s*/) { return 1; }
int regReadRegisteredOwner_Wide(wchar_t* out, int len) {
  if (out && len > 0) {
    const wchar_t* d = L"LinuxOwner";
    wcsncpy(out, d, (size_t)len - 1);
    out[len - 1] = 0;
  }
  return 0;
}
int regCreateKeyTest_Wide(int /*root*/, wchar_t* /*p*/, wchar_t* /*v*/, int* exists) {
  if (exists) *exists = 0;
  return 0;
}
int regCreateKeyWithKeyname_Dword_Wide(int r, wchar_t* p, wchar_t* k, pt_ulong v) {
  if (!p || !k) return 1;
  std::lock_guard<std::mutex> l(g_regMu);
  g_regStr[regKey(r, p, k)] = std::to_wstring(v);
  return 0;
}
int regCreateKeyWithKeyname_String_Wide(int r, wchar_t* p, wchar_t* k, wchar_t* v) {
  if (!p || !k) return 1;
  std::lock_guard<std::mutex> l(g_regMu);
  g_regStr[regKey(r, p, k)] = v ? v : L"";
  return 0;
}
int regReadKeyWithKeyname_String_Wide(int r, wchar_t* p, wchar_t* k, int* exists, wchar_t* v, pt_ulong len) {
  std::lock_guard<std::mutex> l(g_regMu);
  auto it = g_regStr.find(regKey(r, p, k));
  if (it == g_regStr.end()) {
    if (exists) *exists = 0;
    if (v && len > 0) v[0] = 0;
    return 0;
  }
  if (exists) *exists = 1;
  if (v && len > 0) {
    wcsncpy(v, it->second.c_str(), (size_t)len - 1);
    v[len - 1] = 0;
  }
  return 0;
}
int regReadKeyWithKeyname_Dword_Wide(int r, wchar_t* p, wchar_t* k, int* exists, pt_ulong* v) {
  std::lock_guard<std::mutex> l(g_regMu);
  auto it = g_regStr.find(regKey(r, p, k));
  if (it == g_regStr.end()) {
    if (exists) *exists = 0;
    if (v) *v = 0;
    return 0;
  }
  if (exists) *exists = 1;
  if (v) {
    try { *v = (pt_ulong)std::stoul(it->second); } catch (...) { *v = 0; }
  }
  return 0;
}
int regRecursiveDeleteFolder_Wide(int /*r*/, wchar_t* /*p*/) { return 0; }
int regReadRegisteredOwner(char* out, int len) {
  if (out && len > 0) {
    const char* d = "LinuxOwner";
    strncpy(out, d, (size_t)len - 1);
    out[len - 1] = 0;
  }
  return 0;
}
int regRemoveKey(int /*r*/, char* /*p*/) { return 0; }
int regCreateKey(int /*r*/, char* /*p*/, char* /*v*/) { return 0; }
int regReadKey(int /*r*/, char* /*p*/, int* exists, char* v, pt_ulong len) {
  if (exists) *exists = 0;
  if (v && len > 0) v[0] = 0;
  return 0;
}
