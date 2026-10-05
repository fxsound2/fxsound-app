# Project changelog

## 0.1.4 — Pro control refresh

- Corrected the native Pro issue left after 0.1.3: effects/EQ could remain disabled
  and grey despite acknowledged engine power until a repaint. macOS update now
  synchronizes enabled states before values/gradient/visibility, handles relevant
  model notifications and repaints the full Pro view only on power transitions.
  Enablement alone was insufficient because these custom controls did not repaint.
- Fresh native build/schema, 18 power and 18 control-state groups, and the existing
  13-preset/controller suite pass. In the actual macOS 27 window, three ON transitions
  without effects/EQ hover showed all 36 sliders enabled/red with values; OFF showed
  all 36 disabled/grey without labels. IPC confirmed the corresponding engine state.
- Source and read-only package checks pass: version, capture entitlements, signatures,
  minimum macOS 14, 13 factory presets and all 3,818 source files. Original Windows
  paint behavior/layout/assets are retained. No Metal/VBlank defect is claimed;
  native coverage remains limited to this candidate and host.

## 0.1.3 — power-button refresh

- Corrected a power-button cache gap after asynchronous engine-state updates:
  the macOS main-window update now copies acknowledged model power to the original
  button and repaints only when it changes. The same real-component fixture fails
  eight checks on the baseline and passes all 18 cache/original-SVG checks across
  Pro/Lite after the fix, without requiring mouse events or message pumping.
- The native app build, 27 state-schema checks and existing preset/controller/
  permission regressions pass. Read-only package inspection confirms version 0.1.3,
  signatures, audio-input entitlements, minimum macOS 14 and all 3,818 source bytes.
  Programmatic component rendering does not establish native compositor behavior;
  persistent hover/VBlank behavior has not been reproduced conclusively.
- This fixed the button cache but did not resolve the subsequently reproduced
  native Pro control enablement problem tracked for 0.1.4.

## 0.1.2 — macOS capture authorization

- Added the hardened-runtime audio-input entitlement to the app, engine and
  supervisor, including final package re-signing.
- Added native AVFoundation consent on explicit power ON. Capture helpers and
  routing wait for authorization; denied/restricted or canceled requests keep
  processing off. Preset/readiness/control acknowledgements precede activation.
- Corrected silent capture in the app launch context: on macOS 27 Apple Silicon
  with EarPods, native Allow was followed by user-confirmed audible output and
  measurable post-DSP signal from the same GUI-launched engine. Quit restored
  physical output. Other devices and native denial trials remain unverified.
- Native/controller regressions and 195 fully ASan/UBSan-instrumented DSP signal
  cases pass. The inspected personal package retains capture entitlements,
  minimum-OS metadata and matching source for all 3,816 archived files.

## 0.1.1 — macOS preset dropdown

- Enabled the original Pro/Lite preset dropdown when its catalog is nonempty,
  independently of processing power. Windows power gating and layout are retained.
- Deferred selection until engine readiness, serialized rapid changes and persisted
  only matching acknowledgements. Stale replies cannot overwrite the latest choice.
- Actual component/controller tests cover 13 factory presets, power-off selection,
  deferred startup, rapid changes and ACK/NACK handling without route activation.
- Verified the personal package payload and 3,811 corresponding source files.

## 0.1.0 — initial personal macOS port

- Added a Clang build of the original DSP with fixed-width ABI storage and portable
  platform utilities. Corrected a sanitizer-discovered high-rate tiny-block underread;
  that safe fallback requires Windows regression and numerical-parity follow-up.
- Added a stereo Float32/48 kHz HAL driver, CoreAudio engine, bounded adaptive
  resampling, private socket and supervised routing restoration.
- Reused original JUCE views, theme, fonts and assets with macOS controller adapters.
  Original effects, variable-band equalizer, presets and spectrum use real DSP.
- Added atomic preset storage, acknowledged settings and cached control snapshots.
  DSP edits temporarily emit dry audio while transport continues.
- Pinned JUCE 8.0.15 after JUCE 6.1.6 failed against the macOS 27 SDK.
- Added personal PKG/DMG, corresponding source/licenses and guarded uninstall.
  The initial archive contained 3,808 source files.

Windows-only hotkeys, updater, automatic device priority and app-managed startup
registration are unavailable in this target. Windows numerical parity, additional
OS/hardware coverage, latency/quality measurements and public signing/notarization
remain outstanding. See [port status](macos-port-status.md).
