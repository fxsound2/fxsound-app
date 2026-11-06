/*
FxSound Linux port — linux/sndDevices_pw.h (Phase 7)
Windows -> PipeWire mapping (see plan table):
  fxvad.sys -> null sink support.null-audio-sink, node.name=fxsound_audio_enhancer,
    node.description="FxSound Audio Enhancer", media.class=Audio/Sink,
    audio.position=[FL FR], object.linger=false, node.pause-on-idle=false,
    session.suspend-timeout-seconds=0, monitor.channel-volumes per spike.
  IMMDeviceEnumerator -> pw_registry listener on Audio/Sink (description=name, node.name=ID).
  WASAPI loopback -> capture pw_stream stream.capture.sink=true target fxsound_audio_enhancer.
  IAudioRenderClient -> playback pw_stream target=<real node.name>, dont-reconnect/dont-fallback.
  SetDefaultEndpoint -> default metadata default.configured.audio.sink, fallback wpctl.
  Volume mirror one-way fxsound->real (VOLUME_MODE A/B from SPIKE_RESULTS.md).
Untested on Linux. Never includes compat/.
*/
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace fxsound_linux {
struct PwDevice {
  std::string nodeName;
  std::string description;
  bool isDefault = false;
  bool isFxsound = false;
};

// Verbose logging from the start (Phase 7 §1).
void pwSetVerbose(bool v);
bool pwVerbose();

// Lifecycle: connect with backoff 30s, create sink, wait registry, streams, flip default.
bool pwStartup(const std::string& prevSink, int bufferMs);
void pwShutdown(const std::string& prevSink);
bool pwConnectedAndSinkReady();  // replaces FxController.cpp:1504 driver check

std::vector<PwDevice> pwListSinks();  // excludes fxsound sink from output list
bool pwSetOutput(const std::string& nodeName);  // recreate playback target; never fxsound
std::string pwGetSelectedOutput();  // current targeted real sink node name
bool pwSetDefaultFxsound();  // metadata + wpctl fallback
bool pwRestorePrevious(const std::string& prevSink, bool currentIsFxsound);
void pwFlushBuffers();  // zero out capture & playback ring buffers and reset cursors

// Volume (VOLUME_MODE from spike): A=pre (mirror only), B=post (+divide capture).
// Startup: init fxsound vol from real sink so slider doesn't jump.
void pwMirrorVolumeTick();
float pwFxsoundVolume();  // 0..1, floor 0.02 for B-mode divide; 0 if muted

using PwDeviceCb = std::function<void()>;
void pwSetDeviceCallback(PwDeviceCb cb);  // registry listener -> JUCE message thread

// Drift telemetry: underruns/overruns/corrections, logged every 60s when verbose.
struct PwDrift {
  uint64_t underruns = 0, overruns = 0, corrections = 0;
};
PwDrift pwDrift();

// SPSC ring accessors for the DSP worker (linux/AudioPassthru_linux.cpp).
// Frames are interleaved stereo F32 at the negotiated rate (48000 pinned).
size_t pwCaptureRead(float* dst, size_t maxFrames);       // 0..maxFrames frames
size_t pwPlaybackWrite(const float* src, size_t frames);  // frames actually queued
void pwSetFxsoundMuted(bool m);
bool pwIsFxsoundMuted();
}  // namespace fxsound_linux
