package fxsound

import (
	"context"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"time"
)

const (
	statusTimeout   = 5 * time.Second
	statusPollTotal = 2 * time.Second
	statusPollTick  = 100 * time.Millisecond
)

// EqBand is a single equalizer band, as reported by --status.
type EqBand struct {
	Index     int     `json:"index"`
	Frequency float64 `json:"frequency"`
	Gain      float64 `json:"gain"`
}

// PresetInfo is a single preset entry, as reported by --status.
type PresetInfo struct {
	Name     string `json:"name"`
	Modified bool   `json:"modified"`
}

// Presets groups the built-in and user-defined preset lists reported by
// --status.
type Presets struct {
	BuiltIn     []PresetInfo `json:"built_in"`
	UserDefined []PresetInfo `json:"user_defined"`
}

// Equalizer is the equalizer section of --status's output.
type Equalizer struct {
	NumBands       int      `json:"num_bands"`
	MasterGain     float64  `json:"master_gain"`
	Normalization  float64  `json:"normalization"`
	VolumeLeveling float64  `json:"volume_leveling"`
	FilterQ        float64  `json:"filter_q"`
	Balance        float64  `json:"balance"`
	Bands          []EqBand `json:"bands"`
}

// Effects is the effects section of --status's output. Values are on a
// 0-10 scale (matching --set_effect's input range), not the raw 0-1 scale
// FxController stores internally.
type Effects struct {
	Clarity      float64 `json:"clarity"`
	Ambience     float64 `json:"ambience"`
	Surround     float64 `json:"surround"`
	DynamicBoost float64 `json:"dynamicboost"`
	Bass         float64 `json:"bass"`
}

// Status is the shape of %APPDATA%\FxSound\status.json, written by
// FxController::printStatus (the --status command-line option).
type Status struct {
	Version        string    `json:"version"`
	Power          bool      `json:"power"`
	Presets        Presets   `json:"presets"`
	SelectedPreset string    `json:"selected_preset"`
	OutputDevices  []string  `json:"output_devices"`
	SelectedOutput string    `json:"selected_output"`
	Equalizer      Equalizer `json:"equalizer"`
	Effects        Effects   `json:"effects"`
}

func statusFilePath() (string, error) {
	appData := os.Getenv("APPDATA")
	if appData == "" {
		return "", newError(ErrKindUnknown, nil, "%%APPDATA%% not set")
	}
	return filepath.Join(appData, "FxSound", "status.json"), nil
}

// ReadStatus spawns "FxSound.exe --status" and reads back the resulting
// %APPDATA%\FxSound\status.json.
//
// --status requires an already-running instance (see IsFxSoundRunning); a
// cold-start invocation silently no-ops, in which case this may read a
// stale or absent file. There is no synchronous completion signal for the
// write, so this polls for the file's mtime to advance past the moment of
// invocation, bounded by statusPollTotal.
func ReadStatus(ctx context.Context, paths *Paths) (*Status, error) {
	path, err := statusFilePath()
	if err != nil {
		return nil, err
	}

	var before time.Time
	if info, statErr := os.Stat(path); statErr == nil {
		before = info.ModTime()
	}

	if _, err := Run(ctx, statusTimeout, paths.FxSoundExe, "--status"); err != nil {
		return nil, err
	}

	deadline := time.Now().Add(statusPollTotal)
	var lastErr error
	for time.Now().Before(deadline) {
		info, statErr := os.Stat(path)
		if statErr == nil && info.ModTime().After(before) {
			data, readErr := os.ReadFile(path)
			if readErr != nil {
				lastErr = readErr
			} else {
				var s Status
				if jsonErr := json.Unmarshal(data, &s); jsonErr != nil {
					return nil, fmt.Errorf("parse status.json: %w", jsonErr)
				}
				return &s, nil
			}
		} else if statErr != nil {
			lastErr = statErr
		}
		select {
		case <-ctx.Done():
			return nil, ctx.Err()
		case <-time.After(statusPollTick):
		}
	}
	if lastErr != nil {
		return nil, newError(ErrKindRequiresRunningInstance, lastErr, "status.json not updated after --status within %s", statusPollTotal)
	}
	return nil, newError(ErrKindRequiresRunningInstance, nil, "status.json not updated after --status within %s (is FxSound running?)", statusPollTotal)
}
