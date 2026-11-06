/*
FxSound Linux port — linux/dsp_registry_posix.h (Phase 5)
Backs DfxDspRegistry.cpp keys read/written after startup with settings file.
Startup-only keys use logged compile-time defaults (see RECON §4).
Untested.
*/
#pragma once
#include <cstdint>
#include <string>

namespace fxsound_linux {
// LASTUSED_DFXP session values (EQ on, valFidelity..., mode...). Persisted.
bool dspSessionReadInt(const std::wstring& name, int& out);
void dspSessionWriteInt(const std::wstring& name, int v);
bool dspSessionReadReal(const std::wstring& name, float& out);
void dspSessionWriteReal(const std::wstring& name, float v);
// Startup-only (install paths, date_installed): logged defaults, not persisted.
std::wstring dspDefaultSharedFolder();
}  // namespace fxsound_linux
