package main

import (
	"strings"
	"testing"
	"time"

	"github.com/modelcontextprotocol/go-sdk/mcp"
)

// TestPrompts fetches every registered prompt with representative
// arguments and checks the generated message content is coherent: it
// mentions the arguments supplied and references the resources/tools the
// prompt is supposed to guide the model toward.
func TestPrompts(t *testing.T) {
	session, ctx, cleanup := connectTestSession(t, 20*time.Second)
	defer cleanup()

	tests := []struct {
		prompt      string
		args        map[string]string
		mustMention []string
	}{
		{
			prompt:      "diagnose-audio-issue",
			args:        map[string]string{"symptom": "Zoom call volume is very quiet"},
			mustMention: []string{"Zoom call volume is very quiet", "fxsound://diagnostics", "fxsound://status"},
		},
		{
			prompt:      "tune-for-scenario",
			args:        map[string]string{"scenario": "piano and flute clarity"},
			mustMention: []string{"piano and flute clarity", "fxsound://equalizer", "fxsound_set_eq_band_gains"},
		},
		{
			prompt:      "switch-listening-profile",
			args:        map[string]string{"profile": "gaming setup"},
			mustMention: []string{"gaming setup", "fxsound://presets", "fxsound_apply_settings"},
		},
		{
			prompt:      "create-preset-from-current",
			args:        map[string]string{"name": "My Test Preset"},
			mustMention: []string{"My Test Preset", "fxsound://presets", "fxsound_save_preset"},
		},
	}

	for _, tt := range tests {
		t.Run(tt.prompt, func(t *testing.T) {
			res, err := session.GetPrompt(ctx, &mcp.GetPromptParams{Name: tt.prompt, Arguments: tt.args})
			if err != nil {
				t.Fatalf("GetPrompt(%s): %v", tt.prompt, err)
			}
			if len(res.Messages) == 0 {
				t.Fatalf("GetPrompt(%s): no messages returned", tt.prompt)
			}
			tc, ok := res.Messages[0].Content.(*mcp.TextContent)
			if !ok {
				t.Fatalf("GetPrompt(%s): expected TextContent, got %T", tt.prompt, res.Messages[0].Content)
			}
			for _, want := range tt.mustMention {
				if !strings.Contains(tc.Text, want) {
					t.Errorf("GetPrompt(%s): message missing %q\n--- full text ---\n%s", tt.prompt, want, tc.Text)
				}
			}
			t.Logf("%s -> %d chars", tt.prompt, len(tc.Text))
		})
	}
}

// TestListPrompts confirms all four prompts are discoverable via the
// standard prompts/list call.
func TestListPrompts(t *testing.T) {
	session, ctx, cleanup := connectTestSession(t, 10*time.Second)
	defer cleanup()

	want := map[string]bool{
		"diagnose-audio-issue":       false,
		"tune-for-scenario":          false,
		"switch-listening-profile":   false,
		"create-preset-from-current": false,
	}
	for p, err := range session.Prompts(ctx, nil) {
		if err != nil {
			t.Fatalf("Prompts: %v", err)
		}
		if _, ok := want[p.Name]; ok {
			want[p.Name] = true
		}
	}
	for name, found := range want {
		if !found {
			t.Errorf("prompt %q not found in prompts/list", name)
		}
	}
}
