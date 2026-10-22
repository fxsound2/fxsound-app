/*
FxSound DSP tutorial - JavaScript port of the stereo widener ("Surround").
Copyright (C) 2025  FxSound LLC
Contributors: www.theremino.com (2025)

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

Port of dsp/ptechDsp/wide/Wide32/Wide32.c (dspsWideProcess32, "Theremino V2.0.3" rewrite).
Mid/side processing: the side (L-R difference) is boosted and the mid slightly reduced.
*/
(function (root) {
  "use strict";
  var ns = root.FxDsp = root.FxDsp || {};
  var f = Math.fround;

  function Wide() {}

  /** p: { intensity } ; buf interleaved stereo, in place. */
  Wide.prototype.process = function (buf, n, p) {
    var side = f(1 + 3.0 * p.intensity);          // computed in double, stored to float
    var comp = f(1 - 0.3 * p.intensity);
    for (var i = 0, k = 0; i < n; i++, k += 2) {
      var in1 = buf[k], in2 = buf[k + 1];
      var mono = f(f(in1 + in2) * f(0.5));
      var l = f(in1 - mono), r = f(in2 - mono);
      mono = f(mono * comp);
      buf[k] = f(mono + f(side * l));
      buf[k + 1] = f(mono + f(side * r));
    }
  };

  ns.Wide = Wide;
})(typeof globalThis !== "undefined" ? globalThis : window);
