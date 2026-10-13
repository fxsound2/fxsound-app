# FxSound HAL development driver

This is an offline development implementation of AudioServerPlugIn. It exposes `FxSound_Mac_Virtual` (`FxSound Audio Enhancer (macOS)`) with one output and one loopback input stream. It does not contain DSP or route audio to speakers; the engine captures input and renders processed audio to a physical device.

## Build

Run `macos/driver/build.sh [output-directory]` from the repository. The default output is `/private/tmp/fxsound-driver-artifacts/FxSound.driver`. Requires Apple Command Line Tools with macOS SDK, clang, lipo, plutil and codesign. No downloaded compiler dependencies or full Xcode project are required for this offline build.

The build compiles x86_64, arm64 and arm64e with deployment target 14.0, warnings as errors, combines the slices, validates Info.plist, and applies/verifies an ad-hoc hardened-runtime signature. This verifies binary construction; it does not prove HAL loading, distribution signing or compatibility on any target runtime. No install, coreaudiod restart or default-output operation occurs.

## Stream and timestamp contract

| Item | Contract |
|---|---|
| Format | 48,000 Hz only; native-endian interleaved float32 stereo, 8 bytes/frame |
| Ring | Static 32,768 frames (about 683 ms), no callback allocation |
| Writer | HAL `WriteMix` on output stream ID 4; HAL mixes playback clients before writing |
| Readers | HAL `ReadInput` on input stream ID 3; each client's sample timestamp selects frames, no shared read cursor |
| Loopback delay | Read sample N returns written sample N−512; first 512 frames are silence; device input latency reports 512 frames |
| Frame identity | Each ring slot has atomic sample-frame publication tag, restart epoch and atomic float bit storage |
| Overrun/underrun | Overwritten/missing/stale/unpublished frames return silence; no waiting or retry loops |
| Clock | mach_absolute_time anchor; fixed 48k conversion from mach_timebase_info; zero timestamps quantized to 512-frame periods |
| Timestamp seed | Changes on first StartIO and last StopIO; first start resets anchor; ring validity changes via epoch rather than clearing live storage |
| Host correlation | Sample times map to host time through the same anchor/ticks-per-frame mapping returned by GetZeroTimeStamp |
| Start/stop | Lifecycle-only mutex and fixed table of up to 64 active client IDs; duplicate start or unmatched stop is rejected |
| Realtime ownership | One HAL mixed-output writer; multiple independent readers, including across ring wrap; no allocation, mutex, file/network IO or property notification in IO callbacks |
| Callback bounds | At most 32,768 frames; larger requests rejected; operation/stream/sample-time validity checked |

The loopback intentionally adds one 512-frame period so input reads do not depend on output having run in the same callback cycle. The engine must not capture its own physical render into the virtual output. Drift correction between virtual and physical device clocks belongs to the engine. Stopping/restarting IO relies on the HAL lifecycle quiescing prior callbacks; old-generation frames are never accepted by new readers.

## Object and control contract

Plugin ID 1 owns device ID 2. The device owns input 3, output 4, output volume 5, output mute 6 and output data source 7. Plugin/device/stream/control properties expose class/owner/owned objects, UIDs, rates, format lists, active flags, direction, latency and stereo layout. Factory UUID and interface UUID follow the CFPlugIn AudioServerPlugIn contract.

Volume is software gain applied once to mixed output before publication, with scalar [0,1] and decibel [−96,0] control views. Mute returns zero-valued loopback audio. Stream active flags are independently writable. Changed controls and running state notify the host outside realtime IO. Data source has one item, `Engine routing`; it is a stub selector, not physical-device routing.

Custom device selector `'fxvr'` is a read-only CFString `"1"`, registered via CustomPropertyInfoList. It identifies the stream/handshake contract version, not an engine-presence flag. CFString custom marshalling follows the SDK; no private shared-memory channel is used. Rates/formats are fixed and read-only, so configuration changes are not requested and configuration-change callbacks reject unsupported actions.

## Provenance and verification limits

Interface/lifecycle/property scaffolding follows Apple's `Creating An Audio Server Driver Plug In` NullAudio sample, downloaded from https://docs-assets.developer.apple.com/published/430ad6501f6f/CreatingAnAudioServerDriverPlugIn.zip. The implementation is split into small modules rather than importing its monolithic source. Apple sample license is retained in `apple-sample-license.txt` and bundled under Resources. The repository's AGPL license also applies to project changes; the sample notice remains intact.

Observed build host: arm64, macOS 27.2, Command Line Tools SDK 27.0. The plan's macOS 14/15/26, Intel/Apple Silicon load/routing matrix is unverified. Successful ad-hoc verification does not establish signing requirements inside coreaudiod. A supported-runtime signed install/load/uninstall spike, multi-client playback, sleep/wake, clock-drift and crash-recovery tests remain required before GUI integration or distribution. No physical passthrough, real HAL load or audio-quality result is claimed by the offline build.
