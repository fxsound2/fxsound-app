// SPDX-License-Identifier: AGPL-3.0-or-later
// Offscreen production widgets only. No FxSound host, profile, routing or device.
#include <juce_gui_basics/juce_gui_basics.h>
#include "analysis/ui/WaterfallWindow.h"
#include "analysis/experimental/pitch/ui/PitchMotionIndicator.h"
#include "analysis/experimental/pitch/ui/SpectralCorrelationIndicator.h"
#include "analysis/experimental/pitch/ui/HarmonicCombIndicator.h"
#include "analysis/FrequencyScale.h"
#include <cmath>
#include <cstdio>
#include <memory>
#include <set>
#include <vector>
#include <atomic>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

namespace
{
using namespace juce;
using namespace fxanalysis;
using namespace fxanalysis::experimental;
int checks = 0, failures = 0;
std::atomic<int> unexpectedAssertions{0};
Array<var> exportedFiles;

// This process is a noninteractive test host. Keep diagnostic text observable
// by the runner; never change the application's interactive error policy.
class TestLogger final : public Logger
{
public:
    bool verifyAssertionCapture(const File& log)
    {
        expectedLog_ = log.createOutputStream();
        if (!expectedLog_ || !expectedLog_->setPosition(0) || expectedLog_->truncate().failed())
        {
            expectedLog_.reset();
            return false;
        }
        expectedAssertions_ = 0;
        // The genuine core entry does not trigger the debugger-break part of
        // jassert. The separately compiled core must forward it to this logger.
        juce::logAssertion(__FILE__, __LINE__);
        expectedLog_->flush();
        const bool received = expectedAssertions_ == 1 && expectedLog_->getStatus().wasOk();
        expectedLog_.reset();
        expectedAssertions_ = 0;
        return received;
    }

    void logMessage(const String& message) override
    {
        const bool assertion = message.containsIgnoreCase("assertion failure");
        if (assertion && expectedLog_)
        {
            ++expectedAssertions_;
            *expectedLog_ << "EXPECTED_JUCE_ASSERT " << message << "\n";
            return;
        }
        if (assertion) ++unexpectedAssertions;
        std::fprintf(stderr, "%s %s\n", assertion ? "UNEXPECTED_JUCE_ASSERT" : "JUCE_LOG", message.toRawUTF8());
        std::fflush(stderr);
    }
private:
    std::unique_ptr<FileOutputStream> expectedLog_;
    int expectedAssertions_ = 0;
};

#if defined(_MSC_VER) && defined(_DEBUG)
int __cdecl testCrtReport(int reportType, char* message, int* returnValue)
{
    const bool failure = reportType == _CRT_ASSERT || reportType == _CRT_ERROR;
    if (failure) ++unexpectedAssertions;
    std::fprintf(stderr, "%s %s\n", failure ? "UNEXPECTED_CRT_ASSERT_OR_ERROR" : "CRT_WARNING", message ? message : "");
    std::fflush(stderr);
    if (returnValue) *returnValue = 0;
    return 1; // Report handled: no modal dialog or debugger break.
}
#endif

class TestDiagnostics final
{
public:
    TestDiagnostics()
    {
        Logger::setCurrentLogger(&logger_);
#if defined(_MSC_VER) && defined(_DEBUG)
        for (const int kind : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT})
        {
            _CrtSetReportMode(kind, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(kind, _CRTDBG_FILE_STDERR);
        }
        _CrtSetReportHook(testCrtReport);
#endif
    }
    ~TestDiagnostics() { Logger::setCurrentLogger(nullptr); }
    bool verifyAssertionCapture(const File& log) { return logger_.verifyAssertionCapture(log); }
private:
    TestLogger logger_;
};

void check(bool passed, const String& label)
{
    ++checks;
    if (!passed) ++failures;
    std::printf("%s %s\n", passed ? "PASS" : "FAIL", label.toRawUTF8());
}

double bell(double x, double centre, double width)
{
    const auto distance = (x - centre) / width;
    return std::exp(-0.5 * distance * distance);
}

void exportWidget(Component& widget, const File& output, const String& name)
{
    check(widget.getPeer() == nullptr, name + ": no native peer");
    for (const double scale : {1.0, 1.75, 2.0})
    {
        // This tests scaled offscreen exports. It does not impersonate a native
        // Windows peer at that DPI: WaterfallView has no peer and uses scale 1.
        auto image = widget.createComponentSnapshot(widget.getLocalBounds(), true, static_cast<float>(scale));
        const auto filename = name + "-" + String(roundToInt(scale * 100.0)) + ".png";
        check(image.isValid()
            && std::abs(image.getWidth() - widget.getWidth() * scale) <= 1.0
            && std::abs(image.getHeight() - widget.getHeight() * scale) <= 1.0,
            filename + ": requested export dimensions");
        if (!image.isValid()) continue;
        std::set<uint32> colours;
        // Inspect real pixels until the palette criterion is met. A regular
        // sparse grid can alias a narrow compact histogram (notably at 200%).
        for (int y = 0; y < image.getHeight() && colours.size() <= 8; ++y)
            for (int x = 0; x < image.getWidth() && colours.size() <= 8; ++x)
                colours.insert(image.getPixelAt(x, y).getARGB());
        check(colours.size() > 8, filename + ": nonblank production rendering");
        auto stream = output.getChildFile(filename).createOutputStream();
        PNGImageFormat png;
        const bool ready = stream != nullptr && stream->setPosition(0) && stream->truncate().wasOk();
        const bool written = ready && png.writeImageToStream(image, *stream);
        if (stream) stream->flush();
        check(written && output.getChildFile(filename).getSize() > 32, filename + ": PNG written");
        if (written) exportedFiles.add(filename);
    }
}

WaterfallFrame historyFrame(int index, bool carriers, bool overlay = false)
{
    WaterfallFrame result;
    result.time = index * 512.0 / 48000.0;
    result.sampleRate = 48000.0;
    result.hopSize = 512;
    result.channels = 2;
    result.generation = 7;
    result.revision = 4;
    result.hasProfile = carriers;
    result.sourceRevision = 1;
    result.carrierCapture = overlay ? 1 : 0;
    const FrequencyScale frequencies(FrequencySettings::maximumHz);
    const double tone = 190.0 * std::exp(index * 0.002);
    for (int band = 0; band < displayBandCount; ++band)
    {
        const double hz = frequencies.frequency((band + 0.5) / displayBandCount);
        result.frequencies[band] = static_cast<float>(hz);
        const double spectrum = 0.72 * bell(std::log(hz), std::log(tone), 0.10)
            + 0.42 * bell(std::log(hz), std::log(tone * 2), 0.075)
            + 0.24 * bell(std::log(hz), std::log(tone * 3), 0.065);
        result.levels.left[band] = static_cast<float>(jlimit(0.0, 1.0, spectrum));
        result.levels.right[band] = static_cast<float>(jlimit(0.0, 1.0, spectrum * (0.55 + index * 0.001)));
    }
    const auto centre = frequencies.position(tone) * WaterfallSettings::width;
    for (int x = 0; x < WaterfallSettings::width; ++x)
        result.profile[x] = static_cast<float>(0.9 * bell(x, centre, 1.8));
    return result;
}

void historyScene(const File& output, bool carriers, bool overlay)
{
    const String name = carriers ? "history-carriers" : overlay ? "history-fft-overlay" : "history-fft";
    auto view = std::make_unique<WaterfallView>(carriers ? "Synthetic carriers" : "Synthetic FFT");
    view->setSize(380, 640);
    auto pending = std::make_shared<std::vector<WaterfallFrame>>();
    for (int i = 0; i < 180; ++i) pending->push_back(historyFrame(i, carriers, overlay));
    int reads = 0;
    view->setSource([pending, &reads](std::vector<WaterfallFrame>& target)
    {
        ++reads;
        target.assign(pending->begin(), pending->end());
        pending->clear();
        return std::uint64_t{0};
    });
    bool overlayRequested = false;
    if (overlay)
    {
        view->enableCarrierOverlay(0xffffc65a, [&overlayRequested](bool enabled) { overlayRequested = enabled; });
        view->setCarrierOverlayEnabled(true);
    }
    int reportedFailures = 0;
    view->onFailure = [&reportedFailures](const String&) { ++reportedFailures; };
    view->setActive(true);
    view->refreshNow();
    check(reads == 1 && view->receivedFrames() == 180 && view->retainedRows() > 100,
        name + ": inherited provider drains into actual retained raster");
    check(reportedFailures == 0 && !view->rasterImage().isNull(), name + ": valid synthetic frames accepted");
    if (overlay) check(overlayRequested, name + ": active overlay requests its source");
    for (const float scale : {1.0f, 1.75f, 2.0f})
    {
        const auto live = view->physicalLiveBounds(scale);
        const auto plot = view->physicalPlotBounds(scale);
        check(live.getHeight() > 0 && plot.getHeight() > 0 && live.getBottom() <= plot.getY()
            && plot.getX() >= 0 && plot.getRight() <= roundToInt(view->getWidth() * scale) + 1
            && plot.getBottom() <= roundToInt(view->getHeight() * scale) + 1,
            name + ": live/history geometry bounded at " + String(scale));
    }
    exportWidget(*view, output, name);
    view->setActive(false);
    const auto received = view->receivedFrames();
    pending->push_back(historyFrame(180, carriers, overlay));
    view->refreshNow();
    check(view->receivedFrames() == received && pending->size() == 1, name + ": inactive view does not drain provider");
    view->setActive(true);
    view->refreshNow();
    check(view->receivedFrames() == received + 1 && pending->empty(), name + ": reactivation resumes provider");
    view->clearHistory();
    check(view->retainedRows() == 0, name + ": lifecycle reset removes retained rows");
    auto discontinuity = historyFrame(0, carriers, overlay);
    discontinuity.generation = 8;
    discontinuity.discontinuity = true;
    view->offer(discontinuity);
    check(view->retainedRows() == WaterfallSettings::linesPerFrame && reportedFailures == 0,
        name + ": new generation begins a fresh history");
    view->setActive(false);
    view->setSource({});
    view->onFailure = {};
}

void pitchScenes(const File& output)
{
    PitchMotionSnapshot motion;
    motion.status = PitchMotionResult::ok;
    motion.hasEstimate = true;
    motion.matchedMass = 0.8;
    motion.stationaryMass = 0.15;
    motion.meanMatchedDisplacementSemitones = 0.75;
    motion.modeSemitones = 0.75;
    motion.netSemitonesPerSecond = 2.0;
    motion.integratedSemitones = 0.4;
    motion.comparisonSeconds = 0.04;
    motion.stepSeconds = 512.0 / 48000.0;
    motion.sourcePeakCount = motion.targetPeakCount = 12;
    motion.solverIterations = 12;
    motion.solverConverged = true;
    double sum = 0.0;
    for (int i = 0; i < pitchHistogramBinCount; ++i)
        sum += bell(pitchHistogramMinimumSemitones + i * pitchHistogramStepSemitones, 0.75, 0.4);
    for (int i = 0; i < pitchHistogramBinCount; ++i)
    {
        const auto mass = static_cast<float>(0.8 * bell(pitchHistogramMinimumSemitones
            + i * pitchHistogramStepSemitones, 0.75, 0.4) / sum);
        motion.rawHistogram[i] = motion.smoothedHistogram[i] = mass;
    }
    auto movement = std::make_unique<PitchMotionIndicator>();
    movement->setSize(900, 320);
    movement->setSnapshot(motion);
    exportWidget(*movement, output, "pitch-movement");
    movement->setCompact(true);
    movement->setSize(900, 40);
    exportWidget(*movement, output, "pitch-compact");
    movement->clear();
    check(!movement->snapshot().hasEstimate, "pitch: clear removes current estimate");

    auto correlation = std::make_unique<SpectralCorrelationSnapshot>();
    correlation->status = PitchMotionResult::ok;
    correlation->hasEstimate = true;
    correlation->stepSeconds = motion.stepSeconds;
    auto correlationView = std::make_unique<SpectralCorrelationIndicator>();
    correlationView->setSize(900, 620);
    for (int observation = 0; observation < 80; ++observation)
    {
        correlation->centreTimeSeconds = observation * motion.stepSeconds;
        correlation->integratedSemitones = 0.25 * std::sin(observation * 0.035);
        for (int lag = 0; lag < spectralCorrelationLagCount; ++lag)
        {
            auto& result = correlation->lags[lag];
            result.hasEstimate = true;
            result.comparisonSeconds = spectralCorrelationRequestedLags[lag];
            result.displacementSemitones = 0.5 * std::sin(observation * 0.03 + lag * 0.2);
            result.peakCorrelation = 0.9;
            result.prominence = 0.25;
            for (int i = 0; i < pitchHistogramBinCount; ++i)
                result.correlation[i] = static_cast<float>(0.9 * bell(pitchHistogramMinimumSemitones
                    + i * pitchHistogramStepSemitones, result.displacementSemitones, 0.75));
        }
        correlationView->setSnapshot(*correlation);
    }
    check(correlationView->historySize() > 20, "correlation: snapshot timestamps populate real bounded history");
    exportWidget(*correlationView, output, "pitch-correlation");
    correlationView->clear();
    check(correlationView->historySize() == 0, "correlation: clear removes retained history");

    auto harmonic = std::make_unique<HarmonicCombSnapshot>();
    harmonic->status = PitchMotionResult::ok;
    harmonic->hasEstimate = true;
    harmonic->enabled = true;
    harmonic->generation = 3;
    harmonic->componentCount = 2;
    harmonic->components[0] = {196.0, 0.022, 0.022, 0.022, 1, 50.0};
    harmonic->components[1] = {329.63, 0.013, 0.013, 0.013, 2, 50.0};
    harmonic->selectedFrequencyHz = 196.0;
    harmonic->selectedAmplitude = 0.022;
    harmonic->maximumFrequencyHz = 20000.0;
    harmonic->inputBinHz = 48000.0 / 2048.0;
    harmonic->firstInputBin = 4;
    harmonic->inputSpectrumCount = 125;
    harmonic->count = 320;
    harmonic->fittedHarmonics = 16;
    harmonic->fitFraction = 0.85;
    harmonic->removedEnergyFraction = 0.7;
    for (int i = 0; i < harmonic->count; ++i)
    {
        const double hz = 100.0 * std::pow(30.0, i / static_cast<double>(harmonic->count - 1));
        harmonic->frequencyHz[i] = static_cast<float>(hz);
        const auto amplitude = static_cast<float>(0.022 * bell(std::log(hz), std::log(196.0), 0.04)
            + 0.013 * bell(std::log(hz), std::log(329.63), 0.04));
        harmonic->before[i] = amplitude;
        harmonic->after[i] = amplitude * 0.25f;
    }
    for (int i = 0; i < harmonic->inputSpectrumCount; ++i)
    {
        const double hz = (harmonic->firstInputBin + i) * harmonic->inputBinHz;
        harmonic->inputMagnitude[i] = static_cast<float>(0.022 * bell(hz, 196.0, 25.0)
            + 0.013 * bell(hz, 329.63, 25.0));
        harmonic->modelMagnitude[i] = harmonic->inputMagnitude[i] * 0.8f;
        harmonic->errorMagnitude[i] = harmonic->inputMagnitude[i] - harmonic->modelMagnitude[i];
    }
    auto harmonicView = std::make_unique<HarmonicCombIndicator>();
    harmonicView->setSettings(HarmonicCombSettings{});
    harmonicView->setSize(900, 620);
    harmonicView->setSnapshot(*harmonic);
    exportWidget(*harmonicView, output, "pitch-harmonic-model");
    harmonicView->setCompact(true);
    harmonicView->setSize(900, 40);
    exportWidget(*harmonicView, output, "carriers-compact");
    harmonicView->clear();
    check(!harmonicView->snapshot().hasEstimate, "harmonic: clear removes current estimate and trail");
}
}

int main(int argc, char** argv)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    TestDiagnostics diagnostics;
    if (argc != 2 || !juce::File::isAbsolutePath(juce::String::fromUTF8(argv[1])))
    {
        std::fprintf(stderr, "Usage: VisualizationsHarness <absolute output directory>\n");
        return 2;
    }
    const juce::File output(juce::String::fromUTF8(argv[1]));
    if (output.createDirectory().failed()) return 2;
    check(diagnostics.verifyAssertionCapture(output.getChildFile("assertion-self-check.log")),
        "real JUCE assertion reaches the noninteractive logger");
    {
        const juce::ScopedJuceInitialiser_GUI guiLifetime;
        const int desktopBefore = juce::Desktop::getInstance().getNumComponents();
        historyScene(output, false, false);
        historyScene(output, false, true);
        historyScene(output, true, false);
        pitchScenes(output);
        check(juce::Desktop::getInstance().getNumComponents() == desktopBefore,
            "offscreen harness creates no desktop window");
    }
    // Include GUI teardown in the assertion check, not just painting.
    check(unexpectedAssertions.load() == 0, "no unexpected JUCE or Debug CRT assertion/error");
    auto summary = std::make_unique<juce::DynamicObject>();
    summary->setProperty("checks", checks);
    summary->setProperty("failures", failures);
    summary->setProperty("unexpectedAssertions", unexpectedAssertions.load());
    summary->setProperty("exports", juce::var(exportedFiles));
    summary->setProperty("nativePeerDpiExercised", false);
    summary->setProperty("syntheticSnapshotsOnly", true);
    summary->setProperty("juceVersion", juce::SystemStats::getJUCEVersion());
    const bool saved = output.getChildFile("summary.json").replaceWithText(
        juce::JSON::toString(juce::var(summary.release()), true));
    std::printf("RESULT %d/%d passed, %d PNG exports\n", checks - failures, checks, exportedFiles.size());
    return failures == 0 && unexpectedAssertions.load() == 0 && saved ? 0 : 1;
}
