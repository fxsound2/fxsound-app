# Testing

## Running
```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build -L unit        # portable, no audio server needed
ctest --test-dir build -L pw-live     # needs a PipeWire session + pactl/pw-link; skips (77) otherwise
```
Sanitizer builds (used during development, not wired into CI yet):
```sh
cmake -S . -B build-asan -G Ninja -DCMAKE_C_FLAGS=-fsanitize=address -DCMAKE_CXX_FLAGS=-fsanitize=address
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan -L unit
```
Every test sets its own `XDG_CONFIG_HOME` under the build tree. When running a
test binary by hand, set `XDG_CONFIG_HOME` too, or it writes `~/.config/fxsound/registry.ini`.

## Tests
| Test | Label | Covers |
|---|---|---|
| `dsp_selftest` | unit | DSP on/off and EQ change the output (16-bit path) |
| `dsp_selftest_perturb` | unit | same with `MALLOC_PERTURB_`: uninitialized members (D11) |
| `reg_roundtrip`, `reg_persisted` | unit | settings store write/read, UTF-8 load |
| `reg_malformed` | unit | truncated UTF-8 in `registry.ini` (D5), `_GLIBCXX_ASSERTIONS` |
| `reg_rmdir` | unit | recursive delete folder boundary (D10) |
| `process_selftest` | unit | RT block with the real DSP, float path: zero allocations (steady state, 8192 frames, rate change) (D1), output finite and within ±1, silence, mute, no-DSP copy |
| `dsp_settings_selftest` | unit | each control has its own registry key; effects do not flip power; power off/on; effect values; user preset save/load incl. name, eq and a non-ASCII name (D15–D18) |
| `routing_follows_default_sink` | pw-live | processor feeds the user's default sink (D3) |
| `app_exit_restores_default_sink` | pw-live | SIGTERM (logout/shutdown) hands the default sink back (D19) |
| `app_hotplug_updates_output_menu` | pw-live | with power on, added/removed sinks reach the tray output menu (D20) |
| `passthru_selftest` | (manual) | brings the nodes up in the live graph |

## End-to-end harness (manual, desktop session)
`linux/tests/fxtest.py <label> <launch command>` drives a running app through
the tray's dbusmenu (power, presets, output device, exit), records the
processor outputs and checks the PipeWire graph: startup, routing, processing
level, bypass, presets, stereo order, device selection and hot-plug, preset
persistence, default-sink restore (SIGTERM and Exit), 10 restarts, 60 s
playback CPU/xruns, a second instance, no PipeWire server. It mutes real sinks
while running and restores the default sink, mutes and test sinks on exit.
Stop any running FxSound first. Last run (2026-10-17, Release build of
`bugfix/linux-port-hardening`): 36 PASS, 0 FAIL. Original PR head: 20 PASS, 5 FAIL.

## Matrix and status
PASS = executed and passed · FAIL = executed and failed · BLOCKED = prerequisite missing · NOT RUN = not executed yet.

| Area | Check | Status |
|---|---|---|
| Build | fresh checkout, documented steps, Release, Fedora 44 | PASS |
| Build | Ubuntu 22.04 / 20.04 containers | see `BASELINE.md` CI notes |
| Packaging | `cmake --install` to DESTDIR, `desktop-file-validate` | PASS |
| Packaging | AppImage (`packaging/build-appimage.sh`), PKGBUILD on Arch | NOT RUN |
| DSP | float path finite/bounded, silence, mute, bypass copy | PASS |
| DSP | EQ magnitude at band centres vs reference, per-effect numerics | NOT RUN |
| RT safety | no allocation in the RT block | PASS |
| RT safety | no locks in the RT block | FAIL (reg mutex, DEC-3) |
| Concurrency | GUI setters vs processing under TSan | FAIL (69 races, DEC-4) |
| Backend | node lifecycle, links, default restore | PASS |
| Backend | default sink routing | PASS (was FAIL) |
| Backend | device hot-plug / removal, no server | PASS (hot-plug was FAIL) |
| Backend | PipeWire restart while running | NOT RUN |
| E2E | tone through processor (bypass and DSP) | PASS (manual script, not in ctest) |
| E2E | tray controls → DSP (power, presets, output), persistence across restart | PASS (`fxtest.py`) |
| E2E | main-window controls (EQ sliders, effect knobs) | NOT RUN (no window automation) |
| Reliability | 10x start/stop, 60 s playback (≈3 % CPU, 0 xruns) | PASS |
| Reliability | latency, hours-long playback | NOT RUN |
| Tray | SNI dbusmenu items | PASS |
| Hotkeys | X11 hotkeys | NOT RUN |
| Windows | shared-code changes build on MSVC (`u_DfxDsp.h`, `%s`→`%ls` and swprintf sizes in dsp/ and pstr/, `PT_PATH_JOIN_FMT`; all intended to be no-ops on MSVC) | BLOCKED (no Windows toolchain) |
