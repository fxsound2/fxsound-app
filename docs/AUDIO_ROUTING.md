# Audio routing

Open **Settings > Routing** to choose an input and output.

## Inputs

- **FxSound Audio Enhancer (default)** preserves normal FxSound operation. Send application audio to the FxSound playback endpoint.
- **Playback audio (loopback)** captures the mix sent to a chosen playback endpoint, such as a virtual cable's playback side. Select a different device as the output first. Audio may also remain audible on the original source device; loopback does not suppress that device's normal playback.
- **Recording devices** captures a microphone, line-in, or a virtual cable's recording endpoint. Mono recording devices are included. Windows microphone access must allow desktop applications.

Custom inputs use shared-mode WASAPI conversion to the engine's floating-point processing format. FxSound does not change the custom input's endpoint format, volume, mute, or Windows default assignment. A disconnected selected input pauses capture; it never falls back to another microphone. Reconnect it or select a different input to resume. FxSound power-off stops the capture thread, including microphone monitoring.

The existing virtual driver remains required by this application's startup checks. This change does not modify, build, or install a kernel driver.

## Windows default output

**Make FxSound the Windows default output** controls the same setting as:

```bat
reg add "HKCU\SOFTWARE\DFX\13\23\devices\default_device_mode" /ve /t REG_SZ /d off /f
```

Checked saves `on`; unchecked saves `off`. If FxSound currently owns the default assignment, unchecking first hands it back to an available physical output. Output dropdown changes themselves no longer change Windows defaults. Power-off/exit restores a default only when automatic selection is enabled and this process previously made FxSound the default; an unrelated default chosen since then is preserved.

This checkbox is available for the default FxSound input. Custom inputs leave Windows defaults alone regardless of the saved checkbox preference. The existing setter changes the console role; it does not add new communications-role changes.

## Pinned output

**Keep this output device** writes the selected physical output's endpoint ID to the default value of:

```text
HKCU\SOFTWARE\DFX\13\23\devices\user_selected_playback
```

This is the same preference populated by copying `most_recent_playback` in PowerShell. Choosing a different output explicitly while pinning is enabled updates the pin. An automatic fallback does not overwrite it. If the pinned output disconnects, FxSound uses an available output and prefers the saved pin again when it reconnects. Unchecking clears the saved selection.

The new input ID is saved in `user_selected_input` under the same base key. An empty value selects the default FxSound input. Routing settings are preserved across application version changes.

## Build and verify on Windows

1. Install the normal FxSound package so its signed driver is present. Close the installed FxSound before starting a modified build.
2. Use the `develop/windows` build requirements in the repository README: Visual Studio 2026 (v145), a Windows SDK, and JUCE 9. The checked-in project was generated with JUCE 9.0.3; the routing CI pins that version. Its module include path is `C:\JUCE8\modules` (the folder name is historical). Projucer also emits source paths relative to the original developer checkout; adjust those paths for your checkout or re-export with the matching Projucer, restoring the library project references as described in the README. The CI workflow normalizes those paths in its temporary workspace.
3. Open `fxsound/Project/FxSound.sln` and build **Release | x64**. From a Visual Studio developer shell, the equivalent command is:

   ```bat
   msbuild fxsound\Project\FxSound.sln /m /p:Configuration=Release /p:Platform=x64
   ```

4. Run with the presets/resources directory specified by the existing project instructions. Keep the installed build closed to avoid its single-instance handling and competing audio routing.
5. The platform-independent device-selection regression tests can be run from a Visual Studio developer shell:

   ```bat
   cl /nologo /EHsc /std:c++14 /W4 audiopassthru\tests\routing_policy_test.cpp /Fe:routing_policy_test.exe
   routing_policy_test.exe
   ```

The test executable exercises the production selection policy. It does not test COM calls, WASAPI conversion, device clock drift, or the JUCE UI.

## Windows acceptance checks

- With default input and automatic selection enabled, turn power on. App audio follows FxSound and exits through the selected output. Power off stops processing and hands back the physical default.
- Disable automatic default selection. Record the Windows default; select outputs, restart FxSound, and power it on/off. The Windows default must remain unchanged after the initial release of a default owned by FxSound.
- Read `default_device_mode` and `user_selected_playback` with `reg query ... /ve` after changing each checkbox. Confirm that externally written preferences are reflected when reopening Settings.
- Pin one output, attach another, and confirm it cannot steal the route. Disconnect the pin and confirm fallback without overwriting the saved ID. Reconnect it and confirm the pin is preferred again. Choose another output explicitly and confirm that it updates the pin.
- Select a virtual playback device as input, send music to it, and verify effects through a different output. Choosing the same playback device as input/output must be rejected.
- Select a mono microphone, then a line-in or virtual-cable recording endpoint. Verify effects and that the source device's volume/mute and default assignments are unchanged. Power off must stop monitoring. Check denied microphone permission and an exclusive-mode device in use: report failure without repeatedly reopening it.
- Disconnect a selected input. Capture must pause without switching microphones. Reconnection must resume, and Settings must show an unavailable saved input while it is absent.
- Verify 44.1 and 48 kHz sources with 44.1, 48, 96, and 192 kHz outputs, plus supported surround layouts. Run a longer session using independent physical devices to assess clock drift and dropouts.
- Change the Windows default manually while FxSound is running, then exit. FxSound must not overwrite a newer user-selected default.
- Repeat startup and upgrades with registry preferences already present. Confirm that they survive the version change.

## Verification boundary

The implementation was developed on macOS. The portable policy tests were compiled with warnings as errors, AddressSanitizer, and UndefinedBehaviorSanitizer. The contributor reports a successful Windows build and working application after testing the changes. That runtime result was reported by the contributor, rather than independently reproduced in the macOS development environment. The complete device, format, and reconnection matrix above remains a checklist for further validation.

## Contribution and review

This feature is proposed in [issue #666](https://github.com/fxsound2/fxsound-app/issues/666), with related requests #39 and #435. The PR was opened before the issue discussion; maintainer agreement is still pending. The PR targets `develop/windows` and retains its existing volume/mute fixes. Changes under `audiopassthru/` require particular human review before merge.

The `Validate audio routing` workflow runs the production policy tests with sanitizers and builds Release x64 with Visual Studio 2026 and JUCE 9.0.3. Compilation does not verify WASAPI behavior on physical devices. The contributor-reported runtime test used the earlier JUCE 6.1.6/main-based revision; the rebased development version needs the Windows acceptance checks above.
