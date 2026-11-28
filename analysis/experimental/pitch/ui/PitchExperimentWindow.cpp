// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PitchExperimentWindow.h"
#include <algorithm>
#include <cmath>

namespace fxanalysis::experimental
{
using namespace juce;
namespace
{
constexpr juce::uint32 backgroundColour = 0xff101821;
constexpr juce::uint32 textColour = 0xffc3d0dc;
constexpr juce::uint32 buttonColour = 0xff303f4e;
constexpr juce::uint32 buttonOnColour = 0xff286d79;

class ExperimentTabs final : public TabbedComponent
{
public:
    ExperimentTabs() : TabbedComponent(TabbedButtonBar::TabsAtTop) {}
    std::function<void(int)> onViewChanged;
    void currentTabChanged(int index, const String&) override
    {
        if (onViewChanged) onViewChanged(index);
    }
};
}

class PitchExperimentContent final : public Component
{
public:
    explicit PitchExperimentContent(PitchExperimentWindow& owner) : owner_(owner)
    {
        setOpaque(true);
        tabs_.setTabBarDepth(24);
        tabs_.setOutline(0);
        tabs_.addTab("Transport histogram", Colour(backgroundColour), &indicator_, false);
        tabs_.addTab("Spectral correlation", Colour(backgroundColour), &correlation_, false);
        tabs_.addTab("Harmonic carriers", Colour(backgroundColour), &harmonic_, false);
        harmonic_.onSettingsChanged = [this](const HarmonicCombSettings& settings)
        {
            settings_.harmonicComb = settings;
            settingsChanged();
        };
        harmonic_.onStripModeChanged = [this](CompactIndicatorMode mode)
        {
            settings_.harmonicComb.showInStrip = mode == CompactIndicatorMode::carriers;
            if (owner_.onCompactIndicatorChanged) owner_.onCompactIndicatorChanged(mode);
        };
        addAndMakeVisible(tabs_);
        configureButton(smoothed_);
        smoothed_.setClickingTogglesState(true);
        smoothed_.setToggleState(true, dontSendNotification);
        smoothed_.setTooltip("Switch between the current transport histogram and its time-smoothed distribution. Correlation curves are instantaneous.");
        smoothed_.onClick = [this]
        {
            indicator_.setShowSmoothed(smoothed_.getToggleState());
            smoothed_.setButtonText(smoothed_.getToggleState() ? "Smoothed histogram" : "Instantaneous histogram");
        };
        configureButton(weighting_);
        weighting_.setTooltip("Switch shared peak conditioning between baseline (floor 0, prominence 0 dB, exponent 1) and dominant peaks (floor 1, prominence 6 dB, exponent 2).");
        weighting_.onClick = [this]
        {
            const bool chooseDominant = isBaseline();
            auto next = settings_;
            next.noiseFloorStrength = chooseDominant ? 1.0 : 0.0;
            next.minimumPeakProminenceDb = chooseDominant ? 6.0 : 0.0;
            next.peakWeightExponent = chooseDominant ? 2.0 : 1.0;
            setSettings(next);
            settingsChanged();
        };
        configureSlider(smoothing_, 0.0, 2.0, 0.01, " s");
        configureSlider(integration_, 0.05, 5.0, 0.05, " s");
        configureSlider(scale_, 0.5, 12.0, 0.25, " st");
        configureSlider(floor_, 0.0, 2.0, 0.1, "");
        configureSlider(prominence_, 0.0, 18.0, 0.5, " dB");
        configureSlider(exponent_, 1.0, 2.0, 0.1, "");
        configureLabel(smoothingLabel_, "Histogram memory");
        configureLabel(integrationLabel_, "Integral memory");
        configureLabel(scaleLabel_, "Transport scale");
        configureLabel(floorLabel_, "Background floor strength");
        configureLabel(prominenceLabel_, "Peak prominence [dB]");
        configureLabel(exponentLabel_, "Peak power exponent");
        smoothing_.setTooltip("Time constant for the transport histogram. Zero displays its instantaneous distribution.");
        integration_.setTooltip("Memory of the secondary movement integrals. Comparison lags remain independent.");
        scale_.setTooltip("Transport distance scale in semitones. This changes the optimal-transport model only.");
        floor_.setTooltip("Shared strength of spectral-background subtraction before peak weights are formed.");
        prominence_.setTooltip("Shared minimum spectral-peak prominence above its background, in dB.");
        exponent_.setTooltip("Shared power exponent. One preserves energy weighting; two emphasizes dominant peaks. Correlation applies half this exponent to its power cells before L2 normalization.");
        smoothing_.onValueChange = [this] { settings_.smoothingSeconds = smoothing_.getValue(); settingsChanged(); };
        integration_.onValueChange = [this] { settings_.integrationSeconds = integration_.getValue(); settingsChanged(); };
        scale_.onValueChange = [this] { settings_.transportScaleSemitones = scale_.getValue(); settingsChanged(); };
        floor_.onValueChange = [this] { settings_.noiseFloorStrength = floor_.getValue(); settingsChanged(); };
        prominence_.onValueChange = [this] { settings_.minimumPeakProminenceDb = prominence_.getValue(); settingsChanged(); };
        exponent_.onValueChange = [this] { settings_.peakWeightExponent = exponent_.getValue(); settingsChanged(); };
        tabs_.onViewChanged = [this](int index)
        {
            const bool transport = index == 0;
            const bool harmonic = index == 2;
            const std::array<Component*, 9> movementControls{{&smoothed_, &smoothing_, &smoothingLabel_,
                &integration_, &integrationLabel_, &scale_, &scaleLabel_, &exponent_, &exponentLabel_}};
            for (auto* component : movementControls) component->setVisible(!harmonic);
            resized();
            smoothed_.setEnabled(transport);
            smoothing_.setEnabled(transport);
            smoothingLabel_.setEnabled(transport);
            scale_.setEnabled(transport);
            scaleLabel_.setEnabled(transport);
            integration_.setEnabled(index != 2);
            integrationLabel_.setEnabled(index != 2);
            exponent_.setEnabled(index != 2);
            exponentLabel_.setEnabled(index != 2);
        };
        setSettings(settings_);
        setSize(780, 550);
    }
    void setSettings(const PitchMotionSettings& settings)
    {
        settings_ = settings;
        indicator_.setTrajectoryLimitSemitones(settings_.integratorLimitSemitones);
        correlation_.setTrajectoryLimitSemitones(settings_.integratorLimitSemitones);
        harmonic_.setSettings(settings_.harmonicComb);
        smoothing_.setValue(settings_.smoothingSeconds, dontSendNotification);
        integration_.setValue(settings_.integrationSeconds, dontSendNotification);
        scale_.setValue(settings_.transportScaleSemitones, dontSendNotification);
        floor_.setValue(settings_.noiseFloorStrength, dontSendNotification);
        prominence_.setValue(settings_.minimumPeakProminenceDb, dontSendNotification);
        exponent_.setValue(settings_.peakWeightExponent, dontSendNotification);
        refreshWeightingButton();
    }
    void clear()
    {
        indicator_.clear();
        correlation_.clear();
        harmonic_.clear();
    }
    const PitchMotionSettings& settings() const noexcept { return settings_; }
    PitchMotionIndicator& indicator() noexcept { return indicator_; }
    SpectralCorrelationIndicator& correlationIndicator() noexcept { return correlation_; }
    HarmonicCombIndicator& harmonicIndicator() noexcept { return harmonic_; }
    void setCorrelationView(bool correlation) { tabs_.setCurrentTabIndex(correlation ? 1 : 0); }
    void setHarmonicView() { tabs_.setCurrentTabIndex(2); }
    void paint(Graphics& g) override
    {
        g.fillAll(Colour(backgroundColour));
        g.setColour(Colour(textColour));
        g.setFont(16.0f);
        g.drawText("Spectral experiments - movement and harmonic carriers", 12, 4, getWidth() - 24, 24, Justification::centredLeft);
    }
    void resized() override
    {
        constexpr int margin = 12, columnGap = 12;
        const int columnWidth = (std::max)(1, (getWidth() - 2 * margin - 2 * columnGap) / 3);
        const int halfWidth = (std::max)(1, (getWidth() - 3 * margin) / 2);
        smoothed_.setBounds(margin, 32, halfWidth, 23);
        weighting_.setBounds(2 * margin + halfWidth, 32, halfWidth, 23);
        placeSlider(smoothingLabel_, smoothing_, margin, 60, columnWidth);
        placeSlider(integrationLabel_, integration_, margin + columnWidth + columnGap, 60, columnWidth);
        placeSlider(scaleLabel_, scale_, margin + 2 * (columnWidth + columnGap), 60, columnWidth);
        placeSlider(floorLabel_, floor_, margin, 101, columnWidth);
        placeSlider(prominenceLabel_, prominence_, margin + columnWidth + columnGap, 101, columnWidth);
        placeSlider(exponentLabel_, exponent_, margin + 2 * (columnWidth + columnGap), 101, columnWidth);
        if (tabs_.getCurrentTabIndex() == 2)
        {
            weighting_.setBounds(margin, 32, getWidth() - 2 * margin, 23);
            placeSlider(floorLabel_, floor_, margin, 60, halfWidth);
            placeSlider(prominenceLabel_, prominence_, 2 * margin + halfWidth, 60, halfWidth);
        }
        const int top = tabs_.getCurrentTabIndex() == 2 ? 106 : 146;
        tabs_.setBounds(margin, top, (std::max)(1, getWidth() - 2 * margin), (std::max)(1, getHeight() - top - 8));
    }
private:
    void configureButton(TextButton& button)
    {
        button.setColour(TextButton::buttonColourId, Colour(buttonColour));
        button.setColour(TextButton::buttonOnColourId, Colour(buttonOnColour));
        button.setColour(TextButton::textColourOffId, Colour(textColour));
        button.setColour(TextButton::textColourOnId, Colours::white);
        addAndMakeVisible(button);
    }
    void configureSlider(Slider& slider, double minimum, double maximum, double interval, const String& suffix)
    {
        slider.setSliderStyle(Slider::LinearHorizontal);
        slider.setTextBoxStyle(Slider::TextBoxRight, false, 66, 23);
        slider.setRange(minimum, maximum, interval);
        slider.setTextValueSuffix(suffix);
        slider.setNumDecimalPlacesToDisplay(2);
        slider.setColour(Slider::textBoxTextColourId, Colour(textColour));
        slider.setColour(Slider::textBoxBackgroundColourId, Colour(buttonColour));
        slider.setColour(Slider::textBoxOutlineColourId, Colour(buttonColour));
        addAndMakeVisible(slider);
    }
    void configureLabel(Label& label, const String& text)
    {
        label.setText(text, dontSendNotification);
        label.setColour(Label::textColourId, Colour(textColour));
#if JUCE_MAJOR_VERSION >= 8
        label.setFont(Font(FontOptions(10.5f).withMetricsKind(TypefaceMetricsKind::legacy).withName(Font::getDefaultSansSerifFontName())));
#else
        label.setFont(Font(10.5f));
#endif
        addAndMakeVisible(label);
    }
    static void placeSlider(Label& label, Slider& slider, int x, int y, int width)
    {
        label.setBounds(x, y, width, 14);
        slider.setBounds(x, y + 14, width, 24);
    }
    bool isBaseline() const noexcept
    {
        return std::abs(settings_.noiseFloorStrength) < 1.0e-9
            && std::abs(settings_.minimumPeakProminenceDb) < 1.0e-9
            && std::abs(settings_.peakWeightExponent - 1.0) < 1.0e-9;
    }
    void refreshWeightingButton()
    {
        const bool dominant = std::abs(settings_.noiseFloorStrength - 1.0) < 1.0e-9
            && std::abs(settings_.minimumPeakProminenceDb - 6.0) < 1.0e-9
            && std::abs(settings_.peakWeightExponent - 2.0) < 1.0e-9;
        weighting_.setButtonText(isBaseline() ? "Baseline (click for dominant)"
            : dominant ? "Dominant (click for baseline)" : "Custom (click for baseline)");
    }
    void settingsChanged()
    {
        refreshWeightingButton();
        clear();
        if (owner_.onSettingsChanged) owner_.onSettingsChanged(settings_);
    }
    PitchExperimentWindow& owner_;
    PitchMotionSettings settings_;
    PitchMotionIndicator indicator_;
    SpectralCorrelationIndicator correlation_;
    HarmonicCombIndicator harmonic_;
    ExperimentTabs tabs_;
    TextButton smoothed_{"Smoothed histogram"}, weighting_;
    Slider smoothing_, integration_, scale_, floor_, prominence_, exponent_;
    Label smoothingLabel_, integrationLabel_, scaleLabel_, floorLabel_, prominenceLabel_, exponentLabel_;
};

PitchExperimentWindow::PitchExperimentWindow(const String& title)
    : DocumentWindow(title, Colour(backgroundColour), DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    content_ = new PitchExperimentContent(*this);
    setContentOwned(content_, true);
    setResizable(true, false);
    setResizeLimits(640, 480, 1400, 1000);
    centreWithSize(1000, 800);
}
PitchExperimentWindow::~PitchExperimentWindow()
{
    onSettingsChanged = {};
    onClosed = {};
    onVisibilityChanged = {};
    onCompactIndicatorChanged = {};
    clearContentComponent();
}
void PitchExperimentWindow::setSnapshot(const PitchMotionSnapshot& snapshot) { content_->indicator().setSnapshot(snapshot); }
void PitchExperimentWindow::setCorrelationSnapshot(const SpectralCorrelationSnapshot& snapshot)
{
    content_->correlationIndicator().setSnapshot(snapshot);
}
void PitchExperimentWindow::setHarmonicSnapshot(const HarmonicCombSnapshot& snapshot) { content_->harmonicIndicator().setSnapshot(snapshot); }
void PitchExperimentWindow::clear() { content_->clear(); }
void PitchExperimentWindow::setSettings(const PitchMotionSettings& settings) { content_->setSettings(settings); }
void PitchExperimentWindow::setCompactIndicatorMode(CompactIndicatorMode mode) { content_->harmonicIndicator().setStripMode(mode); }
const PitchMotionSettings& PitchExperimentWindow::settings() const noexcept { return content_->settings(); }
PitchMotionIndicator& PitchExperimentWindow::indicator() noexcept { return content_->indicator(); }
SpectralCorrelationIndicator& PitchExperimentWindow::correlationIndicator() noexcept { return content_->correlationIndicator(); }
HarmonicCombIndicator& PitchExperimentWindow::harmonicIndicator() noexcept { return content_->harmonicIndicator(); }
void PitchExperimentWindow::setCorrelationView(bool correlation) { content_->setCorrelationView(correlation); }
void PitchExperimentWindow::setHarmonicView() { content_->setHarmonicView(); }
void PitchExperimentWindow::show()
{
    setVisible(true);
    toFront(true);
}
void PitchExperimentWindow::closeButtonPressed()
{
    setVisible(false);
    if (onClosed) onClosed();
}
void PitchExperimentWindow::visibilityChanged()
{
    DocumentWindow::visibilityChanged();
    if (onVisibilityChanged) onVisibilityChanged();
}
void PitchExperimentWindow::parentHierarchyChanged()
{
    DocumentWindow::parentHierarchyChanged();
    if (onVisibilityChanged) onVisibilityChanged();
}
}
