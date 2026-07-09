/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
	www.theremino.com (2025)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FxController.h"
#include "FxMainWindow.h"
#include "FxSystemTrayView.h"
#include "FxMessage.h"
#include "FxEffects.h"
#include "../Utils/SysInfo/SysInfo.h"
#include <iostream>
#include <cstdio>

class FxDeviceErrorMessage : public FxWindow
{
public:
	FxDeviceErrorMessage()
	{
		setContent(&message_content_);
		centreWithSize(getWidth(), getHeight());
		addToDesktop(0);
		setAlwaysOnTop(true);
	}
	~FxDeviceErrorMessage() = default;

	void closeButtonPressed() override
	{
		exitModalState(0);
		removeFromDesktop();
	}

private:
	class MessageComponent : public Component
	{
	public:
		MessageComponent()
		{
			auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

			error_message_.setFont(theme.getNormalFont());
			contact_message_.setFont(theme.getNormalFont());

			error_message_.setText(TRANS("Oops! There's an issue with your playback device settings.\r\nBefore we can get started, please go through the "), NotificationType::dontSendNotification);
			error_message_.setJustificationType(Justification::topLeft);
			addAndMakeVisible(error_message_);

			error_link_.setButtonText(TRANS("troubleshooting steps here."));
			error_link_.setURL(URL(L"https://www.fxsound.com/learning-center/installation-troubleshooting"));
			error_link_.setJustificationType(Justification::topLeft);
			addAndMakeVisible(error_link_);

			contact_message_.setText(TRANS(" if you're still having problems."), NotificationType::dontSendNotification);
			contact_message_.setJustificationType(Justification::topLeft);
			addAndMakeVisible(contact_message_);

			contact_link_.setButtonText(TRANS("Contact us"));
			contact_link_.setURL(URL(L"https://www.fxsound.com/support"));
			contact_link_.setJustificationType(Justification::topLeft);
			addAndMakeVisible(contact_link_);

			setSize(WIDTH, HEIGHT);
		}
		~MessageComponent() = default;

	private:
		static constexpr int WIDTH = 400;
		static constexpr int HEIGHT = 142;
		static constexpr int MESSAGE_HEIGHT = (24 + 2) * 2;
		static constexpr int HYPERLINK_HEIGHT = 24;

		void resized() override
		{
			auto bounds = getLocalBounds();
			bounds.setTop(10);
			bounds.reduce(20, 0);

			RectanglePlacement placement(RectanglePlacement::xLeft
				| RectanglePlacement::yTop
				| RectanglePlacement::doNotResize);

			auto component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), MESSAGE_HEIGHT);
			error_message_.setBounds(placement.appliedTo(component_area, bounds));
			auto x = error_message_.getX() + error_message_.getBorderSize().getLeft();

			bounds.setTop(error_message_.getBottom() + 2);
			component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), HYPERLINK_HEIGHT);
			error_link_.setBounds(placement.appliedTo(component_area, bounds).withX(x));

			bounds.setTop(error_link_.getBottom() + 20);
			component_area = juce::Rectangle<int>(0, 0, contact_link_.getTextWidth(), HYPERLINK_HEIGHT);
			contact_link_.setBounds(placement.appliedTo(component_area, bounds).withX(x));

			bounds = juce::Rectangle<int>(contact_link_.getRight(), contact_link_.getY(), error_message_.getWidth() - contact_link_.getWidth(), MESSAGE_HEIGHT / 2);
			contact_message_.setBounds(bounds);
		}

		Label error_message_;
		Label contact_message_;
		FxHyperlink error_link_;
		FxHyperlink contact_link_;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MessageComponent)
	};

	MessageComponent message_content_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxDeviceErrorMessage)
};

FxController::FxController() : message_window_(L"FxSoundHotkeys", (WNDPROC)eventCallback)
{
	dfx_enabled_ = true;
	authenticated_ = true;
	minimize_tip_ = true;

	hotkeys_registered_ = false;
	output_changed_ = false;
	playback_device_available_ = true;
	powerNotify_ = nullptr;
	unregister_suspend_resume_notification_ = nullptr;

	device_count_ = 0;
	output_device_name_ = L"";

	audio_process_time_ = 0;
	audio_process_on_counter_ = 0;
	audio_process_off_counter_ = 0;
	audio_process_on_ = false;

	audio_process_start_time_ = -1LL;

	preset_dirty_ = false;
	auto_save_counter_ = 0;

	main_window_ = nullptr;
	audio_passthru_ = nullptr;

	file_logger_.reset(FileLogger::createDefaultAppLogger(L"FxSound", L"fxsound.log", L"FxSound logs"));
	logMessage("v" + JUCEApplication::getInstance()->getApplicationVersion());
	logMessage(SystemStats::getOperatingSystemName());

	SYSTEM_INFO sys_info;
	GetNativeSystemInfo(&sys_info);
	if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
	{
		logMessage(String("x86"));
	}
	else if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
	{
		logMessage(String("x64"));
	}
	else if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
	{
		logMessage(String("ARM64"));
	}

	auto view = settings_.getInt("view");
	if (view <= 0 || view > 2)
	{
		view_ = ViewType::Pro;
	}
	else
	{
		view_ = static_cast<ViewType>(view);
	}
	auto hotkeys_support = settings_.getBool("hotkeys") && SysInfo::canSupportHotkeys();
	FxModel::getModel().setHotkeySupport(hotkeys_support);
	if (hotkeys_support)
	{
		registerHotkeys();
	}
	FxModel::getModel().setMenuClicked(settings_.getBool("menu_clicked"));

	always_on_top_ = settings_.getBool("always_on_top");
	hide_help_tooltips_ = settings_.getBool("hide_help_tooltips");
	hide_notifications_ = settings_.getBool("hide_notifications");
	auto_updates_ = settings_.getBool("automatic_updates", true);
	max_user_presets_ = settings_.getInt("max_user_presets");
	if (max_user_presets_ < 10 || max_user_presets_ > 120)
	{
		settings_.setInt("max_user_presets", 120);
		max_user_presets_ = 120;
	}

	SetWindowLongPtr(message_window_.getHandle(), GWLP_USERDATA, (LONG_PTR)this);

	session_id_ = 0;
	ProcessIdToSessionId(GetCurrentProcessId(), &session_id_);
	WTSRegisterSessionNotification(message_window_.getHandle(), NOTIFY_FOR_THIS_SESSION);
}

FxController::~FxController()
{
	if (powerNotify_ != nullptr && unregister_suspend_resume_notification_ != nullptr)
	{
		unregister_suspend_resume_notification_(powerNotify_);
		powerNotify_ = nullptr;
	}
	stopTimer();
	WTSUnRegisterSessionNotification(message_window_.getHandle());
	unregisterHotkeys();
}


void FxController::initConfig(const String& commandline)
{
	auto arg_list = ArgumentList(File::getSpecialLocation(File::SpecialLocationType::invokedExecutableFile).getFileName(), commandline);

	auto power_state = arg_list.getValueForOption("--power");
	auto preset = arg_list.getValueForOption("--preset").unquoted();
	auto view = arg_list.getValueForOption("--view");
	auto output_device = arg_list.getValueForOption("--output").unquoted();
	auto language = arg_list.getValueForOption("--language");
	auto numbands = arg_list.getValueForOption("--num_bands");
	auto balance = arg_list.getValueForOption("--balance");
	auto filterq = arg_list.getValueForOption("--filter_q");
	auto mastergain = arg_list.getValueForOption("--master_gain");
	auto normalization = arg_list.getValueForOption("--normalization");

	if (arg_list.containsOption("--run_minimized"))
	{
		settings_.setBool("run_minimized", true);
	}

	if (power_state.isNotEmpty())
	{
		if (power_state.getIntValue() == 0)
			settings_.setBool("power", false);
		else
			settings_.setBool("power", true);
	}

	if (preset.isNotEmpty())
	{
		settings_.setString("preset", preset);
	}

	if (output_device.isNotEmpty())
	{
		setOutputName(output_device);
	}

	if (view.isNotEmpty())
	{
		auto value = view.getIntValue();
		if (value == ViewType::Lite || value == ViewType::Pro)
		{
			settings_.setInt("view", value);
			view_ = static_cast<ViewType>(value);
		}
	}

	if (language.isEmpty())
	{
		language = settings_.getString("language");
		if (language.isEmpty())
		{
			language = SystemStats::getDisplayLanguage();
		}
	}

	setLanguage(language);

	// ------------------------------------------------------------------------------------------
	//  NumBands / Balance / FilterQ / MasterGain / Normalization - SET ON START
	// ------------------------------------------------------------------------------------------
	int nb = 10;
	if (numbands == "")
	{
		nb = settings_.getInt("num_bands");
	}
	else
	{
		nb = numbands.getIntValue();
	}
	if (nb != 5 && nb != 10 && nb != 15 && nb != 20 && nb != 31) nb = DEFAULT_NUM_EQ_BANDS;
	setNumEqBands(nb);

	float nm = 0;
	if (normalization == "")
	{
		nm = settings_.getDouble("normalization");
	}
	else
	{
		nm = normalization.getFloatValue();
	}
	if (nm < -20 || nm > 0) nm = DEFAULT_NORMALIZATION;
	setNormalization(nm);

	float bl = 0;
	if (balance == "")
	{
		bl = settings_.getDouble("balance");
	}
	else
	{
		bl = balance.getFloatValue();
	}
	if (bl < -20 || bl > +20) bl = DEFAULT_BALANCE;
	setBalance(bl);

	float fq = 0;
	if (filterq == "")
	{
		fq = settings_.getDouble("filter_q");
	}
	else
	{
		fq = filterq.getFloatValue();
	}
	if (fq < 1 || fq > 3) fq = DEFAULT_FILTER_Q;
	setFilterQ(fq);

	float mg = 0;
	if (mastergain == "")
	{
		mg = settings_.getDouble("master_gain");
	}
	else
	{
		mg = mastergain.getFloatValue();
	}
	if (mg < -20 || mg > +20) mg = DEFAULT_MASTER_GAIN;
	setMasterGain(mg);
}

void FxController::applyConfig(const String& commandline)
{
	auto arg_list = ArgumentList(File::getSpecialLocation(File::SpecialLocationType::invokedExecutableFile).getFileName(), commandline);

	if (arg_list.containsOption("--status"))
	{
		printStatus();
		return;
	}

	auto power_state = arg_list.getValueForOption("--power");
	auto preset = arg_list.getValueForOption("--preset").unquoted();
	auto view = arg_list.getValueForOption("--view");
	auto output_device = arg_list.getValueForOption("--output").unquoted();
	auto language = arg_list.getValueForOption("--language");
	auto numbands = arg_list.getValueForOption("--num_bands");
	auto balance = arg_list.getValueForOption("--balance");
	auto filterq = arg_list.getValueForOption("--filter_q");
	auto mastergain = arg_list.getValueForOption("--master_gain");
	auto normalization = arg_list.getValueForOption("--normalization");
	auto band_freq = arg_list.getValueForOption("--set_band_freq").unquoted();
	auto band_gain = arg_list.getValueForOption("--set_band_gain").unquoted();
	auto effect = arg_list.getValueForOption("--set_effect").unquoted();

	if (power_state.isNotEmpty())
	{
		if (power_state.getIntValue() == 0)
			setPowerState(false);
		else
			setPowerState(true);
	}

	if (preset.isNotEmpty())
	{
		if (FxModel::getModel().getPowerState())
		{
			setPreset(preset);
		}
	}

	if (output_device.isNotEmpty())
	{
		setOutputName(output_device);
		std::vector<SoundDevice> sound_devices = audio_passthru_->getSoundDevices();
		for (auto& sound_device : sound_devices)
		{
			if (output_device == sound_device.deviceFriendlyName.c_str())
			{
				setOutput(sound_device.pwszID.c_str());
			}
		}
	}

	// ------------------------------------------------------------------------------------------
	//  NumBands / Balance / FilterQ / MasterGain / Normalization
	// ------------------------------------------------------------------------------------------
	int nb = DEFAULT_NUM_EQ_BANDS;
	if (numbands.isNotEmpty())
	{
		nb = numbands.getIntValue();
		if (nb != 5 && nb != 10 && nb != 15 && nb != 20 && nb != 31) nb = DEFAULT_NUM_EQ_BANDS;
		setNumEqBands(nb);
	}

	float nm = 0;
	if (normalization.isNotEmpty())
	{
		nm = normalization.getFloatValue();
		if (nm < -20 || nm > 0) nm = DEFAULT_NORMALIZATION;
		setNormalization(nm);
	}


	float bl = 0;
	if (balance.isNotEmpty())
	{
		bl = balance.getFloatValue();
		if (bl < -20 || bl > +20) bl = DEFAULT_BALANCE;
		setBalance(bl);
	}

	float fq = 0;
	if (filterq.isNotEmpty())
	{
		fq = filterq.getFloatValue();
		if (fq < 1 || fq > 3) fq = DEFAULT_FILTER_Q;
		setFilterQ(fq);
	}

	float mg = 0;
	if (mastergain.isNotEmpty())
	{
		mg = mastergain.getFloatValue();
		if (mg < -20 || mg > +20) mg = DEFAULT_MASTER_GAIN;
		setMasterGain(mg);
	}

	if (view.isNotEmpty())
	{
		auto value = view.getIntValue();
		if (value == ViewType::Lite || value == ViewType::Pro)
		{
			settings_.setInt("view", value);
			view_ = static_cast<ViewType>(value);
		}
		showView();
	}

	if (language.isNotEmpty())
	{
		setLanguage(language);
	}

	if (arg_list.containsOption("--run_minimized"))
	{
		settings_.setBool("run_minimized", true);
		hideMainWindow();
	}
	else
	{
		showMainWindow();
	}

	// ------------------------------------------------------------------------------------------
	//  Band frequency / gain / effect values - "<index>:<value>[,<index>:<value>...]"
	// ------------------------------------------------------------------------------------------
	if (band_freq.isNotEmpty())
	{
        auto band_frequency_pairs = StringArray::fromTokens(band_freq, ",", "\"");
		if (band_frequency_pairs.size() <= getNumEqBands())
		{
			for (auto& pair : band_frequency_pairs)
			{
				auto band = pair.upToFirstOccurrenceOf(":", false, false).trim().getIntValue();
				auto freq = pair.fromFirstOccurrenceOf(":", false, false).trim().getFloatValue();
				setEqBandFrequency(band, freq);
			}
        }

		if (view_ == ViewType::Pro)
		{
			main_window_->update();
		}
	}

	if (band_gain.isNotEmpty())
	{
		auto band_gain_pairs = StringArray::fromTokens(band_gain, ",", "\"");
		if (band_gain_pairs.size() <= getNumEqBands())
		{
			for (auto& pair : band_gain_pairs)
			{
				auto band = pair.upToFirstOccurrenceOf(":", false, false).trim().getIntValue();
				auto gain = pair.fromFirstOccurrenceOf(":", false, false).trim().getFloatValue();
				setEqBandBoostCut(band, gain);
			}
		}

		if (view_ == ViewType::Pro)
		{
			main_window_->update();
		}
	}

	if (effect.isNotEmpty())
	{
        auto effect_pairs = StringArray::fromTokens(effect, ",", "\"");
		if (effect_pairs.size() <= 5)
		{
			for (auto& pair : StringArray::fromTokens(effect, ",", "\""))
			{
				auto name = pair.upToFirstOccurrenceOf(":", false, false).trim().toLowerCase();
				auto value = pair.fromFirstOccurrenceOf(":", false, false).trim().getFloatValue();

				if (name == "fidelity" || name == "clarity")
					setEffectValue(FxEffects::Fidelity, value);
				else if (name == "ambience")
					setEffectValue(FxEffects::Ambience, value);
				else if (name == "surround")
					setEffectValue(FxEffects::Surround, value);
				else if (name == "dynamicboost" || name == "dynamic_boost")
					setEffectValue(FxEffects::DynamicBoost, value);
				else if (name == "bass" || name == "bassboost" || name == "bass_boost")
					setEffectValue(FxEffects::Bass, value);
			}

			if (view_ == ViewType::Pro)
			{
				main_window_->update();
			}
		}
	}
}

File FxController::getStatusFile()
{
	return File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("FxSound").getChildFile("status.json");
}

void FxController::printStatus()
{
	auto& model = FxModel::getModel();

	auto* settings = new DynamicObject();
	settings->setProperty("version", ProjectInfo::versionString);
	settings->setProperty("power", model.getPowerState());

	Array<var> built_in_presets, user_presets;
	for (int i = 0; i < model.getPresetCount(); i++)
	{
		auto preset = model.getPreset(i);

		auto* preset_obj = new DynamicObject();
		preset_obj->setProperty("name", preset.name);
		preset_obj->setProperty("modified", preset.modified);

		if (preset.type == FxModel::PresetType::UserPreset)
			user_presets.add(var(preset_obj));
		else
			built_in_presets.add(var(preset_obj));
	}

	auto* presets_obj = new DynamicObject();
	presets_obj->setProperty("built_in", built_in_presets);
	presets_obj->setProperty("user_defined", user_presets);
	settings->setProperty("presets", var(presets_obj));
	settings->setProperty("selected_preset", model.getPreset(model.getSelectedPreset()).name);

	Array<var> output_devices;
	for (auto& device : audio_passthru_->getSoundDevices())
	{
		if (device.isRealDevice)
			output_devices.add(String(device.deviceFriendlyName.c_str()));
	}
	settings->setProperty("output_devices", output_devices);
	settings->setProperty("selected_output", getOutputName());

	Array<var> bands;
	for (int i = 0; i < getNumEqBands(); i++)
	{
		auto* band_obj = new DynamicObject();
		band_obj->setProperty("index", i);
		band_obj->setProperty("frequency", getEqBandFrequency(i));
		band_obj->setProperty("gain", getEqBandBoostCut(i));
		bands.add(var(band_obj));
	}

	auto* eq_obj = new DynamicObject();
	eq_obj->setProperty("num_bands", getNumEqBands());
	eq_obj->setProperty("bands", bands);
	settings->setProperty("equalizer", var(eq_obj));

	auto* effects_obj = new DynamicObject();
	effects_obj->setProperty("clarity", getEffectValue(FxEffects::Fidelity) * 10.0f);
	effects_obj->setProperty("ambience", getEffectValue(FxEffects::Ambience) * 10.0f);
	effects_obj->setProperty("surround", getEffectValue(FxEffects::Surround) * 10.0f);
	effects_obj->setProperty("dynamicboost", getEffectValue(FxEffects::DynamicBoost) * 10.0f);
	effects_obj->setProperty("bass", getEffectValue(FxEffects::Bass) * 10.0f);
	settings->setProperty("effects", var(effects_obj));

	auto json_text = JSON::toString(var(settings));

	auto status_file = getStatusFile();
	status_file.getParentDirectory().createDirectory();
	status_file.replaceWithText(json_text);

	// Best-effort console output for interactive use. Attaches to this
	// (already-running) instance's own parent console - i.e. whichever
	// console FxSound.exe was originally launched from - not the console of
	// the process that just forwarded this command.
	if (AttachConsole(ATTACH_PARENT_PROCESS))
	{
		FILE* fp = nullptr;
		freopen_s(&fp, "CONOUT$", "w", stdout);

		std::cout << json_text << std::endl;
		std::cout.flush();

		FreeConsole();
	}
}

void FxController::init(FxMainWindow* main_window, FxSystemTrayView* system_tray_view, AudioPassthru* audio_passthru)
{
	if (!isTimerRunning())
	{
		main_window_ = main_window;
		audio_passthru_ = audio_passthru;
		system_tray_view_ = system_tray_view;

		if (audio_passthru_->init() != 0)
		{
			String message(TRANS("Error in system audio configuration. Unable to run FxSound"));
			AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, JUCEApplication::getInstance()->getApplicationName(), message, TRANS("OK"));

			JUCEApplication::getInstance()->systemRequestedQuit();
			return;
		}

		auto app_version = JUCEApplication::getInstance()->getApplicationVersion();
		auto prev_version = settings_.getString("version");
		if (prev_version != app_version)
		{
			RegDeleteTree(HKEY_CURRENT_USER, L"Software\\DFX");

			FxModel::getModel().pushMessage(" ", { TRANS("Click here to see what's new on this version!"), "https://www.fxsound.com/changelog" });

			settings_.setString("version", app_version);
			settings_.setBool("run_minimized", false);
		}

		audio_passthru_->setDspProcessingModule(&dfx_dsp_);
		initOutputs(audio_passthru_->getSoundDevices(false));
		if (!dfx_enabled_)
		{
			main_window_->removeFromDesktop();
			FxDeviceErrorMessage error_message;
			error_message.runModalLoop();
			JUCEApplication::getInstance()->systemRequestedQuit();
			return;
		}

		audio_passthru_->registerCallback(this);

		auto path = File(File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName()) + L"FxSound\\Presets");
		if (!path.exists())
		{
			path.createDirectory();
		}

		setPowerState(dfx_enabled_ && settings_.getBool("power"));

		initPresets();

		auto preset_name = settings_.getString("preset");
		setPreset(preset_name);

		showView();

		auto theme_mode = settings_.getInt("theme_mode", 0);
		if (theme_mode < 0 || theme_mode >= static_cast<int>(FxThemeMode::NumModes))
		{
			theme_mode = 0;
		}
		if (FxTheme::getThemeMode() != static_cast<FxThemeMode>(theme_mode))
		{
			FxTheme::setThemeMode(static_cast<FxThemeMode>(theme_mode));
			main_window_->sendLookAndFeelChange();

			auto* theme = dynamic_cast<FxTheme*>(&LookAndFeel::getDefaultLookAndFeel());
			if (theme != nullptr)
			{
				theme->loadFont(language_);
			}
		}

		survey_tip_ = !settings_.getBool("survey_displayed");

		if (!settings_.getBool("run_minimized"))
		{
			showMainWindow();
		}
		else
		{
			hideMainWindow();
		}

		auto power = FxModel::getModel().getPowerState();
		main_window_->setIcon(power, false);
		system_tray_view_->setStatus(power, false);

		HMODULE user32_module = GetModuleHandleW(L"user32.dll");
		if (user32_module != nullptr)
		{
			auto register_suspend_resume_notification = reinterpret_cast<RegisterSuspendResumeNotificationFunc>(
				GetProcAddress(user32_module, "RegisterSuspendResumeNotification"));
			unregister_suspend_resume_notification_ = reinterpret_cast<UnregisterSuspendResumeNotificationFunc>(
				GetProcAddress(user32_module, "UnregisterSuspendResumeNotification"));

			if (register_suspend_resume_notification != nullptr && unregister_suspend_resume_notification_ != nullptr)
			{
				powerNotify_ = register_suspend_resume_notification(message_window_.getHandle(), DEVICE_NOTIFY_WINDOW_HANDLE);
			}
		}
	}
}

String FxController::getAutoSavePath() const
{
	auto data_dir = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName());
	return data_dir + L"FxSound\\AutoSave";
}

String FxController::getAutoSavePresetPath(const String& preset_name) const
{
	return getAutoSavePath() + L"\\" + preset_name + ".fac";
}

void FxController::autoSavePreset(int preset_index)
{
	ScopedLock lock(save_lock_);

	auto preset = FxModel::getModel().getPreset(preset_index);
	if (preset.name.isEmpty())
		return;

	auto auto_save_dir = getAutoSavePath();
	File(auto_save_dir).createDirectory();

	dfx_dsp_.savePreset(preset.name.toWideCharPointer(), auto_save_dir.toWideCharPointer());

	preset_dirty_ = false;
	auto_save_counter_ = 0;
}

void FxController::deleteAutoSavedPreset(const String& preset_name)
{
	File auto_save_file(getAutoSavePresetPath(preset_name));
	if (auto_save_file.existsAsFile())
		auto_save_file.deleteFile();

	preset_dirty_ = false;
	auto_save_counter_ = 0;
}

void FxController::initPresets()
{
	Array<FxModel::Preset> presets;
	auto working_dir = File::getCurrentWorkingDirectory();
	FileSearchPath preset_search_path(File::addTrailingSeparator(working_dir.getFullPathName()) + L"Factsoft");
	auto app_preset_paths = preset_search_path.findChildFiles(File::findFiles, false, "*.fac");
	for (auto path : app_preset_paths)
	{
		auto preset_info = dfx_dsp_.getPresetInfo(path.getFullPathName().toWideCharPointer());
		if (!preset_info.name.empty())
		{
			presets.add({ preset_info.name.c_str(), path.getFullPathName(), FxModel::PresetType::AppPreset });
		}
	}

	auto data_dir = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName());
	FileSearchPath user_preset_search_path(data_dir + L"FxSound\\Presets");
	auto user_preset_paths = user_preset_search_path.findChildFiles(File::findFiles, false, "*.fac");
	for (auto path : user_preset_paths)
	{
		auto preset_info = dfx_dsp_.getPresetInfo(path.getFullPathName().toWideCharPointer());
		if (!preset_info.name.empty())
		{
			presets.add({ preset_info.name.c_str(), path.getFullPathName(), FxModel::PresetType::UserPreset });
		}
	}

	// Mark presets that have an auto-saved version as modified
	for (auto& preset : presets)
	{
		if (File(getAutoSavePresetPath(preset.name)).existsAsFile())
			preset.modified = true;
	}

	FxModel::getModel().initPresets(presets);
}

void FxController::showView()
{
	if (view_ == ViewType::Pro)
	{
		main_window_->showProView();
	}
	else
	{
		main_window_->showLiteView();
	}
}

void FxController::switchView()
{
	if (view_ == ViewType::Pro)
	{
		view_ = ViewType::Lite;
		main_window_->showLiteView();
		settings_.setInt("view", static_cast<int>(view_));
	}
	else
	{
		view_ = ViewType::Pro;
		main_window_->showProView();
		settings_.setInt("view", static_cast<int>(view_));
	}
}

ViewType FxController::getCurrentView()
{
	return view_;
}

void FxController::hideMainWindow()
{
	if (main_window_->isOnDesktop())
	{
		main_window_->removeFromDesktop();
		main_window_->setVisible(false);
		settings_.setBool("run_minimized", true);
	}

	if (minimize_tip_)
	{
		minimize_tip_ = false;
		Timer::callAfterDelay(2000, []() {
			FxModel::getModel().pushMessage(TRANS("FxSound in system tray\r\nClick FxSound icon to reopen"));
			});
	}
}

void FxController::showMainWindow()
{
	if (main_window_ != nullptr)
	{
		settings_.setBool("run_minimized", false);
		main_window_->show();

		auto power = FxModel::getModel().getPowerState();
		main_window_->setIcon(power, audio_process_on_);

		if (survey_tip_)
		{
			uint32_t survey_timer = settings_.getInt("survey_timer");
			if (survey_timer == 0)
			{
				survey_timer = std::time(nullptr) + (7 * (24 * 60 * 60));
				settings_.setInt("survey_timer", survey_timer);
			}
			else
			{
				uint32_t current_time = std::time(nullptr);
				if (current_time > survey_timer)
				{
					survey_tip_ = false;
					settings_.setBool("survey_displayed", true);
					String message = TRANS("Thanks for using FxSound! Would you be\r\ninterested in helping us by taking a quick 4 minute\r\nsurvey so we can make FxSound better?");
					if (authenticated_)
					{
						FxModel::getModel().pushMessage(message, { TRANS("Take the survey."), "https://forms.gle/ATx1ayXDWRaMdiR59" });
					}
				}
			}
		}
	}
}

bool FxController::isMainWindowVisible()
{
	if (main_window_ != nullptr)
	{
		return main_window_->isOnDesktop() && main_window_->isVisible();
	}

	return false;
}

void FxController::setMenuClicked(bool clicked)
{
	settings_.setBool("menu_clicked", clicked);
	FxModel::getModel().setMenuClicked(clicked);
}

FxWindow* FxController::getMainWindow()
{
	return main_window_;
}

Point<int> FxController::getSystemTrayWindowPosition(int width, int height)
{
	return system_tray_view_->getSystemTrayWindowPosition(width, height);
}

void FxController::autoSaveModifiedPreset()
{
	auto& model = FxModel::getModel();
	if (model.isPresetModified())
		autoSavePreset(model.getSelectedPreset());
}

bool FxController::exit()
{
	autoSaveModifiedPreset();

	JUCEApplication::getInstance()->systemRequestedQuit();

	return true;
}

void FxController::setPowerState(bool power_state)
{
	FxModel::getModel().setPowerState(power_state);
	powerOn(power_state);
	settings_.setBool("power", power_state);

	system_tray_view_->setStatus(power_state, audio_process_on_);
	main_window_->setIcon(power_state, audio_process_on_);
}

bool FxController::setPreset(const String& preset_name, bool notify)
{
	auto& model = FxModel::getModel();

	auto preset_count = model.getPresetCount();
	for (auto i = 0; i < preset_count; i++)
	{
		if (model.getPreset(i).name == preset_name)
		{
			return setPreset(i, notify);
		}
	}

	return false;
}

bool FxController::setPreset(int selected_index, bool notify)
{
	auto& model = FxModel::getModel();

	if (selected_index < 0 || selected_index >= model.getPresetCount())
	{
		return false;
	}

	auto preset = model.getPreset(selected_index);

	auto current_preset = model.getSelectedPreset();
	// Auto-save the current preset if it was modified before switching away
	if (selected_index != current_preset)
	{
		if (model.isPresetModified(current_preset))
			autoSavePreset(current_preset);
	}

	if (preset.path.isNotEmpty())
	{
		// If this preset has an auto-saved version, load it instead of the original
		auto auto_save_path = getAutoSavePresetPath(preset.name);
		if (File(auto_save_path).existsAsFile())
		{
			dfx_dsp_.loadPreset(auto_save_path.toWideCharPointer());
			model.setPresetModified(selected_index, true);
		}
		else
		{
			dfx_dsp_.loadPreset(preset.path.toWideCharPointer());
			model.setPresetModified(selected_index, false);
		}

		settings_.setString("preset", preset.name);
		model.selectPreset(selected_index, true);

		for (auto e = 0; e < FxEffects::EffectType::NumEffects; e++)
		{
			auto value = dfx_dsp_.getEffectValue(static_cast<DfxDsp::Effect>(e));
			dfx_dsp_.setEffectValue(static_cast<DfxDsp::Effect>(e), value * 10);
		}

		auto num_bands = getNumEqBands();
		for (auto b = 0; b < num_bands; b++)
		{
			dfx_dsp_.setEqBandFrequency(b, dfx_dsp_.getEqBandFrequency(b));
			dfx_dsp_.setEqBandBoostCut(b, dfx_dsp_.getEqBandBoostCut(b));
		}
	}

	if (notify && FxModel::getModel().getPowerState())
	{
		model.pushMessage(TRANS("Preset: ") + model.getPreset(selected_index).name);
	}

	return true;
}

void FxController::setOutput(const String output_device_id, bool notify)
{
	std::vector<SoundDevice> sound_devices = audio_passthru_->getSoundDevices();

	bool output_found = false;

	for (auto sound_device : sound_devices)
	{
		if (sound_device.isRealDevice)
		{
			if (output_device_id == sound_device.pwszID.c_str())
			{
				output_found = true;
				String message = TRANS("Output: ") + sound_device.deviceFriendlyName.c_str();

				if (getOutputName() != sound_device.deviceFriendlyName.c_str())
				{
					// Auto-select preset for the output device
					auto device_config = DeviceConfig::getDeviceConfig(settings_, sound_device.deviceFriendlyName.c_str());
					if (device_config.preset.isNotEmpty())
					{
						setPreset(device_config.preset, false);

						if (FxModel::getModel().getPowerState())
						{
							message += "\n" + TRANS("Preset: ") + device_config.preset;
						}
					}
				}
				FxModel::getModel().setSelectedOutput(sound_device, notify);
				setOutputName(sound_device.deviceFriendlyName.c_str());

				if (!isTimerRunning())
				{
					// FxSound is off and the selected output device is the default playback device
					if (sound_device.isDefaultDevice)
					{
						break;
					}
				}
				else
				{
					if (sound_device.isTargetedRealPlaybackDevice)
					{
						break;
					}
				}

				audio_passthru_->setAsPlaybackDevice(sound_device);
				output_changed_ = true;

				FxModel::getModel().pushMessage(message);

				break;
			}
		}
	}

	if (!output_found)
	{
		audio_passthru_->mute(true);
		powerOn(false);

		FxModel::getModel().pushMessage(TRANS("Output Disconnected"));
	}
	else
	{
		if (FxModel::getModel().getPowerState())
		{
			powerOn(true);
			audio_passthru_->mute(false);
		}
	}

	system_tray_view_->setStatus(FxModel::getModel().getPowerState(), isAudioProcessing());
}

void FxController::setOutput(int output, bool notify)
{
	auto output_devices = FxModel::getModel().getOutputDevices();

	if (output < output_devices.size())
	{
		setOutput(output_devices[output].pwszID.c_str(), notify);
	}
}

bool FxController::isPlaybackDeviceAvailable()
{
	return playback_device_available_;
}

void FxController::checkDeviceChanges()
{
	audio_passthru_->checkDeviceChanges();
}

void FxController::savePreset(const String& preset_name)
{
	ScopedLock lock(save_lock_);

	auto& model = FxModel::getModel();

	auto preset_index = model.getSelectedPreset();
	auto preset = model.getPreset(preset_index);

	if (preset_name.isEmpty())
	{
		auto path = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName()) + L"FxSound\\Presets";
		dfx_dsp_.savePreset(preset.name.toWideCharPointer(), path.toWideCharPointer());

		deleteAutoSavedPreset(preset.name);
		model.setPresetModified(preset_index, false);

		model.pushMessage(FormatString(TRANS(L"Changes to preset %s are saved."), preset.name));
	}
	else
	{
		auto path = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName()) + L"FxSound\\Presets";
		dfx_dsp_.savePreset(preset_name.toWideCharPointer(), path.toWideCharPointer());

		deleteAutoSavedPreset(preset.name);

		initPresets();

		setPreset(preset_name);

		model.pushMessage(FormatString(TRANS("New preset %s is saved."), preset_name));

		if (model.getUserPresetCount() == FxController::getInstance().getMaxUserPresets())
		{
			Thread::sleep(2000);
			model.pushMessage(TRANS("Reached the limit on new presets."));
		}
	}
}

void FxController::renamePreset(const String& new_name)
{
	ScopedLock lock(save_lock_);

	auto& model = FxModel::getModel();

	auto preset_index = model.getSelectedPreset();
	auto preset = model.getPreset(preset_index);

	if (preset.name == new_name) return;
	if (preset.type == FxModel::PresetType::UserPreset)
	{
		auto path = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName()) + L"FxSound\\Presets";
		dfx_dsp_.savePreset(new_name.toWideCharPointer(), path.toWideCharPointer());

		wchar_t old_path[MAX_PATH] = {};
		wcscpy_s(old_path, preset.path.toWideCharPointer());
		if (preset.path.length() + 1 < MAX_PATH)
		{
			old_path[preset.path.length() + 1] = L'\0';
		}
		SHFILEOPSTRUCT file_op = { NULL, FO_DELETE, old_path, L"",
								   FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT, FALSE, 0, L"" };
		SHFileOperation(&file_op);

		initPresets();

		setPreset(new_name);

		deleteAutoSavedPreset(preset.name);
		model.setPresetModified(FxModel::getModel().getSelectedPreset(), false);
	}
}

void FxController::deletePreset()
{
	auto& model = FxModel::getModel();

	auto preset_index = model.getSelectedPreset();
	auto preset = model.getPreset(preset_index);

	if (preset.type == FxModel::PresetType::UserPreset)
	{
		deleteAutoSavedPreset(preset.name);

		wchar_t path[MAX_PATH] = {};

		wcscpy_s(path, preset.path.toWideCharPointer());
		if (preset.path.length() + 1 < MAX_PATH)
		{
			path[preset.path.length() + 1] = L'\0';
		}

		SHFILEOPSTRUCT file_op = { NULL, FO_DELETE, path, L"",
								   FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT, FALSE, 0, L"" };
		SHFileOperation(&file_op);

		initPresets();

		auto device_config = DeviceConfig::getDeviceConfig(settings_, getOutputName());
		if (device_config.preset.isNotEmpty())
		{
			setPreset(device_config.preset, false);
		}
		else
		{
			setPreset(0);
		}

		FxModel::getModel().pushMessage(FormatString(TRANS("Preset %s is deleted."), preset.name));
	}
}

void FxController::undoPreset()
{
	auto& model = FxModel::getModel();

	auto preset_index = model.getSelectedPreset();
	if (!model.isPresetModified(preset_index))
		return;

	model.setPresetModified(preset_index, false);

	// Delete auto-save and clear modified flag before calling setPreset so it loads from original path
	auto preset = model.getPreset(preset_index);
	deleteAutoSavedPreset(preset.name);

	setPreset(preset_index);
}

void FxController::resetPresets()
{
	auto& model = FxModel::getModel();

	setNumEqBands(DEFAULT_NUM_EQ_BANDS);
	setNormalization(DEFAULT_NORMALIZATION);
	setBalance(DEFAULT_BALANCE);
	setFilterQ(DEFAULT_FILTER_Q);
	setMasterGain(DEFAULT_MASTER_GAIN);

	auto count = model.getPresetCount();
	for (auto i = 0; i < count; i++)
	{
		auto preset = model.getPreset(i);
		if (preset.modified)
		{
			deleteAutoSavedPreset(preset.name);
		}

		if (preset.type == FxModel::PresetType::UserPreset)
		{
			wchar_t path[MAX_PATH] = {};

			wcscpy_s(path, preset.path.toWideCharPointer());
			if (preset.path.length() + 1 < MAX_PATH)
			{
				path[preset.path.length() + 1] = L'\0';
			}

			SHFILEOPSTRUCT file_op = { NULL, FO_DELETE, path, L"",
									   FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT, FALSE, 0, L"" };
			SHFileOperation(&file_op);
		}
	}

	initPresets();

	auto device_config = DeviceConfig::getDeviceConfig(settings_, getOutputName());
	if (device_config.preset.isNotEmpty())
	{
		setPreset(device_config.preset, false);
	}
	else
	{
		setPreset(0);
	}

	FxModel::getModel().pushMessage(TRANS("Presets are restored to factory defaults"));
}

bool FxController::exportPresets(const Array< FxModel::Preset>& presets)
{
	auto path_name = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getFullPathName()) + L"FxSound\\Presets\\Export\\";

	File path(path_name);
	if (!path.exists())
	{
		path.createDirectory();
	}

	bool exported = false;

	for (auto preset : presets)
	{
		bool skip = false;

		auto preset_file = File(path_name + preset.name + ".fac");
		if (preset_file.exists())
		{
			if (!FxConfirmationMessage::showMessage(String::formatted(TRANS("Preset file %s already exists in the export path, do you want to overwrite the preset file?"), preset.name.toWideCharPointer())))
			{
				skip = true;
			}
		}

		if (!skip)
		{
			dfx_dsp_.exportPreset(preset.path.toWideCharPointer(), preset.name.toWideCharPointer(), path.getFullPathName().toWideCharPointer());
			exported = true;
		}
	}

	return exported;
}

bool FxController::importPresets(const Array<File>& preset_files, StringArray& imported_presets, StringArray& skipped_presets)
{
	auto path_name = File::addTrailingSeparator(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName()) + L"FxSound\\Presets";
	File path(path_name);
	if (!path.exists())
	{
		path.createDirectory();
	}

	auto& model = FxModel::getModel();

	for (auto preset_file : preset_files)
	{
		auto preset_info = dfx_dsp_.getPresetInfo(preset_file.getFullPathName().toWideCharPointer());
		if (model.isPresetNameValid(preset_info.name.c_str()))
		{
			File import_file(path_name + "\\" + preset_info.name.c_str() + ".fac");
			if (preset_file.copyFileTo(import_file))
			{
				imported_presets.add(preset_info.name.c_str());
			}
		}
		else
		{
			skipped_presets.add(preset_info.name.c_str());
		}
	}

	if (!imported_presets.isEmpty())
	{
		initPresets();

		auto preset_name = settings_.getString("preset");
		setPreset(preset_name);

		return true;
	}

	return false;
}

void FxController::initOutputs(std::vector<SoundDevice>& sound_devices)
{
	dfx_enabled_ = false;
	active_output_devices_.clear();

	SoundDevice default_output;

	auto device_configs_version = settings_.getInt("device_configs_version");
	if (device_configs_version != 2)
	{
		DeviceConfig::initDeviceConfigs(settings_, sound_devices);
		settings_.setInt("device_configs_version", 2);
	}
	else
	{
		auto device_configs = DeviceConfig::loadDeviceConfigs(settings_, "device_configs");
		if (device_configs.size() == 0)
		{
			// First time running the app, initialize device configs with sound devices
			DeviceConfig::initDeviceConfigs(settings_, sound_devices);
			settings_.setInt("device_configs_version", 2);
		}
		else
		{
			// Update device configs when relaunching the app
			DeviceConfig::updateDeviceConfigs(settings_, sound_devices);
		}
	}


	for (auto sound_device : sound_devices)
	{
		if (sound_device.isRealDevice)
		{
			if (sound_device.isActive && sound_device.deviceNumChannel >= 2)
			{
				active_output_devices_.push_back(sound_device);

				if (sound_device.isDefaultDevice || sound_device.isTargetedRealPlaybackDevice)
				{
					default_output = sound_device;
				}
			}
		}
		else if (sound_device.deviceFriendlyName.find(L"FxSound Audio Enhancer") != std::wstring::npos)
		{
			dfx_enabled_ = true;
		}
	}

	sortByDeviceConfigPriority(active_output_devices_);

	// If output is not set previously, use the system default output.
	if (getOutputName().isEmpty() && default_output.deviceFriendlyName.size() > 0)
	{
		setOutputName(default_output.deviceFriendlyName.c_str());
	}
	else if (active_output_devices_.size() > 0)
	{
		bool found = false;
		for (auto& output_device : active_output_devices_)
		{
			if (getOutputName() == output_device.deviceFriendlyName.c_str())
			{
				default_output = output_device;
				found = true;
				break;
			}
		}
		if (!found)
		{
			default_output = getPreferredOutput();
			setOutputName(default_output.deviceFriendlyName.c_str());
		}
	}

	FxModel::getModel().initOutputs(active_output_devices_);
	setOutput(default_output.pwszID.c_str());
}

void FxController::updateOutputs(std::vector<SoundDevice>& sound_devices)
{
	auto prev_active_devices = active_output_devices_;

	active_output_devices_.clear();

	for (auto sound_device : sound_devices)
	{
		if (sound_device.isActive && sound_device.isRealDevice && sound_device.deviceNumChannel >= 2)
		{
			active_output_devices_.push_back(sound_device);
		}
	}

	sortByDeviceConfigPriority(active_output_devices_);

	SoundDevice preferred_device;

	// New device is connected, find the preferred device if the new device has higher priority than the current output device
	if (sound_devices.size() > device_count_)
	{
		auto device_configs = DeviceConfig::loadDeviceConfigs(settings_, "device_configs");

		for (auto& sound_device : sound_devices)
		{
			if (!sound_device.isRealDevice || sound_device.deviceNumChannel < 2)
				continue;

			// Check if the device is a newly connected device
			auto it = std::find_if(prev_active_devices.begin(), prev_active_devices.end(),
				[&](const SoundDevice& existing) {
					return existing.pwszID == sound_device.pwszID;
				});

			if (it == prev_active_devices.end())
			{
				// If the new device has higher priority, select it as output
				if (compareOutputDevicePriority(sound_device.deviceFriendlyName.c_str(), getOutputName(), device_configs) < 0)
				{
					preferred_device = sound_device;
					break;
				}
			}
		}
	}

	// Output device is removed or a higher priority device is not connected.
	// If the current output device is removed, select the preferred output device.
	// Otherwise, keep the current output device.
	if (preferred_device.pwszID.empty() && active_output_devices_.size() > 0)
	{
		bool found = false;
		for (auto& output_device : active_output_devices_)
		{
			if (getOutputName() == output_device.deviceFriendlyName.c_str())
			{
				preferred_device = output_device;
				found = true;
				break;
			}
		}
		if (!found)
		{
			preferred_device = getPreferredOutput();
		}
	}

	// Initialize the model to update the output devices in the UI.
	FxModel::getModel().initOutputs(active_output_devices_);
	// Set the output to select the new output and update the UI.
	// If the output has not changed, still output has to be set to update the UI.
	setOutput(preferred_device.pwszID.c_str());
}

// Handled when FxSound processing is on
void FxController::selectProcessingOutput(std::vector<SoundDevice>& sound_devices)
{
	auto available = audio_passthru_->isPlaybackDeviceAvailable();
	if (available != playback_device_available_)
	{
		playback_device_available_ = available;
		FxModel::getModel().notifyOutputError();
	}

	// New device is added or removed, so update the output device list and select a preferred output device based on the updated device list.
	if (sound_devices.size() != device_count_)
	{
		updateOutputs(sound_devices);
		device_count_ = (uint32_t)sound_devices.size();

		if (!dfx_enabled_)
		{
			stopTimer();
			main_window_->removeFromDesktop();
			FxDeviceErrorMessage error_message;
			error_message.runModalLoop();
			JUCEApplication::getInstance()->systemRequestedQuit();
			return;
		}
	}
	else
	{
		for (auto sound_device : sound_devices)
		{
			// If the selected output device is different from the output device selected in application UI, update the application UI.
			if (sound_device.isRealDevice && sound_device.isTargetedRealPlaybackDevice && getOutputName() != sound_device.deviceFriendlyName.c_str())
			{
				setOutputName(sound_device.deviceFriendlyName.c_str());
				FxModel::getModel().setSelectedOutput(sound_device);

				auto device_config = DeviceConfig::getDeviceConfig(settings_, getOutputName());
				if (device_config.preset.isNotEmpty())
				{
					setPreset(device_config.preset, false);
				}

				break;
			}
		}
	}
}

// Handled when FxSound processing is off
void FxController::syncOutputWithSystemDefault(std::vector<SoundDevice>& sound_devices)
{
	active_output_devices_.clear();

	for (auto sound_device : sound_devices)
	{
		if (sound_device.isRealDevice && sound_device.deviceNumChannel >= 2)
		{
			active_output_devices_.push_back(sound_device);
		}
	}

	sortByDeviceConfigPriority(active_output_devices_);

	// Update the configuration as per the change in active devices and update the output device list in UI
	if (sound_devices.size() != device_count_)
	{
		FxModel::getModel().initOutputs(active_output_devices_);

		device_count_ = (uint32_t)sound_devices.size();
	}

	bool default_device_found = false;
	for (auto& output_device : active_output_devices_)
	{
		// Change the selected device in UI
		if (output_device.isDefaultDevice)
		{
			setOutputName(output_device.deviceFriendlyName.c_str());
			FxModel::getModel().setSelectedOutput(output_device);

			auto device_config = DeviceConfig::getDeviceConfig(settings_, getOutputName());
			if (device_config.preset.isNotEmpty())
			{
				setPreset(device_config.preset, false);
			}

			default_device_found = true;
			break;
		}
	}

	if (!default_device_found)
	{
		setOutput(getPreferredOutput().pwszID.c_str());
	}
}

void FxController::sortByDeviceConfigPriority(std::vector<SoundDevice>& devices)
{
	auto device_configs = DeviceConfig::loadDeviceConfigs(settings_, "device_configs");

	std::stable_sort(devices.begin(), devices.end(),
		[&](const SoundDevice& a, const SoundDevice& b) {
			return compareOutputDevicePriority(
				String(a.deviceFriendlyName.c_str()),
				String(b.deviceFriendlyName.c_str()),
				device_configs) < 0;
		});
}

void FxController::powerOn(bool on)
{
	if (on)
	{
		dfx_dsp_.powerOn(true);

		if (!isTimerRunning())
		{
			startTimer(100);
		}
	}
	else
	{
		dfx_dsp_.powerOn(false);

		if (isTimerRunning())
		{
			stopTimer();
		}

		audio_passthru_->restoreDefaultPlaybackDevice();
	}
}

float FxController::getEffectValue(FxEffects::EffectType effect)
{
	return dfx_dsp_.getEffectValue(static_cast<DfxDsp::Effect>(effect));
}

void FxController::setEffectValue(FxEffects::EffectType effect, float value)
{
	if (value < 0 || value > 10)
	{
		return;
    }

	dfx_dsp_.setEffectValue(static_cast<DfxDsp::Effect>(effect), value);

	auto& model = FxModel::getModel();
	if (!model.isPresetModified())
	{
		model.setPresetModified(model.getSelectedPreset(), true);
	}
	preset_dirty_ = true;
}

int FxController::getNumEqBands()
{
	return dfx_dsp_.getNumEqBands();
}

void FxController::setNumEqBands(int num_bands)
{
	dfx_dsp_.setNumBands(num_bands);
	settings_.setInt("num_bands", num_bands);

	auto& model = FxModel::getModel();
	if (!model.isPresetModified())
	{
		setPreset(model.getSelectedPreset(), false);
	}
}

float FxController::getNormalization()
{
	return dfx_dsp_.getNormalization();
}

void FxController::setNormalization(float normalization_db)
{
	dfx_dsp_.setNormalization(normalization_db);
	settings_.setDouble("normalization", normalization_db);
}

float FxController::getBalance()
{
	return dfx_dsp_.getBalance();
}

void FxController::setBalance(float balance_db)
{
	dfx_dsp_.setBalance(balance_db);
	settings_.setDouble("balance", balance_db);
}

float FxController::getMasterGain()
{
	return dfx_dsp_.getMasterGain();
}

void FxController::setMasterGain(float gain_db)
{
	dfx_dsp_.setMasterGain(gain_db);
	settings_.setDouble("master_gain", gain_db);
}

float FxController::getFilterQ()
{
	return dfx_dsp_.getFilterQ();
}

void FxController::setFilterQ(float q_multiplier)
{
	dfx_dsp_.setFilterQ(q_multiplier);
	settings_.setDouble("filter_q", q_multiplier);
}

bool FxController::isAudioProcessing()
{
	return audio_process_on_;
}

float FxController::getEqBandFrequency(int band_num)
{
	if (band_num < getNumEqBands())
	{
		return dfx_dsp_.getEqBandFrequency(band_num);
	}
	else
	{
		return 0;
	}
}

void FxController::setEqBandFrequency(int band_num, float freq)
{
	if (band_num < getNumEqBands())
	{
        float min_freq, max_freq;
        getEqBandFrequencyRange(band_num, &min_freq, &max_freq);
		if (freq < min_freq || freq > max_freq)
		{
			return;
        }

		dfx_dsp_.setEqBandFrequency(band_num, freq);

		auto& model = FxModel::getModel();
		if (!model.isPresetModified())
		{
			model.setPresetModified(model.getSelectedPreset(), true);
		}
		preset_dirty_ = true;
	}
}

void FxController::getEqBandFrequencyRange(int band_num, float* min_freq, float* max_freq)
{
	if (band_num < getNumEqBands())
	{
		dfx_dsp_.getEqBandFrequencyRange(band_num, min_freq, max_freq);
	}
	else
	{
		*min_freq = 0;
		*max_freq = 0;
	}
}

float FxController::getEqBandBoostCut(int band_num)
{
	if (band_num < getNumEqBands())
	{
		return dfx_dsp_.getEqBandBoostCut(band_num);
	}
	else
	{
		return 0;
	}
}

void FxController::setEqBandBoostCut(int band_num, float boost)
{
	if (band_num < getNumEqBands())
	{
		if (boost < MIN_GAIN || boost > MAX_GAIN)
		{
			return;
        }

		dfx_dsp_.setEqBandBoostCut(band_num, boost);

		auto& model = FxModel::getModel();
		if (!model.isPresetModified())
		{
			model.setPresetModified(model.getSelectedPreset(), true);
		}
		preset_dirty_ = true;
	}
}

LRESULT CALLBACK FxController::eventCallback(HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param)
{
	static auto os = SystemStats::getOperatingSystemType();
	FxController* controller = (FxController*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_HOTKEY:
	{
		if (w_param == CMD_ON_OFF)
		{
			auto power_state = !FxModel::getModel().getPowerState();
			controller->setPowerState(power_state);

			String param = FxModel::getModel().getPowerState() ? TRANS(L"on") : TRANS(L"off");
			FxModel::getModel().pushMessage(controller->FormatString(TRANS("FxSound is %s."), param));
		}
		if (w_param == CMD_OPEN_CLOSE)
		{
			if (controller->main_window_->isOnDesktop())
			{
				controller->hideMainWindow();
			}
			else
			{
				controller->showMainWindow();
			}
		}
		if (w_param == CMD_NEXT_PRESET && FxModel::getModel().getPowerState())
		{
			auto preset_index = FxModel::getModel().getSelectedPreset();
			auto preset_count = FxModel::getModel().getPresetCount();
			if (preset_count > 1)
			{
				if (preset_index < preset_count - 1)
				{
					preset_index++;
				}
				else
				{
					preset_index = 0;
				}

				controller->setPreset(preset_index);
			}
		}
		if (w_param == CMD_PREVIOUS_PRESET && FxModel::getModel().getPowerState())
		{
			auto preset_index = FxModel::getModel().getSelectedPreset();
			auto preset_count = FxModel::getModel().getPresetCount();
			if (preset_count > 1)
			{
				if (preset_index != 0)
				{
					preset_index--;
				}
				else
				{
					preset_index = preset_count - 1;
				}

				controller->setPreset(preset_index);
			}
		}
		if (w_param == CMD_NEXT_OUTPUT)
		{
			auto output_index = 0;
			for (auto& output_device : controller->active_output_devices_)
			{
				if (FxModel::getModel().getSelectedOutput().pwszID == output_device.pwszID)
				{
					break;
				}
				output_index++;
			}
			int count = 0;
			while (count < controller->active_output_devices_.size())
			{
				count++;
				if (output_index < controller->active_output_devices_.size() - 1)
				{
					output_index++;
				}
				else
				{
					output_index = 0;
				}

				if (controller->active_output_devices_[output_index].deviceNumChannel >= 2)
				{
					controller->setOutput(output_index);
					break;
				}
			}
		}
	}
	break;

	case WM_POWERBROADCAST:
	{
		if (w_param == PBT_APMSUSPEND)
		{
			controller->onSystemSuspend();
		}
		else if (w_param == PBT_APMRESUMESUSPEND || w_param == PBT_APMRESUMEAUTOMATIC)
		{
			controller->onSystemResume();
		}
	}
	break;

	case WM_WTSSESSION_CHANGE:
	{
		WPARAM login_event = (os == SystemStats::OperatingSystemType::Windows7) ? WTS_CONSOLE_CONNECT : WTS_SESSION_DESKTOP_READY;

		if (w_param == login_event || w_param == WTS_SESSION_UNLOCK)
		{
			controller->setPowerState(FxModel::getModel().getPowerState());
		}
		else if (w_param == WTS_CONSOLE_DISCONNECT)
		{
			controller->powerOn(false);
		}
	}
	}

	return DefWindowProc(hwnd, message, w_param, l_param);
}

void FxController::timerCallback()
{
	if (output_changed_)
	{
		output_changed_ = false;
		Thread::sleep(200);
		return;
	}

	audio_passthru_->processTimer();
	auto total_audio_process_time = dfx_dsp_.getTotalAudioProcessedTime();
	if (audio_process_time_ != total_audio_process_time)
	{
		audio_process_time_ = total_audio_process_time;
		audio_process_on_counter_++;
		audio_process_off_counter_ = 0;
	}
	else
	{
		audio_process_off_counter_++;
		audio_process_on_counter_ = 0;
	}

	auto power = FxModel::getModel().getPowerState();
	if (audio_process_on_counter_ == 5 && !audio_process_on_)
	{
		audio_process_on_ = true;
		system_tray_view_->setStatus(power, true);
		main_window_->setIcon(power, true);
		main_window_->startLogoAnimation();
		if (view_ == ViewType::Pro)
		{
			main_window_->showProView();
			main_window_->startVisualizer();
		}
	}
	if (audio_process_off_counter_ == 5 && audio_process_on_)
	{
		audio_process_on_ = false;
		system_tray_view_->setStatus(power, false);
		main_window_->setIcon(power, false);
		main_window_->stopLogoAnimation();
		if (view_ == ViewType::Pro)
		{
			main_window_->showProView();
			main_window_->pauseVisualizer();
		}
	}

	// Auto-save modified preset once per minute if there are unsaved changes
	static constexpr int AUTO_SAVE_INTERVAL = 600; // 600 ticks * 100ms = 60 seconds
	if (++auto_save_counter_ >= AUTO_SAVE_INTERVAL)
	{
		if (preset_dirty_)
		{
			autoSavePreset(FxModel::getModel().getSelectedPreset());
		}
		auto_save_counter_ = 0;
	}

	auto current_time = Time::getCurrentTime();
	if (auto_updates_ && current_time.getHours() == 10 && current_time.getMinutes() == 00 && current_time.getSeconds() == 0)
	{
		checkUpdates();
	}
}

void FxController::onSoundDeviceChange(bool processing)
{
	if (session_id_ != WTSGetActiveConsoleSessionId())
		return; // Ignore device changes on another user session

	ScopedLock auto_lock(lock_);

	if (isTimerRunning())
	{
		if (processing)
		{
			DeviceConfig::updateDeviceConfigs(settings_, audio_passthru_->getSoundDevices(false));
			auto sound_devices = audio_passthru_->getSoundDevices(true);
			selectProcessingOutput(sound_devices);
		}
	}
	else
	{
		DeviceConfig::updateDeviceConfigs(settings_, audio_passthru_->getSoundDevices(false));
		auto sound_devices = audio_passthru_->getSoundDevices(true);
		syncOutputWithSystemDefault(sound_devices);
	}
}

void FxController::onSystemSuspend()
{
	if (FxModel::getModel().getPowerState())
	{
		audio_passthru_->mute(true);
	}
}

void FxController::onSystemResume()
{
	if (FxModel::getModel().getPowerState())
	{
		audio_passthru_->mute(false);
	}
}


void FxController::enableHotkeys(bool enable)
{
	settings_.setBool("hotkeys", enable);
	FxModel::getModel().setHotkeySupport(enable);
	if (enable)
	{
		registerHotkeys();
	}
	else
	{
		unregisterHotkeys();
	}
}

bool FxController::getHotkey(String cmdKey, int& mod, int& vk)
{
	int cmd = settings_.getInt(cmdKey);
	mod = (cmd >> 16) & 0x7;
	vk = cmd & 0xff;
	if ((mod == (MOD_CONTROL | MOD_ALT) || mod == (MOD_CONTROL | MOD_SHIFT)) && ((vk >= 0x30 && vk <= 0x39) || (vk >= 'A' && vk <= 'Z')))
	{
		return true;
	}

	mod = 0;
	vk = 0;

	return false;
}

bool FxController::setHotkey(const String& command, int new_mod, int new_vk)
{
	StringArray hotkey_cmds = { HK_CMD_ON_OFF, HK_CMD_OPEN_CLOSE, HK_CMD_NEXT_PRESET, HK_CMD_PREVIOUS_PRESET, HK_CMD_NEXT_OUTPUT };

	for (int i = 0; i < hotkey_cmds.size(); i++)
	{
		if (command == hotkey_cmds[i])
		{
			continue;
		}

		int mod, vk;
		if (getHotkey(hotkey_cmds[i], mod, vk))
		{
			if (mod == new_mod && vk == new_vk)
			{
				return false;
			}
		}
	}

	unsigned int code = (new_mod << 16) | new_vk;
	if (!isValidHotkey(new_mod, new_vk))
	{
		code = 0;
	}

	int hk_id = 0;

	if (command == HK_CMD_ON_OFF)
	{
		hk_id = CMD_ON_OFF;
	}

	if (command == HK_CMD_OPEN_CLOSE)
	{
		hk_id = CMD_OPEN_CLOSE;
	}

	if (command == HK_CMD_NEXT_PRESET)
	{
		hk_id = CMD_NEXT_PRESET;
	}

	if (command == HK_CMD_PREVIOUS_PRESET)
	{
		hk_id = CMD_PREVIOUS_PRESET;
	}

	if (command == HK_CMD_NEXT_OUTPUT)
	{
		hk_id = CMD_NEXT_OUTPUT;
	}

	if (hk_id != 0)
	{
		::UnregisterHotKey(message_window_.getHandle(), hk_id);

		if (code == 0)
		{
			settings_.setInt(command, code);
			return false;
		}

		if (::RegisterHotKey(message_window_.getHandle(), hk_id, new_mod, new_vk) != FALSE)
		{
			settings_.setInt(command, code);
			return true;
		}
		else
		{
			settings_.setInt(command, 0);
			return false;
		}
	}

	return false;
}

bool FxController::isValidHotkey(int mod, int vk)
{
	if ((mod & MOD_CONTROL) == 0)
	{
		return false;
	}

	HKL hkl = GetKeyboardLayout(0);
	BYTE kbd_state[256] = { 0 };
	kbd_state[VK_CONTROL] = 0x80;

	if ((mod & MOD_ALT) != 0)
	{
		kbd_state[VK_MENU] = 0x80;
	}
	if ((mod & MOD_SHIFT) != 0)
	{
		kbd_state[VK_SHIFT] = 0x80;
	}

	WCHAR output[3] = { 0 };

	int len = ToUnicodeEx(vk, 0, kbd_state, output, 3, 0, hkl);
	if (len > 0 && output[0] >= 0x20) // The hotkey combination generates a non-control character
	{
		return false;
	}

	return true;
}

bool FxController::isHelpTooltipsHidden()
{
	return hide_help_tooltips_;
}

void FxController::setHelpTooltipsHidden(bool status)
{
	hide_help_tooltips_ = status;
	settings_.setBool("hide_help_tooltips", status);
	main_window_->repaint();
}

bool FxController::isNotificationsHidden()
{
	return hide_notifications_;
}

void FxController::setNotificationsHidden(bool status)
{
	hide_notifications_ = status;
	settings_.setBool("hide_notifications", status);
}

String FxController::getLanguage() const
{
	return language_;
}

void FxController::setLanguage(String language_code)
{
	if (language_code.isEmpty())
	{
		language_code = "en";
	}

	language_ = language_code;
	settings_.setString("language", language_);

	LocalisedStrings::setCurrentMappings(nullptr);

	if (language_.startsWithIgnoreCase("ko"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ko_txt, BinaryData::FxSound_ko_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("vi"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_vi_txt, BinaryData::FxSound_vi_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("id"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_id_txt, BinaryData::FxSound_id_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("pt-br"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ptbr_txt, BinaryData::FxSound_ptbr_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("pt"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_pt_txt, BinaryData::FxSound_pt_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("es"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_es_txt, BinaryData::FxSound_es_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("zh-CN"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_zhCN_txt, BinaryData::FxSound_zhCN_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("zh-TW"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_zhTW_txt, BinaryData::FxSound_zhTW_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("fr"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_fr_txt, BinaryData::FxSound_fr_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("sv"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_sv_txt, BinaryData::FxSound_sv_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("it"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_it_txt, BinaryData::FxSound_it_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("ru"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ru_txt, BinaryData::FxSound_ru_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("ro"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ro_txt, BinaryData::FxSound_ro_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("tr"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_tr_txt, BinaryData::FxSound_tr_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("pl"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_pl_txt, BinaryData::FxSound_pl_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("de"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_de_txt, BinaryData::FxSound_de_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("hu"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::fxsound_hu_txt, BinaryData::fxsound_hu_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("th"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_th_txt, BinaryData::FxSound_th_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("nl"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_nl_txt, BinaryData::FxSound_nl_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("ja"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ja_txt, BinaryData::FxSound_ja_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("ar"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ar_txt, BinaryData::FxSound_ar_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("hr"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_hr_txt, BinaryData::FxSound_hr_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("ba"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ba_txt, BinaryData::FxSound_ba_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("fa"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_fa_txt, BinaryData::FxSound_fa_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("ua"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_ua_txt, BinaryData::FxSound_ua_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("no"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_no_txt, BinaryData::FxSound_no_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("sl"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_sl_txt, BinaryData::FxSound_sl_txtSize), false));
	}
	else if (language_.startsWithIgnoreCase("fi"))
	{
		LocalisedStrings::setCurrentMappings(new LocalisedStrings(String::createStringFromData(BinaryData::FxSound_fi_txt, BinaryData::FxSound_fi_txtSize), false));
	}

	auto* theme = dynamic_cast<FxTheme*>(&LookAndFeel::getDefaultLookAndFeel());
	if (theme != nullptr)
	{
		theme->loadFont(language_);
	}

	if (main_window_ != nullptr)
	{
		main_window_->sendLookAndFeelChange();
	}
}

String FxController::getLanguageName(String language_code) const
{
	if (language_code.startsWithIgnoreCase("en"))
	{
		return "English";
	}
	else if (language_code.startsWithIgnoreCase("ko"))
	{
		return L"\ud55c\uad6d\uc5b4";
	}
	else if (language_code.startsWithIgnoreCase("vi"))
	{
		return L"Ti\u1ebfng Vi\u1ec7t";
	}
	else if (language_code.startsWithIgnoreCase("id"))
	{
		return L"bahasa Indonesia";
	}
	else if (language_code.startsWithIgnoreCase("pt-br"))
	{
		return L"portugu\u00eas brasileiro";
	}
	else if (language_code.startsWithIgnoreCase("pt"))
	{
		return L"Portugu\u00eas";
	}
	else if (language_code.startsWithIgnoreCase("es"))
	{
		return L"Espa\u00f1ol";
	}
	else if (language_code.startsWithIgnoreCase("zh-CN"))
	{
		return L"\u7b80\u4f53\u4e2d\u6587";
	}
	else if (language_code.startsWithIgnoreCase("zh-TW"))
	{
		return L"\u7e41\u9ad4\u4e2d\u6587";
	}
	else if (language_code.startsWithIgnoreCase("sv"))
	{
		return L"svenska";
	}
	else if (language_code.startsWithIgnoreCase("fr"))
	{
		return L"fran\u00e7ais";
	}
	else if (language_code.startsWithIgnoreCase("it"))
	{
		return L"Italiano";
	}
	else if (language_code.startsWithIgnoreCase("ru"))
	{
		return L"\u0440\u0443\u0441\u0441\u043a\u0438\u0439";
	}
	else if (language_code.startsWithIgnoreCase("ro"))
	{
		return L"Rom\u00e2n\u0103";
	}
	else if (language_code.startsWithIgnoreCase("tr")) {
		return L"T\u00fcrk";
	}
	else if (language_code.startsWithIgnoreCase("pl"))
	{
		return L"Polski";
	}
	else if (language_code.startsWithIgnoreCase("de"))
	{
		return L"Deutsch";
	}
	else if (language_code.startsWithIgnoreCase("hu"))
	{
		return L"Magyar";
	}
	else if (language_code.startsWithIgnoreCase("th"))
	{
		return L"\u0e41\u0e1a\u0e1a\u0e44\u0e17\u0e22";
	}
	else if (language_code.startsWithIgnoreCase("nl"))
	{
		return L"Nederlands";
	}
	else if (language_code.startsWithIgnoreCase("ja"))
	{
		return L"\u65e5\u672c\u8a9e";
	}
	else if (language_code.startsWithIgnoreCase("ar"))
	{
		return L"\u0627\u0644\u0639\u0631\u0628\u064a\u0629";
	}
	else if (language_code.startsWithIgnoreCase("hr"))
	{
		return L"hrvatski";
	}
	else if (language_code.startsWithIgnoreCase("ba"))
	{
		return L"bosanski";
	}
	else if (language_code.startsWithIgnoreCase("fa"))
	{
		return L"\u0641\u0627\u0631\u0633\u06cc";
	}
	else if (language_code.startsWithIgnoreCase("ua"))
	{
		return L"\u0443\u043a\u0440\u0430\u0457\u043d\u0441\u044c\u043a\u0430";
	}
	else if (language_code.startsWithIgnoreCase("no"))
	{
		return L"Norsk";
	}
	else if (language_code.startsWithIgnoreCase("sl"))
	{
		return L"Sloven\u0161\u010dina";
	}
	else if (language_code.startsWithIgnoreCase("fi"))
	{
		return L"Suomi";
	}

	return "English";
}

int FxController::getMaxUserPresets() const
{
	return max_user_presets_;
}

bool FxController::getAutoUpdates()
{
	return auto_updates_;
}

void FxController::setAutoUpdates(bool enable)
{
	auto_updates_ = enable;
	settings_.setBool("automatic_updates", enable);
}

void FxController::checkUpdates()
{
	if (!isAudioProcessing())
	{
		auto current_time = std::time(nullptr);
		uint32_t last_update_time = settings_.getInt("last_update_time", 0);

		if ((current_time - last_update_time) > (24 * 60 * 60))
		{
			settings_.setInt("last_update_time", static_cast<uint32_t>(current_time));

			ChildProcess child_process;
			child_process.start("updater.exe /silent");
		}
	}
}

void FxController::saveWindowPosition(int x, int y)
{
	settings_.setInt("window_x", x);
	settings_.setInt("window_y", y);
}

void FxController::getWindowPosition(int& x, int& y)
{
	x = settings_.getInt("window_x", 0);
	y = settings_.getInt("window_y", 0);
}

juce::Array<DeviceConfig> FxController::getDeviceConfigs()
{
	return DeviceConfig::loadDeviceConfigs(settings_, "device_configs");
}

void FxController::saveDeviceConfigs(const juce::Array<DeviceConfig>& device_configs)
{
	DeviceConfig::saveDeviceConfigs(settings_, "device_configs", device_configs);
}

bool FxController::isOutputDeviceConnected(const String& output_device_name)
{
	for (auto& output_device : active_output_devices_)
	{
		if (output_device_name == output_device.deviceFriendlyName.c_str())
		{
			return true;
		}
	}

	return false;
}

SoundDevice FxController::getPreferredOutput()
{
	auto device_configs = DeviceConfig::loadDeviceConfigs(settings_, "device_configs");

	for (auto& device_config : device_configs)
	{
		for (auto& device : active_output_devices_)
		{
			if (device_config.device_name == device.deviceFriendlyName.c_str())
			{
				return device;
			}
		}
	}

	if (active_output_devices_.size() > 0)
	{
		return active_output_devices_[0];
	}

	return {};
}

int FxController::compareOutputDevicePriority(const String& output_device_name1, const String& output_device_name2, const juce::Array<DeviceConfig>& device_configs)
{
	int priority1 = (int)device_configs.size();
	int priority2 = (int)device_configs.size();
	int index = 0;

	for (auto& device_config : device_configs)
	{
		if (device_config.device_name == output_device_name1)
			priority1 = index;
		if (device_config.device_name == output_device_name2)
			priority2 = index;

		if (priority1 != (int)device_configs.size() && priority2 != (int)device_configs.size())
			break;

		index++;
	}

	return priority1 - priority2;
}

void FxController::refreshOutputList()
{
	sortByDeviceConfigPriority(active_output_devices_);
	FxModel::getModel().initOutputs(active_output_devices_);
	FxModel::getModel().setSelectedOutput(FxModel::getModel().getSelectedOutput());
}

const String& FxController::getOutputName()
{
	if (output_device_name_.isEmpty())
	{
		output_device_name_ = settings_.getString("output_device_name");
	}

	return output_device_name_;
}

void FxController::setOutputName(const String& output_device_name)
{
	output_device_name_ = output_device_name;
	settings_.setString("output_device_name", output_device_name);
}

bool FxController::isNewOutputPrioritized()
{
	return settings_.getBool("prioritize_new_output", false);
}

void FxController::setNewOutputPrioritized(bool prioritize)
{
	settings_.setBool("prioritize_new_output", prioritize);
}

FxThemeMode FxController::getThemeMode()
{
	return FxTheme::getThemeMode();
}

void FxController::setThemeMode(FxThemeMode mode)
{
	if (mode == getThemeMode())
	{
		return;
	}

	FxTheme::setThemeMode(mode);
	settings_.setInt("theme_mode", static_cast<int>(mode));
	setLanguage(getLanguage()); // To reload font for the new theme
	main_window_->sendLookAndFeelChange();

	auto power = FxModel::getModel().getPowerState();
	main_window_->setIcon(power, audio_process_on_);
	system_tray_view_->setStatus(power, audio_process_on_);
}

bool FxController::isAlwaysOnTop()
{
	return always_on_top_;
}

void FxController::setAlwaysOnTop(bool always_on_top)
{
	always_on_top_ = always_on_top;
	settings_.setBool("always_on_top", always_on_top);
	main_window_->setAlwaysOnTop(always_on_top);
}

bool FxController::isLaunchOnStartup()
{
	HKEY hkey;
	RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &hkey);
	DWORD type;
	DWORD size = 0;
	RegQueryValueEx(hkey, L"FxSound", NULL, &type, NULL, &size);
	RegCloseKey(hkey);

	return size > 0;
}

void FxController::setLaunchOnStartup(bool launch_on_startup)
{
	wchar_t szPath[MAX_PATH];
	GetModuleFileName(NULL, szPath, MAX_PATH);
	HKEY hkey;
	RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hkey);

	if (launch_on_startup)
	{
		RegSetValueEx(hkey, L"FxSound", 0, REG_SZ, (BYTE*)szPath, sizeof(szPath));
	}
	else
	{
		RegDeleteValue(hkey, L"FxSound");
	}

	RegCloseKey(hkey);
}

void FxController::registerHotkeys()
{
	if (!hotkeys_registered_)
	{
		int mod;
		int vk;

		if (getHotkey(HK_CMD_ON_OFF, mod, vk))
		{
			if (isValidHotkey(mod, vk))
			{
				::RegisterHotKey(message_window_.getHandle(), CMD_ON_OFF, mod, vk);
			}
		}

		if (getHotkey(HK_CMD_OPEN_CLOSE, mod, vk))
		{
			if (isValidHotkey(mod, vk))
			{
				::RegisterHotKey(message_window_.getHandle(), CMD_OPEN_CLOSE, mod, vk);
			}

		}

		if (getHotkey(HK_CMD_NEXT_PRESET, mod, vk))
		{
			if (isValidHotkey(mod, vk))
			{
				::RegisterHotKey(message_window_.getHandle(), CMD_NEXT_PRESET, mod, vk);
			}

		}

		if (getHotkey(HK_CMD_PREVIOUS_PRESET, mod, vk))
		{
			if (isValidHotkey(mod, vk))
			{
				::RegisterHotKey(message_window_.getHandle(), CMD_PREVIOUS_PRESET, mod, vk);
			}

		}

		if (getHotkey(HK_CMD_NEXT_OUTPUT, mod, vk))
		{
			if (isValidHotkey(mod, vk))
			{
				::RegisterHotKey(message_window_.getHandle(), CMD_NEXT_OUTPUT, mod, vk);
			}
		}

		hotkeys_registered_ = true;
	}
}

void FxController::unregisterHotkeys()
{
	if (hotkeys_registered_)
	{
		::UnregisterHotKey(message_window_.getHandle(), CMD_ON_OFF);
		::UnregisterHotKey(message_window_.getHandle(), CMD_OPEN_CLOSE);
		::UnregisterHotKey(message_window_.getHandle(), CMD_NEXT_PRESET);
		::UnregisterHotKey(message_window_.getHandle(), CMD_PREVIOUS_PRESET);
		::UnregisterHotKey(message_window_.getHandle(), CMD_NEXT_OUTPUT);
		hotkeys_registered_ = false;
	}
}

void FxController::getSpectrumBandValues(Array<float>& band_values)
{
	float values[NUM_SPECTRUM_BANDS] = { 0 };

	dfx_dsp_.getSpectrumBandValues(values, NUM_SPECTRUM_BANDS);

	band_values.clearQuick();
	for (auto i = 0; i < NUM_SPECTRUM_BANDS; i++)
	{
		if (audio_process_on_)
		{
			band_values.set(i, values[i]);
		}
		else
		{
			band_values.set(i, 0.01);
		}
	}
}

String FxController::FormatString(const String& format, const String& arg)
{
	wchar_t buffer[1024];

	swprintf_s(buffer, format.toWideCharPointer(), arg.toWideCharPointer());

	return String(buffer);
}
