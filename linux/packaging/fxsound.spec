Name:           fxsound
Version:        1.2.17
Release:        1%{?dist}
Summary:        FxSound audio enhancer (Linux port, PipeWire)
License:        AGPL-3.0-or-later
URL:            https://github.com/pourih/fxsound-linux
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++ cmake ninja-build pkgconf-pkg-config
BuildRequires:  pipewire-devel systemd-devel dbus-devel alsa-lib-devel
BuildRequires:  libX11-devel libXext-devel libXinerama-devel libXrandr-devel
BuildRequires:  libXcursor-devel libXcomposite-devel libXrender-devel
BuildRequires:  freetype-devel fontconfig-devel mesa-libGL-devel libcurl-devel

Requires:       pipewire wireplumber pipewire-utils

%description
System-wide audio enhancer. Apps play to the "FxSound Audio Enhancer" null
sink (PipeWire), DSP processes, output goes to the real device. JUCE GUI.
Combined work is AGPL-3.0-or-later (app AGPL + JUCE AGPL; see LICENSE).

%prep
%autosetup -n %{name}-%{version}

%build
cmake -S linux -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target FxSound

%install
DESTDIR=%{buildroot} cmake --install build --prefix %{_prefix}

%files
%{_bindir}/FxSound
%{_datadir}/applications/fxsound.desktop
%{_datadir}/metainfo/com.fxsound.FxSound.metainfo.xml
%{_datadir}/icons/hicolor/*/apps/fxsound.*
%{_datadir}/fxsound/Factsoft/
%{_datadir}/systemd/user/fxsound.service
%license LICENSE

%changelog
* Thu Oct 16 2025 Linux Port <port@localhost> - 1.2.17-1
- Initial Fedora 44 PipeWire port (see linux/docs/linux-port/).
