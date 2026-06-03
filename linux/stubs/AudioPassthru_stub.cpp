/*
FxSound — Linux build (M0 bootstrap stub)

Stub implementation of the AudioPassthru public API
(audiopassthru/include/AudioPassthru.h) so the JUCE GUI compiles and launches
on Linux before the real PipeWire backend is built (M2/M3).
No devices are enumerated and no audio flows.
*/

#include "AudioPassthru.h"

class AudioPassthruPrivate {};

AudioPassthru::AudioPassthru() : data_(nullptr) {}
AudioPassthru::~AudioPassthru() {}

int  AudioPassthru::init()                                  { return 0; }
void AudioPassthru::mute(bool)                              {}

std::vector<SoundDevice> AudioPassthru::getSoundDevices(bool)
{
    return {};
}

int  AudioPassthru::setBufferLength(int)                    { return 0; }
int  AudioPassthru::processTimer()                          { return 0; }
void AudioPassthru::setDspProcessingModule(DfxDsp*)         {}
void AudioPassthru::setAsPlaybackDevice(const SoundDevice)  {}
void AudioPassthru::registerCallback(AudioPassthruCallback*) {}
bool AudioPassthru::isPlaybackDeviceAvailable()            { return false; }
bool AudioPassthru::checkDeviceChanges()                   { return false; }
void AudioPassthru::restoreDefaultPlaybackDevice()         {}
