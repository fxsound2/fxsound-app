# FxSound on Linux (PipeWire)

Linux port of FxSound. The Windows build (Visual Studio solutions under
`fxsound/Project`) is unchanged; this tree adds a CMake build that targets
Linux + PipeWire.

## Status

The app builds, installs, and runs on PipeWire-based distros (Arch, Fedora
34+, Ubuntu 22.04+, etc.).

**Note:** Global hotkeys require an X11 `XGrabKey` backend or an
xdg-desktop-portal `GlobalShortcuts` backend for Wayland. Tracked for a
follow-up PR.

## Install

### AppImage (recommended — no install needed)

```sh
# Build locally:
./packaging/build-appimage.sh
./FxSound-1.2.8.0-x86_64.AppImage

# Or grab the AppImage from the GitHub Actions artifacts / release page.
```

### System install

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

### Arch / CachyOS (AUR)

```sh
cd packaging
makepkg -si
```

## Build from source

```sh
# One-time: fetch JUCE 6.1.6 and the Resources submodule
git submodule update --init Resources
git clone --depth 1 --branch 6.1.6 https://github.com/juce-framework/JUCE third_party/JUCE

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/FxSound
```

### Dependencies (Arch / CachyOS)

```sh
sudo pacman -S --needed base-devel cmake ninja \
  libx11 libxext libxinerama libxrandr libxcursor \
  freetype2 fontconfig mesa alsa-lib \
  xcb-util-keysyms libxcb \
  glib2 pipewire
```

### Dependencies (Ubuntu / Debian)

```sh
sudo apt-get install -y \
  cmake ninja-build \
  libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev \
  libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev libasound2-dev \
  libxcb1-dev libxcb-keysyms1-dev libglib2.0-dev libpipewire-0.3-dev
```

## CI

GitHub Actions builds an AppImage on every push and pull request using an
`ubuntu-20.04` runner (glibc 2.31 baseline). The workflow is at
`.github/workflows/build-appimage.yml`. Artifacts are uploaded for 30 days;
tagged releases (`v*.*.*.*`) produce a draft GitHub Release automatically.

## Layout of the Linux port

**Build system**
- `CMakeLists.txt` — root Linux CMake build. Replicates the Projucer export
  and links DfxDsp + AudioPassthru.
- `dsp/DfxDsp.cmake` — builds the real DSP + shared util sources.
- `audiopassthru/AudioPassthruLinux.cmake` — builds the PipeWire backend.
- `packaging/fxsound.desktop` — freedesktop desktop entry.
- `packaging/PKGBUILD` — AUR package recipe.
- `packaging/build-appimage.sh` — AppImage builder (downloads linuxdeploy +
  appimagetool automatically).
- `.github/workflows/build-appimage.yml` — CI workflow.

**Linux support layer**
- `linux/win_compat.h` — minimal Win32 *type* aliases so Windows-typed GUI
  declarations parse on Linux. Types only; no faked Win32 functions.
- `linux/SysInfo_linux.cpp` — Linux `SysInfo` (the Windows version is COM-based).
- `linux/LinuxHotkeys.cpp/.h` — global hotkey stub (pending XDG portal backend).
- `linux/stubs/` — stub `AudioPassthru` and `DfxDsp` used when the real
  backends are disabled via CMake options (see below).

**Tray icon**
- `linux/tray/fx_tray_sni.h` / `fx_tray_sni.cpp` — `org.kde.StatusNotifierItem`
  + `com.canonical.dbusmenu` over GDBus. Runs its own GLib main loop on a
  background thread. Left-click toggles the main window; the dbusmenu serves
  the full context menu to Wayland panels (Waybar, KDE Plasma) natively —
  no JUCE popup required. Menu refreshes automatically on model state changes.

**DSP Linux support layer** (`dsp/linux/`)
- `windows.h`, `conio.h`, `dos.h`, `direct.h`, `io.h`, `share.h`, `shlobj.h`,
  `Shlwapi.h`, `winbase.h`, `mmsystem.h` — stand-ins for Windows SDK headers.
- `lc_headers/` — symlinks bridging the DSP's inconsistent include casing to
  the real (case-sensitive) header files.
- `reg_linux.cpp` — in-process key/value store replacing the Windows registry.
- `slout_linux.cpp`, `file_linux.cpp`, `pwav_convert_linux.cpp` — Linux
  implementations of the logger, file helpers, and sample converters.
- `test_dsp.cpp` — the `dsp_selftest` binary.

Windows-only code paths in shared GUI sources are guarded with `#ifdef _WIN32`.

## CMake options

| Option | Default | Purpose |
|--------|---------|---------|
| `FXSOUND_STUB_DSP` | `OFF` | Use stub DfxDsp instead of the ported engine |
| `FXSOUND_STUB_AUDIO` | `OFF` | Use stub AudioPassthru instead of PipeWire |
