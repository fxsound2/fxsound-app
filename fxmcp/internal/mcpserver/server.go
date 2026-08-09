// Package mcpserver adapts FxSound's process/file-based interfaces
// (internal/fxsound) into MCP resources, tools, and prompts.
package mcpserver

import (
	"github.com/modelcontextprotocol/go-sdk/mcp"

	"fxmcp/internal/fxsound"
)

// App wires FxSound's process/file-based interfaces into MCP
// resources/tools/prompts registered on an *mcp.Server. It holds no
// per-connection state, so a single App can back any number of server
// sessions.
type App struct {
	Paths *fxsound.Paths
}

// New creates an App bound to the given resolved executable paths.
func New(paths *fxsound.Paths) *App {
	return &App{Paths: paths}
}

// Register adds all resources, tools, and prompts to s.
func (a *App) Register(s *mcp.Server) {
	a.registerResources(s)
	a.registerTools(s)
	a.registerWriteTools(s)
	a.registerEqTools(s)
	a.registerCompositeTool(s)
	a.registerPrompts(s)
}

// boolPtr is a small helper for the *bool fields in mcp.ToolAnnotations
// (DestructiveHint, OpenWorldHint), which distinguish "not set" from an
// explicit false.
func boolPtr(b bool) *bool { return &b }
