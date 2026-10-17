# Known issues and limitations (Linux port)

| ID | Issue | Evidence | Status |
|---|---|---|---|
| D2 | GUI thread mutates DSP state while the RT thread processes; torn filter coefficients possible | TSan, 69 races | open, DEC-4 |
| D3b | Registry mutex taken on the RT thread every block | code + gdb trace | mitigated, DEC-3 |
| D4 | `reroute()` destroys and recreates all links on every node/port event; possible glitches on hot-plug | code | open |
| D6 | `init()` returns 0 even if the processor node id never arrives; `onSoundDeviceChange` is called on the PipeWire loop thread | code | open |
| D7 | Stereo only (FL/FR); multichannel sinks get two channels | code | open |
| D12 | `Prelst.cpp:69/76` passes `sizeof(pointer)/sizeof(wchar_t)` to `swprintf`, truncating the factory/user preset dirs inside the prelst handle (upstream, all platforms) | code | open; handle is currently never created (`preset_list_handle_` unused) |
| D13 | `registry.ini` format is `key=value` per line; values containing a newline would corrupt the file | code | open |
| D14 | The audiopassthru copies of `pt_defs.h`, `reg.h`, `File.h` and others differ from the DSP copies (`long` vs `int32_t`) | diff | build now picks the DSP copies; duplicates remain |
| — | App's own default-sink restore on exit not isolated from WirePlumber's fallback | baseline run | needs a test |

## Found by end-to-end testing (2026-10-17), all fixed on `bugfix/linux-port-hardening`
| ID | Issue (present on the original PR head) | Fix |
|---|---|---|
| D15 | Wide `%s` printed one character on glibc: every registry key collapsed to one letter (`S\D\13\23\L\b`), effect toggles overwrote bypass, so the DSP ran bypassed while the UI showed ON; settings collided | `%s` → `%ls` in wide formats |
| D16 | `swprintf(p, sizeof(p)/sizeof(*p), …)` on pointers truncated EQ values, preset comments, file names (22 sites) | real buffer sizes |
| D17 | User presets saved as `Presets\name.fac` next to the folder, and without their name (narrow `fprintf` on a wide-oriented stream) | `PT_PATH_JOIN_FMT`, byte `fwprintf` shim |
| D18 | Non-ASCII preset names and paths (e.g. `/home/josé`) failed: UTF-8 conversion went through the "C" locale | explicit UTF-8 codec |
| D19 | SIGTERM (logout/shutdown) killed the app before shutdown: default sink stayed "FxSound", preset choice not saved | signal → `systemRequestedQuit()` |
| D20 | With power on, device hot-plug never reached the output list/menu | Linux path posts to the message thread |
| D21 | Uninstalled build found no factory presets (cwd is set to the exe dir) | `FXSOUND_SOURCE_PRESET_DIR` |

Still open: two instances share the FxSound sink name; the second instance's exit can reset the default (seen manually, not reproduced by `fxtest.py` T11).
