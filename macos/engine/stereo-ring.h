#pragma once
#include <array>
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace fxsound {
struct StereoFrame { float left = 0; float right = 0; };
class StereoRing {
public:
    static constexpr uint64_t capacity = 8192;
    bool push(const StereoFrame* frames, size_t count) noexcept {
        const auto write = write_.load(std::memory_order_relaxed);
        const auto read = read_.load(std::memory_order_acquire);
        if (count > capacity || write - read > capacity - count) return false;
        for (size_t i = 0; i < count; ++i) frames_[(write + i) & (capacity - 1)] = frames[i];
        write_.store(write + count, std::memory_order_release);
        return true;
    }
    uint64_t available() const noexcept {
        const auto read = read_.load(std::memory_order_acquire);
        const auto write = write_.load(std::memory_order_acquire);
        return write >= read ? std::min<uint64_t>(write - read, capacity) : 0;
    }
    StereoFrame peek(uint64_t offset) const noexcept {
        return frames_[(read_.load(std::memory_order_relaxed) + offset) & (capacity - 1)];
    }
    void consume(uint64_t count) noexcept {
        read_.store(read_.load(std::memory_order_relaxed) + count, std::memory_order_release);
    }
    void reset() noexcept {
        read_.store(0, std::memory_order_relaxed);
        write_.store(0, std::memory_order_relaxed);
    }
private:
    static_assert(std::atomic<uint64_t>::is_always_lock_free);
    std::array<StereoFrame, capacity> frames_{};
    alignas(64) std::atomic<uint64_t> write_{0};
    alignas(64) std::atomic<uint64_t> read_{0};
};
}
