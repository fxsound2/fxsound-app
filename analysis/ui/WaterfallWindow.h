// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
// Projucer hosts supply their generated configuration; the lab sets it globally.
#if defined(JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED) || defined(JUCE_APP_CONFIG_HEADER)
#include <juce_gui_basics/juce_gui_basics.h>
#else
#include <JuceHeader.h>
#endif
#include "../WaterfallRaster.h"
#include <functional>

namespace fxanalysis
{
class WaterfallView final : public juce::Component, private juce::Timer
{
public:
    using Source = std::function<std::uint64_t(std::vector<WaterfallFrame>&)>;
    explicit WaterfallView(const juce::String& sourceLabel = "FFT");
    ~WaterfallView() override;
    void setSource(Source source) { source_ = std::move(source); }
    void setActive(bool active);
    void setMaximised(bool maximised);
    void enableCarrierOverlay(std::uint32_t colour, std::function<void(bool)> onRequested);
    void setCarrierOverlayEnabled(bool enabled);
    void enableDisplayTiming(double percent, std::function<void(double)> onChanged);
    void setDisplayTimingPercent(double percent);
    void offer(const WaterfallFrame& frame); // UI thread, also used by the WAV host.
    void clearHistory(); // Lifecycle/WAV reset; no manual reset control.
    void refreshNow();
    double lastRenderMilliseconds() const noexcept { return renderMs_; }
    std::uint64_t renderCount() const noexcept { return renders_; }
    std::uint64_t receivedFrames() const noexcept { return received_; }
    int retainedRows() const noexcept { return rows_; }
    const juce::Image& rasterImage() const noexcept { return image_; }
    juce::Rectangle<int> physicalLiveBounds(float scale) const noexcept;
    juce::Rectangle<int> physicalPlotBounds(float scale) const noexcept;
    juce::Rectangle<float> displayedPlotBounds(float scale) const noexcept;
    std::function<void(const juce::String&)> onFailure;
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
private:
    void timerCallback() override { refreshNow(); }
    float displayScale() const noexcept;
    juce::AffineTransform chartTransform(juce::Rectangle<float> plot) const noexcept;
    void ensureBitmapSize();
    void updateCarrierOverlay(const WaterfallFrame& frame, bool reset);
    void clearCarrierOverlay();
    void drawHistoryAndSilhouette(juce::Graphics& g, const juce::Image& visible,
        juce::Rectangle<int> physicalLive, juce::Rectangle<int> physicalPlot, float scale);
    void drawTimeAxis(juce::Graphics& g, juce::Rectangle<int> physicalPlot,
        juce::Rectangle<float> plot, float scale, double rowSeconds);
    void drawFrequencyAxis(juce::Graphics& g, juce::Rectangle<float> plot, float scale);
    void drawHistoryStatus(juce::Graphics& g, double rowSeconds);
    void drawFrequencyReadout(juce::Graphics& g, float scale);
    int historyBottom() const noexcept;
    void reportFailure(const juce::String& message);
    Source source_;
    std::vector<WaterfallFrame> batch_;
    WaterfallRaster raster_;
    juce::Image image_, carrierImage_, maximisedComposite_;
    juce::TextButton carrierButton_;
    juce::Slider timingSlider_;
    juce::Label timingTitle_;
    juce::Label rearmDuration_, attackDuration_, peakDuration_, carrierDuration_;
    std::optional<std::uint32_t> carrierColour_;
    std::unique_ptr<WaterfallRaster> carrierRaster_;
    std::function<void(bool)> onCarrierRequested_;
    int rows_ = 0, maximisedPlotHeight_ = 0;
    bool active_ = false;
    std::uint64_t received_ = 0, dropped_ = 0, renders_ = 0;
    double receivedAt_ = 0.0, renderMs_ = 0.0;
    juce::String failure_, sourceLabel_;
    double sampleRate_ = 0.0;
    std::optional<double> hoverPosition_;
};

// The host owns this window. Closing hides it and releases the host's capture request.
class WaterfallWindow final : public juce::DocumentWindow
{
public:
    explicit WaterfallWindow(const juce::String& title = "FxSound - Spectrum history",
        const juce::String& sourceLabel = "FFT");
    ~WaterfallWindow() override;
    WaterfallView& view() noexcept { return *view_; }
    void show();
    void resized() override;
    void closeButtonPressed() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;
    std::function<void()> onClosed;
    std::function<void()> onVisibilityChanged;
private:
    WaterfallView* view_ = nullptr;
};
}
