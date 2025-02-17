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
	secure_settings_ = app_secure_properties_.getUserSettings();

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

int FxSound::Settings::getInt(StringRef key) noexcept
{
	return user_settings_->getIntValue(key);
}

double FxSound::Settings::getDouble(StringRef key) noexcept
{
	return user_settings_->getDoubleValue(key);
}

bool FxSound::Settings::getBool(StringRef key) noexcept
{
	return user_settings_->getBoolValue(key);
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

String FxSound::Settings::getSecure(String key)
{
	DATA_BLOB data_in;
	DATA_BLOB data_out;

	MemoryOutputStream stream;
	auto encoded_value = secure_settings_->getValue(key);
	if (Base64::convertFromBase64(stream, encoded_value))
	{
		data_in.pbData = (BYTE*) stream.getData();
		data_in.cbData = stream.getDataSize();
		if (CryptUnprotectData(&data_in, NULL, NULL, NULL, NULL, 0, &data_out))
		{
			String value = (char*) data_out.pbData;
			return value;
		}
	}
	
	return { "" };
}

void FxSound::Settings::setSecure(String key, String value)
{
	DATA_BLOB data_in;
	DATA_BLOB data_out;
	DWORD flags = 0;

	data_in.pbData = (BYTE*) value.toRawUTF8();
	data_in.cbData = value.length() + 1;

	if (isAdminUser())
	{
		flags = CRYPTPROTECT_LOCAL_MACHINE;
	}
	if (CryptProtectData(&data_in, NULL, NULL, NULL, NULL, flags, &data_out))
	{
		auto encoded_value = Base64::toBase64(data_out.pbData, data_out.cbData);
		secure_settings_->setValue(key, encoded_value);
	}

    if (secure_settings_->needsToBeSaved())
    {
        secure_settings_->save();
    }
}

bool FxSound::Settings::isAdminUser()
{
	BOOL ret;
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	ret = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup);
	if (ret)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &ret))
		{
			ret = FALSE;
		}
		FreeSid(AdministratorsGroup);
	}

	if (ret)
	{
		return true;
	}
	else
	{
		return false;
	}
}