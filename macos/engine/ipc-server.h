#pragma once
#include <functional>
#include <string>

namespace fxsound {
inline constexpr int ipcVersion = 1;
class IPCServer {
public:
    using Handler = std::function<std::string(const std::string&)>;
    explicit IPCServer(const std::string& path);
    ~IPCServer();
    IPCServer(const IPCServer&) = delete;
    IPCServer& operator=(const IPCServer&) = delete;
    void poll(const Handler& handler);
    static std::string request(const std::string& path, const std::string& json);
private:
    int socket_ = -1;
    int lock_ = -1;
    bool bound_ = false;
    std::string path_;
};
std::string defaultSocketPath();
}
