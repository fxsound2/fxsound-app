package mcpserver

import (
	"context"
	"fmt"

	"github.com/modelcontextprotocol/go-sdk/mcp"

	"fxmcp/internal/fxsound"
)

// applyResult is the common output shape for every write tool in this
// package: a settings-change either applied and returned a short
// human-readable detail, or the call returned an error explaining why it
// didn't (a precondition FxController.cpp would otherwise have silently
// no-op'd on).
type applyResult struct {
	Applied bool   `json:"applied"`
	Detail  string `json:"detail,omitempty"`
}

func boolFlag(b bool) string {
	if b {
		return "1"
	}
	return "0"
}

func (a *App) registerWriteTools(s *mcp.Server) {
	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_set_power",
		Description: "Turns FxSound's audio processing (DFX) on or off. Works whether or not FxSound is " +
			"currently running -- if it isn't, this launches it with power set accordingly.",
		Annotations: &mcp.ToolAnnotations{DestructiveHint: boolPtr(false)},
	}, a.toolSetPower)

	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_select_preset",
		Description: "Selects the active preset by name (case-sensitive, must match an existing preset " +
			"exactly -- see fxsound://presets for valid names). Only takes effect if power is on; works " +
			"whether or not FxSound is currently running.",
		Annotations: &mcp.ToolAnnotations{DestructiveHint: boolPtr(false)},
	}, a.toolSelectPreset)

	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_set_output_device",
		Description: "Selects the audio output device by its exact Windows friendly name (see " +
			"fxsound://status's output_devices or fxsound://diagnostics's device list for valid names). " +
			"Works whether or not FxSound is currently running.",
		Annotations: &mcp.ToolAnnotations{DestructiveHint: boolPtr(false)},
	}, a.toolSetOutputDevice)

	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_save_preset",
		Description: "Saves the current modified settings as a new user preset and selects it. Requires " +
			"FxSound to be running, power to be on, and the current preset to have unsaved changes (check " +
			"fxsound://presets). The name is sanitized (filesystem-reserved characters < > : \" / \\ | ? * " +
			"stripped, truncated to 64 characters) and rejected if that collides with an existing preset name.",
		Annotations: &mcp.ToolAnnotations{DestructiveHint: boolPtr(false)},
	}, a.toolSavePreset)

	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_overwrite_preset",
		Description: "Overwrites the currently selected user preset with its unsaved changes. Requires " +
			"FxSound to be running, power to be on, the selected preset to be a user preset (not built-in), " +
			"and to have unsaved changes. This permanently discards the preset's previously saved content.",
		Annotations: &mcp.ToolAnnotations{DestructiveHint: boolPtr(true)},
	}, a.toolOverwritePreset)

	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_undo_preset",
		Description: "Reverts the currently selected preset's unsaved changes. Requires FxSound to be " +
			"running, power to be on, and the preset to have unsaved changes.",
		Annotations: &mcp.ToolAnnotations{DestructiveHint: boolPtr(false)},
	}, a.toolUndoPreset)

	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_rename_preset",
		Description: "Renames the currently selected user preset. Requires FxSound to be running, power to " +
			"be on, the selected preset to be a user preset (not built-in) with no unsaved changes. The name " +
			"is sanitized and collision-checked the same way as fxsound_save_preset.",
		Annotations: &mcp.ToolAnnotations{DestructiveHint: boolPtr(false)},
	}, a.toolRenamePreset)

	mcp.AddTool(s, &mcp.Tool{
		Name: "fxsound_delete_preset",
		Description: "Deletes the currently selected user preset. Requires FxSound to be running, power to " +
			"be on, and the selected preset to be a user preset (not built-in). This is irreversible.",
		Annotations: &mcp.ToolAnnotations{DestructiveHint: boolPtr(true)},
	}, a.toolDeletePreset)
}

type setPowerInput struct {
	On bool `json:"on" jsonschema:"true to turn FxSound's audio processing on, false to turn it off"`
}

func (a *App) toolSetPower(ctx context.Context, _ *mcp.CallToolRequest, in setPowerInput) (*mcp.CallToolResult, applyResult, error) {
	if err := fxsound.Apply(ctx, a.Paths, map[string]string{"power": boolFlag(in.On)}, false); err != nil {
		return nil, applyResult{}, err
	}
	return nil, applyResult{Applied: true, Detail: fmt.Sprintf("power set to %v", in.On)}, nil
}

type selectPresetInput struct {
	Name string `json:"name" jsonschema:"the preset name to select, case-sensitive"`
}

func (a *App) toolSelectPreset(ctx context.Context, _ *mcp.CallToolRequest, in selectPresetInput) (*mcp.CallToolResult, applyResult, error) {
	if in.Name == "" {
		return nil, applyResult{}, fmt.Errorf("name must not be empty")
	}
	if err := fxsound.Apply(ctx, a.Paths, map[string]string{"preset": in.Name}, false); err != nil {
		return nil, applyResult{}, err
	}
	return nil, applyResult{Applied: true, Detail: fmt.Sprintf("preset selected: %s", in.Name)}, nil
}

type setOutputDeviceInput struct {
	DeviceName string `json:"device_name" jsonschema:"the output device's exact Windows friendly name"`
}

func (a *App) toolSetOutputDevice(ctx context.Context, _ *mcp.CallToolRequest, in setOutputDeviceInput) (*mcp.CallToolResult, applyResult, error) {
	if in.DeviceName == "" {
		return nil, applyResult{}, fmt.Errorf("device_name must not be empty")
	}
	if err := fxsound.Apply(ctx, a.Paths, map[string]string{"output": in.DeviceName}, false); err != nil {
		return nil, applyResult{}, err
	}
	return nil, applyResult{Applied: true, Detail: fmt.Sprintf("output device set to: %s", in.DeviceName)}, nil
}

type presetNameInput struct {
	Name string `json:"name" jsonschema:"the preset name"`
}

func (a *App) toolSavePreset(ctx context.Context, _ *mcp.CallToolRequest, in presetNameInput) (*mcp.CallToolResult, applyResult, error) {
	status, err := readyStatus(ctx, a.Paths)
	if err != nil {
		return nil, applyResult{}, err
	}
	if err := fxsound.ValidateSavePreset(status); err != nil {
		return nil, applyResult{}, err
	}
	sanitized, err := fxsound.ValidatePresetName(in.Name, status)
	if err != nil {
		return nil, applyResult{}, err
	}
	if err := fxsound.Apply(ctx, a.Paths, map[string]string{"save_preset": sanitized}, true); err != nil {
		return nil, applyResult{}, err
	}
	return nil, applyResult{Applied: true, Detail: fmt.Sprintf("saved and selected new preset: %s", sanitized)}, nil
}

func (a *App) toolOverwritePreset(ctx context.Context, _ *mcp.CallToolRequest, _ any) (*mcp.CallToolResult, applyResult, error) {
	status, err := readyStatus(ctx, a.Paths)
	if err != nil {
		return nil, applyResult{}, err
	}
	if err := fxsound.ValidateOverwritePreset(status); err != nil {
		return nil, applyResult{}, err
	}
	if err := fxsound.Apply(ctx, a.Paths, map[string]string{"overwrite_preset": ""}, true); err != nil {
		return nil, applyResult{}, err
	}
	return nil, applyResult{Applied: true, Detail: fmt.Sprintf("overwrote preset: %s", status.SelectedPreset)}, nil
}

func (a *App) toolUndoPreset(ctx context.Context, _ *mcp.CallToolRequest, _ any) (*mcp.CallToolResult, applyResult, error) {
	status, err := readyStatus(ctx, a.Paths)
	if err != nil {
		return nil, applyResult{}, err
	}
	if err := fxsound.ValidateUndoPreset(status); err != nil {
		return nil, applyResult{}, err
	}
	if err := fxsound.Apply(ctx, a.Paths, map[string]string{"undo_preset": ""}, true); err != nil {
		return nil, applyResult{}, err
	}
	return nil, applyResult{Applied: true, Detail: fmt.Sprintf("undid unsaved changes to preset: %s", status.SelectedPreset)}, nil
}

func (a *App) toolRenamePreset(ctx context.Context, _ *mcp.CallToolRequest, in presetNameInput) (*mcp.CallToolResult, applyResult, error) {
	status, err := readyStatus(ctx, a.Paths)
	if err != nil {
		return nil, applyResult{}, err
	}
	if err := fxsound.ValidateRenamePreset(status); err != nil {
		return nil, applyResult{}, err
	}
	sanitized, err := fxsound.ValidatePresetName(in.Name, status)
	if err != nil {
		return nil, applyResult{}, err
	}
	oldName := status.SelectedPreset
	if err := fxsound.Apply(ctx, a.Paths, map[string]string{"rename_preset": sanitized}, true); err != nil {
		return nil, applyResult{}, err
	}
	return nil, applyResult{Applied: true, Detail: fmt.Sprintf("renamed preset %q to %q", oldName, sanitized)}, nil
}

func (a *App) toolDeletePreset(ctx context.Context, _ *mcp.CallToolRequest, _ any) (*mcp.CallToolResult, applyResult, error) {
	status, err := readyStatus(ctx, a.Paths)
	if err != nil {
		return nil, applyResult{}, err
	}
	if err := fxsound.ValidateDeletePreset(status); err != nil {
		return nil, applyResult{}, err
	}
	if err := fxsound.Apply(ctx, a.Paths, map[string]string{"delete_preset": ""}, true); err != nil {
		return nil, applyResult{}, err
	}
	return nil, applyResult{Applied: true, Detail: fmt.Sprintf("deleted preset: %s", status.SelectedPreset)}, nil
}

// readyStatus ensures a live FxSound instance is up (launching one if
// needed) and returns its current status, for the preset-management tools
// that need to validate preconditions (power, modified state, built-in
// vs. user preset) before building the real command.
func readyStatus(ctx context.Context, paths *fxsound.Paths) (*fxsound.Status, error) {
	if err := fxsound.EnsureRunning(ctx, paths); err != nil {
		return nil, err
	}
	status, err := fxsound.ReadStatus(ctx, paths)
	if err != nil {
		return nil, fmt.Errorf("read current state: %w", err)
	}
	return status, nil
}
