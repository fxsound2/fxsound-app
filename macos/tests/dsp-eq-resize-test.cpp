#include "../dsp/float32-engine.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
int main(int argc, char** argv) {
    assert(argc == 2);
    fxsound::Float32Engine engine; assert(engine.prepare(48000, 2)); assert(engine.loadPreset(argv[1]));
    auto& controls = engine.controls();
    std::vector<float> input(1024), output(input.size());
    for (size_t i = 0; i < input.size(); ++i) input[i] = float(0.1 * std::sin(i * 0.1));
    for (int cycle = 0; cycle < 4; ++cycle) {
        for (int bands : {5, 10, 15, 20, 31, 10, 5}) {
            controls.setNumBands(bands); assert(controls.getNumEqBands() == bands);
            controls.setMasterGain(2); controls.setBalance(-1);
            controls.setVolumeLeveling(2); controls.setFilterQ(1.5f);
            assert(controls.getMasterGain() == 2 && controls.getBalance() == -1);
            assert(controls.getVolumeLeveling() == 2 && controls.getFilterQ() == 1.5f);
            for (int band = 0; band < bands; ++band) {
                const float gain = float(band % 7 - 3);
                controls.setEqBandBoostCut(band, gain);
                assert(controls.getEqBandBoostCut(band) == gain);
                float low = 0, high = 0; controls.getEqBandFrequencyRange(band, &low, &high);
                assert(std::isfinite(low) && low > 0 && high >= low);
                float frequency = (low + high) / 2;
                controls.setEqBandFrequency(band, frequency);
                assert(std::abs(controls.getEqBandFrequency(band) - frequency) < 0.01f);
            }
            assert(engine.process(input.data(), output.data(), 512));
            for (float sample : output) assert(std::isfinite(sample));
        }
        assert(engine.loadPreset(argv[1]));
        assert(engine.process(input.data(), output.data(), 512));
        for (float sample : output) assert(std::isfinite(sample));
    }
    std::cout << "DSP EQ:28resizes 5/10/15/20/31/shrink,all gains/frequencies/master controls/finite/preset reset passed\n";
}
