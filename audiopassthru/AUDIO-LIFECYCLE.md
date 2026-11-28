# Audio lifecycle and device notification follow-up

This independent change follows the lifecycle review in PR #634. It changes
audio ownership, notification delivery and shutdown, without changing DSP formulas
or adding the HighRes spectrum, analysis library or output indicators. Its base
is develop/windows; it does not require the visualization contribution to merge.

## Normal device changes

The four MMDevice callbacks stop the current stream immediately, copy the event
and endpoint ID into `DeviceChangeQueue`, then post an owner-window message. They
do not enumerate endpoints or access the mutable endpoint table. The bounded
queue holds 64 events with IDs of at most 511 characters plus a terminator.
Overflow, contention or an oversized ID requests owner-side reconciliation and
emits a diagnostic; a truncated ID is never used as an endpoint identity.

The owner defers enumeration and reinitialization until the processing thread
has stopped. A pending transition uses a 50-ms one-shot retry when necessary;
there is no periodic idle notification timer. Notices arriving before the window
exists stay pending until it is attached. Only repeated device-state notices are
deduplicated: physical default changes A -> B -> A remain meaningful.

Volume callbacks still mirror the existing volume/mute behavior. A shared close
gate and in-flight count protect their table/COM accesses. Endpoint registrations
have explicit flags, so a retry after a partial unregister failure cannot remove
an already removed registration again. MMDevice unregister uses the original
enumerator that registered the callback.

## Application shutdown

`systemRequestedQuit()` closes the controller gate and calls `shutdown(2500)`
before `quit()`. The coordinator runs outside the message thread. It waits up to
2000 ms for the processing thread and entered device/volume callbacks, attempts
physical endpoint restoration, then releases audio resources only after those
users have finished. The outer 2500-ms wait bounds the coordinator wait; it does
not impose a deadline on every other application/UI shutdown operation.

Restoration owns an immutable snapshot of candidate IDs, the virtual ID and the
previous physical mute state. The owner refreshes it during normal enumeration,
selection and reinitialization. The coordinator copies the latest saved mute
after callbacks drain. It creates its own COM apartment and interfaces; it never
enumerates the live table or reads registry state on the quitting message thread.
It preserves an already selected external physical default, otherwise tries
active physical candidates using the existing eConsole policy. It restores the
saved physical endpoint's own mute state without changing its volume. A failed
callback drain disables mute restoration rather than guessing a racing value.

If a driver, callback or COM call remains blocked, shutdown reports an incomplete
outcome. The application retains the audio/controller/DSP graph instead of
freeing memory still reachable by that work. This is an intentional exceptional
process-lifetime retention; the OS reclaims it at process termination. There is
no forced thread termination. A stalled Windows audio service can prevent
successful endpoint restoration despite the bounded coordinator wait.

The coordinator's thread handle must be signalled before an outcome is considered
safe to destroy, including destruction of the worker's captured state. The GUI
teardown is idempotent and the controller is destroyed explicitly only on that
safe path, rather than by JUCE's automatic shutdown singleton list.

## Validation and remaining manual checks

See `tests/README.md` for the reproducible, hardware-free fixtures. The shutdown
fixture exercises real Win32 thread lifetimes and an in-memory endpoint backend;
the notification fixture exercises production callbacks and unregister routines
with fake COM endpoints. They do not validate an actual driver's latency,
Windows session ending, unplug/replug behavior or the Windows default-output UI.

Before distributing a binary, manually check normal quit, Windows sign-out,
physical A -> B -> A changes, unplugging the selected output, mirrored mute/volume,
and restoration after quitting with FxSound selected as the default. Perform
those checks after closing the existing instance, using a separately chosen test
binary; the automated fixtures do not launch FxSound or change user settings.
