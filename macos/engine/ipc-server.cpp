#include "ipc-server.h"
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

namespace fxsound {
static constexpr size_t maxMessage = 16384;
static void fail(const char* operation) { throw std::runtime_error(std::string(operation) + ": " + std::strerror(errno)); }
static sockaddr_un endpoint(const std::string& path) {
    sockaddr_un result{}; result.sun_family = AF_UNIX;
    if (path.size() >= sizeof(result.sun_path)) throw std::runtime_error("IPC path is too long");
    std::memcpy(result.sun_path, path.c_str(), path.size() + 1); return result;
}
static void timeout(int fd) {
    timeval t{1, 0}; int one = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t)) ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(t)) ||
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one))) fail("IPC socket options");
}
static std::string readLine(int fd) {
    std::string result; char buffer[1024];
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    for (;;) {
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
        if (left <= 0) throw std::runtime_error("IPC message deadline exceeded");
        pollfd input{fd, POLLIN, 0};
        int ready = ::poll(&input, 1, static_cast<int>(left));
        if (ready < 0 && errno == EINTR) continue;
        if (ready <= 0) throw std::runtime_error("IPC message deadline exceeded");
        auto count = recv(fd, buffer, sizeof(buffer), 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) throw std::runtime_error("IPC closed or timed out before newline");
        for (ssize_t i = 0; i < count; ++i) {
            if (buffer[i] == '\n') {
                if (i != count - 1) throw std::runtime_error("only one IPC request per connection");
                return result;
            }
            result += buffer[i];
            if (result.size() > maxMessage) throw std::runtime_error("IPC request too large");
        }
    }
}
static void writeLine(int fd, const std::string& text) {
    if (text.size() > maxMessage) throw std::runtime_error("IPC response too large");
    std::string message = text + '\n'; size_t sent = 0;
    while (sent < message.size()) {
        auto count = send(fd, message.data() + sent, message.size() - sent, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) fail("IPC send");
        sent += static_cast<size_t>(count);
    }
}
std::string defaultSocketPath() {
    const char* runtime = std::getenv("TMPDIR");
    if (!runtime || !*runtime) throw std::runtime_error("TMPDIR must identify a private user runtime directory");
    struct stat status{};
    if (stat(runtime, &status) || !S_ISDIR(status.st_mode) || status.st_uid != getuid() || (status.st_mode & 0022))
        throw std::runtime_error("TMPDIR must be user-owned and not writable by other users");
    return std::string(runtime) + (runtime[std::strlen(runtime) - 1] == '/' ? "" : "/") + "fxsound-engine.sock";
}
IPCServer::IPCServer(const std::string& path) : path_(path) {
    auto a = endpoint(path);
    socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_ < 0) fail("IPC socket");
    try {
        if (fcntl(socket_, F_SETFD, FD_CLOEXEC)) fail("IPC close-on-exec");
        lock_ = open((path + ".lock").c_str(), O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (lock_ < 0) fail("IPC ownership lock");
        struct stat lockStatus{};
        if (fstat(lock_, &lockStatus) || !S_ISREG(lockStatus.st_mode) || lockStatus.st_uid != getuid() ||
            (lockStatus.st_mode & 0077)) throw std::runtime_error("unsafe IPC ownership lock");
        if (flock(lock_, LOCK_EX | LOCK_NB)) throw std::runtime_error("engine socket already owned");
        struct stat existing{};
        if (lstat(path.c_str(), &existing) == 0) {
            if (!S_ISSOCK(existing.st_mode) || existing.st_uid != getuid() || (existing.st_mode & 0077))
                throw std::runtime_error("unsafe existing IPC path");
            int probe = socket(AF_UNIX, SOCK_STREAM, 0); if (probe < 0) fail("IPC ownership probe");
            int connected = connect(probe, reinterpret_cast<sockaddr*>(&a), sizeof(a));
            int probeError = errno; close(probe);
            if (connected == 0 || probeError != ECONNREFUSED) throw std::runtime_error("IPC socket is active or cannot be checked");
            if (unlink(path.c_str())) fail("stale IPC socket cleanup");
        } else if (errno != ENOENT) fail("IPC path inspection");
        mode_t oldMask = umask(0077);
        int status = bind(socket_, reinterpret_cast<sockaddr*>(&a), sizeof(a));
        int bindError = errno; umask(oldMask); errno = bindError;
        if (status) fail("IPC bind");
        bound_ = true;
        if (chmod(path.c_str(), 0600) || listen(socket_, 4)) fail("IPC listen");
    } catch (...) {
        if (bound_) unlink(path_.c_str());
        if (lock_ >= 0) close(lock_);
        close(socket_); socket_ = -1; throw;
    }
}
IPCServer::~IPCServer() {
    if (socket_ >= 0) close(socket_);
    if (bound_) unlink(path_.c_str());
    if (lock_ >= 0) close(lock_);
}
void IPCServer::poll(const Handler& handler) {
    pollfd p{socket_, POLLIN, 0};
    int status = ::poll(&p, 1, 50);
    if (status < 0 && errno != EINTR) fail("IPC poll");
    if (status <= 0 || !(p.revents & POLLIN)) return;
    int client = accept(socket_, nullptr, nullptr);
    if (client < 0) { if (errno == EINTR) return; fail("IPC accept"); }
    try {
        uid_t uid; gid_t gid;
        if (getpeereid(client, &uid, &gid) || uid != getuid()) throw std::runtime_error("IPC peer rejected");
        timeout(client);
        auto message = readLine(client);
        writeLine(client, handler(message));
    } catch (const std::exception& error) {
        try { writeLine(client, "{\"version\":1,\"ok\":false,\"error\":\"invalid request or unavailable engine\"}"); } catch (...) {}
    }
    close(client);
}
std::string IPCServer::request(const std::string& path, const std::string& json) {
    auto a = endpoint(path);
    int fd = socket(AF_UNIX, SOCK_STREAM, 0); if (fd < 0) fail("IPC client socket");
    try {
        timeout(fd);
        if (connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof(a))) fail("IPC connect");
        uid_t uid; gid_t gid;
        if (getpeereid(fd, &uid, &gid) || uid != getuid()) throw std::runtime_error("IPC server owner rejected");
        writeLine(fd, json); auto result = readLine(fd); close(fd); return result;
    } catch (...) { close(fd); throw; }
}
}
