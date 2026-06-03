/*
FxSound — Linux build compatibility shim

Provides the minimal set of Win32 *type* aliases so that headers and member
declarations carrying Windows types (HWND, DWORD, LRESULT, ...) parse on Linux.

This header deliberately provides TYPES ONLY (plus a couple of constants used in
class-scope constexpr initializers). It does NOT fake the Win32 *functions*
(RegisterHotKey, Shell_NotifyIcon, CoInitializeEx, MiniDumpWriteDump, ...).
Every call site for those is guarded with `#ifdef _WIN32`; the Linux side gets a
proper implementation in later milestones:
  - global hotkeys / session notifications -> M5 (FxController)
  - system tray (StatusNotifierItem)        -> M5 (FxSystemTrayView)
  - crash handler (signal/backtrace)         -> M5 (Main)
*/

#ifndef FXSOUND_WIN_COMPAT_H
#define FXSOUND_WIN_COMPAT_H

#if !defined(_WIN32)

#include <cstdint>
#include <cwchar>

// Integer/handle aliases ----------------------------------------------------
using BYTE      = unsigned char;
using WORD      = unsigned short;
using DWORD     = uint32_t;
using DWORD64   = uint64_t;
using UINT      = unsigned int;
using BOOL      = int;
using LONG      = long;
using ULONG     = unsigned long;
using LONG_PTR  = intptr_t;
using UINT_PTR  = uintptr_t;
using WPARAM    = uintptr_t;
using LPARAM    = intptr_t;
using LRESULT   = intptr_t;
using HRESULT   = long;
using ATOM      = unsigned short;

// Opaque handles ------------------------------------------------------------
using HANDLE    = void*;
using HWND      = void*;
using HMODULE   = void*;
using HINSTANCE = void*;
using HICON     = void*;
using HKL       = void*;

using LPCWSTR   = const wchar_t*;
using LPWSTR    = wchar_t*;

// COM device interface placeholder (real device handles come from PipeWire).
using IMMDevice = void;

#ifndef CALLBACK
#define CALLBACK
#endif
#ifndef WINAPI
#define WINAPI
#endif

// Win32 window procedure type
using WNDPROC = LRESULT (CALLBACK*)(HWND, UINT, WPARAM, LPARAM);

// 128-bit GUID (layout-compatible with Win32 GUID) --------------------------
#ifndef GUID_DEFINED
#define GUID_DEFINED
struct GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
};
#endif

// Constants referenced at class/constexpr scope (not inside _WIN32 guards) ---
#ifndef WM_APP
#define WM_APP 0x8000
#endif

// ---------------------------------------------------------------------------
// Dormant global-hotkey API shims.
//
// The hotkey *settings* logic (validation, persistence) in FxController is
// platform-neutral and we keep it compiling/working. Only the OS-level
// registration is a no-op on Linux until M5 wires X11/portal global hotkeys.
// These are intentionally trivial leaf shims, not a real Win32 emulation.
// ---------------------------------------------------------------------------
#ifndef MOD_ALT
#define MOD_ALT     0x0001
#define MOD_CONTROL 0x0002
#define MOD_SHIFT   0x0004
#define MOD_WIN     0x0008
#endif
#ifndef VK_SHIFT
#define VK_SHIFT    0x10
#define VK_CONTROL  0x11
#define VK_MENU     0x12
#endif

inline BOOL RegisterHotKey(HWND, int, UINT, UINT)   { return 0; }
inline BOOL UnregisterHotKey(HWND, int)             { return 0; }
inline HKL  GetKeyboardLayout(DWORD)                { return nullptr; }
inline int  ToUnicodeEx(UINT, UINT, const BYTE*, wchar_t*, int, UINT, HKL) { return 0; }

#endif // !_WIN32
#endif // FXSOUND_WIN_COMPAT_H
