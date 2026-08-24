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
#include "codedefs.h"
#include "DfxKeyValueStore.h"

#ifdef _WIN32

/*
 * Windows: delegate to the existing registry helpers unchanged, so key
 * layout and behavior (and therefore existing installs' stored settings)
 * are unaffected by this abstraction.
 */
int PT_DECLSPEC dfxKvWriteString_Wide(int i_root, wchar_t *wcp_full_key_path, wchar_t *wcp_value)
{
	return regCreateKey_Wide(i_root, wcp_full_key_path, wcp_value);
}

int PT_DECLSPEC dfxKvReadString_Wide(int i_root, wchar_t *wcp_full_key_path, int *ip_key_exists,
												  wchar_t *wcp_value_out, unsigned long ul_buffer_len)
{
	return regReadKey_Wide(i_root, wcp_full_key_path, ip_key_exists, wcp_value_out, ul_buffer_len);
}

int PT_DECLSPEC dfxKvRecursiveDeleteFolder_Wide(int i_root, wchar_t *wcp_folder_path)
{
	return regRecursiveDeleteFolder_Wide(i_root, wcp_folder_path);
}

#else //_WIN32

/*
 * Every other platform: a flat "key=value" settings file, one entry per
 * line, keyed by the same backslash-delimited path strings the registry
 * used. Entries are namespaced by root (REG_CURRENT_USER/REG_LOCAL_MACHINE)
 * since there's no OS-level access-control distinction to map them to here.
 */

#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <cstring>
#include <string>
#include <map>
#include <sys/stat.h>
#include <sys/types.h>

namespace
{
	std::string WideToUtf8(const wchar_t *wcp_in)
	{
		std::string result;
		mbstate_t state;
		memset(&state, 0, sizeof(state));
		const wchar_t *cursor = wcp_in;
		size_t needed = wcsrtombs(nullptr, &cursor, 0, &state);
		if (needed == static_cast<size_t>(-1))
			return result;
		result.resize(needed);
		memset(&state, 0, sizeof(state));
		cursor = wcp_in;
		wcsrtombs(&result[0], &cursor, needed, &state);
		return result;
	}

	std::wstring Utf8ToWide(const std::string &in)
	{
		std::wstring result;
		mbstate_t state;
		memset(&state, 0, sizeof(state));
		const char *cursor = in.c_str();
		size_t needed = mbsrtowcs(nullptr, &cursor, 0, &state);
		if (needed == static_cast<size_t>(-1))
			return result;
		result.resize(needed);
		memset(&state, 0, sizeof(state));
		cursor = in.c_str();
		mbsrtowcs(&result[0], &cursor, needed, &state);
		return result;
	}

	/* Values stored here (numeric strings, filesystem paths) never legally
	 * contain '=' or a newline, so a single-line "key=value" format is
	 * sufficient without extra escaping. */
	std::string NamespacedKey(int i_root, const std::string &key_path_utf8)
	{
		char root_prefix[16];
		snprintf(root_prefix, sizeof(root_prefix), "%d:", i_root);
		return std::string(root_prefix) + key_path_utf8;
	}

	std::string SettingsFilePath()
	{
		const char *home = getenv("HOME");
		std::string base = (home != nullptr) ? home : ".";
#ifdef __APPLE__
		base += "/Library/Application Support/FxSound";
#else
		base += "/.fxsound";
#endif
		mkdir(base.c_str(), 0755);
		return base + "/dfx_settings.conf";
	}

	bool LoadAll(std::map<std::string, std::string> *out_entries)
	{
		FILE *fp = fopen(SettingsFilePath().c_str(), "r");
		if (fp == nullptr)
			return false;

		char line[4096];
		while (fgets(line, sizeof(line), fp) != nullptr)
		{
			std::string entry(line);
			while (!entry.empty() && (entry.back() == '\n' || entry.back() == '\r'))
				entry.pop_back();

			size_t sep = entry.find('=');
			if (sep == std::string::npos)
				continue;

			(*out_entries)[entry.substr(0, sep)] = entry.substr(sep + 1);
		}

		fclose(fp);
		return true;
	}

	bool SaveAll(const std::map<std::string, std::string> &entries)
	{
		FILE *fp = fopen(SettingsFilePath().c_str(), "w");
		if (fp == nullptr)
			return false;

		for (std::map<std::string, std::string>::const_iterator it = entries.begin();
			  it != entries.end(); ++it)
		{
			fprintf(fp, "%s=%s\n", it->first.c_str(), it->second.c_str());
		}

		fclose(fp);
		return true;
	}
}

int PT_DECLSPEC dfxKvWriteString_Wide(int i_root, wchar_t *wcp_full_key_path, wchar_t *wcp_value)
{
	if (wcp_full_key_path == nullptr || wcp_value == nullptr)
		return NOT_OKAY;

	std::map<std::string, std::string> entries;
	LoadAll(&entries);

	entries[NamespacedKey(i_root, WideToUtf8(wcp_full_key_path))] = WideToUtf8(wcp_value);

	if (!SaveAll(entries))
		return NOT_OKAY;

	return OKAY;
}

int PT_DECLSPEC dfxKvReadString_Wide(int i_root, wchar_t *wcp_full_key_path, int *ip_key_exists,
												  wchar_t *wcp_value_out, unsigned long ul_buffer_len)
{
	if (wcp_full_key_path == nullptr || ip_key_exists == nullptr || wcp_value_out == nullptr)
		return NOT_OKAY;

	*ip_key_exists = IS_FALSE;

	std::map<std::string, std::string> entries;
	LoadAll(&entries);

	std::map<std::string, std::string>::const_iterator it =
		entries.find(NamespacedKey(i_root, WideToUtf8(wcp_full_key_path)));
	if (it == entries.end())
		return OKAY;

	std::wstring value = Utf8ToWide(it->second);
	if (value.size() + 1 > ul_buffer_len)
		return NOT_OKAY;

	wcsncpy(wcp_value_out, value.c_str(), ul_buffer_len);
	*ip_key_exists = IS_TRUE;

	return OKAY;
}

int PT_DECLSPEC dfxKvRecursiveDeleteFolder_Wide(int i_root, wchar_t *wcp_folder_path)
{
	if (wcp_folder_path == nullptr)
		return NOT_OKAY;

	std::map<std::string, std::string> entries;
	LoadAll(&entries);

	std::string prefix = NamespacedKey(i_root, WideToUtf8(wcp_folder_path));
	for (std::map<std::string, std::string>::iterator it = entries.begin(); it != entries.end();)
	{
		if (it->first.compare(0, prefix.size(), prefix) == 0)
			it = entries.erase(it);
		else
			++it;
	}

	if (!SaveAll(entries))
		return NOT_OKAY;

	return OKAY;
}

#endif //_WIN32
