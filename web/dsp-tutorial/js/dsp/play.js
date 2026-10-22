/*
FxSound DSP tutorial - JavaScript port of the Play effect chain and its bass boost.
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

Port of dsp/ptechDsp/Play/Play32/Play32.c (dspsPlayProcess32). Order on origin/main:
  Aural (Fidelity) -> Lex (Ambience) -> Wide (Surround) -> bass boost -> Maximizer.
Vocal elimination and the old headphone delay are never enabled by FxSound and are omitted.
*/
(function (root) {
  "use strict";
  var ns = root.FxDsp = root.FxDsp || {};
  var f = Math.fround;
  var BIAS = f(1.0e-30);

  function Play(fs, params) {
    this.aural = new ns.Aural();
    this.lex = new ns.Lex(fs, params.lex.roomsize);
    this.wide = new ns.Wide();
    this.maxi = new ns.Maximizer(fs);
    this.w1 = 0; this.w2 = 0; this.v1 = 0; this.v2 = 0;     // bass biquad state (L, R)
  }

  // Bass boost: 90 Hz peaking biquad, transposed direct form II with a1 == b1.
  Play.prototype.bass = function (buf, n, p) {
    for (var i = 0, k = 0; i < n; i++, k += 2) {
      var in1 = buf[k], in2 = buf[k + 1];
      var out1 = f(this.w1 + f(p.b0 * f(in1 + BIAS)));
      this.w1 = f(f(f(in1 - out1) * p.b1) + this.w2);
      this.w2 = f(f(p.b2 * in1) - f(p.a2 * out1));
      var out2 = f(this.v1 + f(p.b0 * f(in2 + BIAS)));
      this.v1 = f(f(f(in2 - out2) * p.b1) + this.v2);
      this.v2 = f(f(p.b2 * in2) - f(p.a2 * out2));
      buf[k] = out1; buf[k + 1] = out2;
    }
  };

  /** Processes one buffer through the chain. stages: tutorial on/off toggles per effect. */
  Play.prototype.process = function (buf, n, params, stages) {
    stages = stages || {};
    if (params.aural.on && stages.fidelity !== false) this.aural.process(buf, n, params.aural);
    if (params.lex.on && stages.ambience !== false) this.lex.process(buf, n, params.lex);
    if (params.wide.on && stages.surround !== false) this.wide.process(buf, n, params.wide);
    if (params.bass.on && stages.bass !== false) this.bass(buf, n, params.bass);
    if (stages.dynamicBoost !== false) this.maxi.process(buf, n, params.maxi);  // always runs in FxSound
  };

  ns.Play = Play;
})(typeof globalThis !== "undefined" ? globalThis : window);
