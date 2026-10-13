#pragma once
#include <string>
#include <array>
#include <cstddef>

namespace fxsound {
void notifyRecovery(int fd, const std::string& uid);
class RecoveryChannel {
public:
    RecoveryChannel();
    ~RecoveryChannel();
    int childFD() const noexcept { return write_; }
    void closeWriter() noexcept;
    void latest(std::string& uid);
private:
    int read_ = -1, write_ = -1;
};
}
