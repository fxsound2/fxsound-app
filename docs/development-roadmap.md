# macOS development roadmap

The current implementation targets a personal Apple Silicon installation while
retaining the original Windows JUCE interface. See [build and validation
instructions](macos-port-status.md) for the reproducible checks and their limits.

| Workstream | Current status | Remaining qualification |
|---|---|---|
| Windows boundary and DSP ABI inventory | Completed | Windows build regression and numerical-reference comparison |
| Portable original DSP | Optimized and sanitizer acceptance passes | Complete realtime profiling and Windows parity |
| HAL virtual device | Three-slice bundle and isolated callback tests pass; bounded live loopback exercised | Supported-OS loading, multi-app soak and lifecycle coverage |
| CoreAudio engine and routing | DSP/IPC/ring/SRC tests pass; live authorized capture exercised | Hardware changes, crash recovery, sleep/wake and latency |
| Original JUCE interface | Original views/resources reused; native component/schema tests pass | Accessibility and display-off CPU measurements |
| Preset selection (0.1.1) | Available at power off; deferred/acknowledged choices tested | Broader interactive/device testing |
| Capture authorization (0.1.2) | Native consent and audible processing confirmed on macOS 27 Apple Silicon with EarPods | Native denial/restriction trials and other devices/OS versions |
| Power-button refresh (0.1.3) | Button cache regression/build/package pass | Native Pro controls still require the separate 0.1.4 fix |
| Pro control refresh (0.1.4) | Update/repaint fixed; component/build, three native no-hover ON transitions and package inspection pass | Broader host/hardware coverage remains unverified |
| Personal PKG/DMG | Ad-hoc signatures, payload metadata and corresponding source verified | Clean install/upgrade/uninstall matrix |
| Public distribution | Not qualified | Developer ID/notarization, Windows parity and OS/hardware matrix |

The app declares deployment minimum macOS 14.0; this does not establish runtime
support on macOS 14/15/26 or Intel. No public release or numerical-parity claim
follows from passing native build, unit tests or local artifact inspection.
