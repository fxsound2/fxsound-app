# FxSound DSP Tutorial: web bundle

An interactive page that teaches raw PCM audio (44.1 kHz, 24-bit, dBFS) and shows what each stage of the FxSound
audio engine does to a waveform. The concepts are documented in [`docs/DSP_TUTORIAL.md`](../../docs/DSP_TUTORIAL.md).

The audio processing in `js/dsp/` is a JavaScript port of the FxSound engine in `dsp/`. It produces output identical
to the real engine (see [tests/README.md](tests/README.md)).

## Publishing

The page is fully static: no build step, no server code, no third-party requests. It is published with
**GitHub Pages** at <https://fxsound2.github.io/fxsound-app/dsp-tutorial/>.

The workflow [`.github/workflows/publish-dsp-tutorial.yml`](../../.github/workflows/publish-dsp-tutorial.yml) runs
whenever a change under `web/dsp-tutorial/` reaches `main` (it can also be started by hand from the Actions tab). It
copies only the files the page needs, never `tests/` or this README:

```
index.html
css/tutorial.css
js/pcm.js  js/explain.js  js/ui.js
js/dsp/controls.js  js/dsp/sos.js  js/dsp/aural.js  js/dsp/lex.js
js/dsp/wide.js  js/dsp/maximizer.js  js/dsp/play.js  js/dsp/pipeline.js
```

GitHub Pages must be set to **Settings → Pages → Source: GitHub Actions** (already done for this repository).

### Updating the page

1. Change the files under `web/dsp-tutorial/`.
2. Bump the version in two places so browsers fetch the new files instead of cached ones:
   - the `?v=` suffix on every `<link>` and `<script>` in `index.html`
   - `VERSION` at the top of `js/ui.js` (shown in the page footer)
3. Merge to `main`. The workflow republishes the page within a few minutes.

### Linking from fxsound.com

fxsound.com runs on Squarespace, which can't host a folder of files with its own paths, so the page stays on GitHub
Pages and fxsound.com links to it or embeds it:

- **Link:** point a menu item, button or blog post at the URL above. Keep the trailing `/`: without it the browser
  resolves `css/` and `js/` one folder too high and the page loads unstyled.
- **Embed:** add a Code Block with the iframe snippet under [Embedding in another page](#embedding-in-another-page),
  using the full URL. The height-reporting `<script>` part needs a Squarespace plan that allows JavaScript in Code
  Blocks; without it the iframe still works at the fixed height you give it.

Optional: to serve the page from an fxsound.com address (for example `learn.fxsound.com`), add a DNS CNAME record
`learn → fxsound2.github.io`, enter the domain under **Settings → Pages → Custom domain**, tick **Enforce HTTPS**,
and verify the domain in the fxsound2 organisation's Pages settings. Then update the URLs in this README and in
[`docs/DSP_TUTORIAL.md`](../../docs/DSP_TUTORIAL.md).

The page names the website's fonts (Nunito Sans, Rubik) first. They are used when the visitor has them; otherwise
the system font is used, because the page loads nothing from third parties.

### Content-Security-Policy

The page uses no inline scripts, inline styles, event-handler attributes or `eval`, so it works under a strict
policy such as:

```
default-src 'none'; script-src 'self'; style-src 'self'; img-src 'self' data:; media-src 'self' blob:
```

Audio playback uses the Web Audio API and needs no extra permissions.

### Embedding in another page

Add `?embed=1` to hide the page's own header and footer. The page then reports its height to the host page so the
iframe can fit its content:

```html
<iframe id="fxsound-dsp-tutorial" src="https://fxsound2.github.io/fxsound-app/dsp-tutorial/?embed=1"
        title="FxSound DSP Tutorial" style="width:100%;border:0;height:900px"></iframe>
<script>
  window.addEventListener("message", function (e) {
    if (e.data && e.data.type === "fxsound-dsp-tutorial:height") {
      document.getElementById("fxsound-dsp-tutorial").style.height = e.data.height + "px";
    }
  });
</script>
```

### Storage

The page remembers the visitor's control positions in `localStorage` (key `fxsound-dsp-tutorial:v1`). Nothing else is
stored or sent anywhere.

## Running it locally

Open `index.html` directly in a browser, or serve it with `node tests/serve.js` (see [tests/README.md](tests/README.md)).

## Licence

The JavaScript DSP ports are derived from FxSound's engine and are licensed under the
[GNU Affero General Public License v3](https://www.gnu.org/licenses/agpl-3.0.html), like the rest of this repository.
The page footer links to this source code, which satisfies the AGPL's source-availability requirement when the page
is served to users.

## Structure

| File | Purpose |
|---|---|
| `js/pcm.js` | Piano synthesis, peak level (dBFS), 24-bit PCM encoding |
| `js/dsp/controls.js` | FxSound slider → internal value → DSP parameter mappings |
| `js/dsp/sos.js` | Equalizer stage: band filters, Master Gain, Balance, Normalization (fixed, no control), Volume Leveling |
| `js/dsp/aural.js` | Clarity (harmonic exciter) |
| `js/dsp/lex.js` | Ambience (plate reverb) |
| `js/dsp/wide.js` | Surround Sound (mid/side widener) |
| `js/dsp/maximizer.js` | Dynamic Boost (look-ahead limiter) |
| `js/dsp/play.js` | Effects chain order and Bass Boost |
| `js/dsp/pipeline.js` | Processing order, power/equalizer rules, 10 ms buffering |
| `js/explain.js` | Text for the "What's happening" panel |
| `js/ui.js` | Controls, plots, playback, embed mode |
