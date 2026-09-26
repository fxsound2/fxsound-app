# Output peak and limiter indicators

The two lights under **L** and **R** show output sample peaks and the activity of
the existing final limiter. They are displayed with HighRes. They help explain
when the current processing settings require gain reduction.

| State | Meaning |
| --- | --- |
| Dark | No recent warning |
| Amber, gradually becoming red | At least 0.5 dB of limiter gain reduction; fully red at 6 dB |
| Red | Strong limiter reduction, or an output sample at/above the near-full-scale threshold |

The tooltip reports gain reduction per channel and separately states whether a
near-full-scale sample was detected. This distinction matters: limiter activity
can be substantial while every output sample remains below full scale.

Each event remains visible for 500 ms from the UI read. A quieter event does not
prolong an older stronger warning; it has its own hold time. Events are latched
until read, so even one offending sample can be reported between refreshes.

## Measurement boundaries

`ClipDetector` scans the raw post-DSP float PCM before FFT processing, weighting
or display filtering. Its per-channel comparison is
`abs(sample) >= 32767.0f / 32768.0f`. Values above full scale also count. Mono is
mirrored; unsupported channel layouts are not analyzed. A full analysis queue
cannot discard this raw peak warning because detection runs before enqueueing.

This is a **sample-peak warning**, not a true-peak meter, a measurement of hardware
output, or detection of clipping already present in a recording. Hitting the
threshold is a useful warning and does not prove audible distortion. The lights
do not measure input clipping before the effects chain.

Limiter instrumentation reads the final maximizer's existing envelope-to-ceiling
ratio, retaining the largest active reciprocal gain in each block. Conversion
to dB is `20 * log10(ratio)` on the UI side. This measures that limiter's dynamic
attenuation; it is not a meter of all gain changes in the processing chain.

## Review map

1. `Maxi16.c` / `Maxi32.c`: collect envelope ratios without changing the audio
   equations, look-ahead, thresholds or output samples.
2. `Play16.c` / `Play32.c`: forward the final maximizer's values through two
   designated slots in the existing auxiliary-meter array. Its layout is unchanged.
3. `Comsftwr.c`, COM headers and `Comwave.cpp`: retain peaks across internal DSP
   blocks and consume them once; bypassed blocks cannot repeat stale meters.
4. `DfxDsp` and `LimiterActivity.h`: publish per-channel peaks through lock-free
   atomics to the UI. Rebuild the DSP library and its consumers together.
5. `AnalysisStream`, `FxController`, `FxClipIndicator` and `FxVisualizer`: raw peak
   latching, consumer reads, UI clock/hold state and drawing.

There are no added allocations, locks, clocks or logarithms in the audio-side
meter publication. The UI hold queue is owned by the UI thread. Neither detector
changes the audio. The existing limiter, intermediate saturation and device
buffer sizes retain their upstream behavior in this contribution.

The new meter transport reports unavailable handles through plain error codes.
The host emits a one-time Windows debug diagnostic if a meter read fails, without
using the class's uninitialized logging sink. These observation APIs do not open
modal error windows; historical DSP diagnostics remain unchanged.

## Tests

The analysis suite also tests detector thresholds, channel independence,
single-sample retention, concurrent producer/consumer handoff, full-queue behavior
and the 500 ms hold using a supplied clock.

`dsp/tests/LimiterMeterTests.cpp` measures actual attenuation in both maximizer
variants, covers mono/stereo and transport through bypass, and exercises the
effects chain at 44.1, 48 and 96 kHz. It can write a deterministic float capture
for comparison with the uninstrumented parent. See [Testing](TESTING.md).
