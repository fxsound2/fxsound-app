/*
FxSound Linux port — linux/message_window_posix.h (Phase 8)
Hoists FxController hidden message-window case bodies (WM_HOTKEY, PBT_APM*,
WTS*) into methods; Linux gets no-op bodies. Suspend/resume: no logind watcher
unless Phase 11 shows streams don't recover on their own.
Single-instance: moreThanOneInstanceAllowed()=false + anotherInstanceStarted()
  already forwards --power/preset/raise via JUCE on Linux; never create second sink.
Untested.
*/
#pragma once

// Win32 hotkey modifier flags (winuser.h) — FxController settings store packed
// mod<<16|vk values, so Linux keeps the same encoding even though global
// hotkeys are disabled (SysInfo::canSupportHotkeys() == false on Linux).
#ifndef MOD_ALT
#define MOD_ALT 0x0001
#define MOD_CONTROL 0x0002
#define MOD_SHIFT 0x0004
#endif

namespace fxsound_linux {
inline void onHotkey(int) {}
inline void onPowerSuspend() {}
inline void onPowerResume() {}
inline void onSessionChange(int) {}
}  // namespace fxsound_linux
