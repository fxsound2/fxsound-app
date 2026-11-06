/*
FxSound Linux port — compat/conio.h stub (Phase 5)
Scoped to DfxDsp/ptutil/AudioPassthruCore only. Provides no-op console I/O
for legacy <conio.h> includes (Comwave.cpp, Prelst.cpp). Windows uses real SDK.
*/
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
static inline int kbhit(void) { return 0; }
static inline int getch(void) { return 0; }
static inline int getche(void) { return 0; }
#ifdef __cplusplus
}
#endif
