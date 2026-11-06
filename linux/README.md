# FxSound (Linux & Windows)

FxSound is an open-source digital audio enhancer built for Windows and Linux. The background processing engine acts as a high-fidelity digital audio enhancer for your system, providing clean signal passthrough, parametric equalization, and active dynamic effects for shaping clarity, bass, and volume.

---

## Linux Port Overview

On modern Linux systems running **PipeWire**, FxSound provides seamless system-wide audio processing:

* **Native PipeWire Integration**: Creates a high-fidelity virtual sink (`FxSound Audio Enhancer`) that captures application audio, routes it through the real-time DSP engine, and streams the enhanced signal to your active output device (speakers, headphones, USB DACs, or Bluetooth headsets).
* **Core DSP Engine**: The complete original 32-bit DSP pipeline (Clarity, Ambience, Surround Sound, Dynamic Boost, Bass Boost, and 9-band Parametric Equalizer) ported natively with fixed-width LP64 64-bit safety.
* **Desktop & System Tray Integration**: Native D-Bus StatusNotifierItem (SNI) system tray with power state indicator, theme icons, context menu, and desktop shortcuts.
* **Single Instance & Autostart**: Robust POSIX single-instance socket and XDG autostart support.

---

## Installation & Packages

Prebuilt Linux release packages are available on [GitHub Releases](https://github.com/pourih/fxsound-linux/releases):

* **Fedora / RHEL / openSUSE (`.rpm`)**:
  ```bash
  sudo dnf install ./fxsound-1.2.17-1.x86_64.rpm
  ```
* **Ubuntu / Debian (`.deb`)**:
  ```bash
  sudo apt install ./fxsound_1.2.17_amd64.deb
  ```
* **Universal Standalone (`.AppImage`)**:
  ```bash
  chmod +x fxsound-1.2.17-x86_64.AppImage
  ./fxsound-1.2.17-x86_64.AppImage
  ```

---

## Building from Source on Linux

### 1. Prerequisites & Dependencies

Install required build tools, PipeWire, JUCE dependencies, and X11 libraries:

**Fedora / RHEL:**
```bash
sudo dnf install -y gcc-c++ cmake ninja-build git pkgconf-pkg-config \
  pipewire-devel systemd-devel dbus-devel alsa-lib-devel \
  libX11-devel libXext-devel libXinerama-devel libXrandr-devel \
  libXcursor-devel libXcomposite-devel libXrender-devel \
  freetype-devel fontconfig-devel mesa-libGL-devel libcurl-devel
```

**Ubuntu / Debian:**
```bash
sudo apt update && sudo apt install -y build-essential cmake ninja-build git pkg-config \
  libpipewire-0.3-dev libsystemd-dev libdbus-1-dev libasound2-dev \
  libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev \
  libxcomposite-dev libxrender-dev libfreetype6-dev libfontconfig1-dev \
  libgl1-mesa-dev libcurl4-openssl-dev
```

**Arch Linux / Manjaro:**
```bash
sudo pacman -S --needed base-devel cmake ninja git pkgconf \
  pipewire systemd dbus alsa-lib \
  libx11 libxext libxinerama libxrandr libxcursor libxcomposite libxrender \
  freetype2 fontconfig mesa curl
```

### 2. Clone with Submodules
```bash
git clone --recurse-submodules https://github.com/pourih/fxsound-linux.git
cd fxsound-linux
```

### 3. Build with CMake & Ninja
```bash
cmake -S linux -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target FxSound -j$(nproc)
```

The resulting binary will be in `build/FxSound_artefacts/Release/FxSound`.

### 4. System-Wide Installation
```bash
sudo cmake --install build
```

### 5. Packaging
To build native `.rpm`, `.deb`, and `.tar.gz` packages using CPack:
```bash
cpack --config build/CPackConfig.cmake -B build/packages -G "DEB;RPM;TGZ"
```

To build a standalone `.AppImage`:
```bash
./linux/packaging/build-appimage.sh build
```

---

## Windows Information & Build Instructions

Please refer to the root [README.md](../README.md) for official Windows build instructions using Visual Studio 2022 and Projucer.

---

## License
FxSound is licensed under the [GNU Affero General Public License v3.0 (AGPL-3.0)](../LICENSE).
