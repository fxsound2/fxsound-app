#include "portable-runtime.h"
#include "codedefs.h"
#include "File.h"
#include "pstr.h"
#include "mth.h"
#include <filesystem>
#include <cerrno>
FILE *fileOpen_Wide(wchar_t *path, wchar_t *mode, CSlout *) {
    if (!path || !mode) return nullptr;
    auto name = dspUtf8(path), flags = dspUtf8(mode);
    std::replace(name.begin(), name.end(), '\\', '/');
    return std::fopen(name.c_str(), flags.c_str());
}
int fileExist_Wide(wchar_t *path, int *exists) {
    if (!path || !exists) return NOT_OKAY;
    std::error_code error;
    *exists = std::filesystem::exists(dspUtf8(path), error);
    return error ? NOT_OKAY : OKAY;
}
int fileCreateDirectoryAndParents_Wide(wchar_t *path, int *created, CSlout *) {
    if (!path || !created) return NOT_OKAY;
    std::error_code error;
    *created = std::filesystem::create_directories(dspUtf8(path), error);
    return error ? NOT_OKAY : OKAY;
}
int pstrCovertUTF8StringToWideCharString_WithAlloc(char *input, wchar_t **output, int *length) {
    if (!input || !output || !length) return NOT_OKAY;
    try {
        auto text = dspWide(input);
        *length = static_cast<int>(text.size());
        *output = static_cast<wchar_t *>(std::calloc(text.size() + 1, sizeof(wchar_t)));
        if (!*output) return NOT_OKAY;
        std::copy(text.begin(), text.end(), *output);
        return OKAY;
    } catch (const std::range_error &) { return NOT_OKAY; }
}
int pstrCovertWideCharStringToUTF8String_WithAlloc(wchar_t *input, char **output, int *length) {
    if (!input || !output || !length) return NOT_OKAY;
    try {
        auto text = dspUtf8(input);
        *length = static_cast<int>(text.size());
        *output = static_cast<char *>(std::calloc(text.size() + 1, 1));
        if (!*output) return NOT_OKAY;
        std::copy(text.begin(), text.end(), *output);
        return OKAY;
    } catch (const std::range_error &) { return NOT_OKAY; }
}
int mthIsLong_Wide(wchar_t *text, int *valid) {
    if (!text || !valid) return NOT_OKAY;
    wchar_t *end = nullptr;
    errno = 0;
    std::wcstol(text, &end, 10);
    *valid = *text && *end == 0 && errno != ERANGE;
    return OKAY;
}

int pstrCalcLocationOfStrInStr_Wide(wchar_t *haystack, wchar_t *needle, int start, int *location, int *found) {
    if (!haystack || !needle || !location || !found || start < 0) return NOT_OKAY;
    *found = 0;
    *location = 0;
    if (static_cast<size_t>(start) > std::wcslen(haystack)) return OKAY;
    auto *result = std::wcsstr(haystack + start, needle);
    if (result) { *found = 1; *location = static_cast<int>(result - haystack); }
    return OKAY;
}
