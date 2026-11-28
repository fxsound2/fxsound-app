/* FxSound — AGPL-3.0-or-later. */
#include "DeviceCatalog.h"
#include "sndDevices.h"
#include <Functiondiscoverykeys_devpkey.h>
#include <endpointvolume.h>
#include <propvarutil.h>
#include <memory>
#include <cstring>

namespace fxsounddevices
{
namespace
{
struct Release { template<class T> void operator()(T* p) const { if (p) p->Release(); } };
template<class T> using Com = std::unique_ptr<T, Release>;
struct FreeId { void operator()(wchar_t* p) const { CoTaskMemFree(p); } };
struct Variant { PROPVARIANT value; Variant() { PropVariantInit(&value); } ~Variant() { PropVariantClear(&value); } };
std::wstring propertyText(IPropertyStore& properties, REFPROPERTYKEY key)
{
    Variant result;
    return SUCCEEDED(properties.GetValue(key, &result.value)) && result.value.vt == VT_LPWSTR
        && result.value.pwszVal ? result.value.pwszVal : L"";
}
std::wstring formFactor(IPropertyStore& properties)
{
    Variant result;
    if (FAILED(properties.GetValue(PKEY_AudioEndpoint_FormFactor, &result.value)) || result.value.vt != VT_UI4) return L"Unknown";
    switch (result.value.uintVal)
    {
    case Speakers: return L"Speakers";
    case Headphones: return L"Headphones";
    case Headset: return L"Headset";
    case DigitalAudioDisplayDevice: return L"HDMI";
    case SPDIF: return L"SPDIF";
    case LineLevel: return L"LineLevel";
    case RemoteNetworkDevice: return L"NetworkDevice";
    case Microphone: return L"Microphone";
    case Handset: return L"Handset";
    case UnknownDigitalPassthrough: return L"DigitalPassthrough";
    default: return L"Unknown";
    }
}
HRESULT endpointId(IMMDevice& device, std::wstring& id)
{
    wchar_t* raw = nullptr;
    const HRESULT hr = device.GetId(&raw);
    const std::unique_ptr<wchar_t, FreeId> owned(raw);
    if (FAILED(hr)) return hr;
    if (!raw || !raw[0]) return E_UNEXPECTED;
    id = raw; return S_OK;
}
}

HRESULT readDeviceCatalogItem(IMMDevice& device, SoundDevice& item)
{
    HRESULT hr = endpointId(device, item.pwszID);
    if (FAILED(hr)) return hr;
    DWORD state = 0;
    if (FAILED(hr = device.GetState(&state))) return hr;
    item.isActive = (state & DEVICE_STATE_ACTIVE) != 0;
    IPropertyStore* rawProperties = nullptr;
    if (FAILED(hr = device.OpenPropertyStore(STGM_READ, &rawProperties))) return hr;
    const Com<IPropertyStore> properties(rawProperties);
    if (!properties) return E_UNEXPECTED;
    item.deviceFriendlyName = propertyText(*properties, PKEY_Device_FriendlyName);
    item.deviceDescription = propertyText(*properties, PKEY_Device_DeviceDesc);
    if (item.deviceFriendlyName.empty()) return E_UNEXPECTED;
    item.deviceFormFactor = formFactor(*properties);
    item.isDFXDevice = item.deviceFriendlyName.find(SND_DEVICES_DFX_DEVICE_STRING) != std::wstring::npos;
    item.isRealDevice = !item.isDFXDevice;
    Variant format;
    hr = properties->GetValue(PKEY_AudioEngine_DeviceFormat, &format.value);
    item.deviceNumChannel = 0;
    if (SUCCEEDED(hr) && format.value.vt == VT_BLOB && format.value.blob.pBlobData
        && format.value.blob.cbSize >= sizeof(WAVEFORMATEX))
    {
        WAVEFORMATEX value{};
        std::memcpy(&value, format.value.blob.pBlobData, sizeof(value));
        item.deviceNumChannel = value.nChannels;
    }
    // Inactive endpoints need no live audio-client activation. If their format
    // metadata is absent, retain the descriptor with an unknown channel count.
    if (item.isActive && item.deviceNumChannel <= 0) return FAILED(hr) ? hr : E_UNEXPECTED;
    return S_OK;
}

HRESULT readDeviceCatalog(std::vector<SoundDevice>& devices, std::wstring& defaultId)
{
    IMMDeviceEnumerator* rawEnumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&rawEnumerator));
    const Com<IMMDeviceEnumerator> enumerator(rawEnumerator);
    if (FAILED(hr)) return hr;
    if (!enumerator) return E_UNEXPECTED;
    IMMDevice* rawDefault = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &rawDefault);
    const Com<IMMDevice> defaultDevice(rawDefault);
    std::wstring currentDefault;
    if (SUCCEEDED(hr)) { if (!defaultDevice) return E_UNEXPECTED; hr = endpointId(*defaultDevice, currentDefault); }
    if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) return hr;
    IMMDeviceCollection* rawCollection = nullptr;
    hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &rawCollection);
    const Com<IMMDeviceCollection> collection(rawCollection);
    if (FAILED(hr)) return hr;
    if (!collection) return E_UNEXPECTED;
    UINT count = 0;
    if (FAILED(hr = collection->GetCount(&count))) return hr;
    std::vector<SoundDevice> next;
    for (UINT i = 0; i < count; ++i)
    {
        IMMDevice* rawDevice = nullptr;
        if (FAILED(hr = collection->Item(i, &rawDevice))) return hr;
        const Com<IMMDevice> device(rawDevice);
        if (!device) return E_UNEXPECTED;
        SoundDevice item{};
        if (FAILED(hr = readDeviceCatalogItem(*device, item))) return hr;
        item.isDefaultDevice = item.pwszID == currentDefault;
        if (!SND_DEVICES_MONO_BUG_SKIP_MONO_DEVICES || item.deviceNumChannel != 1) next.push_back(std::move(item));
    }
    devices = std::move(next); defaultId = std::move(currentDefault); return S_OK;
}
}
