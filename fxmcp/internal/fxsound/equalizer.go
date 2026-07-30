package fxsound

// MinBandGainDb and MaxBandGainDb mirror FxController.h's MIN_GAIN/MAX_GAIN
// constants, the fixed per-band boost/cut range enforced by
// setEqBandBoostCut.
const (
	MinBandGainDb = -12.0
	MaxBandGainDb = 12.0
)

// ValidateBandBatch checks that sending numPairs band settings is
// consistent with FxController.cpp's applyConfig, which drops an ENTIRE
// --set_band_freq/--set_band_gain batch if the pair count exceeds the
// current band count (FxController.cpp: "pairs.size() <= getNumEqBands()"),
// rather than applying the ones that would have been valid.
func ValidateBandBatch(numPairs, numBands int) error {
	if numPairs > numBands {
		return newError(ErrKindValueRejected, nil, "%d band setting(s) exceeds the current %d-band equalizer; FxSound silently ignores the entire batch in this case, not just the excess entries", numPairs, numBands)
	}
	return nil
}

// ValidateBandIndex checks idx is a valid 0-based band index for the
// current equalizer configuration. FxController's setEqBandFrequency and
// setEqBandBoostCut both silently no-op an individual pair whose index is
// out of range instead of erroring.
func ValidateBandIndex(idx, numBands int) error {
	if idx < 0 || idx >= numBands {
		return newError(ErrKindValueRejected, nil, "band index %d is out of range for a %d-band equalizer (valid: 0-%d)", idx, numBands, numBands-1)
	}
	return nil
}

// ValidateBandGain checks gainDb is within FxController's fixed per-band
// boost/cut range. Note: setEqBandFrequency's equivalent frequency range
// is per-band and comes from the DSP engine (dfx_dsp_.getEqBandFrequencyRange),
// which isn't exposed via status.json or any documented constant, so an
// out-of-range frequency can't be validated client-side the way gain can
// -- it will still be silently dropped server-side if out of range.
func ValidateBandGain(gainDb float64) error {
	return ValidateRange("gain", gainDb, MinBandGainDb, MaxBandGainDb)
}
