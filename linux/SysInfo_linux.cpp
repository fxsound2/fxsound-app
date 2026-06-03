/*
FxSound — Linux build

Linux implementation of SysInfo. The Windows version (fxsound/Source/Utils/
SysInfo/SysInfo.cpp) is COM/WASAPI based and is excluded from the Linux build.

Audio-output enumeration is intentionally a no-op here: on Linux the device
list comes from the PipeWire backend (M4) via AudioPassthru, not from SysInfo.
*/

#include "SysInfo.h"

void SysInfo::enumAudioOutputs(StringArray& audioOutputNames)
{
    audioOutputNames.clear();
    // Populated by the PipeWire backend in M4.
}

String SysInfo::getAudioOutputName(IMMDevice* /*pDevice*/)
{
    return {};
}

bool SysInfo::isServiceRunning(LPCWSTR /*service_name*/)
{
    // No Windows-style service dependency on Linux.
    return false;
}

bool SysInfo::canSupportHotkeys()
{
    return true;
}
