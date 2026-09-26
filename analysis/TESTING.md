# Testing the HighRes spectrum

## Numerical and lifecycle tests

`tests/AudioAnalysisTests.cpp` exercises FFT normalization and continuity, smooth
band weighting, order-four aggregation, the frequency axis, display dynamics,
peak retention, queue overflow, reset/recovery and stale-frame handling. Tests
generate their own signals and do not open an audio device or alter user settings.
The executable writes JSON and returns a nonzero exit code on failure.

From a Visual Studio 2022 x64 **Developer PowerShell**, set `$repo` to this checkout
and choose an empty build directory outside all source trees. For example:

```powershell
$repo = (Get-Location).Path
$build = Join-Path $env:TEMP ('fxsound-analysis-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $build | Out-Null
Push-Location $build
try {
    cl /nologo /std:c++17 /EHsc /O2 /MT /DNOMINMAX `
        "$repo/analysis/tests/AudioAnalysisTests.cpp" `
        "$repo/analysis/AudioAnalyzer.cpp" `
        "$repo/analysis/runtime/AnalysisStream.cpp" /Fe:AudioAnalysisTests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
    & ./AudioAnalysisTests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
} finally { Pop-Location }
```

For Debug, replace `/O2 /MT` with `/Od /Zi /MTd`. The tests have no JUCE dependency.
Build the application using the repository's normal JUCE/Visual Studio workflow;
rebuild the audio callback library with its consumers after the interface change.

## DSP meter regression and audio comparison

Build `DfxDsp` in x64 Release first. In an x64 Developer PowerShell, use an empty
external `$build` directory as above and set `$dspLibrary` to that build's
`DfxDsp.lib`. Compile the meter tests there:

```powershell
Push-Location $build
try {
    cl /nologo /std:c++17 /EHsc /O2 /MT /DWIN32 /DUNICODE /D_UNICODE `
        /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /I"$repo/dsp/ptutil/include" `
        "$repo/dsp/tests/LimiterMeterTests.cpp" "$dspLibrary" `
        advapi32.lib user32.lib ole32.lib shell32.lib shlwapi.lib winmm.lib uuid.lib `
        /Fe:LimiterMeterTests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
    & ./LimiterMeterTests.exe ./instrumented.f32
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
} finally { Pop-Location }
```

For Debug, use `/Od /Zi /MTd` and the matching Debug library. All output remains
in the external directory. The test does not play sound or write user settings.

To check audio transparency, build the parent of the meters commit in another
checkout and output directory with the same compiler/configuration. Copy only
`LimiterMeterTests.cpp` into its `dsp/tests` directory and compile it there with
`/DFXSOUND_TEST_BASELINE`, linking that parent's library. This disables references
to the new meter API while generating the same test signal. Save its output as
`baseline.f32`. Compare `Get-FileHash` results (SHA-256) and file lengths: all
432,000 float samples should be byte-identical. This is a numerical regression
check for these cases, not an exhaustive proof for every input or device.

## Manual checks

1. Start FxSound and confirm HighRes is selected. Switch it off and on; the original
   spectrum must remain usable, with a clear selected state on the button.
2. Play left-only and right-only material; activity should predominantly appear
   above and below the center respectively. Check mono, silence and balanced stereo.
3. Play short transients and sustained tones. Attacks should survive between UI
   refreshes while sustained tones remain visible. This is a visual check, not a
   promise to identify every note in a mixture.
4. Pause/resume playback, switch output devices, hide/show the window and disable/
   enable FxSound processing. Old peaks should not reappear after a reset.
5. Check UI responsiveness and CPU use on the target machine, with HighRes both on
   and off. Repeat with the supported sample rates and display scaling settings.
6. With HighRes enabled, exercise left/right output peaks and limiter activity at
   a safe listening volume. Check the independent indicators, tooltip and hold.
   A synthesized impulse is preferable for testing single-sample capture. Red
   indicates a warning; it does not establish that the audio is audibly clipped.

## Listening examples

These are user-supplied test materials, **not recordings of this implementation**
and not automated acceptance tests:

- [STEREO CHECK - Complete Speaker Test](https://youtu.be/uNtfzm-Y0eQ)
- [Additional listening example](https://youtu.be/BAwXtKsm7T4?list=RDuRZAQRhPHuE)
- [Third listening example](https://www.youtube.com/watch?v=8NOBBvQRT-w)

Use the stereo example to inspect channel placement and the other examples to
inspect musical movement. Browser playback, streaming processing and source
mastering are uncontrolled; use generated signals for quantitative measurements.

## Preparation results (2026-09-26)

- Visual Studio 2022, v143/MSVC 14.44, JUCE 6.1.6, x64: application builds passed
  in Release and Debug. A local validation copy adjusted JUCE locations and moved
  outputs outside the source tree; those build overrides are not in this branch.
- HighRes-only commit: 109 checks passed in Release. The combined spectrum/meter
  suite passed all 155 checks in both Release and Debug.
- DSP meter tests: 35 checks passed in both configurations. The uninstrumented
  parent and the instrumented Release build produced identical captures:
  432,000 float samples / 1,728,000 bytes, SHA-256
  `a02e327e9c98526a8c2002df837b55f555504b9d572994f44d366ca96df7401c`.
  The instrumented Debug capture has the same hash.
- Build warnings were reviewed against the base sources; their reported source
  lines are unchanged. The standalone test builds produced no warnings.

These checks do not include fresh interactive testing of the repackaged build,
Win32 or ARM64/JUCE 8 builds, a Projucer regeneration run, or CPU measurements
across devices. Both Projucer files and exported project references were checked
for valid XML and resolution of all newly added source paths.
