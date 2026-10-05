#include "../dsp/float32-engine.h"
#include <atomic>
#include <cassert>
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>
int main(int argc, char** argv) {
    assert(argc == 2);
    std::atomic<int> barrier{0};
    auto run = [&] {
        ++barrier;
        while (barrier.load() < 2) std::this_thread::yield();
        for (int instance = 0; instance < 30; ++instance) {
            fxsound::Float32Engine engine;
            assert(engine.prepare(48000, 2)); assert(engine.loadPreset(argv[1]));
            engine.controls().setEffectValue(DfxDsp::Fidelity, 0.5f);
            std::vector<float> input(1024, 0.05f), output(1024);
            for (int i = 0; i < 100; ++i) {
                assert(engine.process(input.data(), output.data(), 512));
                for (float sample : output) assert(std::isfinite(sample));
            }
        }
    };
    std::thread first(run), second(run); first.join(); second.join();
    std::cout << "DSP concurrent independent instances:60construction/preset/control/processing cycles passed\n";
}
