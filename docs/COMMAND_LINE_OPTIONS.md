# FxSound Command Line Options

FxSound accepts a set of command line options that let you configure the app on launch, or push new settings to an already-running instance.

For example, run `fxsound.exe --preset "Bass Booster"` at any time to switch the running instance's preset without restarting it or affecting any other setting.

Options with numeric values that fall outside their valid range are silently reset to their default — no error is shown.

Values containing spaces (preset names, device names) must be wrapped in double quotes.

| Option | Value | Description |
|---|---|---|
| `--power <0\|1>` | `0` = off, `1` = on | Turns FxSound's audio processing (DFX) on or off. |
| `--preset <name>` | Preset name (case-sensitive) | Selects the active preset. On a running instance, this only takes effect if power is currently on. |
| `--output <device>` | Output device's friendly name | Selects the audio output device. Must match a device name returned by Windows exactly. |
| `--view <1\|2>` | `1` = Lite, `2` = Full | Switches the UI between Lite and Full view. |
| `--language <code>` | Language code, e.g. `en`, `fr`, `fi` | Sets the display language. Falls back to the saved language, then the system display language, if omitted. |
| `--num_bands <n>` | Integer, `5`–`31` | Sets the number of equalizer bands. Default: `10`. |
| `--balance <n>` | Decimal, `-20.0`–`+20.0` | Sets the left/right stereo balance in dB. Default: `0`. |
| `--filter_q <n>` | Decimal, `1.0`–`3.0` | Sets the EQ filter Q factor (controls the bandwidth of each band). Default: `1.0`. |
| `--master_gain <n>` | Decimal, `-20.0`–`+20.0` | Sets the master gain in dB. Default: `0`. |
| `--normalization <n>` | Decimal, `-20.0`–`0.0` | Sets the normalization level in dB. Default: `0`. |
| `--run_minimized` | *(flag, no value)* | Starts FxSound minimized to the system tray without showing the main window. On a running instance, only affects visibility for that launch (hides the main window instead of bringing it to the front). |

## Examples

Start FxSound minimized, with a specific preset and output device:

```
fxsound.exe --preset "Bass Booster" --output "Speakers (Realtek High Definition Audio)" --run_minimized
```

Switch a running instance to Pro view and turn power on:

```
fxsound.exe --view 2 --power 1
```

Tune the equalizer on a running instance:

```
fxsound.exe --num_bands 15 --filter_q 2.0 --master_gain 3.0 --normalization -6.0
```

Change the display language:

```
fxsound.exe --language fr
```
