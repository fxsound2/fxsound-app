package fxsound

import "testing"

// TestValidateBandIndexAcrossBandCounts confirms out-of-range band indices
// are rejected for every band count FxSound supports (5/10/15/20/31),
// without ever spawning a process -- this is what actually guarantees
// "rejected before spawning" for Iteration 6, since ValidateBandIndex is a
// pure function called before Apply/Run is ever reached.
func TestValidateBandIndexAcrossBandCounts(t *testing.T) {
	for _, numBands := range []int{5, 10, 15, 20, 31} {
		// last valid index and one past it
		if err := ValidateBandIndex(numBands-1, numBands); err != nil {
			t.Errorf("numBands=%d: last valid index %d: unexpected error: %v", numBands, numBands-1, err)
		}
		if err := ValidateBandIndex(numBands, numBands); err == nil {
			t.Errorf("numBands=%d: index %d (== numBands): expected error, got nil", numBands, numBands)
		}
		if err := ValidateBandIndex(-1, numBands); err == nil {
			t.Errorf("numBands=%d: index -1: expected error, got nil", numBands)
		}
		if err := ValidateBandIndex(0, numBands); err != nil {
			t.Errorf("numBands=%d: index 0: unexpected error: %v", numBands, err)
		}
	}
}

func TestValidateBandBatch(t *testing.T) {
	for _, numBands := range []int{5, 10, 15, 20, 31} {
		if err := ValidateBandBatch(numBands, numBands); err != nil {
			t.Errorf("numBands=%d: batch of %d: unexpected error: %v", numBands, numBands, err)
		}
		if err := ValidateBandBatch(numBands+1, numBands); err == nil {
			t.Errorf("numBands=%d: batch of %d: expected error, got nil", numBands, numBands+1)
		}
	}
}

func TestValidateBandGain(t *testing.T) {
	if err := ValidateBandGain(MinBandGainDb); err != nil {
		t.Errorf("min gain %.1f: unexpected error: %v", MinBandGainDb, err)
	}
	if err := ValidateBandGain(MaxBandGainDb); err != nil {
		t.Errorf("max gain %.1f: unexpected error: %v", MaxBandGainDb, err)
	}
	if err := ValidateBandGain(MinBandGainDb - 0.1); err == nil {
		t.Error("gain below MinBandGainDb: expected error, got nil")
	}
	if err := ValidateBandGain(MaxBandGainDb + 0.1); err == nil {
		t.Error("gain above MaxBandGainDb: expected error, got nil")
	}
}
