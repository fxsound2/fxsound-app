#include "wav-file.h"
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <cmath>
namespace {
uint16_t u16(const unsigned char *p) { return p[0] | (uint16_t(p[1]) << 8); }
uint32_t u32(const unsigned char *p) { return u16(p) | (uint32_t(u16(p + 2)) << 16); }
void put16(std::ostream &out, uint16_t n) { char bytes[]{char(n), char(n >> 8)}; out.write(bytes, 2); }
void put32(std::ostream &out, uint32_t n) { put16(out, n & 65535); put16(out, n >> 16); }
}
WaveFile readWave(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open input WAV");
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), {});
    if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) ||
        std::memcmp(bytes.data() + 8, "WAVE", 4)) throw std::runtime_error("Invalid RIFF WAV");
    WaveFile wave;
    uint16_t format = 0, bits = 0;
    size_t offset = 0, length = 0;
    for (size_t i = 12; i + 8 <= bytes.size();) {
        uint32_t n = u32(bytes.data() + i + 4);
        if (n > bytes.size() - i - 8) throw std::runtime_error("Truncated WAV chunk");
        auto *data = bytes.data() + i + 8;
        if (!std::memcmp(bytes.data() + i, "fmt ", 4)) {
            if (n < 16) throw std::runtime_error("Invalid WAV format");
            format = u16(data); wave.channels = u16(data + 2); wave.rate = u32(data + 4); bits = u16(data + 14);
        } else if (!std::memcmp(bytes.data() + i, "data", 4)) { offset = i + 8; length = n; }
        i += 8 + n + (n & 1);
    }
    if (!offset || !wave.channels || !wave.rate || (format != 1 && format != 3) ||
        (format == 3 && bits != 32) || (format == 1 && bits != 16 && bits != 24 && bits != 32))
        throw std::runtime_error("Expected PCM16/24/32 or IEEE float32 WAV");
    size_t width = bits / 8;
    if (length % (width * wave.channels)) throw std::runtime_error("Partial WAV frame");
    wave.samples.resize(length / width);
    for (size_t i = 0; i < wave.samples.size(); ++i) {
        auto *data = bytes.data() + offset + i * width;
        if (format == 3) {
            uint32_t word = u32(data); std::memcpy(&wave.samples[i], &word, 4);
        } else if (bits == 16) wave.samples[i] = static_cast<int16_t>(u16(data)) / 32768.0f;
        else if (bits == 24) {
            int32_t word = data[0] | (int32_t(data[1]) << 8) | (int32_t(data[2]) << 16);
            if (word & 0x800000) word |= static_cast<int32_t>(0xff000000);
            wave.samples[i] = word / 8388608.0f;
        } else wave.samples[i] = static_cast<int32_t>(u32(data)) / 2147483648.0f;
        if (!std::isfinite(wave.samples[i])) throw std::runtime_error("Nonfinite WAV sample");
    }
    return wave;
}
void writeWave(const std::string &path, const WaveFile &wave) {
    uint64_t length = wave.samples.size() * 4ULL;
    if (length > UINT32_MAX - 36) throw std::runtime_error("WAV exceeds RIFF32 limit");
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot open output WAV");
    out.write("RIFF", 4); put32(out, uint32_t(length + 36)); out.write("WAVEfmt ", 8);
    put32(out, 16); put16(out, 3); put16(out, wave.channels); put32(out, wave.rate);
    put32(out, wave.rate * wave.channels * 4); put16(out, wave.channels * 4); put16(out, 32);
    out.write("data", 4); put32(out, uint32_t(length));
    for (float value : wave.samples) {
        if (!std::isfinite(value)) throw std::runtime_error("Nonfinite rendered sample");
        uint32_t word; std::memcpy(&word, &value, 4); put32(out, word);
    }
    if (!out) throw std::runtime_error("Cannot write output WAV");
}
