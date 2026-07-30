// Package docsdata embeds a mirrored copy of docs authored elsewhere in the
// fxsound-app repo, so the fxmcp binary ships self-contained.
//
// go:embed cannot reach outside this module tree, so command_line_options.md
// here is a copy of ../../../../docs/COMMAND_LINE_OPTIONS.md (the canonical
// source, maintained alongside FxController.cpp). After editing the
// canonical file, run `go generate ./...` from the fxmcp module root to
// refresh this copy.
package docsdata

import _ "embed"

//go:generate go run ../../../tools/copydocs -src=../../../../docs/COMMAND_LINE_OPTIONS.md -dst=command_line_options.md

//go:embed command_line_options.md
var CommandLineOptions string
