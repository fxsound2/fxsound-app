#pragma once
#include "PlaybackReserve.h"

// This header is included after the common in-memory WASAPI test fixture.
int adjustablePlaybackCases()
{
    if (fxsoundtiming::targetFrames(48000, 19200, 480, 400) != 14400
        || fxsoundtiming::targetFrames(48000, 5760, 480, 300) != 5280) return 19;
    sndDevicesHdlType h{};
    TestEndpoint endpoint;
    endpoint.capacity = 19200; // 400 ms: exercise the full 300-ms reserve plus a packet.
    h.pAudioClientPlayback = &endpoint;
    h.pAudioClientPlaybackRender = &endpoint;
    h.wfxPlayback.nSamplesPerSec = 48000;
    h.wfxPlayback.nChannels = 1;
    h.upsampleRatio = 1;
    h.bufferFrameSizePlayback = endpoint.capacity;
    h.playbackStreamIsTemporarilyPaused = 1;
    std::vector<float> pcm(480);
    int absolute = 0;
    auto& control = fxsoundtiming::playbackReserveControl();
    auto send = [&]() {
        for (int i=0; i<480; ++i) pcm[i]=static_cast<float>(absolute + i);
        absolute += 480;
        const auto offset = endpoint.audio.size();
        h.playbackFrameCount = 480;
        h.fPlaybackBuf = pcm.data();
        int status = -1;
        const int result = sndDevicesDoPlayback(reinterpret_cast<PT_HANDLE*>(&h), &status);
        const auto count = endpoint.audio.size() - offset;
        if (result != OKAY || status != SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS || count < 470 || count > 490)
            return false;
        if (endpoint.audio[offset] != pcm.front() || endpoint.audio.back() != pcm.back()) return false;
        if (!std::is_sorted(endpoint.audio.begin()+offset,endpoint.audio.end())) return false;
        if (!control.transitioning.load() && !std::equal(pcm.begin(),pcm.end(),endpoint.audio.begin()+offset))
            return false;
        return true;
    };
    auto packets = [&](unsigned count) {
        for (unsigned i=0; i<count; ++i) {
            endpoint.queued -= std::min(480u,endpoint.queued);
            if (!send()) return false;
        }
        return true;
    };
    if (control.targetMs.load()!=90 || !send() || endpoint.queued!=4800) return 10;
    control.targetMs.store(0);
    if (!packets(605) || control.transitioning.load() || endpoint.queued!=480) return 11;
    control.targetMs.store(300);
    if (!packets(1505) || control.transitioning.load() || endpoint.queued!=14880) return 12;
    control.targetMs.store(32);
    if (!packets(20) || !control.transitioning.load()) return 13;
    control.targetMs.store(64);
    if (!packets(1500) || control.transitioning.load() || endpoint.queued!=3552) return 14;
    if (endpoint.starts!=1 || endpoint.silentWrites!=1) return 15;
    control.targetMs.store(50);
    endpoint.queued=0; h.playbackStreamIsTemporarilyPaused=1;
    if (!send() || endpoint.starts!=2 || endpoint.queued!=2880 || control.transitioning.load()) return 16;
    std::puts("Live renderer: 90 -> 0 -> 300, reversal, exact plateau, audio order and capped rate, no added silence, re-prime passed.");
    return 0;
}
