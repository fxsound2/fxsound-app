# FxSound on Linux (PipeWire)

Linux port of FxSound. The Windows build (Visual Studio solutions under
`fxsound/Project`) is unchanged; this tree adds a CMake build that targets
Linux + PipeWire.

## Status (milestones)

- [x] **M0** — Linux build bootstrap. JUCE GUI compiles and launches; DSP and
  audio backends are stubbed (no audio yet).
- [x] **M1** — Port DfxDsp. The real DSP engine builds and runs on Linux; the
  app defaults to it. Verified with `dsp_selftest` (processAudio responds to
  power and EQ changes). Build the test with `-DFXSOUND_STUB_DSP=OFF` and run
  `./build/dsp_selftest`.
- [x] **M2** — PipeWire backend. A stereo `FxSound` node (`Audio/Sink`) created
  via libpipewire with input + output ports and a realtime callback. Verified
  with `passthru_selftest` (`pw-cli ls Node`, `pw-link`).
- [x] **M3** — DSP wired into the realtime callback (interleave → int16 →
  `DfxDsp::processAudio` → float). Verified end-to-end: a tone played to the
  FxSound sink is processed and reaches the hardware device.
- [x] **M4** — Device management. Registry-driven sink enumeration
  (`getSoundDevices`), selection (`setAsPlaybackDevice`) and default routing
  (`restoreDefaultPlaybackDevice`), output ports linked to the chosen device via
  the PipeWire link factory, and hot-plug add/remove → `onSoundDeviceChange`.
  Verified: FxSound output auto-links to the real ALSA device.
- [x] **M5** — GUI platform parity. Tray icon via JUCE `SystemTrayIconComponent`
  (left-click toggles window, right-click menu, icon dims when off); autostart
  via `~/.config/autostart/fxsound.desktop`; signal-based crash handler
  (`sigaction` + `backtrace_symbols_fd` → `~/.config/FxSound/fxsound-crash.log`,
  then re-raise for a core dump). **Deferred:** global hotkeys — the settings
  UI/persistence is cross-platform, but OS-level key grabbing needs an X11
  `XGrabKey` backend *and* an xdg-desktop-portal GlobalShortcuts backend
  (Wayland). Tracked for a follow-up.
- [x] **M6** — Packaging. CMake `install()` rules (freedesktop layout:
  `/usr/bin/fxsound`, `.desktop`, hicolor icon, presets under
  `/usr/share/fxsound`) — verified by a staged `DESTDIR` install; the `.desktop`
  passes `desktop-file-validate`. AUR `packaging/PKGBUILD` provided. **Follow-up:**
  AppImage/Flatpak recipes; resolve installed-app preset path (see M7).
- [x] **M7 (core)** — Preset handling fixed for Linux: factory presets resolved
  from `<cwd>/Factsoft` → exe-relative `../share/fxsound/Factsoft` →
  `/usr/share`, and the Windows `FxSound\Presets` backslash paths made portable
  (`File::getSeparatorString()`). Verified: installed binary launches from a
  neutral cwd. **Ongoing QA:** PipeWire latency/quantum tuning, sample-rate
  switching, suspend/resume, multi-distro testing.

## Install

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build      # or: DESTDIR=/tmp/stage cmake --install build
```

Arch users can build a package with `makepkg` from `packaging/PKGBUILD`.

## Build

```sh
# One-time: fetch JUCE 6.1.6 and the Resources submodule
git submodule update --init Resources
git clone --depth 1 --branch 6.1.6 https://github.com/juce-framework/JUCE third_party/JUCE

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/FxSound
```

### Toolchain (Arch/CachyOS)

```sh
sudo pacman -S --needed base-devel cmake ninja \
  libx11 libxext libxinerama libxrandr libxcursor \
  freetype2 fontconfig mesa alsa-lib pipewire
```

## Layout of the Linux port

- `CMakeLists.txt` (repo root) — Linux build. Replicates the Projucer export
  for the JUCE GUI and links the DSP + audio-passthru libs.
- `linux/win_compat.h` — minimal Win32 *type* aliases so Windows-typed GUI
  declarations parse on Linux. Types only; no faked Win32 functions.
- `linux/stubs/` — M0 stub `AudioPassthru` (replaced in M2).
- `linux/SysInfo_linux.cpp` — Linux `SysInfo` (the Windows one is COM-based).
- `dsp/DfxDsp.cmake` — builds the real DSP + shared util sources on Linux.
- `dsp/linux/` — DSP Linux support layer:
  - `windows.h`, `conio.h`, `dos.h`, `direct.h`, `io.h`, `share.h`, `shlobj.h`,
    `Shlwapi.h`, `winbase.h`, `mmsystem.h` — stand-ins for Windows SDK headers
    the DSP includes (mostly typedefs + a few POSIX-backed helpers).
  - `lc_headers/` — generated symlinks bridging the DSP's inconsistent include
    casing to the real (case-sensitive) header files.
  - `reg_linux.cpp` — in-process key/value store replacing the Windows registry
    settings backend (config-file persistence is a follow-up).
  - `slout_linux.cpp`, `file_linux.cpp`, `pwav_convert_linux.cpp` — Linux
    implementations of the logger, file helpers, and sample converters.
  - `test_dsp.cpp` — the `dsp_selftest` target.

Windows-only code paths in the shared GUI sources are guarded with
`#ifdef _WIN32`; the Linux `#else` branch is either a portable JUCE call or a
clearly-marked stub deferred to a later milestone (grep for `M5`, `M1`, etc.).

## Build options

- `-DFXSOUND_STUB_DSP=OFF` — build the real ported DSP (M1+).
- `-DFXSOUND_STUB_AUDIO=OFF` — build the PipeWire backend (M2+).
