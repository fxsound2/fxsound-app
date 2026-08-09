package fxsound

import (
	"context"
	"encoding/json"
	"fmt"
	"time"
)

const diagnosticsTimeout = 5 * time.Second

// Device is a single Windows playback (render) device, as reported by
// fxdiag.exe --json.
type Device struct {
	DeviceID      string `json:"deviceId"`
	DeviceName    string `json:"deviceName"`
	DeviceType    string `json:"deviceType"`
	State         string `json:"state"`
	Channels      int    `json:"channels"`
	SamplesPerSec int    `json:"samplesPerSec"`
	BitsPerSample int    `json:"bitsPerSample"`
	VolumeLevel   int    `json:"volumeLevel"`
}

// Session is a single Windows audio session, as reported by
// fxdiag.exe --json.
type Session struct {
	SessionName   string `json:"sessionName"`
	State         string `json:"state"`
	SystemSession bool   `json:"systemSession"`
	VolumeLevel   int    `json:"volumeLevel"`
	Muted         bool   `json:"muted"`
}

// Diagnostics is the top-level shape of fxdiag.exe --json's output.
type Diagnostics struct {
	Devices  []Device  `json:"devices"`
	Sessions []Session `json:"sessions"`
}

// RunDiagnostics spawns fxdiag.exe --json and parses its stdout. Unlike
// FxSound.exe's --status, fxdiag prints its JSON report synchronously to
// its own stdout, so no file-polling is needed here.
func RunDiagnostics(ctx context.Context, paths *Paths) (*Diagnostics, error) {
	out, err := Run(ctx, diagnosticsTimeout, paths.FxDiagExe, "--json")
	if err != nil {
		return nil, err
	}
	var d Diagnostics
	if err := json.Unmarshal([]byte(out), &d); err != nil {
		return nil, fmt.Errorf("parse fxdiag output: %w", err)
	}
	return &d, nil
}
