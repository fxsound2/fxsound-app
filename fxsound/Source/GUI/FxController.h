/*
FxSound
Copyright (C) 2023  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <JuceHeader.h>
#include "FxModel.h"
#include "FxEffects.h"
#include "../Source/Utils/Settings/Settings.h"
#include "AudioPassthru.h"
#include "DfxDsp.h"
#include <wtsapi32.h>

class FxMainWindow;
class FxWindow;
class FxSystemTrayView;

enum ViewType { Lite = 1, Pro = 2 };

class FxController : public Timer, private AudioPassthruCallback
{
public:
    static constexpr int NUM_SPECTRUM_BANDS = 10;
	static constexpr char HK_CMD_ON_OFF[] = "cmd_on_off";
	static constexpr char HK_CMD_OPEN_CLOSE[] = "cmd_open_close";
	static constexpr char HK_CMD_NEXT_PRESET[] = "cmd_next_preset";
	static constexpr char HK_CMD_PREVIOUS_PRESET[] = "cmd_previous_preset";
	static constexpr char HK_CMD_NEXT_OUTPUT[] = "cmd_change_output";
	
	static FxController& getInstance()
	{
		static FxController controller;
		return controller;
	}
	~FxController();

	FxController(const FxController&) = delete;
	void operator=(FxController&) = delete;

    void config(const String& commandline);
	void init(FxMainWindow* main_window, FxSystemTrayView* system_tray_view, AudioPassthru* audio_passthru);
	void initPresets();

	void showView();
	void switchView();
	ViewType getCurrentView();
	void hideMainWindow();
	void showMainWindow();
    bool isMainWindowVisible();
	void setMenuClicked(bool clicked);
	FxWindow* getMainWindow();
	bool exit();

	void setPowerState(bool power_state);
	bool setPreset(int selected_preset);
	void setOutput(int output, bool notify=true);
    
    bool isPlaybackDeviceAvailable();

	void savePreset(const String& preset_name=L"");
	void renamePreset(const String& new_name);
	void deletePreset();
	void undoPreset();
	void resetPresets();
    bool exportPresets(const Array< FxModel::Preset>& presets);
    bool importPresets(const Array<File>& preset_files, StringArray& imported_presets, StringArray& skipped_presets);

	float getEffectValue(FxEffects::EffectType effect);
	void setEffectValue(FxEffects::EffectType effect, float value);

    bool isAudioProcessing();
	int getNumEqBands();
	float getEqBandFrequency(int band_num);
    void setEqBandFrequency(int band_num, float freq);
    void getEqBandFrequencyRange(int band_num, float* min_freq, float* max_freq);
	float getEqBandBoostCut(int band_num);
	void setEqBandBoostCut(int band_num, float boost);
    void getSpectrumBandValues(Array<float>& band_values);

	void enableHotkeys(bool enable);
	bool getHotkey(String cmdKey, int& mod, int& vk);
	bool setHotkey(const String& command, int new_mod, int vk);
	bool isValidHotkey(int mod, int new_vk);

	std::tuple<String, String> getPreferredOutput();
	String getPreferredOutputId();
	String getPreferredOutputName();
	void setPreferredOutput(String id, String name);

	bool isLaunchOnStartup();
	void setLaunchOnStartup(bool launch_on_startup);

    bool isHelpTooltipsHidden();
    void setHelpTooltipsHidden(bool status);

	bool isNotificationsHidden();
	void setNotificationsHidden(bool status);

    String getLanguage() const;
    void setLanguage(String language_code);
    String getLanguageName(String language_code) const;
	int getMaxUserPresets() const;

	void logMessage(const String& message)
	{
		file_logger_->logMessage(message);
	}

private:
	class MessageWindow
	{
	public:
		MessageWindow(const WCHAR* const wnd_name, WNDPROC wnd_proc)
		{
			String class_name("FXSOUND_");
			class_name << String::toHexString(Time::getHighResolutionTicks());

			HMODULE h_module = (HMODULE)Process::getCurrentModuleInstanceHandle();

			WNDCLASSEXW wc = { 0 };
			wc.cbSize = sizeof(wc);
			wc.lpfnWndProc = wnd_proc;
			wc.hInstance = h_module;
			wc.lpszClassName = class_name.toWideCharPointer();

			atom_ = ::RegisterClassExW(&wc);
			jassert(atom_ != 0);

			hwnd_ = ::CreateWindowW(getClassNameFromAtom(), wnd_name,
				0, 0, 0, 0, 0, 0, 0, h_module, 0);
			jassert(hwnd_ != 0);
		}

		~MessageWindow()
		{
			DestroyWindow(hwnd_);
			UnregisterClassW(getClassNameFromAtom(), 0);
		}

		inline HWND getHandle() const noexcept { return hwnd_; }

	private:
		ATOM atom_;
		HWND hwnd_;

		LPCWSTR getClassNameFromAtom() noexcept { return (LPCWSTR)(pointer_sized_uint)atom_; }
	};

	static constexpr UINT CMD_ON_OFF = 1001;
	static constexpr UINT CMD_OPEN_CLOSE = 1002;
	static constexpr UINT CMD_NEXT_PRESET = 1003;
	static constexpr UINT CMD_PREVIOUS_PRESET = 1004;
	static constexpr UINT CMD_NEXT_OUTPUT = 1005;

	FxController();

	static LRESULT CALLBACK eventCallback(HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param);
	void timerCallback() override;

	void onSoundDeviceChange(std::vector<SoundDevice> sound_devices) override;
	
    void initOutputs(std::vector<SoundDevice>& sound_devices);
	void addPreferredOutput(std::vector<SoundDevice>& sound_devices);
    void selectOutput();
	void updateOutputs(std::vector<SoundDevice>& sound_devices);
	void setSelectedOutput(String id, String name);

	void registerHotkeys();
	void unregisterHotkeys();

    String FormatString(const String& format, const String& arg);

	MessageWindow message_window_;
	bool hotkeys_registered_;

	FxMainWindow* main_window_;
	FxSystemTrayView* system_tray_view_;
	AudioPassthru* audio_passthru_;
	DfxDsp dfx_dsp_;
	FxSound::Settings settings_;
	uint32_t device_count_;
	std::unique_ptr<FileLogger> file_logger_;
	ViewType view_;
    String language_;
	bool dfx_enabled_;
	bool authenticated_;
    bool free_plan_;
	bool output_changed_;
    bool playback_device_available_;
	String output_device_id_;
    String output_device_name_;
    StringArray output_ids_;
	std::vector<SoundDevice> output_devices_;
    bool hide_help_tooltips_;
	bool hide_notifications_;
    
	unsigned long audio_process_time_;
	int audio_process_on_counter_;
	int audio_process_off_counter_;
	bool audio_process_on_;
    unsigned long audio_processed_per_day_;
    std::time_t audio_process_start_time_;

	bool minimize_tip_;
	bool processing_time_over_tip_;
	bool subscription_validity_tip_;
    bool survey_tip_;
    bool subscription_unverified_tip_;
	int max_user_presets_;

	DWORD session_id_;

	CriticalSection lock_;
};