/* FxSound — AGPL-3.0-or-later. Hardware-free production renderer regression. */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "codedefs.h"
#include "sndDevices.h"
#include "PlaybackReserve.h"
#include "u_AudioPassthru.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

// Capacity and queue are frames; scratch/audio contain interleaved channel scalars.
class TestEndpoint final : public IAudioClient, public IAudioRenderClient
{
public:
    UINT32 capacity = 19200, queued = 0, clockStep = 0, channels = 1;
    bool running = false, failWrite = false, failRelease = false, failPadding = false, failStart = false, failStop = false;
    unsigned starts = 0, stops = 0, resets = 0, silentWrites = 0, writes = 0;
    unsigned acquired = 0;
    sndDevicesHdlType* stopAfterWrite = nullptr;
    std::function<void()> onStop, onStart;
    std::vector<float> scratch, audio;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** out) override { if (out) *out = nullptr; return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE GetCurrentPadding(UINT32* n) override {
        if (failPadding) return E_FAIL;
        if (running) queued -= (std::min)(queued, clockStep);
        *n = queued; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetBuffer(UINT32 n, BYTE** dst) override {
        if (failWrite || queued > capacity || n > capacity - queued) return E_FAIL;
        acquired = n;
        scratch.assign(static_cast<std::size_t>(n) * channels, 0.f);
        *dst = reinterpret_cast<BYTE*>(scratch.data()); return S_OK;
    }
    HRESULT STDMETHODCALLTYPE ReleaseBuffer(UINT32 n, DWORD flags) override {
        if (failRelease || n != acquired) return E_FAIL;
        queued += n; ++writes;
        if (flags & AUDCLNT_BUFFERFLAGS_SILENT) ++silentWrites;
        else audio.insert(audio.end(), scratch.begin(), scratch.end());
        if (stopAfterWrite) stopAfterWrite->stopAudioCaptureAndPlaybackLoop = 1;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Start() override {
        if (failStart) return E_FAIL;
        if (onStart) onStart();
        running = true; ++starts; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Stop() override {
        ++stops;
        if (onStop) onStop();
        if (failStop) return E_FAIL;
        running = false; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Reset() override { queued = 0; ++resets; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetStreamLatency(REFERENCE_TIME*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetMixFormat(WAVEFORMATEX**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetBufferSize(UINT32*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetDevicePeriod(REFERENCE_TIME*, REFERENCE_TIME*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Initialize(AUDCLNT_SHAREMODE,DWORD,REFERENCE_TIME,REFERENCE_TIME,const WAVEFORMATEX*,LPCGUID) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE IsFormatSupported(AUDCLNT_SHAREMODE,const WAVEFORMATEX*,WAVEFORMATEX**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetEventHandle(HANDLE) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetService(REFIID,void**) override { return E_NOTIMPL; }
};

#include "PlaybackReserveCases.h" // Preserved private transition regression, default 90 ms.

struct PlaybackReserveTestPeer
{
    static void inert(AudioPassthruPrivate& audio) {
        audio.hp_sndDevices_ = nullptr; audio.hProcessingThread_ = nullptr;
    }
    static void attach(AudioPassthruPrivate& audio, sndDevicesHdlType& handle, HANDLE thread) {
        audio.hp_sndDevices_ = reinterpret_cast<PT_HANDLE*>(&handle);
        audio.hProcessingThread_ = thread; audio.i_kill_processing_thread_ = IS_FALSE;
    }
    static bool killRequested(const AudioPassthruPrivate& audio) { return audio.i_kill_processing_thread_ != IS_FALSE; }
    static HANDLE threadHandle(const AudioPassthruPrivate& audio) { return audio.hProcessingThread_; }
};

namespace
{
int checks = 0, failures = 0;
void check(bool condition, const char* name) {
    ++checks; if (!condition) ++failures;
    std::printf("%s %s\n", condition ? "PASS" : "FAIL", name);
}
bool sameBytes(const std::vector<float>& a, const std::vector<float>& b) {
    return a.size() == b.size() && std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0;
}
struct PlaybackFixture
{
    // A handle contains large fixed device tables. Separate Debug stack slots
    // for successive test scopes must not exhaust the default process stack.
    std::unique_ptr<sndDevicesHdlType> storage = std::make_unique<sndDevicesHdlType>();
    sndDevicesHdlType& h = *storage;
    TestEndpoint ep;
    std::vector<float> pcm;
    PlaybackFixture(unsigned rate, unsigned channels) {
        ep.channels = channels; ep.capacity = rate * 400 / 1000;
        h.pAudioClientPlayback = &ep; h.pAudioClientPlaybackRender = &ep;
        h.wfxPlayback.nSamplesPerSec = rate; h.wfxPlayback.nChannels = static_cast<WORD>(channels);
        h.upsampleRatio = 1; h.bufferFrameSizePlayback = ep.capacity;
        h.playbackStreamIsTemporarilyPaused = 1;
        fxsoundtiming::playbackReserveControl().targetMs = 90;
    }
    int send(unsigned frames, unsigned origin = 0) {
        pcm.resize(static_cast<std::size_t>(frames) * ep.channels);
        for (unsigned f = 0; f < frames; ++f)
            for (unsigned c = 0; c < ep.channels; ++c) pcm[f * ep.channels + c] = static_cast<float>(origin + f * 16 + c);
        h.playbackFrameCount = frames; h.fPlaybackBuf = pcm.data();
        int status = -1;
        const int result = sndDevicesDoPlayback(reinterpret_cast<PT_HANDLE*>(&h), &status);
        return result == OKAY ? status : -1;
    }
};

void rateAndChannelCases(unsigned rate, unsigned channels)
{
    std::printf("CASE %u Hz, %u channels\n", rate, channels);
    PlaybackFixture f(rate, channels);
    const unsigned packet = rate / 100;
    check(f.send(packet) == SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS && f.ep.queued == rate * 90 / 1000 + packet
        && f.ep.starts == 1 && f.ep.silentWrites == 1 && sameBytes(f.ep.audio, f.pcm)
        && fxsoundtiming::playbackReserveControl().appliedTargetMs == 90,
        "actual renderer primes 90 ms and preserves every interleaved scalar");
    bool steady = true;
    for (unsigned i = 0; i < 10; ++i) {
        f.ep.queued -= packet; f.ep.audio.clear();
        steady &= f.send(packet, (i + 1) * 16000) == SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS
            && sameBytes(f.ep.audio, f.pcm) && f.ep.queued == rate * 90 / 1000 + packet;
    }
    check(steady && f.ep.silentWrites == 1, "steady queue is stable and PCM is byte-identical");
    auto transition = [&](unsigned target) {
        auto& control = fxsoundtiming::playbackReserveControl();
        control.targetMs = target;
        const unsigned plateau = rate * target / 1000 + packet;
        for (unsigned i = 0; i < 2500; ++i) {
            f.ep.queued -= (std::min)(packet, f.ep.queued); f.ep.audio.clear();
            if (f.send(packet, i * 16000) != SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS) return false;
            const auto outFrames = f.ep.audio.size() / channels;
            if (outFrames + packet / 50 + 1 < packet || outFrames > packet + packet / 50 + 1) return false;
            for (unsigned c = 0; c < channels; ++c)
                if (f.ep.audio[c] != f.pcm[c]
                    || f.ep.audio[(outFrames - 1) * channels + c] != f.pcm[(packet - 1) * channels + c]) return false;
            if (!control.transitioning.load()) {
                if (!sameBytes(f.ep.audio, f.pcm)) return false;
                if (f.ep.queued == plateau) return true;
            }
        }
        return false;
    };
    check(transition(0) && transition(300) && f.ep.silentWrites == 1,
        "multichannel 90 -> 0 -> 300 reaches exact plateaus with capped rate and no added silence");
    f.ep.queued = 0; f.h.playbackStreamIsTemporarilyPaused = 1; f.ep.audio.clear();
    check(f.send(17) == SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS && sameBytes(f.ep.audio, f.pcm)
        && f.ep.starts == 2 && f.ep.silentWrites == 2, "short packet reprimes after an empty paused stream");
}

void capacityStopAndErrors()
{
    {
        PlaybackFixture f(48000, 2);
        f.ep.capacity = 3840; f.h.bufferFrameSizePlayback = f.ep.capacity;
        fxsoundtiming::playbackReserveControl().targetMs = 300;
        check(f.send(480) == SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS && f.ep.queued == 3840
            && fxsoundtiming::playbackReserveControl().appliedTargetMs == 70 && sameBytes(f.ep.audio, f.pcm),
            "negotiated capacity clamps reserve while retaining a complete packet");
    }
    {
        PlaybackFixture f(48000, 2);
        f.ep.capacity = 5; f.h.bufferFrameSizePlayback = 5; f.ep.clockStep = 3;
        check(f.send(29) == SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS && sameBytes(f.ep.audio, f.pcm)
            && f.ep.writes > 1 && f.ep.silentWrites == 0, "stereo packet larger than capacity is split in order without truncation");
        f.h.stopAudioCaptureAndPlaybackLoop = 1;
        const auto writes = f.ep.writes;
        check(f.send(9) == SND_DEVICES_CAPTURE_FORCED_EXIT && f.ep.writes == writes,
            "stop request causes no further render writes");
    }
    {
        PlaybackFixture f(48000, 2);
        f.ep.capacity = 5; f.h.bufferFrameSizePlayback = 5; f.ep.clockStep = 3; f.ep.stopAfterWrite = &f.h;
        check(f.send(29) == SND_DEVICES_CAPTURE_FORCED_EXIT && f.ep.writes == 1,
            "stop between split writes leaves the remainder untouched");
        check(f.ep.starts == 0 && f.h.playbackStreamIsTemporarilyPaused,
            "stop during the first audio write prevents render startup");
    }
    {
        PlaybackFixture f(48000, 2);
        f.ep.stopAfterWrite = &f.h;
        check(f.send(480) == SND_DEVICES_CAPTURE_FORCED_EXIT && f.ep.writes == 1 && f.ep.silentWrites == 1
            && f.ep.audio.empty() && f.ep.starts == 0 && f.h.playbackStreamIsTemporarilyPaused,
            "stop during silence priming prevents audio writes and render startup");
    }
    for (unsigned error = 0; error < 4; ++error) {
        PlaybackFixture f(48000, 2);
        f.ep.failPadding = error == 0; f.ep.failWrite = error == 1;
        f.ep.failRelease = error == 2; f.ep.failStart = error == 3;
        const int expected[] = {SND_DEVICES_GET_PADDING_FAILED, SND_DEVICES_PLAYBACK_RENDER_FAILED,
            SND_DEVICES_RELEASE_BUFFER_FAILED, SND_DEVICES_PLAYBACK_ERROR};
        check(f.send(480) != SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS && f.h.function_status == expected[error],
            "WASAPI failure propagates through the actual renderer status");
    }
    {
        PlaybackFixture f(48000, 1);
        fxsoundtiming::playbackReserveControl().targetMs = 0;
        f.ep.capacity = 480; f.h.bufferFrameSizePlayback = 480;
        check(f.send(480) == SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS, "stalled-endpoint fixture first fills its real frame capacity");
        const auto before = GetTickCount64();
        const int result = f.send(480);
        const auto elapsed = GetTickCount64() - before;
        check(result != SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS && f.h.function_status == SND_DEVICES_PLAYBACK_ERROR
            && elapsed >= 2000 && elapsed < 5000, "no-progress render wait returns after its existing two-second deadline");
    }
}

class TestCapture final : public IAudioCaptureClient
{
public:
    unsigned frames = 0, reads = 0, releases = 0;
    DWORD flags = 0;
    std::vector<float> input;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** out) override { if (out) *out = nullptr; return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE GetNextPacketSize(UINT32* n) override { *n = frames; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetBuffer(BYTE** data, UINT32* count, DWORD* outFlags, UINT64* position, UINT64* qpc) override {
        ++reads; *data = reinterpret_cast<BYTE*>(input.data()); *count = frames; *outFlags = flags;
        if (position) *position = 0; if (qpc) *qpc = 0; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE ReleaseBuffer(UINT32 count) override { if (count != frames) return E_FAIL; ++releases; return S_OK; }
};

void capturePacketCases(unsigned rate, unsigned channels, unsigned ratio)
{
    PlaybackFixture f(rate * ratio, channels);
    TestEndpoint captureClient;
    TestCapture capture;
    capture.frames = rate / 100; captureClient.queued = capture.frames;
    capture.input.resize(static_cast<std::size_t>(capture.frames) * channels);
    for (unsigned i = 0; i < capture.input.size(); ++i) capture.input[i] = static_cast<float>(i + 1);
    std::vector<float> scratch(capture.input.size()), output(capture.input.size() * ratio);
    f.h.pAudioClientCapture = &captureClient; f.h.pAudioCaptureLoopback = &capture;
    f.h.wfxCapture.nChannels = static_cast<WORD>(channels); f.h.wfxCapture.nSamplesPerSec = rate;
    f.h.wfxDfxProcessing = f.h.wfxPlayback; f.h.wfxDfxProcessing.nSamplesPerSec = rate;
    f.h.upsampleRatio = ratio; f.h.fCaptureBuf = scratch.data(); f.h.fPlaybackBuf = output.data();
    f.h.captureBufAllocSize = static_cast<int>(scratch.size()); f.h.playbackBufAllocSize = static_cast<int>(output.size());
    float* pcm = nullptr; WAVEFORMATEX* format = nullptr; int frames = 0, status = -1;
    const int result = sndDevicesDoCapture(reinterpret_cast<PT_HANDLE*>(&f.h), &pcm, &frames, &format, &status);
    check(result == OKAY && status == SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS && frames == static_cast<int>(capture.frames)
        && capture.reads == 1 && capture.releases == 1 && pcm == output.data() && format == &f.h.wfxDfxProcessing,
        "real capture returns one complete negotiated packet, not a half-buffer batch");
    status = -1;
    const int rendered = sndDevicesDoPlayback(reinterpret_cast<PT_HANDLE*>(&f.h), &status);
    std::vector<float> expected;
    for (unsigned i = 0; i < capture.frames; ++i)
        for (unsigned r = 0; r < ratio; ++r)
            expected.insert(expected.end(), capture.input.begin() + i * channels, capture.input.begin() + (i + 1) * channels);
    check(rendered == OKAY && status == SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS && sameBytes(f.ep.audio, expected),
        "capture-to-render rate conversion preserves channel interleaving and existing frame repetition");
    f.h.captureBufAllocSize = static_cast<int>(scratch.size()) - 1;
    const auto reads = capture.reads;
    status = -1;
    check(sndDevicesDoCapture(reinterpret_cast<PT_HANDLE*>(&f.h), &pcm, &frames, &format, &status) == OKAY
        && status == SND_DEVICES_CAPTURE_ERROR && capture.reads == reads,
        "oversize packet is rejected before acquiring or writing capture data");
}

void deferredStartCase()
{
    PlaybackFixture f(48000, 2);
    TestEndpoint captureClient;
    f.h.pAudioClientCapture = &captureClient;
    check(sndDevicesStartStopCapture(reinterpret_cast<PT_HANDLE*>(&f.h), SND_DEVICES_START_CAPTURE) == OKAY
        && captureClient.starts == 1 && f.ep.starts == 0 && f.h.playbackStreamIsTemporarilyPaused,
        "production startup starts capture and defers render until reserve is primed");
    check(f.send(480) == SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS && f.ep.starts == 1 && f.ep.queued == 4800,
        "first real render packet primes before starting playback");
    check(sndDevicesStartStopCapture(reinterpret_cast<PT_HANDLE*>(&f.h), SND_DEVICES_STOP_CAPTURE) == OKAY
        && captureClient.stops == 1 && f.ep.stops == 1 && f.ep.resets == 0 && f.h.stopAudioCaptureAndPlaybackLoop,
        "base stop contract is retained without a concurrent client Reset");
}

void failedStopCases()
{
    for (bool failCapture : {true, false})
    {
        PlaybackFixture f(48000, 2);
        TestEndpoint captureClient;
        f.h.pAudioClientCapture = &captureClient;
        check(sndDevicesStartStopCapture(reinterpret_cast<PT_HANDLE*>(&f.h), SND_DEVICES_START_CAPTURE) == OKAY
            && f.send(480) == SND_DEVICES_CAPTURE_PLAYBACK_SUCCESS, "failed-Stop fixture starts through the real production path");
        captureClient.failStop = failCapture;
        f.ep.failStop = !failCapture;
        bool flagSeen = true, rendererStopped = true;
        unsigned callbacks = 0;
        const auto inspectStop = [&]
        {
            ++callbacks;
            flagSeen &= f.h.stopAudioCaptureAndPlaybackLoop == 1;
            const auto writes = f.ep.writes, starts = f.ep.starts;
            int status = -1;
            const int result = sndDevicesDoPlayback(reinterpret_cast<PT_HANDLE*>(&f.h), &status);
            rendererStopped &= result == OKAY && status == SND_DEVICES_CAPTURE_FORCED_EXIT
                && f.ep.writes == writes && f.ep.starts == starts;
        };
        captureClient.onStop = inspectStop;
        f.ep.onStop = inspectStop;
        const int paused = f.h.playbackStreamIsTemporarilyPaused;
        const int result = sndDevicesStartStopCapture(reinterpret_cast<PT_HANDLE*>(&f.h), SND_DEVICES_STOP_CAPTURE);
        check(result == NOT_OKAY_NO_BREAK && flagSeen && callbacks == 2
            && captureClient.stops == 1 && f.ep.stops == 1 && f.h.stopAudioCaptureAndPlaybackLoop == 1,
            failCapture ? "capture Stop failure still signals before both COM Stop calls"
                        : "playback Stop failure still signals before both COM Stop calls");
        check(rendererStopped && f.h.playbackStreamIsTemporarilyPaused == paused && f.ep.resets == 0,
            "renderer invoked during failed Stop neither writes nor restarts, and owner leaves paused state untouched");
    }
}

void interruptedStartCases()
{
    {
        PlaybackFixture f(48000, 2);
        TestEndpoint captureClient;
        captureClient.failStart = true;
        f.h.pAudioClientCapture = &captureClient;
        check(sndDevicesStartStopCapture(reinterpret_cast<PT_HANDLE*>(&f.h), SND_DEVICES_START_CAPTURE) == NOT_OKAY_NO_BREAK
            && f.h.stopAudioCaptureAndPlaybackLoop == 1 && captureClient.starts == 0 && f.ep.starts == 0,
            "failed capture Start restores the stop request without starting render or invoking a modal diagnostic");
    }
    for (bool failStop : {false, true})
    {
        PlaybackFixture f(48000, 2);
        TestEndpoint captureClient;
        f.h.pAudioClientCapture = &captureClient;
        check(sndDevicesStartStopCapture(reinterpret_cast<PT_HANDLE*>(&f.h), SND_DEVICES_START_CAPTURE) == OKAY,
            "interrupted-Start fixture starts capture through the real production path");
        f.ep.failStop = failStop;
        int stopResult = -1;
        f.ep.onStart = [&] {
            // Model Start finishing after a control-thread Stop already returned.
            stopResult = sndDevicesStartStopCapture(reinterpret_cast<PT_HANDLE*>(&f.h), SND_DEVICES_STOP_CAPTURE);
        };
        const int result = f.send(480);
        check(result == (failStop ? SND_DEVICES_PLAYBACK_ERROR : SND_DEVICES_CAPTURE_FORCED_EXIT)
            && stopResult == (failStop ? NOT_OKAY_NO_BREAK : OKAY) && f.ep.starts == 1 && f.ep.stops == 2
            && f.ep.running == failStop && f.ep.resets == 0 && f.h.playbackStreamIsTemporarilyPaused,
            failStop ? "failed compensating Stop reports an error without a modal or a false stopped claim"
                     : "Stop during Start is checked after completion and compensated before returning");
    }
}

void failedStopStillJoins()
{
    struct Gates {
        HANDLE entered = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        HANDLE stop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        ~Gates() { CloseHandle(entered); CloseHandle(stop); }
    };
    struct InertOwner {
        AudioPassthruPrivate audio;
        InertOwner() { PlaybackReserveTestPeer::inert(audio); }
        ~InertOwner() { PlaybackReserveTestPeer::inert(audio); }
    };
    for (bool failCapture : {true, false})
    {
        InertOwner owner; // Destructor cannot restore a device or access a user profile.
        PlaybackFixture f(48000, 2);
        TestEndpoint captureClient;
        f.h.pAudioClientCapture = &captureClient;
        captureClient.failStop = failCapture; f.ep.failStop = !failCapture;
        const auto gates = std::make_shared<Gates>();
        check(gates->entered != nullptr && gates->stop != nullptr, "join fixture creates only local synchronization events");
        if (!gates->entered || !gates->stop) continue;
        std::thread worker([gates] {
            SetEvent(gates->entered);
            WaitForSingleObject(gates->stop, 5000);
        });
        HANDLE ownedThread = nullptr;
        const bool duplicated = DuplicateHandle(GetCurrentProcess(), worker.native_handle(), GetCurrentProcess(),
            &ownedThread, 0, FALSE, DUPLICATE_SAME_ACCESS) != FALSE;
        const bool entered = WaitForSingleObject(gates->entered, 1000) == WAIT_OBJECT_0;
        check(duplicated && entered, "join fixture observes a real running worker using its own duplicated handle");
        if (!duplicated || !entered) {
            SetEvent(gates->stop); worker.join(); if (ownedThread) CloseHandle(ownedThread); continue;
        }
        PlaybackReserveTestPeer::attach(owner.audio, f.h, ownedThread);
        bool signalledBeforeStop = true;
        const auto stopWorker = [&] {
            signalledBeforeStop &= PlaybackReserveTestPeer::killRequested(owner.audio)
                && f.h.stopAudioCaptureAndPlaybackLoop == 1;
            SetEvent(gates->stop);
        };
        captureClient.onStop = stopWorker; f.ep.onStop = stopWorker;
        int timedOut = -1;
        const int result = owner.audio.killProcessingThread(&timedOut);
        const bool joined = PlaybackReserveTestPeer::threadHandle(owner.audio) == nullptr;
        SetEvent(gates->stop); worker.join();
        check(result == OKAY && timedOut == IS_FALSE && joined && signalledBeforeStop,
            failCapture ? "real kill joins and closes its worker handle even when capture Stop fails"
                        : "real kill joins and closes its worker handle even when playback Stop fails");
        if (!joined) CloseHandle(ownedThread);
        PlaybackReserveTestPeer::inert(owner.audio);
    }
}
}

int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    check(fxsoundtiming::defaultReserveMs == 90 && fxsoundtiming::maxReserveMs == 300, "shared runtime default and maximum are 90/300 ms");
    check(adjustablePlaybackCases() == 0, "original private renderer transition/reversal regression remains valid");
    deferredStartCase();
    failedStopCases();
    interruptedStartCases();
    failedStopStillJoins();
    for (unsigned rate : {44100u, 48000u, 96000u})
        for (unsigned channels : {1u, 2u, 6u, 8u}) rateAndChannelCases(rate, channels);
    capacityStopAndErrors();
    for (unsigned rate : {44100u, 48000u, 96000u})
        for (unsigned channels : {1u, 2u, 6u, 8u}) capturePacketCases(rate, channels, 1);
    capturePacketCases(48000, 2, 2);
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
