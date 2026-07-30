package main

import (
	"context"
	"encoding/json"
	"fmt"
	"testing"
	"time"

	"github.com/modelcontextprotocol/go-sdk/mcp"

	"fxmcp/internal/fxsound"
)

// TestWriteToolsLifecycle exercises every write tool from Iterations 5-7
// against the real, running FxSound instance, verifying status.json
// reflects each change. It restores the app's original power, preset,
// output device, equalizer, and effect settings afterward, best-effort.
// It skips entirely if FxSound isn't running.
func TestWriteToolsLifecycle(t *testing.T) {
	running, err := fxsound.IsFxSoundRunning()
	if err != nil {
		t.Fatalf("IsFxSoundRunning: %v", err)
	}
	if !running {
		t.Skip("FxSound.exe is not running; write-tool tests need a live instance to verify against")
	}

	paths, err := fxsound.Locate()
	if err != nil {
		t.Fatalf("Locate: %v", err)
	}

	baselineCtx, baselineCancel := context.WithTimeout(context.Background(), 10*time.Second)
	baseline, err := fxsound.ReadStatus(baselineCtx, paths)
	baselineCancel()
	if err != nil {
		t.Fatalf("read baseline status: %v", err)
	}
	t.Logf("baseline: power=%v preset=%q output=%q num_bands=%d",
		baseline.Power, baseline.SelectedPreset, baseline.SelectedOutput, baseline.Equalizer.NumBands)

	session, ctx, cleanup := connectTestSession(t, 90*time.Second)
	defer cleanup()

	defer restoreBaseline(t, paths, baseline)

	t.Run("set_power", func(t *testing.T) { testSetPower(t, session, ctx, paths, baseline) })
	t.Run("select_preset", func(t *testing.T) { testSelectPreset(t, session, ctx, paths, baseline) })
	t.Run("set_output_device", func(t *testing.T) { testSetOutputDevice(t, session, ctx, paths, baseline) })
	t.Run("preset_lifecycle", func(t *testing.T) { testPresetLifecycle(t, session, ctx, paths, baseline) })
	t.Run("eq_bands_and_gains", func(t *testing.T) { testEqBandsAndGains(t, session, ctx, paths, baseline) })
	t.Run("effects", func(t *testing.T) { testEffects(t, session, ctx, paths, baseline) })
	t.Run("composite_multi_field", func(t *testing.T) { testCompositeMultiField(t, session, ctx, paths, baseline) })
	t.Run("composite_mutual_exclusivity", func(t *testing.T) { testCompositeMutualExclusivity(t, session, ctx) })
	t.Run("composite_num_bands_cycle", func(t *testing.T) { testCompositeNumBandsCycle(t, session, ctx, paths, baseline) })
}

type applyResultDTO struct {
	Applied bool   `json:"applied"`
	Detail  string `json:"detail"`
}

func callToolOK(t *testing.T, session *mcp.ClientSession, ctx context.Context, name string, args map[string]any) applyResultDTO {
	t.Helper()
	res, err := session.CallTool(ctx, &mcp.CallToolParams{Name: name, Arguments: args})
	if err != nil {
		t.Fatalf("CallTool(%s): %v", name, err)
	}
	if res.IsError {
		t.Fatalf("CallTool(%s) returned an error result: %s", name, textContent(t, res))
	}
	var out applyResultDTO
	if err := json.Unmarshal([]byte(textContent(t, res)), &out); err != nil {
		t.Fatalf("CallTool(%s): unmarshal result: %v", name, err)
	}
	if !out.Applied {
		t.Fatalf("CallTool(%s): applied = false", name)
	}
	return out
}

func callToolExpectError(t *testing.T, session *mcp.ClientSession, ctx context.Context, name string, args map[string]any) {
	t.Helper()
	res, err := session.CallTool(ctx, &mcp.CallToolParams{Name: name, Arguments: args})
	if err != nil {
		// A transport-level error also demonstrates the call didn't succeed.
		return
	}
	if !res.IsError {
		t.Fatalf("CallTool(%s): expected an error result, got success: %s", name, textContent(t, res))
	}
	t.Logf("CallTool(%s) correctly returned error: %s", name, textContent(t, res))
}

func freshStatus(t *testing.T, ctx context.Context, paths *fxsound.Paths) *fxsound.Status {
	t.Helper()
	status, err := fxsound.ReadStatus(ctx, paths)
	if err != nil {
		t.Fatalf("ReadStatus: %v", err)
	}
	return status
}

func testSetPower(t *testing.T, session *mcp.ClientSession, ctx context.Context, paths *fxsound.Paths, baseline *fxsound.Status) {
	toggled := !baseline.Power
	callToolOK(t, session, ctx, "fxsound_set_power", map[string]any{"on": toggled})
	if got := freshStatus(t, ctx, paths).Power; got != toggled {
		t.Errorf("after set_power(%v): status.power = %v, want %v", toggled, got, toggled)
	}

	callToolOK(t, session, ctx, "fxsound_set_power", map[string]any{"on": baseline.Power})
	if got := freshStatus(t, ctx, paths).Power; got != baseline.Power {
		t.Errorf("after restoring power: status.power = %v, want %v", got, baseline.Power)
	}
}

func testSelectPreset(t *testing.T, session *mcp.ClientSession, ctx context.Context, paths *fxsound.Paths, baseline *fxsound.Status) {
	// Pick any built-in preset different from the current one.
	var target string
	for _, p := range baseline.Presets.BuiltIn {
		if p.Name != baseline.SelectedPreset {
			target = p.Name
			break
		}
	}
	if target == "" {
		t.Skip("no alternate built-in preset available to select")
	}

	callToolOK(t, session, ctx, "fxsound_select_preset", map[string]any{"name": target})
	if got := freshStatus(t, ctx, paths).SelectedPreset; got != target {
		t.Errorf("after select_preset(%q): status.selected_preset = %q", target, got)
	}

	callToolOK(t, session, ctx, "fxsound_select_preset", map[string]any{"name": baseline.SelectedPreset})
	if got := freshStatus(t, ctx, paths).SelectedPreset; got != baseline.SelectedPreset {
		t.Errorf("after restoring preset: status.selected_preset = %q, want %q", got, baseline.SelectedPreset)
	}
}

func testSetOutputDevice(t *testing.T, session *mcp.ClientSession, ctx context.Context, paths *fxsound.Paths, baseline *fxsound.Status) {
	if baseline.SelectedOutput == "" {
		t.Skip("no output device currently selected")
	}
	// Only one device is available on this machine in practice, so this
	// round-trips to the same device -- it still exercises the full tool
	// call path (build args, spawn, forward, status reflects the value).
	callToolOK(t, session, ctx, "fxsound_set_output_device", map[string]any{"device_name": baseline.SelectedOutput})
	if got := freshStatus(t, ctx, paths).SelectedOutput; got != baseline.SelectedOutput {
		t.Errorf("after set_output_device(%q): status.selected_output = %q", baseline.SelectedOutput, got)
	}
}

func testPresetLifecycle(t *testing.T, session *mcp.ClientSession, ctx context.Context, paths *fxsound.Paths, baseline *fxsound.Status) {
	// Find a modified built-in preset to save from; if none, select one and
	// nudge an effect value to mark it modified (setEffectValue marks the
	// preset modified as a side effect, same as the EQ setters).
	var dirtyPreset string
	for _, p := range baseline.Presets.BuiltIn {
		if p.Modified {
			dirtyPreset = p.Name
			break
		}
	}
	if dirtyPreset == "" {
		dirtyPreset = baseline.Presets.BuiltIn[0].Name
		callToolOK(t, session, ctx, "fxsound_select_preset", map[string]any{"name": dirtyPreset})
		callToolOK(t, session, ctx, "fxsound_set_effects", map[string]any{"effects": map[string]float64{"bass": 5.0}})
	} else {
		callToolOK(t, session, ctx, "fxsound_select_preset", map[string]any{"name": dirtyPreset})
	}
	if s := freshStatus(t, ctx, paths); s.SelectedPreset != dirtyPreset {
		t.Fatalf("expected selected preset %q, got %q", dirtyPreset, s.SelectedPreset)
	}

	testName := fmt.Sprintf("fxmcp_test_%d", time.Now().UnixNano()%1_000_000)

	callToolOK(t, session, ctx, "fxsound_save_preset", map[string]any{"name": testName})
	status := freshStatus(t, ctx, paths)
	if status.SelectedPreset != testName {
		t.Fatalf("after save_preset(%q): selected_preset = %q", testName, status.SelectedPreset)
	}
	if !presetExists(status.Presets.UserDefined, testName) {
		t.Fatalf("save_preset(%q): not found in user_defined presets", testName)
	}
	t.Logf("saved test preset %q", testName)

	renamedName := testName + "_renamed"
	callToolOK(t, session, ctx, "fxsound_rename_preset", map[string]any{"name": renamedName})
	status = freshStatus(t, ctx, paths)
	if status.SelectedPreset != renamedName {
		t.Fatalf("after rename_preset(%q): selected_preset = %q", renamedName, status.SelectedPreset)
	}
	t.Logf("renamed test preset to %q", renamedName)

	// Modify, then undo.
	callToolOK(t, session, ctx, "fxsound_set_effects", map[string]any{"effects": map[string]float64{"ambience": 8.0}})
	if s := freshStatus(t, ctx, paths); !presetModified(s, renamedName, false) {
		t.Fatalf("expected preset %q to be modified after set_effects", renamedName)
	}
	callToolOK(t, session, ctx, "fxsound_undo_preset", nil)
	status = freshStatus(t, ctx, paths)
	if presetModified(status, renamedName, false) {
		t.Errorf("expected preset %q to be unmodified after undo_preset", renamedName)
	}

	// Modify, then overwrite.
	callToolOK(t, session, ctx, "fxsound_set_effects", map[string]any{"effects": map[string]float64{"ambience": 8.0}})
	callToolOK(t, session, ctx, "fxsound_overwrite_preset", nil)
	status = freshStatus(t, ctx, paths)
	if presetModified(status, renamedName, false) {
		t.Errorf("expected preset %q to be unmodified after overwrite_preset", renamedName)
	}

	callToolOK(t, session, ctx, "fxsound_delete_preset", nil)
	status = freshStatus(t, ctx, paths)
	if presetExists(status.Presets.UserDefined, renamedName) {
		t.Errorf("delete_preset: %q still present in user_defined presets", renamedName)
	}
	t.Logf("deleted test preset %q", renamedName)
}

func presetExists(presets []fxsound.PresetInfo, name string) bool {
	for _, p := range presets {
		if p.Name == name {
			return true
		}
	}
	return false
}

func presetModified(status *fxsound.Status, name string, defaultVal bool) bool {
	all := append(append([]fxsound.PresetInfo{}, status.Presets.BuiltIn...), status.Presets.UserDefined...)
	for _, p := range all {
		if p.Name == name {
			return p.Modified
		}
	}
	return defaultVal
}

func testEqBandsAndGains(t *testing.T, session *mcp.ClientSession, ctx context.Context, paths *fxsound.Paths, baseline *fxsound.Status) {
	numBands := baseline.Equalizer.NumBands
	if numBands == 0 {
		t.Skip("no equalizer bands reported")
	}

	// Valid call: nudge band 0's gain, verify it landed, then restore.
	origGain := baseline.Equalizer.Bands[0].Gain
	newGain := origGain + 1
	if newGain > fxsound.MaxBandGainDb {
		newGain = origGain - 1
	}
	callToolOK(t, session, ctx, "fxsound_set_eq_band_gains", map[string]any{
		"bands": []map[string]any{{"index": 0, "gain_db": newGain}},
	})
	status := freshStatus(t, ctx, paths)
	if got := status.Equalizer.Bands[0].Gain; got != newGain {
		t.Errorf("after set_eq_band_gains(0, %v): band 0 gain = %v", newGain, got)
	}
	callToolOK(t, session, ctx, "fxsound_set_eq_band_gains", map[string]any{
		"bands": []map[string]any{{"index": 0, "gain_db": origGain}},
	})

	// Invalid calls: rejected before ever reaching the app.
	callToolExpectError(t, session, ctx, "fxsound_set_eq_band_gains", map[string]any{
		"bands": []map[string]any{{"index": numBands, "gain_db": 1.0}},
	})
	callToolExpectError(t, session, ctx, "fxsound_set_eq_band_gains", map[string]any{
		"bands": []map[string]any{{"index": 0, "gain_db": fxsound.MaxBandGainDb + 1}},
	})
	callToolExpectError(t, session, ctx, "fxsound_set_eq_bands", map[string]any{
		"bands": []map[string]any{{"index": numBands, "frequency_hz": 100.0}},
	})

	// Valid frequency nudge, restored afterward.
	origFreq := baseline.Equalizer.Bands[0].Frequency
	callToolOK(t, session, ctx, "fxsound_set_eq_bands", map[string]any{
		"bands": []map[string]any{{"index": 0, "frequency_hz": origFreq}},
	})
}

func testEffects(t *testing.T, session *mcp.ClientSession, ctx context.Context, paths *fxsound.Paths, baseline *fxsound.Status) {
	orig := baseline.Effects.Bass
	newVal := 6.0
	if newVal == orig {
		newVal = 7.0
	}
	callToolOK(t, session, ctx, "fxsound_set_effects", map[string]any{"effects": map[string]float64{"bass": newVal}})
	status := freshStatus(t, ctx, paths)
	if got := status.Effects.Bass; !floatNear(got, newVal, 0.01) {
		t.Errorf("after set_effects(bass=%v): effects.bass = %v", newVal, got)
	}

	// Alias + restore original value in one call.
	callToolOK(t, session, ctx, "fxsound_set_effects", map[string]any{"effects": map[string]float64{"bassboost": orig}})
	status = freshStatus(t, ctx, paths)
	if got := status.Effects.Bass; !floatNear(got, orig, 0.01) {
		t.Errorf("after restoring bass via alias: effects.bass = %v, want %v", got, orig)
	}

	callToolExpectError(t, session, ctx, "fxsound_set_effects", map[string]any{"effects": map[string]float64{"reverb": 5.0}})
	callToolExpectError(t, session, ctx, "fxsound_set_effects", map[string]any{"effects": map[string]float64{"bass": 11.0}})
}

func floatNear(a, b, tolerance float64) bool {
	d := a - b
	if d < 0 {
		d = -d
	}
	return d <= tolerance
}

func testCompositeMultiField(t *testing.T, session *mcp.ClientSession, ctx context.Context, paths *fxsound.Paths, baseline *fxsound.Status) {
	var altPreset string
	for _, p := range baseline.Presets.BuiltIn {
		if p.Name != baseline.SelectedPreset {
			altPreset = p.Name
			break
		}
	}
	if altPreset == "" || baseline.SelectedOutput == "" {
		t.Skip("need an alternate preset and a selected output device")
	}

	callToolOK(t, session, ctx, "fxsound_apply_settings", map[string]any{
		"preset":        altPreset,
		"output_device": baseline.SelectedOutput,
		"view":          "full",
	})
	status := freshStatus(t, ctx, paths)
	if status.SelectedPreset != altPreset {
		t.Errorf("composite call: selected_preset = %q, want %q", status.SelectedPreset, altPreset)
	}
	if status.SelectedOutput != baseline.SelectedOutput {
		t.Errorf("composite call: selected_output = %q, want %q", status.SelectedOutput, baseline.SelectedOutput)
	}

	// Restore preset.
	callToolOK(t, session, ctx, "fxsound_apply_settings", map[string]any{"preset": baseline.SelectedPreset})
}

func testCompositeMutualExclusivity(t *testing.T, session *mcp.ClientSession, ctx context.Context) {
	callToolExpectError(t, session, ctx, "fxsound_apply_settings", map[string]any{
		"undo_preset":   true,
		"delete_preset": true,
	})
}

func testCompositeNumBandsCycle(t *testing.T, session *mcp.ClientSession, ctx context.Context, paths *fxsound.Paths, baseline *fxsound.Status) {
	for _, n := range []int{5, 10, 15, 20, 31} {
		callToolOK(t, session, ctx, "fxsound_apply_settings", map[string]any{"num_eq_bands": n})
		if got := freshStatus(t, ctx, paths).Equalizer.NumBands; got != n {
			t.Errorf("after apply_settings(num_eq_bands=%d): status num_bands = %d", n, got)
		}
	}
	callToolOK(t, session, ctx, "fxsound_apply_settings", map[string]any{"num_eq_bands": baseline.Equalizer.NumBands})
}

// restoreBaseline puts power, selected preset, and output device back to
// their pre-test values, best-effort (logs rather than fails on error,
// since this runs during cleanup after the real assertions already ran).
func restoreBaseline(t *testing.T, paths *fxsound.Paths, baseline *fxsound.Status) {
	ctx, cancel := context.WithTimeout(context.Background(), 20*time.Second)
	defer cancel()

	flags := map[string]string{
		"power":     boolFlagStr(baseline.Power),
		"preset":    baseline.SelectedPreset,
		"num_bands": fmt.Sprintf("%d", baseline.Equalizer.NumBands),
	}
	if baseline.SelectedOutput != "" {
		flags["output"] = baseline.SelectedOutput
	}
	if err := fxsound.Apply(ctx, paths, flags, false); err != nil {
		t.Logf("restoreBaseline: %v", err)
	}
}

func boolFlagStr(b bool) string {
	if b {
		return "1"
	}
	return "0"
}
