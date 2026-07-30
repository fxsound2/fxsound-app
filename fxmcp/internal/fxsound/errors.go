package fxsound

import "fmt"

// ErrorKind categorizes why a call into this package failed, so a caller
// (or test) can branch on the failure category via errors.As instead of
// matching message text.
type ErrorKind int

const (
	// ErrKindUnknown is an unexpected failure that doesn't fit one of the
	// categories below (e.g. an OS-level error enumerating processes).
	ErrKindUnknown ErrorKind = iota
	// ErrKindAppNotFound means FxSound.exe or fxdiag.exe could not be
	// located (see Locate), or a resolved path turned out to be empty.
	ErrKindAppNotFound
	// ErrKindRequiresRunningInstance means the command only applyConfig
	// (not the narrower initConfig cold-start parse) understands, and
	// EnsureRunning could not bring FxSound up or confirm it responsive
	// in time.
	ErrKindRequiresRunningInstance
	// ErrKindValueRejected means FxController.cpp's applyConfig would have
	// silently clamped, reset to a default, dropped, or no-op'd this value
	// or command (an out-of-range number, an invalid band index, a preset
	// name that's empty after sanitizing or collides with an existing
	// one, or a preset-management precondition like power-off/built-in/
	// not-modified) -- rejected here instead, before ever being sent.
	ErrKindValueRejected
	// ErrKindTimeout means a process spawn didn't complete within its
	// allotted timeout.
	ErrKindTimeout
)

func (k ErrorKind) String() string {
	switch k {
	case ErrKindAppNotFound:
		return "app_not_found"
	case ErrKindRequiresRunningInstance:
		return "requires_running_instance"
	case ErrKindValueRejected:
		return "value_rejected"
	case ErrKindTimeout:
		return "timeout"
	default:
		return "unknown"
	}
}

// Error is the error type returned by this package's exported functions
// for the categorized failure modes above.
type Error struct {
	Kind ErrorKind
	Msg  string
	Err  error // wrapped underlying error, if any
}

func (e *Error) Error() string {
	if e.Err != nil {
		return fmt.Sprintf("[%s] %s: %v", e.Kind, e.Msg, e.Err)
	}
	return fmt.Sprintf("[%s] %s", e.Kind, e.Msg)
}

func (e *Error) Unwrap() error { return e.Err }

func newError(kind ErrorKind, err error, format string, args ...any) *Error {
	return &Error{Kind: kind, Msg: fmt.Sprintf(format, args...), Err: err}
}
