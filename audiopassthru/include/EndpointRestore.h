/*
FxSound
Copyright (C) 2025 FxSound LLC
Contributors: Sylvain Petitgrand (2026)

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero General Public License, version 3 or later.
This program is distributed WITHOUT ANY WARRANTY. See <https://www.gnu.org/licenses/>.
*/
#pragma once
#include <Windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <memory>
#include <new>
#include <string>
#include <vector>
#include "PolicyConfig.h"

namespace fxaudio
{
// Captured by value on the device-owner thread before shutdown starts. This
// snapshot contains no MMDevice interfaces, indices or pointers into sndDevices.
struct EndpointRestoreSnapshot
{
    std::vector<std::wstring> candidates;
    std::wstring virtualId;
    std::wstring savedMuteId;
    bool savedMute = false;
    bool hasSavedMute = false;
};

inline bool sameEndpointId(const std::wstring& left, const std::wstring& right) noexcept
{
    return _wcsicmp(left.c_str(), right.c_str()) == 0;
}

// Backend seam exercises the selection policy without touching real devices.
// The production backend below creates and releases all of its own COM objects.
template<class Backend>
HRESULT restoreEndpointWithBackend(const EndpointRestoreSnapshot& snapshot, Backend& backend)
{
    if (snapshot.virtualId.empty()) return S_FALSE; // Never guess the virtual endpoint.
    // FxSound may have propagated its mute to the previous physical endpoint.
    // Restore that endpoint's own setting even if Windows already selected a
    // different default. This operation does not change the default or volume.
    HRESULT muteResult = S_OK;
    if (snapshot.hasSavedMute && !snapshot.savedMuteId.empty()
        && !sameEndpointId(snapshot.savedMuteId, snapshot.virtualId)
        && backend.active(snapshot.savedMuteId))
        muteResult = backend.restoreMute(snapshot.savedMuteId, snapshot.savedMute);

    std::wstring current;
    const HRESULT currentResult = backend.defaultEndpoint(current);
    if (FAILED(currentResult) && currentResult != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        return currentResult;
    // Preserve a default selected externally just before FxSound quit. A removed
    // virtual device can also leave no default; in that case try the saved IDs.
    if (SUCCEEDED(currentResult) && !sameEndpointId(current, snapshot.virtualId))
        return FAILED(muteResult) ? muteResult : S_FALSE;

    HRESULT result = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    for (std::size_t index = 0; index < snapshot.candidates.size(); ++index)
    {
        const auto& id = snapshot.candidates[index];
        if (id.empty() || sameEndpointId(id, snapshot.virtualId)) continue;
        bool duplicate = false;
        for (std::size_t previous = 0; previous < index; ++previous)
            if (sameEndpointId(id, snapshot.candidates[previous])) { duplicate = true; break; }
        if (duplicate || !backend.active(id)) continue;
        result = backend.setDefault(id);
        if (SUCCEEDED(result)) return FAILED(muteResult) ? muteResult : result;
    }
    return result;
}

namespace detail
{
struct ReleaseCom
{
    template<class Interface> void operator()(Interface* value) const noexcept
    {
        if (value != nullptr) value->Release();
    }
};
template<class Interface> using ComReference = std::unique_ptr<Interface, ReleaseCom>;

class EndpointRestoreBackend final
{
public:
    HRESULT initialize() noexcept
    {
        IMMDeviceEnumerator* enumerator = nullptr;
        const HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
            CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator));
        enumerator_.reset(enumerator);
        return SUCCEEDED(result) && enumerator == nullptr ? E_UNEXPECTED : result;
    }
    HRESULT defaultEndpoint(std::wstring& id)
    {
        if (enumerator_ == nullptr) return E_UNEXPECTED;
        IMMDevice* raw = nullptr;
        HRESULT result = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &raw);
        const ComReference<IMMDevice> device(raw);
        if (FAILED(result)) return result;
        if (device == nullptr) return E_UNEXPECTED;
        LPWSTR rawId = nullptr;
        result = device->GetId(&rawId);
        struct FreeId { void operator()(wchar_t* value) const noexcept { CoTaskMemFree(value); } };
        const std::unique_ptr<wchar_t, FreeId> ownedId(rawId);
        if (FAILED(result)) return result;
        if (rawId == nullptr) return E_UNEXPECTED;
        id = rawId;
        return id.empty() ? E_UNEXPECTED : S_OK;
    }
    bool active(const std::wstring& id) noexcept
    {
        if (enumerator_ == nullptr) return false;
        IMMDevice* raw = nullptr;
        const HRESULT result = enumerator_->GetDevice(id.c_str(), &raw);
        const ComReference<IMMDevice> device(raw);
        DWORD state = 0;
        return SUCCEEDED(result) && device != nullptr && SUCCEEDED(device->GetState(&state))
            && (state & DEVICE_STATE_ACTIVE) != 0;
    }
    HRESULT restoreMute(const std::wstring& id, bool muted) noexcept
    {
        if (enumerator_ == nullptr) return E_UNEXPECTED;
        IMMDevice* raw = nullptr;
        HRESULT result = enumerator_->GetDevice(id.c_str(), &raw);
        const ComReference<IMMDevice> device(raw);
        if (FAILED(result)) return result;
        if (device == nullptr) return E_UNEXPECTED;
        IAudioEndpointVolume* rawVolume = nullptr;
        result = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                                 reinterpret_cast<void**>(&rawVolume));
        const ComReference<IAudioEndpointVolume> volume(rawVolume);
        if (FAILED(result)) return result;
        return volume == nullptr ? E_UNEXPECTED : volume->SetMute(muted ? TRUE : FALSE, nullptr);
    }
    HRESULT setDefault(const std::wstring& id) noexcept
    {
        IPolicyConfigVista* rawPolicy = nullptr;
        const HRESULT result = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), nullptr,
            CLSCTX_ALL, __uuidof(IPolicyConfigVista), reinterpret_cast<void**>(&rawPolicy));
        const ComReference<IPolicyConfigVista> policy(rawPolicy);
        if (FAILED(result)) return result;
        return policy == nullptr ? E_UNEXPECTED : policy->SetDefaultEndpoint(id.c_str(), eConsole);
    }
private:
    ComReference<IMMDeviceEnumerator> enumerator_;
};
}

// Run on the shutdown task, never the UI or audio callback. A caller can bound
// its wait for that task even if COM initialization, release or policy calls hang.
inline HRESULT restoreEndpoint(const EndpointRestoreSnapshot& snapshot) noexcept
{
    if (snapshot.virtualId.empty()) return S_FALSE;
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized)) return initialized;
    struct Uninitialize { ~Uninitialize() { CoUninitialize(); } } apartment;
    try
    {
        detail::EndpointRestoreBackend backend;
        const HRESULT result = backend.initialize();
        return FAILED(result) ? result : restoreEndpointWithBackend(snapshot, backend);
    }
    catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
    catch (...) { return E_UNEXPECTED; }
}
}
