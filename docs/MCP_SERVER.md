# FxSound MCP Server

FxSound includes an MCP ([Model Context Protocol](https://modelcontextprotocol.io)) server, `fxmcp.exe`, that lets AI assistants like Claude inspect and control FxSound directly. Once connected, you can ask things like:

- "Is FxSound running, and what preset is selected?"
- "Switch my output device to my headphones."
- "My Zoom call sounds quiet -- can you check why?"
- "Tune the equalizer for a podcast -- more clarity, less bass."
- "Save my current settings as a new preset called 'Racing Games'."

It exposes FxSound's diagnostics (device/session state) and its full command-line control interface (power, presets, equalizer, effects, output device) as MCP resources, tools, and prompts. Everything runs locally: it launches the already-installed `FxSound.exe`/`fxdiag.exe` and reads their local output -- no data is sent anywhere.

## Requirements

- Windows, with FxSound installed at its default location (`%ProgramFiles%\FxSound LLC\FxSound`)
- [Claude Desktop](https://claude.ai/download)

## Installing as a Claude Desktop extension

FxSound ships an installable extension bundle, `fxmcp.mcpb`, found in the FxSound install directory (`%ProgramFiles%\FxSound LLC\FxSound\fxmcp.mcpb`).

1. Open Claude Desktop.
2. Go to **Settings -> Extensions**.
3. Click **Advanced settings**, then **Install Extension**.
4. Browse to and select `fxmcp.mcpb`.
5. Review the extension details and confirm the install.
6. Restart Claude Desktop if prompted.

Alternatively, double-click `fxmcp.mcpb` in File Explorer (opens directly in Claude Desktop if the file association is registered), or drag it onto Claude Desktop's Settings window.

### Verifying it worked

Ask Claude:
> "What FxSound tools do you have available?"

It should list tools such as `fxsound_get_status`, `fxsound_set_power`, and `fxsound_select_preset`. Then try a real request, e.g. "Is FxSound running, and what preset is selected?"

### Troubleshooting

- **Install fails or FxSound's tools don't appear**: confirm FxSound is installed at the default location above -- the extension points at that fixed path and won't find a custom install directory.
- **Double-clicking the `.mcpb` does nothing**: use the Settings -> Extensions -> Install Extension route instead; the file association doesn't always register on the first install.

## More detail

See [fxmcp/README.md](../fxmcp/README.md) for the full list of resources/tools/prompts, and [COMMAND_LINE_OPTIONS.md](COMMAND_LINE_OPTIONS.md) for the underlying command-line options the server drives.
