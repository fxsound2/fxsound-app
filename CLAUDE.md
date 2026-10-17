# FxSound — working notes for Claude Code

Linux port (PipeWire) of FxSound. Windows builds use the Visual Studio
solutions and are untouched by the CMake build. Details:
`docs/development/ARCHITECTURE.md`, `TESTING.md`, `DECISIONS.md`, `KNOWN_ISSUES.md`.

## Key paths
- `CMakeLists.txt`, `dsp/DfxDsp.cmake`, `audiopassthru/AudioPassthruLinux.cmake` — Linux build
- `audiopassthru/src/linux/AudioPassthruPipeWire.cpp` — PipeWire graph; `PipeWireProcess.h` — realtime block
- `dsp/` — DSP engine shared with Windows; `dsp/linux/` — Linux shims, `reg_linux.cpp` settings store
- `fxsound/Source` — JUCE GUI; `linux/` — tray, hotkeys, SysInfo

## Build and test
```sh
git submodule update --init Resources third_party/JUCE   # JUCE 6.1.6
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build
ctest --test-dir build -L unit       # always
ctest --test-dir build -L pw-live    # touches the live PipeWire graph; restores the default sink
```
Needs: cmake ≥ 3.20, ninja, libpipewire-0.3, gio-2.0, X11/xcb/freetype/fontconfig/GL/alsa dev packages
(full list in `linux/README.md`). On immutable hosts build inside a distrobox.
Run test binaries by hand only with `XDG_CONFIG_HOME` set, or they write `~/.config/fxsound`.

## Code graph (local, untracked)
`codegraph` indexes the tree into `.codegraph/` using `codegraph.json` (excludes `third_party/`,
`Installer/`, `bin/`, `release/`, `Resources/`, `fxdiag/`, `build*/`, BinaryData). Both are listed in
`.git/info/exclude`. Refresh with `codegraph sync`; inspect with `codegraph callers|callees|impact <symbol>`.

## Rules
- Realtime path (`on_process` → `FxProcessor` → `DfxDsp::processAudio`): no allocation,
  no file I/O, no blocking locks. `process_selftest` counts mallocs — keep it at 0.
- Test first: add or extend a ctest that fails, then fix. Hardware/graph tests get label
  `pw-live` and exit 77 when the session is missing. No sleeps as correctness checks.
- `dsp/` and `fxsound/Source` are shared with Windows: keep changes minimal, guard Linux-only
  code with `#ifdef _WIN32` / `PT_LINUX`, and say in the commit that Windows was not built.
- Match each module's style (CONTRIBUTING.md). Commits: conventional, lowercase.
- Record design choices in `docs/development/DECISIONS.md`.

## Git safety
- Never push to `origin` (fxsound2) or `pr-head` (ealtun21); no force-push, no history rewrite.
- Never amend, rebase or squash the original contributors' commits.
- No `reset --hard`, `clean`, or stash without explicit approval.

## Ask before
- Changing DSP engine behaviour or threading model (see DEC-4), public interfaces, or adding dependencies.
- Anything that changes the user's audio setup beyond what `pw-live` tests restore.
