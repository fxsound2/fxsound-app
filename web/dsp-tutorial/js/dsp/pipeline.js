/*
FxSound DSP tutorial - the FxSound processing pipeline (stereo, power and EQ semantics).
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

Mirrors dsp/ptutil/dfxp/dfxpProcessReal.cpp (dfxpModifyRealtypeSamples) for 44.1 kHz stereo:
  power on,  EQ on : EQ stage [bands, master gain x balance, normalization, volume leveling]
                     then Play [Fidelity, Ambience, Surround, Bass, Dynamic Boost]
  power on,  EQ off: Play only (the whole EQ stage, incl. master gain/balance/leveling, is skipped)
  power off, EQ on : master gain only
  power off, EQ off: nothing
FxSound receives audio from Windows in buffers; the tutorial uses 10 ms (441-frame) buffers,
which matters for Volume Leveling because it adapts once per buffer.
*/
(function (root) {
  "use strict";
  var ns = root.FxDsp = root.FxDsp || {};

  var DEFAULTS = {
    power: true, eq: true,
    gains: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0], qMultiplier: 1, masterGainDb: 0, balanceDb: 0,
    normalizationDb: 0, volumeLeveling: 0,
    fidelity: 0, ambience: 0, surround: 0, dynamicBoost: 0, bass: 0
  };

  function withDefaults(s) {
    var out = {};
    for (var key in DEFAULTS) out[key] = (s && s[key] !== undefined) ? s[key] : DEFAULTS[key];
    return out;
  }

  /**
   * Renders interleaved stereo float32 audio through FxSound.
   * @param {Float32Array} input   interleaved L/R samples
   * @param {object} settings      FxSound control values (see DEFAULTS)
   * @param {object} stages        tutorial isolation toggles: eqBands, gain, leveling,
   *                               normalization, fidelity, ambience, surround, bass, dynamicBoost
   * @returns {{output: Float32Array, params: object, levelingGain: number[], boostGain: number[]}}
   */
  function render(input, settings, stages, opts) {
    var s = withDefaults(settings);
    stages = stages || {};
    opts = opts || {};
    var fs = opts.sampleRate || 44100;
    var block = opts.bufferFrames || 441;
    var out = new Float32Array(input);
    var frames = out.length / 2;

    var eq = new ns.sos.EqStage(fs, s);
    var params = ns.controls.effectParams(s, fs);
    var play = new ns.Play(fs, params);
    var eqOpts = { eq: stages.eqBands !== false, gain: stages.gain !== false,
                   leveling: stages.leveling !== false, normalization: stages.normalization !== false };

    for (var f0 = 0; f0 < frames; f0 += block) {
      var n = Math.min(block, frames - f0);
      var buf = out.subarray(f0 * 2, (f0 + n) * 2);
      if (s.power) {
        if (s.eq) eq.process(buf, n, eqOpts);
        play.process(buf, n, params, stages);
      } else if (s.eq) {
        eq.processMasterGainOnly(buf);
      }
    }
    return { output: out, params: params, levelingGain: eq.trace, boostGain: play.maxi.trace,
             bufferFrames: block, eqQ: eq.Q };
  }

  ns.pipeline = { render: render, DEFAULTS: DEFAULTS };
})(typeof globalThis !== "undefined" ? globalThis : window);
