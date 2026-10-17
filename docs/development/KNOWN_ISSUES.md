# Known issues and limitations (Linux port)

| ID | Issue | Evidence | Status |
|---|---|---|---|
| D2 | GUI thread mutates DSP state while the RT thread processes; torn filter coefficients possible | TSan, 69 races | open, DEC-4 |
| D3b | Registry mutex taken on the RT thread every block | code + gdb trace | mitigated, DEC-3 |
| D4 | `reroute()` destroys and recreates all links on every node/port event; possible glitches on hot-plug | code | open |
| D6 | `init()` returns 0 even if the processor node id never arrives; `onSoundDeviceChange` is called on the PipeWire loop thread | code | open |
| D7 | Stereo only (FL/FR); multichannel sinks get two channels | code | open |
| D12 | `Prelst.cpp:69/76` passes `sizeof(pointer)/sizeof(wchar_t)` to `swprintf`, truncating the factory/user preset dirs inside the prelst handle (upstream, all platforms) | code | open; handle is currently never created (`preset_list_handle_` unused) |
| D13 | `registry.ini` format is `key=value` per line; values containing a newline would corrupt the file | code | open |
| D14 | The audiopassthru copies of `pt_defs.h`, `reg.h`, `File.h` and others differ from the DSP copies (`long` vs `int32_t`) | diff | build now picks the DSP copies; duplicates remain |
| — | App's own default-sink restore on exit not isolated from WirePlumber's fallback | baseline run | needs a test |
