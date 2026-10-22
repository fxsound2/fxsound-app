// Compares the tutorial's JavaScript DSP ports against golden output produced by the real
// FxSound engine (DfxDsp built from dsp/, driven by a small C++ harness).
//
// Usage: node tests/golden-diff.js <golden-dir>
//   <golden-dir> must contain in_A.f32, in_B.f32, scenarios.json and out/<scenario>.f32
//   (see tests/README.md for how the golden files are produced).
"use strict";
const fs = require("fs");
const path = require("path");

const js = path.join(__dirname, "..", "js", "dsp");
for (const file of ["controls.js", "sos.js", "aural.js", "lex.js", "wide.js", "maximizer.js", "play.js", "pipeline.js"]) {
  require(path.join(js, file));
}
const { pipeline } = globalThis.FxDsp;

const goldenDir = process.argv[2];
if (!goldenDir) { console.error("usage: node tests/golden-diff.js <golden-dir>"); process.exit(2); }

const readF32 = (p) => { const b = fs.readFileSync(p); return new Float32Array(b.buffer, b.byteOffset, b.byteLength / 4); };
const scenarios = JSON.parse(fs.readFileSync(path.join(goldenDir, "scenarios.json"), "utf8"));

const keyMap = { power: "power", eq: "eq", mg: "masterGainDb", bal: "balanceDb", q: "qMultiplier",
  vl: "volumeLeveling", norm: "normalizationDb", fid: "fidelity", amb: "ambience", sur: "surround",
  dyn: "dynamicBoost", bass: "bass" };

function settingsFrom(args) {
  const s = {};
  for (const a of args) {
    const [k, v] = a.split("=");
    if (k === "gains") s.gains = v.split(",").map(Number);
    else if (k === "power" || k === "eq") s[keyMap[k]] = v !== "0";
    else if (keyMap[k]) s[keyMap[k]] = Number(v);
  }
  return s;
}

const TOLERANCE = 1e-5;
let failures = 0;
for (const [name, [sig, args]] of Object.entries(scenarios)) {
  const input = readF32(path.join(goldenDir, `in_${sig}.f32`));
  const golden = readF32(path.join(goldenDir, "out", `${name}.f32`));
  const { output } = pipeline.render(input, settingsFrom(args));
  let maxErr = 0, at = -1;
  for (let i = 0; i < golden.length; i++) {
    const e = Math.abs(output[i] - golden[i]);
    if (e > maxErr) { maxErr = e; at = i; }
  }
  const ok = maxErr <= TOLERANCE;
  if (!ok) failures++;
  console.log(`${ok ? "PASS" : "FAIL"}  ${name.padEnd(12)} max|err| = ${maxErr.toExponential(3)}` +
    (ok ? "" : `  first worst at frame ${Math.floor(at / 2)} ch ${at % 2}: js ${output[at]} vs engine ${golden[at]}`));
}
console.log(failures ? `\n${failures} scenario(s) exceed ${TOLERANCE}` : `\nAll scenarios within ${TOLERANCE}`);
process.exit(failures ? 1 : 0);
