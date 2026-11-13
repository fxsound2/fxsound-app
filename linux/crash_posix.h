/*
FxSound Linux port — linux/crash_posix.h (Phase 8)
Replaces Main.cpp MiniDump/StackWalk64/CoInitializeEx/AttachConsole/NvOptimus on Linux.
Linux: SystemStats::setApplicationCrashHandler + backtrace to log. --status to stdout.
Untested.
*/
#pragma once

namespace fxsound_linux {
void installCrashHandler();  // SIGSEGV/SIGABRT/... -> backtrace in ~/.config/FxSound/crash.log
void installQuitFlagSignals();  // SIGTERM/SIGHUP/SIGINT -> async-safe flag; app polls + quits cleanly
bool quitFlagConsumed();        // true once a quit signal arrived (one-shot)
void logBacktrace();         // non-fatal stack dump (exception catch paths)
bool writeStatusStdout(const char* json);  // --status prints to stdout (not %APPDATA% file)
}  // namespace fxsound_linux
