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

#include "Settings.h"
#include <Windows.h>
#include <wincrypt.h>

FxSound::Settings::Settings()
{
	PropertiesFile::Options options;
	const String properties_xml = R"(
		<PROPERTIES>
			<VALUE name="power" val="1"/>
			<VALUE name="hotkeys" val="1"/>
			<VALUE name="preset" val="General"/>
			<VALUE name="cmd_on_off" val="393297"/>
			<VALUE name="cmd_open_close" val="393285"/>
			<VALUE name="cmd_next_preset" val="393281"/>
			<VALUE name="cmd_previous_preset" val="393306"/>
			<VALUE name="cmd_change_output" val="393303"/>
		</PROPERTIES>
    )";

	options.applicationName = FxSound::APPLICATION_NAME;
	options.folderName = FxSound::SETTINGS_FOLDER;
	options.filenameSuffix = FxSound::SECURE_EXTN;
	options.doNotSave = false;
	app_secure_properties_.setStorageParameters(options);

	options.filenameSuffix = FxSound::SETTINGS_EXTN;
	app_user_properties_.setStorageParameters(options);
	user_settings_ = app_user_properties_.getUserSettings();

	options.doNotSave = true;	
	app_default_properties_.setStorageParameters(options);
	auto common_settings = app_default_properties_.getCommonSettings(false);
	if (common_settings == nullptr || common_settings->getAllProperties().size() == 0)
	{
		std::unique_ptr<XmlElement> default_properties = XmlDocument::parse(properties_xml);
		default_settings_.restoreFromXml(*default_properties);
		user_settings_->setFallbackPropertySet(&default_settings_);
	}
	else
	{
		default_settings_ = *common_settings;
		user_settings_->setFallbackPropertySet(&default_settings_);
	}
}

FxSound::Settings::~Settings()
{
}

String FxSound::Settings::getString(StringRef key) noexcept
{
	return user_settings_->getValue(key);
}

int FxSound::Settings::getInt(StringRef key, int default_value) noexcept
{
	return user_settings_->getIntValue(key, default_value);
}

double FxSound::Settings::getDouble(StringRef key) noexcept
{
	return user_settings_->getDoubleValue(key);
}

bool FxSound::Settings::getBool(StringRef key, bool default_value) noexcept
{
	return user_settings_->getBoolValue(key, default_value);
}

juce::var FxSound::Settings::getJson(juce::StringRef key) noexcept
{
	if (user_settings_ == nullptr)
		return {};

	auto json = user_settings_->getValue(key);

	return json.isNotEmpty() ? juce::JSON::parse(json) : juce::var{};
}

void FxSound::Settings::setString(StringRef key, String value, bool default) noexcept
{
    if (default)
    {
        default_settings_.setValue(key, value);
    }
    else
    {
        user_settings_->setValue(key, value);
    }	
}

void FxSound::Settings::setInt(StringRef key, int value, bool default) noexcept
{
    if (default)
    {
        default_settings_.setValue(key, value);
    }
    else
    {
        user_settings_->setValue(key, value);
    }
}

void FxSound::Settings::setDouble(StringRef key, double value, bool default) noexcept
{
    if (default)
    {
        default_settings_.setValue(key, value);
    }
    else
    {
        user_settings_->setValue(key, value);
    }
}

void FxSound::Settings::setBool(StringRef key, bool value, bool default) noexcept
{
    if (default)
    {
        default_settings_.setValue(key, value);
    }
    else
    {
        user_settings_->setValue(key, value);
    }
}

void FxSound::Settings::setJson(juce::StringRef key, const juce::var& json) noexcept
{
	user_settings_->setValue(key, juce::JSON::toString(json));
}