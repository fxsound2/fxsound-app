package fxsound

import "testing"

func TestNormalizeEffectName(t *testing.T) {
	tests := map[string]string{
		"fidelity":      "clarity",
		"Clarity":       "clarity",
		" CLARITY ":     "clarity",
		"ambience":      "ambience",
		"surround":      "surround",
		"dynamicboost":  "dynamicboost",
		"dynamic_boost": "dynamicboost",
		"DynamicBoost":  "dynamicboost",
		"bass":          "bass",
		"bassboost":     "bass",
		"bass_boost":    "bass",
	}
	for in, want := range tests {
		got, err := NormalizeEffectName(in)
		if err != nil {
			t.Errorf("NormalizeEffectName(%q): unexpected error: %v", in, err)
			continue
		}
		if got != want {
			t.Errorf("NormalizeEffectName(%q) = %q, want %q", in, got, want)
		}
	}

	if _, err := NormalizeEffectName("reverb"); err == nil {
		t.Error(`NormalizeEffectName("reverb"): expected error, got nil`)
	}
}

func TestValidateEffectValue(t *testing.T) {
	if err := ValidateEffectValue(MinEffectValue); err != nil {
		t.Errorf("min value: unexpected error: %v", err)
	}
	if err := ValidateEffectValue(MaxEffectValue); err != nil {
		t.Errorf("max value: unexpected error: %v", err)
	}
	if err := ValidateEffectValue(MinEffectValue - 0.1); err == nil {
		t.Error("below min: expected error, got nil")
	}
	if err := ValidateEffectValue(MaxEffectValue + 0.1); err == nil {
		t.Error("above max: expected error, got nil")
	}
}
