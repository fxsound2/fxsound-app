/* FxSound — AGPL-3.0-or-later. */
// Real owner dispatch/processTimer and real MMDevice callbacks; only endpoint,
// registry, clock and worker creation boundaries are replaced in this TU.
// No public wrapper, init(), shutdown(), device activation or profile is used.
#include <windows.h>
#include "codedefs.h"
#include "sndDevices.h"
#include "u_AudioPassthru.h"
#include "DeviceCatalog.h"
#include "reg.h"
#include "../../fxsound/Source/GUI/OutputRecovery.h"
#include <Functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <functional>
#include <memory>
#include <new>
#include <thread>

namespace fixture
{
struct Backend
{
    std::vector<SoundDevice> devices;
    std::wstring actualDefault = L"A", engineOutput = L"A";
    ULONGLONG now = 0;
    int reads = 0, getAll = 0, reinit = 0, restored = 0, registry = 0, forbidden = 0;
    int failedReads = 0;
    std::vector<std::wstring> selected, started;
    std::function<void()> duringGetAll, duringSet, duringReinit;
};
Backend* backend = nullptr;
SoundDevice descriptor(const wchar_t* id, bool active = true)
{
    SoundDevice d{};
    d.pwszID = id; d.deviceFriendlyName = id; d.deviceDescription = L"test";
    d.deviceFormFactor = L"Speakers"; d.deviceNumChannel = 2; d.isActive = active;
    d.isDFXDevice = d.pwszID == L"V"; d.isRealDevice = !d.isDFXDevice;
    return d;
}
void fill(sndDevicesHdlType& h)
{
    h.totalNumDevices = static_cast<int>(backend->devices.size()); h.numRealDevices = 0;
    h.dfxDeviceNum = h.captureDeviceNum = h.defaultDeviceNum = h.playbackDeviceNum = SND_DEVICES_DEVICE_NOT_PRESENT;
    for (int i = 0; i < h.totalNumDevices; ++i)
    {
        const auto& d = backend->devices[i];
        wcscpy_s(h.pwszID[i], d.pwszID.c_str());
        wcscpy_s(h.deviceFriendlyName[i], d.deviceFriendlyName.c_str());
        wcscpy_s(h.deviceDescription[i], d.deviceDescription.c_str());
        wcscpy_s(h.deviceFormFactor[i], d.deviceFormFactor.c_str());
        h.deviceState[i] = d.isActive ? DEVICE_STATE_ACTIVE : DEVICE_STATE_UNPLUGGED;
        h.deviceNumChannel[i] = d.deviceNumChannel;
        if (d.isDFXDevice) h.dfxDeviceNum = h.captureDeviceNum = i;
        else h.pwszIDRealDevices[h.numRealDevices++] = h.pwszID[i];
        if (d.pwszID == backend->actualDefault) h.defaultDeviceNum = i;
        if (d.pwszID == backend->engineOutput) h.playbackDeviceNum = i;
    }
}
void invokeOnce(std::function<void()>& action) { auto copy = std::move(action); action = {}; if (copy) copy(); }
int getAll(PT_HANDLE* raw, int* count)
{
    ++backend->getAll; fill(*reinterpret_cast<sndDevicesHdlType*>(raw));
    invokeOnce(backend->duringGetAll); *count = static_cast<int>(backend->devices.size()); return OKAY;
}
int reinit(PT_HANDLE* raw, int, int* count, int* enabled, int* status)
{
    ++backend->reinit;
    // The real ReInit owns GetAll/default rules. This seam records which CURRENT
    // Windows default it is given, rather than replaying an old event payload.
    backend->engineOutput = backend->actualDefault;
    fill(*reinterpret_cast<sndDevicesHdlType*>(raw));
    *count = static_cast<int>(backend->devices.size()) - 1; *enabled = IS_TRUE;
    *status = SND_DEVICES_DEVICE_OPERATION_COMPLETED;
    invokeOnce(backend->duringReinit); return OKAY;
}
int getId(PT_HANDLE*, int type, wchar_t* id, int* status)
{
    const wchar_t* value = L"";
    if (type == SND_DEVICES_TARGETED_REAL_PLAYBACK) value = backend->engineOutput.c_str();
    if (type == SND_DEVICES_CAPTURE || type == SND_DEVICES_VIRTUAL_PLAYBACK_DFX) value = L"V";
    if (type == SND_DEVICES_DEFAULT) value = backend->actualDefault.c_str();
    // USER_SELECTED is deliberately empty: the real function reads the registry.
    wcscpy_s(id, PT_MAX_GENERIC_STRLEN, value); *status = SND_DEVICES_DEVICE_OPERATION_COMPLETED; return OKAY;
}
int readRegistry(PT_HANDLE*, int, wchar_t*, wchar_t* value)
{ ++backend->registry; value[0] = 0; return OKAY; }
int setType(PT_HANDLE*, int, wchar_t* id, int* status)
{
    backend->selected.emplace_back(id); backend->actualDefault = id;
    invokeOnce(backend->duringSet); *status = SND_DEVICES_DEVICE_OPERATION_COMPLETED; return OKAY;
}
int restore(PT_HANDLE*, int* status)
{ ++backend->restored; *status = SND_DEVICES_DEVICE_OPERATION_COMPLETED; return OKAY; }
int forbiddenFree(PT_HANDLE*) { ++backend->forbidden; return NOT_OKAY_NO_BREAK; }
int forbiddenInit(PT_HANDLE*, CSlout*, int, int, int*) { ++backend->forbidden; return NOT_OKAY_NO_BREAK; }
HANDLE WINAPI createWorker(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD)
{
    backend->started.push_back(backend->engineOutput);
    // Never enter real WASAPI processing. A healthy worker is represented by a
    // separately owned real waiting thread below; this seam records start intent.
    SetLastError(ERROR_NOT_ENOUGH_MEMORY); return nullptr;
}
ULONGLONG WINAPI clock() { return backend->now; }
}
namespace fxsounddevices
{
HRESULT fixtureCatalog(std::vector<SoundDevice>& devices, std::wstring& defaultId)
{
    ++fixture::backend->reads;
    if (fixture::backend->failedReads > 0) { --fixture::backend->failedReads; return E_FAIL; }
    devices = fixture::backend->devices; defaultId = fixture::backend->actualDefault;
    for (auto& d : devices) d.isDefaultDevice = d.pwszID == defaultId;
    return S_OK;
}
}

// Headers are already parsed. These replacements affect calls in the actual
// owner implementation only, leaving callbacks and catalogue-item parsing real.
#define readDeviceCatalog fixtureCatalog
#define sndDevices_GetAll fixture::getAll
#define sndDevicesReInit fixture::reinit
#define sndDevicesGetID fixture::getId
#define sndDeviceReadFromRegistry fixture::readRegistry
#define sndDevicesSetDeviceType fixture::setType
#define sndDevicesRestoreDefaultDevice fixture::restore
#define sndDevicesFree fixture::forbiddenFree
#define sndDevicesInit fixture::forbiddenInit
#define CreateThread fixture::createWorker
#define GetTickCount64 fixture::clock
#include "../src/AudioPassthru/AudioPassthruPrivate.cpp"
#undef readDeviceCatalog
#undef sndDevices_GetAll
#undef sndDevicesReInit
#undef sndDevicesGetID
#undef sndDeviceReadFromRegistry
#undef sndDevicesSetDeviceType
#undef sndDevicesRestoreDefaultDevice
#undef sndDevicesFree
#undef sndDevicesInit
#undef CreateThread
#undef GetTickCount64

int checks = 0, failures = 0;
void check(bool ok, const char* label)
{ ++checks; if (!ok) ++failures; std::printf("%s %s\n", ok ? "PASS" : "FAIL", label); }

struct AudioPassthruTestPeer
{
    static sndDevicesHdlType& device() { return AudioPassthruPrivate::s_sndDevices_; }
    static void prepare(AudioPassthruPrivate& audio)
    {
        auto& h = device();
        h.deviceNotificationsClosed = false; h.ignoreDeviceCallbacks = FALSE; h.ignoreVolumeCallbacks = FALSE;
        h.deviceCallbacksInFlight = 0; h.playbackDeviceIsUnavailable = FALSE;
        h.stopAudioCaptureAndPlaybackLoop = 0; h.playbackSettingsAreSaved = FALSE;
        h.deviceChanges.~DeviceChangeQueue(); new (&h.deviceChanges) fxsounddevices::DeviceChangeQueue();
        h.DeviceEvents.SetPtHandle(reinterpret_cast<PT_HANDLE*>(&h));
        h.deviceChangeCallback = AudioPassthruPrivate::onDeviceChange;
        AudioPassthruPrivate::s_notification_window_ = nullptr;
        AudioPassthruPrivate::s_device_change_pending_ = false;
        AudioPassthruPrivate::s_device_message_posted_ = false;
        AudioPassthruPrivate::s_device_retry_ = nullptr; AudioPassthruPrivate::s_retry_armed_ = false;
        fixture::fill(h); audio.sndDeviceHandleToSoundDevices(false); audio.publishUsedEndpoints();
    }
    static void attach(AudioPassthruPrivate& audio, HANDLE thread) { audio.hProcessingThread_ = thread; }
    static void cleanup(AudioPassthruPrivate& audio)
    {
        audio.requestStop(); audio.closeDeviceRetry();
        if (audio.hProcessingThread_ && audio.hProcessingThread_ != INVALID_HANDLE_VALUE) CloseHandle(audio.hProcessingThread_);
        audio.hProcessingThread_ = nullptr;
    }
    static bool alive(AudioPassthruPrivate& audio)
    { return audio.hProcessingThread_ && WaitForSingleObject(audio.hProcessingThread_, 0) == WAIT_TIMEOUT; }
    static const std::wstring& pending(AudioPassthruPrivate& audio) { return audio.pending_output_id_; }
    static bool dirty(AudioPassthruPrivate& audio) { return audio.catalogue_dirty_; }
    static bool notified() { return AudioPassthruPrivate::s_device_change_pending_.load(); }
    static void seedRoute(AudioPassthruPrivate& audio)
    { fixture::fill(device()); audio.sndDeviceHandleToSoundDevices(false); audio.publishUsedEndpoints(); }
    static void distinctCapture(AudioPassthruPrivate& audio)
    { device().captureDeviceNum = 3; audio.publishUsedEndpoints(); } // V, A, B, C: capture is C.
};
struct RecordingCallback : AudioPassthruCallback
{
    int catalogues = 0, routes = 0;
    void onSoundDeviceChange(bool processing) override { processing ? ++routes : ++catalogues; }
};
struct Fixture
{
    fixture::Backend backend;
    AudioPassthruPrivate audio;
    RecordingCallback callback;
    HANDLE release = nullptr;
    std::thread worker;
    Fixture(const wchar_t* output = L"A")
    {
        fixture::backend = &backend;
        backend.devices = {fixture::descriptor(L"V"), fixture::descriptor(L"A"), fixture::descriptor(L"B"), fixture::descriptor(L"C")};
        backend.engineOutput = backend.actualDefault = output;
        AudioPassthruTestPeer::prepare(audio); audio.registerCallback(&callback);
    }
    ~Fixture()
    {
        finishWorker(); AudioPassthruTestPeer::cleanup(audio);
        check(backend.forbidden == 0, "fixture never initializes/frees an endpoint runtime");
        fixture::backend = nullptr;
    }
    void holdWorker()
    {
        release = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        worker = std::thread([this] { WaitForSingleObject(release, INFINITE); });
        HANDLE duplicate = nullptr;
        check(release && DuplicateHandle(GetCurrentProcess(), worker.native_handle(), GetCurrentProcess(), &duplicate,
            SYNCHRONIZE, FALSE, 0), "controlled worker owns a separate wait handle");
        AudioPassthruTestPeer::attach(audio, duplicate);
    }
    void finishWorker()
    {
        if (worker.joinable()) { SetEvent(release); worker.join(); }
        if (release) { CloseHandle(release); release = nullptr; }
    }
    void remove(const wchar_t* id)
    {
        backend.devices.erase(std::remove_if(backend.devices.begin(), backend.devices.end(),
            [id](const SoundDevice& d) { return d.pwszID == id; }), backend.devices.end());
        AudioPassthruTestPeer::device().DeviceEvents.OnDeviceRemoved(id);
    }
    void notice(const wchar_t* id) { AudioPassthruTestPeer::device().DeviceEvents.OnDeviceAdded(id); }
    bool has(const wchar_t* id) { const auto all = audio.getSoundDevices(false); return std::any_of(all.begin(), all.end(), [id](const SoundDevice& d) { return d.pwszID == id; }); }
};

void classificationTests()
{
    for (const auto* id : {L"B", L"A", L"C", L"V"})
    {
        Fixture f; f.holdWorker(); AudioPassthruTestPeer::distinctCapture(f.audio);
        auto& h = AudioPassthruTestPeer::device();
        h.DeviceEvents.OnDeviceStateChanged(id, DEVICE_STATE_UNPLUGGED);
        const bool affectsRoute = std::wcscmp(id, L"B") != 0;
        check((h.stopAudioCaptureAndPlaybackLoop != 0) == affectsRoute && AudioPassthruTestPeer::notified(),
            affectsRoute ? "known playback/capture/virtual state change stops before owner wake"
                         : "known unused state change queues a catalogue wake without stopping");
        f.audio.dispatchDeviceChange();
        check(f.callback.catalogues == 1 && AudioPassthruTestPeer::alive(f.audio) && f.backend.getAll == 0,
            "actual state-change dispatch publishes copied catalogue while worker remains held");
    }
    for (int kind = 0; kind < 3; ++kind)
    {
        Fixture f;
        const std::wstring overlong(fxsounddevices::DeviceChangeQueue::idCapacity, L'x');
        const wchar_t* id = kind == 0 ? nullptr : kind == 1 ? L"" : overlong.c_str();
        auto& h = AudioPassthruTestPeer::device();
        h.DeviceEvents.OnDeviceStateChanged(id, DEVICE_STATE_ACTIVE);
        check(h.stopAudioCaptureAndPlaybackLoop == 1 && AudioPassthruTestPeer::notified(),
            "missing, empty or overlong state ID conservatively stops and preserves owner wake");
        if (kind == 2)
            check(h.deviceChanges.usesEndpoint(L"uncertain-route") && h.deviceChanges.droppedEvents() > 0,
                "overlong record invalidates the known route and requests reconciliation");
        f.audio.dispatchDeviceChange();
    }
}

void dispatchTests()
{
    {
        Fixture f; f.holdWorker(); f.remove(L"B");
        check(AudioPassthruTestPeer::device().stopAudioCaptureAndPlaybackLoop == 0, "unused endpoint removal does not stop the healthy route");
        f.audio.dispatchDeviceChange();
        check(!f.has(L"B") && f.callback.catalogues == 1 && AudioPassthruTestPeer::alive(f.audio)
            && f.backend.getAll == 0 && f.backend.reinit == 0, "catalogue refresh reaches UI before any healthy-worker join or backend enumeration");
    }
    {
        Fixture f; f.holdWorker(); f.remove(L"A"); f.audio.dispatchDeviceChange();
        check(AudioPassthruTestPeer::device().stopAudioCaptureAndPlaybackLoop == 1 && !f.has(L"A")
            && f.backend.getAll == 0 && f.callback.catalogues == 1, "used removal stops immediately but catalogue publishes while worker is still held");
        f.audio.setTargetedRealPlaybackDevice(L"B"); f.finishWorker(); f.audio.dispatchDeviceChange();
        check(f.backend.selected == std::vector<std::wstring>{L"B"}, "pending usable fallback is applied only after worker exit");
    }
    {
        Fixture f(L"B"); f.holdWorker();
        f.audio.setTargetedRealPlaybackDevice(L"A");
        f.audio.setTargetedRealPlaybackDevice(L"B");
        check(AudioPassthruTestPeer::pending(f.audio) == L"B", "manual active-B selection replaces pending restore-A before same-ID shortcut");
        f.finishWorker(); f.audio.dispatchDeviceChange(); f.audio.processTimer();
        check(f.backend.selected == std::vector<std::wstring>{L"B"} && f.backend.started == std::vector<std::wstring>{L"B"},
            "actual pending command consumption never starts the superseded restore-A");
    }
    {
        Fixture f; f.backend.devices.push_back(fixture::descriptor(L"new-ID")); f.notice(L"new-ID"); f.audio.dispatchDeviceChange();
        f.audio.setTargetedRealPlaybackDevice(L"new-ID"); f.audio.dispatchDeviceChange();
        check(f.backend.selected == std::vector<std::wstring>{L"new-ID"}, "new cached catalogue ID resolves through stopped-backend GetAll");
    }
    for (int where = 0; where < 2; ++where)
    {
        Fixture f(L"B");
        auto newer = [&] { f.audio.setTargetedRealPlaybackDevice(L"C"); };
        if (where == 0) f.backend.duringGetAll = newer; else f.backend.duringSet = newer;
        f.audio.setTargetedRealPlaybackDevice(L"A"); f.audio.dispatchDeviceChange();
        check(AudioPassthruTestPeer::pending(f.audio) == L"C", where == 0
            ? "new command reentrant during GetAll survives the old apply call" : "new command reentrant during SetDeviceType survives old completion");
        check(where != 0 || f.backend.selected.empty(), "obsolete command is not committed after reentrant GetAll");
        f.audio.processTimer();
        check(!f.backend.started.empty() && f.backend.started.back() == L"C"
            && std::find(f.backend.started.begin(), f.backend.started.end(), L"A") == f.backend.started.end(),
            "last command wins before the next actual startup decision");
    }
    {
        Fixture f;
        f.backend.duringReinit = [&] { f.audio.setTargetedRealPlaybackDevice(L"C"); };
        f.audio.processTimer();
        check(f.backend.started.empty(), "manual selection reentrant during ReInit prevents starting the older route");
        f.audio.processTimer();
        check(f.backend.started == std::vector<std::wstring>{L"C"}, "following timer reinitializes and starts only the newer ID");
    }
    {
        Fixture f; f.backend.actualDefault = L"B";
        f.backend.duringReinit = [&] { f.remove(L"B"); };
        f.audio.processTimer();
        check(f.backend.started.empty() && AudioPassthruTestPeer::device().stopAudioCaptureAndPlaybackLoop == 1,
            "Removed-B during A-to-B ReInit is reclassified against published B before worker start");
    }
    {
        Fixture f;
        // A nominally successful ReInit with an incomplete route must not keep
        // the old coherent A snapshot. No overlong ID enters fixed Snd arrays.
        f.backend.devices.erase(f.backend.devices.begin()); f.audio.processTimer();
        check(f.backend.started.empty() && AudioPassthruTestPeer::device().deviceChanges.usesEndpoint(L"unrelated")
            && AudioPassthruTestPeer::notified(), "failed route publication invalidates stale snapshot and retains reconciliation without a worker");
    }
    {
        Fixture f; auto& h = AudioPassthruTestPeer::device();
        for (std::size_t i = 0; i < fxsounddevices::DeviceChangeQueue::capacity; ++i)
            h.deviceChanges.enqueue(fxsounddevices::DeviceChangeQueue::Kind::added, L"unrelated");
        f.notice(L"overflow-unused");
        check(h.stopAudioCaptureAndPlaybackLoop == 1 && h.deviceChanges.usesEndpoint(L"unknown")
            && h.deviceChanges.droppedEvents() > 0 && AudioPassthruTestPeer::notified(),
            "catalogue-only overflow conservatively invalidates route, stops and preserves wake");
        f.audio.dispatchDeviceChange();
    }
    {
        Fixture f; f.holdWorker(); f.backend.failedReads = 1; f.remove(L"B");
        f.audio.dispatchDeviceChange();
        check(f.has(L"B") && f.callback.catalogues == 0 && AudioPassthruTestPeer::dirty(f.audio), "transient metadata failure keeps last good catalogue and marks dirty");
        f.backend.now = 249; f.audio.dispatchDeviceChange();
        check(f.backend.reads == 1, "internal retries do not poll catalogue COM before 250 ms");
        f.backend.now = 250; f.audio.dispatchDeviceChange();
        check(!f.has(L"B") && f.callback.catalogues == 1 && f.backend.reads == 2
            && AudioPassthruTestPeer::alive(f.audio) && f.backend.reinit == 0
            && AudioPassthruTestPeer::device().stopAudioCaptureAndPlaybackLoop == 0,
            "metadata recovers without new MM notice or healthy-worker interruption");
    }
    {
        Fixture f; f.holdWorker(); f.backend.failedReads = 100; f.notice(L"unready");
        for (int t = 0; t <= 4000; t += 250) { f.backend.now = t; f.audio.dispatchDeviceChange(); }
        check(f.backend.reads == 13 && !AudioPassthruTestPeer::dirty(f.audio) && f.callback.catalogues == 0,
            "catalogue retry budget ends at three seconds without false UI epochs");
    }
    {
        Fixture f(L"B"); f.holdWorker(); f.audio.setTargetedRealPlaybackDevice(L"A");
        f.audio.restoreDefaultPlaybackDevice(); f.finishWorker(); f.audio.dispatchDeviceChange();
        check(f.backend.restored == 1 && f.backend.selected.empty() && f.backend.actualDefault == L"B"
            && AudioPassthruTestPeer::pending(f.audio).empty(), "power-off default restore cancels old pending route before later owner dispatch");
    }
    {
        Fixture f; f.backend.actualDefault = L"C";
        AudioPassthruTestPeer::device().DeviceEvents.OnDefaultDeviceChanged(eRender, eMultimedia, L"A");
        f.audio.processTimer();
        check(f.backend.selected.empty() && f.backend.started == std::vector<std::wstring>{L"C"},
            "stale default event payload cannot overwrite the current default read by ReInit");
    }
    {
        Fixture f; f.backend.actualDefault = L"C";
        AudioPassthruTestPeer::device().DeviceEvents.OnDefaultDeviceChanged(eRender, eMultimedia, L"A");
        f.audio.setTargetedRealPlaybackDevice(L"B"); f.audio.processTimer();
        check(f.backend.selected == std::vector<std::wstring>{L"B"} && f.backend.started == std::vector<std::wstring>{L"B"},
            "latest explicit ID applies before fresh-default ReInit and wins over stale notice");
    }
}

// Minimal COM metadata objects. Activate always fails and is counted; the real
// parser must use property metadata for both active and unplugged endpoints.
struct Properties : IPropertyStore
{
    ULONG refs = 1; bool formatPresent = true, shortFormat = false; WORD channels = 2;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** out) override { *out = this; AddRef(); return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { return --refs; }
    HRESULT STDMETHODCALLTYPE GetCount(DWORD* count) override { *count = 0; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetAt(DWORD, PROPERTYKEY*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetValue(REFPROPERTYKEY key, PROPVARIANT* out) override
    {
        PropVariantInit(out);
        if (IsEqualPropertyKey(key, PKEY_AudioEndpoint_FormFactor)) { out->vt = VT_UI4; out->uintVal = Speakers; return S_OK; }
        if (IsEqualPropertyKey(key, PKEY_AudioEngine_DeviceFormat))
        {
            if (!formatPresent) return E_FAIL;
            out->vt = VT_BLOB; out->blob.cbSize = shortFormat ? 1 : sizeof(WAVEFORMATEX);
            out->blob.pBlobData = static_cast<BYTE*>(CoTaskMemAlloc(out->blob.cbSize));
            if (!out->blob.pBlobData) return E_OUTOFMEMORY;
            WAVEFORMATEX format{}; format.nChannels = channels;
            std::memcpy(out->blob.pBlobData, &format, out->blob.cbSize); return S_OK;
        }
        return InitPropVariantFromString(L"test endpoint", out);
    }
    HRESULT STDMETHODCALLTYPE SetValue(REFPROPERTYKEY, REFPROPVARIANT) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Commit() override { return E_NOTIMPL; }
};
struct Endpoint : IMMDevice
{
    Properties properties; ULONG refs = 1; DWORD state = DEVICE_STATE_UNPLUGGED; int activations = 0;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** out) override { *out = this; AddRef(); return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { return --refs; }
    HRESULT STDMETHODCALLTYPE Activate(REFIID, DWORD, PROPVARIANT*, void**) override { ++activations; return E_FAIL; }
    HRESULT STDMETHODCALLTYPE OpenPropertyStore(DWORD, IPropertyStore** out) override { *out = &properties; properties.AddRef(); return S_OK; }
    HRESULT STDMETHODCALLTYPE GetId(LPWSTR* out) override
    {
        constexpr wchar_t id[] = L"fake-endpoint";
        *out = static_cast<wchar_t*>(CoTaskMemAlloc(sizeof(id)));
        if (!*out) return E_OUTOFMEMORY; std::memcpy(*out, id, sizeof(id)); return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetState(DWORD* out) override { *out = state; return S_OK; }
};
void catalogItemTests()
{
    Endpoint endpoint; SoundDevice d{}; endpoint.properties.formatPresent = false;
    check(SUCCEEDED(fxsounddevices::readDeviceCatalogItem(endpoint, d)) && !d.isActive && d.deviceNumChannel == 0
        && endpoint.activations == 0 && endpoint.properties.refs == 1, "unplugged endpoint with failing Activate and no format still enters catalogue without activation");
    endpoint.properties.formatPresent = true; endpoint.state = DEVICE_STATE_ACTIVE | 0x100;
    check(SUCCEEDED(fxsounddevices::readDeviceCatalogItem(endpoint, d)) && d.isActive && d.deviceNumChannel == 2
        && endpoint.activations == 0, "active membership bit and stored format channels are read without audio-client activation");
    endpoint.properties.shortFormat = true;
    check(FAILED(fxsounddevices::readDeviceCatalogItem(endpoint, d)) && endpoint.properties.refs == 1,
        "short active format blob is rejected and COM property ownership remains balanced");
}
void recoveryTests()
{
    OutputRecovery recovery; int ticks = 0;
    bool premature = false; for (int i = 0; i < 29; ++i) premature |= OutputRecovery::holdExpired(false, ticks, 30);
    check(!premature && OutputRecovery::holdExpired(false, ticks, 30), "existing HDMI hold expires on the thirtieth 100 ms tick");
    ticks = 4; check(!OutputRecovery::holdExpired(true, ticks, 30) && ticks == 4, "suspend does not advance the HDMI hold");
    recovery.rememberLost(L"stable-ID-A"); recovery.committed(L"fallback-B");
    check(recovery.pending() && recovery.wanted() == L"stable-ID-A", "fallback commit retains original stable ID without name or device-count matching");
    check(recovery.requestRestore(5) && !recovery.requestRestore(5) && recovery.requestRestore(6),
        "failed restore is attempted once per catalogue epoch and can retry after a real new notice");
    recovery.committed(L"stable-ID-A"); check(!recovery.pending(), "only confirmed original route commit clears recovery intent");
    recovery.rememberLost(L"A"); recovery.userSelected();
    check(!recovery.pending() && !recovery.requestRestore(7), "new manual UI or CLI choice cancels pending recovery, including the active fallback");
}
int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    classificationTests(); dispatchTests(); catalogItemTests(); recoveryTests();
    std::printf("%d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
