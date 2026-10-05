#pragma once
#include <CoreAudio/CoreAudio.h>
#include <string>
#include <vector>

namespace fxsound {
inline constexpr const char* virtualUID = "FxSound_Mac_Virtual";
struct AudioDevice {
    AudioDeviceID id;
    std::string uid;
    std::string name;
    UInt32 inputs;
    UInt32 outputs;
    double rate;
};
std::vector<AudioDevice> devices();
AudioDevice findDevice(const std::string& uid, bool input);
AudioStreamBasicDescription streamFormat(AudioDeviceID id, bool input);
AudioDeviceID defaultOutput(bool alerts = false);
void setDefaultOutput(AudioDeviceID id, bool alerts = false);
bool deviceAlive(AudioDeviceID id);
std::string deviceUID(AudioDeviceID id);
std::string jsonString(const std::string& value);
std::string devicesJSON();
void checkAudio(OSStatus status, const char* operation);
}
