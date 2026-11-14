/*
FxSound — Linux build

Shim standing in for <windows.h> when building the DSP on Linux. The DSP sources
include <windows.h> for a handful of integer/handle typedefs and macros; this
provides just those. It is found ahead of any system header because dsp/linux is
placed first on the DSP include path. Real Win32 APIs are not emulated — any
call site that needs one is ported behind a platform guard.
*/

#ifndef FXSOUND_DSP_FAKE_WINDOWS_H
#define FXSOUND_DSP_FAKE_WINDOWS_H

#include <stdint.h>   /* C-compatible (DSP has .c translation units) */
#include <stddef.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>

typedef unsigned char      BYTE;
typedef unsigned short     WORD;
typedef uint32_t           DWORD;
typedef int                BOOL;
typedef unsigned int       UINT;
typedef int32_t               LONG;
typedef uint32_t      ULONG;
typedef intptr_t           LONG_PTR;
typedef uintptr_t          ULONG_PTR;
typedef uintptr_t          UINT_PTR;
typedef uintptr_t          WPARAM;
typedef intptr_t           LPARAM;
typedef intptr_t           LRESULT;
typedef int64_t            LONGLONG;
typedef uint64_t           ULONGLONG;

/* MSVC fixed-width keyword. Macro so "unsigned __int64" also rewrites. */
#ifndef __int64
#define __int64 long long
#endif
#ifndef __int32
#define __int32 int
#endif

typedef void*              HANDLE;
typedef void*              HWND;
typedef void*              HINSTANCE;
typedef void*              HMODULE;
typedef void*              HKEY;
typedef void*              HGLOBAL;
typedef void*              LPVOID;
typedef const void*        LPCVOID;

typedef char               CHAR;
typedef wchar_t            WCHAR;
typedef char*              LPSTR;
typedef const char*        LPCSTR;
typedef wchar_t*           LPWSTR;
typedef const wchar_t*     LPCWSTR;
typedef wchar_t            TCHAR;
typedef const wchar_t*     LPCTSTR;
typedef wchar_t*           LPTSTR;
typedef void*              LPSECURITY_ATTRIBUTES;

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
} GUID;
#endif

typedef struct _SYSTEMTIME {
    WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

/* Message boxes are no-ops on Linux (DSP only uses them on error paths). */
#ifndef MB_OK
#define MB_OK 0
#endif
#define MessageBox(hwnd, text, caption, type)  ((int)0)
#define MessageBoxW(hwnd, text, caption, type) ((int)0)
#define MessageBoxA(hwnd, text, caption, type) ((int)0)

#ifndef WINAPI
#define WINAPI
#endif
#ifndef CALLBACK
#define CALLBACK
#endif
#ifndef __cdecl
#define __cdecl
#endif

/* Note: most DSP swprintf calls already use the ISO form
 *   swprintf(buf, count, fmt, ...)
 * and compile as-is on glibc. The few Windows non-conforming calls (no count)
 * are fixed at their call sites — a blanket macro can't tell the two forms
 * apart and would corrupt the count-bearing calls (shifting the format arg). */

/* Module-loading APIs: the Linux DSP is statically linked, so these are
   inert. Any code path that genuinely needs dynamic loading is ported. */
#define LoadLibrary(name)        ((HMODULE)0)
#define LoadLibraryW(name)       ((HMODULE)0)
#define LoadLibraryA(name)       ((HMODULE)0)
#define FreeLibrary(h)           (0)
#define GetProcAddress(h, name)  ((void*)0)
#define GetModuleHandle(name)    ((HMODULE)0)

/* MSVC secure wide fopen -> POSIX. Converts the wide path/mode to multibyte. */
static inline int _wfopen_s(FILE** pf, const wchar_t* name, const wchar_t* mode)
{
    char cname[1024], cmode[16];
    if (!pf) return 22; /* EINVAL */
    wcstombs(cname, name, sizeof(cname));
    wcstombs(cmode, mode, sizeof(cmode));
    *pf = fopen(cname, cmode);
    return *pf ? 0 : 2; /* ENOENT-ish */
}

/* MSVC string helpers used by the shared util sources. */
#define _wcsicmp(a, b)     wcscasecmp((a), (b))
#define _wcsnicmp(a, b, n) wcsncasecmp((a), (b), (n))
#define _stricmp(a, b)     strcasecmp((a), (b))
#define _strnicmp(a, b, n) strncasecmp((a), (b), (n))

/* Windows wide-string conversions used by the DSP. */
#define _wtoi(s) ((int)wcstol((s), NULL, 10))
#define _wtol(s) (wcstol((s), NULL, 10))
#define _wtof(s) (wcstod((s), NULL))
#define wcstok_s(s, d, c) wcstok((s), (d), (c))

/* ---- Codepage / wide-char conversion (UTF-8 backed) -------------------- */
#define CP_ACP        0
#define CP_UTF8       65001
#define MB_PRECOMPOSED 0x00000001
#define S_OK          0
#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200
#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100

/* UTF-8 is encoded/decoded explicitly (the code page argument is CP_UTF8 at
 * every call site). wcstombs/mbstowcs depend on the locale and fail for any
 * non-ASCII text in the default "C" locale. Win32 contract: srclen == -1 means
 * NUL-terminated and the NUL is counted; dstlen == 0 returns the size needed. */
static inline int MultiByteToWideChar(UINT cp, DWORD flags, LPCSTR src, int srclen,
                                      LPWSTR dst, int dstlen)
{
    const unsigned char* s = (const unsigned char*)src;
    size_t len, i = 0;
    int out = 0;
    (void)cp; (void)flags;
    if (!src) return 0;
    len = (srclen < 0) ? strlen(src) + 1 : (size_t)srclen;
    while (i < len) {
        uint32_t c = s[i];
        size_t extra = c < 0x80 ? 0 : c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
        size_t k;
        if (c >= 0x80 && extra == 0) { c = 0xFFFD; i += 1; }        /* stray continuation */
        else if (extra && i + extra >= len) { c = 0xFFFD; i = len; }   /* truncated */
        else {
            if (extra) c &= (0x3F >> extra);
            for (k = 1; k <= extra; ++k) {
                if ((s[i + k] & 0xC0) != 0x80) { c = 0xFFFD; extra = k - 1; break; }
                c = (c << 6) | (s[i + k] & 0x3F);
            }
            i += extra + 1;
        }
        if (dstlen > 0) {
            if (out >= dstlen) return 0;                             /* buffer too small */
            dst[out] = (wchar_t)c;
        }
        ++out;
    }
    return out;
}

static inline int WideCharToMultiByte(UINT cp, DWORD flags, LPCWSTR src, int srclen,
                                      LPSTR dst, int dstlen, const char* def, int* used_def)
{
    size_t len, i;
    int out = 0;
    (void)cp; (void)flags; (void)def;
    if (used_def) *used_def = 0;
    if (!src) return 0;
    len = (srclen < 0) ? wcslen(src) + 1 : (size_t)srclen;
    for (i = 0; i < len; ++i) {
        uint32_t c = (uint32_t)src[i];
        unsigned char b[4];
        int n, k;
        if (c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) c = 0xFFFD;
        if (c < 0x80)         { b[0] = (unsigned char)c; n = 1; }
        else if (c < 0x800)   { b[0] = (unsigned char)(0xC0 | (c >> 6)); b[1] = (unsigned char)(0x80 | (c & 0x3F)); n = 2; }
        else if (c < 0x10000) { b[0] = (unsigned char)(0xE0 | (c >> 12)); b[1] = (unsigned char)(0x80 | ((c >> 6) & 0x3F));
                                b[2] = (unsigned char)(0x80 | (c & 0x3F)); n = 3; }
        else                  { b[0] = (unsigned char)(0xF0 | (c >> 18)); b[1] = (unsigned char)(0x80 | ((c >> 12) & 0x3F));
                                b[2] = (unsigned char)(0x80 | ((c >> 6) & 0x3F)); b[3] = (unsigned char)(0x80 | (c & 0x3F)); n = 4; }
        if (dstlen > 0) {
            if (out + n > dstlen) return 0;                          /* buffer too small */
            for (k = 0; k < n; ++k) dst[out + k] = (char)b[k];
        }
        out += n;
    }
    return out;
}

/* ---- Misc Win32 stubs referenced by the shared util layer -------------- */
static inline DWORD GetLastError(void) { return 0; }
static inline DWORD FormatMessageW(DWORD, const void*, DWORD, DWORD,
                                   LPWSTR buf, DWORD, void*)
{ if (buf) buf[0] = L'\0'; return 0; }
#define FormatMessage FormatMessageW

static inline LONG CompareFileTime(const FILETIME* a, const FILETIME* b)
{
    ULONGLONG ua = ((ULONGLONG)a->dwHighDateTime << 32) | a->dwLowDateTime;
    ULONGLONG ub = ((ULONGLONG)b->dwHighDateTime << 32) | b->dwLowDateTime;
    return (ua < ub) ? -1 : (ua > ub) ? 1 : 0;
}

/* GUID generation: random v4 (sufficient for the DSP's identifier use). */
static inline LONG CoCreateGuid(GUID* g)
{
    if (!g) return -1;
    unsigned char* p = (unsigned char*)g;
    for (size_t i = 0; i < sizeof(GUID); ++i) p[i] = (unsigned char)(rand() & 0xff);
    g->Data3 = (uint16_t)((g->Data3 & 0x0fff) | 0x4000);
    g->Data4[0] = (uint8_t)((g->Data4[0] & 0x3f) | 0x80);
    return S_OK;
}
#ifdef __cplusplus
static inline int StringFromGUID2(const GUID& g, LPWSTR out, int cch)
{
    if (!out || cch < 39) return 0;
    /* (swprintf) parenthesized so the count-injecting macro does not expand. */
    (swprintf)(out, (size_t)cch,
        L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        g.Data1, g.Data2, g.Data3,
        g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
        g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
    return 39;
}
#endif // __cplusplus

#endif // FXSOUND_DSP_FAKE_WINDOWS_H
