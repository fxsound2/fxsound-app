#include <CoreAudio/AudioServerPlugIn.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cassert>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cmath>
#include <iostream>
#include <limits>
#include <thread>
#include <vector>
static std::atomic<UInt32> notifications{0};
static OSStatus notify(AudioServerPlugInHostRef, AudioObjectID, UInt32 count,
                       const AudioObjectPropertyAddress* addresses) {
    assert(count && addresses); notifications += count; return noErr;
}
static AudioObjectPropertyAddress address(AudioObjectPropertySelector selector) {
    return {selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
}
int main(int argc, char** argv) {
    assert(argc == 2);
    auto url = CFURLCreateFromFileSystemRepresentation(nullptr, reinterpret_cast<UInt8*>(argv[1]),
                                                      strlen(argv[1]), true);
    auto bundle = CFBundleCreate(nullptr, url); assert(bundle); CFRelease(url);
    assert(CFBundleLoadExecutable(bundle));
    using Factory = void* (*)(CFAllocatorRef, CFUUIDRef);
    auto factory = reinterpret_cast<Factory>(CFBundleGetFunctionPointerForName(bundle, CFSTR("FxSound_Create")));
    assert(factory && !factory(nullptr, nullptr));
    auto driver = static_cast<AudioServerPlugInDriverRef>(factory(nullptr, kAudioServerPlugInTypeUUID));
    assert(driver && *driver);
    AudioServerPlugInHostInterface host{}; host.PropertiesChanged = notify;
    assert((*driver)->Initialize(driver, &host) == noErr);
    constexpr AudioObjectID device = 2, input = 3, output = 4, volume = 5, mute = 6;
    auto formatAddress = address(kAudioStreamPropertyVirtualFormat);
    UInt32 size = 0;
    assert((*driver)->GetPropertyDataSize(driver, output, 0, &formatAddress, 0, nullptr, &size) == noErr);
    assert(size == sizeof(AudioStreamBasicDescription));
    AudioStreamBasicDescription format{}; UInt32 written = 0;
    assert((*driver)->GetPropertyData(driver, output, 0, &formatAddress, 0, nullptr,
                                    1, &written, &format) == kAudioHardwareBadPropertySizeError);
    assert((*driver)->GetPropertyData(driver, output, 0, &formatAddress, 0, nullptr,
                                    sizeof(format), &written, &format) == noErr);
    assert(format.mSampleRate == 48000 && format.mChannelsPerFrame == 2 && format.mBitsPerChannel == 32);
    format.mSampleRate = 44100;
    assert((*driver)->SetPropertyData(driver, output, 0, &formatAddress, 0, nullptr,
                                    sizeof(format), &format) != noErr);
    auto bad = address('bad!'); assert(!(*driver)->HasProperty(driver, output, 0, &bad));
    assert(!(*driver)->HasProperty(driver, 999, 0, &formatAddress));
    assert((*driver)->StartIO(driver, device, 100) == noErr);
    assert((*driver)->StartIO(driver, device, 100) != noErr);
    assert((*driver)->StartIO(driver, device, 101) == noErr);
    Boolean yes = false, inplace = false;
    assert((*driver)->WillDoIOOperation(driver, device, 100, kAudioServerPlugInIOOperationWriteMix,
                                      &yes, &inplace) == noErr && yes && inplace);
    assert((*driver)->WillDoIOOperation(driver, device, 100, kAudioServerPlugInIOOperationMixOutput,
                                      &yes, &inplace) == noErr && !yes);
    AudioServerPlugInIOCycleInfo info{};
    info.mOutputTime.mFlags = info.mInputTime.mFlags = kAudioTimeStampSampleTimeValid;
    std::vector<float> samples(256 * 2), read(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) samples[i] = float(i) / 1024;
    auto io = [&](bool write, UInt32 frames, float* data) {
        return (*driver)->DoIOOperation(driver, device, write ? output : input, 100,
            write ? kAudioServerPlugInIOOperationWriteMix : kAudioServerPlugInIOOperationReadInput,
            frames, &info, data, nullptr);
    };
    assert(io(true, 256, samples.data()) == noErr);
    info.mInputTime.mSampleTime = 512;
    assert(io(false, 256, read.data()) == noErr && samples == read);
    auto scalar = address(kAudioLevelControlPropertyScalarValue); float gain = 0.5f;
    assert((*driver)->SetPropertyData(driver, volume, 0, &scalar, 0, nullptr, 1, &gain)
           == kAudioHardwareBadPropertySizeError);
    assert((*driver)->SetPropertyData(driver, volume, 0, &scalar, 0, nullptr, 4, &gain) == noErr);
    info.mOutputTime.mSampleTime = 256; info.mInputTime.mSampleTime = 768;
    assert(io(true, 256, samples.data()) == noErr && io(false, 256, read.data()) == noErr);
    for (size_t i = 0; i < samples.size(); ++i) assert(read[i] == samples[i] * 0.5f);
    auto boolean = address(kAudioBooleanControlPropertyValue); UInt32 value = 1;
    assert((*driver)->SetPropertyData(driver, mute, 0, &boolean, 0, nullptr, 4, &value) == noErr);
    info.mOutputTime.mSampleTime = 512; info.mInputTime.mSampleTime = 1024;
    assert(io(true, 256, samples.data()) == noErr && io(false, 256, read.data()) == noErr);
    for (float sample : read) assert(sample == 0);
    float nan = std::numeric_limits<float>::quiet_NaN();
    assert((*driver)->SetPropertyData(driver, volume, 0, &scalar, 0, nullptr, 4, &nan) != noErr);
    info.mOutputTime.mSampleTime = nan; assert(io(true, 256, samples.data()) != noErr);
    assert(io(true, 32769, samples.data()) != noErr);
    Float64 sample1, sample2; UInt64 time1, time2, seed1, seed2;
    assert((*driver)->GetZeroTimeStamp(driver, device, 100, &sample1, &time1, &seed1) == noErr);
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    assert((*driver)->GetZeroTimeStamp(driver, device, 100, &sample2, &time2, &seed2) == noErr);
    assert(sample2 >= sample1 && time2 >= time1 && seed1 == seed2 && fmod(sample2, 512) == 0);
    assert((*driver)->StopIO(driver, device, 100) == noErr);
    assert((*driver)->StopIO(driver, device, 100) != noErr);
    assert((*driver)->StopIO(driver, device, 101) == noErr);
    assert((*driver)->StartIO(driver, device, 100) == noErr);
    assert((*driver)->GetZeroTimeStamp(driver, device, 100, &sample1, &time1, &seed1) == noErr && seed1 != seed2);
    info.mInputTime.mSampleTime = 512;
    assert(io(false, 256, read.data()) == noErr);
    for (float sample : read) assert(sample == 0);
    assert((*driver)->StopIO(driver, device, 100) == noErr && notifications > 0);
    std::atomic<bool> finished{false}; std::atomic<size_t> timestamps{0};
    std::thread clockReader([&] {
        while (!finished.load()) {
            Float64 sample; UInt64 time, seed;
            const auto status = (*driver)->GetZeroTimeStamp(driver, device, 77, &sample, &time, &seed);
            if (status == noErr) { assert(sample >= 0 && fmod(sample, 512) == 0 && seed > 0); ++timestamps; }
            else assert(status == kAudioHardwareIllegalOperationError);
        }
    });
    while (timestamps.load() == 0) std::this_thread::yield();
    for (int i = 0; i < 1000; ++i) {
        assert((*driver)->StartIO(driver, device, 77) == noErr);
        assert((*driver)->StopIO(driver, device, 77) == noErr);
    }
    finished = true; clockReader.join(); assert(timestamps > 0);
    CFBundleUnloadExecutable(bundle); CFRelease(bundle);
    std::cout << "driver: actual bundle/factory/property size/format/two clients/loopback/gain/mute/timestamp/epoch passed\n";
}
