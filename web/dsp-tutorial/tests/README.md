# Tests for the FxSound DSP tutorial

These tests are for developers. Nothing in this folder is deployed.

| Script | What it checks |
|---|---|
| `spot-values.js` | Derived DSP parameters (filter coefficients, delay lengths, Dynamic Boost table) against values computed from the C sources |
| `golden-diff.js` | The JavaScript pipeline's output against output from the **real FxSound engine**, sample for sample |
| `serve.js` | Serves the page the way the FxSound website would (sub-path + strict CSP) and an iframe embed test page |

## Spot values

```bash
node tests/spot-values.js
```

## Golden comparison against the real engine

This proves the JavaScript ports are exact. It builds the real engine from `dsp/`, runs test audio through it with a
small harness (`golden/harness.cpp`, which drives the public `DfxDsp` API the way `FxController` does), and compares.
Requires Windows with Visual Studio 2022.

> **Registry warning.** The engine reads and writes its state under `HKEY_CURRENT_USER\SOFTWARE\DFX`, the same
> place the installed FxSound app keeps it. Back the key up before running the harness and restore it afterwards,
> as shown below.

1. Build the engine libraries and the harness:

   ```bat
   web\dsp-tutorial\tests\golden\build-harness.cmd
   ```

2. Back up FxSound's registry state, run every scenario through the real engine, then restore it (PowerShell):

   ```powershell
   reg export "HKCU\SOFTWARE\DFX" "$env:TEMP\DFX_backup.reg" /y
   node web\dsp-tutorial\tests\golden\run-golden.js
   reg delete "HKCU\SOFTWARE\DFX" /f; reg import "$env:TEMP\DFX_backup.reg"
   ```

   Golden files are written to `tests/golden/work/` (`in_*.f32`, `out/*.f32`, `scenarios.json`).

3. Compare the JavaScript pipeline with the engine:

   ```bash
   node web/dsp-tutorial/tests/golden-diff.js web/dsp-tutorial/tests/golden/work
   ```

   Every scenario should report `max|err| = 0.000e+0`. A tolerance of 1e-5 is allowed, but on x64 with the default
   compiler settings the match has been exact.

The `stale31` scenario runs first on purpose. It loads whatever 31-band EQ values are in the registry and checks that
switching to 10 bands leaves no old band active.

## Hosting check

```bash
node tests/serve.js 8737
```

- Tutorial: <http://localhost:8737/learn/dsp-tutorial/> (served under a sub-path, with
  `Content-Security-Policy: script-src 'self'; style-src 'self'`). The browser console should be empty.
- Embed test: <http://localhost:8737/embed-test.html> loads the page in a fixed-height (`100vh`) `<iframe>` with
  `?embed=1`, as the bundle README recommends. The iframe should keep its height, the tutorial should scroll inside
  it with the graph staying in view, and the page title should be hidden while the footer stays.
- `--capture <dir>` additionally accepts `POST /__capture/<name>.png` with a canvas data URL. This was used to
  produce the images in `docs/img/dsp-tutorial/`: the page in light mode, the plot forced to 960×360 CSS pixels,
  and the canvas drawn onto an opaque white background before saving. The plot canvas itself is transparent, and a
  transparent PNG would show GitHub's dark theme through it.
