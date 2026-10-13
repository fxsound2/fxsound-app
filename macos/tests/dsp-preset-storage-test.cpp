#include "../engine/dsp-controller.h"
#include "../dsp/float32-engine.h"
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <array>
#include <unistd.h>
#include <sys/stat.h>
namespace fs = std::filesystem;
static std::string read(const fs::path& p) {
    std::ifstream stream(p); return {std::istreambuf_iterator<char>(stream), {}};
}
static void clean(const fs::path& p) {
    for (const auto& file : fs::directory_iterator(p))
        assert(file.path().filename().string().find(".fxsound-preset-") != 0);
}
int main(int argc, char** argv) {
    assert(argc == 2);
    char pattern[] = "/private/tmp/fxsound-storage-test-XXXXXX";
    assert(mkdtemp(pattern)); const fs::path directory(pattern), saved = directory / "roundtrip.fac";
    struct Cleanup { fs::path p; ~Cleanup() { fs::remove_all(p); } } cleanup{directory};
    fxsound::DSPController controller;
    controller.edit([&] { controller.preset(argv[1]); controller.numBands(31);
        controller.parameter("eq1", 3); controller.parameter("eq31", -2);
        controller.parameter("fidelity", 7); });
    { std::ofstream(saved) << "existing sentinel"; }
    struct stat before{}, after{}; assert(stat(saved.c_str(), &before) == 0);
    controller.edit([&] { controller.savePreset(saved); });
    assert(stat(saved.c_str(), &after) == 0 && before.st_ino != after.st_ino);
    const auto bytes = read(saved); assert(bytes.size() > 100 && bytes.find("existing sentinel") == std::string::npos);
    controller.edit([&] { controller.preset(argv[1]); controller.preset(saved); });
    fxsound::Float32Engine engine; assert(engine.prepare(48000, 2)); assert(engine.loadPreset(saved));
    auto& restored = engine.controls();
    assert(restored.getNumEqBands() == 31);
    assert(restored.getEqBandBoostCut(0) == 3 && restored.getEqBandBoostCut(30) == -2);
    assert(std::abs(restored.getEffectValue(DfxDsp::Effect::Fidelity) - 0.7) < 1.0 / 127);
    std::array<float,1024> input{},output{};
    for(size_t i=0;i<input.size();++i) input[i]=float(0.1*std::sin(i*0.1));
    assert(engine.process(input.data(),output.data(),512));
    bool changed=false;for(size_t i=0;i<output.size();++i){assert(std::isfinite(output[i]));changed|=output[i]!=input[i];}
    assert(changed);
    for(const auto& bad : {directory / "roundtrip.txt", directory / "missing/roundtrip.fac", directory / "blocked.fac"}) {
        if(bad.filename()=="roundtrip.txt")std::ofstream(bad)<<"preserve text";
        if(bad.filename()=="blocked.fac"){fs::create_directory(bad);std::ofstream(bad/"sentinel")<<"preserve directory";}
        bool rejected=false;
        try{controller.edit([&]{controller.savePreset(bad);});}catch(const std::exception&){rejected=true;}
        assert(rejected && read(saved)==bytes);clean(directory);
        if(bad.filename()=="roundtrip.txt")assert(read(bad)=="preserve text");
        if(bad.filename()=="blocked.fac")assert(read(bad/"sentinel")=="preserve directory");
    }
    clean(directory);
    std::cout<<"Preset storage:atomic inode replacement,31-band gain/effect roundtrip,finite wet render,non-destructive validation/rename failures,stage cleanup passed\n";
}
