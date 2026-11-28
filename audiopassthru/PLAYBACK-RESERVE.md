# Adjustable output reserve

The Audio settings pane exposes the existing private output-reserve engine as
one public control: **0–300 ms**, in 1-ms steps, initially **90 ms**. Double-click
resets to 90 ms. The current process retains a changed target when the settings
dialog is reopened. This runtime setting is not written to JSON, presets or the
registry; restarting FxSound starts at 90 ms again.

The status beside the control shows the latest observed render queue and the
effectively applied target. It shows waiting for playback when no
recent render observation is available. Readback refreshes at 4 Hz while the
Audio pane is showing. This UI refresh does not poll the audio driver: it reads
atomic measurements published by the renderer.

## Signal path and latency

Capture returns a complete packet to the existing DSP/channel mapper instead of
waiting to accumulate half a buffer. Capture starts first; the first processed
packet primes the stopped physical output with the requested reserve, writes
the packet, then starts playback. At 48 kHz, a 90-ms reserve is 4,320 silent
frames; with an ordinary 480-frame packet, the initial queue is 4,800 frames.
An empty paused stream is primed again when audio returns.

The requested WASAPI buffer includes room for the 300-ms maximum and the existing
packet headroom. The capacity actually returned by WASAPI is authoritative. The
effective target cannot exceed capacity minus the current complete packet, so
some endpoints cannot provide the whole requested reserve. The displayed applied
target reports the requested target after that clamp; it is not the endpoint's
maximum supported reserve. Larger output capacity is not itself a promise of lower
latency or improved hardware stability.

A live user target change retains the existing finite correction debt and
temporary rate adjustment, limited to approximately 2%. It gradually adds or
removes the requested queued frames; it does not continually follow padding
jitter. These transitions can briefly affect pitch/time and take several seconds
for a large change. Once the target is stable, samples are copied byte-for-byte
without ongoing resampling. Stop requests and render failures leave through the
existing result/status contract; a stalled queue has a two-second no-progress
deadline. Stop is signalled before either capture/render COM Stop call, including
failed calls. The renderer checks it after priming and before starting playback;
if Stop arrived during Start, it stops the client again and reports failure when
that compensating Stop fails. These error paths log HRESULT diagnostics without
adding modal dialogs, including their immediate worker/control callers. A failed
COM Stop does not abandon the existing worker join: the kill request is set
first and the existing stop-or-timeout polling contract still runs. The
control-thread stop path does not mutate the renderer's
paused state or call Reset while another thread may still hold a render buffer.

More reserve adds output latency. It does not remove Bluetooth/HDMI codec delay,
advance audio that has not arrived or implement negative delay. This is related
to [#679](https://github.com/fxsound2/fxsound-app/issues/679) and
[#221](https://github.com/fxsound2/fxsound-app/issues/221), without claiming to
resolve those requests completely.

## Contribution boundary

The engine comes from the validated private implementation at d1ccaf6, with the
saved default-90-ms header changes. Its transition algorithm is retained.
PlaybackObserver/shared-memory instrumentation, experimental driver compensation,
driver installation, analysis/spectrogram/pitch features and the independent
audio-lifecycle rewrite are excluded. The GUI communicates with the statically
linked renderer through the existing inline atomic control; no driver API is
required to use the slider.

## Reproducible hardware-free regression

Build audiopassthru.lib and DfxDsp.lib from the same checkout first. Use a matching x64 Visual
Studio Developer PowerShell, toolset and CRT for each configuration:

```powershell
./audiopassthru/tests/Run-PlaybackReserveTests.ps1 `
    -Configuration Release -BuildRoot 'C:/validation/reserve' `
    -AudioLibrary '<absolute path to Release/audiopassthru.lib>' `
    -DspLibrary '<absolute path to Release/DfxDsp.lib>'
```

Repeat with Debug and the corresponding Debug library. The mandatory output
root must be outside this source checkout; build/results/stderr logs and the
fixture binary remain beneath it. The runner does not build the application,
launch FxSound, open an endpoint, read the user's profile or change default
devices. It times out only its own fixture process after 30 seconds.

The fixture links the actual production capture/render functions against
in-memory IAudioClient/IAudioCaptureClient/IAudioRenderClient implementations.
The fake capacity/queue are frames, and its storage includes all interleaved
channel scalars. It covers 44.1/48/96 kHz with 1/2/6/8 channels, default priming,
byte-identical plateaus, capped transitions, the preserved private reversal
scenario, short packets/re-prime, capacity clamp, split writes, stop between
writes, failed capture/render Stop, Stop during priming/Start, failed capture Start,
WASAPI failures and the real no-progress deadline. Two additional fixtures call
the actual killProcessingThread with a failed COM Stop and a real event-controlled
thread: its duplicated handle must still be joined/closed. Their owner has an
explicitly inert destructor, so no real restoration/free/profile path executes.
Production capture
packet/buffer guards and existing 48 -> 96-kHz frame repetition are also tested.
Expected negative cases emit debug diagnostics without a modal dialog.

These simulations do not establish real end-to-end latency, listening quality,
HDMI/USB/Bluetooth stability or driver underrun behavior. The new public GUI and
hardware behavior require separate interactive validation with a deliberately
selected instance; automated tests must not disturb the user's running engine.
