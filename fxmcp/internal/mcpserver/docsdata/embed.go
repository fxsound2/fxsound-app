// Package docsdata embeds a mirrored copy of docs authored elsewhere in the
// fxsound-app repo, so the fxmcp binary ships self-contained.
//
// go:embed cannot reach outside this module tree, so command_line_options.md
// here is a copy of ../../../../docs/COMMAND_LINE_OPTIONS.md (the canonical
// source, maintained alongside FxController.cpp). After editing the
// canonical file, run `go generate ./...` from the fxmcp module root to
// refresh this copy.
//
// The copy is a plain PowerShell Copy-Item rather than a small Go helper run
// via `go run`: that compiles a fresh, unsigned exe into the Go build cache
// on every run, and antivirus products (McAfee here) flag it as a false
// positive and refuse to execute it, which breaks the build.
package docsdata

import _ "embed"

//go:generate powershell -NoProfile -NonInteractive -Command "Copy-Item -LiteralPath '../../../../docs/COMMAND_LINE_OPTIONS.md' -Destination 'command_line_options.md' -Force -ErrorAction Stop"

//go:embed command_line_options.md
var CommandLineOptions string
