#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <time.h>
#include "dsp-word.h"
typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef int BOOL;
typedef void *HWND;
typedef void *HINSTANCE;
typedef struct { DWORD dwLowDateTime, dwHighDateTime; } FILETIME;
#define TRUE 1
#define FALSE 0
#define __int64 long long
#define MB_OK 0
#define MB_ICONERROR 0
#define MB_TASKMODAL 0
static inline int MessageBoxA(HWND window, const char *message, const char *title, unsigned flags) {
    (void)window; (void)title; (void)flags;
    fprintf(stderr, "%s\n", message);
    return 0;
}
#ifdef __cplusplus
#include <string>
#include <string_view>
#include <algorithm>
#include <cwctype>
#include <cstdarg>
#include <chrono>
#include <codecvt>
#include <locale>
FILE *dspOpenValidatedPreset(const wchar_t *path);
int dspResizeEqSections(void *handle, int bands);
std::string dspUtf8(std::wstring_view text);
std::wstring dspWide(std::string_view text);
int dspWideFormat(wchar_t *buffer, size_t capacity, const wchar_t *format, va_list args);
int dspSwprintf(wchar_t *buffer, size_t capacity, const wchar_t *format, ...);
template<size_t N, class... Args>
int dspSwprintf(wchar_t (&buffer)[N], const wchar_t *format, Args... args) {
    return dspSwprintf(buffer, N, format, args...);
}
// Legacy dynamically allocated destinations are used for exact string copies.
inline int dspSwprintf(wchar_t *buffer, const wchar_t *format, const wchar_t *value) {
    if (std::wcscmp(format, L"%s") != 0) return -1;
    std::wcscpy(buffer, value);
    return static_cast<int>(std::wcslen(value));
}
wchar_t *dspReadWideLine(wchar_t *buffer, int capacity, FILE *stream);
int dspWriteWide(FILE *stream, const wchar_t *format, ...);
inline unsigned long dspTicks() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
#define swprintf dspSwprintf
#define fgetws dspReadWideLine
#define fwprintf dspWriteWide
#define _wtoi(value) static_cast<int>(std::wcstol(value, nullptr, 10))
#define _wtol(value) std::wcstol(value, nullptr, 10)
#define _wtof(value) std::wcstod(value, nullptr)
#define _wcsicmp wcscasecmp
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define GetTickCount dspTicks
inline int MessageBox(HWND, const wchar_t *message, const wchar_t *, unsigned) {
    std::fprintf(stderr, "%s\n", dspUtf8(message).c_str());
    return 0;
}
#endif
