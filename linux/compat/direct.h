/*
FxSound Linux port — compat/direct.h stub (Phase 5)
Scoped to DfxDsp/ptutil only. Maps _getcwd/_mkdir to POSIX getcwd/mkdir.
Windows uses real SDK.
*/
#pragma once
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif
static inline char* _getcwd(char* buf, int size) { return getcwd(buf, (size_t)size); }
static inline int _mkdir(const char* path) { return mkdir(path, 0755); }
#ifdef __cplusplus
}
#endif
