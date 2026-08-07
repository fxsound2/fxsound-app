package mcpserver

import (
	"context"

	"github.com/modelcontextprotocol/go-sdk/mcp"

	"fxmcp/internal/fxsound"
)

func (a *App) registerTools(s *mcp.Server) {
	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_is_running",
		Description: "Checks whether FxSound.exe is currently running. Several other tools and resources " +
			"(fxsound_get_status, fxsound://status, and anything that changes EQ bands, per-band gains, " +
			"effects, or presets) require a running instance -- a cold-start invocation of FxSound.exe " +
			"silently ignores those settings instead of applying them.",
		Annotations: &mcp.ToolAnnotations{ReadOnlyHint: true, DestructiveHint: boolPtr(false)},
	}, a.toolIsRunning)

	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_get_status",
		Description: "Reads the running FxSound instance's current state: power, presets (built-in and " +
			"user-defined, each with a modified flag), selected preset, output devices, selected output, " +
			"equalizer (band count/frequencies/gains, master gain, volume leveling, filter Q, " +
			"balance), and effect levels on a 0-10 scale (clarity, ambience, surround, dynamicboost, bass). " +
			"Requires FxSound to be running; call fxsound_is_running first if unsure.",
		Annotations: &mcp.ToolAnnotations{ReadOnlyHint: true, DestructiveHint: boolPtr(false)},
	}, a.toolGetStatus)
}

type isRunningOutput struct {
	Running bool `json:"running" jsonschema:"whether FxSound.exe is currently running"`
}

func (a *App) toolIsRunning(_ context.Context, _ *mcp.CallToolRequest, _ any) (*mcp.CallToolResult, isRunningOutput, error) {
	running, err := fxsound.IsFxSoundRunning()
	if err != nil {
		return nil, isRunningOutput{}, err
	}
	return nil, isRunningOutput{Running: running}, nil
}

func (a *App) toolGetStatus(ctx context.Context, _ *mcp.CallToolRequest, _ any) (*mcp.CallToolResult, fxsound.Status, error) {
	status, err := fxsound.ReadStatus(ctx, a.Paths)
	if err != nil {
		return nil, fxsound.Status{}, err
	}
	return nil, *status, nil
}
