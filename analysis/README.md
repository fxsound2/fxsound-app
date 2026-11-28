# HighRes musical spectrum

**Make musical events easier to follow by sight.**

This optional spectrum view emphasizes attacks, sustained tones and stereo
differences, making overlapping musical activity easier to distinguish visually
while leaving the audio unchanged.

The classic spectrum is the initial selection. The flip button selects HighRes
and saves the preference. HighRes draws left-channel activity above the center
and right-channel activity below it.
The bars use the existing spectrum theme colors.

## Scope and integration

| Component | Responsibility |
| --- | --- |
| `AudioAnalyzer` | Synchronous C++17 analysis of borrowed interleaved float PCM; no JUCE, devices, UI or files |
| `AudioPassthruCallback::onAudioBlock` | Read-only tap after DSP, before output conversion; rebuild the callback library and consumers together |
| `runtime/AnalysisStream` | Bounded preallocated audio queue, worker thread, continuity checks and snapshots |
| `BarPeakAccumulator` | Per-channel maxima retained until a display consumes them |
| `FxController` / `FxVisualizer` | Host bridge, mode toggle, presentation and pending peaks until an actual repaint |

The audio callback copies PCM into the queue without allocation or waiting for
the analysis worker. Analysis and snapshot locking happen off the audio thread.
Dropped input or a format change resets analysis history; results older than
400 ms are hidden. Analysis runs while HighRes is visible and output is active.
The worker sleeps while analysis is disabled and wakes on enable or shutdown.

The first version analyzes mono and stereo. Mono is mirrored. Other channel
layouts use the classic spectrum. Returning to mono or stereo restores the saved selection.
This is a perceptual display, not note transcription, source separation, a
calibrated loudness meter or an invertible representation of the waveform.

## Display calculation

At 48 kHz, the default periodic-Hann FFT contains 2,048 samples (42.67 ms), with
a 512-sample hop (10.67 ms, 75% overlap). These are analysis-window durations;
they do not add a delay to audio playback. FFT size and hop can be changed in
`AnalyzerConfig` when the same component is used by an experimental host.

The 100 bands use a shifted logarithmic frequency axis with compressed treble.
Precomputed Gaussian weights overlap smoothly and have a minimum width in FFT
bins. Each channel's display amplitude is `(sum(w * abs(X)^4))^(1/4)`, with
weights normalized to sum to one. This favors strong partials within a band
without simply selecting one bin. Low-frequency bars still share information
where the FFT cannot resolve their separation.

A frequency-dependent dB floor, local spectral contrast, stereo contrast and an
attack/sustain envelope produce the final heights. Two accumulators retain peaks
across worker deliveries and coalesced UI updates. Raw FFT data and reference
band measurements remain available independently of these display operations.

Display tuning is grouped into `static constexpr` constants:

| Group / file | Controls |
| --- | --- |
| `BarSettings` / `BarDynamics.h` | Display dynamic range (dB floor, frequency-dependent floor adjustment and gain), decay, peak retention, attack/sustain balance and stereo contrast |
| `BandSettings` / `AudioAnalyzer.h` | Frequency-weighting widths and local spectral contrast |
| `FrequencySettings` / `FrequencyScale.h` | Frequency limits, shifted logarithmic mapping and treble compression |

These are compile-time parameters; changing them currently requires rebuilding.
They are not exposed as runtime user settings. Selected controls can be moved to
Advanced Settings, with appropriate defaults and ranges, if the maintainer
prefers. That runtime configuration change is open for discussion and is not
implemented in this submission. FFT format and hop are supplied separately
through `AnalyzerConfig`.

These parameters affect visualization only.

## Build and review

The Windows Projucer file and unified checked-in application project list the
analysis sources. Existing toolsets, dependency locations and build conventions are
retained. No VS2026 migration or personal filesystem paths are introduced.

For automated and manual checks, see [Testing](TESTING.md).
The companion [output indicators](METERS.md) report sample peaks and existing
limiter activity independently of the visual FFT and envelopes.
Review the PCM callback first, then the pure analyzer, the worker adapter and the
controller/view. Audio processing and buffering policies are outside this feature.
