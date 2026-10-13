#include "../engine/audio-devices.h"
std::string presetTestDefaultUID() { return fxsound::deviceUID(fxsound::defaultOutput()); }
std::string presetTestDevicesJSON() { return fxsound::devicesJSON(); }
