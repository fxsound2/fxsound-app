/*
FxSound Linux port — linux/crash_posix.cpp (Phase 8)
POSIX replacement for Main.cpp MiniDump/StackWalk64 crash capture.
SIGSEGV/SIGABRT/SIGFPE/SIGILL/SIGBUS handlers append a backtrace to
~/.config/FxSound/crash.log (same location class as the Windows fxsound.dmp).
JUCE-free so it can be linked from any target. Windows never compiles this.
*/
#include "crash_posix.h"
#if !defined(_WIN32)
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

namespace fxsound_linux {
namespace {

static char s_crash_log_path[512] = {0};

void initCrashLogPath() {
  if (s_crash_log_path[0] != '\0') return;
  const char* xdg = std::getenv("XDG_CONFIG_HOME");
  std::string base = (xdg && *xdg) ? std::string(xdg)
                                   : std::string(std::getenv("HOME") ? std::getenv("HOME") : "/tmp") + "/.config";
  std::string dir = base + "/FxSound";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  std::string path = dir + "/crash.log";
  std::strncpy(s_crash_log_path, path.c_str(), sizeof(s_crash_log_path) - 1);
}

// Minimal async-signal-safe integer to string writer
void safeWriteInt(int fd, int val) {
  char buf[32];
  int i = 0;
  if (val == 0) {
    write(fd, "0", 1);
    return;
  }
  if (val < 0) {
    write(fd, "-", 1);
    val = -val;
  }
  while (val > 0 && i < (int)sizeof(buf)) {
    buf[i++] = '0' + (val % 10);
    val /= 10;
  }
  for (int j = 0; j < i / 2; ++j) {
    char t = buf[j];
    buf[j] = buf[i - 1 - j];
    buf[i - 1 - j] = t;
  }
  write(fd, buf, i);
}

void writeCrashHeader(int fd, const char* prefix, int sig) {
  write(fd, prefix, std::strlen(prefix));
  if (sig >= 0) {
    write(fd, "signal ", 7);
    safeWriteInt(fd, sig);
    write(fd, " (pid ", 6);
    safeWriteInt(fd, static_cast<int>(getpid()));
    write(fd, ")", 1);
  }
  write(fd, " ---\n", 5);
}

void crashHandler(int sig) {
  // Strictly async-signal-safe: open/write/close only, no malloc/free, no stdio mutexes
  void* bt[64];
  int n = backtrace(bt, 64);
  if (s_crash_log_path[0] != '\0') {
    int fd = open(s_crash_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
      writeCrashHeader(fd, "--- crash: ", sig);
      backtrace_symbols_fd(bt, n, fd);
      close(fd);
    }
  }
  // Also dump to stderr for systemd journal capture
  writeCrashHeader(STDERR_FILENO, "--- crash: ", sig);
  backtrace_symbols_fd(bt, n, STDERR_FILENO);

  signal(sig, SIG_DFL);
  raise(sig);
}

}  // namespace

void installCrashHandler() {
  initCrashLogPath();
  for (int s : {SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS}) signal(s, crashHandler);
}

namespace {
volatile sig_atomic_t g_quit_requested = 0;
void quitSignalHandler(int) { g_quit_requested = 1; }
}  // namespace

void installQuitFlagSignals() {
  // Catch SIGTERM, SIGHUP, and SIGINT so audio state (default sink), the CLI
  // socket, and user settings are all cleanly restored/saved on exit.
  for (int s : {SIGTERM, SIGHUP, SIGINT}) {
    struct sigaction sa {};
    sa.sa_handler = quitSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(s, &sa, nullptr);
  }
}

bool quitFlagConsumed() {
  if (g_quit_requested) {
    g_quit_requested = 0;
    return true;
  }
  return false;
}

void logBacktrace() {
  initCrashLogPath();
  void* bt[64];
  int n = backtrace(bt, 64);
  if (s_crash_log_path[0] != '\0') {
    int fd = open(s_crash_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
      writeCrashHeader(fd, "--- stack trace ", -1);
      backtrace_symbols_fd(bt, n, fd);
      close(fd);
    }
  }
}

bool writeStatusStdout(const char* json) {
  if (json) std::puts(json);
  return true;
}
}  // namespace fxsound_linux
#endif
