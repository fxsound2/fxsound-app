/*
FxSound
Copyright (C) 2025  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
// Exercises the actual MMDevice callbacks and cleanup with in-memory fixtures.
// No endpoint enumeration, audio stream, user settings or UI is opened.
#include <windows.h>
#include "codedefs.h"
#include "sndDevices.h"
#include "u_AudioPassthru.h"
#include <array>
#include <atomic>
#include <cstdio>
#include <cwchar>
#include <memory>
#include <thread>
#include <vector>
namespace fxsounddevices
{
struct DeviceChangeQueueTestPeer
{
    static void holdStateCache(DeviceChangeQueue& queue) { queue.stateCacheBusy_.test_and_set(std::memory_order_acquire); }
    static void releaseStateCache(DeviceChangeQueue& queue) { queue.stateCacheBusy_.clear(std::memory_order_release); }
};
}
using Queue = fxsounddevices::DeviceChangeQueue;
std::atomic<int> checks{0}, failures{0};
void check(bool ok, const char* name) { ++checks; if (!ok) ++failures; std::printf("%s %s\n", ok ? "PASS" : "FAIL", name); }
sndDevicesHdlType* current = nullptr;
int wakes = 0;
bool stopBeforeWake = true, queuedBeforeWake = true;
void wake() { ++wakes; stopBeforeWake &= current->stopAudioCaptureAndPlaybackLoop.load() == 1; queuedBeforeWake &= current->deviceChanges.hasPending(); }

void callbackTests()
{
    auto h = std::make_unique<sndDevicesHdlType>();
    current = h.get();
    h->DeviceEvents.SetPtHandle(reinterpret_cast<PT_HANDLE*>(h.get()));
    h->deviceChangeCallback = wake;
    h->totalNumDevices = 4;
    h->dfxDeviceNum = 0;
    h->defaultDeviceNum = 1;
    wcscpy_s(h->pwszID[0], L"virtual");
    wcscpy_s(h->pwszID[1], L"physical-A");
    wcscpy_s(h->pwszID[2], L"physical-B"); // Slot 3 deliberately empty.
    h->deviceChanges.setVirtualDeviceId(L"virtual");
    h->EPVolEventsCapture.AddRef();
    check(!h->EPVolEventsCapture.IsRegistered(), "IUnknown reference count is not volume registration state");
    h->EPVolEventsCapture.Release();
    h->EPVolEventsCapture.SetRegistered(true);
    check(h->EPVolEventsCapture.IsRegistered(), "explicit registration state survives independent IUnknown references");
    h->EPVolEventsCapture.SetRegistered(false);
    auto& cb = h->DeviceEvents;
    cb.OnDefaultDeviceChanged(eCapture, eMultimedia, L"capture");
    cb.OnDefaultDeviceChanged(eRender, eCommunications, L"communications");
    check(wakes == 0 && !h->deviceChanges.hasPending(), "ignore other roles and capture defaults");
    cb.OnDefaultDeviceChanged(eRender, eMultimedia, L"virtual");
    check(wakes == 0 && h->stopAudioCaptureAndPlaybackLoop == 0, "ignore own virtual default without restarting");
    wchar_t borrowed[32] = L"physical-B";
    cb.OnDefaultDeviceChanged(eRender, eMultimedia, borrowed);
    wcscpy_s(borrowed, L"changed-after-return");
    Queue::Event event;
    check(h->deviceChanges.tryPop(event) && std::wcscmp(event.id, L"physical-B") == 0, "borrowed endpoint ID copied before callback returns");
    check(stopBeforeWake && queuedBeforeWake, "stop and queued payload precede wake");
    BOOL changed = FALSE;
    check(sndDevicesApplyDeviceChange(reinterpret_cast<PT_HANDLE*>(h.get()), event, &changed) == OKAY
        && changed && h->defaultDeviceNum == 2, "owner applies physical default including an empty slot");
    cb.OnDefaultDeviceChanged(eRender, eMultimedia, L"virtual");
    check(!h->deviceChanges.hasPending() && h->defaultDeviceNum == 2, "self default cannot overwrite pending physical target");
        cb.OnDefaultDeviceChanged(eRender, eMultimedia, L"physical-A");
    cb.OnDefaultDeviceChanged(eRender, eMultimedia, L"virtual");
    check(h->deviceChanges.tryPop(event) && std::wcscmp(event.id, L"physical-A") == 0
        && !h->deviceChanges.hasPending(), "virtual event cannot clobber an undelivered physical default");
    sndDevicesApplyDeviceChange(reinterpret_cast<PT_HANDLE*>(h.get()), event, &changed);
    cb.OnDefaultDeviceChanged(eRender, eMultimedia, nullptr);
    check(h->deviceChanges.tryPop(event) && event.missingId, "missing default endpoint retained");
    sndDevicesApplyDeviceChange(reinterpret_cast<PT_HANDLE*>(h.get()), event, &changed);
    check(changed && h->defaultDeviceNum == SND_DEVICES_DEVICE_NOT_PRESENT, "missing default clears selection");
    cb.OnDefaultDeviceChanged(eRender, eMultimedia, L"unknown");
    h->deviceChanges.tryPop(event);
    sndDevicesApplyDeviceChange(reinterpret_cast<PT_HANDLE*>(h.get()), event, &changed);
    check(changed && h->defaultDeviceNum == SND_DEVICES_DEVICE_NOT_PRESENT, "unknown default requests reconciliation without indexing missing entry");
    for (DWORD state : {DEVICE_STATE_ACTIVE, DEVICE_STATE_ACTIVE, DEVICE_STATE_UNPLUGGED, DEVICE_STATE_ACTIVE})
        cb.OnDeviceStateChanged(L"physical-B", state);
    int applied = 0;
    while (h->deviceChanges.tryPop(event)) { sndDevicesApplyDeviceChange(reinterpret_cast<PT_HANDLE*>(h.get()), event, &changed); applied += changed != FALSE; }
    check(applied == 3, "owner suppresses duplicate state but preserves unplug and reconnect");
        h->stopAudioCaptureAndPlaybackLoop = 0;
    const int beforeDuplicate = wakes;
    cb.OnDeviceStateChanged(L"physical-B", DEVICE_STATE_ACTIVE);
    check(h->stopAudioCaptureAndPlaybackLoop == 0 && wakes == beforeDuplicate && !h->deviceChanges.hasPending(),
        "late repeated state neither stops audio nor wakes the owner");
    bool everyTransitionStops = true;
    for (DWORD state : {DEVICE_STATE_UNPLUGGED, DEVICE_STATE_ACTIVE})
    {
        h->stopAudioCaptureAndPlaybackLoop = 0;
        cb.OnDeviceStateChanged(L"physical-B", state);
        everyTransitionStops &= h->stopAudioCaptureAndPlaybackLoop == 1 && h->deviceChanges.tryPop(event) && event.state == state;
    }
    check(everyTransitionStops, "unplug and active transition each stop immediately");
    h->deviceChangeCallback = nullptr;
    h->stopAudioCaptureAndPlaybackLoop = 0;
    cb.OnDeviceStateChanged(L"contended-A", DEVICE_STATE_ACTIVE);
    bool contendedStops = h->stopAudioCaptureAndPlaybackLoop == 1;
    fxsounddevices::DeviceChangeQueueTestPeer::holdStateCache(h->deviceChanges);
    h->stopAudioCaptureAndPlaybackLoop = 0;
    std::thread contender([&] { cb.OnDeviceStateChanged(L"contended-B", DEVICE_STATE_UNPLUGGED); });
    contender.join(); contendedStops &= h->stopAudioCaptureAndPlaybackLoop == 1;
    h->stopAudioCaptureAndPlaybackLoop = 0;
    cb.OnDeviceStateChanged(L"contended-A", DEVICE_STATE_ACTIVE);
    contendedStops &= h->stopAudioCaptureAndPlaybackLoop == 1;
    fxsounddevices::DeviceChangeQueueTestPeer::releaseStateCache(h->deviceChanges);
    bool allThree = true;
    for (const auto* expected : {L"contended-A", L"contended-B", L"contended-A"})
        allThree &= h->deviceChanges.tryPop(event) && std::wcscmp(event.id, expected) == 0;
    check(contendedStops && allThree && !h->deviceChanges.hasPending(), "contended state cache conservatively preserves real A-B-A callbacks");
    h->stopAudioCaptureAndPlaybackLoop = 0;
    cb.OnDeviceStateChanged(L"contended-A", DEVICE_STATE_ACTIVE);
    check(h->stopAudioCaptureAndPlaybackLoop == 1 && h->deviceChanges.tryPop(event), "contention epoch invalidates the formerly cached state");
    h->stopAudioCaptureAndPlaybackLoop = 0;
    cb.OnDeviceStateChanged(L"contended-A", DEVICE_STATE_ACTIVE);
    check(h->stopAudioCaptureAndPlaybackLoop == 0 && !h->deviceChanges.hasPending(), "uncontended state cache resumes duplicate suppression");
    h->deviceChangeCallback = wake;
    bool defaultsAlwaysStop = true;
    for (int repeat = 0; repeat < 2; ++repeat)
    {
        h->stopAudioCaptureAndPlaybackLoop = 0;
        cb.OnDefaultDeviceChanged(eRender, eMultimedia, L"physical-B");
        defaultsAlwaysStop &= h->stopAudioCaptureAndPlaybackLoop == 1 && h->deviceChanges.tryPop(event);
    }
    check(defaultsAlwaysStop, "same physical default ID is never deduplicated by the state cache");
    bool missingStatesStop = true;
    for (int repeat = 0; repeat < 2; ++repeat)
    {
        h->stopAudioCaptureAndPlaybackLoop = 0;
        cb.OnDeviceStateChanged(nullptr, DEVICE_STATE_ACTIVE);
        missingStatesStop &= h->stopAudioCaptureAndPlaybackLoop == 1 && h->deviceChanges.tryPop(event) && event.missingId;
    }
    check(missingStatesStop, "state without an ID is never deduplicated");
    std::array<wchar_t, Queue::idCapacity + 1> tooLong; tooLong.fill(L'x'); tooLong.back() = L'\0';
    bool oversizedStatesStop = true;
    for (int repeat = 0; repeat < 2; ++repeat)
    {
        h->stopAudioCaptureAndPlaybackLoop = 0;
        cb.OnDeviceStateChanged(tooLong.data(), DEVICE_STATE_ACTIVE);
        oversizedStatesStop &= h->stopAudioCaptureAndPlaybackLoop == 1 && h->deviceChanges.takeReconcileRequired()
            && !h->deviceChanges.tryPop(event);
    }
    check(oversizedStatesStop, "oversize state ID always stops and requests explicit reconciliation");
    cb.OnDeviceAdded(L"added");
    cb.OnDeviceRemoved(L"removed");
    check(h->deviceChanges.tryPop(event) && event.kind == Queue::Kind::added
        && h->deviceChanges.tryPop(event) && event.kind == Queue::Kind::removed
        && stopBeforeWake && queuedBeforeWake, "added and removed callbacks stop before ordered delivery");
    h->deviceChangeCallback = nullptr;
    h->stopAudioCaptureAndPlaybackLoop = 0;
    cb.OnDeviceAdded(L"before-window");
    check(h->stopAudioCaptureAndPlaybackLoop == 1 && h->deviceChanges.tryPop(event), "stop and payload survive missing notification window");
    h->stopAudioCaptureAndPlaybackLoop = 0;
    cb.OnPropertyValueChanged(L"physical-B", PROPERTYKEY{});
    check(h->stopAudioCaptureAndPlaybackLoop == 0 && !h->deviceChanges.hasPending(), "property mute notification retains legacy no-stop policy");
    h->deviceNotificationsClosed = true;
    cb.OnDeviceRemoved(L"late");
    check(h->stopAudioCaptureAndPlaybackLoop == 0 && !h->deviceChanges.hasPending()
        && h->deviceCallbacksInFlight == 0, "late callback after close touches no audio state");
}
void volumeCallbackTests()
{
    auto h = std::make_unique<sndDevicesHdlType>();
    h->EPVolEventsCapture.SetPtHandle(reinterpret_cast<PT_HANDLE*>(h.get()));
    h->EPVolEventsPlayback.SetPtHandle(reinterpret_cast<PT_HANDLE*>(h.get()));
    h->pEndptVolCapture = reinterpret_cast<IAudioEndpointVolume*>(1);
    h->pEndptVolPlayback = reinterpret_cast<IAudioEndpointVolume*>(1);
    AUDIO_VOLUME_NOTIFICATION_DATA notice{};
    notice.guidEventContext.Data1 = 1; // An external notification, not our own context.
    h->deviceNotificationsClosed = true;
    check(h->EPVolEventsCapture.OnNotify(&notice) == S_OK && h->deviceCallbacksInFlight == 0,
        "closed capture-volume callback accesses no endpoint or mute state");
    check(h->EPVolEventsPlayback.OnNotify(&notice) == S_OK && h->deviceCallbacksInFlight == 0,
        "closed playback-volume callback cannot propagate restored physical mute");
    h->deviceNotificationsClosed = false;
    h->ignoreVolumeCallbacks = TRUE;
    check(h->EPVolEventsCapture.OnNotify(&notice) == S_OK && h->EPVolEventsPlayback.OnNotify(&notice) == S_OK
        && h->deviceCallbacksInFlight == 0, "ignored volume callbacks leave the shared in-flight count balanced");
    h->pEndptVolCapture = nullptr; h->pEndptVolPlayback = nullptr;
}
void queueTests()
{
    Queue q;
    Queue::Event event;
    bool filled = true; for (std::size_t i = 0; i < Queue::capacity; ++i) filled &= q.enqueue(Queue::Kind::added, L"bounded");
    check(filled, "all bounded capacity slots accept complete records");
    check(!q.enqueue(Queue::Kind::added, L"overflow") && q.droppedEvents() == 1 && q.takeReconcileRequired(), "overflow explicitly requests reconciliation");
    unsigned popped = 0; while (q.tryPop(event)) ++popped;
    check(popped == Queue::capacity && !q.hasPending(), "overflow leaves complete queued records intact");
    std::array<wchar_t, Queue::idCapacity + 2> longId; longId.fill(L'x'); longId.back() = L'\0';
    check(!q.enqueue(Queue::Kind::defaultChanged, longId.data()) && !q.tryPop(event) && q.takeReconcileRequired(), "oversize ID never published truncated");
    check(q.setVirtualDeviceId(L"old") && q.isVirtualDevice(L"old"), "publish initial immutable identity");
    check(q.setVirtualDeviceId(L"new") && q.isVirtualDevice(L"new") && !q.isVirtualDevice(L"old"), "publish replacement immutable identity");
        bool emptyBoundary = true;
    for (unsigned i = 0; i < 50; ++i)
    {
        std::atomic<bool> emptyObserved{false};
        std::thread producer([&] {
            while (!emptyObserved.load()) std::this_thread::yield();
            q.enqueue(Queue::Kind::added, L"after-empty");
        });
        emptyBoundary &= !q.tryPop(event) && !q.hasPending();
        emptyObserved = true;
        producer.join();
        emptyBoundary &= q.hasPending() && q.tryPop(event) && !q.hasPending();
    }
    check(emptyBoundary, "enqueue after an empty drain reestablishes pending and remains consumable");
    std::atomic<unsigned> done{0}, accepted{0};
    constexpr unsigned writers = 4, perWriter = 500;
    std::array<bool, writers * perWriter> seen{};
    std::vector<std::thread> threads;
    for (unsigned w = 0; w < writers; ++w) threads.emplace_back([&, w] {
        for (unsigned i = 0; i < perWriter; ++i) {
            wchar_t id[64]; swprintf_s(id, L"event-%u-%u", w, i);
            if (q.enqueue(Queue::Kind::added, id, w * perWriter + i)) ++accepted;
        }
        ++done;
    });
    unsigned received = 0; bool intact = true;
    while (done.load() != writers || q.hasPending()) {
        q.takeReconcileRequired();
        if (q.tryPop(event)) {
            unsigned w = 0, i = 0;
            const bool valid = swscanf_s(event.id, L"event-%u-%u", &w, &i) == 2 && w < writers && i < perWriter;
            intact &= valid;
            if (valid) { const unsigned index = w * perWriter + i; intact &= !seen[index] && event.state == index; seen[index] = true; }
            ++received;
        } else std::this_thread::yield();
    }
    for (auto& thread : threads) thread.join();
    check(intact && received == accepted.load(), "concurrent producers publish whole unique records across wraparound");
}
struct FakeEnumerator : IMMDeviceEnumerator
{
    sndDevicesHdlType* handle;
    std::atomic<bool> unregistered{false};
    unsigned releases = 0;
    HRESULT result = S_OK;
    explicit FakeEnumerator(sndDevicesHdlType* value) : handle(value) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** value) override { if (value) *value = nullptr; return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef() override { return 2; }
    ULONG STDMETHODCALLTYPE Release() override { ++releases; return 0; }
    HRESULT STDMETHODCALLTYPE EnumAudioEndpoints(EDataFlow, DWORD, IMMDeviceCollection**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetDefaultAudioEndpoint(EDataFlow, ERole, IMMDevice**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetDevice(LPCWSTR, IMMDevice**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE RegisterEndpointNotificationCallback(IMMNotificationClient*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE UnregisterEndpointNotificationCallback(IMMNotificationClient* value) override {
        check(value == &handle->DeviceEvents && handle->deviceNotificationsClosed.load()
            && handle->deviceChangeCallback.load() == nullptr, "unregister original client after closing producer");
        unregistered = true; return result;
    }
};
struct FakeVolume : IAudioEndpointVolume
{
    std::atomic<bool> entered{false}, proceed{false};
    std::atomic<unsigned> releases{0};
    HRESULT unregisterResult = S_OK;
    std::atomic<unsigned> unregisterCalls{0};
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void**) override {return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef() override {return 1;}
    ULONG STDMETHODCALLTYPE Release() override {return ++releases;}
    HRESULT STDMETHODCALLTYPE RegisterControlChangeNotify(IAudioEndpointVolumeCallback*) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE UnregisterControlChangeNotify(IAudioEndpointVolumeCallback*) override {++unregisterCalls;return unregisterResult;}
    HRESULT STDMETHODCALLTYPE GetChannelCount(UINT*) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetMasterVolumeLevel(float,LPCGUID) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetMasterVolumeLevelScalar(float,LPCGUID) override {return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMasterVolumeLevel(float*) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetMasterVolumeLevelScalar(float* value) override {
        entered = true; const auto deadline = GetTickCount64() + 2000;
        while (!proceed.load() && GetTickCount64() < deadline) std::this_thread::yield();
        *value = 0.5f; return proceed.load() ? S_OK : E_FAIL;
    }
    HRESULT STDMETHODCALLTYPE SetChannelVolumeLevel(UINT,float,LPCGUID) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetChannelVolumeLevelScalar(UINT,float,LPCGUID) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetChannelVolumeLevel(UINT,float*) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetChannelVolumeLevelScalar(UINT,float*) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetMute(BOOL,LPCGUID) override {return S_OK;}
    HRESULT STDMETHODCALLTYPE GetMute(BOOL* value) override {*value=FALSE;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetVolumeStepInfo(UINT*,UINT*) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE VolumeStepUp(LPCGUID) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE VolumeStepDown(LPCGUID) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE QueryHardwareSupport(DWORD*) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetVolumeRange(float*,float*,float*) override {return E_NOTIMPL;}
};
void liveVolumeDrainTest()
{
    auto h = std::make_unique<sndDevicesHdlType>();
    FakeVolume endpoint;
    FakeEnumerator enumerator(h.get());
    h->notificationEnumerator = &enumerator;
    h->pEndptVolCapture = &endpoint; h->pEndptVolPlayback = &endpoint;
    h->EPVolEventsCapture.SetPtHandle(reinterpret_cast<PT_HANDLE*>(h.get()));
    AUDIO_VOLUME_NOTIFICATION_DATA notice{}; notice.guidEventContext.Data1=1;
    HRESULT callbackResult = E_FAIL;
    std::thread callback([&] {callbackResult=h->EPVolEventsCapture.OnNotify(&notice);});
    const auto deadline = GetTickCount64() + 2000;
    while (!endpoint.entered.load() && GetTickCount64()<deadline) std::this_thread::yield();
    check(endpoint.entered && h->deviceCallbacksInFlight == 1, "real volume callback owns an in-flight guard while in COM");
    h->deviceNotificationsClosed = true;
    int cleanupResult = -1;
    std::thread cleanup([&] {cleanupResult=sndDevicesFree(reinterpret_cast<PT_HANDLE*>(h.get()));});
    check(WaitForSingleObject(cleanup.native_handle(), 20) == WAIT_TIMEOUT && endpoint.releases == 0,
        "cleanup cannot release endpoint while an entered volume callback uses it");
    endpoint.proceed = true;
    callback.join(); cleanup.join();
    check(callbackResult == S_OK && cleanupResult == OKAY && h->deviceCallbacksInFlight == 0
        && endpoint.releases == 2, "entered volume callback finishes before endpoint release");
}
void volumeUnregisterFailureTests()
{
    for (bool captureFailure : {true, false})
    {
        auto h = std::make_unique<sndDevicesHdlType>();
        FakeVolume capture, playback;
        capture.unregisterResult = captureFailure ? E_FAIL : S_OK;
        playback.unregisterResult = captureFailure ? S_OK : E_FAIL;
        h->pEndptVolCapture = &capture; h->pEndptVolPlayback = &playback;
        h->EPVolEventsCapture.SetPtHandle(reinterpret_cast<PT_HANDLE*>(h.get()));
        h->EPVolEventsPlayback.SetPtHandle(reinterpret_cast<PT_HANDLE*>(h.get()));
        h->EPVolEventsCapture.AddRef(); h->EPVolEventsPlayback.AddRef();
        h->EPVolEventsCapture.SetRegistered(true); h->EPVolEventsPlayback.SetRegistered(true);
        const int result = sndDevicesFree(reinterpret_cast<PT_HANDLE*>(h.get()));
        check(result == NOT_OKAY_NO_BREAK && capture.releases == 0 && playback.releases == 0
            && h->pEndptVolCapture == &capture && h->pEndptVolPlayback == &playback,
            captureFailure ? "capture-volume unregister failure propagates and retains both endpoints"
                           : "playback-volume unregister failure propagates and retains both endpoints");
        AUDIO_VOLUME_NOTIFICATION_DATA notice{}; notice.guidEventContext.Data1 = 1;
        check(h->EPVolEventsCapture.OnNotify(&notice) == S_OK && h->EPVolEventsPlayback.OnNotify(&notice) == S_OK
            && !capture.entered && !playback.entered, "failed-unregister late volume callbacks see closed process-lifetime state");
        capture.unregisterResult = S_OK; playback.unregisterResult = S_OK;
        const int retry = sndDevicesFree(reinterpret_cast<PT_HANDLE*>(h.get()));
        check(retry == OKAY && capture.releases == 1 && playback.releases == 1
            && !h->EPVolEventsCapture.IsRegistered() && !h->EPVolEventsPlayback.IsRegistered()
            && capture.unregisterCalls == (captureFailure ? 2u : 1u)
            && playback.unregisterCalls == (captureFailure ? 1u : 2u),
            "retry retires only remaining registrations and releases endpoints once");
    }
}
void cleanupTests()
{
    auto h = std::make_unique<sndDevicesHdlType>();
    FakeEnumerator enumerator(h.get());
    h->notificationEnumerator = &enumerator;
    h->deviceCallbacksInFlight = 1;
    int result = -1;
    std::thread cleanup([&] { result = sndDevicesFree(reinterpret_cast<PT_HANDLE*>(h.get())); });
    const auto deadline = GetTickCount64() + 2000;
    while (!enumerator.unregistered.load() && GetTickCount64() < deadline) std::this_thread::yield();
    check(enumerator.unregistered.load(), "unregister reached within test deadline");
    check(enumerator.releases == 0, "enumerator remains alive while a callback is in flight");
    h->deviceCallbacksInFlight = 0;
    cleanup.join();
    check(result == OKAY && enumerator.releases == 1 && h->notificationEnumerator == nullptr, "cleanup drains callbacks before releasing original enumerator");
    FakeEnumerator failed(h.get()); failed.result = E_FAIL; h->notificationEnumerator = &failed;
    check(sndDevicesFree(reinterpret_cast<PT_HANDLE*>(h.get())) == NOT_OKAY_NO_BREAK && failed.releases == 0,
        "unregister failure retains owned resources instead of unsafe cleanup");
    h->notificationEnumerator = nullptr;
}
struct AudioPassthruTestPeer
{
    static std::atomic<unsigned> retries;
    static void CALLBACK observedRetry(PTP_CALLBACK_INSTANCE instance, void* context, PTP_TIMER timer) noexcept {
        ++retries;
        AudioPassthruPrivate::retryDeviceNotice(instance, context, timer);
    }
    static void prepare(AudioPassthruPrivate& audio) {
        auto& device = AudioPassthruPrivate::s_sndDevices_;
        device.deviceNotificationsClosed = false;
        device.ignoreDeviceCallbacks = FALSE;
        device.ignoreVolumeCallbacks = FALSE;
        AudioPassthruPrivate::s_device_change_pending_ = false;
        AudioPassthruPrivate::s_device_message_posted_ = false;
        AudioPassthruPrivate::s_retry_armed_ = false;
        AudioPassthruPrivate::s_notification_window_ = nullptr;
        retries = 0;
        AudioPassthruPrivate::s_device_retry_ = CreateThreadpoolTimer(observedRetry, nullptr, nullptr);
        check(AudioPassthruPrivate::s_device_retry_ != nullptr, "native retry timer created without audio initialization");
    }
    static void notify() { AudioPassthruPrivate::onDeviceChange(); }
    static bool pending() { return AudioPassthruPrivate::s_device_change_pending_.load(); }
    static void acknowledgeWithoutEnumeration() {
        AudioPassthruPrivate::s_device_change_pending_ = false;
        AudioPassthruPrivate::s_device_message_posted_ = false;
    }
    static void close(AudioPassthruPrivate& audio) { audio.requestStop(); audio.closeDeviceRetry(); }
};
std::atomic<unsigned> AudioPassthruTestPeer::retries{0};

void notificationWakeTests()
{
    const auto module = GetModuleHandleW(nullptr);
    WNDCLASSW windowClass{}; windowClass.lpfnWndProc = DefWindowProcW;
    windowClass.hInstance = module; windowClass.lpszClassName = L"FxSoundNotificationFixture";
    const ATOM atom = RegisterClassW(&windowClass);
    const HWND window = CreateWindowExW(0, windowClass.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, module, nullptr);
    check(atom != 0 && window != nullptr, "hidden message-only test window created");
    if (window == nullptr) return;
    auto take = [&]() { MSG message{}; return PeekMessageW(&message, window, AudioPassthru::deviceChangeMessage,
        AudioPassthru::deviceChangeMessage, PM_REMOVE) != FALSE; };
    {
        AudioPassthruPrivate audio;
        AudioPassthruTestPeer::prepare(audio);
        AudioPassthruTestPeer::notify();
        check(AudioPassthruTestPeer::pending() && !take(), "production wake retains pending before window attachment");
        audio.setDeviceNotificationWindow(window);
        check(take(), "attaching real window delivers retained notification");
        AudioPassthruTestPeer::notify(); AudioPassthruTestPeer::notify();
        check(!take(), "repeated production wake coalesces while delivery is outstanding");
        AudioPassthruTestPeer::acknowledgeWithoutEnumeration();
        Sleep(40);
        check(AudioPassthruTestPeer::retries == 0 && !take(), "successful delivery has no permanent retry timer");
        AudioPassthruTestPeer::close(audio);
    }
    {
        AudioPassthruPrivate audio;
        AudioPassthruTestPeer::prepare(audio);
        audio.setDeviceNotificationWindow(reinterpret_cast<HWND>(1)); // Deliberately invalid, no external window.
        AudioPassthruTestPeer::notify();
        const auto deadline = GetTickCount64() + 1000;
        while (AudioPassthruTestPeer::retries.load() == 0 && GetTickCount64() < deadline) Sleep(1);
        check(AudioPassthruTestPeer::retries > 0 && AudioPassthruTestPeer::pending(), "failed real PostMessage arms and runs production one-shot retry");
        audio.setDeviceNotificationWindow(window);
        check(take(), "reattaching valid window delivers after failed post and retry");
        AudioPassthruTestPeer::acknowledgeWithoutEnumeration();
        Sleep(40); const auto idleCount = AudioPassthruTestPeer::retries.load();
        Sleep(40);
        check(AudioPassthruTestPeer::retries == idleCount && !take(), "one-shot retry stops after acknowledged delivery");
        AudioPassthruTestPeer::close(audio);
    }
    {
        AudioPassthruPrivate audio;
        AudioPassthruTestPeer::prepare(audio);
        audio.setDeviceNotificationWindow(reinterpret_cast<HWND>(1));
        AudioPassthruTestPeer::notify();
        const auto deadline = GetTickCount64() + 1000;
        while (AudioPassthruTestPeer::retries.load() == 0 && GetTickCount64() < deadline) Sleep(1);
        AudioPassthruTestPeer::close(audio);
        const auto closedCount = AudioPassthruTestPeer::retries.load();
        AudioPassthruTestPeer::notify(); Sleep(40);
        check(AudioPassthruTestPeer::retries == closedCount && !take(), "closing during retry drains callbacks and prevents later posts");
    }
    DestroyWindow(window); UnregisterClassW(windowClass.lpszClassName, module);
}
int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    callbackTests(); volumeCallbackTests(); queueTests(); liveVolumeDrainTest(); volumeUnregisterFailureTests(); cleanupTests(); notificationWakeTests();
    std::printf("RESULT: %d/%d passed\n", checks.load() - failures.load(), checks.load());
    return failures.load() ? 1 : 0;
}
