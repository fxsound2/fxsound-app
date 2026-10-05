#pragma once
#include <cstdint>
#include <string>
#include <vector>
struct WaveFile { uint32_t rate = 0; uint16_t channels = 0; std::vector<float> samples; };
WaveFile readWave(const std::string &path);
void writeWave(const std::string &path, const WaveFile &wave);
