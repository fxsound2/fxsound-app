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
| `routing_follows_default_sink` | pw-live | processor feeds the user's default sink (D3) |
| `passthru_selftest` | (manual) | brings the nodes up in the live graph |

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
| Backend | device hot-plug / removal, PipeWire restart, no server | NOT RUN |
| E2E | tone through processor (bypass and DSP) | PASS (manual script, not in ctest) |
| E2E | GUI controls → DSP, presets, persistence across restart | NOT RUN |
| Reliability | repeated start/stop, sustained playback, CPU, latency | NOT RUN |
| Hotkeys / tray | X11 hotkeys, SNI menu | NOT RUN |
| Windows | shared-code changes (`u_DfxDsp.h`) build on MSVC | BLOCKED (no Windows toolchain) |
