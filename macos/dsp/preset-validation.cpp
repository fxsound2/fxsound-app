#include "portable-runtime.h"
#include <fstream>
#include <vector>
#include <sstream>
#include <memory>
#include <cerrno>
#include <climits>
#include <cmath>
namespace {
class Lines {
public:
    explicit Lines(const std::string &content) {
        std::istringstream file(content);
        std::string line;
        size_t bytes = 0;
        while (std::getline(file, line)) {
            bytes += line.size() + 1;
            if (bytes > 65536 || line.size() >= 127) throw std::runtime_error("Preset exceeds parser bounds");
            if (!line.empty() && line.back() == '\r') line.pop_back();
            dspWide(line);
            lines_.push_back(std::move(line));
        }
        if (file.bad()) throw std::runtime_error("Cannot finish reading preset");
    }
    const std::string &text() {
        if (index_ == lines_.size()) throw std::runtime_error("Truncated preset");
        return lines_[index_++];
    }
    int integer(int minimum = INT_MIN, int maximum = INT_MAX) {
        auto &line = text(); char *end = nullptr; errno = 0;
        long value = std::strtol(line.c_str(), &end, 10);
        if (end == line.c_str() || errno == ERANGE || value < minimum || value > maximum)
            throw std::runtime_error("Invalid preset integer");
        return static_cast<int>(value);
    }
    float real(float minimum = -3.4e38f, float maximum = 3.4e38f) {
        auto &line = text(); char *end = nullptr; errno = 0;
        float value = std::strtof(line.c_str(), &end);
        if (end == line.c_str() || errno == ERANGE || !std::isfinite(value) || value < minimum || value > maximum)
            throw std::runtime_error("Invalid preset real");
        return value;
    }
private:
    std::vector<std::string> lines_;
    size_t index_ = 0;
};
}
static bool validatePreset(const std::string &bytes) {
    try {
        Lines lines(bytes);
        if (lines.text().find("CLASS1") != 0) return false;
        float version = lines.real(1, 9);
        lines.text();
        int doubled = version > 1 ? lines.integer(0, 1) : 0;
        int elements = lines.integer(0, 8);
        for (int i = 0; i < 6 * (doubled + 1); ++i) lines.integer();
        for (int element = 0; element < elements; ++element) {
            if (lines.integer() != element) return false;
            for (int i = 0; i < 7 * (doubled + 1); ++i) lines.integer();
        }
        int integers = lines.integer(0, 128), reals = lines.integer(0, 128), strings = lines.integer(0, 128);
        for (int i = 0; i < integers; ++i) lines.integer();
        for (int i = 0; i < reals; ++i) lines.real();
        for (int i = 0; i < strings; ++i) { lines.text(); lines.text(); }
        if (version >= 9) {
            int bands = lines.integer(1, 32);
            lines.integer(0, 1);
            for (int i = 0; i < bands; ++i) {
                lines.text(); lines.real(10, 21000); lines.real(-30, 30);
            }
        }
        return true;
    } catch (const std::exception &) { return false; }
}

FILE *dspOpenValidatedPreset(const wchar_t *path) {
    try {
        if (!path) return nullptr;
        std::ifstream source(dspUtf8(path), std::ios::binary);
        if (!source) return nullptr;
        std::string bytes(65537, '\0');
        source.read(bytes.data(), bytes.size());
        if (source.bad() || source.gcount() > 65536) return nullptr;
        bytes.resize(static_cast<size_t>(source.gcount()));
        if (!validatePreset(bytes)) return nullptr;
        std::unique_ptr<FILE, decltype(&std::fclose)> stream(
            fmemopen(nullptr, bytes.size() + 1, "w+b"), &std::fclose);
        if (!stream || std::fwrite(bytes.data(), 1, bytes.size(), stream.get()) != bytes.size()) return nullptr;
        std::rewind(stream.get());
        return stream.release();
    } catch (const std::exception &) { return nullptr; }
}
