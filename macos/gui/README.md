# Original FxSound desktop GUI on macOS

The app builds the existing `fxsound/Source/GUI` main window, Pro and Lite views,
controls, equalizer, visualizer, dialogs, theme, SVG images and Gilroy fonts.
The macOS controller replaces the Windows controller and audio transport. Window
layout and appearance remain defined by the original components.

JUCE 6.1.6 failed against Xcode 27 because `CGWindowListCreateImage` is unavailable.
This build uses official JUCE 8.0.15 at commit
`91ad83ae34a81e0833b1a2b0866f54846370ae53`. Configure the root macOS CMake project
with `FXSOUND_JUCE_SOURCE` pointing to that checkout and build target `FxSound`.
Each helper is ad-hoc signed before the complete app is signed and verified.

The worker uses the private version-1 Unix socket, validates the complete dynamic
control state, batches parameter changes, and displays actual engine controls and
spectrum. EQ counts 5, 10, 15, 20 and 31, editable band centers, gain, balance,
master gain, volume leveling and filter Q call the real DSP APIs. Factory and user
`.fac` presets are loaded by the engine; edits save to the private session preset.
Factory presets and translations are bundled, and user settings use Application
Support/FxSound. Closing the window hides it; the menu bar icon reopens it. Explicit
Quit restores the physical audio output and gracefully shuts down the engine.

Install `FxSound-Install.pkg` from the disk image before normal use. A missing driver
leaves processing off and the original styled message explains installation.
Windows global hotkeys, automatic updates, startup registration and automatic
output priority do not have native equivalents in this personal build. Their
settings do not claim successful operation; use macOS Login Items for startup.

`--smoke-test` opens the real GUI for five seconds without starting the engine or
changing audio routing. Add `--preview` to export the actual component render to
`/private/tmp/fxsound-original-gui-preview.png`. This is a rendering check, not an
installed-driver or audio verification. Real audio, driver loading and macOS
version compatibility still require installation and testing on the target Mac.
