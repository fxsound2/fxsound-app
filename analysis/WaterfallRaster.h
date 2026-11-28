// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "WaterfallFrame.h"
#include <optional>
#include <vector>

namespace fxanalysis
{
struct WaterfallStyle
{
    static constexpr bool shadingEnabled = true;
    static constexpr int liveStripHeight = 2; // Physical pixels, independent of scrolling cadence.
    static constexpr int liveSpectrumHeight = 46; // Additional height above the fixed strip.
    static constexpr int frequencyRadius = 2 * WaterfallSettings::horizontalScale;
    // Measure the local face closer to the centre of the thinner gold lobe.
    static constexpr int carrierFrequencyRadius = 4;
    static constexpr double spectrumExtent = 0.60, carrierExtent = 1.40;
    static constexpr double spectrumLightAmplitude = 0.70;
    static constexpr float frequencyRelief = 2.4f, timeRelief = 3.0f;
    // Reference gains before applying the spectrum's attenuation.
    static constexpr float highlight = 4.0f, shadow = 1.2f;
    // Shadow margin in weighted relief units; zero disables only this margin.
    static constexpr float shadowReferenceSlope = 0.30f;
    static_assert(shadowReferenceSlope >= 0.0f, "Shadow reference slope must be nonnegative");
    // Positive code slope at the reflection maximum; independent of the shadow margin.
    static constexpr float specularPeakSlope = 0.60f;
    static_assert(specularPeakSlope > 0.0f, "Specular peak slope must be positive");
    static constexpr double temporalSeconds = 0.012;
    static constexpr double spectrumShadowSeconds = 0.051, carrierShadowSeconds = 0.090;
    static constexpr float minimumLight = 0.08f;
    static constexpr float shadowSilenceThreshold = 1.0e-4f;
};

// Pure C++ pixel renderer. A bounded recent band of derived lighting inputs
// allows new top rows to cast shadows down-right onto older screen rows.
class WaterfallRaster final
{
public:
    using Levels = WaterfallProfile;
    using PixelRow = std::array<std::uint32_t, WaterfallSettings::width>;
    static_assert(WaterfallSettings::linesPerFrame == 2,
        "The raster emits exactly a midpoint and a current row per FFT");
    struct Rows
    {
        PixelRow current{}, midpoint{}; // Current goes above midpoint in the bitmap.
        bool reset = false;
    };
    // An overlay emits premultiplied ARGB, including transparent pixels and cast shadows.
    explicit WaterfallRaster(std::optional<std::uint32_t> overlayColour = std::nullopt);
    void reset() noexcept;
    bool process(const WaterfallFrame& frame, bool shading, Rows& rows) noexcept;
    Levels interpolate(const BandLevels& bands) const noexcept;
    // Latest accepted observation, shared with the live silhouette; no extra history.
    const Levels& currentLevels() const noexcept { return previous_; }
    const BandLevels& frequencies() const noexcept { return frequencies_; }
    double rowSeconds() const noexcept { return rowSeconds_; }
    // Apply this patch after scrolling the host bitmap by two rows. Row zero is newest.
    int correctedRowCount() const noexcept { return correctedCount_; }
    const PixelRow* correctedRows() const noexcept { return corrected_.data(); }
    // Cropping is irreversible, including after a later enlargement of the host bitmap.
    void cropHistory(int visibleRows) noexcept;
    static std::uint32_t colour(float level) noexcept;
    static constexpr std::uint32_t background = 0xff03050a;
private:
    struct Coefficient { int first = 0, second = 0; float weight = 0; };
    // Fixed tables, built once in the constructor.
    void buildHorizontalInterpolationTable();
    void buildShadedColourTable();
    void buildHighlightTable();
    // Frame decoding and state transitions; no FFT/band spectrum history is retained.
    bool readLevels(const WaterfallFrame& frame, Levels& current) const noexcept;
    bool requiresRestart(const WaterfallFrame& frame, bool shading, double hopSeconds) const noexcept;
    void startObservation(const WaterfallFrame& frame, const Levels& current, double hopSeconds) noexcept;
    static Levels interpolateTemporalMidpoint(const Levels& previous, const Levels& current) noexcept;
    void rememberObservation(const WaterfallFrame& frame, const Levels& current, bool shading) noexcept;
    // Local gradients use older observations below the current screen row.
    float updateReliefSlope(const Levels& levels, int x) noexcept;

    float highlightResponse(float slope) const noexcept;
    static float reliefLight(float localLight, float effectiveShadow, bool shading) noexcept;
    std::uint32_t lookupShadedColour(int paletteIndex, float light) const noexcept;
    struct RenderRow
    {
        Levels localLight{}, shadowSeed{};
        // 256 denotes exact silence; nonzero levels quantized to palette index 0 stay distinct.
        std::array<std::uint16_t, WaterfallSettings::width> palette{};
    };
    void renderRow(const Levels& levels, RenderRow& row) noexcept;
    void insertRow(const RenderRow& row) noexcept;
    void composeHistory(bool shading) noexcept;
    // A diagonal ray leaves the 300-column image after at most 299 row steps.
    // Include the second newly inserted row in the patch as well.
    static constexpr int maximumCorrectedRows = WaterfallSettings::width + 1;
    std::vector<RenderRow> renderHistory_;
    std::vector<PixelRow> corrected_;
    int historyHead_ = 0, retainedRenderRows_ = 0, correctedCount_ = 0;
    int historyLimit_ = maximumCorrectedRows;
    static constexpr int lightSteps = 64, maximumLight = 3;
    static constexpr int levelSteps = 256;
    static constexpr int lastLevel = levelSteps - 1;
    static constexpr double continuityToleranceHops = 0.01;
    std::array<Coefficient, WaterfallSettings::width> coefficients_{};
    const std::optional<std::uint32_t> overlayColour_;
    const double reliefExtent_;
    const int frequencyRadius_;
    const float frequencyRadiusFraction_;
    std::vector<std::uint32_t> table_;
    // Levels and their temporal envelope stay in [0,1], bounding the weighted slope.
    static constexpr float maximumReliefSlope = 0.5f * WaterfallStyle::frequencyRelief + WaterfallStyle::timeRelief;
    static constexpr int highlightIntervals = 1024;
    std::array<float, highlightIntervals + 1> highlights_{};
    Levels previous_{}, temporal_{};
    BandLevels frequencies_{};
    double lastTime_ = 0, sampleRate_ = 0, rowSeconds_ = 0;
    float temporalRetention_ = 0, shadowRetention_ = 0, carrierLevelRetention_ = 0;
    double displayTimingPercent_ = DisplayTiming::defaultPercent;
    int hopSize_ = 0, channels_ = 0;
    std::uint64_t generation_ = 0, revision_ = 0, sourceRevision_ = 0;
    bool haveFrame_ = false, haveLine_ = false, shading_ = true, hasProfile_ = false;
};
}
