// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "../WaterfallFrame.h"
#include <mutex>
#include <vector>

namespace fxanalysis
{
// Worker/UI only. Never called by the audio callback. Bounded storage and short copies.
class WaterfallInbox final
{
public:
    WaterfallInbox() : frames_(WaterfallSettings::queueCapacity) {}
    void setEnabled(bool enabled)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        enabled_ = enabled; read_ = count_ = 0; dropped_ = 0;
    }
    void publish(const WaterfallFrame& frame)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!enabled_) return;
        if (count_ == WaterfallSettings::queueCapacity)
        {
            read_ = (read_ + 1) % WaterfallSettings::queueCapacity;
            --count_; ++dropped_;
            frames_[read_].discontinuity = true;
        }
        frames_[(read_ + count_) % WaterfallSettings::queueCapacity] = frame;
        ++count_;
    }
    std::uint64_t drain(std::vector<WaterfallFrame>& target)
    {
        // Reserve outside the lock so allocation cannot delay the producer.
        target.clear(); target.reserve(WaterfallSettings::queueCapacity);
        std::lock_guard<std::mutex> lock(mutex_);
        for (int i = 0; i < count_; ++i) target.push_back(frames_[(read_ + i) % WaterfallSettings::queueCapacity]);
        read_ = count_ = 0;
        return dropped_;
    }
private:
    std::mutex mutex_;
    std::vector<WaterfallFrame> frames_;
    int read_ = 0, count_ = 0;
    bool enabled_ = false;
    std::uint64_t dropped_ = 0;
};
}
