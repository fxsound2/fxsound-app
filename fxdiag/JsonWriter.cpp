/*
FxSound
Copyright (C) 2026  FxSound LLC

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

#include "fxdiag.h"
#include "JsonWriter.h"

std::wstring JsonEscape(const std::wstring& value)
{
	std::wstringstream escaped;

	for (wchar_t ch : value)
	{
		switch (ch)
		{
		case L'"':
			escaped << L"\\\"";
			break;
		case L'\\':
			escaped << L"\\\\";
			break;
		case L'\n':
			escaped << L"\\n";
			break;
		case L'\r':
			escaped << L"\\r";
			break;
		case L'\t':
			escaped << L"\\t";
			break;
		default:
			if (ch < 0x20)
			{
				escaped << L"\\u" << std::hex << std::setfill(L'0') << std::setw(4) << static_cast<int>(ch) << std::dec;
			}
			else
			{
				escaped << ch;
			}
		}
	}

	return escaped.str();
}

std::wstring JsonString(const std::wstring& key, const std::wstring& value)
{
	std::wstringstream json;
	json << L"\"" << key << L"\":\"" << JsonEscape(value) << L"\"";

	return json.str();
}

std::wstring JsonNumber(const std::wstring& key, double value)
{
	std::wstringstream json;
	json << L"\"" << key << L"\":" << std::fixed << std::setprecision(0) << value;

	return json.str();
}

std::wstring JsonNumber(const std::wstring& key, long long value)
{
	std::wstringstream json;
	json << L"\"" << key << L"\":" << value;

	return json.str();
}

std::wstring JsonBool(const std::wstring& key, bool value)
{
	std::wstringstream json;
	json << L"\"" << key << L"\":" << (value ? L"true" : L"false");

	return json.str();
}
