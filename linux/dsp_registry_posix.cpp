/*
FxSound Linux port — linux/dsp_registry_posix.cpp (Phase 5)
Reuses registry_posix.ini backing. Untested.
*/
#include "dsp_registry_posix.h"
#include "registry_posix.h"
#include <iostream>

namespace fxsound_linux {
static const wchar_t* kSess = L"LASTUSED_DFXP";
bool dspSessionReadInt(const std::wstring& name, int& out) {
  uint32_t v = 0;
  if (!regReadDword(kSess, name, v)) return false;
  out = (int)v;
  return true;
}
void dspSessionWriteInt(const std::wstring& name, int v) {
  regWriteDword(kSess, name, (uint32_t)v);
}
bool dspSessionReadReal(const std::wstring& name, float& out) {
  std::wstring s;
  if (!regReadString(kSess, name, s)) return false;
  try { out = std::stof(s); return true; } catch (...) { return false; }
}
void dspSessionWriteReal(const std::wstring& name, float v) {
  regWriteString(kSess, name, std::to_wstring(v));
}
std::wstring dspDefaultSharedFolder() {
  std::cerr << "[dsp-reg] startup-only install path -> /usr/share/fxsound\n";
  return L"/usr/share/fxsound";
}
}  // namespace fxsound_linux
