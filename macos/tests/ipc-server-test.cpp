#include "../engine/ipc-server.h"
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
using fxsound::IPCServer;
static int connectClient(const std::string& path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0); assert(fd >= 0);
    sockaddr_un a{}; a.sun_family = AF_UNIX;
    std::memcpy(a.sun_path, path.c_str(), path.size() + 1);
    assert(connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof(a)) == 0);
    timeval timeout{3, 0};
    assert(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0);
    int one = 1; assert(setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one)) == 0);
    return fd;
}
static std::string raw(IPCServer& server, const std::string& path, const std::string& text,
                       int& handled) {
    const int fd = connectClient(path);
    std::thread sender([&] {
        size_t sent = 0;
        while (sent < text.size()) {
            auto count = send(fd, text.data() + sent, text.size() - sent, 0);
            assert(count > 0); sent += size_t(count);
        }
    });
    server.poll([&](const std::string&) { ++handled; return "{}"; });
    char reply[1024]; auto count = recv(fd, reply, sizeof(reply), 0); assert(count > 0);
    sender.join(); close(fd); return std::string(reply, size_t(count));
}
int main() {
    char directory[] = "/private/tmp/fxsound-ipc-test-XXXXXX";
    assert(mkdtemp(directory)); const std::string path = std::string(directory) + "/engine.sock";
    {
        IPCServer server(path); struct stat status{};
        assert(stat(path.c_str(), &status) == 0 && S_ISSOCK(status.st_mode));
        assert((status.st_mode & 0777) == 0600 && status.st_uid == getuid());
        bool duplicate = false;
        try { IPCServer second(path); } catch (const std::runtime_error&) { duplicate = true; }
        assert(duplicate && stat(path.c_str(), &status) == 0);
        std::string reply; std::exception_ptr error;
        std::thread client([&] {
            try { reply = IPCServer::request(path, "{\"version\":1}"); }
            catch (...) { error = std::current_exception(); }
        });
        bool handled = false;
        for (int i = 0; i < 100 && !handled; ++i)
            server.poll([&](const std::string& request) {
                assert(request == "{\"version\":1}"); handled = true; return "{\"ok\":true}";
            });
        client.join(); if (error) std::rethrow_exception(error);
        assert(handled && reply == "{\"ok\":true}");
        int calls = 0;
        assert(raw(server, path, "{}\n{}\n", calls).find("\"ok\":false") != std::string::npos);
        assert(raw(server, path, std::string(16385, 'a') + "\n", calls).find("\"ok\":false") != std::string::npos);
        const auto start = std::chrono::steady_clock::now();
        assert(raw(server, path, "", calls).find("\"ok\":false") != std::string::npos);
        const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        assert(elapsed >= 0.8 && elapsed < 3 && calls == 0);
        const int slow = connectClient(path);
        std::thread dribble([&] {
            for (int i = 0; i < 8; ++i) {
                if (send(slow, "x", 1, 0) != 1) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        });
        const auto deadlineStart = std::chrono::steady_clock::now();
        server.poll([&](const std::string&) { ++calls; return "{}"; });
        const double duration = std::chrono::duration<double>(std::chrono::steady_clock::now() - deadlineStart).count();
        assert(duration >= 0.8 && duration < 1.5 && calls == 0);
        dribble.join(); close(slow);
        bool overLimit = false;
        try { IPCServer::request(path, std::string(16385, 'x')); }
        catch (const std::runtime_error&) { overLimit = true; }
        assert(overLimit);
    }
    assert(access(path.c_str(), F_OK) != 0);
    {
        std::ofstream file(path); file << "preserve";
    }
    bool fileBlocked = false;
    try { IPCServer server(path); } catch (const std::runtime_error&) { fileBlocked = true; }
    assert(fileBlocked); std::ifstream file(path); std::string text; file >> text; assert(text == "preserve");
    file.close(); unlink(path.c_str()); rmdir(directory);
    bool longPath = false;
    try { IPCServer server(std::string(200, 'x')); } catch (const std::runtime_error&) { longPath = true; }
    assert(longPath);
    std::cout << "IPC: real AF_UNIX request, same-uid peer,0600, duplicate/file protection, framing/cap/timeout/cleanup passed\n";
}
