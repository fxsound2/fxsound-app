# fxmcp

An MCP (Model Context Protocol) server that exposes [FxSound](https://fxsound.com)'s
diagnostics (`fxdiag.exe`) and control surface (`FxSound.exe`'s command-line
options, driven through `FxController::applyConfig`) as MCP resources,
tools, and prompts — so an AI assistant can inspect the current audio
environment and drive presets, EQ, effects, and output device selection.

Windows-only (it shells out to `FxSound.exe`/`fxdiag.exe` and uses the
Windows registry/process APIs to find and drive them).

## Requirements

- Windows
- [FxSound](https://fxsound.com) installed
- Go 1.26+ (only needed to build from source)

## Building

Release binaries for all three architectures FxSound itself ships for
(x64, x86, arm64) are built with `build.ps1`, which places each one at
`fxsound-app\bin\<arch>\fxmcp.exe` — alongside that architecture's
`FxSound.exe`/`fxdiag.exe`, matching `fxdiag.vcxproj`'s own
`copy $(TargetPath) ..\bin\$(PlatformTarget)\` post-build convention:

```powershell
cd fxmcp
.\build.ps1
```

Or a single architecture/configuration:

```powershell
.\build.ps1 -Platforms x64 -Configuration Debug
```

This produces self-contained `fxmcp.exe` binaries (the
`docs/COMMAND_LINE_OPTIONS.md` reference is embedded into each one at
build time — see
[Keeping the embedded docs in sync](#keeping-the-embedded-docs-in-sync)).
`-Configuration Release` (the default) strips debug symbols via
`-ldflags="-s -w"`; `Debug` keeps them.

For a quick one-off local build without the script (e.g. while
developing), a plain `go build -o dist\fxmcp.exe .\cmd\fxmcp` from within
`fxmcp\` also works and matches your host architecture.

## Running standalone

`fxmcp.exe` speaks MCP over stdio. Running it directly is mainly useful to
confirm it starts and resolves paths correctly — it will then sit waiting
for JSON-RPC input on stdin:

```powershell
..\bin\x64\fxmcp.exe
```

You should see a line on stderr like:

```
fxmcp: resolved FxSound.exe=C:\Program Files\FxSound LLC\FxSound\FxSound.exe fxdiag.exe=C:\Program Files\FxSound LLC\FxSound\fxdiag.exe
```

If instead you see a `warning: could not locate ...` line, `fxmcp` couldn't
find FxSound via the Program Files default location, the Windows uninstall
registry, or `PATH` — resources/tools that need it will fail until that's
resolved (reinstall FxSound, or check it's on `PATH`).

Press Ctrl+C to stop it.

## Integrating with Claude Desktop

1. Build `fxmcp.exe` (above), or use a released binary. Note its full path.
2. Open (or create) Claude Desktop's config file:
   ```
   %APPDATA%\Claude\claude_desktop_config.json
   ```
3. Add an entry under `mcpServers` (create that key if the file is new/empty):
   ```json
   {
     "mcpServers": {
       "fxsound": {
         "command": "C:\\Users\\you\\path\\to\\fxmcp.exe"
       }
     }
   }
   ```
   Use the absolute path to your built `fxmcp.exe`, with backslashes escaped
   as `\\` (JSON). If you already have other servers configured, add
   `"fxsound"` as another key alongside them rather than replacing the file.
4. Restart Claude Desktop completely (quit from the system tray, not just
   close the window) so it picks up the config change.
5. Verify it's connected: open Claude Desktop, look for a tools/MCP icon in
   the composer showing "fxsound" is connected (exact UI varies by
   version), or just ask it directly, e.g.:
   > "What FxSound tools do you have available?"

   Claude should list the tools described below (`fxsound_get_status`,
   `fxsound_set_power`, etc.) if the connection is working.
6. Try a real request, e.g. "Is FxSound running, and what preset is
   selected?" or "Switch my output device to my headphones" — Claude will
   call the relevant resources/tools and report back.

If Claude reports the server failed to start, check Claude Desktop's own
logs (Help menu, or `%APPDATA%\Claude\logs\`) for stderr output from
`fxmcp.exe` — the path-resolution warning mentioned above is the most
common cause.

## What's exposed

### Resources
| URI | Contents |
|---|---|
| `fxsound://diagnostics` | Windows playback device + audio session snapshot (from `fxdiag.exe --json`) |
| `fxsound://status` | Full live FxSound state: power, presets, output, equalizer, effects |
| `fxsound://presets` | Selected preset + built-in/user-defined preset lists |
| `fxsound://equalizer` | Band count, per-band frequency/gain, master gain, volume leveling, filter Q, balance |
| `fxsound://effects` | Clarity/ambience/surround/dynamicboost/bass levels (0-10 scale) |
| `fxsound://docs/command-line-options` | The authoritative option reference (ranges, rounding, preset-command rules) |

### Tools
- `fxsound_is_running`, `fxsound_get_status` — read-only
- `fxsound_set_power`, `fxsound_select_preset`, `fxsound_set_output_device` — single-setting, work whether or not FxSound is already running
- `fxsound_save_preset`, `fxsound_overwrite_preset`, `fxsound_undo_preset`, `fxsound_rename_preset`, `fxsound_delete_preset` — preset management, require a running instance
- `fxsound_set_eq_bands`, `fxsound_set_eq_band_gains`, `fxsound_set_effects` — batch EQ/effects changes, require a running instance
- `fxsound_apply_settings` — composite tool covering the full flag surface in one atomic call

### Prompts
- `diagnose-audio-issue` (arg: `symptom`)
- `tune-for-scenario` (arg: `scenario`)
- `switch-listening-profile` (arg: `profile`)
- `create-preset-from-current` (arg: `name`)

## Development

### Running tests

```powershell
go test .\... -v
```

Pure logic (`internal/fxsound`'s `Build*`/`Validate*`/`Sanitize*` functions)
is unit-tested normally and needs nothing running. Everything that touches
a real process spawn, the Windows registry, or `status.json` polling is
covered by integration tests in `cmd/fxmcp/*_test.go` that build the real
binary and drive it as an actual MCP client over stdio — these need
FxSound installed to run meaningfully, and several subtests additionally
skip themselves (rather than fail) if FxSound isn't currently running,
since some commands are only understood by a running instance. This
doesn't run meaningfully in a non-Windows CI.

`cmd/fxmcp/write_tools_test.go` in particular exercises every write tool
against your real, running FxSound instance and restores your original
power/preset/output/equalizer/effects settings afterward (best-effort) —
expect brief, visible changes to your actual FxSound app while it runs.

### Keeping the embedded docs in sync

`internal/mcpserver/docsdata/command_line_options.md` is a build-time copy
of `docs/COMMAND_LINE_OPTIONS.md` (`go:embed` can't reach outside this
module, so the canonical file living one directory up in the main
`fxsound-app` repo has to be mirrored in). After editing the canonical
file, refresh the copy and rebuild:

```powershell
go generate .\...
go build .\...
```

## Known limitations

- **Per-band EQ frequency range isn't validated client-side.** The valid
  frequency range for `--set_band_freq` is per-band and computed inside
  the DSP engine (`dfx_dsp_.getEqBandFrequencyRange`), which isn't exposed
  via `status.json` or any documented constant. An out-of-range frequency
  is still silently dropped by FxSound itself rather than rejected by this
  server. Band *index* bounds and gain range (`fxsound_set_eq_band_gains`)
  are fully validated.
- **No automation/trigger support yet** (e.g. "apply the gaming preset
  whenever a game launches"). That needs a persistent background
  watcher/rule engine, a different lifecycle than this server's normal
  host-spawned-per-session model, and was deliberately deferred — see the
  design discussion for the tradeoffs.
- **No tool to change a Windows app's own per-app mixer volume/mute**
  (e.g. un-muting Zoom directly) — only FxSound's own settings are
  controllable; diagnosing that kind of issue is supported (`fxsound://diagnostics`
  reports it), fixing it isn't.
