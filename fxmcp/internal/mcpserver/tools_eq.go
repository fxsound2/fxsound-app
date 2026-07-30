package mcpserver

import (
	"context"
	"errors"
	"fmt"
	"strconv"

	"github.com/modelcontextprotocol/go-sdk/mcp"

	"fxmcp/internal/fxsound"
)

// formatFloat renders v the way FxController's parsing expects: the
// shortest exact decimal representation, avoiding forced trailing zeros
// (e.g. "7.5", "3", "-2.5") while still round-tripping exactly.
func formatFloat(v float64) string {
	return strconv.FormatFloat(v, 'f', -1, 64)
}

func (a *App) registerEqTools(s *mcp.Server) {
	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_set_eq_bands",
		Description: "Sets one or more equalizer band center frequencies in a single call. Requires " +
			"FxSound to be running. Each band index must be less than the current band count (see " +
			"fxsound://equalizer's num_bands); frequency range is per-band and enforced server-side, so an " +
			"out-of-range frequency is silently ignored rather than rejected here.",
		Annotations: &mcp.ToolAnnotations{DestructiveHint: boolPtr(false)},
	}, a.toolSetEqBands)

	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_set_eq_band_gains",
		Description: fmt.Sprintf("Sets one or more equalizer band boost/cut values in dB in a single call. "+
			"Requires FxSound to be running. Each band index must be less than the current band count (see "+
			"fxsound://equalizer's num_bands), and each gain must be within %.0f to %.0f dB.",
			fxsound.MinBandGainDb, fxsound.MaxBandGainDb),
		Annotations: &mcp.ToolAnnotations{DestructiveHint: boolPtr(false)},
	}, a.toolSetEqBandGains)

	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_set_effects",
		Description: fmt.Sprintf("Sets one or more effect levels in a single call: fidelity/clarity, "+
			"ambience, surround, dynamicboost, bass. Requires FxSound to be running. Each value must be "+
			"within %.0f to %.0f.", fxsound.MinEffectValue, fxsound.MaxEffectValue),
		Annotations: &mcp.ToolAnnotations{DestructiveHint: boolPtr(false)},
	}, a.toolSetEffects)
}

type bandFrequencyInput struct {
	Index       int     `json:"index" jsonschema:"0-based equalizer band index"`
	FrequencyHz float64 `json:"frequency_hz" jsonschema:"band center frequency in Hz"`
}

type setEqBandsInput struct {
	Bands []bandFrequencyInput `json:"bands" jsonschema:"one or more band frequency settings to apply together"`
}

func (a *App) toolSetEqBands(ctx context.Context, _ *mcp.CallToolRequest, in setEqBandsInput) (*mcp.CallToolResult, applyResult, error) {
	if len(in.Bands) == 0 {
		return nil, applyResult{}, errors.New("bands must be non-empty")
	}
	status, err := readyStatus(ctx, a.Paths)
	if err != nil {
		return nil, applyResult{}, err
	}
	numBands := status.Equalizer.NumBands
	if err := fxsound.ValidateBandBatch(len(in.Bands), numBands); err != nil {
		return nil, applyResult{}, err
	}
	pairs := make([]fxsound.KV, 0, len(in.Bands))
	for _, b := range in.Bands {
		if err := fxsound.ValidateBandIndex(b.Index, numBands); err != nil {
			return nil, applyResult{}, err
		}
		pairs = append(pairs, fxsound.KV{Key: strconv.Itoa(b.Index), Value: formatFloat(b.FrequencyHz)})
	}
	flags := map[string]string{"set_band_freq": fxsound.FormatPairs(pairs)}
	if err := fxsound.Apply(ctx, a.Paths, flags, true); err != nil {
		return nil, applyResult{}, err
	}
	return nil, applyResult{Applied: true, Detail: fmt.Sprintf("set frequency for %d band(s)", len(in.Bands))}, nil
}

type bandGainInput struct {
	Index  int     `json:"index" jsonschema:"0-based equalizer band index"`
	GainDb float64 `json:"gain_db" jsonschema:"band boost/cut in dB"`
}

type setEqBandGainsInput struct {
	Bands []bandGainInput `json:"bands" jsonschema:"one or more band gain settings to apply together"`
}

func (a *App) toolSetEqBandGains(ctx context.Context, _ *mcp.CallToolRequest, in setEqBandGainsInput) (*mcp.CallToolResult, applyResult, error) {
	if len(in.Bands) == 0 {
		return nil, applyResult{}, errors.New("bands must be non-empty")
	}
	status, err := readyStatus(ctx, a.Paths)
	if err != nil {
		return nil, applyResult{}, err
	}
	numBands := status.Equalizer.NumBands
	if err := fxsound.ValidateBandBatch(len(in.Bands), numBands); err != nil {
		return nil, applyResult{}, err
	}
	pairs := make([]fxsound.KV, 0, len(in.Bands))
	for _, b := range in.Bands {
		if err := fxsound.ValidateBandIndex(b.Index, numBands); err != nil {
			return nil, applyResult{}, err
		}
		if err := fxsound.ValidateBandGain(b.GainDb); err != nil {
			return nil, applyResult{}, err
		}
		pairs = append(pairs, fxsound.KV{Key: strconv.Itoa(b.Index), Value: formatFloat(b.GainDb)})
	}
	flags := map[string]string{"set_band_gain": fxsound.FormatPairs(pairs)}
	if err := fxsound.Apply(ctx, a.Paths, flags, true); err != nil {
		return nil, applyResult{}, err
	}
	return nil, applyResult{Applied: true, Detail: fmt.Sprintf("set gain for %d band(s)", len(in.Bands))}, nil
}

type setEffectsInput struct {
	Effects map[string]float64 `json:"effects" jsonschema:"effect name to value (0-10); names: fidelity/clarity, ambience, surround, dynamicboost, bass"`
}

func (a *App) toolSetEffects(ctx context.Context, _ *mcp.CallToolRequest, in setEffectsInput) (*mcp.CallToolResult, applyResult, error) {
	if len(in.Effects) == 0 {
		return nil, applyResult{}, errors.New("effects must be non-empty")
	}
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
	flags := map[string]string{"set_effect": fxsound.FormatPairs(pairs)}
	if err := fxsound.Apply(ctx, a.Paths, flags, true); err != nil {
		return nil, applyResult{}, err
	}
	return nil, applyResult{Applied: true, Detail: fmt.Sprintf("set %d effect(s)", len(in.Effects))}, nil
}
