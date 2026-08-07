package mcpserver

import (
	"context"
	"encoding/json"

	"github.com/modelcontextprotocol/go-sdk/mcp"

	"fxmcp/internal/fxsound"
	"fxmcp/internal/mcpserver/docsdata"
)

const (
	uriDiagnostics            = "fxsound://diagnostics"
	uriStatus                 = "fxsound://status"
	uriPresets                = "fxsound://presets"
	uriEqualizer              = "fxsound://equalizer"
	uriEffects                = "fxsound://effects"
	uriDocsCommandLineOptions = "fxsound://docs/command-line-options"
)

func (a *App) registerResources(s *mcp.Server) {
	s.AddResource(&mcp.Resource{
		URI:  uriDiagnostics,
		Name: "diagnostics",
		Description: "Live snapshot of Windows playback devices and audio sessions, from fxdiag.exe --json. " +
			"Devices include device type, connection state (Active/Not Present/Disabled/Unplugged), mix format " +
			"(channels/sample rate/bit depth) and volume level. Sessions include per-app Windows mixer volume and mute state. " +
			"Playback (render) devices only; no capture/microphone devices.",
		MIMEType: "application/json",
	}, a.readDiagnostics)

	s.AddResource(&mcp.Resource{
		URI:  uriStatus,
		Name: "status",
		Description: "The running FxSound instance's current state: power, presets, selected preset, " +
			"output devices, selected output, equalizer, and effect levels (0-10 scale). Requires FxSound " +
			"to be running -- read fxsound://diagnostics or call fxsound_is_running to check first.",
		MIMEType: "application/json",
	}, a.readStatus)

	s.AddResource(&mcp.Resource{
		URI:  uriPresets,
		Name: "presets",
		Description: "The running FxSound instance's presets: selected preset name and the built-in and " +
			"user-defined preset lists, each entry with a modified flag. A narrower view of fxsound://status " +
			"for lower token cost. Requires FxSound to be running.",
		MIMEType: "application/json",
	}, a.readPresets)

	s.AddResource(&mcp.Resource{
		URI:  uriEqualizer,
		Name: "equalizer",
		Description: "The running FxSound instance's equalizer state: band count, per-band frequency/gain, " +
			"master gain, volume leveling, filter Q, and balance. A narrower view of " +
			"fxsound://status. Requires FxSound to be running.",
		MIMEType: "application/json",
	}, a.readEqualizer)

	s.AddResource(&mcp.Resource{
		URI:  uriEffects,
		Name: "effects",
		Description: "The running FxSound instance's effect levels on a 0-10 scale: clarity, ambience, " +
			"surround, dynamicboost, bass. A narrower view of fxsound://status. Requires FxSound to be running.",
		MIMEType: "application/json",
	}, a.readEffects)

	s.AddResource(&mcp.Resource{
		URI:  uriDocsCommandLineOptions,
		Name: "command-line-options",
		Description: "The authoritative FxSound.exe command-line option reference: value ranges and rounding " +
			"rules, preset-command mutual exclusivity, preset name sanitizing rules, and worked examples. " +
			"Consult this to resolve edge cases (e.g. why a save_preset or set_effect call had no visible " +
			"effect) rather than guessing from tool descriptions alone.",
		MIMEType: "text/markdown",
	}, a.readDocsCommandLineOptions)
}

func (a *App) readDiagnostics(ctx context.Context, req *mcp.ReadResourceRequest) (*mcp.ReadResourceResult, error) {
	diag, err := fxsound.RunDiagnostics(ctx, a.Paths)
	if err != nil {
		return nil, err
	}
	return jsonResource(uriDiagnostics, diag)
}

func (a *App) readStatus(ctx context.Context, req *mcp.ReadResourceRequest) (*mcp.ReadResourceResult, error) {
	status, err := fxsound.ReadStatus(ctx, a.Paths)
	if err != nil {
		return nil, err
	}
	return jsonResource(uriStatus, status)
}

func (a *App) readPresets(ctx context.Context, req *mcp.ReadResourceRequest) (*mcp.ReadResourceResult, error) {
	status, err := fxsound.ReadStatus(ctx, a.Paths)
	if err != nil {
		return nil, err
	}
	payload := struct {
		SelectedPreset string               `json:"selected_preset"`
		BuiltIn        []fxsound.PresetInfo `json:"built_in"`
		UserDefined    []fxsound.PresetInfo `json:"user_defined"`
	}{
		SelectedPreset: status.SelectedPreset,
		BuiltIn:        status.Presets.BuiltIn,
		UserDefined:    status.Presets.UserDefined,
	}
	return jsonResource(uriPresets, payload)
}

func (a *App) readEqualizer(ctx context.Context, req *mcp.ReadResourceRequest) (*mcp.ReadResourceResult, error) {
	status, err := fxsound.ReadStatus(ctx, a.Paths)
	if err != nil {
		return nil, err
	}
	return jsonResource(uriEqualizer, status.Equalizer)
}

func (a *App) readEffects(ctx context.Context, req *mcp.ReadResourceRequest) (*mcp.ReadResourceResult, error) {
	status, err := fxsound.ReadStatus(ctx, a.Paths)
	if err != nil {
		return nil, err
	}
	return jsonResource(uriEffects, status.Effects)
}

func (a *App) readDocsCommandLineOptions(ctx context.Context, req *mcp.ReadResourceRequest) (*mcp.ReadResourceResult, error) {
	return &mcp.ReadResourceResult{
		Contents: []*mcp.ResourceContents{{
			URI:      uriDocsCommandLineOptions,
			MIMEType: "text/markdown",
			Text:     docsdata.CommandLineOptions,
		}},
	}, nil
}

// jsonResource marshals v as JSON and wraps it as a single resource
// content item, the convention used throughout this file.
func jsonResource(uri string, v any) (*mcp.ReadResourceResult, error) {
	data, err := json.Marshal(v)
	if err != nil {
		return nil, err
	}
	return &mcp.ReadResourceResult{
		Contents: []*mcp.ResourceContents{{
			URI:      uri,
			MIMEType: "application/json",
			Text:     string(data),
		}},
	}, nil
}
