/*
FxSound
Copyright (C) 2026 FxSound LLC
SPDX-License-Identifier: AGPL-3.0-or-later
*/
// Low-level numerical DSP only: no DfxDsp host, registry, device, profile or UI.
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "codedefs.h"
#include "sos.h"
#include "pwav.h"
#include "pt_defs.h"
#include "c_play.h"
extern "C" {
#include "c_dsps.h"
#include "comSftwr.h"
}

namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr int frames = 480;
bool expectClipped = false;
int checks = 0, failures = 0;
std::vector<float> unchangedAudio;

void check(bool passed, const char* name)
{
    ++checks;
    if (!passed) ++failures;
    std::printf("%s %s\n", passed ? "PASS" : "FAIL", name);
}

bool process(PT_HANDLE* sos, float* input, float* output, int channels, float rate)
{
    return (channels <= 2
        ? sosProcessBuffer(sos, input, output, frames, channels, rate)
        : sosProcessSurroundBuffer(sos, input, output, frames, channels, rate)) == OKAY;
}

void remember(const std::vector<float>& audio)
{
    unchangedAudio.insert(unchangedAudio.end(), audio.begin(), audio.end());
}

void bassHeadroom(int channels, float rate, bool inPlace)
{
    PT_HANDLE* sos = nullptr;
    PT_HANDLE* attenuation = nullptr;
    if (sosNew(&sos, nullptr, 10) != OKAY || sosNew(&attenuation, nullptr, 10) != OKAY)
    {
        check(false, "SOS initialization");
        sosFreeUp(&sos);
        sosFreeUp(&attenuation);
        return;
    }
    bool valid = sosSetMasterGain(sos, 4.0f) == OKAY
        && sosSetVolumeLeveling(sos, 0.125f) == OKAY
        && sosSetMasterGain(attenuation, 0.25f) == OKAY;
    const float scales[] = {1.0f, -0.25f, 0.5f, 1.7f, -0.5f, 0.125f, -0.75f, 0.25f};
    std::vector<float> input(frames * channels), output(input.size()), expected(input.size());
    float peak = 0.0f, minValue = 0.0f, rampError = 0.0f, channelError = 0.0f;
    float recoveryError = 0.0f, lfeError = 0.0f;
    // The 120 Hz detector sidechain attenuates this 20 Hz tone. Its actual audio
    // peaks must remain available to downstream processing, not flatten here.
    for (int block = 0; block < 100; ++block)
    {
        for (int frame = 0; frame < frames; ++frame)
            for (int channel = 0; channel < channels; ++channel)
                input[frame * channels + channel] = scales[channel] * 0.6f
                    * static_cast<float>(std::sin(2.0 * pi * 20.0 * (block * frames + frame) / rate));
        output = input;
        valid &= process(sos, inPlace ? output.data() : input.data(), output.data(), channels, rate);
        // Infer the common linear gain ramp from its endpoints. This checks
        // waveform preservation without reimplementing the leveling detector.
        int first = 0, last = frames - 1;
        while (first < last && std::fabs(input[first * channels]) < 0.02f) ++first;
        while (last > first && std::fabs(input[last * channels]) < 0.02f) --last;
        if (first == last) { valid = false; break; }
        const float firstGain = output[first * channels] / (4.0f * input[first * channels]);
        const float lastGain = output[last * channels] / (4.0f * input[last * channels]);
        for (int frame = 0; frame < frames; ++frame)
        {
            const float gain = firstGain + (lastGain - firstGain) * (frame - first) / (last - first);
            for (int channel = 0; channel < channels; ++channel)
            {
                const int index = frame * channels + channel;
                const bool lfe = channels > 2 && channel == 3;
                const float reference = input[index] * 4.0f * (lfe ? 1.0f : gain);
                expected[index] = reference * 0.25f;
                valid &= std::isfinite(output[index]);
                if (lfe)
                    lfeError = (std::max)(lfeError, std::fabs(output[index] - input[index] * 4.0f));
                else
                {
                    rampError = (std::max)(rampError, std::fabs(output[index] - reference));
                    channelError = (std::max)(channelError,
                        std::fabs(output[index] - output[frame * channels] * scales[channel]));
                    peak = (std::max)(peak, output[index]);
                    minValue = (std::min)(minValue, output[index]);
                }
            }
        }
        valid &= sosProcessBuffer_MasterGainOnly(attenuation, output.data(), output.data(), frames, channels) == OKAY;
        for (int index = 0; index < frames * channels; ++index)
            recoveryError = (std::max)(recoveryError, std::fabs(output[index] - expected[index]));
    }
    char label[128];
    std::snprintf(label, sizeof(label), "%s bass headroom %d channels %.0f Hz %s",
        expectClipped ? "baseline clipped" : "preserved", channels, rate, inPlace ? "in-place" : "separate");
    std::printf("  peak=%.8g min=%.8g ramp_error=%.8g channel_error=%.8g recovery_error=%.8g lfe_error=%.8g\n",
        peak, minValue, rampError, channelError, recoveryError, lfeError);
    const bool shape = expectClipped
        ? peak > 0.8f && peak <= 1.00001f && minValue >= -1.00001f && rampError > 0.01f && recoveryError > 0.002f
        : peak > 1.1f && minValue < -1.1f && rampError < 2e-5f && channelError < 2e-5f && recoveryError < 5e-6f;
    check(valid && shape && lfeError < 1e-6f, label);
    check(sosFreeUp(&sos) == OKAY && sosFreeUp(&attenuation) == OKAY, "SOS headroom cleanup");
}

void unchangedSosPaths(int channels, float rate)
{
    PT_HANDLE* sos = nullptr;
    bool valid = sosNew(&sos, nullptr, 10) == OKAY;
    if (!valid) { check(false, "SOS unchanged-path initialization"); return; }
    std::vector<float> input(frames * channels), output(input.size());
    for (int i = 0; i < frames * channels; ++i) input[i] = i % 2 ? -2.5f : 1.5f;
    valid &= process(sos, input.data(), output.data(), channels, rate);
    check(valid && input == output, "disabled leveling preserves over-range floats exactly");
    remember(output);
    valid &= sosSetMasterGain(sos, 0.25f) == OKAY;
    valid &= sosProcessBuffer_MasterGainOnly(sos, input.data(), output.data(), frames, channels) == OKAY;
    for (int i = 0; i < frames * channels; ++i) valid &= output[i] == input[i] * 0.25f;
    check(valid, "existing MasterGainOnly multiplies without a PCM clamp");
    remember(output);
    valid &= sosSetMasterGain(sos, 1.0f) == OKAY && sosSetVolumeLeveling(sos, 0.125f) == OKAY;
    std::fill(input.begin(), input.end(), 0.0f);
    valid &= process(sos, input.data(), output.data(), channels, rate);
    check(valid && input == output, "enabled leveling preserves exact silence");
    remember(output);
    // Capture paths that must remain byte-identical to the preserved public
    // baseline. These signals stay below the removed intermediate ceiling.
    for (int block = 0; block < 20; ++block)
    {
        for (int frame = 0; frame < frames; ++frame)
            for (int channel = 0; channel < channels; ++channel)
                input[frame * channels + channel] = 0.25f * (channel % 2 ? -0.5f : 1.0f)
                    * static_cast<float>(std::sin(2.0 * pi * 1000.0 * (block * frames + frame) / rate));
        valid &= process(sos, input.data(), output.data(), channels, rate);
        for (float sample : output) valid &= std::isfinite(sample) && std::fabs(sample) < 0.9f;
        remember(output);
    }
    check(valid, "below-ceiling leveling remains finite and unclipped");
    check(sosFreeUp(&sos) == OKAY, "SOS unchanged-path cleanup");
}

void finalLimiter(int channels, float rate)
{
    std::vector<float> params(DSPFX_MAX_NUM_PROCS * 2 * DSPS_MAX_NUM_PARAMS);
    std::vector<float> state(DSPFX_MAX_NUM_PROCS * DSPS_NUM_STATE_VARS);
    std::vector<float> memory(DSPS_SOFT_MEM_PLAY_LENGTH);
    reinterpret_cast<long*>(params.data())[DSP_PLAY_STEREO_MODE_INDEX] = channels == 2;
    bool valid = dspsPlayInit(params.data(), memory.data(), static_cast<long>(memory.size()), state.data(),
        DSPS_INIT_PARAMS | DSPS_INIT_MEMORY, rate) == OKAY;
    std::vector<float> audio(frames * channels);
    float peak = 0.0f;
    hardwareMeterValType meters{};
    for (int block = 0; block < 100 && valid; ++block)
    {
        for (int frame = 0; frame < frames; ++frame)
            for (int channel = 0; channel < channels; ++channel)
                audio[frame * channels + channel] = (channel == 0 ? 3.0f : -2.0f)
                    * static_cast<float>(std::sin(2.0 * pi * 40.0 * (block * frames + frame) / rate));
        dspsPlayProcess32(reinterpret_cast<long*>(audio.data()), frames, params.data(), memory.data(),
            state.data(), &meters, COM_32_BIT_FLOAT_SAMPLES);
        for (float sample : audio)
        {
            valid &= std::isfinite(sample);
            peak = (std::max)(peak, std::fabs(sample));
        }
        remember(audio);
    }
    char label[96];
    std::snprintf(label, sizeof(label), "unchanged final limiter %d channels %.0f Hz", channels, rate);
    std::printf("  final_peak=%.8g\n", peak);
    check(valid && peak > 0.5f && peak <= 0.966151f, label);
}

void integerRepresentationBoundary()
{
    // The unchanged real PCM24 converter is an integer representation boundary,
    // not permission to clamp the intermediate float path at every stage.
    const float samples[] = {-2.0f, -1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 2.0f};
    const int expected[] = {-8388608, -8388608, -4194304, 0, 4194304, 8388607, 8388607};
    std::vector<float> input(samples, samples + 7);
    unsigned char bytes[21] = {};
    bool valid = pwavFloatTo24Bit(input.data(), reinterpret_cast<char*>(bytes), 7, IS_FALSE) == OKAY;
    for (int i = 0; i < 7; ++i)
    {
        const unsigned bits = bytes[i * 3] | (static_cast<unsigned>(bytes[i * 3 + 1]) << 8)
            | (static_cast<unsigned>(bytes[i * 3 + 2]) << 16);
        const int sample = (bits & 0x800000u) ? static_cast<int>(bits) - 0x1000000 : static_cast<int>(bits);
        valid &= sample == expected[i];
    }
    check(valid, "existing PCM24 conversion retains integer saturation and sample order");
}
}

int main(int argc, char** argv)
{
    const char* capturePath = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--expect-clipped") == 0) expectClipped = true;
        else if (std::strcmp(argv[i], "--capture") == 0 && i + 1 < argc) capturePath = argv[++i];
        else { std::fprintf(stderr, "Unknown/incomplete argument: %s\n", argv[i]); return 2; }
    }
    std::printf("Expected intermediate output: %s\n", expectClipped ? "public baseline clamp" : "float headroom");
    for (float rate : {44100.0f, 48000.0f, 96000.0f})
    {
        for (int channels : {1, 2, 6, 8})
        {
            for (bool inPlace : {false, true}) bassHeadroom(channels, rate, inPlace);
            unchangedSosPaths(channels, rate);
        }
        for (int channels : {1, 2}) finalLimiter(channels, rate);
    }
    integerRepresentationBoundary();
    if (capturePath != nullptr)
    {
        FILE* file = nullptr;
        if (fopen_s(&file, capturePath, "wb") != 0 || file == nullptr) return 2;
        const bool written = std::fwrite(unchangedAudio.data(), sizeof(float), unchangedAudio.size(), file) == unchangedAudio.size();
        const bool closed = std::fclose(file) == 0;
        check(written && closed, "write unchanged-path PCM for exact baseline comparison");
        std::printf("Captured %zu unchanged-path float samples\n", unchangedAudio.size());
    }
    std::printf("RESULT: %d/%d passed\n", checks - failures, checks);
    return failures == 0 ? 0 : 1;
}
