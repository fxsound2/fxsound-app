#include "../engine/recovery-channel.h"
#include <cassert>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
static bool rejects(int fd, const std::string& uid) {
    try { fxsound::notifyRecovery(fd, uid); return false; }
    catch (const std::runtime_error&) { return true; }
}
static std::string packet(uint32_t length, const std::string& text) {
    std::string result(4, '\0'); std::memcpy(result.data(), &length, 4); return result + text;
}
int main() {
    std::signal(SIGPIPE, SIG_IGN);
    fxsound::RecoveryChannel channel; std::string uid = "initial";
    int type = 0; socklen_t typeSize = sizeof(type);
    assert(getsockopt(channel.childFD(), SOL_SOCKET, SO_TYPE, &type, &typeSize) == 0 && type == SOCK_DGRAM);
    assert(fcntl(channel.childFD(), F_GETFL) & O_NONBLOCK);
    auto start = std::chrono::steady_clock::now(); channel.latest(uid);
    assert(uid == "initial" && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(100));
    fxsound::notifyRecovery(channel.childFD(), "speakers");
    fxsound::notifyRecovery(channel.childFD(), "latest headphones");
    channel.latest(uid); assert(uid == "latest headphones");
    assert(rejects(channel.childFD(), "") && rejects(channel.childFD(), std::string(513, 'x')));
    const std::string maximum(512, 'm'); fxsound::notifyRecovery(channel.childFD(), maximum);
    channel.latest(uid); assert(uid == maximum);
    bool full = false;
    for (int i = 0; i < 100000 && !full; ++i) full = rejects(channel.childFD(), maximum);
    assert(full); channel.latest(uid); assert(uid == maximum);
    fxsound::notifyRecovery(channel.childFD(), "after draining"); channel.latest(uid);
    assert(uid == "after draining");
    channel.closeWriter(); channel.closeWriter();
    channel.latest(uid);
    assert(uid == "after draining");
    for (const auto& invalid : {std::string(), std::string("x"), std::string("abc"), packet(0, ""),
                                packet(513, std::string(513, 'x')), packet(5, "xy"),
                                packet(1, "xy"), packet(512, std::string(1024, 'x'))}) {
        fxsound::RecoveryChannel bad; uid = "unchanged";
        assert(send(bad.childFD(), invalid.data(), invalid.size(), 0) == ssize_t(invalid.size()));
        bool rejected = false;
        try { bad.latest(uid); } catch (const std::runtime_error&) { rejected = true; }
        assert(rejected && uid == "unchanged");
        fxsound::notifyRecovery(bad.childFD(), "valid after rejection");
        bad.latest(uid); assert(uid == "valid after rejection");
    }
    int fds[2]; assert(socketpair(AF_UNIX, SOCK_DGRAM, 0, fds) == 0); close(fds[0]);
    assert(rejects(fds[1], "no receiver")); close(fds[1]);
    fxsound::notifyRecovery(-1, "optional disconnected sink");
    std::cout << "recovery datagrams: latest/repeated/512 cap/atomic full refusal+drain/malformed/closed peer passed\n";
}
