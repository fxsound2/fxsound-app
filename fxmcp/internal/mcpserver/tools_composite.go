package mcpserver

import (
	"context"
	"errors"
	"fmt"
	"strconv"
	"strings"

	"github.com/modelcontextprotocol/go-sdk/mcp"

	"fxmcp/internal/fxsound"
)

func (a *App) registerCompositeTool(s *mcp.Server) {
	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_apply_settings",
		Description: "Applies any combination of FxSound settings in a single call: power, preset " +
			"selection or preset management (save_preset/overwrite_preset/undo_preset/rename_preset/" +
			"delete_preset -- exactly one of these preset actions may be set per call), output device, view " +
			"(\"lite\" or \"full\"), language, num_eq_bands (5/10/15/20/31), balance, filter_q, master_gain, " +
			"volume_leveling, eq_bands (per-band frequency), eq_band_gains (per-band boost/" +
			"cut), effects (fidelity/clarity, ambience, surround, dynamicboost, bass), and run_minimized. " +
			"Multiple fields are sent as one atomic command line (a single FxSound.exe invocation), which is " +
			"more efficient than several separate tool calls and avoids intermediate UI states. Prefer the " +
			"focused single-purpose tools (fxsound_set_power, fxsound_select_preset, etc.) for a single " +
			"change; use this one when several settings should change together.",
		// Conditionally destructive: overwrite_preset/delete_preset can be
		// set through this tool, so it's annotated as potentially
		// destructive rather than falsely reassuring a client that every
		// call is safe.
		Annotations: &mcp.ToolAnnotations{DestructiveHint: boolPtr(true)},
	}, a.toolApplySettings)
}

type applySettingsInput struct {
	Power           *bool                `json:"power,omitempty" jsonschema:"turn audio processing on/off"`
	Preset          string               `json:"preset,omitempty" jsonschema:"select this preset by name"`
	SavePreset      string               `json:"save_preset,omitempty" jsonschema:"save current changes as a new preset with this name"`
	OverwritePreset bool                 `json:"overwrite_preset,omitempty" jsonschema:"overwrite the selected user preset with its unsaved changes"`
	UndoPreset      bool                 `json:"undo_preset,omitempty" jsonschema:"revert the selected preset's unsaved changes"`
	RenamePreset    string               `json:"rename_preset,omitempty" jsonschema:"rename the selected user preset to this name"`
	DeletePreset    bool                 `json:"delete_preset,omitempty" jsonschema:"delete the selected user preset"`
	OutputDevice    string               `json:"output_device,omitempty" jsonschema:"select this output device by exact Windows friendly name"`
	View            string               `json:"view,omitempty" jsonschema:"\"lite\" or \"full\""`
	Language        string               `json:"language,omitempty" jsonschema:"display language code as a BCP 47 tag, e.g. en, fr, fi"`
	NumEqBands      int                  `json:"num_eq_bands,omitempty" jsonschema:"one of 5, 10, 15, 20, 31"`
	Balance         *float64             `json:"balance,omitempty" jsonschema:"stereo balance in dB, -20 to 20"`
	FilterQ         *float64             `json:"filter_q,omitempty" jsonschema:"EQ filter Q factor, 1 to 3"`
	MasterGain      *float64             `json:"master_gain,omitempty" jsonschema:"master gain in dB, -20 to 20"`
	VolumeLeveling  *float64             `json:"volume_leveling,omitempty" jsonschema:"volume leveling amount, 0 to 4"`
	EqBands         []bandFrequencyInput `json:"eq_bands,omitempty" jsonschema:"per-band center frequency settings"`
	EqBandGains     []bandGainInput      `json:"eq_band_gains,omitempty" jsonschema:"per-band boost/cut settings in dB"`
	Effects         map[string]float64   `json:"effects,omitempty" jsonschema:"effect name to value (0-10)"`
	RunMinimized    *bool                `json:"run_minimized,omitempty" jsonschema:"start/show minimized to the system tray"`
}

func viewFlagValue(view string) (string, error) {
	switch strings.ToLower(strings.TrimSpace(view)) {
	case "lite":
		return "1", nil
	case "full":
		return "2", nil
	default:
		return "", fmt.Errorf(`view %q is invalid (must be "lite" or "full")`, view)
	}
}

func countSetPresetActions(in applySettingsInput) int {
	n := 0
	if in.Preset != "" {
		n++
	}
	if in.SavePreset != "" {
		n++
	}
	if in.OverwritePreset {
		n++
	}
	if in.UndoPreset {
		n++
	}
	if in.RenamePreset != "" {
		n++
	}
	if in.DeletePreset {
		n++
	}
	return n
}

func (a *App) toolApplySettings(ctx context.Context, _ *mcp.CallToolRequest, in applySettingsInput) (*mcp.CallToolResult, applyResult, error) {
	if n := countSetPresetActions(in); n > 1 {
		return nil, applyResult{}, fmt.Errorf(
			"only one preset action may be set per call (preset, save_preset, overwrite_preset, undo_preset, rename_preset, delete_preset); got %d", n)
	}

	needsPresetValidation := in.SavePreset != "" || in.OverwritePreset || in.UndoPreset || in.RenamePreset != "" || in.DeletePreset
	needsBandValidation := len(in.EqBands) > 0 || len(in.EqBandGains) > 0
	needsStatus := needsPresetValidation || needsBandValidation
	requireRunning := needsStatus || len(in.Effects) > 0

	var status *fxsound.Status
	if needsStatus {
		s, err := readyStatus(ctx, a.Paths)
		if err != nil {
			return nil, applyResult{}, err
		}
		status = s
	}

	flags := map[string]string{}

	if in.Power != nil {
		flags["power"] = boolFlag(*in.Power)
	}
	if in.Preset != "" {
		flags["preset"] = in.Preset
	}
	if in.SavePreset != "" {
		if err := fxsound.ValidateSavePreset(status); err != nil {
			return nil, applyResult{}, err
		}
		sanitized, err := fxsound.ValidatePresetName(in.SavePreset, status)
		if err != nil {
			return nil, applyResult{}, err
		}
		flags["save_preset"] = sanitized
	}
	if in.OverwritePreset {
		if err := fxsound.ValidateOverwritePreset(status); err != nil {
			return nil, applyResult{}, err
		}
		flags["overwrite_preset"] = ""
	}
	if in.UndoPreset {
		if err := fxsound.ValidateUndoPreset(status); err != nil {
			return nil, applyResult{}, err
		}
		flags["undo_preset"] = ""
	}
	if in.RenamePreset != "" {
		if err := fxsound.ValidateRenamePreset(status); err != nil {
			return nil, applyResult{}, err
		}
		sanitized, err := fxsound.ValidatePresetName(in.RenamePreset, status)
		if err != nil {
			return nil, applyResult{}, err
		}
		flags["rename_preset"] = sanitized
	}
	if in.DeletePreset {
		if err := fxsound.ValidateDeletePreset(status); err != nil {
			return nil, applyResult{}, err
		}
		flags["delete_preset"] = ""
	}
	if in.OutputDevice != "" {
		flags["output"] = in.OutputDevice
	}
	if in.View != "" {
		v, err := viewFlagValue(in.View)
		if err != nil {
			return nil, applyResult{}, err
		}
		flags["view"] = v
	}
	if in.Language != "" {
		if err := fxsound.ValidateLanguageCode(in.Language); err != nil {
			return nil, applyResult{}, err
		}
		flags["language"] = in.Language
	}
	if in.NumEqBands != 0 {
		if err := fxsound.ValidateNumEqBands(in.NumEqBands); err != nil {
			return nil, applyResult{}, err
		}
		flags["num_bands"] = strconv.Itoa(in.NumEqBands)
	}
	if in.Balance != nil {
		if err := fxsound.ValidateRange("balance", *in.Balance, -20, 20); err != nil {
			return nil, applyResult{}, err
		}
		flags["balance"] = formatFloat(*in.Balance)
	}
	if in.FilterQ != nil {
		if err := fxsound.ValidateRange("filter_q", *in.FilterQ, 1, 3); err != nil {
			return nil, applyResult{}, err
		}
		flags["filter_q"] = formatFloat(*in.FilterQ)
	}
	if in.MasterGain != nil {
		if err := fxsound.ValidateRange("master_gain", *in.MasterGain, -20, 20); err != nil {
			return nil, applyResult{}, err
		}
		flags["master_gain"] = formatFloat(*in.MasterGain)
	}
	if in.VolumeLeveling != nil {
		if err := fxsound.ValidateRange("volume_leveling", *in.VolumeLeveling, 0, 4); err != nil {
			return nil, applyResult{}, err
		}
		flags["volume_leveling"] = formatFloat(*in.VolumeLeveling)
	}
	if len(in.EqBands) > 0 {
		numBands := status.Equalizer.NumBands
		if err := fxsound.ValidateBandBatch(len(in.EqBands), numBands); err != nil {
			return nil, applyResult{}, err
		}
		pairs := make([]fxsound.KV, 0, len(in.EqBands))
		for _, b := range in.EqBands {
			if err := fxsound.ValidateBandIndex(b.Index, numBands); err != nil {
				return nil, applyResult{}, err
			}
			pairs = append(pairs, fxsound.KV{Key: strconv.Itoa(b.Index), Value: formatFloat(b.FrequencyHz)})
		}
		flags["set_band_freq"] = fxsound.FormatPairs(pairs)
	}
	if len(in.EqBandGains) > 0 {
		numBands := status.Equalizer.NumBands
		if err := fxsound.ValidateBandBatch(len(in.EqBandGains), numBands); err != nil {
			return nil, applyResult{}, err
		}
		pairs := make([]fxsound.KV, 0, len(in.EqBandGains))
		for _, b := range in.EqBandGains {
			if err := fxsound.ValidateBandIndex(b.Index, numBands); err != nil {
				return nil, applyResult{}, err
			}
			if err := fxsound.ValidateBandGain(b.GainDb); err != nil {
				return nil, applyResult{}, err
			}
			pairs = append(pairs, fxsound.KV{Key: strconv.Itoa(b.Index), Value: formatFloat(b.GainDb)})
		}
		flags["set_band_gain"] = fxsound.FormatPairs(pairs)
	}
	if len(in.Effects) > 0 {
		pairs := make([]fxsound.KV, 0, len(in.Effects))
		for name, value := range in.Effects {
			canonical, err := fxsound.NormalizeEffectName(name)
			if err != nil {
				return nil, applyResult{}, err
			}
			if err := fxsound.ValidateEffectValue(value); err != nil {
				return nil, applyResult{}, err
			}
			pairs = append(pairs, fxsound.KV{Key: canonical, Value: formatFloat(value)})
		}
		flags["set_effect"] = fxsound.FormatPairs(pairs)
	}
	if in.RunMinimized != nil && *in.RunMinimized {
		flags["run_minimized"] = ""
	}

	if len(flags) == 0 {
		return nil, applyResult{}, errors.New("no settings provided")
	}

	if err := fxsound.Apply(ctx, a.Paths, flags, requireRunning); err != nil {
		return nil, applyResult{}, err
	}
	return nil, applyResult{Applied: true, Detail: fmt.Sprintf("applied %d setting(s) in one call", len(flags))}, nil
}
