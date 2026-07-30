package fxsound

import (
	"context"
	"fmt"
	"sort"
	"strings"
	"time"
)

const (
	applyConfigTimeout = 5 * time.Second

	launchPollTick  = 200 * time.Millisecond
	launchPollTotal = 3 * time.Second

	readyPollTick  = 300 * time.Millisecond
	readyPollTotal = 8 * time.Second
)

// BuildCommandLine converts a set of FxController.cpp applyConfig flag
// values into command-line arguments in the `--flag=value` form it
// requires (a space between flag and value, e.g. "--power 1", is not
// supported -- see docs/COMMAND_LINE_OPTIONS.md).
//
// A flag mapped to the empty string is emitted as a bare flag with no
// "=value" (e.g. "overwrite_preset" -> "--overwrite_preset"), matching the
// flags that take no value. Values containing whitespace are wrapped in
// double quotes, matching how COMMAND_LINE_OPTIONS.md documents quoting
// preset/device names; FxController's ArgumentList parsing strips a
// matching pair of quotes via .unquoted() before use.
//
// A value containing a literal `"` character is rejected outright rather
// than quoted, escaped, or stripped. FxController's own re-tokenizer (see
// buildRawCmdLine's doc comment in process.go) treats every `"` as a bare
// quote-region toggle with no backslash-escape awareness, so a value like
// `Foo" --power=0 --preset="Foo` would break out of its quoted segment and
// splice extra --flag=value tokens into the literal command line sent to
// FxSound.exe -- an argument-injection path, since these values can
// originate from an untrusted upstream MCP client/LLM. None of the flags
// this builds have a legitimate reason to contain a literal quote
// character (preset/device names and language codes never do), so
// rejecting is safe and simpler than trying to escape a tokenizer that
// has no escape syntax of its own.
//
// Map keys are sorted for deterministic output; argument order does not
// matter to FxController's flag-by-name parsing.
func BuildCommandLine(flags map[string]string) ([]string, error) {
	keys := make([]string, 0, len(flags))
	for k := range flags {
		keys = append(keys, k)
	}
	sort.Strings(keys)

	args := make([]string, 0, len(keys))
	for _, k := range keys {
		v := flags[k]
		if strings.Contains(v, `"`) {
			return nil, newError(ErrKindValueRejected, nil, "value for --%s contains a literal double-quote character, which is not allowed", k)
		}
		if v == "" {
			args = append(args, "--"+k)
			continue
		}
		if strings.ContainsAny(v, " \t") {
			v = `"` + v + `"`
		}
		args = append(args, "--"+k+"="+v)
	}
	return args, nil
}

// Apply builds a command line from flags and sends it to FxSound.exe.
//
// If requireRunning is true, this first ensures an instance is already
// running (launching one if not) before sending flags, since several
// applyConfig flags (--status, --set_band_freq, --set_band_gain,
// --set_effect, and the preset-management commands) are only understood
// by applyConfig, not by the narrower initConfig parse a cold-start
// invocation uses -- sending them to a cold start would silently no-op.
// Flags that both initConfig and applyConfig understand (power, preset,
// output, view, language, num_bands, balance, filter_q, master_gain,
// normalization, volume_leveling, run_minimized) don't need this: a single
// invocation works whether or not FxSound is already running.
func Apply(ctx context.Context, paths *Paths, flags map[string]string, requireRunning bool) error {
	if requireRunning {
		if err := EnsureRunning(ctx, paths); err != nil {
			return err
		}
	}
	args, err := BuildCommandLine(flags)
	if err != nil {
		return err
	}
	if _, err := Run(ctx, applyConfigTimeout, paths.FxSoundExe, args...); err != nil {
		return err
	}
	return nil
}

// EnsureRunning makes sure a live FxSound instance is up and responding
// before a caller sends a command that only applyConfig (not initConfig)
// understands. If FxSound isn't running, it launches one with no
// arguments and waits (bounded) for it to appear as a running process,
// then for it to become responsive -- confirmed via a successful --status
// read, not just process existence, since FxController's model needs to
// finish initializing before it will react to a forwarded command.
//
// The cold-start launch is intentionally not waited on (cmd.Start, not
// cmd.Run/Wait): it becomes the persistent instance and runs until the
// user quits it, not until this call returns.
func EnsureRunning(ctx context.Context, paths *Paths) error {
	running, err := IsFxSoundRunning()
	if err != nil {
		return fmt.Errorf("check running state: %w", err)
	}
	if running {
		return nil
	}

	if err := launch(paths.FxSoundExe); err != nil {
		return newError(ErrKindRequiresRunningInstance, err, "launch FxSound.exe")
	}

	if err := pollUntil(ctx, launchPollTotal, launchPollTick, func() (bool, error) {
		return IsFxSoundRunning()
	}); err != nil {
		return newError(ErrKindRequiresRunningInstance, err, "FxSound.exe did not start within %s", launchPollTotal)
	}

	var lastErr error
	if err := pollUntil(ctx, readyPollTotal, readyPollTick, func() (bool, error) {
		if _, statusErr := ReadStatus(ctx, paths); statusErr != nil {
			lastErr = statusErr
			return false, nil
		}
		return true, nil
	}); err != nil {
		return newError(ErrKindRequiresRunningInstance, lastErr, "FxSound.exe did not become ready within %s", readyPollTotal)
	}
	return nil
}

// pollUntil calls check repeatedly (bounded by total, spaced by tick)
// until it reports true, an error, or the deadline/context is exceeded.
func pollUntil(ctx context.Context, total, tick time.Duration, check func() (bool, error)) error {
	deadline := time.Now().Add(total)
	for {
		ok, err := check()
		if err != nil {
			return err
		}
		if ok {
			return nil
		}
		if !time.Now().Before(deadline) {
			return fmt.Errorf("timed out after %s", total)
		}
		select {
		case <-ctx.Done():
			return ctx.Err()
		case <-time.After(tick):
		}
	}
}

// KV is an ordered key/value pair for the "key:value[,key:value...]"
// mini-format used by --set_band_freq, --set_band_gain, and --set_effect.
type KV struct {
	Key   string
	Value string
}

// FormatPairs renders pairs in that mini-format.
func FormatPairs(pairs []KV) string {
	parts := make([]string, len(pairs))
	for i, p := range pairs {
		parts[i] = p.Key + ":" + p.Value
	}
	return strings.Join(parts, ",")
}

// ValidateRange checks that v falls within [min, max], returning an error
// naming label if not. FxController.cpp's applyConfig silently resets an
// out-of-range global EQ value (balance, filter_q, master_gain,
// normalization, volume_leveling) to its default instead of erroring; this
// turns that into an explicit error before the value is ever sent.
func ValidateRange(label string, v, min, max float64) error {
	if v < min || v > max {
		return newError(ErrKindValueRejected, nil, "%s %.2f is out of range (%.2f to %.2f)", label, v, min, max)
	}
	return nil
}

// ValidateNumEqBands checks n is one of the band counts FxController
// accepts (5, 10, 15, 20, 31); any other value is silently reset to
// DEFAULT_NUM_EQ_BANDS by applyConfig instead of erroring.
func ValidateNumEqBands(n int) error {
	switch n {
	case 5, 10, 15, 20, 31:
		return nil
	default:
		return newError(ErrKindValueRejected, nil, "num_eq_bands %d is invalid (must be one of 5, 10, 15, 20, 31)", n)
	}
}
