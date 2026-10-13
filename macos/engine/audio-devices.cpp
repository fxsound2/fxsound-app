#include "audio-devices.h"
#include <CoreFoundation/CoreFoundation.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace fxsound {
void checkAudio(OSStatus status, const char* operation) {
    if (status) throw std::runtime_error(std::string(operation) + ": " + std::to_string(status));
}
static AudioObjectPropertyAddress address(AudioObjectPropertySelector selector, bool input = false) {
    return {selector, input ? kAudioObjectPropertyScopeInput : kAudioObjectPropertyScopeOutput,
            kAudioObjectPropertyElementMain};
}
template<class T> static T property(AudioObjectID id, AudioObjectPropertySelector selector) {
    AudioObjectPropertyAddress a{selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
    T result{}; UInt32 size = sizeof(result);
    checkAudio(AudioObjectGetPropertyData(id, &a, 0, nullptr, &size, &result), "read audio property");
    return result;
}
static std::string stringProperty(AudioDeviceID id, AudioObjectPropertySelector selector) {
    auto text = property<CFStringRef>(id, selector);
    if (!text) return {};
    const auto size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(text), kCFStringEncodingUTF8) + 1;
    std::vector<char> buffer(static_cast<size_t>(size));
    bool ok = CFStringGetCString(text, buffer.data(), size, kCFStringEncodingUTF8);
    CFRelease(text);
    if (!ok) throw std::runtime_error("audio property UTF-8 conversion failed");
    return buffer.data();
}
std::string deviceUID(AudioDeviceID id) { return stringProperty(id, kAudioDevicePropertyDeviceUID); }
static UInt32 channels(AudioDeviceID id, bool input) {
    auto a = address(kAudioDevicePropertyStreamConfiguration, input);
    UInt32 size = 0;
    checkAudio(AudioObjectGetPropertyDataSize(id, &a, 0, nullptr, &size), "read channel configuration size");
    std::vector<unsigned char> bytes(size);
    checkAudio(AudioObjectGetPropertyData(id, &a, 0, nullptr, &size, bytes.data()), "read channel configuration");
    auto* buffers = reinterpret_cast<const AudioBufferList*>(bytes.data());
    UInt32 count = 0;
    for (UInt32 i = 0; i < buffers->mNumberBuffers; ++i) count += buffers->mBuffers[i].mNumberChannels;
    return count;
}
std::vector<AudioDevice> devices() {
    AudioObjectPropertyAddress a{kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
                                 kAudioObjectPropertyElementMain};
    UInt32 size = 0;
    checkAudio(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &a, 0, nullptr, &size), "device list size");
    std::vector<AudioDeviceID> ids(size / sizeof(AudioDeviceID));
    checkAudio(AudioObjectGetPropertyData(kAudioObjectSystemObject, &a, 0, nullptr, &size, ids.data()), "device list");
    std::vector<AudioDevice> result;
    for (auto id : ids) {
        if (!deviceAlive(id)) continue;
        result.push_back({id, deviceUID(id), stringProperty(id, kAudioObjectPropertyName),
                          channels(id, true), channels(id, false),
                          property<Float64>(id, kAudioDevicePropertyNominalSampleRate)});
    }
    return result;
}
AudioDevice findDevice(const std::string& uid, bool input) {
    for (const auto& device : devices())
        if (device.uid == uid && (input ? device.inputs : device.outputs)) return device;
    throw std::runtime_error("audio device unavailable: " + uid);
}
AudioStreamBasicDescription streamFormat(AudioDeviceID id, bool input) {
    auto a = address(kAudioDevicePropertyStreams, input);
    UInt32 streamBytes = 0;
    checkAudio(AudioObjectGetPropertyDataSize(id, &a, 0, nullptr, &streamBytes), "stream list size");
    if (!streamBytes) throw std::runtime_error("device has no audio streams");
    std::vector<AudioStreamID> streams(streamBytes / sizeof(AudioStreamID));
    checkAudio(AudioObjectGetPropertyData(id, &a, 0, nullptr, &streamBytes, streams.data()), "stream list");
    a = {kAudioStreamPropertyVirtualFormat, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
    AudioStreamBasicDescription format{}; UInt32 size = sizeof(format);
    checkAudio(AudioObjectGetPropertyData(streams.front(), &a, 0, nullptr, &size, &format), "stream format");
    return format;
}
AudioDeviceID defaultOutput(bool alerts) {
    return property<AudioDeviceID>(kAudioObjectSystemObject,
        alerts ? kAudioHardwarePropertyDefaultSystemOutputDevice : kAudioHardwarePropertyDefaultOutputDevice);
}
void setDefaultOutput(AudioDeviceID id, bool alerts) {
    AudioObjectPropertyAddress a{alerts ? kAudioHardwarePropertyDefaultSystemOutputDevice :
                                        kAudioHardwarePropertyDefaultOutputDevice,
                                 kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
    checkAudio(AudioObjectSetPropertyData(kAudioObjectSystemObject, &a, 0, nullptr, sizeof(id), &id), "default output");
}
bool deviceAlive(AudioDeviceID id) {
    AudioObjectPropertyAddress a{kAudioDevicePropertyDeviceIsAlive, kAudioObjectPropertyScopeGlobal,
                                 kAudioObjectPropertyElementMain};
    UInt32 alive = 0, size = sizeof(alive);
    return AudioObjectGetPropertyData(id, &a, 0, nullptr, &size, &alive) == noErr && alive != 0;
}
std::string jsonString(const std::string& value) {
    std::ostringstream out; out << '"';
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') out << '\\' << c;
        else if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << unsigned(c) << std::dec;
        else out << c;
    }
    out << '"'; return out.str();
}
std::string devicesJSON() {
    std::ostringstream out; out << "["; bool first = true;
    for (const auto& d : devices()) {
        if (!first) out << ',';
        first = false;
        out << "{\"id\":" << d.id << ",\"uid\":" << jsonString(d.uid) << ",\"name\":" << jsonString(d.name)
            << ",\"inputs\":" << d.inputs << ",\"outputs\":" << d.outputs << ",\"sampleRate\":" << d.rate
            << ",\"virtual\":" << (d.uid == virtualUID ? "true" : "false") << '}';
    }
    out << "]"; return out.str();
}
}
