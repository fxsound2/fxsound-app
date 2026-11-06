/*
FxSound Linux port — linux/single_instance_posix.{h,cpp} (Phase 8)
JUCE 6.1.6 Linux MessageManager::broadcastMessage is a TODO stub, so its
single-instance CLI forwarding ("fxsound --status" from a second process)
never reaches the running instance. Replacement: a UNIX domain socket in
XDG_RUNTIME_DIR (per-user, 0700).

  forwardIfRunning(cmd)  — second process: deliver cmd to the primary, return
                           true when delivered (caller quits). No lock held
                           => no primary => false (we become the primary).
  startServer(cb)        — primary: accept loop on a worker thread; each
                           command line is marshaled to the JUCE message
                           thread by the caller-provided callback.
  stopServer()           — primary shutdown.

Windows keeps the JUCE/Win32 mechanism; this file is Linux-only.
*/
#include "single_instance_posix.h"
#if !defined(_WIN32)
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <atomic>
#include <csignal>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <chrono>
#include <future>
#include <thread>

namespace fxsound_linux {
namespace {

// Writing to a client that already hung up must not kill the process.
void ignoreSigpipe() { signal(SIGPIPE, SIG_IGN); }

std::string socketPath() {
  const char* xdg = std::getenv("XDG_RUNTIME_DIR");
  std::string base = (xdg && *xdg) ? std::string(xdg)
                                   : std::string("/tmp");
  return base + "/fxsound-cli.sock";
}

bool writeAll(int fd, const char* data, size_t len) {
  size_t off = 0;
  while (off < len) {
    ssize_t n = write(fd, data + off, len - off);
    if (n <= 0) {
      if (n < 0 && errno == EINTR) continue;
      return false;
    }
    off += (size_t)n;
  }
  return true;
}

std::atomic<bool> g_running{false};
std::thread g_acceptThread;
int g_listenFd = -1;
// onCommand(cmd, done): run cmd (JUCE thread) and invoke done() when finished.
std::function<void(const std::string&, std::function<void()>)> g_onCommand;

std::string statusJsonPath() {
  const char* xdg = std::getenv("XDG_CONFIG_HOME");
  std::string base = (xdg && *xdg) ? std::string(xdg)
                                   : std::string(std::getenv("HOME") ? std::getenv("HOME") : "/tmp") + "/.config";
  return base + "/FxSound/status.json";
}

void acceptLoop() {
  while (g_running.load()) {
    int fd = accept(g_listenFd, nullptr, nullptr);
    if (fd < 0) {
      if (errno == EINTR) continue;
      break;  // socket closed on shutdown
    }
    std::string cmd;
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) cmd.append(buf, (size_t)n);

    // NOTE: an empty command line is meaningful ("bare relaunch" = show the
    // window); it still flows to onCommand. Only --status gets a response.

    if (cmd.find("--status") != std::string::npos) {
      // Request/response: run to completion, then return status.json content so
      // the caller's terminal prints it (the primary has no controlling tty).
      std::promise<void> done;
      auto doneFut = done.get_future();
      g_onCommand(cmd, [&done] { done.set_value(); });
      if (doneFut.wait_for(std::chrono::seconds(10)) == std::future_status::ready) {
        FILE* f = fopen(statusJsonPath().c_str(), "rb");
        if (f) {
          char jbuf[16384];
          size_t jn;
          while ((jn = fread(jbuf, 1, sizeof(jbuf), f)) > 0) writeAll(fd, jbuf, jn);
          fclose(f);
        }
      }
      shutdown(fd, SHUT_WR);
      close(fd);
    } else {
      close(fd);
      if (g_onCommand) g_onCommand(cmd, [] {});
    }
  }
}

}  // namespace

bool forwardIfRunning(const std::string& commandLine) {
  ignoreSigpipe();

  std::string path = socketPath();
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) return false;

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (connect(fd, (const sockaddr*)&addr, sizeof(addr)) != 0) {
    close(fd);
    // stale socket from a crashed primary -> remove so a new primary can bind
    unlink(path.c_str());
    return false;
  }

  bool ok = writeAll(fd, commandLine.data(), commandLine.size());
  if (ok && commandLine.find("--status") != std::string::npos) {
    // Request/response: primary sends status.json content, we print it to the
    // caller's stdout (the primary has no controlling terminal).
    shutdown(fd, SHUT_WR);
    char buf[16384];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) fwrite(buf, 1, (size_t)n, stdout);
    std::fflush(stdout);
  }
  close(fd);
  return ok;
}

void startServer(std::function<void(const std::string&, std::function<void()>)> onCommand) {
  if (g_running.load()) return;
  ignoreSigpipe();

  std::string path = socketPath();
  unlink(path.c_str());

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) return;

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (bind(fd, (const sockaddr*)&addr, sizeof(addr)) != 0 || listen(fd, 4) != 0) {
    close(fd);
    return;
  }

  g_listenFd = fd;
  g_onCommand = std::move(onCommand);
  g_running.store(true);
  g_acceptThread = std::thread(acceptLoop);
}

void stopServer() {
  if (!g_running.exchange(false)) return;
  shutdown(g_listenFd, SHUT_RDWR);
  close(g_listenFd);
  g_listenFd = -1;
  if (g_acceptThread.joinable()) g_acceptThread.join();
  g_onCommand = nullptr;
  unlink(socketPath().c_str());
}

}  // namespace fxsound_linux
#else
#include "single_instance_posix.h"
namespace fxsound_linux {
bool forwardIfRunning(const std::string&) { return false; }
void startServer(std::function<void(const std::string&, std::function<void()>)>) {}
void stopServer() {}
}  // namespace fxsound_linux
#endif
