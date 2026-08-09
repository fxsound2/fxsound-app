package fxsound

import "testing"

func TestSanitizePresetName(t *testing.T) {
	tests := map[string]string{
		"Bass Booster":        "Bass Booster",
		`Mu:sic`:              "Music",
		`a<b>c:d"e/f\g|h?i*j`: "abcdefghij",
	}
	for in, want := range tests {
		if got := SanitizePresetName(in); got != want {
			t.Errorf("SanitizePresetName(%q) = %q, want %q", in, got, want)
		}
	}

	long := ""
	for i := 0; i < 100; i++ {
		long += "a"
	}
	got := SanitizePresetName(long)
	if len(got) != maxPresetNameLength {
		t.Errorf("SanitizePresetName(100 chars) length = %d, want %d", len(got), maxPresetNameLength)
	}
}

func testStatus() *Status {
	return &Status{
		Power:          true,
		SelectedPreset: "Music",
		Presets: Presets{
			BuiltIn: []PresetInfo{
				{Name: "Music", Modified: false},
				{Name: "Gaming", Modified: true},
			},
			UserDefined: []PresetInfo{
				{Name: "My Preset", Modified: true},
			},
		},
	}
}

func TestValidatePresetName(t *testing.T) {
	status := testStatus()

	if _, err := ValidatePresetName("New Preset", status); err != nil {
		t.Errorf("unique name: unexpected error: %v", err)
	}

	// Collides after sanitizing: stripping ':' from "Mu:sic" produces "Music".
	if _, err := ValidatePresetName("Mu:sic", status); err == nil {
		t.Error(`"Mu:sic" colliding with "Music" after sanitizing: expected error, got nil`)
	}

	// Case-insensitive collision against an existing user preset.
	if _, err := ValidatePresetName("MY PRESET", status); err == nil {
		t.Error(`"MY PRESET" colliding with "My Preset": expected error, got nil`)
	}

	// Empty after stripping reserved characters.
	if _, err := ValidatePresetName(`<>:"/\|?*`, status); err == nil {
		t.Error("all-reserved-characters name: expected error, got nil")
	}
}

func TestSelectedPresetInfo(t *testing.T) {
	status := testStatus()
	info, isBuiltIn, found := SelectedPresetInfo(status)
	if !found {
		t.Fatal("expected to find selected preset")
	}
	if !isBuiltIn {
		t.Error("Music: expected isBuiltIn = true")
	}
	if info.Modified {
		t.Error("Music: expected Modified = false")
	}

	status.SelectedPreset = "My Preset"
	info, isBuiltIn, found = SelectedPresetInfo(status)
	if !found {
		t.Fatal("expected to find selected preset")
	}
	if isBuiltIn {
		t.Error("My Preset: expected isBuiltIn = false")
	}
	if !info.Modified {
		t.Error("My Preset: expected Modified = true")
	}
}

func TestValidatePresetPreconditions(t *testing.T) {
	t.Run("save requires power on", func(t *testing.T) {
		status := testStatus()
		status.Power = false
		if err := ValidateSavePreset(status); err == nil {
			t.Error("expected error when power is off")
		}
	})

	t.Run("save requires modified", func(t *testing.T) {
		status := testStatus() // selected preset "Music" is not modified
		if err := ValidateSavePreset(status); err == nil {
			t.Error("expected error when current preset is not modified")
		}
	})

	t.Run("save succeeds when modified", func(t *testing.T) {
		status := testStatus()
		status.SelectedPreset = "Gaming" // modified: true
		if err := ValidateSavePreset(status); err != nil {
			t.Errorf("unexpected error: %v", err)
		}
	})

	t.Run("overwrite rejects built-in", func(t *testing.T) {
		status := testStatus()
		status.SelectedPreset = "Gaming" // built-in, modified
		if err := ValidateOverwritePreset(status); err == nil {
			t.Error("expected error overwriting a built-in preset")
		}
	})

	t.Run("overwrite succeeds for modified user preset", func(t *testing.T) {
		status := testStatus()
		status.SelectedPreset = "My Preset" // user, modified
		if err := ValidateOverwritePreset(status); err != nil {
			t.Errorf("unexpected error: %v", err)
		}
	})

	t.Run("undo requires modified", func(t *testing.T) {
		status := testStatus() // "Music", not modified
		if err := ValidateUndoPreset(status); err == nil {
			t.Error("expected error when nothing to undo")
		}
	})

	t.Run("rename rejects built-in", func(t *testing.T) {
		status := testStatus()
		status.SelectedPreset = "Music" // built-in
		if err := ValidateRenamePreset(status); err == nil {
			t.Error("expected error renaming a built-in preset")
		}
	})

	t.Run("rename rejects modified", func(t *testing.T) {
		status := testStatus()
		status.SelectedPreset = "My Preset" // user, but modified: true
		if err := ValidateRenamePreset(status); err == nil {
			t.Error("expected error renaming a preset with unsaved changes")
		}
	})

	t.Run("delete rejects built-in", func(t *testing.T) {
		status := testStatus()
		status.SelectedPreset = "Music"
		if err := ValidateDeletePreset(status); err == nil {
			t.Error("expected error deleting a built-in preset")
		}
	})

	t.Run("delete succeeds for user preset", func(t *testing.T) {
		status := testStatus()
		status.SelectedPreset = "My Preset"
		if err := ValidateDeletePreset(status); err != nil {
			t.Errorf("unexpected error: %v", err)
		}
	})
}
