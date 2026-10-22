// Spot checks of derived DSP parameters against values computed from the C sources.
// Usage: node tests/spot-values.js
"use strict";
const path = require("path");
for (const f of ["controls", "sos", "aural", "lex", "wide", "maximizer", "play", "pipeline"]) {
  require(path.join(__dirname, "..", "js", "dsp", f + ".js"));
}
const { controls, sos, Lex } = globalThis.FxDsp;

let failures = 0;
function check(label, actual, expected, tol) {
  const ok = Math.abs(actual - expected) <= tol;
  if (!ok) failures++;
  console.log(`${ok ? "PASS" : "FAIL"}  ${label.padEnd(44)} ${String(actual).padEnd(22)} expected ${expected}`);
}

const fx = controls.fixedParams(44100);
check("Aural tune (MIDI 53) gain", fx.aural.gain, 0.839410, 5e-6);
check("Aural tune (MIDI 53) a1", fx.aural.a1, 1.652862, 5e-6);
check("Aural tune (MIDI 53) a0", fx.aural.a0, -0.704777, 5e-6);
check("Maximizer look-ahead (samples)", fx.maxi.maxDelay, 33, 0);
check("Maximizer release beta", fx.maxi.releaseBeta, 0.997776, 5e-6);
check("Lex room size (MIDI 64)", fx.lex.roomsize, 1.003937, 5e-6);
check("Lex MasterLen at room size 1.003937", new Lex(44100, fx.lex.roomsize).len, 38040, 0);

const bass = controls.effectParams({ bass: 5 }, 44100).bass;   // slider 5 -> MIDI 64
const bass68 = sos.calcParametric(44100, 90, Math.fround(68 * Math.fround(15 / 127)), 2.5);
check("Bass MIDI 68 b0", bass68.b0, 1.00322233, 5e-7);
check("Bass MIDI 68 b1", bass68.b1, -1.99559883, 5e-7);
check("Bass MIDI 68 b2", bass68.b2, 0.99254058, 5e-7);
check("Bass MIDI 68 a2", bass68.a2, 0.99576291, 5e-7);
check("Bass slider 5 boost (dB)", bass.boostDb, 64 * 15 / 127, 1e-5);

const boostDb = [0, 1.6, 3.1, 4.7, 6.6, 10.0, 11.6, 11.6, 11.6, 11.6, 11.6];
for (let s = 0; s <= 10; s++) {
  check(`Dynamic Boost slider ${s} (dB)`, Math.round(controls.effectParams({ dynamicBoost: s }, 44100).maxi.gainBoostDb * 10) / 10, boostDb[s], 0.051);
}
check("10-band base Q", sos.bandQ(1), 1.5976, 1e-4);
check("Slider 3 -> MIDI", controls.sliderToMidi(3), 38, 0);

console.log(failures ? `\n${failures} check(s) failed` : "\nAll spot checks passed");
process.exit(failures ? 1 : 0);
