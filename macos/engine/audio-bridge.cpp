#include "audio-bridge.h"
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <mach/mach_time.h>

namespace fxsound {
static bool floatStereo(const AudioStreamBasicDescription& f) {
    return f.mFormatID == kAudioFormatLinearPCM && (f.mFormatFlags & kAudioFormatFlagIsFloat) &&
           !(f.mFormatFlags & kAudioFormatFlagIsBigEndian) && f.mBitsPerChannel == 32 && f.mChannelsPerFrame == 2;
}
static UInt32 frameCount(const AudioBufferList* list) noexcept {
    if (!list || !list->mNumberBuffers) return 0;
    UInt32 count = UINT32_MAX, channels = 0;
    for (UInt32 i = 0; i < list->mNumberBuffers; ++i) {
        const auto& b = list->mBuffers[i];
        if (!b.mData || !b.mNumberChannels) return 0;
        channels += b.mNumberChannels;
        count = std::min(count, UInt32(b.mDataByteSize / (sizeof(float) * b.mNumberChannels)));
    }
    return channels == 2 ? count : 0;
}
static float sample(const AudioBufferList* list, UInt32 frame, UInt32 channel) noexcept {
    for (UInt32 i = 0; i < list->mNumberBuffers; ++i) {
        const auto& b = list->mBuffers[i];
        if (channel < b.mNumberChannels) return static_cast<const float*>(b.mData)[frame * b.mNumberChannels + channel];
        channel -= b.mNumberChannels;
    }
    return 0;
}
static void store(AudioBufferList* list, UInt32 frame, UInt32 channel, float value) noexcept {
    for (UInt32 i = 0; i < list->mNumberBuffers; ++i) {
        auto& b = list->mBuffers[i];
        if (channel < b.mNumberChannels) {
            static_cast<float*>(b.mData)[frame * b.mNumberChannels + channel] = value; return;
        }
        channel -= b.mNumberChannels;
    }
}
AudioBridge::~AudioBridge() { stop(); }
void AudioBridge::setProcessor(Processor processor, void* context) {
    if (captureID_ || renderID_) throw std::logic_error("stop IO before changing DSP ownership");
    processor_ = processor; processorContext_ = context;
}
void AudioBridge::start(const std::string& outputUID) {
    stop();
    if (outputUID == virtualUID) throw std::runtime_error("virtual output would cause audio feedback");
    input_ = findDevice(virtualUID, true).id; output_ = findDevice(outputUID, false).id;
    try {
        const auto in = streamFormat(input_, true), out = streamFormat(output_, false);
        if (!floatStereo(in) || in.mSampleRate != 48000) throw std::runtime_error("virtual stream requires stereo Float32/48 kHz");
        if (!floatStereo(out) || (out.mSampleRate != 44100 && out.mSampleRate != 48000 && out.mSampleRate != 96000))
            throw std::runtime_error("physical output requires stereo Float32 at 44.1/48/96 kHz; mono/SCO is unavailable");
        outputRate_ = out.mSampleRate;
        mach_timebase_info_data_t timebase{};
        if (mach_timebase_info(&timebase) != KERN_SUCCESS || timebase.denom == 0)
            throw std::runtime_error("could not establish callback clock");
        tickNanoseconds_ = double(timebase.numer) / timebase.denom;
        ring_.reset(); resampler_.prepare(outputRate_);
        captured_ = rendered_ = delivered_ = overruns_ = underruns_ = hostTime_ = 0; driftPPM_ = 0; changed_ = false;
        captureTick_ = renderTick_ = 0;
        checkAudio(AudioDeviceCreateIOProcID(input_, capture, this, &captureID_), "create capture IOProc");
        checkAudio(AudioDeviceCreateIOProcID(output_, render, this, &renderID_), "create render IOProc");
        listeners(true);
        checkAudio(AudioDeviceStart(output_, renderID_), "start render");
        checkAudio(AudioDeviceStart(input_, captureID_), "start capture");
    } catch (...) { stop(); throw; }
}
void AudioBridge::stop() noexcept {
    if (captureID_) { AudioDeviceStop(input_, captureID_); AudioDeviceDestroyIOProcID(input_, captureID_); captureID_ = nullptr; }
    if (renderID_) { AudioDeviceStop(output_, renderID_); AudioDeviceDestroyIOProcID(output_, renderID_); renderID_ = nullptr; }
    if (listening_) listeners(false);
    input_ = output_ = 0;
}
OSStatus AudioBridge::capture(AudioDeviceID, const AudioTimeStamp*, const AudioBufferList* input,
                             const AudioTimeStamp* stamp, AudioBufferList*, const AudioTimeStamp*, void* context) {
    auto& self = *static_cast<AudioBridge*>(context);
    auto frames = frameCount(input);
    if (!frames || frames > self.scratch_.size()) { self.overruns_.fetch_add(1, std::memory_order_relaxed); return noErr; }
    for (UInt32 i = 0; i < frames; ++i) self.scratch_[i] = {sample(input, i, 0), sample(input, i, 1)};
    if (!self.ring_.push(self.scratch_.data(), frames)) self.overruns_.fetch_add(1, std::memory_order_relaxed);
    else {
        self.captured_.fetch_add(frames, std::memory_order_relaxed);
        self.captureTick_.store(mach_absolute_time(), std::memory_order_relaxed);
    }
    if (stamp && (stamp->mFlags & kAudioTimeStampHostTimeValid)) self.hostTime_.store(stamp->mHostTime, std::memory_order_relaxed);
    return noErr;
}
OSStatus AudioBridge::render(AudioDeviceID, const AudioTimeStamp*, const AudioBufferList*,
                            const AudioTimeStamp*, AudioBufferList* output, const AudioTimeStamp*, void* context) {
    auto& self = *static_cast<AudioBridge*>(context);
    auto frames = frameCount(output);
    if (!frames || frames > 4096) {
        if (output) for (UInt32 i = 0; i < output->mNumberBuffers; ++i)
            if (output->mBuffers[i].mData) std::memset(output->mBuffers[i].mData, 0, output->mBuffers[i].mDataByteSize);
        self.underruns_.fetch_add(1, std::memory_order_relaxed); return noErr;
    }
    self.resampler_.update(self.ring_.available(), frames);
    self.driftPPM_.store(self.resampler_.ppm(), std::memory_order_relaxed);
    bool missing = false; UInt32 delivered = 0;
    for (UInt32 i = 0; i < frames; ++i) {
        StereoFrame frame;
        bool valid = self.resampler_.next(self.ring_, frame);
        missing |= !valid; delivered += valid ? 1 : 0;
        self.renderScratch_[i * 2] = frame.left; self.renderScratch_[i * 2 + 1] = frame.right;
    }
    if (self.processor_) self.processor_(self.processorContext_, self.renderScratch_.data(), frames);
    float peakLeft = 0, peakRight = 0;
    for (UInt32 i = 0; i < frames; ++i) {
        peakLeft = std::max(peakLeft, std::abs(self.renderScratch_[i * 2]));
        peakRight = std::max(peakRight, std::abs(self.renderScratch_[i * 2 + 1]));
        store(output, i, 0, self.renderScratch_[i * 2]); store(output, i, 1, self.renderScratch_[i * 2 + 1]);
    }
    self.peakLeft_.store(peakLeft, std::memory_order_relaxed); self.peakRight_.store(peakRight, std::memory_order_relaxed);
    if (missing) self.underruns_.fetch_add(1, std::memory_order_relaxed);
    self.rendered_.fetch_add(frames, std::memory_order_relaxed);
    self.delivered_.fetch_add(delivered, std::memory_order_relaxed);
    if (delivered) self.renderTick_.store(mach_absolute_time(), std::memory_order_relaxed);
    return noErr;
}
bool AudioBridge::ready() const noexcept {
    const auto now = mach_absolute_time(), capture = captureTick_.load(), render = renderTick_.load();
    const bool fresh = capture && render && now >= capture && now >= render &&
                       (now - capture) * tickNanoseconds_ < 500000000 && (now - render) * tickNanoseconds_ < 500000000;
    return fresh && captured_.load() >= AdaptiveResampler::targetFrames && delivered_.load() > 0 && hostTime_.load() != 0 && captureID_ && renderID_;
}
OSStatus AudioBridge::propertyChanged(AudioObjectID, UInt32, const AudioObjectPropertyAddress*, void* context) {
    static_cast<AudioBridge*>(context)->changed_.store(true, std::memory_order_relaxed); return noErr;
}
void AudioBridge::listeners(bool add) {
    listening_ = add;
    for (const auto id : {input_, output_})
        for (const auto selector : {kAudioDevicePropertyDeviceIsAlive, kAudioDevicePropertyNominalSampleRate}) {
            AudioObjectPropertyAddress a{selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
            if (add) checkAudio(AudioObjectAddPropertyListener(id, &a, propertyChanged, this), "device listener");
            else AudioObjectRemovePropertyListener(id, &a, propertyChanged, this);
        }
    listening_ = add;
}
std::string AudioBridge::metricsJSON() const {
    std::ostringstream out;
    out << "{\"capturedFrames\":" << captured_.load() << ",\"renderedFrames\":" << rendered_.load()
        << ",\"deliveredFrames\":" << delivered_.load()
        << ",\"overruns\":" << overruns_.load() << ",\"underruns\":" << underruns_.load()
        << ",\"occupancyFrames\":" << ring_.available() << ",\"driftPPM\":" << driftPPM_.load()
        << ",\"captureHostTime\":" << hostTime_.load() << ",\"outputSampleRate\":" << outputRate_
        << ",\"peakLeft\":" << peakLeft_.load() << ",\"peakRight\":" << peakRight_.load()
        << ",\"targetQueueMs\":" << (AdaptiveResampler::targetFrames * 1000.0 / 48000)
        << ",\"bypass\":" << (processor_ ? "false" : "true")
        << ",\"dspReady\":" << (processor_ ? "true" : "false")
        << ",\"ready\":" << (ready() ? "true" : "false") << '}';
    return out.str();
}
}
