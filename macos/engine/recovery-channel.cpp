#include "recovery-channel.h"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>

namespace fxsound {
void notifyRecovery(int fd, const std::string& uid) {
    if (fd < 0) return;
    if (uid.empty() || uid.size() > 512) throw std::runtime_error("invalid recovery device UID");
    std::array<char, 516> frame{}; uint32_t length = static_cast<uint32_t>(uid.size());
    std::memcpy(frame.data(), &length, 4); std::memcpy(frame.data() + 4, uid.data(), length);
    ssize_t count;
    do { count = send(fd, frame.data(), length + 4, 0); } while (count < 0 && errno == EINTR);
    if (count != static_cast<ssize_t>(length + 4)) throw std::runtime_error("supervisor recovery channel unavailable");
}
RecoveryChannel::RecoveryChannel() {
    int fds[2]; if (socketpair(AF_UNIX, SOCK_DGRAM, 0, fds)) throw std::runtime_error("could not create recovery channel");
    read_ = fds[0]; write_ = fds[1];
    if (fcntl(read_, F_SETFD, FD_CLOEXEC) ||
        fcntl(read_, F_SETFL, fcntl(read_, F_GETFL) | O_NONBLOCK) ||
        fcntl(write_, F_SETFL, fcntl(write_, F_GETFL) | O_NONBLOCK)) {
        close(read_); close(write_); throw std::runtime_error("could not configure recovery channel");
    }
}
RecoveryChannel::~RecoveryChannel() { if (read_ >= 0) close(read_); closeWriter(); }
void RecoveryChannel::closeWriter() noexcept { if (write_ >= 0) { close(write_); write_ = -1; } }
void RecoveryChannel::latest(std::string& uid) {
    for (;;) {
        std::array<char, 517> frame{};
        ssize_t count = recv(read_, frame.data(), frame.size(), 0);
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ECONNRESET || errno == ENOTCONN) return;
            throw std::runtime_error("supervisor recovery channel read failed");
        }
        if (count < 4) throw std::runtime_error("invalid recovery frame");
        uint32_t length; std::memcpy(&length, frame.data(), 4);
        if (!length || length > 512 || count != static_cast<ssize_t>(length + 4))
            throw std::runtime_error("invalid recovery frame");
        uid.assign(frame.data() + 4, length);
    }
}
}
