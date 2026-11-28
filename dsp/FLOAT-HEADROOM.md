# Floating-point headroom and existing Master Gain

Volume Leveling still detects levels, computes its gain and interpolates that gain
across each audio block. Its output can now exceed full scale temporarily instead
of flattening each sample at its intermediate detector ceiling. Downstream effects
and the existing final Maximizer receive those preserved peaks. Clipping a peak
earlier would lose information that a later volume reduction cannot recover.

This change does not disable Normalization, Volume Leveling, Clarity/Aural or the
final peak limiter. They are intentional level-dependent effects. It does not add
the private rounded-peak mode, a preamp slider, automatic gain compensation, or
change the DSP's integer-format conversions.

The existing **Master Gain** is a manual gain in the Graphic EQ path. It precedes
leveling and the remaining effects. Its present behavior is unchanged: disabling
EQ bypasses that path, including Master Gain, and unsupported EQ channel layouts
also bypass it. It is not an independent output volume control.

Lowering Master Gain and adding its inverse after the effects would generally
change the sound: gain does not commute with a level detector, harmonic generator
or limiter. This contribution therefore uses the existing control without adding
a compensating output stage or changing EQ-Off/bypass semantics.

Floating-point headroom is different from integer representation. An actual
conversion to PCM16/24/32 still needs the range required by that format. Pointer
types such as `short*`/`long*`, and names such as `Maxi32`, do not by themselves
identify an integer signal: the current FxSound processing path uses float PCM.
This narrow correction does not promise safe handling of arbitrary non-finite or
near-maximum float magnitudes, or remove the final output ceiling.

Related to [#112](https://github.com/fxsound2/fxsound-app/issues/112). This addresses
premature intermediate clipping rather than a universal independent preamp.
DSP changes require explicit human review before merge. Numerical regression
instructions and their limits are in [tests/README.md](tests/README.md).
