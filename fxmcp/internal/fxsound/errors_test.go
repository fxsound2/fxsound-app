package fxsound

import (
	"errors"
	"testing"
)

func TestErrorKindIsDiscoverable(t *testing.T) {
	err := ValidateRange("gain", 99, -12, 12)

	var fxErr *Error
	if !errors.As(err, &fxErr) {
		t.Fatalf("expected errors.As to find *fxsound.Error, got %T: %v", err, err)
	}
	if fxErr.Kind != ErrKindValueRejected {
		t.Errorf("Kind = %v, want %v", fxErr.Kind, ErrKindValueRejected)
	}
}

func TestErrorKindStrings(t *testing.T) {
	tests := map[ErrorKind]string{
		ErrKindUnknown:                 "unknown",
		ErrKindAppNotFound:             "app_not_found",
		ErrKindRequiresRunningInstance: "requires_running_instance",
		ErrKindValueRejected:           "value_rejected",
		ErrKindTimeout:                 "timeout",
	}
	for kind, want := range tests {
		if got := kind.String(); got != want {
			t.Errorf("ErrorKind(%d).String() = %q, want %q", kind, got, want)
		}
	}
}

func TestErrorUnwrap(t *testing.T) {
	inner := errors.New("boom")
	err := newError(ErrKindTimeout, inner, "spawn failed")
	if !errors.Is(err, inner) {
		t.Error("expected errors.Is to find the wrapped inner error")
	}
}
