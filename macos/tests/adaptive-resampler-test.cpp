#include "../engine/adaptive-resampler.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace fxsound;
static void ramp(double rate) {
    StereoRing ring; AdaptiveResampler src; src.prepare(rate);
    std::vector<StereoFrame> data(4096);
    for (size_t i = 0; i < data.size(); ++i) data[i] = {float(i), -float(i)};
    assert(ring.push(data.data(), data.size()));
    for (int i = 0; i < 1000; ++i) {
        StereoFrame frame;
        assert(src.next(ring, frame));
        const double expected = i * 48000.0 / rate;
        assert(std::abs(frame.left - expected) < 0.0002);
        assert(frame.right == -frame.left);
        if (rate == 48000) assert(frame.left == float(i));
    }
    const auto used = data.size() - ring.available();
    assert(used == uint64_t(std::floor(1000 * 48000.0 / rate)));
}
static void skew(double ppm, int block = 128, bool reserve = false) {
    StereoRing ring; AdaptiveResampler src; src.prepare(48000);
    std::vector<StereoFrame> frames(8192, {0.25f, -0.25f});
    assert(ring.push(frames.data(), 512));
    double production = 0; uint64_t low = ring.capacity, high = 0;
    size_t underruns = 0, overflows = 0;
    const int blocks = 19200000 / block;
    for (int b = 0; b < blocks; ++b) {
        production += block * (1 + ppm / 1000000);
        const auto count = size_t(production); production -= count;
        if (!ring.push(frames.data(), count)) ++overflows;
        src.update(ring.available(), reserve ? block : 0);
        for (int i = 0; i < block; ++i) {
            StereoFrame frame;
            if (!src.next(ring, frame)) ++underruns;
            else assert(frame.left == 0.25f && frame.right == -0.25f);
        }
        if (b > blocks / 2) { low = std::min(low, ring.available()); high = std::max(high, ring.available()); }
        assert(std::abs(src.ppm()) <= 1000.00001);
    }
    std::cout << "diagnostic skew=" << ppm << " underruns=" << underruns << " overflows=" << overflows << " occupancy=" << low << ".." << high << std::endl;
    assert(underruns == 0 && overflows == 0);
    assert(low > 64 && high < 2048);
    assert(std::abs(src.ppm() - ppm) < 10);
    std::cout << "skew " << ppm << " ppm block " << block << " reserve " << reserve << ": 400s simulated, occupancy " << low << ".." << high
              << ", correction " << src.ppm() << ", xruns 0\n";
}
int main() {
    ramp(48000); ramp(44100); ramp(96000);
    StereoRing empty; AdaptiveResampler src; src.prepare(48000); StereoFrame f{1, 1};
    assert(!src.next(empty, f) && f.left == 0 && f.right == 0);
    for (int i = 0; i < 2000; ++i) src.update(8192);
    assert(std::abs(src.ppm() - 1000) < 0.00001);
    src.prepare(48000);
    for (int i = 0; i < 2000; ++i) src.update(0);
    assert(std::abs(src.ppm() + 1000) < 0.00001);
    skew(200); skew(-200); skew(800); skew(-800);
    for (int block : {128, 512, 4096}) { skew(800, block, true); skew(-800, block, true); }
    std::cout << "SRC: same-rate exact/rate ratios/startup silence/correction limits passed\n";
}
