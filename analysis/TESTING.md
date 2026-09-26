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

## Listening examples

These are user-supplied test materials, **not recordings of this implementation**
and not automated acceptance tests:

- [STEREO CHECK - Complete Speaker Test](https://youtu.be/uNtfzm-Y0eQ)
- [Additional listening example](https://youtu.be/BAwXtKsm7T4?list=RDuRZAQRhPHuE)

Use the stereo example to inspect channel placement and the second example to
inspect musical movement. Browser playback, streaming processing and source
mastering are uncontrolled; use generated signals for quantitative measurements.
