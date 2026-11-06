/*
FxSound Linux port — compat/win_compat.h (Phase 5)
Narrow Windows shim for DfxDsp / ptutil / AudioPassthruCore ONLY.
Never force-included; never on PipeWire include path.
Uses <cstdint> types. Does NOT define min/max/near/far/VOID.
Untested on Linux.
*/
#pragma once
#ifdef __cplusplus
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <thread>
#include <chrono>
#else
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#endif
#ifndef __int64
#define __int64 long long
#endif

#ifdef __cplusplus
using DWORD = uint32_t;
using LONG = int32_t;
using BOOL = int32_t;
using BYTE = uint8_t;
using WORD = uint16_t;
using WCHAR = wchar_t;
#ifndef HANDLE
using HANDLE = void*;
#endif
#ifndef HWND
using HWND = void*;
#endif
#ifndef HINSTANCE
using HINSTANCE = void*;
#endif
using HRESULT = int32_t;
using WPARAM = uintptr_t;
using LPARAM = intptr_t;
using LRESULT = intptr_t;
using LPVOID = void*;
using LPCVOID = const void*;
using LPSTR = char*;
using LPCSTR = const char*;
using LPWSTR = wchar_t*;
using LPCWSTR = const wchar_t*;
using LPCTSTR = const char*;
using LPTSTR = char*;
struct SECURITY_ATTRIBUTES;
using LPSECURITY_ATTRIBUTES = SECURITY_ATTRIBUTES*;
#else
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef int32_t BOOL;
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef wchar_t WCHAR;
#ifndef HANDLE
typedef void* HANDLE;
#endif
#ifndef HWND
typedef void* HWND;
#endif
#ifndef HINSTANCE
typedef void* HINSTANCE;
#endif
typedef int32_t HRESULT;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef const char* LPCTSTR;
typedef char* LPTSTR;
struct SECURITY_ATTRIBUTES;
typedef struct SECURITY_ATTRIBUTES* LPSECURITY_ATTRIBUTES;
#endif
#ifndef _WIN64
#define _WIN64 1
#endif
#ifdef __cplusplus
struct FILETIME {
  DWORD dwLowDateTime = 0;
  DWORD dwHighDateTime = 0;
};
#else
struct FILETIME {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
};
#endif
#ifndef MB_OK
#define MB_OK 0
#endif
// Neutralize MSVC declspec on GCC (dllexport/dllimport only; Phase 6).
// Does NOT define min/max/near/far/VOID per plan.
#ifndef __declspec
#define __declspec(x)
#endif
#ifdef __cplusplus
constexpr HRESULT S_OK = 0;
constexpr HRESULT E_FAIL = (HRESULT)0x80004005;
inline bool SUCCEEDED(HRESULT h) { return h >= 0; }
inline bool FAILED(HRESULT h) { return h < 0; }
#else
#define S_OK ((HRESULT)0)
#define E_FAIL ((HRESULT)0x80004005)
#define SUCCEEDED(h) ((h) >= 0)
#define FAILED(h) ((h) < 0)
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifdef __cplusplus
inline void Sleep(DWORD ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
#else
#include <unistd.h>
static inline void Sleep(DWORD ms) { usleep((useconds_t)ms * 1000); }
#endif

#ifdef __cplusplus
// _stricmp / _strnicmp / _wcsicmp as inline functions (not macros)
inline int _stricmp(const char* a, const char* b) {
  for (; *a && *b; ++a, ++b) {
    int ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : (unsigned char)*a;
    int cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : (unsigned char)*b;
    if (ca != cb) return ca - cb;
  }
  return (unsigned char)*a - (unsigned char)*b;
}
inline int _strnicmp(const char* a, const char* b, size_t n) {
  for (size_t i = 0; i < n; ++i, ++a, ++b) {
    if (!*a && !*b) return 0;
    int ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : (unsigned char)*a;
    int cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : (unsigned char)*b;
    if (ca != cb) return ca - cb;
    if (!*a) break;
  }
  return 0;
}
inline int _wcsicmp(const wchar_t* a, const wchar_t* b) {
  for (; *a && *b; ++a, ++b) {
    wchar_t ca = (*a >= L'A' && *a <= L'Z') ? *a + 32 : *a;
    wchar_t cb = (*b >= L'A' && *b <= L'Z') ? *b + 32 : *b;
    if (ca != cb) return (int)(ca - cb);
  }
  return (int)(*a - *b);
}
inline int wcscpy_s(wchar_t* dst, size_t n, const wchar_t* src) {
  if (!dst || !src || n == 0) return 1;
  size_t i = 0;
  for (; i + 1 < n && src[i]; ++i) dst[i] = src[i];
  dst[i] = 0;
  return src[i] ? 1 : 0;
}
template <size_t N>
inline int wcscpy_s(wchar_t (&dst)[N], const wchar_t* src) {
  return wcscpy_s(dst, N, src);
}
#else
static inline int _stricmp(const char* a, const char* b) {
  for (; *a && *b; ++a, ++b) {
    int ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : (unsigned char)*a;
    int cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : (unsigned char)*b;
    if (ca != cb) return ca - cb;
  }
  return (unsigned char)*a - (unsigned char)*b;
}
#endif
#if !defined(_WIN32) && defined(__cplusplus)
#include <cstdarg>
#include <cwchar>
#include <cwctype>
#include <cerrno>
#include <ctime>
#include <random>
// Linux compat: Windows 2-arg swprintf(dest, L"fmt", ...) (MSVC _CRT_NON_CONFORMING)
// vs ISO 3-arg swprintf(dest, size, L"fmt", ...). Overload by 2nd arg type so both
// call forms compile on Linux without editing 145 call sites. Windows unchanged
// (uses real SDK). Size 8192 covers PT_MAX_PATH_STRLEN=1024 buffers in practice;
// Windows had no bounds check either.
inline int swprintf(wchar_t* dest, const wchar_t* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vswprintf(dest, 8192, fmt, ap);
  va_end(ap);
  return r;
}
// wsprintf (Windows, no size): same 8192 assumption.
inline int wsprintfW(wchar_t* dest, const wchar_t* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  int r = vswprintf(dest, 8192, fmt, ap);
  va_end(ap); return r;
}
inline int wsprintfA(char* dest, const char* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  int r = vsnprintf(dest, 8192, fmt, ap);
  va_end(ap); return r;
}
inline int wsprintf(wchar_t* dest, const wchar_t* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  int r = vswprintf(dest, 8192, fmt, ap);
  va_end(ap); return r;
}
inline int wsprintf(char* dest, const char* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  int r = vsnprintf(dest, 8192, fmt, ap);
  va_end(ap); return r;
}
// wcstok_s (MSVC) == wcstok (POSIX) signature.
inline wchar_t* wcstok_s(wchar_t* str, const wchar_t* delim, wchar_t** ctx) {
  return wcstok(str, delim, ctx);
}
#include <cstdio>
#include <cstdlib>
inline int _wtoi(const wchar_t* s) {
  if (!s) return 0;
  return (int)wcstol(s, nullptr, 10);
}
inline long _wtol(const wchar_t* s) {
  if (!s) return 0;
  return wcstol(s, nullptr, 10);
}
inline double _wtof(const wchar_t* s) {
  if (!s) return 0.0;
  return wcstod(s, nullptr);
}
using errno_t = int;
inline errno_t _wfopen_s(FILE** pFile, const wchar_t* filename, const wchar_t* mode) {
  if (!pFile) return 1;
  *pFile = nullptr;
  if (!filename || !mode) return 1;
  char mfilename[4096] = {0};
  char mmode[32] = {0};
  if (wcstombs(mfilename, filename, sizeof(mfilename) - 1) == (size_t)-1) return 1;
  if (wcstombs(mmode, mode, sizeof(mmode) - 1) == (size_t)-1) return 1;
  *pFile = fopen(mfilename, mmode);
  return (*pFile == nullptr) ? 1 : 0;
}
using HMODULE = void*;
inline HMODULE LoadLibraryW(const wchar_t*) { return nullptr; }
inline HMODULE LoadLibraryA(const char*) { return nullptr; }
inline HMODULE LoadLibrary(const wchar_t*) { return nullptr; }
inline HMODULE LoadLibrary(const char*) { return nullptr; }
inline int FreeLibrary(HMODULE) { return 1; }
inline void* GetProcAddress(HMODULE, const char*) { return nullptr; }
// FILETIME/SYSTEMTIME + error + GUID shims for pstr/ptime (Linux only).
struct SYSTEMTIME {
  WORD wYear = 0, wMonth = 0, wDayOfWeek = 0, wDay = 0;
  WORD wHour = 0, wMinute = 0, wSecond = 0, wMilliseconds = 0;
};
using LPFILETIME = FILETIME*;
using LPSYSTEMTIME = SYSTEMTIME*;
using LONGLONG = long long;
struct TIME_ZONE_INFORMATION {
  LONG Bias = 0;
  WCHAR StandardName[32] = {0};
  SYSTEMTIME StandardDate = {};
  LONG StandardBias = 0;
  WCHAR DaylightName[32] = {0};
  SYSTEMTIME DaylightDate = {};
  LONG DaylightBias = 0;
};
inline DWORD GetTimeZoneInformation(TIME_ZONE_INFORMATION* tz) {
  if (tz) *tz = TIME_ZONE_INFORMATION{};
  return 0;
}
inline int SystemTimeToTzSpecificLocalTime(const TIME_ZONE_INFORMATION*, const SYSTEMTIME* src, SYSTEMTIME* dst) {
  if (!src || !dst) return 0;
  *dst = *src;
  return 1;
}
#ifndef LOCALE_SYSTEM_DEFAULT
#define LOCALE_SYSTEM_DEFAULT 0x0800
#endif
#ifndef LOCALE_NOUSEROVERRIDE
#define LOCALE_NOUSEROVERRIDE 0x80000000
#endif
inline int GetDateFormatW(DWORD, DWORD, const SYSTEMTIME* st, const wchar_t*, wchar_t* out, int outlen) {
  if (!st || !out || outlen <= 0) return 0;
  int r = swprintf(out, (size_t)outlen, L"%02d/%02d/%04d", st->wMonth, st->wDay, st->wYear);
  return r > 0 ? r + 1 : 0;
}
inline int GetTimeFormatW(DWORD, DWORD, const SYSTEMTIME* st, const wchar_t*, wchar_t* out, int outlen) {
  if (!st || !out || outlen <= 0) return 0;
  int r = swprintf(out, (size_t)outlen, L"%02d:%02d:%02d", st->wHour, st->wMinute, st->wSecond);
  return r > 0 ? r + 1 : 0;
}
inline int SystemTimeToFileTime(const SYSTEMTIME* st, FILETIME* ft) {
  if (!st || !ft) return 0;
  struct tm t = {};
  t.tm_year = st->wYear - 1900; t.tm_mon = st->wMonth - 1; t.tm_mday = st->wDay;
  t.tm_hour = st->wHour; t.tm_min = st->wMinute; t.tm_sec = st->wSecond;
  time_t uts = timegm(&t);
  if (uts == (time_t)-1) return 0;
  uint64_t wts = (uint64_t)uts * 10000000ULL + 116444736000000000ULL;
  ft->dwLowDateTime = (DWORD)(wts & 0xFFFFFFFF);
  ft->dwHighDateTime = (DWORD)(wts >> 32);
  return 1;
}
inline int FileTimeToSystemTime(const FILETIME* ft, SYSTEMTIME* st) {
  if (!ft || !st) return 0;
  uint64_t wts = ((uint64_t)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
  if (wts < 116444736000000000ULL) return 0;
  time_t uts = (time_t)((wts - 116444736000000000ULL) / 10000000ULL);
  struct tm t = {};
  gmtime_r(&uts, &t);
  st->wYear = (WORD)(t.tm_year + 1900); st->wMonth = (WORD)(t.tm_mon + 1);
  st->wDay = (WORD)t.tm_mday; st->wHour = (WORD)t.tm_hour;
  st->wMinute = (WORD)t.tm_min; st->wSecond = (WORD)t.tm_sec;
  st->wMilliseconds = 0; st->wDayOfWeek = (WORD)t.tm_wday;
  return 1;
}
inline int CompareFileTime(const FILETIME* a, const FILETIME* b) {
  if (!a || !b) return 0;
  uint64_t av = ((uint64_t)a->dwHighDateTime << 32) | a->dwLowDateTime;
  uint64_t bv = ((uint64_t)b->dwHighDateTime << 32) | b->dwLowDateTime;
  if (av < bv) return -1;
  if (av > bv) return 1;
  return 0;
}
inline DWORD GetLastError() { return (DWORD)errno; }
#ifndef FORMAT_MESSAGE_FROM_SYSTEM
#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000
#endif
#ifndef FORMAT_MESSAGE_IGNORE_INSERTS
#define FORMAT_MESSAGE_IGNORE_INSERTS 0x00000200
#endif
inline DWORD FormatMessageW(DWORD, const void*, DWORD, DWORD, wchar_t* buf, DWORD size, void*) {
  if (!buf || size == 0) return 0;
  const char* m = strerror(errno);
  char tmp[1024];
  snprintf(tmp, sizeof(tmp), "%s (%d)", m ? m : "error", errno);
  size_t n = mbstowcs(buf, tmp, size - 1);
  if (n == (size_t)-1) { buf[0] = 0; return 0; }
  buf[n] = 0;
  return (DWORD)n;
}
inline DWORD FormatMessage(DWORD f, const void* s, DWORD id, DWORD lang, wchar_t* buf, DWORD size, void* a) {
  return FormatMessageW(f, s, id, lang, buf, size, a);
}
struct GUID {
  uint32_t Data1 = 0;
  uint16_t Data2 = 0, Data3 = 0;
  uint8_t Data4[8] = {0};
};
inline HRESULT CoCreateGuid(GUID* g) {
  if (!g) return E_FAIL;
  static thread_local std::mt19937_64 rng{std::random_device{}()};
  uint64_t hi = rng(), lo = rng();
  g->Data1 = (uint32_t)(hi >> 32);
  g->Data2 = (uint16_t)(hi >> 16);
  g->Data3 = (uint16_t)hi;
  for (int i = 0; i < 8; ++i) g->Data4[i] = (uint8_t)(lo >> (i * 8));
  g->Data4[0] = (uint8_t)((g->Data4[0] & 0x3F) | 0x80);
  g->Data3 = (uint16_t)((g->Data3 & 0x0FFF) | 0x4000);
  return S_OK;
}
inline int StringFromGUID2(const GUID& g, wchar_t* out, int len) {
  if (!out || len < 39) return 0;
  swprintf(out, 8192, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
    g.Data1, g.Data2, g.Data3,
    g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
    g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
  return 39;
}
#ifndef CP_ACP
#define CP_ACP 0
#endif
#ifndef CP_UTF8
#define CP_UTF8 65001
#endif
#ifndef MB_PRECOMPOSED
#define MB_PRECOMPOSED 0x00000001
#endif
#ifndef WC_ERR_INVALID_CHARS
#define WC_ERR_INVALID_CHARS 0x00000080
#endif
inline int MultiByteToWideChar(unsigned, unsigned long, const char* src, int srclen, wchar_t* dst, int dstlen) {
  if (!src) return 0;
  size_t inlen = (srclen < 0) ? strlen(src) : (size_t)srclen;
  // Query required size (Windows two-call pattern: dst==NULL → return count incl. null).
  if (!dst || dstlen <= 0) {
    char tmp[8192];
    if (inlen >= sizeof(tmp)) inlen = sizeof(tmp) - 1;
    memcpy(tmp, src, inlen); tmp[inlen] = 0;
    // mbstowcs(NULL,0) returns required length (excluding null); +1 for null.
    size_t need = mbstowcs(nullptr, tmp, 0);
    if (need == (size_t)-1) return 0;
    return (int)(need + 1);
  }
  char tmp[8192];
  if (inlen >= sizeof(tmp)) inlen = sizeof(tmp) - 1;
  memcpy(tmp, src, inlen); tmp[inlen] = 0;
  size_t r = mbstowcs(dst, tmp, (size_t)(dstlen - 1));
  if (r == (size_t)-1) { dst[0] = 0; return 0; }
  dst[r] = 0;
  return (int)(r + 1);
}
inline int WideCharToMultiByte(unsigned, unsigned long, const wchar_t* src, int srclen, char* dst, int dstlen, const char*, int*) {
  if (!src) return 0;
  size_t inlen = (srclen < 0) ? wcslen(src) : (size_t)srclen;
  if (!dst || dstlen <= 0) {
    wchar_t tmp[4096];
    if (inlen >= 4096) inlen = 4095;
    wcsncpy(tmp, src, inlen); tmp[inlen] = 0;
    size_t need = wcstombs(nullptr, tmp, 0);
    if (need == (size_t)-1) return 0;
    return (int)(need + 1);
  }
  wchar_t tmp[4096];
  if (inlen >= 4096) inlen = 4095;
  wcsncpy(tmp, src, inlen); tmp[inlen] = 0;
  size_t r = wcstombs(dst, tmp, (size_t)(dstlen - 1));
  if (r == (size_t)-1) { dst[0] = 0; return 0; }
  dst[r] = 0;
  return (int)(r + 1);
}
#endif
