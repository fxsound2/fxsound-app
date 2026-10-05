#include "float32-engine.h"
#include "wav-file.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <algorithm>
int main(int argc, char **argv) {
    try {
        if (argc < 3 || argc > 5) {
            std::cerr << "Usage: fxsound-render input.wav output.wav [preset.fac|-] [blockFrames]\n";
            return 2;
        }
        auto wave = readWave(argv[1]);
        size_t block = argc > 4 ? std::stoul(argv[4]) : 256;
        if (!block || block > 16384) throw std::runtime_error("Invalid block size");
        fxsound::Float32Engine engine;
        if (!engine.prepare(wave.rate, wave.channels)) throw std::runtime_error("Unsupported DSP format");
        if (argc > 3 && std::string(argv[3]) != "-" && !engine.loadPreset(argv[3]))
            throw std::runtime_error("Cannot load preset");
        for (size_t frame = 0, total = wave.samples.size() / wave.channels; frame < total; frame += block) {
            auto *samples = wave.samples.data() + frame * wave.channels;
            if (!engine.process(samples, samples, std::min(block, total - frame)))
                throw std::runtime_error("DSP processing failed");
        }
        writeWave(argv[2], wave);
        std::ofstream hex(std::string(argv[2]) + ".float32.hex");
        if (!hex) throw std::runtime_error("Cannot write float dump");
        for (size_t i = 0; i < std::min<size_t>(1024, wave.samples.size()); ++i) {
            uint32_t bits; std::memcpy(&bits, &wave.samples[i], sizeof(bits));
            hex << std::hex << std::setfill('0') << std::setw(8) << bits << '\n';
        }
        if (!hex) throw std::runtime_error("Cannot finish float dump");
        std::cout << "Rendered " << wave.samples.size() / wave.channels << " frames at " << wave.rate
                  << " Hz, " << wave.channels << " channels, block " << block << '\n';
        return 0;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
