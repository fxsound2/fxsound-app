// SPDX-License-Identifier: AGPL-3.0-or-later
// Numerical regression tests. No audio device, user settings or UI are used.
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
#include <memory>
#include "codedefs.h"
#include "sos.h"
#include "pt_defs.h"
extern "C" {
#include "c_dsps.h"
#include "comSftwr.h"
#include "../ptComSftDfx/u_comSftwr.h"
}
#include "com.h"
#include "../ptutil/COM/u_com.h"
#include "c_play.h"
#include "c_max.h"


namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr int frames = 480;
int failures = 0;
int checks = 0;
std::vector<float> capturedAudio;

void check(bool passed, const char* name)
{
    ++checks;
    if (!passed) ++failures;
    std::printf("%s: %s\n", passed ? "PASS" : "FAIL", name);
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
    // Exercise the real effects chain and its final maximizer with over-range input.
    for (int block = 0; block < 100 && valid; ++block)
    {
        for (int frame = 0; frame < frames; ++frame)
            for (int channel = 0; channel < channels; ++channel)
                audio[frame * channels + channel] = (channel == 0 ? 3.0f : -2.0f)
                    * static_cast<float>(std::sin(2.0 * pi * 40.0 * (block * frames + frame) / rate));
        dspsPlayProcess32(reinterpret_cast<long*>(audio.data()), frames, params.data(), memory.data(),
            state.data(), &meters, COM_32_BIT_FLOAT_SAMPLES);
        capturedAudio.insert(capturedAudio.end(), audio.begin(), audio.end());
        for (float sample : audio)
        {
            valid &= std::isfinite(sample);
            peak = std::max(peak, std::fabs(sample));
        }
    }
    char label[128];
    std::snprintf(label, sizeof(label), "effects chain final limiter %d channels %.0f Hz", channels, rate);
    std::printf("  final_peak=%.8g\n", peak);
    check(valid && peak > 0.5f && peak <= 0.966151f, label);
#ifndef FXSOUND_TEST_BASELINE
    check(meters.aux_vals[DFX_LIMITER_RATIO_LEFT] > 1.0f
        && meters.aux_vals[DFX_LIMITER_RATIO_RIGHT] > 1.0f, "effects chain publishes limiter activity");
#endif
}

#ifndef FXSOUND_TEST_BASELINE
void limiterMeters(bool floatVariant, int channels, float left, float right)
{
    std::vector<float> params(DSPFX_MAX_NUM_PROCS * 2 * DSPS_MAX_NUM_PARAMS);
    std::vector<float> state(DSPFX_MAX_NUM_PROCS * DSPS_NUM_STATE_VARS);
    std::vector<float> memory(DSPS_SOFT_MEM_MAXIMIZER_LENGTH);
    reinterpret_cast<long*>(params.data())[DSP_PLAY_STEREO_MODE_INDEX] = channels == 2;
    bool valid = dspsMaximizerInit(params.data(), memory.data(), 0, state.data(),
        DSPS_INIT_PARAMS | DSPS_INIT_MEMORY, 48000.0f) == OKAY;
    auto* maxi = reinterpret_cast<dspMaxiStructType*>(params.data());
    maxi->wet_gain = 1.0f;
    maxi->dry_gain = 0.0f;
    maxi->gain_boost = 1.0f;
    maxi->target_level = 100.0f; // Keep automatic boost adaptation out of this measurement.
    maxi->quantize_on_flag = 0;
    std::vector<float> audio(frames * channels);
    for (int i = 0; i < frames; ++i)
        for (int c = 0; c < channels; ++c) audio[i * channels + c] = c == 0 ? left : right;
    hardwareMeterValType meters{};
    auto run = floatVariant ? dspsMaximizerProcess32 : dspsMaximizerProcess;
    run(reinterpret_cast<long*>(audio.data()), frames, params.data(), memory.data(), state.data(),
        &meters, COM_32_BIT_FLOAT_SAMPLES);
    // Measure attenuation from the actual delayed output. The legacy envelope
    // can overshoot the constant input during its initial ramp; input amplitude
    // alone is therefore not an exact oracle for peak gain reduction.
    float measured[2] = {};
    for (int i = 0; i < frames; ++i)
        for (int c = 0; c < channels; ++c)
            if (std::fabs(audio[i * channels + c]) > 1.0e-12f)
                measured[c] = std::max(measured[c], std::fabs((c == 0 ? left : right)
                    * maxi->max_output / audio[i * channels + c]));
    const float expectedLeft = measured[0] > 1.00001f ? measured[0] : 0.0f;
    const float expectedRight = channels == 1 ? expectedLeft : (measured[1] > 1.00001f ? measured[1] : 0.0f);
    std::printf("  variant=%d channels=%d input=(%.3f,%.3f) ratios=(%.8g,%.8g) expected=(%.3f,%.3f)\n",
        floatVariant, channels, left, right, meters.aux_vals[DFX_LIMITER_RATIO_LEFT],
        meters.aux_vals[DFX_LIMITER_RATIO_RIGHT], expectedLeft, expectedRight);
    check(valid && std::fabs(meters.aux_vals[DFX_LIMITER_RATIO_LEFT] - expectedLeft) < 0.0001f
        && std::fabs(meters.aux_vals[DFX_LIMITER_RATIO_RIGHT] - expectedRight) < 0.0001f,
        floatVariant ? "Maxi32 exact peak reduction and channels" : "Maxi16 exact peak reduction and channels");
}

void meterTransport()
{
    float invalidLeft = -1.0f, invalidRight = -1.0f;
    check(comTakeLimiterActivity(nullptr, &invalidLeft, &invalidRight) == NOT_OKAY_NO_BREAK,
        "transport rejects a missing handle");
    comHdlType missing{};
    missing.softdsp_mode = 1;
    check(comTakeLimiterActivity(reinterpret_cast<PT_HANDLE*>(&missing), &invalidLeft, &invalidRight) == NOT_OKAY_NO_BREAK,
        "transport reports a missing software meter");
    check(comTakeLimiterActivity(reinterpret_cast<PT_HANDLE*>(&missing), nullptr, &invalidRight) == NOT_OKAY_NO_BREAK
        && comTakeLimiterActivity(reinterpret_cast<PT_HANDLE*>(&missing), &invalidLeft, nullptr) == NOT_OKAY_NO_BREAK,
        "transport rejects missing output pointers");
    auto soft = std::make_unique<comSftwrHdlType>();
    std::vector<float> memory(DSPS_SOFT_MEM_PLAY_LENGTH);
    soft->dsp_memory = memory.data();
    soft->dsp_function_index = 0;
    soft->comSftDspProcessPtr[0] = dspsPlayProcess32;
    reinterpret_cast<long*>(soft->dsp_params)[DSP_PLAY_STEREO_MODE_INDEX] = 1;
    bool valid = dspsPlayInit(soft->dsp_params, memory.data(), static_cast<long>(memory.size()),
        soft->dsp_state, DSPS_INIT_PARAMS | DSPS_INIT_MEMORY, 48000.0f) == OKAY;
    comHdlType com{};
    com.softdsp_mode = 1;
    com.comSftwr_hdl = reinterpret_cast<PT_HANDLE*>(soft.get());
    float left = -1.0f, right = -1.0f;
    valid &= comTakeLimiterActivity(reinterpret_cast<PT_HANDLE*>(&com), &left, &right) == OKAY;
    check(valid && left == 0.0f && right == 0.0f, "transport initially clear");
    std::vector<float> audio(frames * 2, 3.0f);
    valid &= comSftwrProcessWaveBuffer(com.comSftwr_hdl, reinterpret_cast<long*>(audio.data()),
        frames, 1, 1, COM_32_BIT_FLOAT_SAMPLES) == OKAY;
    const float peakLeft = soft->limiter_peak_ratio[0];
    const float peakRight = soft->limiter_peak_ratio[1];
    // Bypassed blocks must not repeat stale meters or erase an unconsumed event.
    reinterpret_cast<dspPlayStructType*>(soft->dsp_params)->bypass_on = 1;
    std::fill(audio.begin(), audio.end(), 0.0f);
    valid &= comSftwrProcessWaveBuffer(com.comSftwr_hdl, reinterpret_cast<long*>(audio.data()),
        frames, 1, 1, COM_32_BIT_FLOAT_SAMPLES) == OKAY;
    valid &= comTakeLimiterActivity(reinterpret_cast<PT_HANDLE*>(&com), &left, &right) == OKAY;
    check(valid && left > 1.0f && right > 1.0f && left == peakLeft && right == peakRight,
        "transport retains maximum across DSP blocks and bypass");
    valid &= comSftwrProcessWaveBuffer(com.comSftwr_hdl, reinterpret_cast<long*>(audio.data()),
        frames, 1, 1, COM_32_BIT_FLOAT_SAMPLES) == OKAY;
    valid &= comTakeLimiterActivity(reinterpret_cast<PT_HANDLE*>(&com), &left, &right) == OKAY;
    check(valid && left == 0.0f && right == 0.0f, "transport consume and bypass clear stale events");
}
#endif
}

int main(int argc, char** argv)
{
    // No device/settings access. Invalid meter requests return non-modal status codes.
#ifndef FXSOUND_TEST_BASELINE
    meterTransport();
    for (bool floatVariant : {false, true})
        for (int channels : {1, 2})
            for (const auto levels : {std::pair<float, float>{0.1f, -0.25f}, {2.0f, -0.1f}, {0.1f, -4.0f}, {2.0f, -4.0f}})
                limiterMeters(floatVariant, channels, levels.first, levels.second);
#endif
    for (float rate : {44100.0f, 48000.0f, 96000.0f})
    {
        for (int channels : {1, 2}) finalLimiter(channels, rate);
    }
    if (argc >= 2)
    {
        FILE* file = nullptr;
        if (fopen_s(&file, argv[1], "wb") != 0 || file == nullptr) return 2;
        const bool written = std::fwrite(capturedAudio.data(), sizeof(float), capturedAudio.size(), file) == capturedAudio.size();
        std::fclose(file);
        check(written, "audio comparison capture");
    }
    std::printf("RESULT: %d/%d passed\n", checks - failures, checks);
    return failures == 0 ? 0 : 1;
}
