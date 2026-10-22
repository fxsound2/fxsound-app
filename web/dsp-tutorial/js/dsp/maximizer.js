/*
FxSound DSP tutorial - JavaScript port of the Maximizer ("Dynamic Boost").
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

Port of dsp/ptechDsp/Maximizer/Maxi32/Maxi32.c (dspsMaximizerInit / dspsMaximizerProcess32).
A boost whose amount backs off as the (left-channel) level rises, followed by a look-ahead
peak limiter that keeps every output sample at or below max_output (-0.3 dBFS).
The level estimator is double precision in C (double level, a0, filt_gain); the rest is float.
This stage ALWAYS runs when FxSound is powered on, even with Dynamic Boost at 0.
*/
(function (root) {
  "use strict";
  var ns = root.FxDsp = root.FxDsp || {};
  var f = Math.fround;
  var ENV_BIAS = f(1.0e-24);   // MAXI_ENVELOPE_BIAS

  function Channel(maxLen) {
    this.dly = new Float32Array(maxLen); this.ptr = 0;
    this.maxAbs = 0; this.delta = 0; this.env = 0; this.ramp = 0;
  }

  function Maximizer(fs) {
    var omega = 6.283185 * 0.1 / f(fs);                      // MAXIMIZE_LEVEL_FILT_CUTOFF 0.1 Hz
    var c = Math.cos(omega);
    var root2 = Math.sqrt(c * c - 4.0 * c + 3.0);
    var d = 2.0 - c - root2;
    this.a0 = f(d);                                          // (realtype) cast, stored in a double
    this.filtGain = f(1.0 - d);
    this.level = 0;
    this.L = new Channel(96); this.R = new Channel(96);      // MAXI_MAX_DELAY_LEN
    this.trace = [];
  }

  function envelope(ch, dlyOut, newAbs, p) {
    var absOut;
    if (ch.ramp) {
      absOut = f(Math.abs(dlyOut));
      if (absOut > ch.env) ch.env = absOut;
      if (newAbs > ch.maxAbs) {
        ch.maxAbs = newAbs;
        ch.ramp = p.maxDelay;
        var tmp = f(f(newAbs - ch.env) / f(p.maxDelay + 1));
        if (tmp > ch.delta) ch.delta = tmp;
      } else {
        ch.ramp--;
      }
      ch.env = f(ch.env + ch.delta);
    } else {
      ch.env = f(f(ch.env * p.releaseBeta) + ENV_BIAS);
      absOut = f(Math.abs(dlyOut));
      if (absOut > ch.env) ch.env = absOut;
      if (newAbs > ch.env) {
        ch.maxAbs = newAbs;
        ch.delta = f(f(newAbs - ch.env) / f(p.maxDelay + 1));
        ch.env = f(ch.env + ch.delta);
        ch.ramp = p.maxDelay;
      }
    }
    return ch.env > p.maxOutput ? f(f(dlyOut * p.maxOutput) / ch.env) : dlyOut;
  }

  function delayLine(ch, value, p) {
    var out = ch.dly[ch.ptr];
    ch.dly[ch.ptr] = value;
    ch.ptr++;
    if (ch.ptr >= p.maxDelay) ch.ptr = 0;
    return out;
  }

  /** p: { gainBoost, maxOutput, releaseBeta, targetLevel, maxDelay } ; wet 1.0, dry 0.0. */
  Maximizer.prototype.process = function (buf, n, p) {
    var gainSum = 0;
    for (var i = 0, k = 0; i < n; i++, k += 2) {
      var in1 = buf[k], in2 = buf[k + 1];
      var inSqr = f(in1 * in1);                               // left channel only
      this.level = this.level * this.a0 + inSqr * this.filtGain;   // double precision
      var sqrtLevel = f(Math.sqrt(this.level));
      var gb, result = f(p.gainBoost * sqrtLevel);
      if (result > p.targetLevel) {
        gb = f(p.targetLevel / sqrtLevel);
        if (gb < f(1.06)) gb = f(1.06);
      } else {
        gb = p.gainBoost;
      }
      gainSum += gb;

      var w1 = f(f(gb * p.maxOutput) * in1);
      var d1 = delayLine(this.L, w1, p);
      var out1 = envelope(this.L, d1, f(Math.abs(w1)), p);

      var w2 = f(f(gb * p.maxOutput) * in2);
      var d2 = delayLine(this.R, w2, p);
      var out2 = envelope(this.R, d2, f(Math.abs(w2)), p);

      buf[k] = f(f(out1 * f(1.0)) + f(f(0.0) * in1));       // kerWetDry(wet 1, dry 0)
      buf[k + 1] = f(f(out2 * f(1.0)) + f(f(0.0) * in2));
    }
    this.trace.push(gainSum / n);
  };

  ns.Maximizer = Maximizer;
})(typeof globalThis !== "undefined" ? globalThis : window);
