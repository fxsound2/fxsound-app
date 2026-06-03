/* FxSound — Linux build: minimal stand-in for Windows <Shlwapi.h> path helpers
   used by the shared file utilities. */
#ifndef FXSOUND_DSP_FAKE_SHLWAPI_H
#define FXSOUND_DSP_FAKE_SHLWAPI_H

#include <windows.h>
#include <sys/stat.h>

static inline BOOL PathFileExistsW(LPCWSTR path)
{
    if (!path) return FALSE;
    char cpath[4096];
    wcstombs(cpath, path, sizeof(cpath));
    struct stat st;
    return stat(cpath, &st) == 0 ? TRUE : FALSE;
}
static inline BOOL PathFileExistsA(LPCSTR path)
{
    if (!path) return FALSE;
    struct stat st;
    return stat(path, &st) == 0 ? TRUE : FALSE;
}
#ifdef UNICODE
#define PathFileExists PathFileExistsW
#else
#define PathFileExists PathFileExistsA
#endif

#endif
