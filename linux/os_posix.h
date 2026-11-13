/*
FxSound Linux port — linux/os_posix.h (Phase 5)
Sibling/stub for operatingSystem*.cpp (GetModuleHandle kernel32,
IsWow64Process, GetDiskFreeSpaceEx, GlobalMemoryStatusEx, ShellExecuteEx,
WMP Active Setup check, system sounds).
Linux: stubs or /proc, logging when hit. No Win32.
Untested.
*/
#pragma once
#include <string>

namespace fxsound_linux {
bool isRemoteSession();  // always false on Linux (SysInfo)
std::string cpuArch();   // from uname / SystemStats later
bool isServiceRunning(const char* name);  // logs + false (no SCM)
void logOsStubHit(const char* fn);
}  // namespace fxsound_linux
