/*
FxSound DSP tutorial - user interface.
Copyright (C) 2025  FxSound LLC. Licensed under the GNU Affero General Public License v3 or later.

No inline scripts or handlers: everything is wired here with addEventListener, so the page works
under a strict Content-Security-Policy (script-src 'self') and inside an <iframe> (?embed=1).
*/
(function () {
  "use strict";
  var VERSION = "1.2.1";
  var STORAGE_KEY = "fxsound-dsp-tutorial:v1";
  var Pcm = window.FxPcm, Dsp = window.FxDsp, Explain = window.FxExplain;
  var FS = Pcm.SAMPLE_RATE;
  var BAND_LABELS = ["62.5", "116", "214", "397", "735", "1.36k", "2.52k", "4.67k", "8.64k", "16k"];
  var DEFAULT_MELODY = ["C4", "D4", "E4"];
  var MAX_NOTES = 16;

  // ---- State ---------------------------------------------------------------------------
  function defaultState() {
    return {
      melody: DEFAULT_MELODY.slice(), repeat15: false, dbfs: -6, view: "wave", cursor: 0.2, dbScale: false, align: true,
      settings: {
        power: true, eq: true, gains: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0], qMultiplier: 1, masterGainDb: 0,
        balanceDb: 0, normalizationDb: 0, volumeLeveling: 0,
        fidelity: 0, ambience: 0, surround: 0, dynamicBoost: 0, bass: 0
      },
      stages: { eqBands: true, gain: true, leveling: true, fidelity: true, ambience: true,
                surround: true, bass: true, dynamicBoost: true }
    };
  }
  var state = loadState();
  var audio = { dry: null, dryCodes: null, wet: null, result: null };

  function loadState() {
    var s = defaultState();
    try {
      var saved = JSON.parse(window.localStorage.getItem(STORAGE_KEY) || "null");
      if (saved && typeof saved === "object") {
        ["melody", "repeat15", "dbfs", "view", "cursor", "dbScale", "align"].forEach(function (k) { if (k in saved) s[k] = saved[k]; });
        if (saved.settings) Object.keys(s.settings).forEach(function (k) { if (k in saved.settings) s.settings[k] = saved.settings[k]; });
        if (saved.stages) Object.keys(s.stages).forEach(function (k) { if (k in saved.stages) s.stages[k] = saved.stages[k]; });
      }
    } catch (e) { /* storage unavailable: use defaults */ }
    s.settings.normalizationDb = 0;   // not a user control in FxSound
    s.settings.eq = true;             // the app never turns the equalizer stage off (no UI for DfxDsp::eqOn)
    return s;
  }
  function saveState() {
    try { window.localStorage.setItem(STORAGE_KEY, JSON.stringify(state)); } catch (e) { /* ignore */ }
  }

  // ---- Small DOM helpers -------------------------------------------------------------------
  function $(id) { return document.getElementById(id); }
  function el(tag, attrs, children) {
    var node = document.createElement(tag);
    Object.keys(attrs || {}).forEach(function (k) {
      if (k === "text") node.textContent = attrs[k];
      else if (k === "className") node.className = attrs[k];
      else node.setAttribute(k, attrs[k]);
    });
    (children || []).forEach(function (c) { node.appendChild(c); });
    return node;
  }
  function fmtDb(v) { return v === -Infinity ? "-∞ dBFS" : v.toFixed(1) + " dBFS"; }
  function fmtSigned(v, unit) { return (v > 0 ? "+" : "") + v + (unit || ""); }

  // ---- Explanations panel ------------------------------------------------------------------
  function showExplain(key) {
    var entry = Explain[key] || Explain.intro;
    var body = $("explain-body");
    body.textContent = "";
    entry.text.forEach(function (t) { body.appendChild(el("p", { text: t })); });
    $("explain-source").textContent = entry.source ? "Source: " + entry.source : "";
  }
  function wireExplain(node, key) {
    node.setAttribute("data-explain", key);
    ["focus", "pointerenter", "input"].forEach(function (ev) {
      node.addEventListener(ev, function () { showExplain(key); });
    });
  }

  // ---- Controls ------------------------------------------------------------------------------
  function rangeLine(opts) {
    var id = "ctl-" + opts.key;
    var input = el("input", { type: "range", id: id, min: opts.min, max: opts.max, step: opts.step, value: opts.value });
    var out = el("output", { "for": id });
    var line = el("div", { className: "control-line" + (opts.disabled ? " disabled" : "") },
      [el("label", { "for": id, text: opts.label }), input, out]);
    if (opts.disabled) input.disabled = true;
    function update() { out.textContent = opts.format(Number(input.value)); }
    input.addEventListener("input", function () { update(); if (opts.onChange) opts.onChange(Number(input.value)); });
    update();
    wireExplain(input, opts.explain);
    line.input = input; line.update = update;
    return line;
  }

  var effectDefs = [
    { key: "fidelity", label: "Clarity" }, { key: "ambience", label: "Ambience" },
    { key: "surround", label: "Surround Sound" }, { key: "dynamicBoost", label: "Dynamic Boost" },
    { key: "bass", label: "Bass Boost" }
  ];
  var controls = {};

  function buildControls() {
    var effects = $("effects");
    effectDefs.forEach(function (d) {
      controls[d.key] = rangeLine({
        key: d.key, label: d.label, min: 0, max: 10, step: 1, value: state.settings[d.key], explain: d.key,
        format: function (v) { return v === 0 ? "Off" : String(v); },
        onChange: function (v) { state.settings[d.key] = v; changed("process"); }
      });
      effects.appendChild(controls[d.key]);
    });

    var eq = $("eq");
    BAND_LABELS.forEach(function (label, i) {
      var id = "band-" + i;
      var input = el("input", { type: "range", id: id, min: -12, max: 12, step: 1, value: state.settings.gains[i],
                                "aria-label": label + " Hz band" });
      var out = el("output", { "for": id });
      function update() { out.textContent = fmtSigned(Number(input.value)); }
      input.addEventListener("input", function () { update(); state.settings.gains[i] = Number(input.value); changed("process"); });
      update();
      wireExplain(input, "band");
      eq.appendChild(el("div", { className: "band" }, [out, input, el("span", { text: label })]));
      controls["band" + i] = { input: input, update: update };
    });

    var settings = $("settings");
    controls.masterGainDb = rangeLine({
      key: "masterGainDb", label: "Master Gain", min: -20, max: 20, step: 2, value: state.settings.masterGainDb,
      explain: "masterGain", format: function (v) { return fmtSigned(v, " dB"); },
      onChange: function (v) { state.settings.masterGainDb = v; changed("process"); }
    });
    controls.balanceDb = rangeLine({
      key: "balanceDb", label: "Balance", min: -20, max: 20, step: 2, value: state.settings.balanceDb, explain: "balance",
      format: function (v) { return v === 0 ? "Center" : (v > 0 ? "Left -" + v + " dB" : "Right " + v + " dB"); },
      onChange: function (v) { state.settings.balanceDb = v; changed("process"); }
    });
    controls.qMultiplier = rangeLine({
      key: "qMultiplier", label: "Filter Q", min: 1, max: 3, step: 0.5, value: state.settings.qMultiplier,
      explain: "filterQ", format: function (v) { return "x" + v.toFixed(1); },
      onChange: function (v) { state.settings.qMultiplier = v; changed("process"); }
    });
    controls.volumeLeveling = rangeLine({
      key: "volumeLeveling", label: "Volume Leveling", min: 0, max: 4, step: 0.5, value: state.settings.volumeLeveling,
      explain: "volumeLeveling", format: function (v) { return v === 0 ? "Off" : v.toFixed(1); },
      onChange: function (v) { state.settings.volumeLeveling = v; changed("process"); }
    });
    // No Normalization slider: the app has none, and at its fixed default the engine skips it.
    [controls.masterGainDb, controls.balanceDb, controls.qMultiplier, controls.volumeLeveling]
      .forEach(function (c) { settings.appendChild(c); });

    var power = $("power");
    power.checked = state.settings.power;
    power.addEventListener("change", function () { state.settings.power = power.checked; changed("process"); });
    wireExplain(power, "power");

    $("btn-reset").addEventListener("click", function () {
      var keep = { melody: state.melody, repeat15: state.repeat15, dbfs: state.dbfs, view: state.view, cursor: state.cursor,
                   dbScale: state.dbScale, align: state.align };
      state = defaultState();
      Object.keys(keep).forEach(function (k) { state[k] = keep[k]; });
      syncControls(); changed("process");
    });
  }

  function syncControls() {
    effectDefs.forEach(function (d) { controls[d.key].input.value = state.settings[d.key]; controls[d.key].update(); });
    for (var i = 0; i < 10; i++) { controls["band" + i].input.value = state.settings.gains[i]; controls["band" + i].update(); }
    ["masterGainDb", "balanceDb", "qMultiplier", "volumeLeveling"].forEach(function (k) {
      controls[k].input.value = state.settings[k]; controls[k].update();
    });
    $("power").checked = state.settings.power;
    buildStages();
  }

  // ---- Stage isolation list (true FxSound order) --------------------------------------------
  var stageDefs = [
    { key: "eqBands", label: "Equalizer bands (10 peaking filters)", group: "Equalizer stage", explain: "band" },
    { key: "gain", label: "Master Gain x Balance", group: "Equalizer stage", explain: "masterGain" },
    { key: "leveling", label: "Volume Leveling", group: "Equalizer stage", explain: "volumeLeveling" },
    { key: "fidelity", label: "Clarity (harmonic exciter)", group: "Effects chain", explain: "fidelity" },
    { key: "ambience", label: "Ambience (plate reverb)", group: "Effects chain", explain: "ambience" },
    { key: "surround", label: "Surround Sound (mid/side widener)", group: "Effects chain", explain: "surround" },
    { key: "bass", label: "Bass Boost (90 Hz filter)", group: "Effects chain", explain: "bass" },
    { key: "dynamicBoost", label: "Dynamic Boost (look-ahead limiter, always on)", group: "Effects chain", explain: "dynamicBoost" }
  ];

  function stageInactiveReason(key) {
    var s = state.settings;
    if (!s.power) return "Power is off: audio bypasses FxSound";
    if (key === "eqBands" && s.gains.every(function (g) { return g === 0; })) return "All bands at 0 dB: no band filter runs";
    if (key === "leveling" && !s.volumeLeveling) return "Volume Leveling is off";
    if (["fidelity", "ambience", "surround", "bass"].indexOf(key) >= 0 && !s[key]) return "Slider at 0: stage not running";
    return "";
  }

  function buildStages() {
    var list = $("stages");
    list.textContent = "";
    stageDefs.forEach(function (d) {
      var id = "stage-" + d.key;
      var box = el("input", { type: "checkbox", id: id });
      box.checked = state.stages[d.key];
      box.addEventListener("change", function () { state.stages[d.key] = box.checked; changed("process"); });
      wireExplain(box, d.explain);
      var reason = stageInactiveReason(d.key);
      var li = el("li", { className: reason ? "inactive" : "" }, [
        box, el("label", { "for": id, text: d.label }), el("span", { className: "group", text: d.group })
      ]);
      if (reason) li.appendChild(el("span", { className: "why", text: reason }));
      list.appendChild(li);
    });
  }

  // ---- Melody ------------------------------------------------------------------------------
  function buildKeyboard() {
    var kb = $("keyboard");
    Pcm.NOTES.forEach(function (n) {
      var key = el("button", { type: "button", className: "key" + (n.name.charAt(0) === "C" ? " c-key" : ""),
                               "aria-label": n.name + ", " + n.freq.toFixed(1) + " hertz" },
        [document.createTextNode(n.name), el("small", { text: n.freq < 1000 ? n.freq.toFixed(0) : (n.freq / 1000).toFixed(2) + "k" })]);
      key.addEventListener("click", function () {
        if (state.melody.length >= MAX_NOTES) state.melody.shift();
        state.melody.push(n.name);
        key.classList.add("flash");
        setTimeout(function () { key.classList.remove("flash"); }, 150);
        renderSequenceChips(); changed("generate");
      });
      kb.appendChild(key);
    });
    var c4 = kb.querySelector('[aria-label^="C4,"]');
    if (c4) kb.scrollLeft = c4.offsetLeft - 120;
  }

  // Note-name -> frequency table (octaves as rows), built from the same NOTES list the synth uses.
  function buildNoteTable() {
    var letters = ["C", "D", "E", "F", "G", "A", "B"], table = $("note-table");
    var head = el("tr", {}, [el("th", { text: "Octave" })].concat(letters.map(function (l) { return el("th", { text: l }); })));
    var body = el("tbody");
    for (var oct = 1; oct <= 7; oct++) {
      var row = el("tr", {}, [el("th", { text: String(oct) })]);
      letters.forEach(function (l) {
        var n = Pcm.NOTES.filter(function (x) { return x.name === l + oct; })[0];
        row.appendChild(el("td", { text: n ? n.freq.toFixed(n.freq < 1000 ? 2 : 1) : "" }));
      });
      body.appendChild(row);
    }
    table.appendChild(el("thead", {}, [head])); table.appendChild(body);
    wireExplain(document.querySelector("details.notes summary"), "notes");
  }

  function renderSequenceChips() {
    var seq = $("sequence");
    seq.textContent = "";
    if (!state.melody.length) { seq.appendChild(el("span", { className: "chip empty", text: "No notes yet: click a key" })); return; }
    state.melody.forEach(function (name, i) {
      var chip = el("button", { type: "button", className: "chip", title: "Remove", text: name + " ×" });
      chip.addEventListener("click", function () { state.melody.splice(i, 1); renderSequenceChips(); changed("generate"); });
      seq.appendChild(chip);
    });
  }

  // ---- Rendering ---------------------------------------------------------------------------
  var pending = null, pendingKind = "process";
  function changed(kind) {
    if (kind === "generate") pendingKind = "generate";
    buildStages();
    saveState();
    $("status").textContent = "Rendering…";
    if (pending) clearTimeout(pending);
    pending = setTimeout(function () {
      pending = null;
      var k = pendingKind; pendingKind = "process";
      if (k === "generate") generate();
      process();
    }, 60);
  }

  function generate() {
    var notes = state.melody.map(function (name) { return Pcm.NOTES.filter(function (n) { return n.name === name; })[0]; })
      .filter(Boolean);
    var raw = notes.length ? Pcm.renderSequence(notes, state.repeat15 ? 15 : 0) : new Float32Array(2 * FS);
    Pcm.setPeakLevel(raw, state.dbfs);
    var pcm = Pcm.to24BitPcm(raw);
    audio.dry = pcm.samples; audio.dryCodes = pcm.codes;
  }

  // The app's Power switch does more than DfxDsp::powerOn(false): FxController also restores the default
  // playback device, so Windows sends audio straight to the speakers. Power off is therefore a true bypass.
  function process() {
    var t0 = performance.now();
    var frames = audio.dry.length / 2;
    if (state.settings.power) {
      audio.result = Dsp.pipeline.render(audio.dry, state.settings, state.stages);
    } else {
      audio.result = { output: new Float32Array(audio.dry), params: Dsp.controls.effectParams(state.settings, FS),
                       levelingGain: [], boostGain: [], bufferFrames: 441 };
    }
    audio.wet = audio.result.output;
    var ms = performance.now() - t0;
    $("status").textContent = state.settings.power
      ? (frames / FS).toFixed(1) + " s rendered through FxSound in " + ms.toFixed(0) + " ms"
      : "Power is off: the output is the original audio";
    updateMeters(); updateCursor(); draw();
  }

  // ---- Meters and sample inspector ------------------------------------------------------------
  function codeOf(v) { return Pcm.quantizeTo24Bit(v); }
  function updateMeters() {
    var d = Pcm.measure(audio.dry), w = Pcm.measure(audio.wet);
    $("m-dry-peak").textContent = fmtDb(d.peakDbfs);
    $("m-dry-rms").textContent = fmtDb(d.rmsDbfs);
    $("m-dry-code").textContent = codeOf(d.peak).toLocaleString();
    $("m-wet-peak").textContent = fmtDb(w.peakDbfs);
    $("m-wet-rms").textContent = fmtDb(w.rmsDbfs);
    var wc = $("m-wet-code");
    wc.textContent = codeOf(w.peak).toLocaleString() + (w.peak > 1 ? " (clips)" : "");
    wc.className = w.peak > 1 ? "clip" : "";
  }

  // Dynamic Boost's look-ahead delays FxSound's output by max_delay samples (33 at 44.1 kHz).
  // When "line up" is ticked, plots compare original sample i with processed sample i + delay.
  function wetDelay() {
    if (!state.align || !audio.result || !state.settings.power || state.stages.dynamicBoost === false) return 0;
    return audio.result.params.maxi.maxDelay;
  }

  function cursorFrame() {
    var frames = audio.dry.length / 2;
    return Math.min(frames - 1, Math.max(0, Math.round(state.cursor * (frames - 1))));
  }
  function sampleText(buf, i) {
    var v = buf[i], code = codeOf(v);
    return code.toLocaleString() + " / " + v.toFixed(6) + " / " + fmtDb(20 * Math.log10(Math.abs(v)));
  }
  function updateCursor() {
    var i = cursorFrame();
    $("cursor-out").textContent = "#" + i.toLocaleString() + " (" + (i / FS).toFixed(4) + " s)";
    $("s-dry-l").textContent = sampleText(audio.dry, 2 * i);
    $("s-dry-r").textContent = sampleText(audio.dry, 2 * i + 1);
    var w = Math.min(i + wetDelay(), audio.wet.length / 2 - 1);
    $("s-wet-l").textContent = sampleText(audio.wet, 2 * w);
    $("s-wet-r").textContent = sampleText(audio.wet, 2 * w + 1);
  }

  // ---- Plotting ---------------------------------------------------------------------------------
  var canvas = $("plot"), ctx = canvas.getContext("2d");
  function cssVar(name) { return getComputedStyle(document.documentElement).getPropertyValue(name).trim(); }

  function prepareCanvas() {
    var dpr = window.devicePixelRatio || 1;
    // The canvas's CSS height is set by the layout (the graph pane must fit the window), not by an aspect ratio.
    var w = canvas.clientWidth || 960, h = canvas.clientHeight || Math.round(w * 360 / 960);
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
      canvas.width = Math.round(w * dpr); canvas.height = Math.round(h * dpr);
    }
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, w, h);
    return { w: w, h: h };
  }

  function text(str, x, y, color, align) {
    ctx.fillStyle = color || cssVar("--muted");
    ctx.font = "11px system-ui, sans-serif";
    ctx.textAlign = align || "left";
    ctx.fillText(str, x, y);
  }

  function hline(x0, x1, y, color) { ctx.strokeStyle = color; ctx.lineWidth = 1; ctx.beginPath(); ctx.moveTo(x0, y); ctx.lineTo(x1, y); ctx.stroke(); }
  function cursorLine(x, y0, y1) {
    ctx.strokeStyle = cssVar("--cursor"); ctx.lineWidth = 1; ctx.setLineDash([4, 3]);
    ctx.beginPath(); ctx.moveTo(x, y0); ctx.lineTo(x, y1); ctx.stroke(); ctx.setLineDash([]);
  }
  // In the waveform, FxSound's output is drawn on top of the original with this opacity, so both stay visible.
  var WET_ALPHA = 0.6;

  function drawWave(box) {
    var frames = audio.dry.length / 2, pad = 40, laneH = (box.h - 34) / 2, floorDb = -90;
    [0, 1].forEach(function (ch) {
      var top = 6 + ch * (laneH + 14), mid = top + laneH / 2, amp = laneH / 2 - 2, bottom = top + laneH;
      text(ch ? "R" : "L", 4, mid + 4, cssVar("--text"));
      if (state.dbScale) {
        // Peak level per pixel column in dBFS: 0 dBFS at the top of the lane, -90 dBFS at the bottom.
        [0, -30, -60, -90].forEach(function (d) {
          var y = top + d / floorDb * laneH;
          hline(pad, box.w - 4, y, cssVar("--grid")); text(d + "", pad - 4, y + 4, null, "right");
        });
      } else {
        hline(pad, box.w - 4, mid, cssVar("--grid"));
        hline(pad, box.w - 4, mid - amp, cssVar("--grid"));
        hline(pad, box.w - 4, mid + amp, cssVar("--grid"));
        text("+1", pad - 4, mid - amp + 4, null, "right"); text("-1", pad - 4, mid + amp + 4, null, "right");
      }
      // FxSound's output (bright red) underneath, then the original, then the output again, see-through.
      // Output-only areas stay bright red, overlaps turn rose, and the original keeps its own dark colour.
      [[audio.wet, cssVar("--wet-light"), 1], [audio.dry, cssVar("--dry"), 1],
       [audio.wet, cssVar("--wet-light"), WET_ALPHA]].forEach(function (series) {
        var buf = series[0], cols = box.w - pad - 4;
        ctx.fillStyle = series[1]; ctx.globalAlpha = series[2];
        for (var x = 0; x < cols; x++) {
          var a = Math.floor(x * frames / cols), b = Math.max(a + 1, Math.floor((x + 1) * frames / cols));
          var lo = 1e9, hi = -1e9, pk = 0;
          for (var i = a; i < b; i++) { var v = buf[2 * i + ch]; if (v < lo) lo = v; if (v > hi) hi = v; if (Math.abs(v) > pk) pk = Math.abs(v); }
          if (state.dbScale) {
            var db = pk > 0 ? Math.max(floorDb, 20 * Math.log10(pk)) : floorDb;
            var y = top + Math.min(1, db / floorDb) * laneH;
            ctx.fillRect(pad + x, y, 1, Math.max(1, bottom - y));
          } else {
            lo = Math.max(-1.2, lo); hi = Math.min(1.2, hi);
            ctx.fillRect(pad + x, mid - hi * amp, 1, Math.max(1, (hi - lo) * amp));
          }
        }
        ctx.globalAlpha = 1;
      });
    });
    var cx = pad + state.cursor * (box.w - pad - 4);
    cursorLine(cx, 4, box.h - 22);
    var seconds = frames / FS;
    for (var s = 0; s <= seconds; s += seconds > 8 ? 2 : 1) text(s + " s", pad + s / seconds * (box.w - pad - 4), box.h - 6, null, "center");
  }

  function drawZoom(box) {
    var frames = audio.dry.length / 2, center = cursorFrame(), half = 110;
    var a = Math.max(0, center - half), b = Math.min(frames - 1 - wetDelay(), center + half), d = wetDelay();
    var peak = 1e-9;
    for (var i = a; i <= b; i++) peak = Math.max(peak, Math.abs(audio.dry[2 * i]), Math.abs(audio.wet[2 * (i + d)]));
    var pad = 56, mid = box.h / 2 - 8, amp = (box.h / 2 - 24) / peak;
    hline(pad, box.w - 4, mid, cssVar("--grid"));
    hline(pad, box.w - 4, mid - peak * amp, cssVar("--grid"));
    hline(pad, box.w - 4, mid + peak * amp, cssVar("--grid"));
    text("+" + peak.toFixed(4), pad - 4, mid - peak * amp + 4, null, "right");
    text("-" + peak.toFixed(4), pad - 4, mid + peak * amp + 4, null, "right");
    text("0", pad - 4, mid + 4, null, "right");
    function xOf(i) { return pad + (i - a) / Math.max(1, b - a) * (box.w - pad - 8); }
    // Original: larger opaque dots. FxSound: smaller semi-transparent dots on top.
    [[audio.dry, cssVar("--dry"), 0, 1, 3], [audio.wet, cssVar("--wet"), d, 0.65, 2.2]].forEach(function (series) {
      var buf = series[0], off = series[2];
      ctx.strokeStyle = series[1]; ctx.fillStyle = series[1]; ctx.lineWidth = 1.5; ctx.globalAlpha = series[3];
      ctx.beginPath();
      for (var j = a; j <= b; j++) { var y = mid - buf[2 * (j + off)] * amp; if (j === a) ctx.moveTo(xOf(j), y); else ctx.lineTo(xOf(j), y); }
      ctx.stroke();
      for (var k = a; k <= b; k++) { ctx.beginPath(); ctx.arc(xOf(k), mid - buf[2 * (k + off)] * amp, series[4], 0, 2 * Math.PI); ctx.fill(); }
      ctx.globalAlpha = 1;
    });
    cursorLine(xOf(center), 4, box.h - 24);
    text("Left channel, samples #" + a.toLocaleString() + " to #" + b.toLocaleString() + " (" + ((b - a) / FS * 1000).toFixed(1) + " ms)" +
      (d ? " — processed shifted back " + d + " samples to line up" : ""), pad, box.h - 6);
  }

  function fft(re, im) {
    var n = re.length, i, j, k, len;
    for (i = 1, j = 0; i < n; i++) {
      var bit = n >> 1;
      for (; j & bit; bit >>= 1) j ^= bit;
      j ^= bit;
      if (i < j) { var t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; }
    }
    for (len = 2; len <= n; len <<= 1) {
      var ang = -2 * Math.PI / len, wr = Math.cos(ang), wi = Math.sin(ang);
      for (i = 0; i < n; i += len) {
        var cr = 1, ci = 0;
        for (k = 0; k < len / 2; k++) {
          var ur = re[i + k], ui = im[i + k];
          var vr = re[i + k + len / 2] * cr - im[i + k + len / 2] * ci;
          var vi = re[i + k + len / 2] * ci + im[i + k + len / 2] * cr;
          re[i + k] = ur + vr; im[i + k] = ui + vi;
          re[i + k + len / 2] = ur - vr; im[i + k + len / 2] = ui - vi;
          var ncr = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = ncr;
        }
      }
    }
  }

  // Hann-windowed 8192-point spectrum of the L/R average. With `minus` (and its offset), analyses
  // buf - minus, i.e. only what processing changed.
  function spectrumOf(buf, start, minus, minusStart) {
    var N = 8192, re = new Float64Array(N), im = new Float64Array(N), frames = buf.length / 2, wsum = 0;
    start = Math.max(0, Math.min(start, frames - N));
    var shift = minus ? minusStart - start : 0;
    for (var i = 0; i < N; i++) {
      var w = 0.5 - 0.5 * Math.cos(2 * Math.PI * i / (N - 1)); wsum += w;
      var idx = start + i, v = idx < frames ? (buf[2 * idx] + buf[2 * idx + 1]) * 0.5 : 0;
      if (minus) { var m = idx + shift; if (m >= 0 && m < minus.length / 2) v -= (minus[2 * m] + minus[2 * m + 1]) * 0.5; }
      re[i] = v * w;
    }
    fft(re, im);
    var mags = new Float64Array(N / 2);
    for (var k = 0; k < N / 2; k++) mags[k] = 20 * Math.log10(Math.max(1e-12, 2 * Math.hypot(re[k], im[k]) / wsum));
    return mags;
  }

  function eqResponseDb(freq, sections) {
    var w = 2 * Math.PI * freq / FS, c1 = Math.cos(w), s1 = Math.sin(w), c2 = Math.cos(2 * w), s2 = Math.sin(2 * w), db = 0;
    sections.forEach(function (s) {
      if (!s.on) return;
      var nr = s.b0 + s.b1 * c1 + s.b2 * c2, ni = -s.b1 * s1 - s.b2 * s2;
      var dr = 1 + s.b1 * c1 + s.a2 * c2, di = -s.b1 * s1 - s.a2 * s2;
      db += 10 * Math.log10((nr * nr + ni * ni) / (dr * dr + di * di));
    });
    return db;
  }

  function drawSpectrum(box) {
    var pad = 44, bottom = box.h - 22, top = 8, minDb = -130, maxDb = 0;
    var fMin = 20, fMax = 20000;
    function xOf(fr) { return pad + Math.log(fr / fMin) / Math.log(fMax / fMin) * (box.w - pad - 40); }
    function yOf(db) { return top + (maxDb - Math.max(minDb, Math.min(maxDb, db))) / (maxDb - minDb) * (bottom - top); }
    [20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000].forEach(function (fr) {
      ctx.strokeStyle = cssVar("--grid"); ctx.beginPath(); ctx.moveTo(xOf(fr), top); ctx.lineTo(xOf(fr), bottom); ctx.stroke();
      text(fr >= 1000 ? fr / 1000 + "k" : String(fr), xOf(fr), box.h - 6, null, "center");
    });
    for (var db = 0; db >= minDb; db -= 20) { hline(pad, box.w - 40, yOf(db), cssVar("--grid")); text(db + " dB", pad - 4, yOf(db) + 4, null, "right"); }
    var start = cursorFrame();
    var ws = start + wetDelay();
    [[spectrumOf(audio.dry, start), cssVar("--dry"), 1, 1.5], [spectrumOf(audio.wet, ws), cssVar("--wet"), 0.7, 1.5],
     [spectrumOf(audio.wet, ws, audio.dry, start), cssVar("--diff"), 0.85, 1.25]].forEach(function (series) {
      ctx.strokeStyle = series[1]; ctx.globalAlpha = series[2]; ctx.lineWidth = series[3]; ctx.beginPath();
      var mags = series[0], started = false;
      for (var k = 1; k < mags.length; k++) {
        var fr = k * FS / 8192;
        if (fr < fMin || fr > fMax) continue;
        if (!started) { ctx.moveTo(xOf(fr), yOf(mags[k])); started = true; } else ctx.lineTo(xOf(fr), yOf(mags[k]));
      }
      ctx.stroke(); ctx.globalAlpha = 1;
    });
    var sections = new Dsp.sos.EqStage(FS, state.settings).sections;
    var eqActive = state.settings.power && state.stages.eqBands;
    var span = 15;
    function yEq(d) { return top + (span - Math.max(-span, Math.min(span, d))) / (2 * span) * (bottom - top); }
    ctx.strokeStyle = cssVar("--eqc"); ctx.lineWidth = 2.5; ctx.beginPath();
    for (var x = 0; x <= 200; x++) {
      var fr2 = fMin * Math.pow(fMax / fMin, x / 200), d2 = eqActive ? eqResponseDb(fr2, sections) : 0;
      if (x === 0) ctx.moveTo(xOf(fr2), yEq(d2)); else ctx.lineTo(xOf(fr2), yEq(d2));
    }
    ctx.stroke(); ctx.lineWidth = 1;
    [15, 0, -15].forEach(function (d) { text((d > 0 ? "+" : "") + d, box.w - 36, yEq(d) + 4, cssVar("--eqc")); });
    text("EQ dB", box.w - 36, top + 22, cssVar("--eqc"));
  }

  function drawGain(box) {
    var r = audio.result, pad = 44, bottom = box.h - 22, top = 8, minDb = -12, maxDb = 21;
    var bufs = Math.max(r.levelingGain.length, r.boostGain.length, 1);
    function xOf(i) { return pad + i / bufs * (box.w - pad - 8); }
    function yOf(db) { return top + (maxDb - Math.max(minDb, Math.min(maxDb, db))) / (maxDb - minDb) * (bottom - top); }
    for (var db = -12; db <= 21; db += 3) { hline(pad, box.w - 8, yOf(db), cssVar("--grid")); if (db % 6 === 0) text((db > 0 ? "+" : "") + db + " dB", pad - 4, yOf(db) + 4, null, "right"); }
    hline(pad, box.w - 8, yOf(0), cssVar("--dry"));
    if (!r.levelingGain.length && !r.boostGain.length) text("Power is off: no gain is applied", pad + 8, top + 16, cssVar("--text"));
    [[r.levelingGain, cssVar("--lev")], [r.boostGain, cssVar("--wet")]].forEach(function (series) {
      var g = series[0];
      if (!g.length) return;
      ctx.strokeStyle = series[1]; ctx.lineWidth = 2; ctx.beginPath();
      g.forEach(function (v, i) { var y = yOf(20 * Math.log10(Math.max(1e-6, v))); if (i === 0) ctx.moveTo(xOf(i), y); else ctx.lineTo(xOf(i), y); });
      ctx.stroke(); ctx.lineWidth = 1;
    });
    var seconds = bufs * r.bufferFrames / FS;
    for (var s = 0; s <= seconds; s += seconds > 8 ? 2 : 1) text(s + " s", xOf(s * FS / r.bufferFrames), box.h - 6, null, "center");
  }

  var LEGENDS = {
    wave: [["dry", "Original"], ["wet", "With FxSound"], ["mix", "Both overlap"]],
    zoom: [["dry", "Original samples"], ["wet", "With FxSound (see-through, on top)"]],
    spectrum: [["dry", "Original"], ["wet", "With FxSound"], ["diff", "Difference: what FxSound changed"],
               ["eqc", "Equalizer response (right axis)"]],
    gain: [["lev", "Volume Leveling gain"], ["wet", "Dynamic Boost gain (before limiting)"]]
  };
  function draw() {
    if (!audio.wet) return;
    var box = prepareCanvas();
    ({ wave: drawWave, zoom: drawZoom, spectrum: drawSpectrum, gain: drawGain })[state.view](box);
    var legend = $("legend");
    legend.textContent = "";
    LEGENDS[state.view].forEach(function (l) {
      legend.appendChild(el("span", { className: l[0] }, [el("i"), document.createTextNode(l[1])]));
    });
    $("view-hint").textContent = Explain.views[state.view];
  }

  function wireTabs() {
    Array.prototype.forEach.call(document.querySelectorAll(".tab"), function (tab) {
      var on = tab.getAttribute("data-view") === state.view;
      tab.classList.toggle("active", on); tab.setAttribute("aria-selected", on ? "true" : "false");
      tab.addEventListener("click", function () {
        state.view = tab.getAttribute("data-view"); saveState();
        Array.prototype.forEach.call(document.querySelectorAll(".tab"), function (t) {
          var sel = t === tab; t.classList.toggle("active", sel); t.setAttribute("aria-selected", sel ? "true" : "false");
        });
        draw();
      });
    });
  }

  // ---- Playback (Web Audio) ----------------------------------------------------------------------
  var ac = null, source = null;
  function play(buf) {
    stop();
    try {
      if (!ac) {
        var Ctx = window.AudioContext || window.webkitAudioContext;
        try { ac = new Ctx({ sampleRate: FS }); } catch (e) { ac = new Ctx(); }
      }
      if (ac.state === "suspended") ac.resume();
      var frames = buf.length / 2, ab = ac.createBuffer(2, frames, FS);
      var l = ab.getChannelData(0), r = ab.getChannelData(1);
      for (var i = 0; i < frames; i++) { l[i] = Math.max(-1, Math.min(1, buf[2 * i])); r[i] = Math.max(-1, Math.min(1, buf[2 * i + 1])); }
      source = ac.createBufferSource(); source.buffer = ab; source.connect(ac.destination);
      source.onended = function () { source = null; };
      source.start();
    } catch (e) {
      $("status").textContent = "Audio playback is not available in this browser.";
    }
  }
  function stop() { if (source) { try { source.stop(); } catch (e) { /* already stopped */ } source = null; } }

  // ---- Embedding -----------------------------------------------------------------------------------
  function setupEmbed() {
    var params = new URLSearchParams(window.location.search);
    if (params.get("embed") !== "1") return;
    document.documentElement.classList.add("embed");
    if (window.parent === window || !("ResizeObserver" in window)) return;
    new ResizeObserver(function () {
      window.parent.postMessage({ type: "fxsound-dsp-tutorial:height", height: document.documentElement.scrollHeight }, "*");
    }).observe(document.body);
  }

  // ---- Start ---------------------------------------------------------------------------------------
  function init() {
    setupEmbed();
    $("version").textContent = "Tutorial version " + VERSION + ".";
    buildKeyboard(); buildNoteTable(); renderSequenceChips(); buildControls(); buildStages(); wireTabs();
    showExplain("intro");

    var dbfs = $("dbfs"), dbfsOut = $("dbfs-out");
    dbfs.value = state.dbfs; dbfsOut.textContent = state.dbfs + " dBFS";
    dbfs.addEventListener("input", function () { state.dbfs = Number(dbfs.value); dbfsOut.textContent = state.dbfs + " dBFS"; changed("generate"); });
    wireExplain(dbfs, "dbfs");

    var cursor = $("cursor");
    cursor.value = Math.round(state.cursor * 1000);
    cursor.addEventListener("input", function () { state.cursor = Number(cursor.value) / 1000; saveState(); updateCursor(); draw(); });
    wireExplain(cursor, "cursor");

    var dbScale = $("db-scale"), align = $("align");
    dbScale.checked = state.dbScale; align.checked = state.align;
    dbScale.addEventListener("change", function () { state.dbScale = dbScale.checked; saveState(); draw(); });
    align.addEventListener("change", function () { state.align = align.checked; saveState(); updateCursor(); draw(); });
    wireExplain(align, "align");

    var repeat = $("repeat15");
    repeat.checked = state.repeat15;
    repeat.addEventListener("change", function () { state.repeat15 = repeat.checked; changed("generate"); });
    wireExplain(repeat, "volumeLeveling");

    $("btn-clear").addEventListener("click", function () { state.melody = []; renderSequenceChips(); changed("generate"); });
    $("btn-default").addEventListener("click", function () { state.melody = DEFAULT_MELODY.slice(); renderSequenceChips(); changed("generate"); });
    $("play-dry").addEventListener("click", function () { if (audio.dry) play(audio.dry); });
    $("play-wet").addEventListener("click", function () { if (audio.wet) play(audio.wet); });
    $("stop").addEventListener("click", stop);

    if ("ResizeObserver" in window) new ResizeObserver(function () { draw(); }).observe(canvas);
    if (window.matchMedia) window.matchMedia("(prefers-color-scheme: dark)").addEventListener("change", draw);

    generate(); process();
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init); else init();
})();
