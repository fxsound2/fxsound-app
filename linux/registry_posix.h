/*
FxSound Linux port — linux/registry_posix.h (Phase 5)
Replaces reg/... + sndDevicesReg.cpp + DfxDspRegistry.cpp on Linux.
Settings file: ~/.config/FxSound/linux_registry.ini
dfx_guid becomes sink node.name (fxsound_audio_enhancer).
Startup-only keys return logged compile-time defaults.
Untested on Linux.
*/
#pragma once
#include <cstdint>
#include <string>

namespace fxsound_linux {
std::string configDir();  // ~/.config/FxSound (creates)
std::string registryPath();

// Narrow interface mirroring legacy reg helpers (string + dword, with/without keyname).
bool regReadString(const std::wstring& subkey, const std::wstring& value, std::wstring& out);
bool regWriteString(const std::wstring& subkey, const std::wstring& value, const std::wstring& data);
bool regReadDword(const std::wstring& subkey, const std::wstring& value, uint32_t& out);
bool regWriteDword(const std::wstring& subkey, const std::wstring& value, uint32_t data);

// sndDevices keys (HKCU devices/*, HKLM dfx_guid/default_buffer_size)
std::wstring readDfxGuid();            // persisted node.name, default L"fxsound_audio_enhancer"
uint32_t readDefaultBufferSizeMs();    // default 60
uint32_t readUserBufferSizeMs(uint32_t fallback);
void writeUserBufferSizeMs(uint32_t ms);
std::wstring readUserSelectedPlayback();  // persisted node.name or empty
void writeUserSelectedPlayback(const std::wstring& nodeName);
}  // namespace fxsound_linux
