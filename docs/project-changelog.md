# Project changelog

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
