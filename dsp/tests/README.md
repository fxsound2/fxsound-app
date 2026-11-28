# Float-headroom numerical regression

`FloatHeadroomTests.cpp` links the actual DSP library. It creates only low-level
SOS handles and DSP parameter/state/memory buffers. It never constructs DfxDsp,
FxController, a GUI, a user profile, a registry-backed host or an audio endpoint.
The calls use valid numerical API inputs and null diagnostic sinks; no intentional
error/MessageBox path is exercised. The runner bounds the process to 30 seconds.

Coverage:

- 20 Hz transients beyond the old intermediate ceiling, mono/stereo/6/8 channels,
  44.1/48/96 kHz, in-place and separate buffers. A fitted common gain ramp tests
  waveform preservation without duplicating the leveling detector. A later gain
  reduction verifies that no premature flattened peaks need reconstruction.
- LFE remains excluded from surround leveling; disabled leveling, exact silence,
  existing MasterGainOnly and below-ceiling processing remain covered.
- The real Play/Maximizer chain still limits its final output. The real PCM24
  converter retains integer saturation. These checks do not certify every legacy
  integer conversion, global EQ bypass, extreme/non-finite floats or hardware.

Run in an **x64 Visual Studio Developer PowerShell**, with a DSP static library
built using the same toolset and corresponding Debug/Release CRT. For example:

```powershell
./dsp/tests/Run-FloatHeadroomTests.ps1 `
  -BuildRoot 'C:/validation/headroom' -Configuration Release `
  -DspLibrary 'C:/validation/libs/Release/DfxDsp.lib'
```

First preserve a public baseline library before rebuilding the modified DSP. Run
the exact same fixture with that baseline and `-ExpectClipped`: it must demonstrate
the old bounded plateau and distortion of the gain ramp, not falsely claim the
baseline preserves headroom. The modified library must pass without that switch.
Repeat both runs with Debug libraries and `-Configuration Debug`.

Products and logs are isolated under
`<BuildRoot>/FloatHeadroomTests/x64/<Configuration>/{Headroom,BaselineClipped}`.
`inputs.json` records the compiler, library and fixture hashes. Compare
`unchanged-paths.f32` byte-for-byte between the two modes in each configuration.
That capture contains disabled-leveling, MasterGainOnly, silence, below-ceiling
leveling and direct Play/final-limiter outputs; the deliberately changed
over-ceiling SOS cases are excluded from that equality claim. For example:

```powershell
$root = 'C:/validation/headroom/FloatHeadroomTests/x64/Release'
$before = [IO.File]::ReadAllBytes("$root/BaselineClipped/unchanged-paths.f32")
$after = [IO.File]::ReadAllBytes("$root/Headroom/unchanged-paths.f32")
[Linq.Enumerable]::SequenceEqual[byte]($before, $after)
```

The two runs intentionally use different expected over-ceiling behavior. For a
negative regression, running the baseline without `-ExpectClipped` must fail its
headroom cases. This is a numerical assertion failure, not an invalid API call.
No listening, real-device stability or end-to-end latency result is implied.
