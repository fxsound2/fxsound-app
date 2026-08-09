//go:build windows

package fxsound

import (
	"bytes"
	"context"
	"os/exec"
	"strings"
	"sync"
	"syscall"
	"time"
)

// spawnMu serializes every invocation of FxSound.exe/fxdiag.exe this
// package makes. Several code paths -- a resource read of fxsound://status,
// a write tool's own EnsureRunning/ReadStatus precondition check, and
// EnsureRunning's readiness poll -- can all want to spawn "FxSound.exe
// --status" (or send a settings change) around the same time if two MCP
// tool calls happen concurrently. Without serialization, two overlapping
// invocations could race on status.json: one call's mtime-poll could pick
// up a write caused by the OTHER call, misattributing it as its own
// command taking effect. Holding this lock for the full spawn-to-exit
// duration of each call (not just while starting the process) makes every
// interaction with the app atomic from this server's point of view.
var spawnMu sync.Mutex

// runProcess actually starts and waits for a process; it's a package
// variable so tests can substitute a counting stub to verify how many
// times a real spawn would occur, without launching one.
var runProcess = func(ctx context.Context, exePath string, args []string, stdout, stderr *bytes.Buffer) error {
	cmd := exec.CommandContext(ctx, exePath)
	cmd.SysProcAttr = &syscall.SysProcAttr{CmdLine: buildRawCmdLine(exePath, args)}
	cmd.Stdout = stdout
	cmd.Stderr = stderr
	return cmd.Run()
}

// buildRawCmdLine assembles the exact literal command-line string the
// child process will see via GetCommandLineW(), bypassing Go's normal
// per-argument escaping.
//
// This matters because FxController.cpp's applyConfig (and initConfig, on
// a cold start) doesn't receive its command line as parsed argv -- JUCE's
// JUCEApplicationBase::getCommandLineParameters() (juce_ApplicationBase.cpp)
// re-derives it from the raw, unprocessed tail of GetCommandLineW() via
// CharacterFunctions::findEndOfToken, and FxController re-tokenizes that
// raw string itself via StringArray::fromTokens. That tokenizer
// (findEndOfToken, juce_CharacterFunctions.h) is a simple quote-toggling
// state machine with NO backslash-escape awareness: a `"` always flips in
// or out of a quoted region, full stop. Go's default argv-to-command-line
// escaping (syscall.escapeArg) assumes the opposite -- standard MSVCRT
// argv-splitting semantics, where `\"` is an escaped literal quote -- so
// letting Go auto-escape an argument already containing our own quote
// characters (from BuildCommandLine) corrupts it: e.g. a value like
// `"Speakers (2- Realtek(R) Audio)"` arrives at FxSound with a stray
// literal backslash and gets truncated at the first space, because JUCE's
// tokenizer sees `\` as an ordinary character and `"` as a real
// quote-region toggle it didn't expect there.
//
// The fix is to never go through Go's per-argument escaping for this
// executable: build the whole command line ourselves, with a single,
// unescaped pair of quotes around any token containing whitespace
// (matching what BuildCommandLine already produces for flag values, and
// applied here to the executable path too, since an install path like
// "C:\Program Files\FxSound LLC\FxSound\FxSound.exe" contains a space and
// JUCE's tokenizer must also see it quoted to skip over it correctly as
// argument 0).
func buildRawCmdLine(exePath string, args []string) string {
	head := exePath
	if strings.ContainsAny(head, " \t") {
		head = `"` + head + `"`
	}
	parts := append([]string{head}, args...)
	return strings.Join(parts, " ")
}

// Run executes exePath with args, waiting up to timeout for it to exit, and
// returns its captured stdout.
//
// Executables in this package are expected to exit quickly: fxdiag.exe
// always returns promptly, and FxSound.exe invocations either become the
// running instance's startup (for a cold launch with no status/query
// flags) or forward their command line to an already-running instance over
// a Windows message broadcast and exit almost immediately. The timeout
// guards against a hang, not a normal slow response.
func Run(ctx context.Context, timeout time.Duration, exePath string, args ...string) (stdout string, err error) {
	if exePath == "" {
		return "", newError(ErrKindAppNotFound, nil, "executable path not resolved (see fxsound.Locate)")
	}

	spawnMu.Lock()
	defer spawnMu.Unlock()

	ctx, cancel := context.WithTimeout(ctx, timeout)
	defer cancel()

	var out, stderr bytes.Buffer
	runErr := runProcess(ctx, exePath, args, &out, &stderr)
	if runErr != nil {
		if ctx.Err() == context.DeadlineExceeded {
			return "", newError(ErrKindTimeout, nil, "%s timed out after %s", exePath, timeout)
		}
		return "", newError(ErrKindUnknown, runErr, "%s failed (stderr: %s)", exePath, stderr.String())
	}
	return out.String(), nil
}

// launch starts exePath (FxSound.exe with no arguments) without waiting
// for it to exit.
//
// Deliberately takes no context: a cold launch becomes the persistent,
// long-running FxSound instance, and must not be killed just because the
// caller's request context is later canceled (e.g. once an MCP tool call
// that triggered the launch returns). No arguments means no quoting
// concerns -- see buildRawCmdLine's doc comment for why that matters for
// Run.
func launch(exePath string) error {
	if exePath == "" {
		return newError(ErrKindAppNotFound, nil, "executable path not resolved (see fxsound.Locate)")
	}

	spawnMu.Lock()
	defer spawnMu.Unlock()

	cmd := exec.Command(exePath)
	return cmd.Start()
}
