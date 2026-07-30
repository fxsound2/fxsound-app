package fxsound

import (
	"bytes"
	"context"
	"strings"
	"testing"
)

func TestBuildCommandLine(t *testing.T) {
	tests := []struct {
		name  string
		flags map[string]string
		want  []string
	}{
		{
			name:  "simple value",
			flags: map[string]string{"power": "1"},
			want:  []string{"--power=1"},
		},
		{
			name:  "bare flag",
			flags: map[string]string{"overwrite_preset": ""},
			want:  []string{"--overwrite_preset"},
		},
		{
			name:  "value with space is quoted",
			flags: map[string]string{"preset": "Bass Booster"},
			want:  []string{`--preset="Bass Booster"`},
		},
		{
			name:  "sorted by key for determinism",
			flags: map[string]string{"view": "2", "power": "1", "output": "Speakers"},
			want:  []string{"--output=Speakers", "--power=1", "--view=2"},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got := BuildCommandLine(tt.flags)
			if len(got) != len(tt.want) {
				t.Fatalf("BuildCommandLine(%v) = %v, want %v", tt.flags, got, tt.want)
			}
			for i := range got {
				if got[i] != tt.want[i] {
					t.Errorf("BuildCommandLine(%v)[%d] = %q, want %q", tt.flags, i, got[i], tt.want[i])
				}
			}
		})
	}
}

// TestApplySpawnsExactlyOnce verifies that Apply, given any number of
// flags in a single map, results in exactly one underlying process spawn
// -- the property fxsound_apply_settings (Iteration 7) relies on to send
// several settings as one atomic command line.
func TestApplySpawnsExactlyOnce(t *testing.T) {
	var calls []struct {
		exePath string
		args    []string
	}
	orig := runProcess
	runProcess = func(_ context.Context, exePath string, args []string, stdout, stderr *bytes.Buffer) error {
		calls = append(calls, struct {
			exePath string
			args    []string
		}{exePath, args})
		return nil
	}
	t.Cleanup(func() { runProcess = orig })

	paths := &Paths{FxSoundExe: `C:\fake\FxSound.exe`}
	flags := map[string]string{
		"preset": "Bass Booster",
		"output": "Speakers",
		"view":   "1",
	}
	if err := Apply(context.Background(), paths, flags, false); err != nil {
		t.Fatalf("Apply: %v", err)
	}

	if len(calls) != 1 {
		t.Fatalf("expected exactly 1 process spawn, got %d: %+v", len(calls), calls)
	}
	joined := strings.Join(calls[0].args, " ")
	for _, want := range []string{`--preset="Bass Booster"`, "--output=Speakers", "--view=1"} {
		if !strings.Contains(joined, want) {
			t.Errorf("command line %q missing %q", joined, want)
		}
	}
}

func TestValidateRange(t *testing.T) {
	if err := ValidateRange("x", 5, 0, 10); err != nil {
		t.Errorf("5 in [0,10]: unexpected error: %v", err)
	}
	if err := ValidateRange("x", -1, 0, 10); err == nil {
		t.Error("-1 in [0,10]: expected error, got nil")
	}
	if err := ValidateRange("x", 11, 0, 10); err == nil {
		t.Error("11 in [0,10]: expected error, got nil")
	}
	// boundaries are inclusive
	if err := ValidateRange("x", 0, 0, 10); err != nil {
		t.Errorf("0 in [0,10]: unexpected error: %v", err)
	}
	if err := ValidateRange("x", 10, 0, 10); err != nil {
		t.Errorf("10 in [0,10]: unexpected error: %v", err)
	}
}

func TestValidateNumEqBands(t *testing.T) {
	for _, n := range []int{5, 10, 15, 20, 31} {
		if err := ValidateNumEqBands(n); err != nil {
			t.Errorf("ValidateNumEqBands(%d): unexpected error: %v", n, err)
		}
	}
	for _, n := range []int{0, 1, 4, 6, 11, 32, -5} {
		if err := ValidateNumEqBands(n); err == nil {
			t.Errorf("ValidateNumEqBands(%d): expected error, got nil", n)
		}
	}
}

func TestFormatPairs(t *testing.T) {
	got := FormatPairs([]KV{{Key: "0", Value: "60"}, {Key: "1", Value: "150"}})
	want := "0:60,1:150"
	if got != want {
		t.Errorf("FormatPairs = %q, want %q", got, want)
	}
}
