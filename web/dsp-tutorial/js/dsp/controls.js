/*
FxSound DSP tutorial - JavaScript port of the FxSound control mappings.
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

Ports (from https://github.com/fxsound2/fxsound-app, dsp/):
  ptutil/Qnt/Qntitor.cpp      qntIToRInit (LINEAR, EXP, MAXI_BOOST_DSP tables)
  ptutil/Qnt/Qnt2But.cpp      qnt2ndOrderButterworthInit, qntIToRSimpleLowpassInit
  ptutil/Qnt/Qntitor2.cpp     qntIToRTimeConstantBeta
  ptutil/Qnt/QntitoBoostCut.cpp qntIToBoostCutInit
  ptutil/Filt/Fil12But.cpp    filtDesign2ndButHighPass, filtDesignSimple1rstLowPass
  ptutil/dfxp/dfxpComm.cpp    dfxp_Communicate* (slider -> DSP parameter mapping)
  DfxDspPrivate.cpp           setEffectValue (slider 0..10 -> knob 0..1 -> MIDI)

The engine computes in 32-bit float (realtype == float). Every operation that is
float in C is rounded with Math.fround (f) so tables match the engine bit-for-bit.
*/
(function (root) {
  "use strict";
  var ns = root.FxDsp = root.FxDsp || {};
  var f = Math.fround;

  var TWO_PI = 6.283185307;            // pt_defs.h
  var MIDI_MAX = 127;

  // ---- qnt tables (Qntitor.cpp) -------------------------------------------------
  function linearTable(min, max) {
    min = f(min); max = f(max);
    var t = new Float32Array(MIDI_MAX + 1);
    var scale = f((max - min) / MIDI_MAX);
    for (var i = 0; i <= MIDI_MAX; i++) t[i] = f(min + f(scale * i));
    t[MIDI_MAX] = max;                 // "Hard set endpoint"
    return t;
  }

  function expTable(min, max) {
    min = f(min); max = f(max);
    var t = new Float32Array(MIDI_MAX + 1);
    var factor = f(Math.pow(f(max / min), 1.0 / MIDI_MAX));
    t[0] = min; t[MIDI_MAX] = max;
    for (var i = 1; i < MIDI_MAX; i++) t[i] = f(t[i - 1] * factor);
    return t;
  }

  // QNT_RESPONSE_MAXI_BOOST_DSP: piecewise dB grid built with float loops, then dB -> linear.
  function maxiBoostDspTable() {
    var t = new Float32Array(MIDI_MAX + 1);
    t[0] = 0; t[MIDI_MAX] = 30;
    var index = 0, v, tmp;
    var s1 = f(0.1), s2 = f(0.2), s3 = f(0.5);
    for (v = 0; (tmp = f(v * s1)) < 6; v++) t[index++] = tmp;
    for (v = 0; (tmp = f(f(v * s2) + 6)) < 12; v++) t[index++] = tmp;
    for (v = 0; (tmp = f(f(v * s3) + 12)) < 30; v++) t[index++] = tmp;
    t[126] = t[127];
    for (var i = 0; i <= MIDI_MAX; i++) t[i] = f(Math.pow(10.0, t[i] / 20.0));
    return t;
  }

  // ---- filter designs (Fil12But.cpp) --------------------------------------------
  function butterworthHighPass(omega) {
    var omega2 = f(omega * omega);
    var twoRoot2omega = f(f(2.0 * Math.sqrt(2.0)) * omega);
    var tmp = f(1 / f(f(4 + omega2) + twoRoot2omega));
    return {
      gain: f(4 * tmp),
      a1: f(f(8 - f(2 * omega2)) * tmp),             // stored already negated
      a0: f(f(f(twoRoot2omega - 4) - omega2) * tmp)  // stored already negated
    };
  }

  function simpleLowpassCoeff(omega) {
    var cos = f(Math.cos(omega));
    var root = f(Math.sqrt(f(f(f(cos * cos) - f(4 * cos)) + 3)));
    return f(f(2 - cos) - root);
  }

  // ---- per-sample-rate fixed parameters (dfxp_InitDynamicQnts_* / CommunicateFixed) --
  var fixedCache = {};
  function fixedParams(fs) {
    if (fixedCache[fs]) return fixedCache[fs];
    fs = f(fs);
    var period = f(1 / fs);
    var midi = { auralTune: 53, roomSize: 64, rolloff: 89, damping: 81, releaseBeta: 85 }; // c_play.h

    // Aural high-pass tune (MIDI 53 on a 500 Hz .. 10 kHz exponential omega grid)
    var omegaMin = f(f(f(TWO_PI) * f(500)) * period);
    var omegaMax = f(f(f(TWO_PI) * f(10000)) * period);
    var factor = f(Math.pow(f(omegaMax / omegaMin), 1.0 / MIDI_MAX));
    var omega = omegaMin;
    for (var i = 1; i <= midi.auralTune; i++) omega = f(omega * factor);
    var hp = butterworthHighPass(omega);

    // Lex rolloff/damping: 1..20 kHz exponential knob, converted to one-pole lowpass coeffs
    var knob = expTable(1.0, 20.0);
    function lowpassAt(m) {
      var freq = f(knob[m] * f(1.0e3));
      return simpleLowpassCoeff(f(f(f(TWO_PI) * freq) * period));
    }
    var bandwidth = lowpassAt(midi.rolloff);
    var damping = lowpassAt(midi.damping);

    // Maximizer release: 0.1 .. 100 ms exponential time constant -> exponential beta
    var tc = expTable(0.1, 100.0)[midi.releaseBeta];
    var beta = f(Math.exp(-f(1 / f(f(tc * f(0.001)) * fs))));

    var params = {
      fs: fs,
      aural: { gain: hp.gain, a1: hp.a1, a0: hp.a0, odd: f(1.5), even: 0,
               wet: f(0.377953), dry: f(0.622047) },               // Play32.c init
      lex: { roomsize: linearTable(0.5, 1.5)[midi.roomSize],
             bandwidth: bandwidth, oneMinusBandwidth: f(1 - bandwidth),
             damping: damping, oneMinusDamping: f(1 - damping),
             lat1: f(0.75), lat3: f(0.625), lat5: f(0.70), preDelay: 1 }, // Lex32.c init
      maxi: { releaseBeta: beta, targetLevel: f(0.32), maxOutput: f(0.966051),
              maxDelay: Math.trunc(f(fs * f(0.00075))) }
    };
    fixedCache[fs] = params;
    return params;
  }

  // ---- slider-dependent tables (dfxp_InitStaticQnts) -----------------------------
  var DRIVE_MAX = f(f(TWO_PI / 4.0 * 1.8 * 2.0 * 0.75) * f(0.8));   // c_aural.h * PLY_FIDELITY_INTENSITY_MAX_SCALE
  var tables = {
    fidelity: linearTable(0, DRIVE_MAX),
    ambience: expTable(0.095, 0.95),                // PLY_DECAY_MIN/MAX
    surround: linearTable(0, f(1.0 * f(0.7))),      // DSP_WID_INTENSITY_MAX * PLY_WIDENER_BOOST_MAX_SCALE
    dynamicBoost: maxiBoostDspTable()
  };
  var bassCache = {};
  function bassTable(fs) {
    if (bassCache[fs]) return bassCache[fs];
    var factor = f(f(15) / f(127));               // DSP_PLY_BASSBOOST 0..15 dB, 90 Hz, Q 2.5
    var t = [];
    for (var i = 0; i <= MIDI_MAX; i++) {
      var boost = f(0 + f(i * factor));
      t.push(ns.sos.calcParametric(f(fs), f(90), boost, f(2.5)));
    }
    bassCache[fs] = t;
    return t;
  }

  // DfxDspPrivate::setEffectValue -> dfxpSetKnobValue(value/10) -> qntRToICalc
  function sliderToMidi(slider) {
    var knob = f(f(slider) / f(10));
    return Math.trunc(f(f(f(knob - 0) * f(127)) + f(0.5)));
  }

  /**
   * Derives every DSP parameter the Play chain uses from the five FxSound sliders
   * (0..10), exactly as dfxpComm.cpp does in the default MUSIC2 music mode.
   */
  function effectParams(input, fs) {
    var sliders = {};
    ["fidelity", "ambience", "surround", "dynamicBoost", "bass"].forEach(function (k) {
      sliders[k] = (input && typeof input[k] === "number") ? input[k] : 0;
    });
    var fixed = fixedParams(fs);
    var m = {
      fidelity: sliderToMidi(sliders.fidelity),
      ambience: sliderToMidi(sliders.ambience),
      surround: sliderToMidi(sliders.surround),
      dynamicBoost: sliderToMidi(sliders.dynamicBoost),
      bass: sliderToMidi(sliders.bass)
    };

    // Fidelity (dfxp_CommunicateFidelity): music mode only changes it in SPEECH mode.
    var drive = tables.fidelity[m.fidelity];

    // Ambience (dfxp_CommunicateAmbience + dfxp_CommAmbienceBypass), MUSIC2 warp x0.34
    var pc = Math.trunc(f(m.ambience) * 0.34);
    var roomsize = fixed.lex.roomsize;
    var decay = f(Math.pow(tables.ambience[pc], roomsize));
    var lat6 = f(decay + f(0.15));
    if (lat6 < f(0.25)) lat6 = f(0.25);
    if (lat6 > f(0.5)) lat6 = f(0.5);
    var wet, dry;
    if (pc > 40) {
      wet = f(0.21 * 1.3);
      dry = f(0.69 * 1.3);
    } else {
      wet = f((pc - 12) * (1.0 / (40 - 12)) * (0.21 * 1.3));
      dry = f(f(0.897) + f(f((40 - pc) * (1.0 / (40 - 12))) * f(1.0 - 0.897)));
    }
    var ambienceOn = sliders.ambience !== 0 && m.ambience > 12;  // bypass uses the raw MIDI value

    // Dynamic Boost (dfxp_CommunicateDynamicBoost), MUSIC2 warp x1.8, then x0.7
    var pcBoost = Math.trunc(f(f(1.8) * m.dynamicBoost));
    if (pcBoost > MIDI_MAX) pcBoost = MIDI_MAX;
    if (sliders.dynamicBoost === 0) pcBoost = 0;                  // button off -> MIDI 0
    pcBoost = Math.trunc(f(f(pcBoost) * f(0.7)));
    var gainBoost = tables.dynamicBoost[pcBoost];

    var bass = bassTable(fixed.fs)[m.bass];

    return {
      midi: m,
      aural: { on: sliders.fidelity !== 0, drive: drive, odd: fixed.aural.odd, even: fixed.aural.even,
               gain: fixed.aural.gain, a1: fixed.aural.a1, a0: fixed.aural.a0,
               wet: fixed.aural.wet, dry: fixed.aural.dry },
      lex: { on: ambienceOn, warpedMidi: pc, decay: decay, lat6: lat6, wet: wet, dry: dry,
             roomsize: roomsize, bandwidth: fixed.lex.bandwidth,
             oneMinusBandwidth: fixed.lex.oneMinusBandwidth, damping: fixed.lex.damping,
             oneMinusDamping: fixed.lex.oneMinusDamping, lat1: fixed.lex.lat1,
             lat3: fixed.lex.lat3, lat5: fixed.lex.lat5, preDelay: fixed.lex.preDelay },
      wide: { on: sliders.surround !== 0, intensity: tables.surround[m.surround] },
      bass: { on: sliders.bass !== 0, b0: bass.b0, b1: bass.b1, b2: bass.b2, a2: bass.a2,
              boostDb: bass.boost },
      maxi: { gainBoost: gainBoost, gainBoostDb: 20 * Math.log10(gainBoost),
              releaseBeta: fixed.maxi.releaseBeta, targetLevel: fixed.maxi.targetLevel,
              maxOutput: fixed.maxi.maxOutput, maxDelay: fixed.maxi.maxDelay }
    };
  }

  ns.controls = {
    f: f,
    linearTable: linearTable,
    expTable: expTable,
    maxiBoostDspTable: maxiBoostDspTable,
    butterworthHighPass: butterworthHighPass,
    simpleLowpassCoeff: simpleLowpassCoeff,
    fixedParams: fixedParams,
    sliderToMidi: sliderToMidi,
    effectParams: effectParams,
    tables: tables
  };
})(typeof globalThis !== "undefined" ? globalThis : window);
