# Failed processing initialization retries

`ProcessingRetryTests.cpp` includes the real `AudioPassthruPrivate.cpp` and calls
its real `processTimer`. Only the fixture redirects initialization, the clock,
thread handles, selection, restoration, freeing and diagnostics. It does not open
an audio endpoint, start a worker, read settings, or change the user's default
device. Its peer makes teardown inert; restoration/Free stubs are a second guard.
An unexpected modal diagnostic or debugger break fails the fixture.
The unchanged hard selection-error diagnostic is deliberately intercepted and
counted in its negative test; no message box or debugger break is executed.

Run from an x64 Visual Studio Developer PowerShell with libraries built from the
same source revision, compiler and Debug/Release CRT. For example:

```powershell
./audiopassthru/tests/Run-ProcessingRetryTests.ps1 `
  -BuildRoot 'C:/validation/idle' -Configuration Release `
  -AudioLibrary 'C:/validation/libs/Release/audiopassthru.lib' `
  -DspLibrary 'C:/validation/libs/Release/DfxDsp.lib'
```

Repeat with the corresponding Debug libraries and `-Configuration Debug`.
All products, build diagnostics, results and stderr are written beneath BuildRoot,
which must be outside the source checkout. No interactive app is launched.

The checks cover a constant one-second delay after hard ReInit/CreateThread
failures, immediate notification/selection recovery, notification during a failing
call, no unconditional throttle after success, the existing unavailable-output
sentinel, diagnostic transitions and long uptime.
The selection tests cover both rejected status and hard error without lifting
the retry deadline; a hard failure followed by unavailable output must stop
periodic retries through the existing sentinel. Lower-level historical modal
errors inside ReInit are not changed by this narrow fix and are stubbed here.
These tests measure retry scheduling, not real idle CPU, memory growth, or hardware
recovery; they do not establish that issues #671 or #580 are resolved.
