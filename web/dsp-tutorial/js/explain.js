/*
FxSound DSP tutorial - plain-language explanations shown in the "What's happening" panel.
Copyright (C) 2025  FxSound LLC. Licensed under the GNU Affero General Public License v3 or later.
*/
(function (root) {
  "use strict";
  root.FxExplain = {
    intro: {
      text: ["Sound is a pressure wave. Digital audio stores it as a list of numbers called samples: " +
        "44,100 of them every second for each channel. Each sample is the height of the wave at that instant.",
        "Click piano keys to build a melody, then move the FxSound controls and watch the waveform change. " +
        "Hover over or move any control to see what it does."],
      source: ""
    },
    dbfs: {
      text: ["dBFS means decibels relative to full scale. 0 dBFS is the loudest value a sample can hold " +
        "(a 24-bit code of 8,388,607). Every 6 dB down halves the amplitude: -6 dBFS is half of full scale, " +
        "-12 dBFS a quarter.",
        "This slider sets the loudest peak of your melody. Quieter peaks leave more headroom, which FxSound's " +
        "boost stages can then use."],
      source: ""
    },
    cursor: {
      text: ["Pick one sample (one instant) to inspect. You see its 24-bit integer code, the same value as a " +
        "float between -1 and +1, and its level in dBFS, before and after FxSound."],
      source: ""
    },
    align: {
      text: ["FxSound's output arrives 33 samples (0.75 ms) after its input. Dynamic Boost holds the audio in a short " +
        "delay so it can see a loud peak coming and turn the gain down before the peak reaches the output " +
        "(a 'look-ahead' limiter).",
        "0.75 ms is far too short to hear, but it means sample number N of the output corresponds to sample " +
        "N - 33 of the input. With this ticked, the zoom view and sample readout compare matching moments."],
      source: "dsp/ptechDsp/Maximizer/Maxi32/Maxi32.c — look-ahead delay (MAXI_LOOK_AHEAD_DELAY)"
    },
    power: {
      text: ["Power off takes FxSound out of the audio path: the app switches Windows back to your real speakers " +
        "or headphones, so you hear the original audio, untouched.",
        "With Power on, even with every control at zero, the Dynamic Boost limiter still runs. It delays audio by " +
        "0.75 ms and keeps peaks at or below -0.3 dBFS, so the output is about 0.3 dB quieter than the input.",
        "The engine itself also has an equalizer on/off switch, but the FxSound app has no control for it: the " +
        "equalizer stage is always on."],
      source: "fxsound/Source/GUI/FxController.cpp — FxController::powerOn; dsp/ptutil/dfxp/dfxpProcessReal.cpp"
    },
    notes: {
      text: ["Each piano key has a fixed pitch (frequency). In equal temperament every semitone multiplies the " +
        "frequency by the twelfth root of 2, about 1.059, so twelve semitones (one octave) exactly double it. " +
        "A4 is tuned to 440 Hz; C4 (middle C) is 261.63 Hz.",
        "A note also contains harmonics at 2x, 3x, 4x ... its frequency. Those are the peaks you see to the right " +
        "of each note in the Spectrum view."],
      source: ""
    },
    fidelity: {
      text: ["Clarity makes the treble crisper, the way a slightly overdriven amplifier adds brightness. It takes " +
        "a copy of just the high frequencies (above about 1.7 kHz), gently distorts that copy so it gains new, " +
        "even higher overtones, and mixes it back with the original. Bass and midrange are left alone.",
        "The slider sets how hard the copy is driven: higher means more added sparkle. In the Samples view the " +
        "wave of a high note gets small extra wiggles; in the Spectrum view new peaks appear above the note's " +
        "own harmonics."],
      source: "dsp/ptechDsp/Aural/Aural032/Auralp32.c — dspsAuralProcess32"
    },
    ambience: {
      text: ["Ambience is a plate reverb (the 'Lex' algorithm): a network of delays and all-pass filters that " +
        "builds a dense, decaying echo. The slider sets how long the echo takes to die away and how much of it " +
        "is mixed in.",
        "Engine quirk, reproduced exactly: at positions 1 and 2 the mix formula gives the reverb a small " +
        "negative level (it is subtracted, not added) and lifts the dry sound slightly above its original level; " +
        "at position 3 the reverb runs but is mixed in at zero. From 4 upward it is added normally. Look for " +
        "the tail after the last note in the Waveform view."],
      source: "dsp/ptechDsp/Lex/Lex32/Lex32.c — dspsLexReverbProcess32"
    },
    surround: {
      text: ["Surround Sound is mid/side processing. 'Mid' is what both channels share (L+R)/2; 'side' is the " +
        "difference. The side part is boosted (up to 3.1x) and the mid part slightly reduced, which widens the " +
        "stereo image.",
        "A mono sound has no side part, so it is only made a little quieter. Compare the two channels in the " +
        "Waveform view: they move further apart."],
      source: "dsp/ptechDsp/wide/Wide32/Wide32.c — dspsWideProcess32"
    },
    dynamicBoost: {
      text: ["Dynamic Boost raises the overall level and uses a look-ahead limiter so peaks never pass -0.3 dBFS. " +
        "The boost backs off automatically when the music is already loud, but never below about +0.5 dB — so " +
        "very loud material gets that small boost even with the slider at 0.",
        "The slider is not linear: positions 1-5 give about 1.6, 3.1, 4.7, 6.6 and 10 dB, and 6 to 10 all give " +
        "11.6 dB. It does not add harmonics. See the 'Gain over time' view."],
      source: "dsp/ptechDsp/Maximizer/Maxi32/Maxi32.c — dspsMaximizerProcess32"
    },
    bass: {
      text: ["Bass Boost is a single peaking filter centred on 90 Hz with a Q of 2.5. The slider sets its boost " +
        "from 0 to 15 dB. Low notes (C1 to C3) are affected most; high notes barely change."],
      source: "dsp/ptechDsp/Play/Play32/Play32.c — bass boost biquad"
    },
    band: {
      text: ["Each equalizer band is a peaking (bell) filter: it boosts or cuts a range of frequencies around its " +
        "centre and leaves the rest untouched. A band at 0 dB leaves its frequencies unchanged; the engine " +
        "doesn't even run that band's filter.",
        "Boosting a band changes notes whose harmonics fall near that frequency. Use the Spectrum view, where the " +
        "violet curve is the combined response of all bands."],
      source: "dsp/ptutil/Filt/FiltCalcBiqd.cpp — filtCalcParametric"
    },
    masterGain: {
      text: ["Master Gain is a plain volume multiplier applied after the equalizer bands: +6 dB doubles the " +
        "amplitude, -6 dB halves it. Like every other control, it has no effect while Power is off."],
      source: "dsp/ptutil/SOS/SosProcess.cpp — sosProcessBuffer"
    },
    balance: {
      text: ["Balance turns one channel down; it never turns the other up. Positive values reduce the left channel, " +
        "negative values reduce the right."],
      source: "dsp/ptutil/DspUtil/GraphicEq/GraphicEqSet.cpp — GraphicEqSetBalance"
    },
    filterQ: {
      text: ["Filter Q sets how wide each equalizer band's bell is. Low Q is a wide, gentle bell that also " +
        "affects frequencies some way from the band's centre; high Q is a narrow, steep bell that changes only " +
        "frequencies close to the centre.",
        "Try it: boost one band, open the Spectrum view and move Filter Q from 1 to 3. The violet bell gets " +
        "narrower while its peak stays at the gain you set. For 10 bands the base Q is about 1.6, and this " +
        "control multiplies it (1.6 to 4.8)."],
      source: "dsp/ptutil/DspUtil/GraphicEq/GraphicEqSet.cpp — GraphicEqReSetAllBandFreqs"
    },
    volumeLeveling: {
      text: ["Volume Leveling steers loudness toward a target set by this control (higher = louder target: " +
        "about -18, -12 and -6 dBFS RMS at settings 1, 2 and 4). Every buffer it measures the audio and nudges " +
        "the gain: down for louder audio, up for quieter audio — normally by at most x1, x2 or x4 at those " +
        "settings. If the audio stays very quiet for about 10 seconds, it allows up to x10 (+20 dB).",
        "It adapts once per buffer (10 ms here), so its effect builds up over seconds. Tick 'Repeat melody to 15 s' " +
        "and watch the 'Gain over time' view."],
      source: "dsp/ptutil/SOS/SosProcess.cpp — applyVolumeLeveling"
    },
    normalization: {
      text: ["Normalization is an automatic volume cap. It measures the level (RMS) of each buffer and, if the " +
        "audio is louder than a target, turns it down; it never turns quiet audio up.",
        "Volume Leveling does the same job and more (it also lifts quiet audio and allows for tone and headroom), " +
        "so the FxSound app has no Normalization control. It stays at its default target of 0 dB, where the " +
        "engine skips it completely."],
      source: "dsp/ptutil/SOS/SosProcess.cpp — sosProcessBuffer, normalization block"
    },
    views: {
      wave: "The whole melody. Horizontal: time in seconds. Vertical: sample value, from -1 to +1 (full scale). " +
        "Top lane: left channel; bottom lane: right channel. Each thin column shows the lowest and highest sample " +
        "in that slice of time. Grey is the original; red is FxSound's output, see-through and drawn on top, " +
        "so where they overlap the two colours mix (see the legend).",
      zoom: "About 5 ms of the left channel around the inspected sample, so you can see the individual samples as dots " +
        "(grey: original, red: with FxSound). The vertical scale zooms to fit. " +
        "With 44,100 samples per second, each dot is 22.7 microseconds apart.",
      spectrum: "How much energy is at each frequency (log scale), starting at the inspected sample. Each note shows as " +
        "a row of peaks: the fundamental and its harmonics. Grey is the original and red is with FxSound. Blue is the difference " +
        "(processed minus original): what FxSound added or removed. Tip: untick Dynamic Boost in step 4 to remove its " +
        "fixed -0.3 dB, so the blue curve shows only the stage you are studying. Violet is the combined equalizer response.",
      gain: "The gain Volume Leveling (blue) and Dynamic Boost (red) apply over time, in dB. 0 dB means no change."
    }
  };
})(typeof globalThis !== "undefined" ? globalThis : window);
