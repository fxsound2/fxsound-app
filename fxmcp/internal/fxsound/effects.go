package fxsound

import "strings"

// MinEffectValue and MaxEffectValue mirror --set_effect's documented
// accepted range (docs/COMMAND_LINE_OPTIONS.md), the same 0-10 scale
// --status reports effect levels on.
const (
	MinEffectValue = 0.0
	MaxEffectValue = 10.0
)

// effectAliases maps every name FxController.cpp's applyConfig accepts
// for --set_effect (case-insensitively) to a single canonical name -- the
// same key --status reports it under (see FxController.cpp:604-616 and
// FxController.cpp:691-695, where the enum FxEffects::Fidelity is
// reported under the JSON key "clarity").
var effectAliases = map[string]string{
	"fidelity":      "clarity",
	"clarity":       "clarity",
	"ambience":      "ambience",
	"surround":      "surround",
	"dynamicboost":  "dynamicboost",
	"dynamic_boost": "dynamicboost",
	"bass":          "bass",
	"bassboost":     "bass",
	"bass_boost":    "bass",
}

// NormalizeEffectName maps any of FxController's accepted aliases for an
// effect (case-insensitive, surrounding whitespace ignored) to the single
// canonical name used elsewhere in this package and in --status's output.
func NormalizeEffectName(name string) (string, error) {
	canonical, ok := effectAliases[strings.ToLower(strings.TrimSpace(name))]
	if !ok {
		return "", newError(ErrKindValueRejected, nil, "unknown effect %q (valid: fidelity/clarity, ambience, surround, dynamicboost/dynamic_boost, bass/bassboost/bass_boost)", name)
	}
	return canonical, nil
}

// ValidateEffectValue checks value is within --set_effect's accepted
// range. FxController.cpp silently leaves an out-of-range effect value
// unchanged (rather than resetting it to a default, unlike the global EQ
// parameters) instead of erroring.
func ValidateEffectValue(value float64) error {
	return ValidateRange("effect value", value, MinEffectValue, MaxEffectValue)
}
