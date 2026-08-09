# fxmcp Claude Desktop Extension (.mcpb)

`manifest.json` here is the source of truth for fxmcp's [MCPB](https://github.com/modelcontextprotocol/mcpb)
bundle -- the one-click-installable connector Claude Desktop uses instead of
manually editing `claude_desktop_config.json`.

`icon.png` and `LICENSE` are **not** checked in here; `..\build-mcpb.ps1`
copies them fresh from their canonical locations (`fxsound\Images\fxsound_large.png`
and the repo's top-level `LICENSE`) on every build, so they can't drift out
of sync.

## Why this bundle carries no binary

`fxmcp.exe` is installed by the main FxSound installer at a fixed location
(`%ProgramFiles%\FxSound LLC\FxSound\fxmcp.exe`), so `manifest.json`'s
`server.mcp_config.command` points directly at that path rather than
bundling a copy under `${__dirname}`. This also means one `.mcpb` covers
all three architectures (x64/x86/arm64) fxmcp ships for -- the FxSound
installer already resolved which binary to install at build/install time,
and always places it at this same path.

The tradeoff: this only works for a default-location FxSound install. If
FxSound was installed somewhere else, this bundle's command path won't
find it (MCPB has no built-in variable for "Program Files" to make this
robust to a custom install directory).

## Building

```powershell
cd fxmcp
.\build-mcpb.ps1
```

Requires the `mcpb` CLI: `npm install -g @anthropic-ai/mcpb`. Produces
`dist\fxmcp.mcpb`.

## Verifying by hand

```powershell
mcpb validate mcpb\manifest.json
mcpb info dist\fxmcp.mcpb
```

Then install it into Claude Desktop via Settings -> Extensions -> Advanced
settings -> Install Extension (or double-click the file, though that relies
on Windows file association being registered, which isn't always reliable
on first install).
