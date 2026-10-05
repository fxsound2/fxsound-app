#include "portable-runtime.h"
#include <vector>
std::string dspUtf8(std::wstring_view text) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(text.data(), text.data() + text.size());
}
std::wstring dspWide(std::string_view text) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(text.data(), text.data() + text.size());
}
static std::wstring portableFormat(const wchar_t *format) {
    std::wstring result;
    for (size_t i = 0; format[i]; ++i) {
        if (format[i] == '%' && format[i + 1] == 's') {
            result += L"%ls";
            ++i;
        } else result += format[i];
    }
    return result;
}
int dspWideFormat(wchar_t *buffer, size_t capacity, const wchar_t *format, va_list args) {
    auto normalized = portableFormat(format);
    return std::vswprintf(buffer, capacity, normalized.c_str(), args);
}
int dspSwprintf(wchar_t *buffer, size_t capacity, const wchar_t *format, ...) {
    va_list args;
    va_start(args, format);
    int result = dspWideFormat(buffer, capacity, format, args);
    va_end(args);
    return result;
}
wchar_t *dspReadWideLine(wchar_t *buffer, int capacity, FILE *stream) {
    if (capacity <= 0) return nullptr;
    std::vector<char> bytes(static_cast<size_t>(capacity) * 4);
    if (!std::fgets(bytes.data(), static_cast<int>(bytes.size()), stream)) return nullptr;
    auto wide = dspWide(bytes.data());
    if (wide.size() >= static_cast<size_t>(capacity)) return nullptr;
    std::copy(wide.begin(), wide.end(), buffer);
    buffer[wide.size()] = 0;
    return buffer;
}
int dspWriteWide(FILE *stream, const wchar_t *format, ...) {
    wchar_t buffer[8192];
    va_list args;
    va_start(args, format);
    int count = dspWideFormat(buffer, 8192, format, args);
    va_end(args);
    if (count < 0) return count;
    auto bytes = dspUtf8(buffer);
    return std::fwrite(bytes.data(), 1, bytes.size(), stream) == bytes.size() ? count : -1;
}
