/*
FxSound DSP tutorial - JavaScript port of the Aural exciter ("Fidelity" / "Clarity").
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

Port of dsp/ptechDsp/Aural/Aural032/Auralp32.c (dspsAuralProcess32), float arithmetic.
A 2nd-order Butterworth high-pass isolates the treble, which is driven into sin() to
create odd harmonics and mixed back with the dry signal.
*/
(function (root) {
  "use strict";
  var ns = root.FxDsp = root.FxDsp || {};
  var f = Math.fround;
  var BIAS = f(1.0e-30);

  function Aural() { this.o1 = 0; this.o2 = 0; this.i1 = 0; this.i2 = 0; // left: out-1, out-2, in-1, in-2
                     this.p1 = 0; this.p2 = 0; this.j1 = 0; this.j2 = 0; } // right

  /** p: { drive, odd, even, gain, a1, a0, wet, dry } ; buf interleaved stereo, in place. */
  Aural.prototype.process = function (buf, n, p) {
    for (var i = 0, k = 0; i < n; i++, k += 2) {
      var in1 = buf[k], in2 = buf[k + 1];

      var h = f(f(this.o1 * p.a1) + f(this.o2 * p.a0));
      this.o2 = this.o1;
      h = f(h + f(f(f(f(in1 + BIAS) - f(2 * this.i1)) + this.i2) * p.gain));
      this.o1 = h; this.i2 = this.i1; this.i1 = in1;
      h = f(h * p.drive);
      var odd1 = f(Math.sin(h)), even1 = h > 0 ? h : 0;
      var out1 = f(in1 + f(f(p.even * even1) + f(p.odd * odd1)));

      var h2 = f(f(this.p1 * p.a1) + f(this.p2 * p.a0));
      this.p2 = this.p1;
      h2 = f(h2 + f(f(f(f(in2 + BIAS) - f(2 * this.j1)) + this.j2) * p.gain));
      this.p1 = h2; this.j2 = this.j1; this.j1 = in2;
      h2 = f(h2 * p.drive);
      var odd2 = f(Math.sin(h2)), even2 = h2 > 0 ? h2 : 0;
      var out2 = f(in2 + f(f(p.even * even2) + f(p.odd * odd2)));

      buf[k] = f(f(out1 * p.wet) + f(p.dry * in1));        // kerWetDry
      buf[k + 1] = f(f(out2 * p.wet) + f(p.dry * in2));
    }
  };

  ns.Aural = Aural;
})(typeof globalThis !== "undefined" ? globalThis : window);
