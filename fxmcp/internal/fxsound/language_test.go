package fxsound

import "testing"

func TestValidateLanguageCode(t *testing.T) {
	for _, code := range []string{"en", "fr", "fi", "en-US", "zh-Hans"} {
		if err := ValidateLanguageCode(code); err != nil {
			t.Errorf("ValidateLanguageCode(%q): unexpected error: %v", code, err)
		}
	}

	for _, code := range []string{
		"",
		"not a language",
		`en" --power=0 --preset="Foo`,
		"???",
	} {
		if err := ValidateLanguageCode(code); err == nil {
			t.Errorf("ValidateLanguageCode(%q): expected error, got nil", code)
		}
	}
}
