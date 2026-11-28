// SPDX-License-Identifier: AGPL-3.0-or-later
#include "WaterfallWindow.h"
#include "../FrequencyScale.h"
#include <algorithm>
#include <cstring>
#include <exception>

namespace fxanalysis
{
using namespace juce;
namespace
{
struct HistoryLayout
{
    // Logical JUCE units. The raster dimensions remain physical pixels.
    static constexpr int windowWidth = 380;
    static constexpr int initialViewHeight = 640, initialWindowHeight = 660;
    static constexpr int minimumWindowHeight = 300, maximumWindowHeight = 1200;
    static constexpr int plotLeft = 48, liveTop = 26, footerHeight = 46;
    static constexpr int timeAxisDivisions = 4;
    static constexpr int statusLeft = 6, statusTopFromBottom = 43;
    static constexpr int scaleTopFromBottom = 23;
    static constexpr int frequencyLabelWidth = 30;
    static constexpr int frequencyLabelHalfWidth = frequencyLabelWidth / 2;
    static constexpr int frequencyLabelGap = 1;
    static constexpr int carrierButtonWidth = 108, carrierButtonHeight = 26, carrierButtonBottom = 12;
    static constexpr int timingTitleHeight = 18, timingSliderHeight = 23, timingReadoutHeight = 34;
    static constexpr int timingPanelHeight = timingTitleHeight + timingSliderHeight + timingReadoutHeight;
    static constexpr int percentBoxWidth = 70;
    static constexpr float timingReadoutFont = 13.0f;
    static constexpr double noAudioAfterMilliseconds = 400.0;
    static constexpr juce::uint32 background = 0xff101821, axisColour = 0xffb4c0ce;
};

String compactNumber(double value)
{
    return String(value, 2).trimCharactersAtEnd("0").trimCharactersAtEnd(".");
}

void resizeHistoryImage(Image& image, int height, Colour background)
{
    if (!image.isNull() && image.getHeight() == height) return;
    Image replacement(Image::ARGB, WaterfallSettings::width, height, true);
    if (!background.isTransparent()) replacement.clear(replacement.getBounds(), background);
    if (!image.isNull()) { Graphics g(replacement); g.drawImageAt(image, 0, 0); }
    image = replacement;
}

void insertHistoryRows(Image& image, const WaterfallRaster& raster, bool reset, Colour background)
{
    if (reset) image.clear(image.getBounds(), background);
    else image.moveImageSection(0, WaterfallSettings::linesPerFrame, 0, 0, WaterfallSettings::width,
                                image.getHeight() - WaterfallSettings::linesPerFrame);
    const int patchHeight = (std::min)(image.getHeight(), raster.correctedRowCount());
    Image::BitmapData pixels(image, 0, 0, WaterfallSettings::width, patchHeight, Image::BitmapData::writeOnly);
    const auto* corrected = raster.correctedRows();
    for (int y = 0; y < patchHeight; ++y)
    {
#if JUCE_ANDROID
        // This backend uses a different native channel order.
        for (int x = 0; x < WaterfallSettings::width; ++x)
        {
            const auto argb = corrected[y][x];
            const PixelARGB pixel(static_cast<uint8>(argb >> 24), static_cast<uint8>(argb >> 16),
                                 static_cast<uint8>(argb >> 8), static_cast<uint8>(argb));
            std::memcpy(pixels.getPixelPointer(x, y), &pixel, sizeof(pixel));
        }
#else
        // ARGB BitmapData uses this mask order and premultiplied alpha.
        static_assert(sizeof(PixelARGB) == sizeof(std::uint32_t));
        std::memcpy(pixels.getLinePointer(y), corrected[y].data(), sizeof(WaterfallRaster::PixelRow));
#endif
    }
}
}

WaterfallView::WaterfallView(const String& sourceLabel) : sourceLabel_(sourceLabel)
{
    setOpaque(true);
    batch_.reserve(WaterfallSettings::queueCapacity);
    setSize(HistoryLayout::windowWidth, HistoryLayout::initialViewHeight);
}
WaterfallView::~WaterfallView() { stopTimer(); }

void WaterfallView::setActive(bool active)
{
    if (active_ == active) return;
    active_ = active;
    if (active) startTimerHz(WaterfallSettings::refreshHz); else stopTimer();
    if (carrierRaster_ && onCarrierRequested_) onCarrierRequested_(active);
    repaint();
}

void WaterfallView::enableCarrierOverlay(std::uint32_t colour, std::function<void(bool)> onRequested)
{
    carrierColour_ = colour;
    onCarrierRequested_ = std::move(onRequested);
    carrierButton_.setButtonText(TRANS("Porteuses"));
    carrierButton_.setTooltip(TRANS("Afficher les porteuses en surimpression."));
    carrierButton_.setClickingTogglesState(true);
    carrierButton_.setColour(TextButton::buttonOnColourId, Colour(0xff664724));
    carrierButton_.setColour(TextButton::textColourOnId, Colour(colour));
    carrierButton_.onClick = [this] { setCarrierOverlayEnabled(carrierButton_.getToggleState()); };
    addAndMakeVisible(carrierButton_);
    resized();
}

void WaterfallView::setCarrierOverlayEnabled(bool enabled)
{
    if (!carrierColour_ || enabled == (carrierRaster_ != nullptr)) return;
    try
    {
        if (enabled) carrierRaster_ = std::make_unique<WaterfallRaster>(*carrierColour_);
        else
        {
            clearCarrierOverlay();
            carrierRaster_.reset();
        }
        carrierButton_.setToggleState(enabled, dontSendNotification);
        if (active_ && onCarrierRequested_) onCarrierRequested_(enabled);
        repaint();
    }
    catch (const std::exception& error)
    {
        carrierButton_.setToggleState(carrierRaster_ != nullptr, dontSendNotification);
        if (onFailure) onFailure(String("Cannot change carrier overlay: ") + error.what());
    }
}

void WaterfallView::enableDisplayTiming(double percent, std::function<void(double)> onChanged)
{
    const auto title = TRANS(String::fromUTF8(u8"R\u00e9ponse temporelle"));
    timingTitle_.setText(title + "    " + String(DisplayTiming::minimumPercent) + " % " + TRANS("attaques")
        + " / " + String(DisplayTiming::maximumPercent) + " % " + TRANS(String::fromUTF8(u8"liss\u00e9")), dontSendNotification);
    timingSlider_.setName(title);
    timingSlider_.setComponentID("displayTimingPercent");
    timingSlider_.setSliderStyle(Slider::LinearHorizontal);
    timingSlider_.setRange(DisplayTiming::minimumPercent, DisplayTiming::maximumPercent, DisplayTiming::stepPercent);
    timingSlider_.setTooltip(TRANS(String::fromUTF8(u8"Double-clic pour retrouver la r\u00e9ponse de r\u00e9f\u00e9rence"))
        + " (" + String(DisplayTiming::defaultPercent) + " %)");
    timingSlider_.setTextBoxStyle(Slider::TextBoxRight, false, HistoryLayout::percentBoxWidth, HistoryLayout::timingSliderHeight);
    timingSlider_.setTextValueSuffix(" %");
    timingSlider_.textFromValueFunction = compactNumber;
    timingSlider_.valueFromTextFunction = [](const String& text) { return text.replaceCharacter(',', '.').getDoubleValue(); };
    timingSlider_.setDoubleClickReturnValue(true, DisplayTiming::defaultPercent);
    timingSlider_.onValueChange = [this, changed = std::move(onChanged)]
    {
        const double percent = timingSlider_.getValue();
        setDisplayTimingPercent(percent);
        changed(percent);
    };
    addAndMakeVisible(timingTitle_);
    addAndMakeVisible(timingSlider_);
    for (auto* label : {&rearmDuration_, &attackDuration_, &peakDuration_, &carrierDuration_})
    {
#if JUCE_MAJOR_VERSION >= 8
        label->setFont(Font(FontOptions(HistoryLayout::timingReadoutFont).withMetricsKind(TypefaceMetricsKind::legacy).withName(Font::getDefaultSansSerifFontName())));
#else
        label->setFont(Font(HistoryLayout::timingReadoutFont));
#endif
        addAndMakeVisible(*label);
    }
    setDisplayTimingPercent(percent);
    resized();
}

void WaterfallView::setDisplayTimingPercent(double percent)
{
    timingSlider_.setValue(percent, dontSendNotification);
    const auto timing = DisplayTiming::atPercent(percent);
    const auto ms = [](double value) { return compactNumber(value) + " ms"; };
    rearmDuration_.setText(TRANS(String::fromUTF8(u8"R\u00e9armement")) + " " + ms(timing.levelReleaseMs), dontSendNotification);
    attackDuration_.setText(TRANS(String::fromUTF8(u8"D\u00e9croissance attaque")) + " " + ms(timing.attackDecayMs), dontSendNotification);
    peakDuration_.setText(TRANS("Arrondi du pic") + " " + ms(timing.peakReleaseMs), dontSendNotification);
    carrierDuration_.setText(TRANS("Lissage porteuses") + " " + ms(timing.carrierLevelMs), dontSendNotification);
}

int WaterfallView::historyBottom() const noexcept
{
    return getHeight() - (timingSlider_.isVisible() ? HistoryLayout::timingPanelHeight : 0);
}

void WaterfallView::resized()
{
    carrierButton_.setBounds(getWidth() - HistoryLayout::statusLeft - HistoryLayout::carrierButtonWidth,
        historyBottom() - HistoryLayout::carrierButtonBottom - HistoryLayout::carrierButtonHeight,
        HistoryLayout::carrierButtonWidth, HistoryLayout::carrierButtonHeight);
    auto panel = getLocalBounds().removeFromBottom(HistoryLayout::timingPanelHeight).reduced(HistoryLayout::statusLeft, 0);
    timingTitle_.setBounds(panel.removeFromTop(HistoryLayout::timingTitleHeight));
    timingSlider_.setBounds(panel.removeFromTop(HistoryLayout::timingSliderHeight));
    auto left = panel.removeFromLeft(panel.getWidth() / 2);
    const int rowHeight = HistoryLayout::timingReadoutHeight / 2;
    rearmDuration_.setBounds(left.removeFromTop(rowHeight));
    attackDuration_.setBounds(panel.removeFromTop(rowHeight));
    peakDuration_.setBounds(left);
    carrierDuration_.setBounds(panel);
}

float WaterfallView::displayScale() const noexcept
{
    const auto* peer = getPeer();
    return peer ? static_cast<float>(peer->getPlatformScaleFactor()) : 1.0f;
}

void WaterfallView::setMaximised(bool maximised)
{
    if (maximised == (maximisedPlotHeight_ > 0)) return;
    // Capture the normal raster height before the native window resizes its content.
    maximisedPlotHeight_ = maximised ? physicalPlotBounds(displayScale()).getHeight() : 0;
    if (!maximised) maximisedComposite_ = {};
    repaint();
}

void WaterfallView::ensureBitmapSize()
{
    const int height = physicalPlotBounds(displayScale()).getHeight();
    resizeHistoryImage(image_, height, Colour(WaterfallRaster::background));
    if (!carrierImage_.isNull())
    {
        resizeHistoryImage(carrierImage_, height, Colours::transparentBlack);
        carrierRaster_->cropHistory(height);
    }
    rows_ = (std::min)(rows_, height);
    raster_.cropHistory(height);
}

void WaterfallView::clearHistory()
{
    if (source_) dropped_ = source_(batch_);
    batch_.clear(); raster_.reset(); rows_ = 0; receivedAt_ = 0;
    sampleRate_ = 0.0; hoverPosition_.reset();
    clearCarrierOverlay();
    if (!image_.isNull()) image_.clear(image_.getBounds(), Colour(WaterfallRaster::background));
    failure_.clear(); repaint();
}

void WaterfallView::reportFailure(const String& message)
{
    setActive(false);
    failure_ = message;
    if (onFailure) onFailure(failure_);
    repaint();
}

void WaterfallView::offer(const WaterfallFrame& frame)
{
    if (!active_) return;
    try
    {
        const double start = Time::getMillisecondCounterHiRes();
        ensureBitmapSize();
        WaterfallRaster::Rows lines;
        if (!raster_.process(frame, WaterfallStyle::shadingEnabled, lines))
        {
            if (failure_.isEmpty())
            {
                failure_ = "Invalid spectrum history frame; frame skipped.";
                if (onFailure) onFailure(failure_);
            }
            return;
        }
        insertHistoryRows(image_, raster_, lines.reset, Colour(WaterfallRaster::background));
        sampleRate_ = frame.sampleRate;
        if (lines.reset) rows_ = 0;
        raster_.cropHistory(image_.getHeight());
        rows_ = (std::min)(rows_ + WaterfallSettings::linesPerFrame, image_.getHeight());
        updateCarrierOverlay(frame, lines.reset);
        ++received_; ++renders_; receivedAt_ = Time::getMillisecondCounterHiRes();
        renderMs_ = receivedAt_ - start;
    }
    catch (const std::exception& error) { reportFailure(String("Spectrum history stopped: ") + error.what()); }
}

void WaterfallView::updateCarrierOverlay(const WaterfallFrame& frame, bool reset)
{
    if (!carrierRaster_) return;
    if (frame.carrierCapture == 0)
    {
        if (!carrierImage_.isNull()) clearCarrierOverlay();
        return;
    }

    auto carriers = frame;
    carriers.hasProfile = true;
    if (reset) carrierRaster_->reset();
    WaterfallRaster::Rows lines;
    if (!carrierRaster_->process(carriers, WaterfallStyle::shadingEnabled, lines))
    {
        clearCarrierOverlay();
        if (failure_.isEmpty())
        {
            failure_ = "Invalid carrier overlay frame; frame skipped.";
            if (onFailure) onFailure(failure_);
        }
        return;
    }
    resizeHistoryImage(carrierImage_, image_.getHeight(), Colours::transparentBlack);
    insertHistoryRows(carrierImage_, *carrierRaster_, lines.reset, Colours::transparentBlack);
    carrierRaster_->cropHistory(carrierImage_.getHeight());
}

void WaterfallView::clearCarrierOverlay()
{
    carrierImage_ = {};
    if (carrierRaster_) carrierRaster_->reset();
}

void WaterfallView::refreshNow()
{
    try
    {
        ensureBitmapSize();
        if (active_ && source_)
        {
            dropped_ = source_(batch_);
            for (const auto& frame : batch_) offer(frame);
            batch_.clear();
        }
        repaint();
    }
    catch (const std::exception& error) { reportFailure(String("Spectrum history stopped: ") + error.what()); }
}

Rectangle<int> WaterfallView::physicalLiveBounds(float scale) const noexcept
{
    return {roundToInt(HistoryLayout::plotLeft * scale), roundToInt(HistoryLayout::liveTop * scale),
            WaterfallSettings::width, WaterfallStyle::liveStripHeight + WaterfallStyle::liveSpectrumHeight};
}

Rectangle<int> WaterfallView::physicalPlotBounds(float scale) const noexcept
{
    const auto live = physicalLiveBounds(scale);
    const int top = live.getBottom();
    const int height = maximisedPlotHeight_ > 0 ? maximisedPlotHeight_
        : (std::max)(WaterfallSettings::linesPerFrame, roundToInt((historyBottom() - HistoryLayout::footerHeight) * scale) - top);
    return {live.getX(), top, WaterfallSettings::width, height};
}

AffineTransform WaterfallView::chartTransform(Rectangle<float> plot) const noexcept
{
    if (maximisedPlotHeight_ == 0 || getWidth() <= 0 || historyBottom() <= HistoryLayout::footerHeight)
        return {};

    // Include the axes in the fit and leave the footer at its standard size.
    const float chartWidth = plot.getRight() + HistoryLayout::statusLeft;
    const float chartHeight = plot.getBottom();
    const float availableHeight = static_cast<float>(historyBottom() - HistoryLayout::footerHeight);
    const float factor = (std::min)(getWidth() / chartWidth, availableHeight / chartHeight);
    return AffineTransform::scale(factor).translated((getWidth() - chartWidth * factor) * 0.5f,
                                                    (availableHeight - chartHeight * factor) * 0.5f);
}

Rectangle<float> WaterfallView::displayedPlotBounds(float scale) const noexcept
{
    const auto plot = physicalPlotBounds(scale).toFloat() / scale;
    return plot.transformedBy(chartTransform(plot));
}

void WaterfallView::paint(Graphics& g)
{
    g.fillAll(Colour(HistoryLayout::background));
    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    const auto physicalPlot = physicalPlotBounds(scale);
    const auto physicalLive = physicalLiveBounds(scale);
    const auto plot = physicalPlot.toFloat() / scale;
    const auto& visible = rasterImage();
    const double rowSeconds = raster_.rowSeconds();
    {
        Graphics::ScopedSaveState chart(g);
        g.addTransform(chartTransform(plot));
        g.setImageResamplingQuality(maximisedPlotHeight_ > 0
            ? Graphics::mediumResamplingQuality : Graphics::lowResamplingQuality);
        drawHistoryAndSilhouette(g, visible, physicalLive, physicalPlot, scale);
        g.setColour(Colour(HistoryLayout::axisColour)); g.setFont(12.0f);
        drawTimeAxis(g, physicalPlot, plot, scale, rowSeconds);
        drawFrequencyAxis(g, plot, scale);
    }
    g.setFont(12.0f);
    drawHistoryStatus(g, rowSeconds);
    drawFrequencyReadout(g, scale);
}

void WaterfallView::drawHistoryAndSilhouette(Graphics& g, const Image& visible,
    Rectangle<int> physicalLive, Rectangle<int> physicalPlot, float scale)
{
    // Cancel the DPI scale so source pixels map one-to-one in windowed mode.
    // The chart transform, if active, scales the same source on both axes.
    {
        Graphics::ScopedSaveState state(g);
        g.addTransform(AffineTransform::scale(1.0f / scale));
        g.setColour(Colour(WaterfallRaster::background));
        g.fillRect(physicalLive);
        g.fillRect(physicalPlot);
        if (!visible.isNull())
        {
            // Extend the latest row upward: a fixed strip keeps quiet components
            // visible, and the silhouette shows the same normalized visual level.
            // Reuse its shaded color exactly, including at the history junction.
            const Image::BitmapData pixels(visible, 0, 0, WaterfallSettings::width, 1);
            const auto& levels = raster_.currentLevels();
            for (int x = 0; x < WaterfallSettings::width; ++x)
            {
                const int height = WaterfallStyle::liveStripHeight
                    + roundToInt(WaterfallStyle::liveSpectrumHeight * levels[x]);
                g.setColour(pixels.getPixelColour(x, 0));
                g.fillRect(physicalLive.getX() + x, physicalLive.getBottom() - height, 1, height);
            }
            if (!carrierImage_.isNull())
            {
                const Image::BitmapData carrierPixels(carrierImage_, 0, 0, WaterfallSettings::width, 1);
                const auto& carrierLevels = carrierRaster_->currentLevels();
                for (int x = 0; x < WaterfallSettings::width; ++x)
                {
                    const auto pixel = carrierPixels.getPixelColour(x, 0);
                    if (pixel.isTransparent()) continue;
                    const int height = WaterfallStyle::liveStripHeight
                        + roundToInt(WaterfallStyle::liveSpectrumHeight * carrierLevels[x]);
                    g.setColour(pixel);
                    g.fillRect(physicalLive.getX() + x, physicalLive.getBottom() - height, 1, height);
                }
            }
            Graphics::ScopedSaveState plotState(g);
            g.reduceClipRegion(physicalPlot);
            // The last live column may have supplied a translucent brush.
            g.setOpacity(1.0f);
            if (maximisedPlotHeight_ > 0 && !carrierImage_.isNull())
            {
                // Blend at raster size and resample the result once for the display.
                resizeHistoryImage(maximisedComposite_, visible.getHeight(), Colour(WaterfallRaster::background));
                {
                    Graphics composite(maximisedComposite_);
                    composite.drawImageAt(visible, 0, 0);
                    composite.drawImageAt(carrierImage_, 0, 0);
                }
                g.drawImageAt(maximisedComposite_, physicalPlot.getX(), physicalPlot.getY());
            }
            else
            {
                g.drawImageAt(visible, physicalPlot.getX(), physicalPlot.getY());
                if (!carrierImage_.isNull())
                    g.drawImageAt(carrierImage_, physicalPlot.getX(), physicalPlot.getY());
            }
        }
    }
}

void WaterfallView::drawTimeAxis(Graphics& g, Rectangle<int> physicalPlot,
    Rectangle<float> plot, float scale, double rowSeconds)
{
    for (int tick = 0; tick <= HistoryLayout::timeAxisDivisions; ++tick)
    {
        const int row = tick * physicalPlot.getHeight() / HistoryLayout::timeAxisDivisions;
        const double age = row * rowSeconds;
        const int y = roundToInt(plot.getY() + row / scale);
        g.drawText(tick ? "-" + String(age, 1) + "s" : "Now", 2, y - 8, 40, 16, Justification::centredRight);
    }
}

void WaterfallView::drawFrequencyAxis(Graphics& g, Rectangle<float> plot, float)
{
    if (sampleRate_ <= 0.0) return;
    const double maximumHz = (std::min)(FrequencySettings::maximumHz, sampleRate_ * 0.5);
    const FrequencyScale mapping(maximumHz);
    int previousRight = -100;
    for (const double frequency : {50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0})
    {
        if (frequency < FrequencySettings::minimumHz || frequency > maximumHz) continue;
        const int x = roundToInt(plot.getX() + mapping.position(frequency) * plot.getWidth());
        if (x - HistoryLayout::frequencyLabelHalfWidth < previousRight) continue;
        g.drawText(frequency >= 1000 ? String(frequency / 1000, 0) + "k" : String(frequency, 0),
                   x - HistoryLayout::frequencyLabelHalfWidth, 5, HistoryLayout::frequencyLabelWidth, 18, Justification::centred);
        previousRight = x + HistoryLayout::frequencyLabelHalfWidth + HistoryLayout::frequencyLabelGap;
    }
}

void WaterfallView::mouseMove(const MouseEvent& event)
{
    const auto plot = displayedPlotBounds(displayScale());
    if (sampleRate_ > 0.0 && plot.contains(event.position))
        hoverPosition_ = jlimit(0.0, 1.0, (event.position.x - plot.getX()) / static_cast<double>(plot.getWidth()));
    else hoverPosition_.reset();
    repaint();
}

void WaterfallView::mouseExit(const MouseEvent&)
{
    hoverPosition_.reset();
    repaint();
}

void WaterfallView::drawFrequencyReadout(Graphics& g, float scale)
{
    if (!hoverPosition_ || sampleRate_ <= 0.0) return;
    const auto plot = displayedPlotBounds(scale);
    const FrequencyScale mapping((std::min)(FrequencySettings::maximumHz, sampleRate_ * 0.5));
    const double hz = mapping.frequency(*hoverPosition_);
    const float x = plot.getX() + static_cast<float>(*hoverPosition_) * plot.getWidth();
    g.setColour(Colour(HistoryLayout::axisColour).withAlpha(0.7f));
    g.drawVerticalLine(roundToInt(x), plot.getY(), plot.getBottom());
    auto label = Rectangle<float>(jlimit(plot.getX(), (std::max)(plot.getX(), plot.getRight() - 140.0f), x + 5.0f),
        plot.getY() + 5.0f, 140.0f, 20.0f);
    g.setColour(Colour(HistoryLayout::background).withAlpha(0.9f));
    g.fillRoundedRectangle(label, 3.0f);
    g.setColour(Colour(HistoryLayout::axisColour));
    g.setFont(12.0f);
    g.drawText(TRANS("Frequency") + ": " + String(hz, hz < 1000.0 ? 1 : 0) + " Hz", label, Justification::centred);
}

void WaterfallView::drawHistoryStatus(Graphics& g, double rowSeconds)
{
    g.setColour(Colour(HistoryLayout::axisColour));
    String status = !active_ ? "Stopped" : receivedAt_ == 0 ? "Waiting for audio" :
        Time::getMillisecondCounterHiRes() - receivedAt_ > HistoryLayout::noAudioAfterMilliseconds ? "No new audio" : "Live";
    if (dropped_) status += " | lost: " + String(static_cast<int64>(dropped_));
    if (failure_.isNotEmpty()) status = failure_;
    const int textRight = carrierColour_ ? carrierButton_.getX() : getWidth();
    const int textWidth = textRight - 2 * HistoryLayout::statusLeft;
    g.drawFittedText(status, HistoryLayout::statusLeft, historyBottom() - HistoryLayout::statusTopFromBottom, textWidth, 20, Justification::centredLeft, 1);
    const auto timeUnit = maximisedPlotHeight_ > 0 ? " ms/row" : " ms/px";
    g.drawText(String(WaterfallSettings::width) + " px | " + sourceLabel_ + " x" + String(WaterfallSettings::linesPerFrame) + " | " + String(rowSeconds * DisplayTiming::millisecondsPerSecond, 2) + timeUnit, HistoryLayout::statusLeft, historyBottom() - HistoryLayout::scaleTopFromBottom,
               textWidth, 18, Justification::centredLeft);
}

WaterfallWindow::WaterfallWindow(const String& title, const String& sourceLabel)
    : DocumentWindow(title, Colour(HistoryLayout::background), DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    view_ = new WaterfallView(sourceLabel);
    setContentOwned(view_, true);
    setResizable(true, false); setResizeLimits(HistoryLayout::windowWidth, HistoryLayout::minimumWindowHeight,
                                               HistoryLayout::windowWidth, HistoryLayout::maximumWindowHeight);
    centreWithSize(HistoryLayout::windowWidth, HistoryLayout::initialWindowHeight);
}
WaterfallWindow::~WaterfallWindow()
{
    onVisibilityChanged = {}; onClosed = {};
    view_->setActive(false); view_->setSource({});
    view_ = nullptr;
    clearContentComponent();
}
void WaterfallWindow::show()
{
    if (!isVisible()) view_->clearHistory();
    view_->setActive(true); setVisible(true); toFront(true);
}
void WaterfallWindow::resized()
{
    if (view_) view_->setMaximised(isFullScreen());
    DocumentWindow::resized();
}

void WaterfallWindow::closeButtonPressed()
{
    view_->setActive(false); setVisible(false);
    if (onClosed) onClosed();
}
void WaterfallWindow::visibilityChanged()
{
    DocumentWindow::visibilityChanged();
    if (view_ && !isShowing()) view_->setActive(false);
    if (onVisibilityChanged) onVisibilityChanged();
}
void WaterfallWindow::parentHierarchyChanged()
{
    DocumentWindow::parentHierarchyChanged();
    if (view_ && !isShowing()) view_->setActive(false);
    if (onVisibilityChanged) onVisibilityChanged();
}
}
