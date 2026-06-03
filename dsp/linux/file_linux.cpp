/*
FxSound — Linux build

Linux implementation of the two "file" helpers the DSP references. The full
FileGeneral.cpp is built on Win32 file APIs (_wstat/_wsplitpath/CreateDirectory
/...) and is excluded; only these portable entry points are needed.
*/

#include "codedefs.h"
#include "File.h"

#include <cstdio>
#include <cstring>
#include <cwchar>
#include <sys/stat.h>

static void wideToUtf8(const wchar_t* w, char* out, size_t n)
{
    if (!w) { if (n) out[0] = '\0'; return; }
    size_t r = wcstombs(out, w, n);
    if (r == (size_t)-1) out[0] = '\0';
    else if (r < n) out[r] = '\0';
}

FILE* fileOpen_Wide(wchar_t* path, wchar_t* mode, CSlout*)
{
    char cpath[4096], cmode[16];
    wideToUtf8(path, cpath, sizeof(cpath));
    wideToUtf8(mode, cmode, sizeof(cmode));
    return std::fopen(cpath, cmode);
}

int fileExist_Wide(wchar_t* path, int* exists)
{
    char cpath[4096];
    wideToUtf8(path, cpath, sizeof(cpath));
    struct stat st;
    if (exists) *exists = (stat(cpath, &st) == 0) ? IS_TRUE : IS_FALSE;
    return OKAY;
}

/* Split a full path into directory (incl. trailing separator) and filename. */
int fileSplitFullpath_Wide(wchar_t* full_path, wchar_t* dir_out, wchar_t* name_out, CSlout*)
{
    if (!full_path) return NOT_OKAY;
    const wchar_t* sep = wcsrchr(full_path, L'/');
    if (!sep) sep = wcsrchr(full_path, L'\\');
    if (sep)
    {
        size_t dlen = (size_t)(sep - full_path) + 1;
        if (dir_out)  { wmemcpy(dir_out, full_path, dlen); dir_out[dlen] = L'\0'; }
        if (name_out) wcscpy(name_out, sep + 1);
    }
    else
    {
        if (dir_out)  dir_out[0] = L'\0';
        if (name_out) wcscpy(name_out, full_path);
    }
    return OKAY;
}

/* File last-modified time as a Win32 FILETIME (100ns ticks since 1601). */
int fileGetModifiedDate_Wide(wchar_t* path, FILETIME* ft, CSlout*)
{
    char cpath[4096];
    wideToUtf8(path, cpath, sizeof(cpath));
    struct stat st;
    if (!ft) return NOT_OKAY;
    if (stat(cpath, &st) != 0) { ft->dwLowDateTime = ft->dwHighDateTime = 0; return NOT_OKAY; }
    unsigned long long ticks = (unsigned long long)st.st_mtime * 10000000ULL + 116444736000000000ULL;
    ft->dwLowDateTime  = (DWORD)(ticks & 0xffffffffULL);
    ft->dwHighDateTime = (DWORD)(ticks >> 32);
    return OKAY;
}
