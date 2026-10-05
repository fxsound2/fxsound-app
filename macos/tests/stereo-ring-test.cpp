#include "../engine/stereo-ring.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
using namespace fxsound;
int main() {
    StereoRing ring;
    assert(ring.available() == 0);
    std::vector<StereoFrame> data(StereoRing::capacity);
    for (size_t i = 0; i < data.size(); ++i) data[i] = {float(i), -float(i)};
    assert(!ring.push(data.data(), data.size() + 1));
    assert(ring.push(data.data(), data.size()));
    assert(!ring.push(data.data(), 1));
    assert(ring.available() == data.size());
    for (size_t i = 0; i < data.size(); ++i) assert(ring.peek(i).left == float(i));
    ring.consume(data.size() - 3);
    assert(ring.push(data.data(), 17));
    for (size_t i = 0; i < 3; ++i) assert(ring.peek(i).left == float(data.size() - 3 + i));
    for (size_t i = 0; i < 17; ++i) assert(ring.peek(i + 3).left == float(i));
    ring.consume(20);
    assert(ring.available() == 0);
    ring.reset();
    constexpr size_t total = 1000000;
    std::atomic<bool> done{false};
    std::thread observer([&] {
        while (!done.load(std::memory_order_acquire)) {
            const auto count = ring.available();
            assert(count <= StereoRing::capacity);
        }
    });
    std::thread producer([&] {
        for (size_t i = 0; i < total; ++i) {
            StereoFrame f{float(i), -float(i)};
            while (!ring.push(&f, 1)) std::this_thread::yield();
        }
    });
    std::thread consumer([&] {
        for (size_t i = 0; i < total; ++i) {
            while (!ring.available()) std::this_thread::yield();
            const auto f = ring.peek(0);
            assert(f.left == float(i) && f.right == -float(i));
            ring.consume(1);
        }
    });
    producer.join(); consumer.join();
    done.store(true, std::memory_order_release); observer.join();
    assert(ring.available() == 0);
    std::cout << "ring: full/empty/wrap/overflow + 1000000 concurrent frames passed\n";
}
