//go:build windows

package fxsound

import (
	"fmt"
	"strings"
	"unsafe"

	"golang.org/x/sys/windows"
)

// IsProcessRunning reports whether a process named exeName (e.g.
// "FxSound.exe", matched case-insensitively) currently has a running
// instance, by walking a toolhelp32 snapshot of the system process list.
func IsProcessRunning(exeName string) (bool, error) {
	snapshot, err := windows.CreateToolhelp32Snapshot(windows.TH32CS_SNAPPROCESS, 0)
	if err != nil {
		return false, fmt.Errorf("snapshot process list: %w", err)
	}
	defer windows.CloseHandle(snapshot)

	var entry windows.ProcessEntry32
	entry.Size = uint32(unsafe.Sizeof(entry))
	if err := windows.Process32First(snapshot, &entry); err != nil {
		return false, fmt.Errorf("enumerate processes: %w", err)
	}
	for {
		name := windows.UTF16ToString(entry.ExeFile[:])
		if strings.EqualFold(name, exeName) {
			return true, nil
		}
		if err := windows.Process32Next(snapshot, &entry); err != nil {
			if err == windows.ERROR_NO_MORE_FILES {
				return false, nil
			}
			return false, fmt.Errorf("enumerate processes: %w", err)
		}
	}
}

// IsFxSoundRunning reports whether FxSound.exe is currently running.
//
// This gates any command that FxController.cpp's applyConfig only honors
// against an already-running instance (--status, --set_band_freq,
// --set_band_gain, --set_effect, and the preset-management commands): a
// cold-start invocation is parsed by the narrower initConfig instead, which
// silently ignores those flags.
func IsFxSoundRunning() (bool, error) {
	return IsProcessRunning("FxSound.exe")
}
