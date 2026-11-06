/*
FxSound Linux port — dsp/include/ptLong.h (Phase 6 fixed-width long pass)

The legacy Power Technology DSP code was written when the host `long` was a
32-bit word (Windows LLP64, and 32-bit builds generally). It uses `long` as:
  - the DSP word / state-slot type (Comsftwr.c passes `long *` data and state,
    and the per-algorithm `fpp`/`lpp` pointer-aliasing in the ptechDsp
    algorithms stores 8-byte pointers across pairs of 4-byte `long` slots),
  - the parameter-block word type for .vals/.fac structures (valsCfgHdlType,
    dfxpHdlType, comSftwr *WriteParam/ReadParam APIs),
  - the sample-word and meter types (pt_defs.h DspState).
On Linux LP64 `long` is 64-bit, which corrupts every one of those layouts
(misaligned `volatile long` stores into float banks in dly8p32.c, shifted
param-block offsets, and mono-masks DSP state). This header aliases the
DSP-side `long` to a fixed 32-bit width.

Windows keeps byte-identical semantics (typedef long == native long), so the
Windows .vcxproj build is unaffected. `long long` is NOT aliased (it is
platform-identical in both LLP64 and LP64 and stays 64-bit) — the token pass
that produced these edits explicitly preserves `long long`.

Scope: DfxDsp + AudioPassthruCore compiled boundary only (see
docs/linux-port/DECISIONS.md 2026-10-16 session-2 entries). PipeWire code
(linux/..._pw, AudioPassthruPW) and compat/ are excluded by design.

Cross-boundary API parity: AudioPassthruCore (MTH/pstr/slout/mry/ptime),
linux/registry_posix.*, linux/dsp_registry_posix.*, linux/file_reg_posix.*
implement functions declared in headers included by DfxDsp, so those files use
pt_long as well — declarations and definitions then agree on both platforms.
*/
#ifndef PT_LONG_H
#define PT_LONG_H

#include <stdint.h>

#if defined(_WIN32) || defined(_MSC_VER) || defined(__CYGWIN__)
typedef long           pt_long;
typedef unsigned long  pt_ulong;
#else
/* NOTE: pt_ulong is required because `unsigned` + int32_t does not compose
 * (int32_t may be a non-composable builtin in recent toolchains). */
typedef int32_t        pt_long;
typedef uint32_t       pt_ulong;
#endif

#endif /* PT_LONG_H */
