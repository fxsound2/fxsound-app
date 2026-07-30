package main

import (
	"context"
	"encoding/json"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"testing"
	"time"

	"github.com/modelcontextprotocol/go-sdk/mcp"

	"fxmcp/internal/fxsound"
)

// TestServerInitializes builds the fxmcp binary and connects a real MCP
// client to it over stdio, verifying the initialize handshake succeeds.
// This is the harness later iterations extend to exercise tools/resources
// end-to-end without needing a real FxSound install for the parts that
// don't touch it.
func TestServerInitializes(t *testing.T) {
	_, _, cleanup := connectTestSession(t, 10*time.Second)
	defer cleanup()
}

// TestDiagnosticsResource reads fxsound://diagnostics against the real,
// installed fxdiag.exe on this machine and sanity-checks the shape of the
// result matches the documented schema.
func TestDiagnosticsResource(t *testing.T) {
	session, ctx, cleanup := connectTestSession(t, 15*time.Second)
	defer cleanup()

	res, err := session.ReadResource(ctx, &mcp.ReadResourceParams{URI: "fxsound://diagnostics"})
	if err != nil {
		t.Fatalf("ReadResource(fxsound://diagnostics): %v", err)
	}
	if len(res.Contents) != 1 {
		t.Fatalf("expected 1 content item, got %d", len(res.Contents))
	}

	var diag struct {
		Devices []struct {
			DeviceID      string `json:"deviceId"`
			DeviceName    string `json:"deviceName"`
			DeviceType    string `json:"deviceType"`
			State         string `json:"state"`
			Channels      int    `json:"channels"`
			SamplesPerSec int    `json:"samplesPerSec"`
			BitsPerSample int    `json:"bitsPerSample"`
			VolumeLevel   int    `json:"volumeLevel"`
		} `json:"devices"`
		Sessions []struct {
			SessionName   string `json:"sessionName"`
			State         string `json:"state"`
			SystemSession bool   `json:"systemSession"`
			VolumeLevel   int    `json:"volumeLevel"`
			Muted         bool   `json:"muted"`
		} `json:"sessions"`
	}
	if err := json.Unmarshal([]byte(res.Contents[0].Text), &diag); err != nil {
		t.Fatalf("unmarshal diagnostics: %v\nraw: %s", err, res.Contents[0].Text)
	}
	if len(diag.Devices) == 0 {
		t.Error("expected at least one device")
	}
	t.Logf("got %d devices, %d sessions", len(diag.Devices), len(diag.Sessions))
}

// TestIsRunningTool calls fxsound_is_running and checks it agrees with a
// direct process-list check for FxSound.exe.
func TestIsRunningTool(t *testing.T) {
	session, ctx, cleanup := connectTestSession(t, 10*time.Second)
	defer cleanup()

	want, err := fxsound.IsFxSoundRunning()
	if err != nil {
		t.Fatalf("IsFxSoundRunning: %v", err)
	}

	res, err := session.CallTool(ctx, &mcp.CallToolParams{Name: "fxsound_is_running"})
	if err != nil {
		t.Fatalf("CallTool(fxsound_is_running): %v", err)
	}
	if res.IsError {
		t.Fatalf("fxsound_is_running returned an error result: %+v", res.Content)
	}

	var out struct {
		Running bool `json:"running"`
	}
	if err := json.Unmarshal([]byte(textContent(t, res)), &out); err != nil {
		t.Fatalf("unmarshal result: %v", err)
	}
	if out.Running != want {
		t.Errorf("fxsound_is_running = %v, want %v (from direct process check)", out.Running, want)
	}
	t.Logf("FxSound.exe running: %v", out.Running)
}

// TestGetStatus exercises both fxsound://status and fxsound_get_status
// against a real running FxSound instance and sanity-checks the schema.
// It skips if FxSound isn't running, since --status silently no-ops on a
// cold start.
func TestGetStatus(t *testing.T) {
	running, err := fxsound.IsFxSoundRunning()
	if err != nil {
		t.Fatalf("IsFxSoundRunning: %v", err)
	}
	if !running {
		t.Skip("FxSound.exe is not running; --status requires a running instance")
	}

	session, ctx, cleanup := connectTestSession(t, 15*time.Second)
	defer cleanup()

	checkStatus := func(t *testing.T, raw string) {
		t.Helper()
		var status struct {
			Version string `json:"version"`
			Power   bool   `json:"power"`
			Presets struct {
				BuiltIn     []struct{ Name string } `json:"built_in"`
				UserDefined []struct{ Name string } `json:"user_defined"`
			} `json:"presets"`
			SelectedPreset string   `json:"selected_preset"`
			OutputDevices  []string `json:"output_devices"`
			SelectedOutput string   `json:"selected_output"`
			Equalizer      struct {
				NumBands int      `json:"num_bands"`
				Bands    []EqBand `json:"bands"`
			} `json:"equalizer"`
		}
		if err := json.Unmarshal([]byte(raw), &status); err != nil {
			t.Fatalf("unmarshal status: %v\nraw: %s", err, raw)
		}
		if status.Version == "" {
			t.Error("expected non-empty version")
		}
		if status.SelectedPreset == "" {
			t.Error("expected non-empty selected_preset")
		}
		if len(status.Equalizer.Bands) != status.Equalizer.NumBands {
			t.Errorf("equalizer.bands has %d entries, want num_bands=%d", len(status.Equalizer.Bands), status.Equalizer.NumBands)
		}
		t.Logf("power=%v preset=%q output=%q num_bands=%d", status.Power, status.SelectedPreset, status.SelectedOutput, status.Equalizer.NumBands)
	}

	t.Run("resource", func(t *testing.T) {
		res, err := session.ReadResource(ctx, &mcp.ReadResourceParams{URI: "fxsound://status"})
		if err != nil {
			t.Fatalf("ReadResource(fxsound://status): %v", err)
		}
		if len(res.Contents) != 1 {
			t.Fatalf("expected 1 content item, got %d", len(res.Contents))
		}
		checkStatus(t, res.Contents[0].Text)
	})

	t.Run("tool", func(t *testing.T) {
		res, err := session.CallTool(ctx, &mcp.CallToolParams{Name: "fxsound_get_status"})
		if err != nil {
			t.Fatalf("CallTool(fxsound_get_status): %v", err)
		}
		if res.IsError {
			t.Fatalf("fxsound_get_status returned an error result: %+v", res.Content)
		}
		checkStatus(t, textContent(t, res))
	})
}

// TestDerivedResources exercises the presets/equalizer/effects resources,
// which are narrower views over the same --status read as fxsound://status.
// It skips if FxSound isn't running, for the same reason as TestGetStatus.
func TestDerivedResources(t *testing.T) {
	running, err := fxsound.IsFxSoundRunning()
	if err != nil {
		t.Fatalf("IsFxSoundRunning: %v", err)
	}
	if !running {
		t.Skip("FxSound.exe is not running; --status requires a running instance")
	}

	session, ctx, cleanup := connectTestSession(t, 15*time.Second)
	defer cleanup()

	t.Run("presets", func(t *testing.T) {
		res, err := session.ReadResource(ctx, &mcp.ReadResourceParams{URI: "fxsound://presets"})
		if err != nil {
			t.Fatalf("ReadResource(fxsound://presets): %v", err)
		}
		var presets struct {
			SelectedPreset string                  `json:"selected_preset"`
			BuiltIn        []struct{ Name string } `json:"built_in"`
			UserDefined    []struct{ Name string } `json:"user_defined"`
		}
		if err := json.Unmarshal([]byte(res.Contents[0].Text), &presets); err != nil {
			t.Fatalf("unmarshal presets: %v\nraw: %s", err, res.Contents[0].Text)
		}
		if presets.SelectedPreset == "" {
			t.Error("expected non-empty selected_preset")
		}
		if len(presets.BuiltIn) == 0 {
			t.Error("expected at least one built-in preset")
		}
		t.Logf("selected=%q built_in=%d user_defined=%d", presets.SelectedPreset, len(presets.BuiltIn), len(presets.UserDefined))
	})

	t.Run("equalizer", func(t *testing.T) {
		res, err := session.ReadResource(ctx, &mcp.ReadResourceParams{URI: "fxsound://equalizer"})
		if err != nil {
			t.Fatalf("ReadResource(fxsound://equalizer): %v", err)
		}
		var eq struct {
			NumBands int      `json:"num_bands"`
			Bands    []EqBand `json:"bands"`
		}
		if err := json.Unmarshal([]byte(res.Contents[0].Text), &eq); err != nil {
			t.Fatalf("unmarshal equalizer: %v\nraw: %s", err, res.Contents[0].Text)
		}
		if len(eq.Bands) != eq.NumBands {
			t.Errorf("bands has %d entries, want num_bands=%d", len(eq.Bands), eq.NumBands)
		}
		t.Logf("num_bands=%d", eq.NumBands)
	})

	t.Run("effects", func(t *testing.T) {
		res, err := session.ReadResource(ctx, &mcp.ReadResourceParams{URI: "fxsound://effects"})
		if err != nil {
			t.Fatalf("ReadResource(fxsound://effects): %v", err)
		}
		var effects struct {
			Clarity      float64 `json:"clarity"`
			Ambience     float64 `json:"ambience"`
			Surround     float64 `json:"surround"`
			DynamicBoost float64 `json:"dynamicboost"`
			Bass         float64 `json:"bass"`
		}
		if err := json.Unmarshal([]byte(res.Contents[0].Text), &effects); err != nil {
			t.Fatalf("unmarshal effects: %v\nraw: %s", err, res.Contents[0].Text)
		}
		t.Logf("clarity=%v ambience=%v surround=%v dynamicboost=%v bass=%v",
			effects.Clarity, effects.Ambience, effects.Surround, effects.DynamicBoost, effects.Bass)
	})
}

// TestDocsResource reads fxsound://docs/command-line-options, which is
// served from an embedded copy and needs no running FxSound instance, and
// checks it looks like the real reference doc.
func TestDocsResource(t *testing.T) {
	session, ctx, cleanup := connectTestSession(t, 10*time.Second)
	defer cleanup()

	res, err := session.ReadResource(ctx, &mcp.ReadResourceParams{URI: "fxsound://docs/command-line-options"})
	if err != nil {
		t.Fatalf("ReadResource(fxsound://docs/command-line-options): %v", err)
	}
	if len(res.Contents) != 1 {
		t.Fatalf("expected 1 content item, got %d", len(res.Contents))
	}
	text := res.Contents[0].Text
	if res.Contents[0].MIMEType != "text/markdown" {
		t.Errorf("MIMEType = %q, want text/markdown", res.Contents[0].MIMEType)
	}
	for _, want := range []string{"--power", "--set_band_freq", "--status", "mutually exclusive"} {
		if !strings.Contains(text, want) {
			t.Errorf("docs resource missing expected content %q", want)
		}
	}
}

// EqBand mirrors fxsound.EqBand's wire shape, for test-local decoding.
type EqBand struct {
	Index     int     `json:"index"`
	Frequency float64 `json:"frequency"`
	Gain      float64 `json:"gain"`
}

// connectTestSession builds the server, connects a client to it over
// stdio, and returns the session, a bounded context, and a cleanup func.
func connectTestSession(t *testing.T, timeout time.Duration) (*mcp.ClientSession, context.Context, func()) {
	t.Helper()
	bin := buildServer(t)

	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	client := mcp.NewClient(&mcp.Implementation{Name: "test-client", Version: "0.0.1"}, nil)
	transport := &mcp.CommandTransport{Command: exec.CommandContext(ctx, bin)}

	session, err := client.Connect(ctx, transport, nil)
	if err != nil {
		cancel()
		t.Fatalf("connect: %v", err)
	}
	return session, ctx, func() {
		session.Close()
		cancel()
	}
}

// textContent extracts the text of a tool result's first content item,
// which is how every tool in this package returns its JSON payload.
func textContent(t *testing.T, res *mcp.CallToolResult) string {
	t.Helper()
	if len(res.Content) == 0 {
		t.Fatal("tool result has no content")
	}
	tc, ok := res.Content[0].(*mcp.TextContent)
	if !ok {
		t.Fatalf("expected TextContent, got %T", res.Content[0])
	}
	return tc.Text
}

// buildServer compiles the fxmcp binary into a temp dir and returns its path.
func buildServer(t *testing.T) string {
	t.Helper()
	bin := t.TempDir() + `\fxmcp_test.exe`
	goExe, err := exec.LookPath("go")
	if err != nil {
		goExe = filepath.Join(runtime.GOROOT(), "bin", "go.exe")
	}
	cmd := exec.Command(goExe, "build", "-o", bin, ".")
	out, err := cmd.CombinedOutput()
	if err != nil {
		t.Fatalf("go build: %v\n%s", err, out)
	}
	return bin
}
