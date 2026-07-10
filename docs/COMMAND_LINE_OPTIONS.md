# FxSound Command Line Options

FxSound accepts a set of command line options that let you configure the app on launch, or push new settings to an already-running instance.

For example, run `fxsound.exe --preset="Bass Booster"` at any time to switch the running instance's preset without restarting it or affecting any other setting.

Each option's value must be attached directly to the option with `=` — e.g. `--power=1`. A space between the option and its value is **not** supported for these options; `--power 1` is parsed as two separate, unrelated arguments and the value is silently ignored.

Options with numeric values that fall outside their valid range are silently reset to their default — no error is shown.

Values containing spaces (preset names, device names) must be wrapped in double quotes immediately after the `=`, e.g. `--preset="Bass Booster"`.

| Option | Value | Description |
|---|---|---|
| `--power=<0\|1>` | `0` = off, `1` = on | Turns FxSound's audio processing (DFX) on or off. |
| `--preset=<name>` | Preset name (case-sensitive) | Selects the active preset. On a running instance, this only takes effect if power is currently on. |
| `--output=<device>` | Output device's friendly name | Selects the audio output device. Must match a device name returned by Windows exactly. |
| `--view=<1\|2>` | `1` = Lite, `2` = Full | Switches the UI between Lite and Full view. |
| `--language=<code>` | Language code, e.g. `en`, `fr`, `fi` | Sets the display language. Falls back to the saved language, then the system display language, if omitted. |
| `--num_bands=<n>` | Integer, one of `5`, `10`, `15`, `20`, `31` | Sets the number of equalizer bands. Default: `10`. |
| `--balance=<n>` | Decimal, `-20.0`–`+20.0` | Sets the left/right stereo balance in dB. Default: `0`. |
| `--filter_q=<n>` | Decimal, `1.0`–`3.0` | Sets the EQ filter Q factor (controls the bandwidth of each band). Default: `1.0`. |
| `--master_gain=<n>` | Decimal, `-20.0`–`+20.0` | Sets the master gain in dB. Default: `0`. |
| `--normalization=<n>` | Decimal, `-20.0`–`0.0` | Sets the normalization level in dB. Default: `0`. |
| `--set_band_freq=<band:freq[,band:freq...]>` | `band` = 0-based band index, `freq` = frequency in Hz | Sets one or more equalizer band center frequencies on a running instance. `band` must be less than the current number of bands; `freq` must fall within that band's allowed range or the pair is ignored. Running instance only. |
| `--set_band_gain=<band:gain[,band:gain...]>` | `band` = 0-based band index, `gain` = decimal, `-12.0`–`+12.0` | Sets one or more equalizer band boost/cut values in dB on a running instance. `band` must be less than the current number of bands, or the pair is ignored. Running instance only. |
| `--set_effect=<name:value[,name:value...]>` | `name` = one of `fidelity`/`clarity`, `ambience`, `surround`, `dynamicboost`/`dynamic_boost`, `bass`/`bassboost`/`bass_boost`; `value` = decimal, `0.0`–`10.0` | Sets one or more effect levels on a running instance. Out-of-range values are ignored (left unchanged) rather than reset to a default. Running instance only. |
| `--status` | *(flag, no value)* | Writes the running instance's current state (power, presets, output devices, equalizer, effects) as JSON to `%APPDATA%\FxSound\status.json`; any other options passed alongside `--status` are ignored. |
| `--run_minimized` | *(flag, no value)* | Starts FxSound minimized to the system tray without showing the main window. On a running instance, only affects visibility for that launch (hides the main window instead of bringing it to the front). |

## Examples

Start FxSound minimized, with a specific preset and output device:

```
fxsound.exe --preset="Bass Booster" --output="Speakers (Realtek High Definition Audio)" --run_minimized
```

Switch a running instance to Pro view and turn power on:

```
fxsound.exe --view=2 --power=1
```

Tune the equalizer on a running instance:

```
fxsound.exe --num_bands=15 --filter_q=2.0 --master_gain=3.0 --normalization=-6.0
```

Set individual equalizer band frequencies and gains on a running instance:

```
fxsound.exe --set_band_freq="0:60,1:150" --set_band_gain="0:3.0,1:-2.5"
```

Set effect levels on a running instance:

```
fxsound.exe --set_effect="bass:7.5,ambience:4.0"
```

Change the display language:

```
fxsound.exe --language=fr
```

Print the running instance's current status as JSON:

```
fxsound.exe --status
```
