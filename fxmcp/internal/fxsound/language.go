package fxsound

import "golang.org/x/text/language"

// ValidateLanguageCode checks that code parses as a valid BCP 47 language
// tag (e.g. "en", "fr", "fi" -- the codes docs/COMMAND_LINE_OPTIONS.md
// documents for --language). FxController.cpp passes the value straight
// through to its locale machinery without validating it itself, so an
// invalid code doesn't error there either -- it just silently fails to
// change the display language. This catches an obviously malformed code
// (including one crafted to smuggle extra content into the command line)
// before it's ever sent.
func ValidateLanguageCode(code string) error {
	if _, err := language.Parse(code); err != nil {
		return newError(ErrKindValueRejected, err, "language code %q is not a valid language tag", code)
	}
	return nil
}
