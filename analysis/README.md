# HighRes musical spectrum

**Make musical events easier to follow by sight.**

This optional spectrum view emphasizes attacks, sustained tones and stereo
differences, making overlapping musical activity easier to distinguish visually
while leaving the audio unchanged.

HighRes starts enabled. Its toggle restores the original spectrum. Left-channel
activity is drawn above the center and right-channel activity below it. The
selection currently lasts for the lifetime of the view; it is not a saved setting.
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
400 ms are hidden. Turning HighRes off disables analysis input. The worker stays
alive and polls its queue. This has a cost even when no spectrum is being computed.

The first version analyzes mono and stereo. Mono is mirrored. Other channel
layouts keep their existing audio behavior but are not analyzed by this view.
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

Tuning is localized in `AudioAnalyzer.h` (`BandSettings`, `AnalyzerConfig`),
`FrequencyScale.h` and `BarDynamics.h`. These parameters affect visualization
only. The standalone WAV laboratory is not required to build or use this feature.

## Build and review

Both Projucer files and the checked-in desktop/ARM app projects list the analysis
sources. Existing toolsets, dependency locations and build conventions are
retained. No VS2026 migration or personal filesystem paths are introduced.

For automated and manual checks, see [Testing](TESTING.md).
Review the PCM callback first, then the pure analyzer, the worker adapter and the
controller/view. Audio processing and buffering policies are outside this feature.
