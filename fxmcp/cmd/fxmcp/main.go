// Command fxmcp is an MCP server that exposes FxSound's diagnostics
// (fxdiag.exe) and control surface (FxSound.exe command-line options) as
// MCP resources, tools, and prompts.
//
// resource_windows_{386,amd64,arm64}.syso in this directory embed the
// Windows file properties (Copyright/FileDescription/ProductName/Version)
// shown on fxmcp.exe's Details tab. They're checked in since a plain `go
// build` needs them present to produce a binary with those properties --
// regenerate after editing versioninfo.json with (one-time prerequisite:
// go install github.com/josephspurrier/goversioninfo/cmd/goversioninfo@latest):
//
//go:generate goversioninfo -platform-specific versioninfo.json
package main

import (
	"context"
	"log"
	"os"

	"github.com/modelcontextprotocol/go-sdk/mcp"

	"fxmcp/internal/fxsound"
	"fxmcp/internal/mcpserver"
)

func main() {
	// The stdio transport uses stdout for the JSON-RPC channel, so all
	// diagnostic logging must go to stderr.
	logger := log.New(os.Stderr, "fxmcp: ", log.LstdFlags)

	paths, err := fxsound.Locate()
	if err != nil {
		logger.Printf("warning: %v (tools/resources requiring FxSound will fail until this is resolved)", err)
	} else {
		logger.Printf("resolved FxSound.exe=%s fxdiag.exe=%s", paths.FxSoundExe, paths.FxDiagExe)
	}

	server := mcp.NewServer(&mcp.Implementation{Name: "fxsound", Version: "0.1.0"}, nil)
	mcpserver.New(paths).Register(server)

	if err := server.Run(context.Background(), &mcp.StdioTransport{}); err != nil {
		logger.Fatalf("server exited: %v", err)
	}
}
