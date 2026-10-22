/*
FxSound DSP tutorial - JavaScript port of the Lex plate reverb ("Ambience").
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

Port of dsp/ptechDsp/Lex/Lex32/Lex32.c (dspsLexReverbInit / dspsLexReverbProcess32),
DFX build (PT_DSP_BUILD == PT_DSP_DFX): the delay-modulation oscillator is compiled out,
so lattices 5 and 7 use fixed integer delays.

All delay lines live in ONE circular buffer walked by ONE pointer: per sample the pointer
advances by every segment's length in turn (MasterLen + 1 in total, i.e. +1 modulo
MasterLen). The C code compares pointers after casting them to 32-bit `long`, which on x64
only compares their low 32 bits; this port uses array indices (the intended behaviour).
*/
(function (root) {
  "use strict";
  var ns = root.FxDsp = root.FxDsp || {};
  var f = Math.fround;
  var DENORM_BIAS = f(1.0e-36);   // boardrv1.h DSP_DENORM_BIAS

  // c_lex.h lengths in seconds (double constants)
  var L = {
    PRE: 100.0 / 1000.0, LAT1: 4.77 / 1000.0, LAT2: 3.595 / 1000.0, LAT3: 12.73 / 1000.0,
    LAT4: 9.31 / 1000.0, LAT5: 22.6 / 1000.0, MOD: 2.0 / 1000.0,
    D1: [10.1 / 1000.0, 66.9 / 1000.0, 121.9 / 1000.0, 149.6 / 1000.0],
    LAT6: [6.28 / 1000.0, 41.26 / 1000.0, 60.5 / 1000.0],
    D2: [35.8 / 1000.0, 89.8 / 1000.0, 125.0 / 1000.0], LAT7: 30.5 / 1000.0,
    D3: [10.1 / 1000.0, 70.9 / 1000.0, 99.9 / 1000.0, 141.7 / 1000.0],
    LAT8: [11.25 / 1000.0, 64.3 / 1000.0, 89.2 / 1000.0],
    D4: [4.065 / 1000.0, 67.1 / 1000.0, 106.3 / 1000.0]
  };

  /** fs: sample rate; roomsize: float room size (sets every delay length). */
  function Lex(fs, roomsize) {
    fs = f(fs);
    var flt = function (sec) { return Math.trunc(f(fs * f(sec))); };          // float * float
    var r = f(roomsize * fs);
    var dbl = function (sec) { return Math.trunc(sec * r); };                 // double * float
    this.pre = flt(L.PRE);
    this.lat1 = flt(L.LAT1); this.lat2 = flt(L.LAT2); this.lat3 = flt(L.LAT3); this.lat4 = flt(L.LAT4);
    this.lat5 = dbl(L.LAT5);
    this.lat5max = this.lat5 + flt(L.MOD) + 1;
    this.d1 = L.D1.map(dbl);
    this.lat6t1 = dbl(L.LAT6[0]); this.lat6t2 = dbl(L.LAT6[1]); this.lat6 = dbl(L.LAT6[2]);
    this.d2 = L.D2.map(dbl);
    this.lat7 = dbl(L.LAT7);
    this.lat7max = this.lat7 + flt(L.MOD) + 1;
    this.d3 = L.D3.map(dbl);
    this.lat8t1 = dbl(L.LAT8[0]); this.lat8t2 = dbl(L.LAT8[1]); this.lat8 = dbl(L.LAT8[2]);
    this.d4 = L.D4.map(dbl);
    this.len = this.pre + this.lat1 + this.lat2 + this.lat3 + this.lat4 + this.lat5max + this.d1[3]
      + this.lat6 + this.d2[2] + this.lat7max + this.d3[3] + this.lat8 + this.d4[2];
    this.mem = new Float32Array(this.len);
    this.ptr = 0;
    this.bw = 0; this.damp1 = 0; this.damp2 = 0; this.d4out = 0;
  }

  /** p: { decay, lat6, wet, dry, bandwidth, oneMinusBandwidth, damping, oneMinusDamping,
   *       lat1, lat3, lat5, preDelay } ; buf interleaved stereo, in place. */
  Lex.prototype.process = function (buf, n, p) {
    var m = this.mem, len = this.len, end = len - 1;
    var self = this;
    function adv(step) { self.ptr += step; if (self.ptr > end) self.ptr -= len; }
    function tap(back) { var q = self.ptr - back; if (q < 0) q += len; return m[q]; }

    for (var i = 0, k = 0; i < n; i++, k += 2) {
      var in1 = f(buf[k] + DENORM_BIAS), in2 = f(buf[k + 1] + DENORM_BIAS);
      var out1, out2, ta, tb, t1, t2, t3, t4, next, din, dout, ido;

      adv(this.pre + 1);                                      // pre-delay
      tb = tap(p.preDelay);
      next = m[this.ptr];
      m[this.ptr] = f(in1 + in2);

      ta = f(f(tb * p.oneMinusBandwidth) + f(p.bandwidth * this.bw));   // bandwidth lowpass
      this.bw = ta;

      adv(this.lat1); dout = next; din = f(ta - f(p.lat1 * dout)); next = m[this.ptr]; m[this.ptr] = din; tb = f(dout + f(p.lat1 * din));
      adv(this.lat2); dout = next; din = f(tb - f(p.lat1 * dout)); next = m[this.ptr]; m[this.ptr] = din; ta = f(dout + f(p.lat1 * din));
      adv(this.lat3); dout = next; din = f(ta - f(p.lat3 * dout)); next = m[this.ptr]; m[this.ptr] = din; tb = f(dout + f(p.lat3 * din));
      adv(this.lat4); dout = next; din = f(tb - f(p.lat3 * dout));                       m[this.ptr] = din; ido = f(dout + f(p.lat3 * din));

      // Lattice 5 (decay diffuser, fixed delay in the DFX build)
      tb = f(ido + f(p.decay * this.d4out));
      adv(this.lat5max);
      dout = tap(this.lat5);
      din = f(tb + f(p.lat5 * dout)); next = m[this.ptr]; m[this.ptr] = din; ta = f(dout - f(p.lat5 * din));

      // D1: 4 taps
      adv(this.d1[3]); t4 = next; next = m[this.ptr]; m[this.ptr] = ta;
      t1 = tap(this.d1[0]); t2 = tap(this.d1[1]); t3 = tap(this.d1[2]);
      out1 = f(-t2); out2 = f(t1 + t3);

      ta = f(f(t4 * p.oneMinusDamping) + f(p.damping * this.damp1)); this.damp1 = ta;   // damping lowpass
      ta = f(ta * p.decay);

      // Lattice 6 (tapped)
      adv(this.lat6); dout = next; din = f(ta - f(p.lat6 * dout)); next = m[this.ptr]; m[this.ptr] = din; tb = f(dout + f(p.lat6 * din));
      t1 = tap(this.lat6t1); t2 = tap(this.lat6t2);
      out1 = f(out1 - t1); out2 = f(out2 - t2);

      // D2: 3 taps (does not fetch next_out)
      adv(this.d2[2]); t3 = next; m[this.ptr] = tb;
      t1 = tap(this.d2[0]); t2 = tap(this.d2[1]);
      out1 = f(out1 - t1); out2 = f(out2 + t2);

      // Lattice 7
      tb = f(ido + f(p.decay * t3));
      adv(this.lat7max);
      dout = tap(this.lat7);
      din = f(tb + f(p.lat5 * dout)); next = m[this.ptr]; m[this.ptr] = din; ta = f(dout - f(p.lat5 * din));

      // D3: 4 taps
      adv(this.d3[3]); t4 = next; next = m[this.ptr]; m[this.ptr] = ta;
      t1 = tap(this.d3[0]); t2 = tap(this.d3[1]); t3 = tap(this.d3[2]);
      out1 = f(out1 + f(t1 + t3)); out2 = f(out2 - t2);

      ta = f(f(t4 * p.oneMinusDamping) + f(p.damping * this.damp2)); this.damp2 = ta;
      ta = f(ta * p.decay);

      // Lattice 8 (tapped, uses lat6 coefficient)
      adv(this.lat8); dout = next; din = f(ta - f(p.lat6 * dout)); next = m[this.ptr]; m[this.ptr] = din; tb = f(dout + f(p.lat6 * din));
      t1 = tap(this.lat8t1); t2 = tap(this.lat8t2);
      out1 = f(out1 - t2); out2 = f(out2 - t1);

      // D4: 3 taps (does not fetch next_out)
      adv(this.d4[2]); t3 = next; m[this.ptr] = tb;
      t1 = tap(this.d4[0]); t2 = tap(this.d4[1]);
      out1 = f(out1 + t2); out2 = f(out2 - t1);
      this.d4out = t3;

      out1 = f(out1 * f(0.6 * 0.5)); out2 = f(out2 * f(0.6 * 0.5));
      buf[k] = f(f(out1 * p.wet) + f(p.dry * in1));          // kerWetDry (dry uses biased input)
      buf[k + 1] = f(f(out2 * p.wet) + f(p.dry * in2));
    }
  };

  ns.Lex = Lex;
})(typeof globalThis !== "undefined" ? globalThis : window);
