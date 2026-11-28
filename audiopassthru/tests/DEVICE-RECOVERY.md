# HDMI and endpoint recovery

This change depends on the audio-lifecycle work in PR #703. It does not include
the idle retry, output-reserve, headroom, spectrogram or pitch changes.

## Behaviour and ownership

MMDevice callbacks copy endpoint IDs and queue notices. A coherent, immutable
snapshot contains the virtual, capture and playback endpoint IDs actually used
by the worker. An added/removed/state callback for another endpoint queues a
catalogue update without stopping healthy playback. The owner's established
priority policy can still intentionally select a newly available higher-priority
output after that update, including a known UNPLUGGED endpoint becoming ACTIVE.
Missing identities, lost queue
records and failed snapshot publication conservatively request route recovery.
The owner reclassifies queued notices against the latest route after ReInit, so
an endpoint removed while a replacement route is being prepared cannot start.

The independent catalogue reader uses copied property values, including the
stored `PKEY_AudioEngine_DeviceFormat` channel count. It never activates an
unplugged audio client. A transient metadata error preserves the last complete
catalogue and retries at most every 250 ms for three seconds through the existing
notification retry mechanism. A successful publication sends one catalogue
callback; retries do not manufacture UI catalogue epochs. An error persisting
beyond this budget waits for a later real notice or explicit refresh.

The UI preserves the existing three-second HDMI hold. It can then use the
existing preferred fallback while remembering the lost endpoint's stable ID.
When that ID becomes usable again, the UI requests its return. An unsuccessful
return is not repeated in the same catalogue epoch. The intent is session-only;
this adds no persistent output pin or new preference. A later manual selection,
including selection of the current fallback, cancels that return. The existing
CLI output-selection path counts as manual selection too.

Only the owner applies a pending endpoint ID after the worker has stopped.
Before each COM operation it copies the ID and command generation. Reentrant
newer choices survive an older operation's completion, and prevent the older
route from starting. Power-off/default restoration cancels a pending command
before restoring policy. ReInit reads the current Windows default through its
normal rules; an old default-event payload is never replayed over that default.
Windows MMDevice notices do not identify whether an external default change was
manual or automatic, so the existing Windows-default and priority rules remain.

## Automated checks

Run the unchanged `Run-AudioLifecycleTests.ps1` first against libraries built
from this checkout. Run the additional fixture from an x64 Visual Studio
Developer PowerShell, with matching Debug or Release libraries:

```powershell
./Run-DeviceCatalogTests.ps1 -Configuration Release -BuildRoot C:/task/validation `
    -AudioLibrary C:/task/libs/Release/audiopassthru.lib `
    -DspLibrary C:/task/libs/Release/DfxDsp.lib
```

Repeat with Debug libraries and `-Configuration Debug`. All outputs stay under
`BuildRoot/DeviceCatalogTests/x64/<configuration>`: compiler log, unbuffered test
results, stderr and native exit code. The build root must be outside the sources.

`DeviceCatalogDispatchTests.cpp` compiles the actual Private owner implementation
with narrow replacements at enumeration, registry, worker-creation and clock
boundaries, and links the real callback/queue and catalogue-item parser. It uses
controlled native thread handles to exercise actual nonblocking join behaviour.
It covers known-snapshot state changes for distinct playback/capture/virtual
IDs, unused IDs, missing/empty/overlong IDs, unrelated endpoint changes during healthy playback, removed current
endpoints, newly catalogued ID selection, same-fallback cancellation, reentrant
GetAll/SetDeviceType/ReInit choices, removal during ReInit, queue/snapshot loss,
bounded metadata retries, power-off cancellation and fresh-default ordering.
Metadata fakes prove an inactive endpoint need not Activate and invalid active
format blobs fail without leaking property references. The UI's actual recovery
policy helper is tested for its original hold, suspend, ID intent and epoch rules.

The fixture does not instantiate FxController, DfxDsp, Settings or the public
AudioPassthru wrapper. Private's destructor only unregisters its callback; the
fixture never calls init/shutdown or real endpoint restoration. GetID, including
its normally registry-backed user-selection case, is replaced too. Every test
thread is joined, its duplicate wait handle closed and retry/window state detached.

## Hardware validation still required

Software checks do not prove HDMI driver timing, actual WASAPI restart, audible
continuity, device-format negotiation, Windows policy or GUI interaction. Test
headset playback while an unrelated lower-priority HDMI display sleeps/wakes;
then unplug or
sleep the selected HDMI route, wait for fallback and wake it. Confirm automatic
return, then repeat while manually choosing another output before HDMI returns
(including reselecting the fallback). Repeat through suspend/resume, power-off,
and rapid alternating device additions/removals. Confirm the output list changes
while unrelated healthy playback continues, and compare with two endpoints of
the same name. Independently test the existing priority policy: configure an
unplugged endpoint B above active A, reconnect B and confirm B is selected even
though its ID was already known while UNPLUGGED. Repeat with B below A and confirm
A continues. This Controller preference path is audited at its call site, not
instantiated by the profile-free fixture. Check a return that initially fails format negotiation: fallback
must remain usable and a later genuine endpoint notice may retry the return.
