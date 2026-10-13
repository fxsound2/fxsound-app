# macOS system architecture

The macOS target retains the original JUCE interface and DSP alongside the existing
Windows implementation. Platform adapters replace the Windows controller/WASAPI
transport; the macOS build excludes those Windows transport sources.

```text
System audio → HAL virtual output (48 kHz stereo Float32)
             → timestamped driver loopback → CoreAudio input callback
             → SPSC ring → adaptive SRC at physical-output clock
             → original FxSound DSP → selected physical output

Original JUCE GUI → EngineClient worker → private Unix socket → engine controls
                 → supervisor → engine lifecycle/restoration
```

The AudioServerPlugIn provides one virtual output and one loopback input, with a
512-frame loopback delay and stamped/epoch-protected ring storage. It performs no
DSP and does not route directly to hardware. See the [driver contract](../macos/driver/README.md).
The engine uses the physical output as consumption clock master and bounded linear
adaptive resampling to absorb independent-clock drift. DSP runs after resampling
at the negotiated physical rate. No mastering-quality SRC claim is made.

The GUI compiles the original main window, Pro/Lite views, controls, dialogs, theme
and resources against pinned JUCE 8.0.15. A versioned private Unix socket carries
validated control commands and state; its worker keeps IPC off the message/audio
callback paths. Factory presets/translations are bundled, and user presets/settings
live in Application Support. Preset writes are atomic and preferences are committed
after engine acknowledgement. Preset selection remains available with power off;
a controller queue retains and serializes pending choices until the engine is ready.

Native AVFoundation capture authorization is required before starting capture
helpers or routing. Consent is requested only on explicit power ON while permission
is undetermined. Denied/restricted or canceled intent leaves processing off. The
hardened app, engine and supervisor retain the audio-input entitlement when signed.
Readiness, preset/restoration ACKs and successful unbypass precede route activation.

DSP control mutation uses a bounded edit handshake: callbacks pass dry audio while
control work completes, and cached controls/atomic spectrum isolate status reads
from processing. There is one processing owner because legacy DSP branches retain
process-wide state. Tests cover exercised allocation/concurrency paths rather than
certifying every branch or realtime deadline.

The supervisor restarts with bounded backoff. Routing ownership records the physical
output before activation and restores it on shutdown/crash only while the default
still belongs to FxSound, respecting subsequent user output changes. Real crash,
sleep/wake and hotplug recovery across devices remain to be qualified.

The personal installer places the arm64 app in `/Applications` and the driver in
`/Library/Audio/Plug-Ins/HAL`. Bundles are ad-hoc signed and the PKG is unsigned;
Developer ID/notarization are separate outstanding distribution requirements.
See [port status](macos-port-status.md) for build commands and verified scope.
