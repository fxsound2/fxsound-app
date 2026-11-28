# Testing the HighRes spectrum

## Numerical and lifecycle tests

`tests/AudioAnalysisTests.cpp` exercises FFT normalization and continuity, smooth
band weighting, order-four aggregation, the frequency axis, display dynamics,
peak retention, queue overflow, reset/recovery and stale-frame handling. Tests
generate their own signals and do not open an audio device or alter user settings.
The executable writes JSON and returns a nonzero exit code on failure.

From a Visual Studio 2022/v143 or Visual Studio 2026/v145 x64 **Developer
PowerShell**, invoke the checked-in runner with an absolute output directory
outside the source checkout:

```powershell
$repo = (Get-Location).Path
$build = Join-Path $env:TEMP ('fxsound-analysis-' + [guid]::NewGuid().ToString('N'))
& "$repo/analysis/tests/Run-VisualizationTests.ps1" -BuildRoot $build -Configuration Release
& "$repo/analysis/tests/Run-VisualizationTests.ps1" -BuildRoot $build -Configuration Debug
```

The runner compiles the shared core once for each configuration, then links and
runs two independent executables: the existing **158** analyzer/lifecycle checks
and **384** additional history/model/request-session checks. It records build
logs, JSON results, stderr and native exit status outside the checkout. It has no
JUCE dependency and opens no audio endpoint or application profile. Debug CRT
reports are configured for noninteractive execution. The existing suite has empty
stderr. The additional suite emits 31 expected numerical diagnostic/timing lines:
rumble and pursuit comparisons, profile/decay/widening checks, PCM chirps and
carrier-sine oracles, and one 2,000-frame raster timing. They are not assertion or
exception reports. Inspect their values against the checks; the single timing
does not establish a performance improvement.

For direct `cl` usage, both fixtures now require `AudioAnalyzer.cpp`,
`runtime/AnalysisStream.cpp`, `WaterfallRaster.cpp` and the three model sources
`experimental/pitch/{PitchMotion,SpectralCorrelation,HarmonicCombProfile}.cpp`.
Use consistent `/MT` Release or `/MTd` Debug settings throughout. The runner is the
maintained command reference; the earlier three-source example is insufficient.

Build the application using the repository's normal JUCE/Visual Studio workflow;
rebuild the audio callback library with its consumers after the observation-token
header change. Its third opaque `requestSession` field prevents old in-flight PCM
from entering a reopened display session; it does not alter routing or DSP.

[Optional views and controls](VISUALIZATIONS.md) describes the experimental model
limits. The versioned [GUI harness and runner](tests/ui/README.md) rebuild the
actual widgets against explicitly supplied matching JUCE module objects. Scaled
offscreen widget exports exercise real
paint code at 100/175/200% export sizes; they do not establish native Windows peer
DPI behavior or validate interactive menus/window teardown in the full app.

## Windows application build on develop/windows

Initialize the Resources submodule before building. The current `develop/windows`
projects select MSVC v145, as described in `README.md`; the older VS 2022/JUCE 6
instructions in `CLAUDE.md` describe a different toolchain. Validate the native
configuration and any requested compatibility configuration separately:

| Configuration | Solution | JUCE | Compiler |
| --- | --- | --- | --- |
| Current Windows | `fxsound/Project/FxSound.sln` | 9.0.5 (`C:\JUCE` by default) | VS 2026/v145 |
| Current Win7 project | `fxsound/ProjectWin7/FxSound.sln` | 6.1.6 (`C:\JUCE6` by default) | v145 as declared in the project |
| VS 2022 compatibility | `fxsound/ProjectWin7/FxSound.sln` | 6.1.6 | VS 2022/v143, explicitly retargeting the complete solution |

Use the external import below to redirect SDK paths and build products away
from the checkout. The checked-in modern project includes the split GUI units.

From an x64 Developer PowerShell at the repository root, set
FXSOUND_JUCE_ROOT to the extracted JUCE 9.0.5 directory, then run:

```powershell
$repo = (Get-Location).Path
$juce = Join-Path $env:FXSOUND_JUCE_ROOT 'modules'
$build = Join-Path $env:TEMP ('fxsound-build-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $build | Out-Null
$targets = Join-Path $build 'Validation.targets'
$xml = @'
<Project>
  <PropertyGroup>
    <OutDir>$(FxValidationRoot)\$(Platform)\$(Configuration)\$(MSBuildProjectName)\</OutDir>
    <IntDir>$(FxValidationRoot)\$(Platform)\$(Configuration)\$(MSBuildProjectName)\obj\</IntDir>
    <PostBuildEventUseInBuild>false</PostBuildEventUseInBuild>
  </PropertyGroup>
  <ItemDefinitionGroup>
    <ClCompile>
      <AdditionalIncludeDirectories>$(FxValidationJuce);%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
    </ClCompile>
    <PostBuildEvent><Command></Command></PostBuildEvent>
  </ItemDefinitionGroup>
</Project>
'@
$xml |
    Set-Content -LiteralPath $targets -Encoding utf8
$env:INCLUDE = "$juce;$env:INCLUDE"
foreach ($configuration in 'Debug', 'Release') {
    msbuild "$repo/fxsound/Project/FxSound.sln" /m `
        "/p:Configuration=$configuration" /p:Platform=x64 `
        "/p:ForceImportBeforeCppTargets=$targets" `
        "/p:FxValidationRoot=$build" "/p:FxValidationJuce=$juce" `
        "/bl:$build/$configuration.binlog"
    if ($LASTEXITCODE -ne 0) { throw "$configuration build failed" }
}
```

To validate the native Win7 project, use JUCE 6.1.6, replace Project/FxSound.sln with
ProjectWin7/FxSound.sln, and use a separate build directory. Do not pass
`/p:PlatformToolset=v145`: the checked-in project already selects that toolset.
For the additional VS 2022 compatibility check, use an x64 VS 2022 Developer
PowerShell and `/p:PlatformToolset=v143` for the complete solution, including
audiopassthru and DfxDsp. Record that explicit retargeting in the results.
Use separate intermediate/output directories for every toolchain and JUCE variant.
Projucer exports share `fxsound/JuceLibraryCode`; do not regenerate the Win7 and
current Windows variants in the same checkout during concurrent validation.
Compilation on a current Windows machine does not validate Windows 7 runtime behavior.

The DSP library is under x64/<configuration>/DfxDsp in the build directory.
Rebuild the audio callback library and its consumers together. For the PCM
comparison, build DfxDsp from the same develop/windows revision and toolchain
in a separate directory.

## DSP meter regression and audio comparison

In the Windows software DSP, the Play and Maximizer entry points receive
caller-owned meter storage. The limiter uses auxiliary slots 4 and 5 in that
storage. The application displays the front limiter activity on mono and stereo outputs.

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

To check audio transparency, build the PR's exact `develop/windows` base in another
checkout and output directory with the same compiler/configuration. Compile the
same `LimiterMeterTests.cpp` with `/DFXSOUND_TEST_BASELINE`, linking that base's
library. This disables references
to the new meter API while generating the same test signal. Save its output as
`baseline.f32`. Compare `Get-FileHash` results (SHA-256) and file lengths: all
432,000 float samples should be byte-identical. This is a numerical regression
check for these cases, not an exhaustive proof for every input or device.

## Manual checks

1. With fresh settings, start FxSound in the classic view. Switch to HighRes with
   the flip button and restart to verify the saved choice. On a multichannel
   output the classic spectrum should appear. Return to stereo to recover the
   selected HighRes view.
2. Play left-only and right-only material; activity should predominantly appear
   above and below the center respectively. Check mono, silence and balanced stereo.
3. Play short transients and sustained tones. Attacks should survive between UI
   refreshes while sustained tones remain visible. This is a visual check, not a
   promise to identify every note in a mixture.
4. Pause/resume playback, switch output devices, hide/show the window and disable/
   enable FxSound processing. Old peaks should not reappear after a reset.
5. Check CPU use, wakeups and audio dropouts with HighRes visible, in classic mode,
   and with FxSound in the tray. Mute the output, check that no new peaks or
   limiter activity appear, and confirm recovery after unmute. Repeat at supported
   sample rates and display scales. Switch Lite → Pro while playback is already
   running. Repeat a device switch between two stereo outputs and verify that
   the earlier meter holds disappear. Quit during active playback and after a
   device change to exercise the callback shutdown path.
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

## Recording validation results

Record results for the exact revision being reviewed, with its base and head
commits, compiler/SDK/JUCE versions, commands and any build-path or toolset
overrides. Keep analysis and DSP test counts separate and attach their JSON/log
outputs. For the PCM comparison, identify both source revisions and compare
matching compiler/configuration pairs. Historical results from an earlier
revision are not evidence for the current application build.

Separate successful compilation and numerical tests from manual device, UI,
shutdown, Windows 7 runtime and performance checks. List checks not performed;
do not infer them from an older local or trial executable.

## Optional visualization validation (2026-11-28)

The optional-views work is based on spectrum-only commit `0c55538`. The following
results cover the final versioned test fixtures, with all build products outside
the checkout and no normal FxSound instance started:

| Check | v145 / JUCE 9.0.5 | v143 / JUCE 6.1.6 |
| --- | --- | --- |
| Application x64 | Debug and Release passed | Debug and Release passed |
| Existing portable analyzer suite | 158/158, Debug and Release | 158/158, Debug and Release |
| Additional histories/models/request sessions | 384/384, Debug and Release | 384/384, Debug and Release |
| Offscreen production-widget harness | 115/115, Debug and Release | 115/115, Debug and Release |

The portable suites have no JUCE dependency; the SDK labels identify the matching
application/harness toolchain, not a numerical dependency. Each GUI run exported
24 PNGs at 100/175/200%, returned exit 0, reported zero unexpected assertions and
had no unexpected diagnostic markers. The runner records module/configuration
provenance and hashes. Readable representative images are included in
[the view documentation](VISUALIZATIONS.md); they are synthetic widget exports.

The final harness locally recompiles the genuine JUCE core with assertion logging
enabled, leaving cached application objects unchanged. Its additional check
proves receipt of one expected real JUCE assertion message in a separate log,
before normal scenes and explicit GUI teardown. Unexpected diagnostics remain
failures. Assertions already compiled out of other Release modules and static
destruction after main are outside that diagnostic coverage.

The v145 Release portable suites also passed all 158+384 checks under
AddressSanitizer, with debug symbols and no sanitizer finding. Remaining compiler
warnings were inspected: the portable strncpy deprecation and application warning
sites are inherited from the spectrum-only base. Numerical/performance stderr
observations are not an application CPU benchmark.

Initial validation attempts exposed a missing real `gui_extra` module object in
the harness link and a sparse-pixel sampling false negative in one compact export.
The runner now links that genuine matching module, and the fixture scans actual
pixels rather than an alias-prone grid. The render criterion was not reduced;
these were test-only corrections. Earlier attempts remain in the validation
record rather than being reported as successful runs.

The raw short-FFT reference comparison and its unmatched-window exclusions are
described in [the view documentation](VISUALIZATIONS.md). Its exact equality does
not establish equal hybrid bars, model history, musical accuracy or audible
behavior. In particular, the rich 150 Hz fixture's 0/13 near-fundamental recovery
is a documented accuracy limitation, separate from its solver-validity checks.

The GUI harness does not exercise the full application, menus, auxiliary-window
interaction/teardown, actual Windows peer DPI, device changes or real playback.
Those behaviors remain manual-validation boundaries; scaled exports and source
review do not replace them.

## Historical validation of the narrowed PR (2026-11-25)

The following runs use the spectrum/indicator changes after restoring the
upstream shutdown and device-notification implementation. Their base is
`develop/windows` commit `191d91c6cb843a3f9d1e792f6c5f0e9f79e75f55`.
All application builds target x64 and use Windows SDK 10.0.26100.0, with separate
output/intermediate directories outside the source checkout.

| Application | Toolchain | Debug | Release |
| --- | --- | --- | --- |
| Current Windows, JUCE 9.0.5 | VS 2026, v145/MSVC 14.51 | Passed | Passed |
| Win7 project, JUCE 6.1.6, native toolset | VS 2026, v145/MSVC 14.51; no toolset override | Passed | Passed |
| Win7 project, JUCE 6.1.6, VS 2022 compatibility | VS 2022, v143/MSVC 14.44; explicit solution-wide v143 override | Passed | Passed |

- Analysis tests: **158/158** passed in each of Debug and Release on both v143
  and v145 (four runs). Their compilation logs have no warnings and their run
  stderr streams are empty.
- DSP meter tests: **35/35** passed in Debug and Release on v145. The matching
  uninstrumented base passed its seven applicable checks in both configurations.
  Each PR/base pair produced byte-identical captures: **432,000 float samples /
  1,728,000 bytes**, SHA-256
  `a02e327e9c98526a8c2002df837b55f555504b9d572994f44d366ca96df7401c`.
- Application warnings were checked at their source locations against the base:
  the reported lines are unchanged. The upstream application was not rebuilt as
  part of that warning comparison; its DSP was rebuilt for the PCM comparison.
- Commands, evaluated output paths, build logs/binlogs and numerical JSON
  results were retained with the validation record.

No application instance was launched or restarted for these runs. Interactive
UI, device switching, HDMI, system shutdown/sign-out, Windows 7 runtime,
Win32/ARM64, Projucer regeneration and performance measurements were not tested.
