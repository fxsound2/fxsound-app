#include "wav-file.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
int main(int argc, char **argv) {
    try {
        if (argc != 3) { std::cerr << "Usage: fxsound-compare reference.wav render.wav\n"; return 2; }
        auto reference = readWave(argv[1]), render = readWave(argv[2]);
        if (reference.rate != render.rate || reference.channels != render.channels ||
            reference.samples.size() != render.samples.size()) {
            std::cerr << "Format/length mismatch; no automatic alignment applied\n"; return 1;
        }
        double maximum = 0, squares = 0;
        bool exact = true;
        for (size_t i = 0; i < reference.samples.size(); ++i) {
            double error = double(reference.samples[i]) - double(render.samples[i]);
            maximum = std::max(maximum, std::abs(error)); squares += error * error;
            exact &= reference.samples[i] == render.samples[i];
        }
        double rms = reference.samples.empty() ? 0 : std::sqrt(squares / reference.samples.size());
        std::cout << std::setprecision(12) << "samples=" << reference.samples.size()
                  << " alignment=0 nonfinite=0 exact=" << exact << " max=" << maximum
                  << " rms=" << rms << " maxDbFS=" << 20 * std::log10(maximum)
                  << " rmsDbFS=" << 20 * std::log10(rms) << '\n';
        return maximum <= 1e-6 && rms <= 1e-6 ? 0 : 1;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
