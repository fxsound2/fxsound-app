/*
FxSound Linux port — linux/AudioPassthru_linux.cpp (Phase 7)
Linux implementation of the AudioPassthru facade (audiopassthru/include/AudioPassthru.h)
replacing AudioPassthruPrivate + WASAPI sndDevices with the PipeWire backend
(linux/sndDevices_pw.cpp). Windows keeps audiopassthru/src/AudioPassthru/*.cpp
(not compiled here — see CMake AudioPassthruCore exclusion note).

Mapping (same behavior as Windows AudioPassthruPrivate):
  init()                       -> pwStartup (connect, create fxsound sink, streams, default)
  getSoundDevices(active)      -> pwListSinks + always-present fxsound virtual device
  setAsPlaybackDevice(sd)      -> pwSetOutput(node.name from pwszID "pw:<name>")
  processTimer()               -> start DSP worker once PipeWire is ready; mirrors
                                  Windows "start processing thread on first tick" flow
                                  (callback onSoundDeviceChange(true) fires on start)
  DSP worker                   -> capture ring (F32) -> S16 -> processAudio (bypass when
                                  power off == bit-exact passthrough) -> F32 -> playback ring
  mute(bool)                   -> silence the playback path (fxsound sink mute not forced)
  checkDeviceChanges()         -> registry change flag consumed by FxController timer
  restoreDefaultPlaybackDevice -> pwRestorePrevious (only if default is still fxsound)
Volume mirror (Mode A) and default switching are event-driven inside sndDevices_pw.cpp.
JUCE is used only for marshaling device callbacks to the message thread.
*/
#include "AudioPassthru.h"
#include "sndDevices_pw.h"
#include "float_convert.h"

#include <JuceHeader.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int kSampleRate = 48000;
constexpr int kChannels = 2;
constexpr int kBitsPerSample = 16;

constexpr const wchar_t* kFxDeviceName = L"FxSound Audio Enhancer";
constexpr const wchar_t* kFxDeviceDescription = L"FxSound Speakers";

inline std::wstring utf8ToWide(const std::string& utf8) {
  return juce::String::fromUTF8(utf8.data(), (int)utf8.size()).toWideCharPointer();
}

inline std::string wideToUtf8(const std::wstring& wide) {
  return juce::String(wide.c_str()).toStdString();
}

// Windows sndDevices.h equivalents (SND_DEVICES_DFX_DEVICE_STRING etc.).
std::wstring nodeNameFromPwszID(const std::wstring& pwszID) {
  // IDs are built as L"pw:" + node.name; the fxsound device is its node name.
  const std::wstring prefix = L"pw:";
  if (pwszID.rfind(prefix, 0) == 0) return pwszID.substr(prefix.size());
  return pwszID;
}

class AudioPassthruPrivateLinux
{
public:
	AudioPassthruPrivateLinux() = default;
	~AudioPassthruPrivateLinux() { stopWorker(); }

	int init()
	{
		// Power-state DSP bypass is decided by the GUI (DfxDsp::powerOn); audio
		// must flow from the very first tick, bypassed or not (like Windows).
		if (!fxsound_linux::pwStartup(/*prevSink*/ "", /*bufferMs*/ 0))
			return -1;

		selected_output_name_ = utf8ToWide(fxsound_linux::pwGetSelectedOutput());
		fxsound_linux::pwSetDeviceCallback([this] { onPwDeviceChanged(); });
		return 0;
	}

	void setDspProcessingModule(DfxDsp* p_dfx_dsp)
	{
		std::lock_guard<std::recursive_mutex> lock(dsp_mutex_);
		p_dfx_dsp_ = p_dfx_dsp;
		if (p_dfx_dsp_)
		{
			p_dfx_dsp_->setSignalFormat(kBitsPerSample, kChannels, kSampleRate, kBitsPerSample);
		}
	}

	std::vector<SoundDevice> getSoundDevices(bool active_devices)
	{
		(void)active_devices;  // PipeWire registry lists live sinks; unplugged ones vanish.
		std::vector<SoundDevice> devices;

		const std::wstring selected = selected_output_name_;
		const std::string activeNode = fxsound_linux::pwGetSelectedOutput();

		// The fxsound virtual device — always present while the backend runs,
		// so FxController's dfx_enabled_ check (deviceFriendlyName contains
		// "FxSound Audio Enhancer") behaves like the Windows driver check.
		SoundDevice fx;
		fx.isPlaybackDevice = true;
		fx.isRealDevice = false;
		fx.isDFXDevice = true;
		fx.isActive = true;
		fx.deviceNumChannel = kChannels;
		fx.pwszID = kFxDeviceName;
		fx.deviceFriendlyName = kFxDeviceName;
		fx.deviceDescription = kFxDeviceDescription;
		fx.deviceDescriptionRealDevices = L"";
		fx.deviceFormFactor = L"Speakers";
		devices.push_back(fx);

		for (const auto& sink : fxsound_linux::pwListSinks())
		{
			SoundDevice sd;
			sd.isPlaybackDevice = true;
			sd.isRealDevice = true;
			sd.isActive = true;
			sd.deviceNumChannel = kChannels;
			sd.pwszID = L"pw:" + utf8ToWide(sink.nodeName);
			sd.deviceFriendlyName = utf8ToWide(sink.description);
			sd.deviceDescription = sd.deviceFriendlyName;
			sd.deviceFriendlyNameRealDevices = sd.deviceFriendlyName;
			sd.deviceDescriptionRealDevices = sd.deviceDescription;
			sd.deviceFormFactor = L"Speakers";
			if (sink.nodeName == activeNode)
			{
				sd.isUserSelectedPlaybackDevice = true;
				if (fxsound_linux::pwConnectedAndSinkReady())
					sd.isTargetedRealPlaybackDevice = true;
			}
			else if (!activeNode.empty() && utf8ToWide(sink.nodeName) == selected)
			{
				sd.isUserSelectedPlaybackDevice = true;
			}
			if (sink.isDefault)
				sd.isDefaultDevice = true;
			devices.push_back(sd);
		}

		return devices;
	}

	void setAsPlaybackDevice(const SoundDevice sound_device)
	{
		std::wstring nodeName = nodeNameFromPwszID(sound_device.pwszID);
		if (nodeName == kFxDeviceName) return;  // never feedback
		std::string utf8 = wideToUtf8(nodeName);
		if (fxsound_linux::pwSetOutput(utf8))
			selected_output_name_ = nodeName;
	}

	int setBufferLength(int i_buffer_length_msecs)
	{
		if (i_buffer_length_msecs < 10 || i_buffer_length_msecs > 100) return -1;
		buffer_ms_ = i_buffer_length_msecs;
		return 0;
	}

	int processTimer()
	{
		if (!fxsound_linux::pwConnectedAndSinkReady())
			return -1;

		bool notify_worker_started = false;
		if (!worker_running_.load())
		{
			{
				std::lock_guard<std::recursive_mutex> lock(dsp_mutex_);
				if (!worker_running_.load() && p_dfx_dsp_ != nullptr)
				{
					startWorker();
					notify_worker_started = true;
				}
			}
			// Mirror Windows: processing thread start signals the GUI once, strictly OUTSIDE dsp_mutex_
			if (notify_worker_started && s_callback_ != nullptr)
				s_callback_->onSoundDeviceChange(true);
		}

		if (device_changed_.exchange(false))
		{
			if (s_callback_ != nullptr)
				s_callback_->onSoundDeviceChange(true);
		}
		return 0;
	}

	void mute(bool mute)
	{
		fxsound_linux::pwSetFxsoundMuted(mute);
	}

	void registerCallback(AudioPassthruCallback* callback)
	{
		s_callback_ = callback;
	}

	bool isPlaybackDeviceAvailable()
	{
		return fxsound_linux::pwConnectedAndSinkReady();
	}

	bool checkDeviceChanges()
	{
		// Device-change notification is event-driven (onPwDeviceChanged ->
		// JUCE message thread -> onSoundDeviceChange(true)); the Windows
		// polling path returns "changed" so getSoundDevices refreshes. Keep the
		// flag consumed here for API parity with AudioPassthruPrivate.
		bool changed = device_changed_.exchange(false);
		return changed;
	}

	void restoreDefaultPlaybackDevice()
	{
		fxsound_linux::pwFlushBuffers();
		fxsound_linux::pwRestorePrevious(/*prevSink*/ "", /*currentIsFxsound*/ true);
	}

	void setPower(bool on)
	{
		if (on)
		{
			fxsound_linux::pwSetDefaultFxsound();
			if (!power_on_.exchange(true))
			{
				fxsound_linux::pwFlushBuffers();
				std::lock_guard<std::recursive_mutex> lock(dsp_mutex_);
				if (p_dfx_dsp_ != nullptr)
				{
					p_dfx_dsp_->setSignalFormat(kBitsPerSample, kChannels, kSampleRate, kBitsPerSample);
				}
			}
		}
		else
		{
			if (power_on_.exchange(false))
			{
				fxsound_linux::pwFlushBuffers();
				fxsound_linux::pwRestorePrevious(/*prevSink*/ "", /*currentIsFxsound*/ true);
			}
		}
	}

private:
	void onPwDeviceChanged()
	{
		device_changed_.store(true);
		bool expected = false;
		if (device_change_pending_.compare_exchange_strong(expected, true))
		{
			juce::MessageManager::callAsync([this] {
				device_change_pending_.store(false);
				if (s_callback_ != nullptr)
					s_callback_->onSoundDeviceChange(true);
			});
		}
	}

	void startWorker()
	{
		worker_running_.store(true);
		worker_ = std::thread([this] { workerLoop(); });
	}

	void stopWorker()
	{
		worker_running_.store(false);
		if (worker_.joinable())
			worker_.join();
	}

	void workerLoop()
	{
		std::vector<float> f32buf(1024 * kChannels);
		std::vector<int16_t> s16buf(1024 * kChannels);

		while (worker_running_.load())
		{
			size_t frames = fxsound_linux::pwCaptureRead(f32buf.data(), 1024);
			if (frames == 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
			}
			else
			{
				bool processed = false;
				if (!fxsound_linux::pwIsFxsoundMuted())
				{
					std::lock_guard<std::recursive_mutex> lock(dsp_mutex_);
					if (p_dfx_dsp_ != nullptr)
					{
						fxsound_linux::f32_to_s16_block(f32buf.data(), s16buf.data(), frames * kChannels);
						p_dfx_dsp_->processAudio(s16buf.data(), s16buf.data(), (int)frames, 0);
						fxsound_linux::s16_to_f32_block(s16buf.data(), f32buf.data(), frames * kChannels);
						processed = true;
					}
				}

				if (!processed)
				{
					// No DSP or muted: output silence (mute is one-way, user-facing).
					std::fill(f32buf.begin(), f32buf.begin() + (long)(frames * kChannels), 0.0f);
				}
				fxsound_linux::pwPlaybackWrite(f32buf.data(), frames);
			}
		}
	}

	std::recursive_mutex dsp_mutex_;
	DfxDsp* p_dfx_dsp_ = nullptr;
	int buffer_ms_ = 40;
	std::wstring selected_output_name_;
	std::atomic<bool> worker_running_{false};
	std::thread worker_;
	static inline AudioPassthruCallback* s_callback_ = nullptr;
	std::atomic<bool> device_changed_{false};
	std::atomic<bool> device_change_pending_{false};
	std::atomic<bool> power_on_{false};
};

}  // namespace

// ================= AudioPassthru facade (Linux) =================

AudioPassthru::AudioPassthru()
{
	data_ = reinterpret_cast<AudioPassthruPrivate*>(new AudioPassthruPrivateLinux());
}

AudioPassthru::~AudioPassthru()
{
	delete reinterpret_cast<AudioPassthruPrivateLinux*>(data_);
}

int AudioPassthru::init()
{
	try
	{
		return reinterpret_cast<AudioPassthruPrivateLinux*>(data_)->init();
	}
	catch (...)
	{
		return -1;
	}
}

void AudioPassthru::setDspProcessingModule(DfxDsp* p_dfx_dsp)
{
	reinterpret_cast<AudioPassthruPrivateLinux*>(data_)->setDspProcessingModule(p_dfx_dsp);
}

std::vector<SoundDevice> AudioPassthru::getSoundDevices(bool active_devices)
{
	return reinterpret_cast<AudioPassthruPrivateLinux*>(data_)->getSoundDevices(active_devices);
}

void AudioPassthru::setAsPlaybackDevice(const SoundDevice sound_device)
{
	reinterpret_cast<AudioPassthruPrivateLinux*>(data_)->setAsPlaybackDevice(sound_device);
}

int AudioPassthru::setBufferLength(int i_buffer_length_msecs)
{
	return reinterpret_cast<AudioPassthruPrivateLinux*>(data_)->setBufferLength(i_buffer_length_msecs);
}

int AudioPassthru::processTimer()
{
	auto* impl = reinterpret_cast<AudioPassthruPrivateLinux*>(data_);
	auto ret = impl->processTimer();
	if (!impl->isPlaybackDeviceAvailable())
		return -1;
	return ret;
}

void AudioPassthru::mute(bool mute)
{
	reinterpret_cast<AudioPassthruPrivateLinux*>(data_)->mute(mute);
}

void AudioPassthru::registerCallback(AudioPassthruCallback* callback)
{
	reinterpret_cast<AudioPassthruPrivateLinux*>(data_)->registerCallback(callback);
}

bool AudioPassthru::isPlaybackDeviceAvailable()
{
	return reinterpret_cast<AudioPassthruPrivateLinux*>(data_)->isPlaybackDeviceAvailable();
}

bool AudioPassthru::checkDeviceChanges()
{
	return reinterpret_cast<AudioPassthruPrivateLinux*>(data_)->checkDeviceChanges();
}

void AudioPassthru::restoreDefaultPlaybackDevice()
{
	reinterpret_cast<AudioPassthruPrivateLinux*>(data_)->restoreDefaultPlaybackDevice();
}

void AudioPassthru::setPower(bool on)
{
	reinterpret_cast<AudioPassthruPrivateLinux*>(data_)->setPower(on);
}
