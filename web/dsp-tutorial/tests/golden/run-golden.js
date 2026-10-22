// Generates deterministic stereo test signals and runs every scenario through the real
// engine (harness.exe), writing golden outputs to <work-dir>/out/<scenario>.f32
// Usage: node run-golden.js [work-dir] [path-to-harness.exe]
const fs = require("fs");
const path = require("path");
const { execFileSync } = require("child_process");

const FS = 44100;
const dir = path.resolve(process.argv[2] || path.join(__dirname, "work"));
const harness = path.resolve(process.argv[3] || path.join(dir, "harness.exe"));
const outDir = path.join(dir, "out");
fs.mkdirSync(outDir, { recursive: true });
if (!fs.existsSync(harness)) { console.error("harness.exe not found: " + harness); process.exit(2); }

// Deterministic PRNG (mulberry32)
function rng(seed) {
  return function () {
    seed |= 0; seed = (seed + 0x6D2B79F5) | 0;
    let t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

// Signal A: 4 s "music-like" stereo: harmonic tones with different L/R mixes, decaying
// envelopes, a little noise, and a loud section at 2.5-3.0 s to exercise the limiter.
function signalA() {
  const n = 4 * FS, buf = new Float32Array(n * 2), r = rng(12345);
  const tones = [
    { f: 110, l: 0.30, r: 0.20 }, { f: 220, l: 0.15, r: 0.25 }, { f: 440, l: 0.20, r: 0.20 },
    { f: 1760, l: 0.06, r: 0.03 }, { f: 5000, l: 0.02, r: 0.04 },
  ];
  for (let i = 0; i < n; i++) {
    const t = i / FS;
    const env = Math.exp(-((t % 1.0)) * 2.0) * (t >= 2.5 && t < 3.0 ? 1.8 : 1.0);
    let L = 0, R = 0;
    for (const k of tones) { const s = Math.sin(2 * Math.PI * k.f * t); L += k.l * s; R += k.r * s; }
    const noise = (r() * 2 - 1) * 0.01;
    buf[2 * i] = (L + noise) * env * 0.8;
    buf[2 * i + 1] = (R - noise) * env * 0.8;
  }
  return buf;
}

// Signal B: 16 s for Volume Leveling: quiet (but audible) for 12 s, then moderate for 4 s.
function signalB() {
  const n = 16 * FS, buf = new Float32Array(n * 2);
  for (let i = 0; i < n; i++) {
    const t = i / FS;
    const amp = t < 12 ? 0.015 : 0.25;
    const s = Math.sin(2 * Math.PI * 330 * t) + 0.5 * Math.sin(2 * Math.PI * 990 * t);
    buf[2 * i] = amp * s;
    buf[2 * i + 1] = amp * s * 0.9;
  }
  return buf;
}

const signals = { A: signalA(), B: signalB() };
for (const [k, v] of Object.entries(signals)) fs.writeFileSync(path.join(dir, `in_${k}.f32`), Buffer.from(v.buffer));

const EQ = "0,-12,0,0,0,12,0,0,6,0";
const scenarios = {
  // Must run first: later scenarios zero the 31 registry bands, which would hide the effect.
  stale31: ["A", ["stale=1"]],
  neutral: ["A", []],
  eq_gains: ["A", [`gains=${EQ}`]],
  eq_gains_q3: ["A", [`gains=${EQ}`, "q=3"]],
  mg_bal_a: ["A", ["mg=6", "bal=-10"]],
  mg_bal_b: ["A", ["mg=-10", "bal=12"]],
  fid5: ["A", ["fid=5"]], fid10: ["A", ["fid=10"]],
  amb2: ["A", ["amb=2"]], amb5: ["A", ["amb=5"]], amb10: ["A", ["amb=10"]],
  sur5: ["A", ["sur=5"]], sur10: ["A", ["sur=10"]],
  dyn0_hot: ["A", ["mg=12"]],
  dyn3: ["A", ["dyn=3"]], dyn5: ["A", ["dyn=5"]], dyn10: ["A", ["dyn=10"]],
  bass5: ["A", ["bass=5"]], bass10: ["A", ["bass=10"]],
  all: ["A", [`gains=${EQ}`, "mg=-4", "bal=4", "fid=5", "amb=5", "sur=5", "dyn=5", "bass=5", "vl=2"]],
  eq_off: ["A", ["eq=0", "mg=6", "fid=5"]],
  power_off: ["A", ["power=0", "mg=6", "fid=5", "vl=2"]],
  vl2: ["B", ["vl=2"]], vl4: ["B", ["vl=4"]],
};

for (const [name, [sig, args]] of Object.entries(scenarios)) {
  const res = execFileSync(harness,
    [path.join(dir, `in_${sig}.f32`), path.join(outDir, `${name}.f32`), ...args], { encoding: "utf8" });
  console.log(name.padEnd(12), sig, args.join(" ").padEnd(70), res.trim());
}
fs.writeFileSync(path.join(dir, "scenarios.json"), JSON.stringify(scenarios, null, 2));
