#include "../dsp/float32-engine.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <unistd.h>
using fxsound::Float32Engine;
static bool render(int rate, int channels, const std::string& preset) {
    Float32Engine engine; assert(engine.prepare(rate, channels));
    assert(engine.loadPreset(preset)); engine.setPower(true);
    std::vector<float> input(16384 * channels), output(input.size());
    size_t cursor = 0; bool changed = false, nonzero = false;
    for (size_t frames : {size_t(1), size_t(2), size_t(3), size_t(127), size_t(128), size_t(511), size_t(4096), size_t(16384)}) {
        for (size_t i = 0; i < frames; ++i)
            for (int c = 0; c < channels; ++c)
                input[i * channels + c] = float(0.1 * std::sin(2 * 3.141592653589793 * 440 * (cursor + i) / rate));
        assert(engine.process(input.data(), output.data(), frames));
        for (size_t i = 0; i < frames * channels; ++i) {
            assert(std::isfinite(output[i]));
            changed |= output[i] != input[i]; nonzero |= std::abs(output[i]) > 0.000001f;
        }
        cursor += frames;
    }
    assert(nonzero);
    engine.setPower(false);
    assert(engine.process(input.data(), output.data(), 4096));
    assert(std::memcmp(input.data(), output.data(), 4096 * channels * sizeof(float)) == 0);
    engine.setPower(true); std::fill(input.begin(), input.end(), 0);
    input[0] = 0.1f;
    assert(engine.process(input.data(), output.data(), 4096));
    for (int block = 0; block < 5; ++block) {
        std::fill(input.begin(), input.end(), 0);
        assert(engine.process(input.data(), output.data(), 4096));
        for (float sample : output) assert(std::isfinite(sample));
    }
    return changed;
}
int main(int argc, char** argv) {
    assert(argc == 2); const std::filesystem::path root(argv[1]);
    Float32Engine engine;
    for (int i = 0; i < 5; ++i)
        assert(std::isfinite(engine.controls().getEffectValue(static_cast<DfxDsp::Effect>(i))));
    float samples[32]{};
    assert(!engine.process(samples, samples, 16));
    for (int channels : {0, 1, 3, 5, 7, 9}) assert(!engine.prepare(48000, channels));
    for (int rate : {0, 15999, 96001, 128000, 192001}) assert(!engine.prepare(rate, 2));
    for (int rate : {16000, 44100, 48000, 96000, 192000}) assert(engine.prepare(rate, 2));
    assert(!engine.process(nullptr, samples, 1));
    assert(!engine.process(samples, nullptr, 1));
    assert(!engine.process(samples, samples, 16385));
    assert(!engine.process(samples, samples + 1, 4));
    assert(engine.process(samples, samples, 0));
    const auto factory = root / "Installer/Resources/Factsoft";
    size_t presets = 0, changedRenders = 0;
    for (const auto& entry : std::filesystem::directory_iterator(factory)) {
        if (entry.path().extension() != ".fac") continue;
        ++presets;
        for (int rate : {44100, 48000, 96000})
            for (int channels : {2, 4, 6, 8}) changedRenders += render(rate, channels, entry.path().string());
    }
    assert(presets >= 5 && changedRenders > 0);
    for (int channels : {2, 4, 6, 8}) render(192000, channels, (factory / "Default.fac").string());
    assert(engine.prepare(48000, 2)); assert(engine.loadPreset((factory / "Default.fac").string()));
    engine.controls().setEffectValue(DfxDsp::Fidelity, 4.5f);
    engine.controls().setEqBandBoostCut(3, 2.5f);
    const auto effect = engine.controls().getEffectValue(DfxDsp::Fidelity);
    const auto eq = engine.controls().getEqBandBoostCut(3);
    char directory[] = "/private/tmp/fxsound-dsp-test-XXXXXX"; assert(mkdtemp(directory));
    const auto saved = std::filesystem::path(directory) / "Roundtrip.fac";
    assert(engine.controls().savePreset(L"Roundtrip", std::filesystem::path(directory).wstring()) == 0);
    Float32Engine restored; assert(restored.prepare(48000, 2)); assert(restored.loadPreset(saved.string()));
    assert(std::abs(restored.controls().getEffectValue(DfxDsp::Fidelity) - effect) < 0.01f);
    assert(std::abs(restored.controls().getEqBandBoostCut(3) - eq) < 0.01f);
    assert(!restored.loadPreset((saved.string() + ".missing")));
    const auto bad = std::filesystem::path(directory) / "malformed.fac";
    for (const auto* text : {"", "CLASS1\n9\n", "invalid", "CLASS1\nnan\n"}) {
        std::ofstream file(bad); file << text; file.close(); assert(!restored.loadPreset(bad.string()));
        assert(restored.process(samples, samples, 8));
    }
    std::filesystem::remove_all(directory);
    std::cout << "DSP: " << presets << " factory presets x3 rates x4 layouts,variable blocks/finite/silence/impulse/bypass/roundtrip/errors passed\n";
}
