# Offscreen visualization-widget harness

This harness builds and renders the actual visualization widgets from the chosen
source checkout. It reuses the matching JUCE module objects from an application
build; those objects and their compiler/configuration provenance are explicit inputs.

`VisualizationsHarness.cpp` instantiates the actual inherited WaterfallView and
three pitch widgets with deterministic synthetic frames/snapshots. It renders FFT,
FFT with carrier overlay, carrier history, movement, correlation, harmonic model,
and two compact indicators. Each scene exports PNG at 100/175/200%: 24 PNG files.
It also checks real provider draining, inactive/resumed views, reset/generation,
retained correlation history, geometry bounds and nonblank rendering.

It never constructs FxController, FxModel, DfxDsp, a native DocumentWindow, an
audio endpoint or profile. All components remain offscreen; JUCE is initialized
only within this harness process. It does not change Windows display settings.

Important limitation: inherited WaterfallView uses its native peer scale, or 1.0
without a peer. Scaled snapshots are genuine exports of its actual paint code,
but do not exercise a Windows peer at 175/200% DPI. The separate physical geometry
helpers are checked at those scales. Real peer DPI, FxSound menus/main visualizer,
window ownership/teardown and runtime demand unions require their own validation.
No estimated pitch or audible result is derived from these synthetic pictures.

## Link contract

The runner compiles 12 sources: this harness, the public AudioAnalyzer, the 9
new inherited production CPP files, and the real JUCE core module wrapper. It
reuses the existing JUCE data_structures, events, graphics, gui_basics and
gui_extra objects. The real
module configuration enables GUI-extra interop symbols from gui_basics, so its
matching object is required even though this harness creates no GUI-extra window.
No Main, Controller,
BinaryData, DSP or AudioPassthru object/library is linked. PitchExperimentWindow
is compiled/linked to check its compatibility but is never constructed at runtime.

JUCE6 has 6 required module objects (5 reused, 1 local); JUCE9.0.5 has 29
(28 reused, 1 local), including split graphics/codec and gui_basics units. The
runner names the reused objects explicitly and records their hashes.
Its Win32 SDK libraries and every object are listed in `link.rsp`. Auto-link
directives within the genuine JUCE objects are retained; debug/release COM support
libraries are selected by those directives rather than mixed manually.

Run with the same x64 Developer PowerShell/toolset/CRT as the original modules.
Pass the module build's actual AppConfig hash and CL.command.1.tlog; do not invent
metadata by hashing a later configuration and assuming the old objects match it.
The runner checks AppConfig, VCToolsVersion and CRT, and reuses the module command's
preprocessor definitions. If provenance is uncertain, rebuild the modules within
the existing task outputs; do not silently accept incompatible objects.

Locate the object directory and `CL.command.1.tlog` of the application build you
just performed. The compiler tlog is normally in the project's `*.tlog` child.
Use the actual `fxsound/JuceLibraryCode/AppConfig.h` consumed by that build and
retain its SHA-256 with the module-build inputs. A changed AppConfig or toolset
requires matching rebuilt modules; an old executable is not sufficient evidence.

From an x64 Developer PowerShell, an example invocation is:

```powershell
$repo = '<absolute source checkout>'
& "$repo/analysis/tests/ui/Run-VisualizationsHarness.ps1" `
  -SourceRoot $repo -JuceModuleRoot '<matching SDK>/modules' `
  -AppConfigHeader '<matching AppConfig.h>' -ExpectedAppConfigSha256 '<recorded SHA256>' `
  -ModuleObjectDirectory '<matching app object directory>' `
  -ModuleCompileCommandLog '<matching CL.command.1.tlog>' `
  -ExpectedVCToolsVersion $env:VCToolsVersion -Variant JUCE9-v145 -Configuration Release `
  -BuildRoot '<absolute external build directory>' -RunTag initial
```

Repeat with JUCE6-v143 and Debug matching their modules. `-CompileOnly` performs
no harness run. A new RunTag preserves earlier logs/images. Products stay outside
SourceRoot and module caches remain read-only. The runner bounds only its own
process to 60 seconds and records native exit status; no normal FxSound is launched.

The test process installs a JUCE Logger that writes all messages to stderr and
counts unexpected assertions. Debug MSVC CRT warning/error/assert reports are
routed to stderr without modal dialogs or debugger breaks. The assertion check
includes GUI teardown; the runner also rejects assertion markers in stderr even
if a process reports exit 0. Ordinary JUCE informational messages remain visible
and do not by themselves fail a run. This policy is harness-only.

The original cached `juce_core` object is not linked. Its genuine source wrapper
is compiled only under the harness output directory with `JUCE_LOG_ASSERTIONS=1`,
matching the SDK, CRT, configuration and other definitions. Without that flag,
JUCE's default Debug assertion path can bypass Logger via OutputDebugString.
No cached application object, SDK file or AppConfig is modified. Other reused
modules preserve their application configuration, including assertions compiled
out of Release code.

Before the widget scenes, a self-check calls the real `juce::logAssertion` entry
without the debugger-break macro. It verifies delivery to the test logger,
records that one expected message in `png/assertion-self-check.log`, and resets
the expected counter. Normal stderr and the unexpected-assertion count remain
reserved for unexpected diagnostics. A missing logger callback fails the run.

Before claiming validation, actually compile/link, run, read all logs and inspect
the PNGs. Source compatibility and linker completeness are provisional until that
real run against the final 07 files; parser success alone proves neither.
