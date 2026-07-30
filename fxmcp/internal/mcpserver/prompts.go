package mcpserver

import (
	"context"

	"github.com/modelcontextprotocol/go-sdk/mcp"
)

func (a *App) registerPrompts(s *mcp.Server) {
	s.AddPrompt(&mcp.Prompt{
		Name:        "diagnose-audio-issue",
		Description: "Diagnose an audio problem (e.g. low call volume, no sound, muffled output) by cross-referencing Windows device/session state with FxSound's own settings.",
		Arguments: []*mcp.PromptArgument{
			{
				Name:        "symptom",
				Description: "What's wrong, e.g. \"Zoom call volume is very quiet\" or \"no sound at all\"",
				Required:    true,
			},
		},
	}, a.promptDiagnoseAudioIssue)

	s.AddPrompt(&mcp.Prompt{
		Name:        "tune-for-scenario",
		Description: "Tune FxSound's equalizer and effects for a listening scenario or instrument focus (e.g. piano and flute clarity, bass-heavy for a party).",
		Arguments: []*mcp.PromptArgument{
			{
				Name:        "scenario",
				Description: "The listening scenario or instrument focus, e.g. \"piano and flute clarity\" or \"bass-heavy for a party\"",
				Required:    true,
			},
		},
	}, a.promptTuneForScenario)

	s.AddPrompt(&mcp.Prompt{
		Name:        "switch-listening-profile",
		Description: "Switch preset, output device, and view together to match a named setup (e.g. gaming setup, movie night).",
		Arguments: []*mcp.PromptArgument{
			{
				Name:        "profile",
				Description: "The target setup, e.g. \"gaming setup\" or \"movie night\"",
				Required:    true,
			},
		},
	}, a.promptSwitchListeningProfile)

	s.AddPrompt(&mcp.Prompt{
		Name:        "create-preset-from-current",
		Description: "Save the current modified FxSound settings as a new named preset, checking preconditions first so the save doesn't silently no-op.",
		Arguments: []*mcp.PromptArgument{
			{
				Name:        "name",
				Description: "Name for the new preset",
				Required:    true,
			},
		},
	}, a.promptCreatePresetFromCurrent)
}

func textMessage(text string) *mcp.PromptMessage {
	return &mcp.PromptMessage{
		Role:    "user",
		Content: &mcp.TextContent{Text: text},
	}
}

func (a *App) promptDiagnoseAudioIssue(_ context.Context, req *mcp.GetPromptRequest) (*mcp.GetPromptResult, error) {
	symptom := req.Params.Arguments["symptom"]
	text := "I'm having an audio problem: " + symptom + "\n\n" +
		"Please diagnose this:\n" +
		"1. Read fxsound://diagnostics for Windows-level device and session state -- is the relevant " +
		"app's session (e.g. zoom.exe, ms-teams.exe) muted or at a low volumeLevel? Is the currently " +
		"selected output device's state Active, or is it Not Present/Disabled/Unplugged?\n" +
		"2. Read fxsound://status for FxSound's own state -- is power on? Is the selected output device " +
		"the one you'd expect? Could master_gain, normalization, or volume_leveling be suppressing " +
		"loudness?\n" +
		"3. Cross-reference both to identify the most likely cause.\n" +
		"4. Explain the cause, and if it's something a tool can fix (fxsound_set_power, " +
		"fxsound_set_output_device, fxsound_apply_settings), propose the specific fix and ask before " +
		"applying it.\n\n" +
		"Note: there's currently no tool to change a Windows app's own per-app mixer volume/mute " +
		"directly (only FxSound's own settings) -- if that turns out to be the cause, tell me so I can " +
		"fix it in the Windows Volume Mixer myself."
	return &mcp.GetPromptResult{
		Description: "Diagnose an audio problem",
		Messages:    []*mcp.PromptMessage{textMessage(text)},
	}, nil
}

func (a *App) promptTuneForScenario(_ context.Context, req *mcp.GetPromptRequest) (*mcp.GetPromptResult, error) {
	scenario := req.Params.Arguments["scenario"]
	text := "I want to tune FxSound for this listening scenario: " + scenario + "\n\n" +
		"Please:\n" +
		"1. Read fxsound://equalizer to see the current band layout (num_bands, and each band's current " +
		"frequency and gain) and fxsound://effects for current effect levels.\n" +
		"2. Using your own audio-engineering knowledge of the frequency ranges and effect balance " +
		"involved in \"" + scenario + "\", work out which bands to boost or cut and/or which effect " +
		"levels to change.\n" +
		"3. Apply the changes with fxsound_set_eq_band_gains and/or fxsound_set_effects (or " +
		"fxsound_apply_settings if you also want to change the preset, output device, or other settings " +
		"at the same time).\n" +
		"4. Briefly explain what you changed and why, referencing the actual band frequencies/effect " +
		"names you touched."
	return &mcp.GetPromptResult{
		Description: "Tune the equalizer/effects for a listening scenario",
		Messages:    []*mcp.PromptMessage{textMessage(text)},
	}, nil
}

func (a *App) promptSwitchListeningProfile(_ context.Context, req *mcp.GetPromptRequest) (*mcp.GetPromptResult, error) {
	profile := req.Params.Arguments["profile"]
	text := "Switch FxSound to my \"" + profile + "\" listening profile.\n\n" +
		"Please:\n" +
		"1. Read fxsound://presets to see the available preset names and fxsound://status (or " +
		"fxsound://diagnostics) to see the available output devices.\n" +
		"2. Pick the preset, output device, and view (\"lite\" or \"full\") that best match \"" + profile + "\". " +
		"Ask me if it's ambiguous which preset or device I mean.\n" +
		"3. Apply them together in a single fxsound_apply_settings call rather than several separate " +
		"tool calls, since they're one logical change.\n" +
		"4. Confirm the change by reading fxsound://status afterward and reporting what's now selected."
	return &mcp.GetPromptResult{
		Description: "Switch preset/output/view together to match a named profile",
		Messages:    []*mcp.PromptMessage{textMessage(text)},
	}, nil
}

func (a *App) promptCreatePresetFromCurrent(_ context.Context, req *mcp.GetPromptRequest) (*mcp.GetPromptResult, error) {
	name := req.Params.Arguments["name"]
	text := "Save my current FxSound settings as a new preset named \"" + name + "\".\n\n" +
		"Please:\n" +
		"1. Read fxsound://presets and check whether the currently selected preset actually has " +
		"unsaved changes (the modified flag) -- fxsound_save_preset is a no-op if there's nothing to " +
		"save, or if power is off.\n" +
		"2. Note the name will be sanitized (characters < > : \" / \\ | ? * stripped, then truncated to " +
		"64 characters) and rejected if it collides (case-insensitively) with an existing preset name " +
		"after sanitizing -- if \"" + name + "\" looks like it would collide or become empty, tell me " +
		"before proceeding rather than guessing.\n" +
		"3. Call fxsound_save_preset with the name.\n" +
		"4. Confirm success by reading fxsound://presets afterward and reporting the final saved name " +
		"and whether it's now selected."
	return &mcp.GetPromptResult{
		Description: "Save current settings as a new preset",
		Messages:    []*mcp.PromptMessage{textMessage(text)},
	}, nil
}
