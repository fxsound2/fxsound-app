#pragma once
#include "stereo-ring.h"
#include <algorithm>
#include <cmath>

namespace fxsound {
class AdaptiveResampler {
public:
    static constexpr uint64_t targetFrames = 512;
    void prepare(double outputRate) noexcept {
        nominal_ = 48000.0 / outputRate;
        phase_ = correction_ = 0;
        primed_ = false;
    }
    void update(uint64_t occupancy, uint64_t outputFrames = 0) noexcept {
        const double target = targetFrames + std::ceil(outputFrames * nominal_);
        const double goal = std::clamp((double(occupancy) - target) * 0.00001, -0.001, 0.001);
        const double slew = 0.000001 * std::max(1.0, outputFrames * nominal_ / 128.0);
        correction_ += std::clamp(goal - correction_, -slew, slew);
    }
    bool next(StereoRing& ring, StereoFrame& frame) noexcept {
        if (!primed_) {
            if (ring.available() < targetFrames) { frame = {}; return false; }
            primed_ = true;
        }
        const double advance = nominal_ * (1 + correction_);
        const uint64_t consumed = static_cast<uint64_t>(phase_ + advance);
        if (ring.available() < std::max<uint64_t>(2, consumed + 1)) {
            primed_ = false; phase_ = 0; frame = {}; return false;
        }
        const auto a = ring.peek(0), b = ring.peek(1);
        if (phase_ == 0) frame = a;
        else frame = {float(a.left + (b.left - a.left) * phase_), float(a.right + (b.right - a.right) * phase_)};
        phase_ += advance;
        ring.consume(consumed);
        phase_ -= consumed;
        return true;
    }
    double ppm() const noexcept { return correction_ * 1000000; }
private:
    double nominal_ = 1, phase_ = 0, correction_ = 0;
    bool primed_ = false;
};
}
