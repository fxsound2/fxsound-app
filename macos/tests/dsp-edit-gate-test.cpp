#include "../engine/audio-bridge.h"
#include <memory>
#include <atomic>
#include <functional>
#include <mutex>
#define private public
#include "../engine/dsp-controller.h"
#undef private
#include "../dsp/float32-engine.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <thread>
#include <array>
#include <chrono>
#ifdef FXSOUND_TEST_ALLOCATIONS
#include <malloc/malloc.h>
#include <cstdlib>
static thread_local bool trackAllocations = false;
static std::atomic<unsigned> allocations{0};
static void recordAllocation() { if (trackAllocations) ++allocations; }
extern "C" void* malloc(size_t n) { recordAllocation(); return malloc_zone_malloc(malloc_default_zone(), n); }
extern "C" void* calloc(size_t n, size_t size) { recordAllocation(); return malloc_zone_calloc(malloc_default_zone(), n, size); }
extern "C" void* realloc(void* p, size_t n) { recordAllocation(); return malloc_zone_realloc(malloc_default_zone(), p, n); }
#endif
int main(int argc, char** argv) {
    assert(argc == 2);
#ifdef FXSOUND_TEST_ALLOCATIONS
    trackAllocations = true;
    void* (*volatile allocate)(size_t) = malloc;
    free(allocate(32)); trackAllocations = false; assert(allocations > 0); allocations = 0;
#endif
    fxsound::DSPController controller;
    assert(controller.dsp_->prepare(48000, 2)); controller.preset(argv[1]); controller.bypass(false);
    std::array<float, 512> input{}, samples{};
    for (size_t i = 0; i < input.size(); ++i) input[i] = float(0.1 * std::sin(i * 0.1));
    std::atomic<bool> held{false}, release{false};
    std::thread edit([&] { controller.edit([&] {
        held = true; while (!release.load()) std::this_thread::yield();
        controller.parameter("fidelity", 5);
    }); });
    while (!held.load()) std::this_thread::yield();
    for (int i = 0; i < 1000; ++i) {
        samples = input;
#ifdef FXSOUND_TEST_ALLOCATIONS
        trackAllocations = true;
#endif
        controller.process(&controller, samples.data(), 256);
#ifdef FXSOUND_TEST_ALLOCATIONS
        trackAllocations = false;
#endif
        assert(std::memcmp(input.data(), samples.data(), sizeof(input)) == 0);
    }
    release = true; edit.join();
    bool wet = false;
    for (int i = 0; i < 100; ++i) {
        samples = input;
#ifdef FXSOUND_TEST_ALLOCATIONS
        trackAllocations = true;
#endif
        controller.process(&controller, samples.data(), 256);
#ifdef FXSOUND_TEST_ALLOCATIONS
        trackAllocations = false;
#endif
        for (float sample : samples) assert(std::isfinite(sample));
        wet |= std::memcmp(input.data(), samples.data(), sizeof(input)) != 0;
    }
    assert(wet);
    try { controller.edit([] { throw 7; }); assert(false); } catch (int value) { assert(value == 7); }
    assert(!controller.configuring_.load());
    bool mutated = false; controller.active_ = 1;
    const auto start = std::chrono::steady_clock::now();
    try { controller.edit([&] { mutated = true; }); assert(false); } catch (const std::runtime_error&) {}
    assert(!mutated && !controller.configuring_.load()); controller.active_ = 0;
    assert(std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(900));
    std::atomic<bool> stop{false}; std::atomic<unsigned> callbacks{0};
    std::thread callback([&] {
        std::array<float, 512> block;
        while (!stop.load()) {
            block = input;
#ifdef FXSOUND_TEST_ALLOCATIONS
            trackAllocations = true;
#endif
            controller.process(&controller, block.data(), 256);
#ifdef FXSOUND_TEST_ALLOCATIONS
            trackAllocations = false;
#endif
            for (float sample : block) assert(std::isfinite(sample));
            ++callbacks;
        }
    });
    while (callbacks.load() < 100) std::this_thread::yield();
    for (int i = 0; i < 100; ++i) controller.edit([&] {
        controller.parameter("fidelity", i % 11);
        controller.numBands(i % 2 ? 31 : 5);
        controller.parameter("eq1", i % 7 - 3);
        if (i % 10 == 0) controller.preset(argv[1]);
    });
    for (int i = 0; i < 1000; ++i) assert(!controller.stateJSON().empty());
    stop = true; callback.join();
    assert(callbacks.load() >= 100 && controller.active_.load() == 0);
#ifdef FXSOUND_TEST_ALLOCATIONS
    assert(allocations == 0);
    std::cout << "Callback malloc/calloc/realloc observed=" << allocations << '\n';
#endif
    std::cout << "DSP edit gate:1000 exact dry blocks,wet resume,exception/timeout reset,100 concurrent real edits passed; callbacks=" << callbacks.load() << '\n';
}
