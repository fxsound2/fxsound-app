/* FxSound — AGPL-3.0-or-later. */
#pragma once
#include "AudioPassthru.h"

namespace fxsounddevices
{
// Owner-thread values only. No pointer or index into the running engine is used.
HRESULT readDeviceCatalog(std::vector<SoundDevice>& devices, std::wstring& defaultId);
HRESULT readDeviceCatalogItem(IMMDevice& device, SoundDevice& item);
}
