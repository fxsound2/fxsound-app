#pragma once
#include <JuceHeader.h>
#include "FxModel.h"
#include "FxTheme.h"
#include "FxAudioControls.h"
#include "../../fxsound/Source/Utils/Settings/Settings.h"
#include "../../fxsound/Source/Utils/Settings/DeviceConfig.h"
#include "engine-client.h"
using namespace FxSound;
class FxMainWindow;
class FxWindow;
enum ViewType { Lite=1,Pro=2 };
class FxController : public DeletedAtShutdown {
public:
    static constexpr int NUM_SPECTRUM_BANDS = 10;
	static constexpr int DEFAULT_NUM_EQ_BANDS = 10;
	static constexpr float DEFAULT_NORMALIZATION = 0.0f;
	static constexpr float DEFAULT_VOLUME_LEVELING = 0.0f;
	static constexpr float DEFAULT_BALANCE = 0.0f;
	static constexpr float DEFAULT_FILTER_Q = 1.0f;
	static constexpr float DEFAULT_MASTER_GAIN = 0.0f;
	static constexpr float MIN_GAIN = -12.0f;
	static constexpr float MAX_GAIN = 12.0f;
	static constexpr char HK_CMD_ON_OFF[] = "cmd_on_off";
	static constexpr char HK_CMD_OPEN_CLOSE[] = "cmd_open_close";
	static constexpr char HK_CMD_NEXT_PRESET[] = "cmd_next_preset";
	static constexpr char HK_CMD_PREVIOUS_PRESET[] = "cmd_previous_preset";
	static constexpr char HK_CMD_NEXT_OUTPUT[] = "cmd_change_output";

	static FxController& getInstance()
	{
		static FxController* controller = new FxController();
		return *controller;
	}
	~FxController();

	FxController(const FxController&) = delete;
	void operator=(FxController&) = delete;

    void initConfig(const String& commandline);
	void applyConfig(const String& commandline);
	void init(FxMainWindow* main_window, bool smoke);
	void initPresets();
	void printStatus();
	static File getStatusFile();

	void showView();
	void switchView();
	ViewType getCurrentView();
	void hideMainWindow();
	void showMainWindow();
    bool isMainWindowVisible();
	void setMenuClicked(bool clicked);
	FxWindow* getMainWindow();
	Point<int> getSystemTrayWindowPosition(int width, int height);
	bool exit();

	void setPowerState(bool power_state);
    bool setPreset(const String& preset_name, bool notify = true);
	bool setPreset(int preset_index, bool notify=true);
	void setOutput(const String output_device_id, bool notify=true);
	void setOutput(int output, bool notify=true);

    bool isPlaybackDeviceAvailable();
	void checkDeviceChanges();

	void autoSaveModifiedPreset();
	void savePreset(const String& preset_name=L"");
	void renamePreset(const String& new_name);
	void deletePreset();
	void undoPreset();
	void resetPresets();
    bool exportPresets(const Array< FxModel::Preset>& presets);
    bool importPresets(const Array<File>& preset_files, StringArray& imported_presets, StringArray& skipped_presets);

	float getEffectValue(FxEffects::EffectType effect);
	void setEffectValue(FxEffects::EffectType effect, float value);

	int getNumEqBands();
	void setNumEqBands(int num_bands);
	float getVolumeLeveling();
	void setVolumeLeveling(float gain_db);
	void setBalance(float gain_db);
	float getBalance();
	void setMasterGain(float gain_db);
	float getMasterGain();
	void setFilterQ(float q_multiplier);
	float getFilterQ();

    bool isAudioProcessing();
	float getEqBandFrequency(int band_num);
    void setEqBandFrequency(int band_num, float freq);
    void getEqBandFrequencyRange(int band_num, float* min_freq, float* max_freq);
	float getEqBandBoostCut(int band_num);
	void setEqBandBoostCut(int band_num, float boost);
    void getSpectrumBandValues(Array<float>& band_values);

	void enableHotkeys(bool enable);
	bool getHotkey(String cmdKey, int& mod, int& vk);
	bool setHotkey(const String& command, int new_mod, int new_vk);
	bool isValidHotkey(int mod, int new_vk);

	juce::Array<DeviceConfig> getDeviceConfigs();
    void saveDeviceConfigs(const juce::Array<DeviceConfig>& device_configs);
	bool isOutputDeviceConnected(const String& output_device_name);
	bool isOutputDevicePresent(const String& output_device_name);
	SoundDevice getPreferredOutput();
	int compareOutputDevicePriority(const String& output_device_name1, const String& output_device_name2, const juce::Array<DeviceConfig>& device_configs);
	void refreshOutputList();
	const String& getOutputName();
    void setOutputName(const String& output_device_name);
    bool isNewOutputPrioritized();
    void setNewOutputPrioritized(bool prioritize_new_devices);

	FxThemeMode getThemeMode();
	void setThemeMode(FxThemeMode mode);

	bool isAlwaysOnTop();
	void setAlwaysOnTop(bool always_on_top);

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

	bool getAutoUpdates();
	void setAutoUpdates(bool enable);
	void checkUpdates();

	void saveWindowPosition(int x, int y);
	void getWindowPosition(int& x, int& y);

	void logMessage(const String&) {}

	FxSound::Settings& getSettings() { return settings_; }

private:
 friend class FxPresetControllerTestAccess;
 FxController();
 void receive(fxgui::Event);
 void updateState(const var&);
 void updateDevices(const var&);
 void beginAudio();
 void advancePower();
 void permissionResult(bool);
 void modified();
 void applyPendingPreset();
 void presetResult(const fxgui::Event&);
 float control(const char*,float fallback=0) const;
 void parameter(const String&,float);
 File userPresets() const;
 FxMainWindow* main_window_=nullptr;
 fxgui::EngineClient client_;
 FxSound::Settings settings_;
 var state_;
 Array<float> effects_,gains_,frequencies_;
 Array<DeviceConfig> device_configs_;
 int bands_=10;
 bool ready_=false,driver_=false,smoke_=false,requested_power_=false,restored_=false;
 bool permission_pending_=false,power_waiting_=false,start_pending_=false;
 bool bypass_pending_=false,routing_pending_=false;
 int restore_pending_=0;
 std::shared_ptr<int> lifetime_=std::make_shared<int>(0);
 ViewType view_=Pro;
 String output_device_name_;
 String pending_save_,pending_remove_;
 String preset_pending_,preset_in_flight_,preset_confirmed_;
 std::function<void()> quit_;
public:
 void detach() { lifetime_.reset();main_window_=nullptr;client_.deactivate();client_.onEvent={}; }
 void onQuit(std::function<void()> callback) {quit_=std::move(callback);}
};
