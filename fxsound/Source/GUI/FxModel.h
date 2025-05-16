/*
FxSound
Copyright (C) 2025  FxSound LLC

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

#ifndef FXMODEL_H
#define FXMODEL_H

#include <JuceHeader.h>
#include "AudioPassthru.h"

class FxModel final
{
public:
	enum Event { Notification=1, Subscription, PresetSelected, PresetListUpdated, PresetModified, OutputSelected, OutputListUpdated, OutputError, Other };
	enum PresetType { AppPreset=1, UserPreset=2 };

	struct AccountInfo final
	{
		String email;
		String subscription;
		bool trial;
	};

	struct AccountLog final
	{
		int64_t next_limited_playback_time;
		uint32_t playback_duration;
	};

	struct Preset final
	{
		String name;
		String path;
		PresetType type;
	};

	class Listener
	{
	public:
		Listener() = default;
		virtual ~Listener() = default;

		virtual void modelChanged(Event) {}
	};

	static FxModel& getModel()
	{
		static FxModel model;
		return model;
	}

	FxModel(const FxModel&) = delete;
	void operator=(const FxModel&) = delete;

	void initOutputs(const std::vector<SoundDevice>& output_devices);

	void initPresets(const Array<Preset>& presets);
	int  addPreset(const Preset& preset);
	void removePreset(int preset);
	int  selectPreset(const String& selected_preset, bool notify=true);
	void selectPreset(int selected_preset, bool notify=true);
	int getSelectedPreset() const;
	int getPresetCount() const;
	int getUserPresetCount() const;
	Preset getPreset(int preset) const;
	bool isPresetModified() const;
	void setPresetModified(bool preset_modified);
    bool isPresetNameValid(const String& preset_name);

	bool getPowerState()
	{
		return power_state_;
	}

	void setPowerState(bool power_state)
	{
		power_state_ = power_state;
		notifyListeners();
	}

	const StringArray& getOutputNames() const
	{
		return output_names_;
	}

	std::vector<SoundDevice> getOutputDevices() const
	{
		return output_devices_;
	}

	int getSelectedOutput()
	{
		return selected_output_;
	}

	void setSelectedOutput(int selected_output, const SoundDevice& sound_device, bool notify=true)
	{
		selected_output_ = selected_output;
		selected_output_device_ = sound_device;
		if (notify)
		{
			notifyListeners(Event::OutputSelected);
		}
	}

	bool isMonoOutputSelected()
	{
		return selected_output_device_.deviceNumChannel < 2;
	}

    void notifyOutputError()
    {
        notifyListeners(Event::OutputError);
    }

	AccountInfo getAccountInfo() const
	{
		return account_info_;
	}

	void setEmail(String email)
	{
		account_info_.email = email;
	}

	void setSubscription(String subscription)
	{
		account_info_.subscription = subscription;
		notifyListeners(Event::Subscription);
	}

	AccountLog getAccountLog() const
	{
		return account_log_;
	}

	void setAccountLog(const AccountLog& account_log)
	{
		account_log_ = account_log;
	}

	void setTrial(bool trial)
	{
		account_info_.trial = trial;
	}

	bool getHotkeySupport()
	{
		return hotkey_support_;
	}

	void setHotkeySupport(bool hotkey_support)
	{
		hotkey_support_ = hotkey_support;
	}

	bool isMenuClicked()
	{
		return menu_clicked_;
	}

	void setMenuClicked(bool clicked)
	{
		menu_clicked_ = clicked;
	}

	int getLanguage()
	{
		return language_;
	}

	void setLanguage(int language)
	{
		language_ = language;
	}

	bool getDebugLogging()
	{
		return debug_logging_;
	}

	void setDebugLogging(bool debug_logging)
	{
		debug_logging_ = debug_logging;
	}
	
	void pushMessage(String message, std::pair<String, String> link = {})
	{
		message_ = message;
		message_link_ = link;
		notifyListeners(Event::Notification);
	}

	void popMessage(String& message, std::pair<String, String>& link)
	{
		message = message_;
		link = message_link_;
		message_.clear();
		message_link_ = { "", "" };
	}

	void addListener(Listener* l) { listeners_.add(l); }
	void removeListener(Listener* l) { listeners_.remove(l); }
	void notifyListeners(Event model_event = Event::Other);

private:
	FxModel();

	bool power_state_;
	Array<Preset> presets_;
	bool preset_modified_;
	StringArray output_names_;
	int selected_preset_;
	int selected_output_;
    bool output_disconnected_;
	AccountInfo account_info_;
	AccountLog account_log_;
	bool hotkey_support_;
	bool menu_clicked_;
	int language_;
	bool debug_logging_;
	std::vector<SoundDevice> output_devices_;
	SoundDevice selected_output_device_;

	String message_;
	std::pair<String, String> message_link_;

	ListenerList<Listener> listeners_;
};

#endif
