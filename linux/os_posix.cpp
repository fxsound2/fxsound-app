/*
FxSound Linux port — linux/os_posix.cpp (Phase 5)
Untested.
*/
#include "os_posix.h"
#include <iostream>

namespace fxsound_linux {
void logOsStubHit(const char* fn) {
  std::cerr << "[os-posix] stub hit: " << fn << "\n";
}
bool isRemoteSession() { return false; }
std::string cpuArch() {
#if defined(__x86_64__)
  return "x64";
#elif defined(__aarch64__)
  return "arm64";
#else
  return "unknown";
#endif
}
bool isServiceRunning(const char* name) {
  logOsStubHit(name ? name : "service");
  return false;
}
}  // namespace fxsound_linux
