package main

import (
	"context"
	"testing"
	"time"

	"fxmcp/internal/fxsound"
)

// TestArgumentInjectionRejected confirms an embedded-double-quote payload
// -- which could otherwise splice extra --flag=value tokens into the
// literal command line sent to FxSound.exe (FxController's tokenizer has
// no backslash-escape awareness, see process.go's buildRawCmdLine) -- is
// rejected by the real running server, and that FxSound's actual state is
// left untouched by the attempt. Skips if FxSound isn't running.
func TestArgumentInjectionRejected(t *testing.T) {
	running, err := fxsound.IsFxSoundRunning()
	if err != nil {
		t.Fatalf("IsFxSoundRunning: %v", err)
	}
	if !running {
		t.Skip("FxSound.exe is not running")
	}
	paths, err := fxsound.Locate()
	if err != nil {
		t.Fatalf("Locate: %v", err)
	}

	before, err := fxsound.ReadStatus(context.Background(), paths)
	if err != nil {
		t.Fatalf("read baseline status: %v", err)
	}

	session, ctx, cleanup := connectTestSession(t, 30*time.Second)
	defer cleanup()

	// The payload: if it broke out of its quoted segment, this would turn
	// into "--preset=\"Music\" --power=0 --preset=\"Music\"" and actually
	// turn power off as a side effect of merely selecting a preset.
	payload := `Music" --power=0 --preset="Music`

	callToolExpectError(t, session, ctx, "fxsound_select_preset", map[string]any{"name": payload})
	callToolExpectError(t, session, ctx, "fxsound_set_output_device", map[string]any{"device_name": payload})
	callToolExpectError(t, session, ctx, "fxsound_apply_settings", map[string]any{"language": `en" --power=0`})

	after := freshStatus(t, ctx, paths)
	if after.Power != before.Power {
		t.Errorf("power changed from %v to %v -- an injection attempt had a side effect", before.Power, after.Power)
	}
	if after.SelectedPreset != before.SelectedPreset {
		t.Errorf("selected_preset changed from %q to %q -- an injection attempt had a side effect", before.SelectedPreset, after.SelectedPreset)
	}
}
