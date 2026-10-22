/*
FxSound DSP tutorial - JavaScript port of the FxSound graphic EQ stage.
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

Ports (from https://github.com/fxsound2/fxsound-app, dsp/):
  ptutil/Filt/FiltCalcBiqd.cpp           filtBW2ANGLE, filtCalcParametric
  ptutil/DspUtil/GraphicEq/GraphicEqSet.cpp  band layout, Q, master gain, balance,
                                         normalization, volume leveling setters
  ptutil/SOS/SosProcess.cpp              sosProcessBuffer (stereo), normalization,
                                         applyVolumeLeveling and helpers
In FxSound all of these run inside ONE stage (GraphicEqProcess), before the effects.
*/
(function (root) {
  "use strict";
  var ns = root.FxDsp = root.FxDsp || {};
  var f = Math.fround;

  var PI = 3.141592653589793238462643;   // FiltCalcBiqd.cpp
  var SPN = 1.65436e-24;
  var SOS_FLOAT_BIAS = f(1.0e-30);       // u_sos.h

  // ---- Filter design (FiltCalcBiqd.cpp), all in float like biqdRealtype ----------
  function bw2angle(a, bandwidth) {
    var twoPi = f(2 * PI);
    var T = f(Math.tan(f(twoPi * bandwidth)));
    var a2 = f(a * a), a4 = f(a2 * a2);
    var d = f(f(2 * a2) * T);
    var sn = f(f(1 + a4) * T);
    var cs = f(1 - a4);
    var mag = f(Math.sqrt(f(f(sn * sn) + f(cs * cs))));
    d = f(d / mag);
    var delta = f(Math.atan2(sn, cs));
    var asnd = f(Math.asin(d));
    var theta = f(f(0.5) * f(f(f(PI) - asnd) - delta));
    var tmp = f(f(0.5) * f(asnd - delta));
    if (tmp > 0 && tmp < theta) theta = tmp;
    if (bandwidth >= 0.5) theta = f(0.005);
    return f(theta / twoPi);
  }

  /** filtCalcParametric: peaking boost/cut biquad, numerator and denominator share b1 (a1 = b1). */
  function calcParametric(sampFreq, centerFreq, boost, Q) {
    if (boost === 0) return { on: false, boost: 0, b0: 1, b1: 0, b2: 0, a1: 0, a2: 0, Q: Q };
    var maxQ;
    if (centerFreq < 60) {                                   // limit low-frequency Q
      maxQ = f(f(f(centerFreq - 20) * f(f(19) / f(40))) + 1);
      if (Q > maxQ) Q = maxQ;
    }
    var absBoost = f(Math.abs(boost));
    if (absBoost < 6) {                                      // limit low-boost Q
      maxQ = f(f(absBoost * f(f(f(20) - f(0.2)) / f(6))) + f(0.2));
      if (Q > maxQ) Q = maxQ;
    }
    var center = f(centerFreq / sampFreq);
    var bandwidth = f(center / Q);
    var a = f(Math.tan(f(f(PI) * f(center - f(0.25)))));
    var asq = f(a * a);
    var A = f(Math.pow(10.0, f(boost / f(20))));
    var F;
    if (boost < 6 && boost > -6) F = f(Math.sqrt(A));
    else if (A > 1) F = f(A / f(Math.sqrt(2.0)));
    else F = f(A * f(Math.sqrt(2.0)));
    var xfmbw = bw2angle(a, bandwidth);
    var C = f(1 / f(Math.tan(f(f(2 * PI) * xfmbw))));
    var F2 = f(F * F);
    var tmp = f(f(A * A) - F2);
    var alphad = (Math.abs(tmp) <= SPN) ? C : f(Math.sqrt(f(f(f(C * C) * f(F2 - 1)) / tmp)));
    var alphan = f(A * alphad);
    var a2plus1 = f(1 + asq), ma2plus1 = f(1 - asq);
    var b0 = f(a2plus1 + f(alphan * ma2plus1));
    var b1 = f(4 * a);
    var b2 = f(a2plus1 - f(alphan * ma2plus1));
    var a0 = f(a2plus1 + f(alphad * ma2plus1));
    var a2 = f(a2plus1 - f(alphad * ma2plus1));
    var recip = f(1 / a0);
    b0 = f(b0 * recip); b1 = f(b1 * recip); b2 = f(b2 * recip); a2 = f(a2 * recip);
    return { on: true, boost: boost, b0: b0, b1: b1, b2: b2, a1: b1, a2: a2, Q: Q };
  }

  // ---- Band layout (GraphicEqReSetAllBandFreqs, num_bands == 10) -----------------
  var BANDS_10 = [62.5, 115.734, 214.311, 396.85, 734.867, 1360.79, 2519.84, 4666.12, 8640.48, 16000.0].map(f);

  function bandQ(qMultiplier) {
    var r = Math.pow(16000 / 62.5, 1.0 / (BANDS_10.length - 1));
    var Q = f(Math.sqrt(r) / (r - 1.0));
    Q = f(Q * f(qMultiplier));
    if (Q < 1) Q = f(1);
    return Q;
  }

  // ---- Volume Leveling constants (SosProcess.cpp, namespace {} at top) ------------
  var K = {
    Ceiling: f(1.0), AttackAlpha: f(0.10), ReleaseAlphaFast: f(0.05), ReleaseAlphaSlow: f(0.02),
    ReleaseGapThreshold: f(0.15), PredictionStrength: f(0.35), PredictionClamp: f(0.15),
    PredictionMissRatio: f(0.35), SidechainHpfHz: f(120.0), ToneLowHz: f(180.0),
    ToneBodyHz: f(1200.0), TonePresenceHz: f(4500.0), MinRatioPerBuffer: f(0.8912509381337456),
    TonalityDbRange: f(7.0), TonalitySmoothing: f(0.08), MuffledTargetBoost: f(0.12),
    ClearTargetReduction: f(0.18), ClearCeilingReduction: f(0.08), HeadroomTimeSeconds: f(60.0),
    HeadroomTargetBoost: f(0.08), HeadroomTargetReduction: f(0.14), HeadroomComfortThreshold: f(0.18),
    HeadroomNearCeilingThreshold: f(0.985), HeadroomHitThreshold: f(0.002),
    VeryQuietRmsThreshold: f(0.035), QuietAudiblePeakThreshold: f(0.0035), QuietFullBoostPeak: f(0.02),
    QuietMaxGain: f(10.0), QuietReleaseAlpha: f(0.18), QuietActivationSeconds: f(10.0),
    QuietActivationRampSeconds: f(2.0), QuietFloorReleaseRmsThreshold: f(0.06),
    QuietFloorReleaseAlpha: f(0.02), QuietFloorSilenceDecayAlpha: f(0.08),
    QuietPeakBucketSeconds: f(1.0), QuietPeakTargetRatio: f(0.98), QuietPeakFloorRaiseTimeSeconds: f(6.0),
    Pi: f(3.14159265358979323846), HistorySize: 6, PeakWindowSize: 30
  };
  function clampReal(v, lo, hi) { return Math.max(lo, Math.min(v, hi)); }
  function onePoleAlpha(cutoff, fs) {
    var dt = f(1 / fs), rc = f(1 / f(f(f(2) * K.Pi) * cutoff));
    return f(dt / f(rc + dt));
  }

  /**
   * One FxSound graphic-EQ stage (sosHdlType + GraphicEqHdlType state) for stereo audio.
   * settings: { gains[10] dB, qMultiplier, masterGainDb, balanceDb, normalizationDb, volumeLeveling }
   */
  function EqStage(fs, settings) {
    this.fs = f(fs);
    var Q = bandQ(settings.qMultiplier);
    this.Q = Q;
    this.sections = [];
    for (var i = 0; i < BANDS_10.length; i++) {
      var g = settings.gains[i] || 0;
      if (g > 20) g = 20; if (g < -20) g = -20;            // GRAPHIC_EQ_DEFAULT_MAX_BOOST_OR_CUT
      var c = (g === 0 || f(BANDS_10[i] * 2) >= this.fs)
        ? { on: false, b0: 1, b1: 0, b2: 0, a2: 0 }
        : calcParametric(this.fs, BANDS_10[i], f(g), Q);
      c.s1 = 0; c.s2 = 0; c.s3 = 0; c.s4 = 0;
      this.sections.push(c);
    }
    this.masterGain = f(Math.pow(10.0, f(f(settings.masterGainDb || 0) / f(20))));
    var bal = f(settings.balanceDb || 0);
    this.balanceLeft = f(1); this.balanceRight = f(1);
    if (bal > 0) this.balanceLeft = f(Math.pow(10.0, f(f(-bal) / f(20))));
    else if (bal < 0) this.balanceRight = f(Math.pow(10.0, f(bal / f(20))));
    this.targetRms = f(Math.pow(10.0, f(f(settings.normalizationDb || 0) / f(20))));
    this.normalizationGain = f(1);
    var vl = f(Math.max(0, Math.min(settings.volumeLeveling || 0, 4)));
    this.vlTargetRms = f(f(vl / f(4)) * f(0.5));            // kVolumeLevelingMax{ControlValue,TargetRms}
    this.vl = {
      gain: f(1), powerHistory: new Float32Array(K.HistorySize), powerSum: 0, powerIndex: 0,
      powerCount: 0, prevAvg: 0, prevPred: 0, alphaFs: 0, scAlpha: 0, lowA: 0, bodyA: 0, presA: 0,
      scPrevIn: [0, 0], scPrevOut: [0, 0], tone: [[0, 0, 0], [0, 0, 0]], tonality: 0, headroom: 0,
      quietDuration: 0, quietFloor: f(1), peakHistory: new Float32Array(K.PeakWindowSize),
      peakIndex: 0, peakCount: 0, bucketMax: 0, bucketSeconds: 0
    };
    this.trace = [];   // per-buffer {gain} for the tutorial's gain-over-time view
  }

  /** Power off but EQ on: GraphicEqProcess_MasterGainOnly (no balance/normalization/leveling). */
  EqStage.prototype.processMasterGainOnly = function (buf) {
    for (var i = 0; i < buf.length; i++) buf[i] = f(buf[i] * this.masterGain);
  };

  /** sosProcessBuffer (stereo), in place on an interleaved L/R Float32Array of n frames. */
  EqStage.prototype.process = function (buf, n, opts) {
    opts = opts || {};
    var eqOn = opts.eq !== false, gainOn = opts.gain !== false;
    var mg = gainOn ? this.masterGain : f(1);
    var bl = gainOn ? this.balanceLeft : f(1), br = gainOn ? this.balanceRight : f(1);
    var secs = this.sections, sumSquares = f(0), k = 0;
    for (var j = 0; j < n; j++, k += 2) {
      var in1 = buf[k], in2 = buf[k + 1], out1 = in1, out2 = in2;
      if (eqOn) {
        for (var i = 0; i < secs.length; i++) {
          var s = secs[i];
          if (!s.on) continue;
          out1 = f(f(s.s1 + f(s.b0 * in1)) + SOS_FLOAT_BIAS);
          s.s1 = f(f(f(in1 - out1) * s.b1) + s.s2);
          s.s2 = f(f(s.b2 * in1) - f(s.a2 * out1));
          in1 = out1;
          out2 = f(f(s.s3 + f(s.b0 * in2)) + SOS_FLOAT_BIAS);
          s.s3 = f(f(f(in2 - out2) * s.b1) + s.s4);
          s.s4 = f(f(s.b2 * in2) - f(s.a2 * out2));
          in2 = out2;
        }
      }
      buf[k] = f(f(out1 * mg) * bl);
      buf[k + 1] = f(f(out2 * mg) * br);
      if (this.targetRms !== f(1)) sumSquares = f(sumSquares + f(f(buf[k] * buf[k]) + f(buf[k + 1] * buf[k + 1])));
    }
    if (opts.normalization !== false && this.targetRms !== f(1)) this.normalize(buf, n, sumSquares);
    if (opts.leveling !== false) this.applyVolumeLeveling(buf, n, 2, this.fs, -1);
    this.trace.push(this.vl.gain);
  };

  // "NORMALIZATION NEW" block of sosProcessBuffer
  EqStage.prototype.normalize = function (buf, n, sumSquares) {
    var currentRms = f(Math.sqrt(f(sumSquares / f(n * 2))));
    if (currentRms < f(1e-6)) currentRms = f(1e-6);
    var targetGain = f(this.targetRms / currentRms);
    targetGain = Math.max(f(0.01), Math.min(targetGain, f(1)));
    var diff = f(Math.abs(f(targetGain - this.normalizationGain)));
    var smoothing = targetGain > this.normalizationGain ? f(f(0.0005) + f(diff * f(0.001))) : f(1);
    smoothing = Math.min(smoothing, f(0.5));
    this.normalizationGain = f(this.normalizationGain + f(f(targetGain - this.normalizationGain) * smoothing));
    for (var i = 0; i < n * 2; i++) buf[i] = f(buf[i] * this.normalizationGain);
  };

  EqStage.prototype.updateQuietPeakWindow = function (postPeak, dur) {
    var v = this.vl;
    v.bucketMax = Math.max(v.bucketMax, postPeak);
    v.bucketSeconds = f(v.bucketSeconds + dur);
    while (v.bucketSeconds >= K.QuietPeakBucketSeconds) {
      v.peakHistory[v.peakIndex] = v.bucketMax;
      v.peakIndex = (v.peakIndex + 1) % K.PeakWindowSize;
      if (v.peakCount < K.PeakWindowSize) v.peakCount++;
      v.bucketSeconds = f(v.bucketSeconds - K.QuietPeakBucketSeconds);
      v.bucketMax = 0;
    }
  };

  EqStage.prototype.quietPeakWindowMax = function () {
    var v = this.vl, m = v.bucketMax;
    for (var i = 0; i < v.peakCount; i++) m = Math.max(m, v.peakHistory[i]);
    return m;
  };

  /** applyVolumeLeveling (SosProcess.cpp), line-for-line, float arithmetic. */
  EqStage.prototype.applyVolumeLeveling = function (buf, n, ch, sampFreq, excluded) {
    var v = this.vl;
    if (this.vlTargetRms <= 0 || n <= 0 || ch <= 0) { v.gain = f(1); return; }
    var analyzed = excluded >= 0 ? ch - 1 : ch;
    if (analyzed <= 0) return;

    var sumSquares = f(0), peak = f(0), low = f(0), body = f(0), pres = f(0), air = f(0), idx = 0;
    var fs = sampFreq > 1000 ? sampFreq : f(48000);
    if (v.alphaFs !== fs) {                                   // updateVolumeLevelingCoefficients
      var dt = f(1 / fs), rc = f(1 / f(f(f(2) * K.Pi) * K.SidechainHpfHz));
      v.scAlpha = f(rc / f(rc + dt));
      v.lowA = onePoleAlpha(K.ToneLowHz, fs);
      v.bodyA = onePoleAlpha(K.ToneBodyHz, fs);
      v.presA = onePoleAlpha(K.TonePresenceHz, fs);
      v.alphaFs = fs;
    }
    for (var s = 0; s < n; s++, idx += ch) {
      for (var c = 0; c < ch; c++) {
        if (c === excluded) continue;
        var value = buf[idx + c];
        var sc = f(v.scAlpha * f(f(v.scPrevOut[c] + value) - v.scPrevIn[c]));
        v.scPrevIn[c] = value; v.scPrevOut[c] = sc;
        sumSquares = f(sumSquares + f(sc * sc));
        var av = f(Math.abs(sc));
        if (av > peak) peak = av;
        var t = v.tone[c];
        t[0] = f(t[0] + f(v.lowA * f(value - t[0])));
        t[1] = f(t[1] + f(v.bodyA * f(value - t[1])));
        t[2] = f(t[2] + f(v.presA * f(value - t[2])));
        var lb = t[0], bb = f(t[1] - t[0]), pb = f(t[2] - t[1]), ab = f(value - t[2]);
        low = f(low + f(lb * lb)); body = f(body + f(bb * bb));
        pres = f(pres + f(pb * pb)); air = f(air + f(ab * ab));
      }
    }

    var currentPower = f(sumSquares / f(n * analyzed));
    var currentRms = f(Math.sqrt(currentPower));
    var gainStart = v.gain, gainEnd = gainStart;
    var hReduce = Math.max(v.headroom, 0), hBoost = Math.max(-v.headroom, 0);
    var floor = Math.max(v.quietFloor, f(1));
    var quietBefore = v.quietDuration;
    var num = f(f(pres + f(air * f(0.75))) + f(1e-12));
    var den = f(f(f(body * f(1.15)) + f(low * f(0.85))) + f(1e-12));
    var tonalityDb = f(10.0 * Math.log10(f(num / den)));
    var targetTonality = clampReal(f(tonalityDb / K.TonalityDbRange), f(-1), f(1));
    v.tonality = f(f(v.tonality * f(1 - K.TonalitySmoothing)) + f(targetTonality * K.TonalitySmoothing));

    var clear = Math.max(v.tonality, 0), muffled = Math.max(-v.tonality, 0);
    var dur = f(n / fs);
    var upAuth = clampReal(f(f(f(1) - f(hReduce * f(1.5))) + f(hBoost * f(0.25))), f(0), f(1));
    var downAuth = clampReal(f(f(0.35) + f(upAuth * f(0.65))), f(0.35), f(1));
    var gMuffled = f(muffled * upAuth), gClear = f(clear * downAuth);
    var effTarget = f(this.vlTargetRms * f(f(f(f(f(1) + f(gMuffled * K.MuffledTargetBoost))
      - f(gClear * K.ClearTargetReduction)) + f(hBoost * K.HeadroomTargetBoost)) - f(hReduce * K.HeadroomTargetReduction)));
    var ceiling = clampReal(f(K.Ceiling * f(f(1) - f(gClear * K.ClearCeilingReduction))), f(0.92), K.Ceiling);
    var maxGainCap = Math.max(f(effTarget / f(0.125)), floor);

    if (v.powerCount === K.HistorySize) v.powerSum = f(v.powerSum - v.powerHistory[v.powerIndex]);
    else v.powerCount++;
    v.powerHistory[v.powerIndex] = currentPower;
    v.powerSum = f(v.powerSum + currentPower);
    v.powerIndex = (v.powerIndex + 1) % K.HistorySize;

    var averaged = currentRms;
    if (v.powerCount > 0) averaged = f(Math.sqrt(f(v.powerSum / v.powerCount)));

    var predicted = averaged, missed = false;
    if (v.prevAvg > f(1e-6)) {
      var pd = f(v.prevPred - v.prevAvg), ad = f(averaged - v.prevAvg);
      var missThreshold = Math.max(f(v.prevAvg * f(0.01)), f(1e-5));
      if (Math.abs(pd) > missThreshold) {
        var wrong = f(pd * ad) < 0;
        var overshot = Math.abs(ad) < f(Math.abs(pd) * K.PredictionMissRatio);
        if (wrong || overshot) missed = true;
      }
    }
    if (v.prevAvg > f(1e-6)) {
      var gradient = f(averaged - v.prevAvg);
      predicted = f(predicted + f(gradient * K.PredictionStrength));
      predicted = clampReal(predicted, f(averaged * f(1 - K.PredictionClamp)), f(averaged * f(1 + K.PredictionClamp)));
    }
    if (missed) predicted = averaged;
    predicted = Math.max(predicted, f(1e-6));
    v.prevAvg = averaged; v.prevPred = predicted;

    if (currentRms > f(1e-6)) {
      var qActivation = clampReal(f(f(v.quietDuration - K.QuietActivationSeconds) / K.QuietActivationRampSeconds), f(0), f(1));
      var qRms = clampReal(f(f(K.VeryQuietRmsThreshold - predicted) / K.VeryQuietRmsThreshold), f(0), f(1));
      var qPeak = clampReal(f(f(peak - K.QuietAudiblePeakThreshold) / f(K.QuietFullBoostPeak - K.QuietAudiblePeakThreshold)), f(0), f(1));
      var qBoost = f(f(qRms * qPeak) * qActivation);
      if (qBoost > 0) {
        var qCap = Math.max(maxGainCap, K.QuietMaxGain);
        maxGainCap = f(maxGainCap + f(f(qCap - maxGainCap) * qBoost));
      }
      var desired = f(effTarget / predicted);
      desired = Math.min(desired, maxGainCap);
      desired = Math.max(desired, floor);
      var alpha = K.AttackAlpha;
      if (desired >= v.gain) {
        var gap = f(desired - v.gain);
        var ratio = f(gap / Math.max(v.gain, f(1e-6)));
        alpha = ratio > K.ReleaseGapThreshold ? K.ReleaseAlphaSlow : K.ReleaseAlphaFast;
        alpha = f(alpha * clampReal(f(f(f(1) + f(gMuffled * f(0.20))) - f(gClear * f(0.35))), f(0.55), f(1.20)));
        alpha = Math.max(alpha, f(K.QuietReleaseAlpha * qBoost));
      }
      gainEnd = f(f(v.gain * f(f(1) - alpha)) + f(desired * alpha));
    }

    if (gainEnd < gainStart) {                       // avoid an abrupt "crushed" sound
      var minEnd = f(gainStart * K.MinRatioPerBuffer);
      if (gainEnd < minEnd) gainEnd = minEnd;
    }
    if (peak > f(1e-6)) {                            // never let a peak exceed the ceiling
      var safe = f(ceiling / peak);
      if (gainEnd > safe) gainEnd = safe;
      if (gainStart > safe) gainStart = safe;
    }
    gainStart = clampReal(gainStart, f(0), maxGainCap);
    gainEnd = clampReal(gainEnd, f(0), maxGainCap);
    v.gain = gainEnd;

    idx = 0;
    var postSum = f(0), postPeak = f(0), hits = 0;
    for (var sm = 0; sm < n; sm++, idx += ch) {
      var tt = f(sm / n);
      var g = f(gainStart + f(tt * f(gainEnd - gainStart)));   // sample-accurate gain ramp
      for (var cc = 0; cc < ch; cc++) {
        if (cc === excluded) continue;
        var o = f(buf[idx + cc] * g);
        var oa = f(Math.abs(o));
        postSum = f(postSum + f(o * o));
        if (oa > postPeak) postPeak = oa;
        if (oa >= f(ceiling * K.HeadroomNearCeilingThreshold)) hits++;
        if (o > ceiling) o = ceiling; else if (o < -ceiling) o = -ceiling;
        buf[idx + cc] = o;
      }
    }

    var postRms = f(Math.sqrt(f(postSum / f(n * analyzed))));
    this.updateQuietPeakWindow(postPeak, dur);
    var rolling = this.quietPeakWindowMax();
    if (peak > K.QuietAudiblePeakThreshold && postRms < K.VeryQuietRmsThreshold) v.quietDuration = f(v.quietDuration + dur);
    else v.quietDuration = 0;

    if (quietBefore >= K.QuietActivationSeconds && gainEnd > f(effTarget / f(0.125)) && gainEnd > floor) floor = gainEnd;
    var windowReady = v.peakCount === K.PeakWindowSize;
    var floorActive = quietBefore >= K.QuietActivationSeconds || floor > 1;
    var peakTarget = f(ceiling * K.QuietPeakTargetRatio);
    var headroomAvail = rolling > K.QuietAudiblePeakThreshold && rolling < peakTarget && hReduce < f(0.25);
    if (windowReady && floorActive && headroomAvail) {
      var desiredFloor = f(floor * f(peakTarget / Math.max(rolling, f(1e-6))));
      desiredFloor = clampReal(desiredFloor, floor, K.QuietMaxGain);
      var raise = clampReal(f(dur / K.QuietPeakFloorRaiseTimeSeconds), f(0.0005), f(0.05));
      floor = f(f(floor * f(f(1) - raise)) + f(desiredFloor * raise));
    }
    if (peak <= K.QuietAudiblePeakThreshold) floor = f(floor + f(f(f(1) - floor) * K.QuietFloorSilenceDecayAlpha));
    else if (postRms > K.QuietFloorReleaseRmsThreshold) floor = f(floor + f(f(f(1) - floor) * K.QuietFloorReleaseAlpha));
    if (floor < f(1.0001)) floor = f(1);
    v.quietFloor = floor;

    var hitRatio = f(hits / f(n * analyzed));
    var headroomRatio = clampReal(f(f(ceiling - postPeak) / ceiling), f(0), f(1));
    var targetHs = 0;
    if (hitRatio > K.HeadroomHitThreshold) targetHs = clampReal(f(hitRatio / f(0.02)), f(0), f(1));
    else if (postPeak >= f(ceiling * K.HeadroomNearCeilingThreshold))
      targetHs = clampReal(f(f(f(postPeak / ceiling) - K.HeadroomNearCeilingThreshold) / f(f(1) - K.HeadroomNearCeilingThreshold)), f(0), f(1));
    else if (headroomRatio > K.HeadroomComfortThreshold)
      targetHs = -clampReal(f(f(headroomRatio - K.HeadroomComfortThreshold) / f(0.25)), f(0), f(1));
    var hAlpha = clampReal(f(dur / K.HeadroomTimeSeconds), f(0.0005), f(0.05));
    v.headroom = f(f(v.headroom * f(f(1) - hAlpha)) + f(targetHs * hAlpha));
  };

  ns.sos = {
    calcParametric: calcParametric,
    bw2angle: bw2angle,
    bandQ: bandQ,
    BANDS_10: BANDS_10,
    EqStage: EqStage
  };
})(typeof globalThis !== "undefined" ? globalThis : window);
