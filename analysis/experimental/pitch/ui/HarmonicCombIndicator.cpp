// SPDX-License-Identifier: AGPL-3.0-or-later
#include "HarmonicCombIndicator.h"
#include "../HarmonicCarrierDisplay.h"
#include <algorithm>
#include <cmath>

namespace fxanalysis::experimental
{
using namespace juce;
namespace
{
constexpr juce::uint32 background = 0xff101821, plotBackground = 0xff172430;
constexpr juce::uint32 text = 0xffc3d0dc, rawColour = harmonicCarrierGold;
constexpr juce::uint32 residualColour = 0xff59d9da, selectedColour = 0xffffdd97;
constexpr double maximumTrailAlpha = 115.0;
constexpr juce::uint32 inputColour = 0xffd5e2ee;
const String carrierBand = String(harmonicCarrierMinimumHz, 0) + " Hz - " + String(harmonicCarrierMaximumHz / 1000.0, 0) + " kHz";
const String fullTooltip = "Carrier search " + carrierBand + ". Gold shows up to " + String(harmonicMaximumComponents) + " components. White shows the raw FFT. Diagnostics show the reconstructed model and positive fit error.";
const String compactTooltip = carrierBand + ". Gold carrier lobes and fading history align with the red spectrum bars. Double-click for details.";


}
HarmonicCombIndicator::HarmonicCombIndicator()
{
    setOpaque(true);
    setTooltip(fullTooltip);
    for (auto* button : {&enabled_, &strip_, &scale_, &reference_, &mode_, &diagnostics_})
    {
        button->setColour(TextButton::buttonColourId, Colour(0xff303f4e));
        button->setColour(TextButton::textColourOffId, Colour(text));
        addAndMakeVisible(*button);
    }
    const auto configure = [this](Slider& slider, Label& label, const String& name,
        const HarmonicControlRange& range)
    {
        slider.setSliderStyle(Slider::LinearHorizontal);
        slider.setTextBoxStyle(Slider::TextBoxRight, false, 54, 22);
        slider.setRange(range.minimum, range.maximum, range.step);
        slider.setNumDecimalPlacesToDisplay(2);
        slider.setColour(Slider::textBoxTextColourId, Colour(text));
        slider.setColour(Slider::textBoxBackgroundColourId, Colour(0xff303f4e));
        label.setText(name, dontSendNotification);
        label.setColour(Label::textColourId, Colour(text));
#if JUCE_MAJOR_VERSION >= 8
        label.setFont(Font(FontOptions(10.5f).withMetricsKind(TypefaceMetricsKind::legacy).withName(Font::getDefaultSansSerifFontName())));
#else
        label.setFont(Font(10.5f));
#endif
        addAndMakeVisible(slider);
        addAndMakeVisible(label);
    };
    configure(subtraction_, subtractionLabel_, "Subtracted fraction", harmonicControls::subtraction);
    configure(decay_, decayLabel_, "Decay exponent p (1/h^p)", harmonicControls::decay);
    configure(gain_, gainLabel_, "Fixed display gain", harmonicControls::gain);
    configure(range_, rangeLabel_, "Visible range [dB]", harmonicControls::rangeDb);
    configure(spread_, spreadLabel_, "Widening [%]", harmonicControls::wideningPercent);
    configure(persistence_, persistenceLabel_, "Strip trail half-life [ms]", harmonicControls::trailMs);
    enabled_.onClick = [this] { settings_.enabled = !settings_.enabled; changed(); };
    strip_.onClick = [this]
    {
        const auto next = stripMode_ == CompactIndicatorMode::none ? CompactIndicatorMode::pitch
            : stripMode_ == CompactIndicatorMode::pitch ? CompactIndicatorMode::carriers : CompactIndicatorMode::none;
        setStripMode(next);
        settings_.showInStrip = next == CompactIndicatorMode::carriers;
        if (onStripModeChanged) onStripModeChanged(next);
        else changed();
    };
    subtraction_.onValueChange = [this]
    {
        if (settings_.iterativeExtraction) settings_.maximumComponents = roundToInt(subtraction_.getValue());
        else settings_.subtractionFraction = subtraction_.getValue();
        changed();
    };
    decay_.onValueChange = [this] { settings_.harmonicDecay = decay_.getValue(); changed(); };
    gain_.onValueChange = [this] { settings_.displayGain = gain_.getValue(); changed(); };
    range_.onValueChange = [this] { settings_.displayRangeDb = range_.getValue(); changed(); };
    spread_.onValueChange = [this] { settings_.harmonicWideningPercent = spread_.getValue(); changed(); };
    persistence_.onValueChange = [this] { settings_.trailHalfLifeSeconds = DisplayTiming::seconds(persistence_.getValue()); changed(); };
    scale_.onClick = [this] { settings_.logarithmicDisplay = !settings_.logarithmicDisplay; changed(); };
    reference_.onClick = [this] { settings_.highResReference = !settings_.highResReference; changed(); };
    mode_.onClick = [this] { settings_.iterativeExtraction = !settings_.iterativeExtraction; changed(); };
    diagnostics_.onClick = [this] { setShowDiagnostics(!diagnosticsVisible_); };
    diagnostics_.setButtonText("FFT diagnostics: off");
    mode_.setTooltip("Iterative mode jointly fits up to four fixed harmonic combs. Legacy profile mode retains the earlier one-family partial cancellation for comparison.");
    subtraction_.setTooltip("One cancellation pass. Zero retains the original curve; one removes the fitted observed partials completely.");
    decay_.setTooltip("Exponent p in amplitude(h)=amplitude(1)/h^p. p=0: equal teeth; p=0.5: H2=71%; p=1: H2=50%; p=2: H2=25%. Values above 1 mean faster decay, not an amplitude above 100%.");
    gain_.setTooltip("Same fixed gain for all curves and frames. The top is reference-normalized amplitude 1/gain. No per-frame peak normalization.");
    scale_.setTooltip("dB compresses weak amplitudes into a fixed visible range. Linear retains amplitude proportions. Neither changes the numerical fit or selected carrier.");
    range_.setTooltip("Visible decibels below the fixed reference ceiling 1/gain. 60 dB shows amplitudes down to one thousandth of this ceiling. Independent of the HighRes/flat reference switch.");
    reference_.setTooltip("Shared HighRes frequency reference anchored at 1 kHz. In iterative mode it ranks candidates and sets marker heights; FFT-domain subtraction uses the unweighted fitted amplitudes. Flat bypasses this reference.");
    spread_.setTooltip("Widens the fitted FFT teeth by the selected percentage, with an extra gradual bass boost on the fundamental only. Higher harmonics keep the selected widening. At 0% all teeth retain their base width of four FFT bins.");
    persistence_.setTooltip("Compact strip only: old marker positions and heights fade in opacity. Zero disables the trail. No component identities or pitch smoothing.");
    setSettings(settings_);
}
void HarmonicCombIndicator::setSettings(const HarmonicCombSettings& settings)
{
    settings_ = settings;
    enabled_.setButtonText(settings_.enabled ? "Calculation: on" : "Calculation: off");
    setStripMode(stripMode_);
    mode_.setButtonText(settings_.iterativeExtraction ? "Mode: extracted components" : "Mode: legacy profile");
    const auto& range = settings_.iterativeExtraction ? harmonicControls::components : harmonicControls::subtraction;
    subtraction_.setRange(range.minimum, range.maximum, range.step);
    subtraction_.setNumDecimalPlacesToDisplay(settings_.iterativeExtraction ? 0 : 2);
    subtractionLabel_.setText(settings_.iterativeExtraction ? "Maximum components" : "Subtracted fraction", dontSendNotification);
    subtraction_.setValue(settings_.iterativeExtraction ? settings_.maximumComponents : settings_.subtractionFraction, dontSendNotification);
    subtraction_.setTooltip(settings_.iterativeExtraction ? "Maximum accepted components per frame, with early stopping. Full joint model cancellation at each step."
        : "One legacy cancellation pass; fraction zero retains the original profile.");
    decay_.setValue(settings_.harmonicDecay, dontSendNotification);
    gain_.setValue(settings_.displayGain, dontSendNotification);
    range_.setValue(settings_.displayRangeDb, dontSendNotification);
    spread_.setValue(settings_.harmonicWideningPercent, dontSendNotification);
    spread_.setEnabled(settings_.iterativeExtraction);
    spreadLabel_.setEnabled(settings_.iterativeExtraction);
    persistence_.setValue(DisplayTiming::millisecondsPerSecond * settings_.trailHalfLifeSeconds, dontSendNotification);
    range_.setEnabled(settings_.logarithmicDisplay);
    rangeLabel_.setEnabled(settings_.logarithmicDisplay);
    scale_.setButtonText(settings_.logarithmicDisplay ? "Scale: dB" : "Scale: linear");
    reference_.setButtonText(settings_.highResReference ? "Ref: HighRes" : "Ref: flat");
}
void HarmonicCombIndicator::setStripMode(CompactIndicatorMode mode)
{
    stripMode_ = mode;
    settings_.showInStrip = mode == CompactIndicatorMode::carriers;
    strip_.setButtonText(mode == CompactIndicatorMode::none ? TRANS("Main strip: none")
        : mode == CompactIndicatorMode::pitch ? TRANS("Main strip: pitch movement") : TRANS("Main strip: carriers"));
}

void HarmonicCombIndicator::changed()
{
    setSettings(settings_);
    clear();
    if (onSettingsChanged) onSettingsChanged(settings_);
}
void HarmonicCombIndicator::setSnapshot(const HarmonicCombSnapshot& snapshot)
{
    if (compact_ && snapshot.iterativeExtraction && snapshot.enabled
        && (snapshot.status == PitchMotionResult::ok || snapshot.status == PitchMotionResult::noSignal))
    {
        if (snapshot.displayGain != snapshot_.displayGain || snapshot.logarithmicDisplay != snapshot_.logarithmicDisplay
            || snapshot.highResReference != snapshot_.highResReference || snapshot.displayRangeDb != snapshot_.displayRangeDb)
            clearTrail();
        trail_.observe(snapshot, DisplayTiming::seconds(Time::getMillisecondCounterHiRes()));
    }
    else clearTrail();
    snapshot_ = snapshot;
    updateTrailTimer();
    repaint();
}
void HarmonicCombIndicator::clearTrail() { stopTimer(); trail_.clear(); repaint(); }
void HarmonicCombIndicator::clear() { clearTrail(); snapshot_ = {}; repaint(); }
void HarmonicCombIndicator::setUnavailable()
{
    if (!snapshot_.iterativeExtraction) { clear(); return; }
    snapshot_.hasEstimate = false;
    snapshot_.componentCount = 0;
    trail_.expire(DisplayTiming::seconds(Time::getMillisecondCounterHiRes()), snapshot_.trailHalfLifeSeconds);
    updateTrailTimer();
    repaint();
}
void HarmonicCombIndicator::updateTrailTimer()
{
    if (compact_ && isShowing() && trail_.count() > 0) { if (!isTimerRunning()) startTimerHz(30); }
    else stopTimer();
}
void HarmonicCombIndicator::timerCallback()
{
    trail_.expire(DisplayTiming::seconds(Time::getMillisecondCounterHiRes()), snapshot_.trailHalfLifeSeconds);
    updateTrailTimer();
    repaint();
}
void HarmonicCombIndicator::visibilityChanged() { if (!isShowing()) clearTrail(); else updateTrailTimer(); }
void HarmonicCombIndicator::setCompact(bool compact)
{
    compact_ = compact;
    clearTrail();
    setTooltip(compact_ ? compactTooltip : fullTooltip);
    const std::array<Component*, 18> controls{{&enabled_, &strip_, &scale_, &reference_, &mode_, &diagnostics_, &subtraction_, &decay_, &gain_, &range_, &spread_, &persistence_,
        &subtractionLabel_, &decayLabel_, &gainLabel_, &rangeLabel_, &spreadLabel_, &persistenceLabel_}};
    for (auto* component : controls) component->setVisible(!compact_);
    resized();
    repaint();
}
void HarmonicCombIndicator::setShowDiagnostics(bool show)
{
    diagnosticsVisible_ = show;
    diagnostics_.setButtonText(show ? "FFT diagnostics: on" : "FFT diagnostics: off");
    repaint();
}
void HarmonicCombIndicator::resized()
{
    if (compact_)
    {
        if (getWidth() > 0 && getHeight() > 0) trailMask_ = Image(Image::ARGB, getWidth(), getHeight(), true);
        return;
    }
    const int width = (std::max)(1, getWidth() - 16), column = (width - 16) / 3;
    enabled_.setBounds(8, 4, 100, 23);
    mode_.setBounds(116, 4, 190, 23);
    strip_.setBounds(314, 4, (std::max)(1, getWidth() - 322), 23);
    reference_.setBounds(8, 34, 132, 23);
    scale_.setBounds(148, 34, 120, 23);
    diagnostics_.setBounds(276, 34, 160, 23);
    int index = 0;
    for (auto pair : {std::pair<Label*, Slider*>{&subtractionLabel_, &subtraction_},
        {&decayLabel_, &decay_}, {&gainLabel_, &gain_}, {&spreadLabel_, &spread_},
        {&persistenceLabel_, &persistence_}, {&rangeLabel_, &range_}})
    {
        const int x = 8 + (index % 3) * (column + 8), y = 65 + (index / 3) * 42;
        pair.first->setBounds(x, y, column, 14);
        pair.second->setBounds(x, y + 15, column, 22);
        ++index;
    }
}
void HarmonicCombIndicator::drawTrail(Graphics& g, Rectangle<float> area)
{
    if (!compact_ || trail_.count() == 0 || !trailMask_.isValid()) return;
    trailMask_.clear(trailMask_.getBounds(), Colours::transparentBlack);
    const double now = DisplayTiming::seconds(Time::getMillisecondCounterHiRes());
    {
        Image::BitmapData pixels(trailMask_, Image::BitmapData::readWrite);
        const auto put = [&pixels](int x, int y, uint8 alpha)
        {
            if (x >= 0 && y >= 0 && x < pixels.width && y < pixels.height
                && alpha > pixels.getPixelColour(x, y).getAlpha())
                pixels.setPixelColour(x, y, Colour(rawColour).withAlpha(alpha));
        };
        for (int i = 0; i < trail_.count(); ++i)
        {
            const auto& marker = trail_.marker(i);
            const double centre = harmonicPlotPosition(snapshot_, true, marker.frequencyHz) * area.getWidth();
            const double height = area.getHeight() * harmonicDisplayHeight(marker.amplitude,
                snapshot_.displayGain, snapshot_.logarithmicDisplay, snapshot_.displayRangeDb);
            const double sigma = harmonicMarkerSigmaPixels(snapshot_, true, marker.frequencyHz, area.getWidth());
            const double alpha = std::clamp(maximumTrailAlpha * HarmonicMarkerTrail::opacity(marker, now,
                snapshot_.trailHalfLifeSeconds), 0.0, maximumTrailAlpha);
            const int first = (std::max)(0, static_cast<int>(std::floor(centre - markerSupportSigmas * sigma)));
            const int last = (std::min)(pixels.width - 1, static_cast<int>(std::ceil(centre + markerSupportSigmas * sigma)));
            for (int x = first; x <= last; ++x)
            {
                const double top = area.getHeight() - height * harmonicMarkerLobe(x - centre, sigma);
                for (int row = (std::max)(0, static_cast<int>(std::floor(top))); row < pixels.height; ++row)
                    put(x, row, static_cast<uint8>(alpha * std::clamp(row + 1.0 - top, 0.0, 1.0)));
            }
        }
    }
    // Maximum alpha per pixel, never additive accumulation of overlapping stamps.
    g.drawImageAt(trailMask_, roundToInt(area.getX()), roundToInt(area.getY()));
}
void HarmonicCombIndicator::mouseDoubleClick(const MouseEvent&) { if (onDoubleClick) onDoubleClick(); }
void HarmonicCombIndicator::drawProfiles(Graphics& g, Rectangle<float> area)
{
    g.setColour(Colour(plotBackground));
    g.fillRect(area);
    Graphics::ScopedSaveState plotState(g);
    g.reduceClipRegion(area.getSmallestIntegerContainer());
    const auto level = [this](double amplitude)
    {
        return static_cast<float>(harmonicDisplayHeight(amplitude, snapshot_.displayGain,
            snapshot_.logarithmicDisplay, snapshot_.displayRangeDb));
    };
    if (!compact_)
    {
        Path input;
        for (int index = 0; index < snapshot_.inputSpectrumCount; ++index)
        {
            const double hz = (snapshot_.firstInputBin + index) * snapshot_.inputBinHz;
            const float x = area.getX() + static_cast<float>(harmonicPlotPosition(snapshot_, false, hz)) * area.getWidth();
            const float height = level(snapshot_.inputMagnitude[index] * harmonicReferenceGain(hz, snapshot_.highResReference));
            const float y = area.getBottom() - height * area.getHeight();
            if (index == 0) input.startNewSubPath(x, y); else input.lineTo(x, y);
        }
        g.setColour(Colour(inputColour));
        g.strokePath(input, PathStrokeType(1.25f));
    }
    if (snapshot_.iterativeExtraction)
    {
        drawTrail(g, area);
        if (!compact_ && diagnosticsVisible_)
            for (bool error : {false, true})
            {
                Path path;
                for (int i = 0; i < snapshot_.inputSpectrumCount; ++i)
                {
                    const double hz = (snapshot_.firstInputBin + i) * snapshot_.inputBinHz;
                    const double value = error ? std::max(0.0f, snapshot_.errorMagnitude[i]) : snapshot_.modelMagnitude[i];
                    const float x = area.getX() + static_cast<float>(harmonicPlotPosition(snapshot_, false, hz)) * area.getWidth();
                    const float y = area.getBottom() - level(value * harmonicReferenceGain(hz, snapshot_.highResReference)) * area.getHeight();
                    if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
                }
                g.setColour(Colour(error ? residualColour : selectedColour));
                g.strokePath(path, PathStrokeType(1.4f));
            }
        std::array<HarmonicDisplayLobe, harmonicMaximumComponents> lobes{};
        const int count = std::clamp(snapshot_.componentCount, 0, harmonicMaximumComponents);
        for (int i = 0; i < count; ++i)
        {
            lobes[i] = harmonicDisplayLobe(snapshot_, compact_, snapshot_.components[i], area.getWidth());
            lobes[i].height *= area.getHeight();
        }
        // One outer envelope lets neighbouring detections coalesce. Taking the
        // maximum retains the fixed amplitude scale without summing detections.
        if (count > 0)
        {
            Path envelope, outline;
            bool outlineActive = false;
            envelope.startNewSubPath(area.getX(), area.getBottom());
            const int columns = (std::max)(1, static_cast<int>(std::ceil(area.getWidth())));
            for (int column = 0; column <= columns; ++column)
            {
                const double offset = (std::min)(static_cast<double>(column), static_cast<double>(area.getWidth()));
                double height = 0.0;
                for (int i = 0; i < count; ++i)
                    height = (std::max)(height, lobes[i].height * harmonicMarkerLobe(offset - lobes[i].centre, lobes[i].sigma));
                const float x = area.getX() + static_cast<float>(offset);
                const float y = area.getBottom() - static_cast<float>(height);
                envelope.lineTo(x, y);
                if (height > 0.1)
                {
                    if (outlineActive) outline.lineTo(x, y); else outline.startNewSubPath(x, y);
                    outlineActive = true;
                }
                else outlineActive = false;
            }
            envelope.lineTo(area.getRight(), area.getBottom());
            envelope.closeSubPath();
            g.setColour(Colour(rawColour).withAlpha(0.60f));
            g.fillPath(envelope);
            g.setColour(Colour(selectedColour));
            g.strokePath(outline, PathStrokeType(1.25f));
        }
        if (!compact_)
            for (int i = 0; i < count; ++i)
            {
                g.setColour(Colour(i == 0 ? selectedColour : rawColour));
                g.drawText(String(snapshot_.components[i].extractionRank),
                    roundToInt(area.getX() + lobes[i].centre) - 10,
                    roundToInt(area.getBottom() - lobes[i].height) - 20, 20, 16, Justification::centred);
            }
        return;
    }
    // Paint the original last so coincident curves retain its gold foreground.
    for (bool residual : {true, false})
    {
        if (compact_ && residual) continue;
        Path path;
        bool started = false;
        std::array<float, harmonicProfileCapacity> values{};
        for (int index = 0; index < snapshot_.count; ++index)
        {
            const float amplitude = residual ? snapshot_.after[index] : snapshot_.before[index];
            if (!std::isfinite(amplitude)) { started = false; continue; }
            values[index] = static_cast<float>(amplitude
                * harmonicReferenceGain(snapshot_.frequencyHz[index], snapshot_.highResReference));
            const float x = area.getX() + static_cast<float>(harmonicPlotPosition(snapshot_, compact_, snapshot_.frequencyHz[index])) * area.getWidth();
            const float height = level(values[index]);
            const float y = area.getBottom() - height * area.getHeight();
            if (started) path.lineTo(x, y); else { path.startNewSubPath(x, y); started = true; }
        }
        g.setColour(Colour(residual ? residualColour : rawColour));
        g.strokePath(path, PathStrokeType(residual ? 1.5f : compact_ ? 2.0f : 3.0f));
        const float radius = residual ? 2.25f : compact_ ? 2.0f : 3.5f;
        for (int index = 0; index < snapshot_.count; ++index)
        {
            if (!harmonicLocalMaximum(values, snapshot_.count, index) || level(values[index]) <= 0.0f) continue;
            const float x = area.getX() + static_cast<float>(harmonicPlotPosition(snapshot_, compact_, snapshot_.frequencyHz[index])) * area.getWidth();
            const float y = area.getBottom() - level(values[index]) * area.getHeight();
            if (!residual)
            {
                g.setColour(Colour(rawColour).withAlpha(compact_ ? 0.8f : 0.55f));
                g.drawVerticalLine(roundToInt(x), y, area.getBottom());
                g.setColour(Colour(rawColour));
            }
            g.fillEllipse(x - radius, y - radius, 2.0f * radius, 2.0f * radius);
        }
    }
    if (snapshot_.hasEstimate)
    {
        const float x = area.getX() + static_cast<float>(harmonicPlotPosition(snapshot_, compact_, snapshot_.selectedFrequencyHz)) * area.getWidth();
        const float height = level(snapshot_.components[0].displayAmplitude) * area.getHeight();
        g.setColour(Colour(selectedColour));
        g.drawVerticalLine(roundToInt(x), area.getBottom() - height, area.getBottom());
        if (!compact_) g.fillEllipse(x - 2.5f, area.getBottom() - height - 2.5f, 5.0f, 5.0f);
    }
}
void HarmonicCombIndicator::paint(Graphics& g)
{
    g.fillAll(Colour(background));
    if (compact_) { drawProfiles(g, getLocalBounds().toFloat()); return; }
    const Rectangle<float> plot(52.0f, 191.0f, (std::max)(1.0f, getWidth() - 65.0f),
        (std::max)(1.0f, getHeight() - 294.0f));
    g.setColour(Colour(text));
    g.setFont(11.0f);
    const String legend = carrierBand + (snapshot_.iterativeExtraction
        ? (diagnosticsVisible_ ? " | gold: components/model | white: raw FFT | cyan: positive fit error"
                               : " | gold: extracted components | white: raw FFT")
        : " | legacy gold: original profile | cyan: residual profile | white: raw FFT");
    g.drawFittedText(legend, 8, 151, getWidth() - 16, 17, Justification::centredLeft, 1);
    g.drawText("Reference amplitude | " + String(settings_.logarithmicDisplay ? "dB" : "linear")
        + (settings_.highResReference ? " | HighRes ref @1k" : " | flat ref")
        + " | gain x" + String(settings_.displayGain, 1), 52, 171, getWidth() - 65, 16, Justification::centredLeft);
    drawProfiles(g, plot);
    g.setFont(10.5f);
    if (settings_.logarithmicDisplay)
        for (int tick = 0; tick <= 3; ++tick)
        {
            const float y = plot.getY() + plot.getHeight() * tick / 3.0f;
            g.setColour(Colour(text).withAlpha(0.15f));
            g.drawHorizontalLine(roundToInt(y), plot.getX(), plot.getRight());
            g.setColour(Colour(text));
            g.drawText(tick == 0 ? String("0") : String(-settings_.displayRangeDb * tick / 3.0, 1),
                0, roundToInt(y) - 7, 47, 14, Justification::centredRight);
        }
    else
    {
        g.setColour(Colour(text));
        g.drawText(String(1.0 / settings_.displayGain, 3), 0, roundToInt(plot.getY()) - 7, 47, 15, Justification::centredRight);
    }
    g.setColour(Colour(text));
    for (double hz : {harmonicCarrierMinimumHz, 200.0, 300.0, 500.0, 1000.0, 2000.0, harmonicCarrierMaximumHz})
    {
        const float x = plot.getX() + static_cast<float>(harmonicPlotPosition(snapshot_, false, hz)) * plot.getWidth();
        g.drawText(hz >= 1000 ? String(hz / 1000.0, 0) + "k" : String(hz, 0),
            roundToInt(x) - 17, roundToInt(plot.getBottom()) + 3, 34, 15, Justification::centred);
    }
    const String summary = !settings_.enabled ? String("Calculation disabled") : !snapshot_.hasEstimate
        ? String(describe(snapshot_.status))
        : String(snapshot_.componentCount) + " components | " + String(snapshot_.extractionSteps)
            + " steps | fit " + String(100.0 * snapshot_.fitFraction, 2) + "% | KKT " + String(snapshot_.fitKktResidual, 8)
            + " | p=" + String(settings_.harmonicDecay, 1);
    g.drawFittedText(summary, 8, getHeight() - 78, getWidth() - 16, 17, Justification::centredLeft, 1);
    for (int i = 0; i < snapshot_.componentCount; ++i)
    {
        const auto& c = snapshot_.components[i];
        const int column = i % 2, row = i / 2, width = (getWidth() - 24) / 2;
        g.setColour(Colour(i == 0 ? selectedColour : rawColour));
        g.drawFittedText(String(c.extractionRank) + ": " + String(c.frequencyHz, 2) + " Hz | A="
            + String(c.fitAmplitude, 4) + " | w=" + String(c.wideningPercent, 0) + "%", 8 + column * (width + 8), getHeight() - 58 + row * 18,
            width, 17, Justification::centredLeft, 1);
    }
    g.setColour(Colour(text));
    g.drawFittedText(snapshot_.iterativeExtraction
        ? "Joint positive comb amplitudes; signed FFT error. Markers use final fit amplitudes."
        : "Legacy comparison: one family, independently refitted partials, adjustable cancellation.",
        8, getHeight() - 20, getWidth() - 16, 16, Justification::centredLeft, 1);
}
}
