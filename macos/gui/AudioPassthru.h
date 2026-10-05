#pragma once
#include <string>
#include <vector>
struct SoundDevice {
 std::wstring pwszID,deviceFriendlyName,deviceDescription,deviceFormFactor;
 int deviceNumChannel=0;
 bool isRealDevice=true,isDFXDevice=false,isPlaybackDevice=true,isActive=true;
};
