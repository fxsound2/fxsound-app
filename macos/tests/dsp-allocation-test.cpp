#include "../dsp/float32-engine.h"
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <malloc/malloc.h>
#include <vector>
static std::atomic<bool> tracking{false};
static std::atomic<size_t> allocations{0};
static void record() { if (tracking.load(std::memory_order_relaxed)) allocations.fetch_add(1); }
extern "C" void* malloc(size_t size) { record(); return malloc_zone_malloc(malloc_default_zone(), size); }
extern "C" void* calloc(size_t count, size_t size) { record(); return malloc_zone_calloc(malloc_default_zone(), count, size); }
extern "C" void* realloc(void* pointer, size_t size) { record(); return malloc_zone_realloc(malloc_default_zone(), pointer, size); }
__attribute__((noinline)) static void probe() {
    void* (*volatile function)(size_t) = malloc;
    void* memory = function(32); free(memory);
}
int main(int argc, char** argv) {
    assert(argc == 2);
    tracking = true; probe(); tracking = false;
    assert(allocations > 0); allocations = 0;
    tracking = true;
    fxsound::Float32Engine engine;
    tracking = false; assert(allocations > 0); allocations = 0;
    assert(engine.prepare(48000, 2)); assert(engine.loadPreset(argv[1]));
    std::vector<float> input(512 * 2, 0.01f), output(input.size());
    engine.setPower(true);
    for (int i = 0; i < 20; ++i) assert(engine.process(input.data(), output.data(), 512));
    tracking = true;
    for (int i = 0; i < 10000; ++i) assert(engine.process(input.data(), output.data(), 512));
    tracking = false;
    std::cout << "DSP malloc/calloc/realloc calls over10000 warm callbacks=" << allocations << std::endl;
    assert(allocations == 0);
}
