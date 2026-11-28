# Testing the HighRes spectrum

## Numerical and lifecycle tests

`tests/AudioAnalysisTests.cpp` exercises FFT normalization and continuity, smooth
band weighting, order-four aggregation, the frequency axis, display dynamics,
peak retention, queue overflow, reset/recovery and stale-frame handling. Tests
generate their own signals and do not open an audio device or alter user settings.
The executable writes JSON and returns a nonzero exit code on failure.

From a Visual Studio 2022/v143 or Visual Studio 2026/v145 x64 **Developer PowerShell**, set `$repo` to this checkout
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

## Validation of the narrowed PR (2026-11-25)

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
