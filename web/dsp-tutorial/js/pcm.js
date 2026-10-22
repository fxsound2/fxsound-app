/*
FxSound DSP tutorial - raw PCM generation: piano notes, dBFS level, 24-bit quantization.
Copyright (C) 2025  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
(function (root) {
  "use strict";
  var ns = root.FxPcm = root.FxPcm || {};

  var SAMPLE_RATE = 44100;
  var BIT_DEPTH = 24;
  var FULL_SCALE = 8388608;          // 2^23: the 24-bit code for an amplitude of 1.0
  var NOTE_SECONDS = 1.0;            // a half note at 120 bpm in 4/4
  var RING_SECONDS = 0.3;            // each note rings into the next (legato), so there are no silent gaps
  var TAIL_SECONDS = 1.5;            // room for the reverb tail after the last note

  // ---- Notes: white keys C1..C7 (A4 = 440 Hz, equal temperament) ----------------
  var NAMES = ["C", "D", "E", "F", "G", "A", "B"];
  var SEMITONES = [0, 2, 4, 5, 7, 9, 11];
  var NOTES = [];
  for (var octave = 1; octave <= 7; octave++) {
    for (var i = 0; i < 7; i++) {
      if (octave === 7 && i > 0) break;
      var midi = 12 * (octave + 1) + SEMITONES[i];
      NOTES.push({ name: NAMES[i] + octave, midi: midi, freq: 440 * Math.pow(2, (midi - 69) / 12) });
    }
  }

  // ---- Piano-like additive synthesis ---------------------------------------------
  // Partials get slightly sharper with order (string stiffness), softer with order, and
  // decay faster the higher they are. The note is panned by key position (low notes left,
  // high notes right) and the right channel's partials are offset slightly in phase, so
  // the stereo image has some width for the Surround control to act on.
  var noteCache = {};
  function renderNote(out, startFrame, note) {
    var cached = noteCache[note.name];
    if (!cached) {
      cached = new Float32Array(2 * Math.round((NOTE_SECONDS + RING_SECONDS) * SAMPLE_RATE));
      synthesizeNote(cached, note);
      noteCache[note.name] = cached;
    }
    for (var i = 0; i < cached.length; i++) out[2 * startFrame + i] += cached[i];
  }

  function synthesizeNote(out, note) {
    var startFrame = 0;
    var frames = Math.round((NOTE_SECONDS + RING_SECONDS) * SAMPLE_RATE);
    var f0 = note.freq;
    var B = 0.00012 * Math.pow(f0 / 261.63, 0.5);                 // inharmonicity
    var baseTau = Math.min(3.0, 1.6 * Math.pow(261.63 / f0, 0.45)); // low notes ring longer
    var pan = Math.max(-0.8, Math.min(0.8, (note.midi - 60) / 30));
    var gl = Math.cos((pan + 1) * Math.PI / 4), gr = Math.sin((pan + 1) * Math.PI / 4);
    var partials = [];
    for (var n = 1; n <= 24; n++) {
      var fn = n * f0 * Math.sqrt(1 + B * n * n);
      if (fn > SAMPLE_RATE * 0.45) break;
      partials.push({
        w: 2 * Math.PI * fn / SAMPLE_RATE,
        amp: Math.abs(Math.sin(Math.PI * n * 0.13)) / Math.pow(n, 1.05),  // hammer at 1/8 of string
        tau: baseTau / (1 + 0.35 * (n - 1)),
        phaseR: 0.35 * n
      });
    }
    var attack = Math.round(0.004 * SAMPLE_RATE), release = Math.round(RING_SECONDS * SAMPLE_RATE);
    for (var t = 0; t < frames; t++) {
      var sec = t / SAMPLE_RATE, l = 0, r = 0;
      for (var p = 0; p < partials.length; p++) {
        var q = partials[p], e = q.amp * Math.exp(-sec / q.tau);
        l += e * Math.sin(q.w * t);
        r += e * Math.sin(q.w * t + q.phaseR);
      }
      var env = Math.min(1, t / attack) * Math.min(1, (frames - t) / release);
      var k = 2 * (startFrame + t);
      out[k] += l * env * gl;
      out[k + 1] += r * env * gr;
    }
  }

  /** Renders a melody (array of NOTES entries) as interleaved stereo float samples. */
  function renderSequence(notes, repeatToSeconds) {
    var seq = notes.slice();
    if (repeatToSeconds) {
      while (seq.length && seq.length * NOTE_SECONDS < repeatToSeconds) seq = seq.concat(notes);
    }
    var noteFrames = Math.round(NOTE_SECONDS * SAMPLE_RATE);
    var total = seq.length * noteFrames + Math.round(TAIL_SECONDS * SAMPLE_RATE);
    var out = new Float32Array(total * 2);
    for (var i = 0; i < seq.length; i++) renderNote(out, i * noteFrames, seq[i]);
    return out;
  }

  // ---- Levels ----------------------------------------------------------------------
  function toDbfs(amplitude) { return amplitude > 0 ? 20 * Math.log10(amplitude) : -Infinity; }

  function measure(buf) {
    var peak = 0, sum = 0;
    for (var i = 0; i < buf.length; i++) {
      var a = Math.abs(buf[i]);
      if (a > peak) peak = a;
      sum += buf[i] * buf[i];
    }
    var rms = buf.length ? Math.sqrt(sum / buf.length) : 0;
    return { peak: peak, rms: rms, peakDbfs: toDbfs(peak), rmsDbfs: toDbfs(rms) };
  }

  /** Scales the buffer so its highest peak sits exactly at `peakDbfs`. */
  function setPeakLevel(buf, peakDbfs) {
    var peak = measure(buf).peak;
    if (peak === 0) return buf;
    var g = Math.pow(10, peakDbfs / 20) / peak;
    for (var i = 0; i < buf.length; i++) buf[i] *= g;
    return buf;
  }

  // ---- 24-bit PCM ------------------------------------------------------------------
  /**
   * Converts one float sample (nominally -1.0 .. +1.0) to a signed 24-bit integer PCM code.
   * Valid codes run from -8388608 to +8388607 (one more negative code than positive).
   */
  function quantizeTo24Bit(sample) {
      return Math.max(
          -FULL_SCALE,
          Math.min(FULL_SCALE - 1, Math.round(sample * FULL_SCALE))
      );
  }

  /** Encodes a float buffer to 24-bit codes and decodes it back, as a 24-bit WAV would be. */
  function to24BitPcm(buf) {
    var codes = new Int32Array(buf.length);
    var decoded = new Float32Array(buf.length);
    for (var i = 0; i < buf.length; i++) {
      codes[i] = quantizeTo24Bit(buf[i]);
      decoded[i] = codes[i] / FULL_SCALE;
    }
    return { codes: codes, samples: decoded };
  }

  ns.SAMPLE_RATE = SAMPLE_RATE;
  ns.BIT_DEPTH = BIT_DEPTH;
  ns.FULL_SCALE = FULL_SCALE;
  ns.NOTE_SECONDS = NOTE_SECONDS;
  ns.NOTES = NOTES;
  ns.renderSequence = renderSequence;
  ns.measure = measure;
  ns.toDbfs = toDbfs;
  ns.setPeakLevel = setPeakLevel;
  ns.quantizeTo24Bit = quantizeTo24Bit;
  ns.to24BitPcm = to24BitPcm;
})(typeof globalThis !== "undefined" ? globalThis : window);
