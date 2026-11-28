# Audio lifecycle regression fixtures

Run from an **x64 Visual Studio Developer PowerShell**, after building the matching
`audiopassthru.lib` and `DfxDsp.lib` from this checkout. Pass absolute paths to
those libraries and an output root outside all source trees. Match toolset,
architecture and configuration; the fixtures use the application's static CRT
(`/MT` in Release, `/MTd` in Debug).

```powershell
./audiopassthru/tests/Run-AudioLifecycleTests.ps1 `
    -Configuration Release -BuildRoot 'C:/Temp/FxSound-validation' `
    -AudioLibrary '<absolute path to Release/audiopassthru.lib>' `
    -DspLibrary '<absolute path to Release/DfxDsp.lib>'
```

Repeat with `-Configuration Debug` and Debug libraries. The script compiles and
runs both fixtures, keeps build/results/error logs under the supplied output
root, and fails on a nonzero exit or a 30-second fixture timeout. It terminates
only its own fixture on timeout.

- `AudioShutdownTests.cpp`: 21 checks covering normal and delayed work, timeout
  retention, completion after timeout, exception paths, endpoint fallback,
  external-default preservation and physical mute restoration.
- `DeviceChangeTests.cpp`: 64 checks covering copied notification IDs, filtering,
  A -> B -> A, repeated state deduplication, bounded concurrent producers,
  overflow reconciliation, pending messages and one-shot retries, callback drain,
  unregister failure and partial-unregister retries.

The fixtures open no audio stream and change no user settings. Notification tests
create a hidden message-only window and use fake COM objects; their controlled
owner acknowledgement does not exercise hardware endpoint enumeration.
Expected negative cases emit debug diagnostics. No interactive diagnostic dialog
is required. See `../AUDIO-LIFECYCLE.md` for the production flow and manual limits.
