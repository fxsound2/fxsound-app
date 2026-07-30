package fxsound

import "strings"

const (
	presetNameReservedChars = `<>:"/\|?*`
	maxPresetNameLength     = 64
)

// SanitizePresetName mirrors the sanitizePresetName lambda in
// FxController.cpp's applyConfig: strip the filesystem-reserved
// characters `<>:"/\|?*`, then truncate to 64 characters. It does not
// perform the existing-name collision check -- see ValidatePresetName.
//
// This operates on runes, not UTF-16 code units like the C++ side's
// std::wstring truncation; for the ASCII-range names this is used for in
// practice, the result is identical.
func SanitizePresetName(name string) string {
	var b strings.Builder
	for _, r := range name {
		if strings.ContainsRune(presetNameReservedChars, r) {
			continue
		}
		b.WriteRune(r)
	}
	sanitized := []rune(b.String())
	if len(sanitized) > maxPresetNameLength {
		sanitized = sanitized[:maxPresetNameLength]
	}
	return string(sanitized)
}

// ValidatePresetName sanitizes name the same way --save_preset/
// --rename_preset do server-side, then checks the result isn't empty and
// doesn't collide (case-insensitively) with any existing preset name in
// status. FxController silently no-ops rather than erroring in both
// cases; this turns that into an explicit error before the value is sent.
func ValidatePresetName(name string, status *Status) (string, error) {
	sanitized := SanitizePresetName(name)
	if sanitized == "" {
		return "", newError(ErrKindValueRejected, nil, "preset name %q is empty after stripping reserved characters (< > : \" / \\ | ? *)", name)
	}
	for _, existing := range allPresetNames(status) {
		if strings.EqualFold(existing, sanitized) {
			return "", newError(ErrKindValueRejected, nil, "a preset named %q already exists (case-insensitive match against sanitized name %q)", existing, sanitized)
		}
	}
	return sanitized, nil
}

func allPresetNames(status *Status) []string {
	names := make([]string, 0, len(status.Presets.BuiltIn)+len(status.Presets.UserDefined))
	for _, p := range status.Presets.BuiltIn {
		names = append(names, p.Name)
	}
	for _, p := range status.Presets.UserDefined {
		names = append(names, p.Name)
	}
	return names
}

// SelectedPresetInfo finds the PresetInfo entry matching status's
// currently selected preset, and reports whether it's a built-in preset.
func SelectedPresetInfo(status *Status) (info PresetInfo, isBuiltIn bool, found bool) {
	for _, p := range status.Presets.BuiltIn {
		if p.Name == status.SelectedPreset {
			return p, true, true
		}
	}
	for _, p := range status.Presets.UserDefined {
		if p.Name == status.SelectedPreset {
			return p, false, true
		}
	}
	return PresetInfo{}, false, false
}

// The Validate*Preset functions below check the same preconditions
// FxController.cpp's applyConfig checks before acting on each preset
// command (see docs/COMMAND_LINE_OPTIONS.md's Preset commands section),
// returning a clear error instead of letting the command silently no-op.

// ValidateSavePreset checks the preconditions for --save_preset (name
// sanitizing/collision is checked separately via ValidatePresetName).
func ValidateSavePreset(status *Status) error {
	if !status.Power {
		return newError(ErrKindValueRejected, nil, "cannot save a preset while FxSound's power is off")
	}
	info, _, found := SelectedPresetInfo(status)
	if !found {
		return newError(ErrKindValueRejected, nil, "could not determine the currently selected preset's state")
	}
	if !info.Modified {
		return newError(ErrKindValueRejected, nil, "current preset %q has no unsaved changes to save", status.SelectedPreset)
	}
	return nil
}

// ValidateOverwritePreset checks the preconditions for --overwrite_preset.
func ValidateOverwritePreset(status *Status) error {
	if !status.Power {
		return newError(ErrKindValueRejected, nil, "cannot overwrite a preset while FxSound's power is off")
	}
	info, isBuiltIn, found := SelectedPresetInfo(status)
	if !found {
		return newError(ErrKindValueRejected, nil, "could not determine the currently selected preset's state")
	}
	if isBuiltIn {
		return newError(ErrKindValueRejected, nil, "cannot overwrite built-in preset %q", status.SelectedPreset)
	}
	if !info.Modified {
		return newError(ErrKindValueRejected, nil, "current preset %q has no unsaved changes to overwrite", status.SelectedPreset)
	}
	return nil
}

// ValidateUndoPreset checks the preconditions for --undo_preset.
func ValidateUndoPreset(status *Status) error {
	if !status.Power {
		return newError(ErrKindValueRejected, nil, "cannot undo preset changes while FxSound's power is off")
	}
	info, _, found := SelectedPresetInfo(status)
	if !found {
		return newError(ErrKindValueRejected, nil, "could not determine the currently selected preset's state")
	}
	if !info.Modified {
		return newError(ErrKindValueRejected, nil, "current preset %q has no unsaved changes to undo", status.SelectedPreset)
	}
	return nil
}

// ValidateRenamePreset checks the preconditions for --rename_preset (name
// sanitizing/collision is checked separately via ValidatePresetName).
func ValidateRenamePreset(status *Status) error {
	if !status.Power {
		return newError(ErrKindValueRejected, nil, "cannot rename a preset while FxSound's power is off")
	}
	info, isBuiltIn, found := SelectedPresetInfo(status)
	if !found {
		return newError(ErrKindValueRejected, nil, "could not determine the currently selected preset's state")
	}
	if isBuiltIn {
		return newError(ErrKindValueRejected, nil, "cannot rename built-in preset %q", status.SelectedPreset)
	}
	if info.Modified {
		return newError(ErrKindValueRejected, nil, "current preset %q has unsaved changes; save, overwrite, or undo them before renaming", status.SelectedPreset)
	}
	return nil
}

// ValidateDeletePreset checks the preconditions for --delete_preset.
func ValidateDeletePreset(status *Status) error {
	if !status.Power {
		return newError(ErrKindValueRejected, nil, "cannot delete a preset while FxSound's power is off")
	}
	_, isBuiltIn, found := SelectedPresetInfo(status)
	if !found {
		return newError(ErrKindValueRejected, nil, "could not determine the currently selected preset's state")
	}
	if isBuiltIn {
		return newError(ErrKindValueRejected, nil, "cannot delete built-in preset %q", status.SelectedPreset)
	}
	return nil
}
