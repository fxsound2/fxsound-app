# Building FxSound on Linux

### 1. Install Build Dependencies
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

### 2. Initialize Submodules
```bash
git submodule update --init --recursive
```

### 3. Configure and Build
```bash
# Configure Release build using Ninja
cmake -S linux -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Compile FxSound GUI
cmake --build build --target FxSound -j$(nproc)
```

### 4. Run FxSound
```bash
./build/FxSound_artefacts/Release/FxSound
```

### 5. Packaging & Installation
To install system-wide:
```bash
sudo cmake --install build
```
To generate distribution packages (`.deb`, `.rpm`, `.tar.gz`), use CPack:
```bash
cpack --config build/CPackConfig.cmake -B build/packages -G "DEB;RPM;TGZ"
```
To generate an AppImage, run:
```bash
./linux/packaging/build-appimage.sh build
```
