/*
FxSound Linux port — linux/notify_posix.cpp (Phase 9). Untested.
Prefers org.freedesktop.Notifications via gdbus/dbus-send fallback; keeps
FxNotification custom popup as fallback when notifications hidden=false.
Respects hide_notifications setting (caller checks before calling notify()).
*/
#include "notify_posix.h"
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

namespace fxsound_linux {

void notify(const std::string& title, const std::string& body) {
  // Best-effort non-blocking notification dispatch without subshell overhead
  pid_t pid = fork();
  if (pid == 0) {
    // Child: double fork to detach from parent
    pid_t pid2 = fork();
    if (pid2 > 0) {
      _exit(0);
    } else if (pid2 == 0) {
      int devnull = open("/dev/null", O_WRONLY);
      if (devnull >= 0) {
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        close(devnull);
      }
      execlp("gdbus", "gdbus", "call", "--session",
             "--dest", "org.freedesktop.Notifications",
             "--object-path", "/org/freedesktop/Notifications",
             "--method", "org.freedesktop.Notifications.Notify",
             "FxSound", "0", "", title.c_str(), body.c_str(), "[]", "{}", "5000",
             nullptr);
      // Fallback to notify-send
      execlp("notify-send", "notify-send", "-a", "FxSound", title.c_str(), body.c_str(), nullptr);
      _exit(127);
    }
    _exit(1);
  } else if (pid > 0) {
    waitpid(pid, nullptr, 0);
  }
}
}  // namespace fxsound_linux
