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

#ifndef FXSOUND_SETTINGS_H
#define FXSOUND_SETTINGS_H

#include <JuceHeader.h>

namespace FxSound
{
	constexpr wchar_t APPLICATION_NAME[] = L"FxSound";
	constexpr wchar_t SETTINGS_FOLDER[] = L"FxSound";
	constexpr wchar_t SETTINGS_EXTN[] = L"settings";
	constexpr wchar_t SECURE_EXTN[] = L"secure";

	class Settings final
	{
	public:
		Settings();
		~Settings();

		String getString(StringRef key) noexcept;
		int    getInt(StringRef key) noexcept;
		double getDouble(StringRef key) noexcept;
		bool   getBool(StringRef key) noexcept;

		void setString(StringRef key, String value, bool default=false) noexcept;
		void setInt(StringRef key, int value, bool default = false) noexcept;
		void setDouble(StringRef key, double value, bool default = false) noexcept;
		void setBool(StringRef key, bool value, bool default = false) noexcept;

		String getSecure(String key);
		void   setSecure(String key, String value);

		static bool isAdminUser();

	private:
		ApplicationProperties app_default_properties_;
		ApplicationProperties app_user_properties_;
		ApplicationProperties app_secure_properties_;
		PropertySet default_settings_;
		PropertiesFile* user_settings_;
		PropertiesFile* secure_settings_;
	};
}

#endif