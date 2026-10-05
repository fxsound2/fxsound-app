# FxSound macOS port status

The macOS target reuses the original Windows JUCE views/resources and FxSound DSP.
Version 0.1.6 supports a personal arm64 app build with a fixed stereo Float32/48 kHz
HAL transport. Generated installers are local build artifacts, not public downloads.
Windows numerical parity and a supported OS/hardware release matrix are outstanding.
Historical 0.1.2 capture and 0.1.3 button fixes remain separately qualified; the button
fix did not resolve Pro enablement/repaint. The 0.1.4 update/repaint fix passes focused
tests, three native no-hover ON transitions and package inspection.
The 0.1.5 loader now replays original postload effect/EQ setters; 39 baseline failures
pass optimized/SAN real-state tests. Affected suites/build pass; same-input settled
output matches the original caller sequence; final package inspection passes.
The 0.1.6 ICON_BIG configuration builds valid AppIcon.icns whose ic08 PNG exactly
matches original Windows artwork. Affected GUI tests and final package inspection pass.

## Toolchain and source inputs

| Input | Revision used for validation |
|---|---|
| App baseline | `d8e7a23d37ed5939c2a3090a1c1756c7f2500b17` |
| Resources submodule | `8f0385f015cd6b10026b2db6a0625a6db4de89b0` |
| Windows driver reference only | `c78fc6d031d16bd0a5dbbdff4871cfb8715d343d` |
| JUCE 8.0.15 | `91ad83ae34a81e0833b1a2b0866f54846370ae53` |

The validation host used Apple Silicon, macOS 27 and Xcode 27/macOS SDK 27.
JUCE 6.1.6 failed because `CGWindowListCreateImage` is unavailable in that SDK;
JUCE 8.0.15 is pinned instead. Deployment minimum 14.0 is encoded in the app and
helpers, but runtime compatibility on macOS 14/15/26 has not been established.
The driver builds x86_64/arm64/arm64e; those slices do not establish Intel app support.

## Build

Requires CMake >=3.22, Apple developer tools, Python 3 and a checkout of the pinned
JUCE source. Run from the repository root; set `JUCE_SOURCE` to that checkout.
The Makefiles generator is required by the current native component-test helper.

```sh
git submodule update --init --recursive
cmake -S macos -B build/macos -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 -DFXSOUND_JUCE_SOURCE="$JUCE_SOURCE"
cmake --build build/macos --target FxSound fxsound-gui-state-test
bash macos/driver/build.sh "$PWD/build/driver"
```

The app is generated at `build/macos/gui/FxSound_artefacts/Release/FxSound.app`.
The GUI build signs engine/supervisor first, then the complete app with the
minimal audio-input entitlement. Building alone does not install the driver or
change the host output. See [GUI notes](../macos/gui/README.md) and the
[driver contract](../macos/driver/README.md).

## Reproducible offline checks

The following exercise the real native components, local socket and actual driver
bundle. Device-read tests enumerate/read default devices but do not activate routing.
The component fixture uses the specified app's real resources, isolated preferences
and an inactive EngineClient worker; it does not operate live audio.

```sh
FXSOUND_TEST_IPC=1 FXSOUND_TEST_DEVICE_READ=1 sh macos/tests/build.sh
sh macos/tests/driver-build.sh "$PWD/build/driver/FxSound.driver"
build/macos/gui/fxsound-gui-state-test_artefacts/Release/fxsound-gui-state-test
python3 macos/tests/preset-dropdown-build.py "$PWD/build/macos/gui" \
  "$PWD/build/macos/gui/FxSound_artefacts/Release/FxSound.app"
python3 macos/tests/power-button-build.py "$PWD/build/macos/gui"
build/macos/gui/FxSound_artefacts/Release/FxSound.app/Contents/MacOS/FxSound --smoke-test
```

For production-DSP instrumentation, build separate optimized, ASan/UBSan and TSan
archives. CMake's DSP-only target does not require JUCE.

```sh
cmake -S macos/dsp -B build/dsp-opt -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
cmake --build build/dsp-opt
cmake -S macos/dsp -B build/dsp-asan -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 -DFXSOUND_SANITIZE=ON
cmake --build build/dsp-asan
cmake -S macos/dsp -B build/dsp-tsan -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
  -DCMAKE_C_FLAGS=-fsanitize=thread -DCMAKE_CXX_FLAGS=-fsanitize=thread
cmake --build build/dsp-tsan
sh macos/tests/dsp-build.sh "$PWD/build/dsp-opt/libfxsound-dsp.a" \
  "$PWD/build/dsp-asan/libfxsound-dsp.a"
FXSOUND_TEST_DEVICE_READ=1 sh macos/tests/engine-dsp-build.sh \
  "$PWD/build/dsp-opt/libfxsound-dsp.a"
sh macos/tests/dsp-preset-storage-build.sh "$PWD/build/dsp-asan/libfxsound-dsp.a"
sh macos/tests/dsp-preset-switch-build.sh "$PWD/build/dsp-opt/libfxsound-dsp.a"
FXSOUND_TEST_FLAGS="-fsanitize=address,undefined" sh macos/tests/dsp-preset-switch-build.sh "$PWD/build/dsp-asan/libfxsound-dsp.a"
sh macos/tests/dsp-audible-signal-build.sh "$PWD/build/dsp-asan/libfxsound-dsp.a"
sh macos/tests/dsp-concurrency-build.sh "$PWD/build/dsp-tsan/libfxsound-dsp.a"
sh macos/tests/dsp-edit-gate-build.sh "$PWD/build/dsp-tsan/libfxsound-dsp.a"
FXSOUND_TEST_ALLOCATIONS=1 sh macos/tests/dsp-edit-gate-build.sh \
  "$PWD/build/dsp-opt/libfxsound-dsp.a"
```

Passing checks observed during development include:

- Actual DSP rendering for 13 presets, 44.1/48/96 kHz and 2/4/6/8-channel layouts;
  high-rate tiny blocks, exact dry bypass, preset rejection/storage and EQ resizing.
- Fully instrumented ASan/UBSan and TSan tests, including concurrent edit/status
  polling. The signal test covers 195 preset/rate/EQ-count cases with finite nonzero
  settled RMS. Allocation interception records zero ordinary heap calls in the
  exercised warmed callback/control-gate branches, not every allocation mechanism.
- Actual socket framing/size/deadline/ownership and atomic recovery datagram tests;
  ring concurrency and 400-second simulated independent-clock skew cases.
- Actual isolated driver factory/vtable, delayed loopback, gain/mute, two clients
  and lifecycle/timestamp concurrency. Isolated loading is separate from coreaudiod.
- Both original Pro/Lite views expose 13 presets while power is off. Actual controller
  units cover deferred/latest selection, matching/stale ACK/NACK and power sequencing.
  Native permission mapping uses AVFoundation; event-state units do not simulate a
  real OS grant or certify native denied/restricted-user behavior.
- Native GUI smoke/render and 27 state-schema checks.
- The [power-button fixture](../macos/tests/power-button-test.cpp), built by the
  [focused helper](../macos/tests/power-button-build.py), links the actual app objects
  and original SVG resources. Across Pro/Lite, 18 cache/pixel checks pass after
  macOS update synchronization; the identical fixture linked to the baseline fails
  eight checks. It does not start helpers, request consent or operate audio, and
  validates component rendering rather than native compositor/hover behavior.

## Narrow native Pro refresh acceptance

On the same macOS 27 Apple Silicon host, the actual 0.1.3 window showed a red power
button and ready/unbypassed/routed engine but grey, disabled Pro controls for over
30 seconds, until a static-label click caused repaint. An intermediate candidate
set accessibility enabled states but remained visually grey, establishing that
state synchronization and explicit repaint were both required.

The final 0.1.4 build-path candidate passed initial ON plus two OFF→ON transitions
without hovering effects/EQ: all 36 sliders were enabled/red with values ON, and
all 36 were disabled/grey with value labels absent OFF. IPC confirmed routing and
bypass followed power. Existing installed 0.1.3 was untouched. The extended real
fixture passes 18 power checks and 18 control-state groups, with existing preset/
controller and 27 state-schema regression passing after the repaint change.
This is native interaction evidence on one host, not a Metal/VBlank diagnosis or
OS/hardware qualification. The final 0.1.4 package also passes read-only inspection.

## Narrow live capture acceptance

On macOS 27 Apple Silicon with EarPods, the previous GUI-launched engine captured
silence although the same engine under Terminal produced signal. The browser was
observed following the virtual default output. Version 0.1.2 adds native capture
consent on explicit power ON and audio-input entitlements to the hardened app,
engine and supervisor; no capture helper/routing starts before authorization.

The actual bundle launched through LaunchServices reported NotDetermined before
helpers started. After native Allow, audible processing was confirmed, and a known
0.0198-peak sine through that GUI-launched engine produced post-DSP peaks around
0.041. Quit restored physical output. This confirms the reproduced permission
incident, not the complete device/lifecycle matrix. No TCC database access/reset
or synthetic grant was used. App-managed Windows hotkeys, updater, automatic
output priority and startup registration are unavailable in this target.

## Personal packaging

```sh
bash macos/packaging/build-personal.sh \
  "$PWD/build/macos/gui/FxSound_artefacts/Release/FxSound.app" \
  "$PWD/build/driver/FxSound.driver" "$PWD/dist" "$JUCE_SOURCE"
```

Generated paths are `dist/FxSound-macOS-personal.dmg` and `dist/FxSound-Install.pkg`.
The package installs `/Applications/FxSound.app` and
`/Library/Audio/Plug-Ins/HAL/FxSound.driver`, requests administrator authorization
and restarts the audio service. Close the app with its menu bar Quit before upgrading;
closing the window alone leaves it running. Explicit power ON requests native audio
permission when needed. Ad-hoc signature changes may require consent again.

The inspected 0.1.2 package is unsigned; app/helper/driver bundles are ad-hoc signed.
Read-only mount/extraction verified versions, usage descriptions, retained capture
entitlements, strict nested signatures, every executable minimum 14.0, architecture
requirements, script/payload ownership and matching source. All 3,816 archived source
members matched the manifest and source bytes. Inspection did not run the installer.
Generated package timestamps/signatures mean later builds need not share these hashes.

Read-only 0.1.3 inspection confirms version metadata, capture entitlements, strict
signatures, all executable minimum 14.0, 13 factory presets and unchanged engine/
supervisor helper code. All3,818 archived source files match the current selected
source, including the focused power-button regression. The installer was not run.

Read-only 0.1.5 inspection confirms version, both capture usage keys, audio-input
entitlements, strict signatures, 6 Mach-O minimum14.0,13 presets and all 3,820 current
source members. Native unsigned code matches; historical results stay qualified.

| Locally validated 0.1.6 artifact | SHA-256 |
|---|---|
| DMG | `dcd55fa8ef148b0b82384cfb186012068f53153c5d25887359f99ea2e6b77bcb` |
| PKG | `9946fc39e61b60d8976212cf6971330782e7ca1ffd2233fdedf59d14f21c0dfd` |

## Outstanding qualification

Windows/MSVC build and full-render numerical parity are unverified. The short
high-rate block safety fallback changes previously undefined behavior; Windows
regression and block-partition equivalence still require measurement. macOS
14/15/26, Intel, other devices, native denied/restricted trials, multi-app soak,
hotplug, sleep/wake, live crash recovery and clean install/upgrade/uninstall matrix
remain unverified. End-to-end latency, display-off CPU and linear SRC quality have
not been qualified. Developer ID signing/notarization and public Gatekeeper
acceptance are not provided by this personal package.
